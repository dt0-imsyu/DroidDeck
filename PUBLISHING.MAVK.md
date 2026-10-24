# Developer handoff / publication gates

This is a local experimental prototype, not a conformant Vulkan implementation
or a proven60FPS game release. No push was performed.

## Local source trees

- DroidDeck fork: this repository, branch `mavk-demo`, upstream base
  `0916a538aff4c08f799a4bd9fa17dd3c98b01e99`.
- Mavk sibling project: `../win11ontab/Mavk`; reusable Android adapter,
  compatibility code and MIT WSI snapshot with exact source provenance.
- The current APK/rollback are local artifacts, not tracked game/runtime data.
  See README.MAVK.md for hashes and the exact measured limitations.

## Reproduce checks

Use Linux/WSL for Python tests requiring fcntl, Unix executable permissions and
shell scripts. Preserve LF line endings. Run the81 game-environment and30
controller tests listed in README.MAVK.md. Mavk has17 runtime packaging tests.
Android `FrameLoopTest` has4 passing lifecycle cases.

Android Kotlin source compile was verified with SDK36, build-tools36.0.0,
NDK28.2.13676358 and the experimental package suffix. Example on Windows:

    gradlew.bat :app:compileDebugKotlin -PmavkExperimental=true -PmavkCompileSdk=36 -PmavkBuildTools=36.0.0 -PndkVersion=28.2.13676358 -PskipRust=true

Quote dotted property arguments in PowerShell. `skipRust` is sufficient for this
compile check only: it is NOT a way to produce a complete release APK. Install
the normal upstream Rust/cargo-ndk and runtime preparation prerequisites for a
clean full build. Do not disable `verifyRuntimeInputs` to conceal missing assets.

The installed candidate was produced by pinned APK asset/native rollups, verified
to retain unrelated entries. It is not claimed to be a clean Gradle release.
Changing the signing identity requires an explicit migration/reinstall plan;
do not uninstall the user's existing app/prefixes merely to test signing.

### Subsequent ordinary Gradle build

The latest source-demo APK675e9cb6 supersedes the rollup for developer checks.
`assembleDebug` passed with `verifyRuntimeInputs` enabled; `runtime_inputs.py apk`
then verified the actual packaged bytes. Android code/native targets are rebuilt;
external dependencies remain pinned prebuilt inputs. Rust download engines are
still absent as in the old baseline, not silently claimed working.

`tools/release/import_runtime_seed.py` provides a bounded offline preparation
path when an existing dependency cache is available. Required arguments: seed
checkout, preparation manifest, preserved APK, both SHA256 pins. Default is a
read-only audit; `--apply` creates only missing generated inputs, never tracked
sources or a different existing file. Every byte must match BOTH pins. Current
receipt: manifestfde81948ea86fb37f39105b9f3a5503df211f9c08feb46abf9c32d0461cd40f9,
seed APK360c47655ba1acff64eae9ef68d69fb0c4f9c49376fcae21f9ef939103f36fdb,
70inputs/100138173bytes. Run5 helper regressions with test_runtime_seed.py.

After preparation, run the normal `runtime_inputs.py record` and `verify`; never
disable package verification. Unix recipe hashes require LF byte-exact source
files, now enforced for tools/ by .gitattributes. Merely changing release pins to
hide CRLF differences would be wrong; the original pins are retained unchanged.

Local assembly uses the compile command above, replacing compileDebugKotlin with
assembleDebug and adding -PmavkWslVerify=true. The current Windows helper uses a
read-only local zstd tools cache; other machines should use normal Linux build
prerequisites. No cache, APK, installed container, Steam data or signing key is
part of the source commits.

## Before public source/APK release

1. Review the local commits and configure a NEW personal fork remote. Keep
   `upstream` pointing at Droid-Deck/DroidDeck; never push changes to it.
2. Complete game regressions when Steam is free: Muck real gameplay at matched
   settings/thermal state, installed HL2 if available, then one practical heavier
   title. Record actual route, GPU/CPU/frametime and displayed unique-frame limits.
3. Check captured mouse/touch/wheel and clean launch resolution changes. A second
   scaled absolute pointer point now passes; this is not complete input coverage.
4. Assemble exact corresponding sources/patches and full notices for every runtime
   payload dependency (especially GPL/LGPL ones). Preserve DroidDeck GPLv3,
   individually licensed Mavk adapters, MIT WSI/protocol and other upstream notices.
   Ignored runtime.zip and APK files are not a compliance source package.
5. Build/sign and smoke-test the final release with the intended personal key.
   Do not publish keys, Steam credentials/data, prefixes, device dumps or caches.
6. Only after these gates should the owner choose repository visibility and upload
   sources/releases. Current preparation deliberately makes no publication call.

## Rollback

Settings → Launcher: turn off experimental AHB, restart session to use X11/SHM.
Disable Mavk hardware Steam UI separately if necessary. Preserve the stock
Adreno/Turnip/default DXVK route; no global replacement is required. The old
known-good APK is retained locally; reinstall with `adb install -r`, not uninstall.
