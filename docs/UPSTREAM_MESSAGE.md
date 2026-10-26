# Draft for the DroidDeck team

Unsent draft. Add the repository, test APK and Google Drive video links before
sending. The standard packaged launch route has been tested; this remains an
experimental review build. This file does not send or publish anything.

Hi DroidDeck team!

I've built an experimental DroidDeck fork with a custom Mali Vulkan compatibility
backend (Mavk) and working AHardwareBuffer presentation on a stock Galaxy Tab S11
(Mali-G925), without root.

The AHB path avoids CPU framebuffer readback/copies and preserves SHM fallback.
Some formats still require a GPU conversion. The presentation work is separable
from Mavk and could potentially benefit Adreno and other GPUs too; those devices
have not yet been tested.

Windows games run through the existing Wine/Proton and DXVK-Sarek runtime.
In my Half-Life 2 gameplay at 1920×1080, low settings with medium object and
texture detail deliver 30+ FPS, reaching a stable 60 FPS in some areas.
Performance varies by scene; this is not a claim of locked 60 FPS throughout
the game. Muck also runs, although its gameplay performance is much lower.

I've prepared a test APK so you can install it, select Mavk/AHB in the launcher,
and launch games through the normal Steam UI. No per-game terminal commands,
manual library copying or source build are needed to try the packaged backend.
The usual first-run runtime download and Steam sign-in are still required.

I'd really appreciate a review, especially of AHB buffer/fence ownership,
fullscreen/resize handling and the reusable presentation integration. I don't
have enough time to develop and optimize all of this alone, and would welcome
collaboration, feedback or help sharing the work.

- DroidDeck fork: [insert published fork URL]
- Mavk: https://github.com/dt0-imsyu/Mavk (currently private)
- Test APK: [insert reviewer download URL]
- Gameplay video (Google Drive): [link to be added]

Architecture, build inputs, reproduction steps and known limitations are in
README.MAVK.md and docs/MAVK_DEVELOPER_REVIEW.md. The short install-and-play
guide is docs/TRY_TEST_APK.md. This remains experimental, with known rendering,
resize and compatibility limitations; it is not a complete Vulkan implementation.

Thanks for your work on DroidDeck!
