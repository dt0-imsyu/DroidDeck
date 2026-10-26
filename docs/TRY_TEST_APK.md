# Try the experimental Mavk APK

Primary tested device: Samsung Galaxy Tab S11, Mali-G925, stock Android 16.
This is a developer review build. Adreno and other Mali devices have not been
verified with this backend; the original supported graphics paths remain.

## Install and play — no terminal commands

1. Download **DroidDeck-Mavk-demo.apk** from the supplied reviewer link and
   install it through Android. Allow installation for the app opening the APK
   if Android asks. The experimental package is separate from stock DroidDeck.
2. Follow DroidDeck's normal first-run Linux/runtime and Steam download flow.
   Wait for installation to finish; no separate Mavk ZIP or library copying is
   needed. Mavk, the compatibility layers and experimental Sarek are in the APK.
   On Android 14+, enable **Disable child process restrictions** in Android
   Developer options, as required by the normal upstream DroidDeck runtime.
3. Before starting a session, open Launcher settings → Device and enable
   **Mavk / Mali — experimental** and **Mavk AHB display (experimental)**.
   **Hardware Steam interface** is optional. Restart DroidDeck if requested
   after changing AHB settings.
4. Select **1920×1080** before starting Steam. Keep HDR off for this fullscreen
   SDR presentation path. Avoid changing resolution inside HL2 during the test;
   that can crash the game. Stop the session before choosing another mode.
5. Start Steam normally and sign in with your own account. Install Half-Life 2
   or Muck from your library, then press the ordinary **Play** button in Steam
   or the DroidDeck library. No developer shell, manual prefix edit or custom
   launch command is needed for the packaged route tested here.
6. For the HL2 demonstration, use low settings with medium object and texture
   detail. The owner's Full HD gameplay observation is **30+ FPS, with stable
   60 FPS in some areas**; this is not a controlled benchmark or locked 60 FPS
   across the whole game. Startup/first-chapter transitions can take time and
   temporarily show a black image. Rare character/audio hitches remain.

The opted-in profile chooses Windows/Proton for dual-platform x86 games. It
preserves explicit Steam compatibility-tool choices: if you previously forced
a native Linux or unrelated tool, remove that override in Steam's game
Compatibility settings to use the automatic profile. PE32 D3D9/DXGI and
ARM64/AMD64 D3D11 are the tested routes; PE32 D3D11 is not implemented.

## Fallback and feedback

For a presentation problem, stop the session, turn **Mavk AHB display** off and
restart as requested to use X11/SHM. To leave the Mavk backend, disable its main
toggle before starting another session. Stock app data is separate.

Please report device/GPU, APK version, game, resolution/settings, whether AHB
was enabled, and what failed. Distinguish gameplay FPS from menus/loading.
The source/build guides are optional reading for review, not required setup
steps for this APK. Review availability is not a broad production or game
compatibility guarantee. Existing third-party licenses remain in force.
