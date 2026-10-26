# DroidDeck + Mavk — experimental developer prototype

This fork keeps DroidDeck’s normal Android UI, Steam, ARM64 Proton/Wine/FEX and
Adreno/Turnip support. It adds an optional Mali Vulkan compatibility backend and
AHardwareBuffer presentation. Tested target: stock Galaxy Tab S11 / Mali-G925,
Android16, without root or system modifications.

**Reviewer guide:** [components, reproduction and recording](docs/MAVK_DEVELOPER_REVIEW.md).
**Try the APK:** [install and play through the UI](docs/TRY_TEST_APK.md) — no
per-game commands or separate Mavk library installation.
Mavk Core/Compatibility and the MIT WSI fork live in the separate Mavk repository;
Android session/compositor integration belongs here. Original licenses remain intact.

## Actual results and limitations

| Scenario | Evidence |
| --- | --- |
| HL2,1920×1080, nearly maximum settings | User-reported10–30FPS, usually20–30; actual gameplay |
| HL2,1920×1080, low settings | User-reported30–50FPS, mostly stable and playable |
| HL2,1920×1080, low settings with medium objects/textures | Latest user report:30+FPS, stable60 in some areas; scene-dependent |
| HL2 rare hitches | Characters sometimes pause with repeated sound; cause not localized |
| Muck moving menu | About34–35 displayed game buffers/s, AHB/copy0 |
| Muck heavy gameplay | About12–13 in one scene;9.1 in another view at1600p and1200p |
| Final same-scene Muck shader A/B |12.5 /12.7 /12.3 displayed buffers/s; no material gain |
| ARM64/AMD64 D3D11 consumer | Rendered;1920→1280→1920 ResizeBuffers and CoreZero checks passed |
| SHM fallback | Actual moving D3D11 stream,598 copied buffers/10s |
| DDNet native Linux/FEX | Loader failure before Vulkan; no gameplay PASS |
| Adreno / other Mali | Original paths retained; Mavk/AHB portability not hardware-proven |

HL2 FPS above is the user’s gameplay observation, not an independent captured
benchmark. Android displayed buffers, Present calls and unique gameplay frames
are different metrics. The device/game resolution was independently confirmed
as1920×1080 from D3D9 and Vulkan swapchain parameters.

In-game resolution changes can crash HL2 and leave Steam unresponsive. Choose
the desired display mode before session start. Occasional diagonal black artifacts
were reported in earlier tests; a universal fix is not established. GPU-query/CSE
experiments are disabled. No stable60FPS or complete Vulkan CTS/upstream DXVK
compatibility claim is made. Sarek’s diagnostic feature admission is experimental.

## Architecture and ordinary game launch

Steam → ARM64 Proton/Wine/FEX → experimental DXVK-Sarek → Mavk Compatibility →
libhybris → stock Android Vulkan/Mali. Native RGBA AHB presentation avoids CPU
framebuffer copies; BGRA needs one GPU conversion blit. Acquire/release fences
manage ownership. X11/SHM remains the rollback.

Select Mavk and optional Mavk AHB once in Launcher settings, then start Steam
and use its ordinary Play buttons. The new package contains ARM64X D3D11 and
PE32 D3D9/DXGI. The opted-in profile prefers Windows/Proton for x86 dual-platform
applications, avoiding the separate broken native Linux/FEX default. Explicit
compatibility-tool choices and native ARM64 applications remain respected.
PE32 D3D11 is not implemented. Standard Adreno/DXVK selections are not replaced.

The packaged route was checked on the device after recording: standard Steam
launch requests started HL2 (Full HD menu/first train scene) and Muck (moving
menu) using the packaged runtime, without private PE/layer paths. The diagnostic
focus observer does not recognize these private-Xwayland windows; that timeout
is not a game-launch failure. See the build checkpoint below.

## Build and reproducibility

Standard full upstream runtime preparation is `tools/build_local.sh` and needs
its documented Docker/cross-compilation dependencies. External components retain
pinned versions. A faster developer build may reuse checksum-verified prepared
runtime inputs; that is **not** rebuilding every third-party dependency from source.

The tested Windows app build uses JDK17+, Android SDK36/build-tools36.0.0,
NDK28.2.13676358 and Linux/WSL for runtime verification:

```sh
./gradlew :app:stageSessionScripts -PmavkExperimental=true -PmavkCompileSdk=36 -PmavkBuildTools=36.0.0 -PndkVersion=28.2.13676358 -PskipRust=true
python3 tools/release/runtime_inputs.py record
./gradlew :app:assembleDebug -PmavkExperimental=true -PmavkCompileSdk=36 -PmavkBuildTools=36.0.0 -PndkVersion=28.2.13676358 -PskipRust=true
python3 tools/release/runtime_inputs.py apk app/build/outputs/apk/debug/app-debug.apk
```

Prepare runtime inputs first; `record` attests existing dependencies, it does not
fetch/build them. On Windows use `gradlew.bat` and `-PmavkWslVerify=true`; run the
Python attestation inside WSL with zstd available. `skipRust` preserves the tested
Steam-focused build; optional Rust store downloaders are not included.
The `.mavk` package suffix preserves the stock application.

Runtime ZIP/version/manifests must change together. The packaged game policy
must match the checked-in source, not a private device directory. Never publish
Steam prefixes, account data, signing secrets or unsanitized device logs.

## Publication status

Local source/APK preparation only: no push or publication. The new API runtime is
`8213c6709ef42a73cbd32c1e48aadad49788800a8552fe83f802fa6472ba08cb`.
A normal Gradle experimental APK builds from a clean checkout with pinned
prebuilt dependencies and was installed/tested. See
[build checkpoint](docs/BUILD_CHECKPOINT_2026-10-26.md) for hashes, device
observations and the non-green broad Windows-host test suite.

DroidDeck is GPLv3. Mavk’s own code is Apache2 except marked derivatives; WSI is
MIT and the derived Proton wrapper remains GPLv3. Sarek, Mesa/Zink, libhybris,
Openbox, SPIRV-Tools and other components keep their own licenses. Notices are
included, but complete corresponding-source coverage for all prebuilt runtime
components remains a binary-distribution gate. Source review can proceed without
pretending this is a fully certified/conformant release.
