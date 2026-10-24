import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from zipfile import ZipFile

RELEASE = Path(__file__).resolve().parents[1] / 'release'
sys.path.insert(0, str(RELEASE))
spec = importlib.util.spec_from_file_location('runtime_seed', RELEASE / 'import_runtime_seed.py')
seed_tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(seed_tool)


class RuntimeSeedTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.base = Path(self.tmp.name)
        self.root, self.seed = self.base/'checkout', self.base/'seed'
        self.root.mkdir()
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        self.name = 'assets/linuxfs/test-prebuilt.so'
        self.relative = 'app/src/main/' + self.name
        source = self.seed/self.relative
        source.parent.mkdir(parents=True)
        source.write_bytes(b'pinned bytes')
        self.manifest, self.apk = self.base/'inputs.json', self.base/'baseline.apk'
        self.manifest.write_text(json.dumps(dict(version=1, outputs={self.name: dict(
            path=self.relative, sha256=hashlib.sha256(source.read_bytes()).hexdigest())})))
        with ZipFile(self.apk, 'w') as archive:
            archive.writestr(self.name, source.read_bytes())

    def run_import(self, **changes):
        args = dict(root=self.root, seed=self.seed, manifest=self.manifest,
                    manifest_hash=seed_tool.digest(self.manifest), apk=self.apk,
                    apk_hash=seed_tool.digest(self.apk))
        args.update(changes)
        return seed_tool.import_seed(**args)

    def test_audit_is_read_only_then_exclusive_copy_and_repeat(self):
        self.assertEqual(self.run_import()['missing_inputs'], 1)
        self.assertFalse((self.root/self.relative).exists())
        self.run_import(apply=True)
        self.assertEqual((self.root/self.relative).read_bytes(), b'pinned bytes')
        self.assertEqual(self.run_import(apply=True)['retained_inputs'], 1)

    def test_both_pins_required(self):
        for name in ('manifest_hash', 'apk_hash'):
            with self.assertRaises(ValueError):
                self.run_import(**{name: '0'*64}, apply=True)
        self.assertFalse((self.root/self.relative).exists())

    def test_mismatched_seed_and_existing_files_are_preserved(self):
        target = self.root/self.relative
        target.parent.mkdir(parents=True)
        target.write_bytes(b'newer own file')
        with self.assertRaises(ValueError):
            self.run_import(apply=True)
        self.assertEqual(target.read_bytes(), b'newer own file')
        (self.seed/self.relative).write_bytes(b'unpinned bytes')
        with self.assertRaises(ValueError):
            self.run_import()

    def test_tracked_inputs_are_not_replaced(self):
        target = self.root/self.relative
        target.parent.mkdir(parents=True)
        target.write_bytes(b'maintained source')
        subprocess.run(['git', '-C', str(self.root), 'add', self.relative], check=True)
        self.assertEqual(self.run_import(apply=True)['missing_inputs'], 0)
        self.assertEqual(target.read_bytes(), b'maintained source')

    def test_paths_cannot_escape_or_target_source(self):
        for path in ('../outside', 'app/src/main/java/Bad.java', '/absolute', 'app\\src\\main\\assets\\x'):
            with self.assertRaises(ValueError):
                seed_tool.destination(self.name, dict(path=path))


if __name__ == '__main__':
    unittest.main()
