# Draft for the DroidDeck team

Send only after the repository links, video and final ordinary-launch regression
are ready. This file does not send a message or authorize publication.

Hi DroidDeck team!

I've built an experimental DroidDeck fork with a custom Mali Vulkan compatibility
backend (Mavk) and working AHardwareBuffer presentation on a stock Galaxy Tab S11
(Mali-G925), without root.

The AHB path avoids CPU framebuffer readback/copies and preserves SHM fallback.
Some formats still require a GPU conversion. The presentation work is separable
from Mavk and could potentially benefit Adreno and other GPUs too; those devices
have not yet been tested.

Windows games run through the existing Wine/Proton and DXVK-Sarek runtime.
Half-Life 2 gameplay at 1920×1080 was user-tested at roughly 20–30 FPS near
maximum settings and 30–50 FPS on low settings. Muck also runs, but performance
in its game world is substantially lower. These are experimental results, not
full Vulkan conformance or broad game-compatibility claims.

I'd really appreciate a review, especially of AHB buffer/fence ownership,
fullscreen/resize handling and the reusable presentation integration. I don't
have enough time to develop and optimize all of this alone, and would welcome
collaboration, feedback or help sharing the work.

- DroidDeck fork: [insert published fork URL]
- Mavk: https://github.com/dt0-imsyu/Mavk (currently private)
- Gameplay video: [insert video URL]

Architecture, build inputs, reproduction steps and known limitations are in
README.MAVK.md and docs/MAVK_DEVELOPER_REVIEW.md.

Thanks for your work on DroidDeck!
