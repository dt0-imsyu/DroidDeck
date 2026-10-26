# Reviewing the Mavk / AHB fork

This branch preserves DroidDeck's Steam, Proton/Wine/FEX, launcher and normal
Adreno/Turnip path. Mavk is an optional experimental Mali userspace compatibility
backend. No root, bootloader unlock or Android system changes are required.

## Useful entry points

* `app/src/main/cpp/waylandcomp/src/compositor.c`: layer eligibility, fullscreen
  viewport handling and presentation statistics. A destination viewport alone
  must not disable a geometrically identical direct layer.
* `app/src/main/cpp/waylandcomp/src/ahb_swapchain.c`: Android buffer/lifetime and
  synchronization handling; protocol in `protocols/banner-ahb-v1.xml`.
* `app/src/main/java/com/droiddeck/launcher/session/MavkExperimental.kt`:
  explicit backend selection, content-addressed runtime and AHB/SHM rollback.
* `tools/linuxfs/overlay/usr/local/bin/droiddeck-game-env`: normal Proton launch
  policy, validated AHB capability receipt, PE architecture checks and WoW64
  placement selection. The packaged policy must equal this source.
* `tools/linuxfs/overlay/usr/local/bin/steam-compatibility`: the opted-in Mavk
  session selects Windows/Proton for x86 dual-platform titles. Native ARM64 and
  explicit compatibility-tool selections are preserved. Linux-only x86 titles
  retain their separate FEX route and its documented limitations.

The separate Mavk checkout contains `third_party/wsi-layer`, `adapters/android`,
`adapters/droiddeck` and the Vulkan Core/Compatibility sources. Keep this split:
the reusable WSI/AHB work can be reviewed without adopting Mavk's experimental
shader emulation or Sarek admissions.

## Frame path and accounting

Wine's XCB surface is associated with a Wayland fullscreen proxy. Vulkan WSI
exports/imports Android hardware buffers and transports ownership with explicit
acquire/release fences. The compositor can place the buffer directly in the
Android display layer. Native RGBA avoids a conversion; BGRA needs a GPU blit.
Neither AHB route reads the framebuffer back to the CPU. SHM is a real fallback,
with CPU copying, and must remain available.

Do not equate Present calls, game-buffer arrivals, Android displayed buffers and
unique gameplay frames. Record a moving game scene and preserve resolution,
duration, application/frame counters, compositor copy/zero-copy counts and
thermal state. Startup/loading/menu results must not be labelled gameplay FPS.

## Reproduction through the ordinary UI

1. Install the `.mavk` experimental APK; keep the stock app separately.
2. Install/start the normal Linux runtime and Steam using DroidDeck's UI.
3. Select the experimental Mavk backend and optional Mavk AHB presentation in
   Launcher settings. Enable hardware Steam UI if desired. These are session
   settings, not terminal commands per game.
4. Select the desired display resolution **before** starting the session.
5. Sign in to Steam yourself, install Muck or HL2, then use the normal Play
   button. The packaged runtime supplies ARM64X D3D11 and optional PE32
   D3D9/DXGI; it does not implement PE32 D3D11.
6. To roll back presentation, select X11/SHM and restart the session. To compare
   Adreno, retain the original supported driver/DXVK selection instead of Mavk.

The new ordinary-button path still requires its final APK device regression.
Do not use the developer's private `files/mavk-*-test` directories as dependencies
for someone else's installation. They are development artifacts, not the runtime.

## Short gameplay recording

Use a 60–120 second clip: show the selected backend/resolution, start a game
normally, enter actual gameplay, move/look around, and show working input/audio.
Keep the game scene/settings unchanged while reporting a sample. Samsung screen
recording may affect performance; label the recorded run accordingly. Do not
show Steam login details, account pages or personal notifications in public clips.

HL2 is the preferred demonstration, with Muck as a fallback. At Full HD the user
reported 10–30 FPS near maximum graphics (usually 20–30), and mostly 30–50 FPS
on low settings. These are user gameplay observations, not a controlled
benchmark. Rare character pauses with repeating audio remain unlocalized.
Full HD was independently confirmed from D3D9/Vulkan swapchain parameters.
In-game resolution changes can crash HL2 and leave Steam unresponsive; restart
with the target mode already selected. Neither game demonstrates stable 60 FPS.

## Known gaps to review

* Low scene-dependent performance; no fixed30FPS cap was established. The final
  shader A/B did not produce a material win. FPS investigation is now closed.
* Occasional diagonal black artifacts have been reported; no universal fix.
* Some D3D9 fixed-function pipelines still expose conflicting live image aliases.
  Specialization-only aliases and enabled scalar layouts have focused checks;
  this is not full Vulkan descriptor/shader conformance.
* DDNet's native x86 Linux/FEX route fails before Vulkan. Do not treat it as a
  Mali shader bug or promise that Windows graphics changes fix that runtime.
* Adreno AHB and other Mali generations remain hardware-unverified. Port the
  neutral presentation pieces first and keep native driver/SHM fallbacks.

GPLv3 applies to DroidDeck; preserve the original licenses and provenance.
Mavk/WSI and bundled dependencies retain their own terms. Refer to README.MAVK.md
and Mavk/THIRD_PARTY_NOTICES.md for build inputs and binary/source publication
gates. This preparation does not authorize or perform a push/publication.
