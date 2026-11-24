# Pecislav Sound Mixer

Native grouped audio mixer dock for **OBS Studio 33.0.2 / Windows x64**.

Created by **[Pecislav](https://github.com/Pecislav)**.

**Download `Pecislav-Sound-Mixer-Setup.exe` from the Assets below to install.**

- Draggable channels and groups.
- Vertical strips expanding right, compact rows expanding down.
- Real volume, mute, audio monitoring and smooth live meters.
- Hide/restore channels without changing audio or routing.
- Animated muted state, full-name tooltips and source context menus.
- Layouts saved per scene collection.
- Compact footer with settings under ⋮, responsive compact rows, aligned meters/faders.
- Two meter bars for mono and stereo; volume changes by wheel are disabled.

## Install

Close OBS and run `Pecislav-Sound-Mixer-Setup.exe`. Confirm the OBS folder,
then reopen OBS and enable **Docks → Sound Mixer** (localized to your OBS language). The installer requests
administrator rights and includes an uninstaller. It is currently unsigned.
The Windows ZIP remains available for manual/portable installation.

The UI follows the OBS language (Czech, English, Slovak, German or Polish; English fallback). Windows ARM64 and other OBS major versions need their
own builds. Groups organize sources and provide shared mute; they are not audio
buses. This is an early test release.

The source ZIP is for development, not installation. See `SHA256SUMS.txt` for
package checksums.
