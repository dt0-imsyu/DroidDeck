# DroidDeck + Mavk — experimental developer prototype

This is a fork of DroidDeck, not a replacement Steam client or a Mali kernel
driver. The original README/LICENSE remain intact. Primary target: stock
Galaxy Tab S11 / Mali-G925, without root or system modifications.

## Architecture

Existing Steam → ARM64 Proton/Wine/FEX → experimental DXVK-Sarek → Mavk
compatibility layer → libhybris → stock Android Vulkan/Mali.

Presentation: optional XCB-to-Wayland fullscreen proxy → AHardwareBuffer with
explicit acquire/release fences → existing DroidDeck SurfaceControl layer.
Native RGBA is direct; BGRA requires one GPU conversion blit, not CPU readback.
The source for the WSI bridge and reusable Android interfaces is maintained in
the separate Mavk repository. Android lifecycle/selection belongs to this fork.

The normal Adreno/Turnip and standard DXVK selections remain available. AHB is
off by default; use Settings → Launcher → Mavk experimental AHB, then restart
the session. SHM remains the explicit rollback. No device-wide library override.
Adreno AHB portability is not yet hardware-tested.

## Actual evidence

| Scenario | Result |
|---|---|
| Muck moving menu,1920×1200 | AHB direct layer,29.65 displayed buffers/s over20.27s |
| Same menu,2560×1600 | AHB direct layer,29.94 displayed buffers/s over20.24s |
| Later1200p run |349 GPU=349zero-copy=349on-screen buffers/10s |
| ARM64/AMD64 Wine D3D11 consumer | Rendered; standard ResizeBuffers passed |
| Scaled pointer candidate | Native D3D11 received button/key; X server coordinates checked |
| Current Muck animated menu | ~34–35 displayed buffers/s, actual AHB, copy0 |
| Same heavy Muck view,1600p→1200p |9.1→9.1 displayed buffers/s; not a resolution-only bottleneck |
| Half-Life2 / heavier gameplay | HL2 not installed in the sole current library; no large download started |
| DDNet Linux/FEX | Startup fails before Vulkan; no current gameplay PASS |

Steam/game use is authorized now. These are not60unique gameFPS, a complete benchmark, or proof of gaming parity
with native Android/Turnip. No specific30FPS root cause is established yet.
The same-menu resolution comparison does not support a fill-rate-only diagnosis.
Existing diagnostic Sarek feature admissions are experimental, not Vulkan CTS
conformance or complete upstream DXVK compatibility.

## Build / installation status

Use the standard DroidDeck Gradle build with `-PmavkExperimental=true`; the
experimental package suffix is `.mavk`, preserving the stock app. The Android
SDK/NDK and normal DroidDeck runtime build prerequisites are still required.
Mavk runtime archives are content-addressed and manifest-checked at installation.
The current local runtime/version assets are build inputs, not source releases.

The independent Mavk bridge source is under `third_party/wsi-layer` with pinned
provenance; Android-neutral helpers are under `adapters/android`. Do not copy
unversioned libraries into an installed runtime. Build/package tooling must
update the archive manifest, library receipt and version hash together.

Latest installed source-built experimental APK:
Mavk/recovery/2026-10-25/DroidDeck-Mavk-fex-boundary-demo.apk,
SHA2566c8680f51db2db0551675b84f58ae09e4535ffb5326c0fd54535e549aa915b88.
Built, APK-attested, installed; normal Steam and animated Muck AHB menu pass.
The portable-FEX child environment now honors DroidDeck's server directory.
Native DDNet still has a later loader blocker, also reproduced by x86-64 true;
see [exact boundary and rollback](docs/MAVK_NATIVE_FEX_2026-10-25.md).
Earlier notices APK443c300b is retained for rollback. Vulkan/WSI payload unchanged.
Runtime3bd128fb adds exact package notices and file attribution; executable bytes
are unchanged from the previous source-built runtime. All230 Openbox runtime files
matched retained package archives; this does not establish complete corresponding
source for public binary distribution. The previous675e9cb6 source APK is retained.
Normal Gradle assembleDebug rebuilt Android Kotlin/Java/native/DEX/resources and
verified every declared runtime input against the APK.70 missing prebuilt runtime
inputs (100MB) were imported only after matching both a pinned preparation
manifest and preserved APK. It is NOT a rebuild of all third-party dependencies.
Optional Rust store downloader remains skipped, as absent in the previous working
APK; this is a Steam-focused developer build, not a complete store/release build.

AMD64 Win32/Wine consumer on that actual APK passed1920→1280→1920 ResizeBuffers,
S_OK/11_0,exit0,CoreLiveZero,gaps0. No Steam or game prefix was started/changed.
This is integration evidence, not a gameplay FPS claim.

Live gameplay profiling places the main delay before Android presentation:
compositor scenes~0.6ms, no CPU copy/pool drops, no eight-core CPU saturation.
GPU query spans24.9/55.7ms include synchronization and are not shader-only times.
Mali errors/black tiles occurred during GPU profiling; it is disabled. Diagonal
artifacts were also reported with the CSE candidate without Mali errors, so the
artifact root cause is still OPEN. CSE and temporary per-game profiling overrides
were restored. Thermal Status3 was observed; hot/cold FPS is not a fair A/B.
Never advertise this APK as a stable60FPS game build or a permanent artifact fix.

Previous rollback APK: Mavk/recovery/2026-10-25/DroidDeck-Mavk-demo2.apk,
SHA256360c47655ba1acff64eae9ef68d69fb0c4f9c49376fcae21f9ef939103f36fdb.
It is an asset/native rollup of the preserved known-good APK, not a clean full
source APK. Complete Kotlin compilation and the native compositor target
(79 clean build steps from this C: checkout) passed separately. Runtime33c0cfe7
and WSIb1bad07f are manifest-checked.

Actual scaled input check: output2000,1200 maps to X client1820,1108; no stuck
buttons. Fullscreen AHB input is selected before the underlying X11 desktop.
Policy AHB↔SHM reversal and invalid-geometry fallback passed. Captured relative
mouse input and real game input after this change remain unverified.

Tests (Linux/WSL):81 game-environment and30 controller cases PASS. Mavk packaging:
17PASS. Run with `python3 -m unittest discover -s tools/tests -p test_game_environment.py`
and similarly test_droiddeckctl.py. Tests requiring fcntl/executable shebangs are
Linux tests, not Windows-native Python tests.

Final complete release build and full game regression are still pending. This
document does not label the prototype release-ready before those checks finish.

## Licensing / publication checklist

DroidDeck remains GPLv3; retain its notices and provide corresponding source for
the distributed build. Mavk's own code is Apache2 unless individually marked;
the WSI layer is MIT. DXVK-Sarek, Mesa/Zink, libhybris, Openbox, SPIRV-Tools,
Vulkan headers and other payload components retain their respective licenses.
Check Mavk/THIRD_PARTY_NOTICES.md and the payload license directory.

Before distributing APKs: review exact binary/source revisions, publish required
corresponding sources/patches and notices, choose a personal signing key, and
rerun device/game tests. Never publish private signing keys, Steam data, prefixes,
or device logs containing account information. Runtime binaries are ignored by
this source checkout until the source-compliance bundle is finalized.

No push or public repository creation is performed by this preparation.
