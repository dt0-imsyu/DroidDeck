# Developer build checkpoint — 2026-10-26

This is a local experimental review build, not a published release.

## Builds and focused tests

* Clean checkout at `ea9c18b`: normal Gradle `assembleDebug` PASS. Native Android
  compositor code was rebuilt. External runtime dependencies were imported by
  `tools/release/import_runtime_seed.py`, checked against the pinned preparation
  manifest and APK; this is not an all-dependencies source rebuild.
* Packaged runtime: `8213c6709ef42a73cbd32c1e48aadad49788800a8552fe83f802fa6472ba08cb`.
* APK SHA256: `36d5efc4749ba6ee22726d6be26f9be04f757a24d2ef8f1d4fa4ed7caf59c6f0`.
* `runtime_inputs.py record` and final `apk` attestation: PASS.
* Linux/WSL launch-policy tests: game environment 82 PASS; FEX/compatibility
  mapping 38 PASS; live Steam compatibility 16 tests, 2 skipped.
* Mavk package/update tests 23 PASS; PE payload routing tests 4 PASS.
* Independent Mavk Core builds from a clean checkout with Android NDK 28.2.

The broad Windows-host Android JVM suite is **not green**: 866 tests,
105 failures, 2 skipped. Several failures explicitly require POSIX symbolic
links/paths that this Windows host cannot supply; other assertion failures were
not localized. Do not reclassify the whole suite as a platform-only failure or
claim that all Android regression tests passed. No app Kotlin code was changed
for this package. The existing FrameLoop suite passed its four checks.

## Device boundary

The clean APK was installed on the stock Tab S11. Session startup selected the
new packaged runtime. HL2 was launched via Steam's standard application launch
request with no private PE/layer overrides; its process used the packaged Proton
wrapper, automatically selected PE32/placed memory, and displayed the Full HD
menu and first train scene. Keyboard navigation worked. AHB game buffers and
zero-copy presentation were observed, with no CPU/layer copies in full game
intervals. The initial chapter introduction was black for a period; displayed
buffer rates during that interval are not gameplay FPS.

`droiddeckctl launch --wait` timed out because its Gamescope focus observer did
not recognize the private-Xwayland game window. The game actually launched and
rendered. This diagnostic observer limitation remains open.

User-observed Full HD gameplay before repackaging: usually 20–30 FPS near maximum
graphics, mostly 30–50 on low; rare character/audio repeats. No performance
improvement is claimed for this packaging checkpoint. In-game resize remains
unreliable; choose the mode before starting the session.

Final Muck/restart/fallback observations are recorded in the closure report.

## Publication gates

Source commits, reviewer guide and message draft are local. No push, account
change, root, flash, U-AVF or VM-data change was performed.

Before public binary distribution, finish the exact corresponding-source and
license package for all bundled GPL/LGPL components. Retaining notices and
checking source provenance does not alone close that gate. Source review can
proceed with this limitation explicit. Also add the owner's fork/video URLs to
`UPSTREAM_MESSAGE.md`; no personal publishing remote has been configured.
