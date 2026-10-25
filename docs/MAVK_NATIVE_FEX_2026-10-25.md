# Native Linux/FEX launch boundary on Tab S11

Current library contains Muck, DDNet, FEX, Proton and Linux runtimes; no HL2 in
the sole library. No large game download was started. Windows Muck still works;
this report concerns the separate native Linux DDNet launch.

## Reproduced boundary

Normal Steam DDNet launch selected Valve FEX with SteamLinuxRuntime_sniper and
exited before a window, repeatedly failing to connect to:
`.../compatdata/412220/fex-emu/Server//10261.FEXServer.Socket` (ENOENT).
The existing DroidDeck FEX tool was available but not explicitly selected.
Selected it only for DDNet through the existing droiddeck compatibility helper;
the same socket error remained. Previous selected value was the empty string;
fallback steamlinuxruntime_sniper. Saved the full tool receipt locally.

The helper correctly sets FEX_APP_DATA_LOCATION under the session runtime, but
Steam FEX ignores that override when STEAM_COMPAT_DATA_PATH exists. Upstream
[GetDataDirectory](https://github.com/FEX-Emu/FEX/blob/main/Source/Common/Config.cpp)
checks the Steam variable first under FEX_STEAM_SUPPORT;
[FEXServerClient](https://github.com/FEX-Emu/FEX/blob/main/Source/Common/FEXServerClient.cpp)
uses the resulting data directory for its Steam server socket. These are current
source semantics, not a binary build-revision claim for the installed FEX.

## Minimal candidate and result

In guest_environment, only for portable FEX, remove STEAM_COMPAT_DATA_PATH from
the copied child environment after configuring DroidDeck's own FEX directories.
SteamAppId, shader path and parent environment remain intact; system FEX keeps
the Steam variable. No Proton/Wine launcher or app data is changed by this code.
Existing plus focused regression:37 tests PASS under WSL.

Staged the exact candidate script after verifying that removing only the two
new lines reproduced the installed source. Original bytes preserved in private
`files/mavk-perf/droiddeck-fex.before-steam-data-fix`, baseline SHA256:
`c1dcc2e71804469744271a5edc5834f35211a2364e33427fc10d8a4c5a623367`.

Actual DDNet launch advanced past FEXServer connection failure, then exited:

```
E Failed to remap /proc/pid/cmdline data (prctl failed: result -1, errno 1)
sbrk() failure while processing tunables
```

Crucial isolation: the same prepared FEX environment running Sniper's own
`files/usr/bin/true` (ELF machine62, x86-64) exited127 with the same pair of
messages. No Vulkan workload or DDNet graphics is required to reproduce it.
This is a native Linux/FEX runtime blocker; do not attribute it to Mavk, shaders,
WSI or Mali from this evidence. The first prctl warning alone is not established
as the cause of the later loader failure.

## Rollback / remaining work

Restored the installed launcher byte-for-byte using the verified baseline hash;
restored DDNet's selected compatibility tool to the original empty value.
The diagnostic hotpatch was restored before building the source APK. Subsequently
the same child-environment fix was packaged normally: experimental APK6c8680f5
(full hash in README.MAVK.md), runtime3bd128. Gradle assembleDebug and full APK
runtime attestation PASS. Installed normally; new Steam session and Muck animated
AHB menu render, copy0/errors0. Installed helper hash now
`1d374a8660a268410189a2e09b2828a4e3ed462de62119a83a6ddf89593ce45c`.
Prior APK443c remains the rollback. The source change repairs precedence but is
NOT a working native-game fix; the saved DDNet compatibility selection remains
the original empty value. No false native-game/60FPS claim.
Next useful investigation is the minimal x86-64 true loader/brk contract in FEX
and the downloaded Sniper version. No repeated renderer experiments until that
startup boundary changes. No root, SELinux changes, Steam restart or game data
reset was used. Production U-AVF/Windows/Ubuntu remain untouched.

## Background Steam load

Using existing Steam Window.Minimize on the main and friends windows made the
main page report hidden/minimized. In a15.015s process-time sample, Steam helper
CPU fell to1.4%+0.47%+0.07% of one core; Muck119.15%, busiest dxvk-cs47.22%.
Animated menu measured354buffers/10.1s=34.9/s, compositor350–351GPU/zero-copy/
on-screen buffers per10s, copy0, pooldrops0. This is lower background work, NOT
a proven large FPS gain: previous normal menu33.6–33.8/s, thermal states differ.
No automatic Steam-window behavior was added to the app.
