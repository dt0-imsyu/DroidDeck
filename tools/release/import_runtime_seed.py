"""Import missing pinned runtime build inputs, never source files or user data.

This reuses prebuilt dependencies; it is not a corresponding-source rebuild.
Every imported byte must match both a pinned preparation manifest and APK.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import subprocess
from zipfile import ZipFile

from check_session_assets import source_assets


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def destination(name, record):
    logical, relative = PurePosixPath(name), PurePosixPath(record['path'])
    for value in (logical, relative):
        if value.is_absolute() or any(p in ('', '.', '..') for p in value.parts) or '\\' in str(value):
            raise ValueError('Unsafe seed path')
    if name.startswith('assets/'):
        valid = ('app/src/main/' + name, 'app/build/prepared-assets/' + name[7:])
    elif name in ('lib/arm64-v8a/libproot.so', 'lib/arm64-v8a/libproot-loader.so',
                  'lib/arm64-v8a/libdirectaudiorelay.so'):
        valid = ('app/src/main/jniLibs/' + name[4:],)
    else:
        raise ValueError('Unexpected runtime member: ' + name)
    if str(relative) not in valid:
        raise ValueError('Member/destination mismatch: ' + name)
    return str(relative)


def import_seed(root, seed, manifest, manifest_hash, apk, apk_hash, apply=False):
    if digest(manifest) != manifest_hash or digest(apk) != apk_hash:
        raise ValueError('Pinned manifest/APK mismatch')
    data = json.loads(manifest.read_text())
    if data.get('version') != 1 or not isinstance(data.get('outputs'), dict) or len(data['outputs']) > 512:
        raise ValueError('Preparation manifest schema/budget')
    source_names = set(source_assets().values())
    tracked = set(subprocess.check_output(['git', 'ls-files', '-z'], cwd=root).decode().split('\0'))
    planned, retained, total = [], 0, 0
    root, seed = root.resolve(), seed.resolve()
    with ZipFile(apk) as archive:
        if len(archive.namelist()) != len(set(archive.namelist())):
            raise ValueError('Duplicate APK members')
        for name, record in data['outputs'].items():
            relative = destination(name, record)
            if name.startswith('assets/mavk/') or name in source_names or relative in tracked:
                continue
            original, target = seed / relative, root / relative
            if not original.resolve().is_relative_to(seed) or not target.resolve().is_relative_to(root):
                raise ValueError('Seed mapping escapes checkout')
            if original.is_symlink() or target.is_symlink():
                raise ValueError('No seed symlinks')
            size = original.stat().st_size
            total += size
            if size > 128*1024*1024 or total > 384*1024*1024:
                raise ValueError('Runtime seed size budget')
            expected = record['sha256']
            if digest(original) != expected or hashlib.sha256(archive.read(name)).hexdigest() != expected:
                raise ValueError('Seed differs from pinned APK: ' + name)
            if target.exists():
                if digest(target) != expected:
                    raise ValueError('Preserve different existing input: ' + relative)
                retained += 1
            else:
                planned.append((original, target, expected))
    if apply:
        for original, target, expected in planned:
            target.parent.mkdir(parents=True, exist_ok=True)
            # Exclusive creation prevents a concurrent preparation from being overwritten.
            with original.open('rb') as source, target.open('xb') as output:
                shutil.copyfileobj(source, output)
            if digest(target) != expected:
                raise ValueError('Copied input checksum mismatch')
    return dict(applied=apply, missing_inputs=len(planned), retained_inputs=retained,
                checked_bytes=total, source_assets_excluded=len(source_names),
                manifest_sha256=manifest_hash, apk_sha256=apk_hash,
                scope='Pinned prebuilt dependencies; not a full source rebuild')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('seed', 'manifest', 'apk'):
        p.add_argument(name, type=Path)
    p.add_argument('--manifest-sha256', required=True)
    p.add_argument('--apk-sha256', required=True)
    p.add_argument('--apply', action='store_true')
    a = p.parse_args()
    print(json.dumps(import_seed(Path(__file__).resolve().parents[2], a.seed, a.manifest,
                                 a.manifest_sha256, a.apk, a.apk_sha256, a.apply), indent=2))
