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
| Half-Life2 / heavier gameplay | Pending; Steam currently reserved by owner |

These are not60unique gameFPS, a gameplay benchmark, or proof of gaming parity
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

Installed local experimental APK: Mavk/recovery/2026-10-25/DroidDeck-Mavk-demo2.apk,
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

Final clean release build and full game regression are still pending. This
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
