# Rebuilding the local review APK

The source implementation is committed. This optional developer seed reuses
prebuilt dependencies; it is not a complete corresponding-source distribution.
Do not publish these binaries until the license/source gate in README.MAVK.md
is closed. No Steam installation, prefix, account or device preferences are in
the seed.

Build inputs for the checked `ea9c18b` APK:

| Input | SHA256 |
| --- | --- |
| `DroidDeck-Mavk-demo.apk` | `36d5efc4749ba6ee22726d6be26f9be04f757a24d2ef8f1d4fa4ed7caf59c6f0` |
| `DroidDeck-pinned-dependency-seed.zip` | `2e3f1d06bc35d58b9190b20408df7abb44b977fc34031dc5b5f6f95acceaf7d6` |
| seed `runtime-inputs.json` | `7d43b9228345b11df1d99a91bd22838afc07f9412a71f7b34a099ebc1af25b89` |
| Mavk `runtime.zip` | `8213c6709ef42a73cbd32c1e48aadad49788800a8552fe83f802fa6472ba08cb` |

1. Check out `ea9c18b` (later documentation commits do not change runtime code).
2. Verify the seed/archive hashes and extract the seed outside the checkout.
3. With Python 3.11+, import only the pinned missing dependency inputs:

```sh
python3 tools/release/import_runtime_seed.py /path/to/extracted-seed \
  /path/to/extracted-seed/runtime-inputs.json /path/to/DroidDeck-Mavk-demo.apk \
  --manifest-sha256 7d43b9228345b11df1d99a91bd22838afc07f9412a71f7b34a099ebc1af25b89 \
  --apk-sha256 36d5efc4749ba6ee22726d6be26f9be04f757a24d2ef8f1d4fa4ed7caf59c6f0 --apply
```

The importer rejects mismatches and deliberately never replaces source files.
Copy only the separately hash-checked seed `app/src/main/assets/mavk/runtime.zip`
to that same path in the checkout; the tracked version must match. Configure
the Android SDK/JDK locally. Follow the stage/record/assemble/apk commands in
README.MAVK.md. On Windows, quote each `-P...` argument in PowerShell so dotted
version values reach Gradle intact; use WSL Python with zstd for attestation.

The app/native compositor rebuild was verified in a new checkout with JDK,
SDK 36, build-tools 36.0.0 and NDK 28.2.13676358. Gradle may reuse its dependency
cache. APK bytes need not be identical across local signing/build environments;
the packaged runtime and attested dependencies must match.

Mavk Core separately builds from its own checkout using the documented CMake
target. Exact PE32 Sarek build source is retained in the local review artifacts
as `DXVK-Sarek-PE32-build-source.zip`; it includes the original notices and
subprojects. This does not close source obligations for other bundled packages.
