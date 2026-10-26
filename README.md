# DroidDeck + Mavk — experimental Mali fork

**Steam and Windows games on a stock Galaxy Tab S11 / Mali-G925, with an
experimental Vulkan compatibility backend and AHardwareBuffer presentation.**

[Русский](README.ru.md) · [Download test APK](https://github.com/dt0-imsyu/DroidDeck/releases) · [Install and play](docs/TRY_TEST_APK.md) · [Developer review](docs/MAVK_DEVELOPER_REVIEW.md) · [Upstream DroidDeck](https://github.com/Droid-Deck/DroidDeck)

This is an independent experimental fork by **dt0-imsyu**, not an official
DroidDeck release. It retains DroidDeck's interface, Steam, ARM64 Proton/Wine/FEX
and original supported Adreno/Turnip path. No root, bootloader unlock, firmware
change or SELinux modification is required. The primary tested target is stock
Android 16 on Tab S11; support for other GPUs is not established by these tests.

## Install and try — no per-game commands

1. Download **DroidDeck-Mavk-demo.apk** from this fork's **Releases** and install
   it through Android. The `.mavk` package is separate from the stock app.
2. Complete DroidDeck's normal Linux/runtime installation. Steam downloads on
   first start. On Android 14+, enable **Disable child process restrictions**
   in Developer options, as required by upstream DroidDeck.
3. Before starting a session, open Launcher settings → Device and enable
   **Mavk / Mali — experimental** and **Mavk AHB display (experimental)**.
   **Hardware Steam interface** is optional. Restart DroidDeck if requested.
4. Choose **1920×1080** before launching Steam; keep HDR off for this fullscreen
   SDR path. Sign in with your own Steam account and install a game you own.
5. Use the ordinary **Play** button in Steam or DroidDeck. The APK includes Mavk,
   the tested Sarek payload and launch policy. No manual library copying, source
   build, terminal or per-game shell command is needed for the tested route.

Use low graphics with medium object/texture detail for the HL2 demonstration.
Avoid changing resolution inside HL2; stop the session before selecting another
mode. The initial chapter can have a black transition before the scene appears.
For presentation problems, disable the AHB toggle and restart to use X11/SHM.

[Detailed quick start and fallback](docs/TRY_TEST_APK.md). Explicit Steam
compatibility-tool choices are respected: remove an old native/unrelated override
if you want the automatic Windows/Proton profile for a dual-platform title.

## What actually works

| Scenario | Evidence on Tab S11 / Mali-G925 |
| --- | --- |
| HL2, Full HD, low graphics + medium objects/textures | Owner reports 30+ FPS, stable 60 in some areas |
| HL2, nearly maximum graphics | Owner reports usually 20–30 FPS; scene-dependent |
| Muck | Playable rendering; roughly 12–13 displayed buffers/s in one heavy scene, 9.1 in another |
| Muck moving menu | Around 34–35 displayed buffers/s, AHB with no CPU framebuffer copies |
| Standard packaged launches | HL2 menu/first train scene and Muck menu verified; owner also rechecked both Play buttons |
| AHB / SHM | AHB game-buffer/copy counters verified; moving D3D11 SHM fallback and game loading image tested |
| Resize/input | Focused D3D11 ResizeBuffers test passed; absolute pointer scaling corrected |

The HL2 gameplay figures are owner observations, not a controlled benchmark or
locked 60 FPS everywhere. Present calls, Android displayed buffers and unique
rendered gameplay frames are different metrics. FPS varies with scene/settings
and thermal state; menu/loading rates are not gameplay scores.

Known limitations: low Muck world FPS; occasional black/diagonal artifacts;
rare HL2 character/audio hitches; unreliable in-game HL2 resolution changes;
some live D3D9 descriptor aliases; PE32 D3D11 not implemented; native Linux/FEX
startup blockers such as DDNet. Full upstream DXVK/Vulkan conformance and
Adreno/other-Mali AHB portability are not claimed. This Steam-focused build
omits optional Rust store download engines.

## Architecture and what can be reused

`Steam → ARM64 Proton/Wine/FEX → experimental DXVK-Sarek → Mavk → libhybris → stock Android Vulkan/Mali`

XCB/Wayland WSI imports Android hardware buffers and uses explicit acquire/release
fences. Native RGBA can reach its Android layer without CPU framebuffer copies;
BGRA requires a GPU conversion blit. SHM remains a functional fallback. This
presentation work can be reviewed separately from Mavk shader compatibility and
may benefit other GPUs, but that portability still needs hardware testing.

* This repository: launcher/session lifecycle, Android compositor, input, normal
  game-launch policy and optional backend selection.
* [Mavk](https://github.com/dt0-imsyu/Mavk): reusable Core/Compatibility, Vulkan
  adapter and WSI source. Its owner controls access; the repository may remain
  private until separately opened.
* [Review guide](docs/MAVK_DEVELOPER_REVIEW.md): source entry points, buffer/fence
  ownership, reproduction and limitations.

The original Adreno driver/DXVK route remains selectable. Sarek is an experimental
Mali consumer, not a mandatory replacement for all devices. Mavk does not itself
provide WDDM or hardware-accelerated Windows DWM in U-AVF.

## Source builds and debugging

**To try games, use the APK.** Source builders can follow [README.MAVK.md](README.MAVK.md)
and [the pinned-input build guide](docs/PINNED_REVIEW_BUILD.md). A normal Gradle
app/native compositor build was checked from a clean checkout with checksum-verified
external dependencies. That is not an all-third-party-dependencies source rebuild.
Full upstream preparation is `tools/build_local.sh` and requires its documented
Docker/JDK/Android SDK/NDK/Rust/Python/zstd toolchain. Optional stores can be omitted.

For bug reports use **Share logs** from Setup/session drawer. Include device/GPU,
APK version, game, resolution/settings and AHB/SHM selection, with a short moving
scene. Remove personal/account data. [Build checkpoint](docs/BUILD_CHECKPOINT_2026-10-26.md)
records the focused passing tests and the non-green broad Windows-host JVM suite;
do not interpret a packaged-game PASS as validation of every upstream feature.

## Credits and licenses

DroidDeck remains **GPLv3**; see [LICENSE](LICENSE). Original runtime, input and
controller work builds on DroidDeck, WinNative and Bannerlator/maxjivi05. Store
integration also credits the upstream Legendary/gogdl projects and their authors;
Steam/Proton belong to Valve. This fork is not affiliated with Valve or an
endorsed upstream release.

Original Mavk work is by dt0-imsyu and contributors under Apache 2.0, except marked
third-party/derived components. The WSI fork is MIT; the derived Proton wrapper
remains GPLv3. Sarek, Mesa, libhybris, Openbox and compiler dependencies retain
their original terms/notices. Source-coverage limitations remain documented;
calling the APK experimental does not change those licenses.

[Official upstream community](https://discord.gg/JRGAvawjsm) · [Prepared review message and contact routes](docs/DROIDDECK_CONTACTS.md)
