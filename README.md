<p align="center">
  <img src="docs/images/logo.png" alt="Fire Emblem: The Sacred Stones Recomp" width="70%">
</p>

<h1 align="center">fire-emblem-ss-recomp</h1>

<p align="center">
  <b>Fire Emblem: The Sacred Stones, running natively on Android and PC / SteamOS.</b><br>
  No emulator: the game's own C code, compiled for your device, with save states, touch controls and video options.
</p>

<p align="center">
  <a href="https://github.com/kalasyan/fire-emblem-ss-recomp/actions/workflows/build.yml"><img src="https://github.com/kalasyan/fire-emblem-ss-recomp/actions/workflows/build.yml/badge.svg" alt="Build"></a>
  <img src="https://img.shields.io/badge/platform-Android%20%7C%20Linux%20%7C%20Steam%20Deck-blue" alt="Platforms">
  <img src="https://img.shields.io/badge/region-EU%20%7C%20US-green" alt="Regions">
  <img src="https://img.shields.io/badge/built%20with-C%20%2B%20SDL3-orange" alt="C + SDL3">
  <img src="https://img.shields.io/badge/license-GPL--3.0-lightgrey" alt="License">
</p>

---

## ✨ What is this?

**fire-emblem-ss-recomp** turns *Fire Emblem: The Sacred Stones* (GBA) into a
native app for Android and PC. It is built from the
[fireemblem8u](https://github.com/FireEmblemUniverse/fireemblem8u)
decompilation: the game code is compiled as C for the device, with an SDL3
platform layer shared with its sister projects,
[metroid-zm-recomp](https://github.com/kalasyan/metroid-zm-recomp) and
[metroid-fs-recomp](https://github.com/kalasyan/metroid-fs-recomp).

The graphics, maps, text and music are read from **your own ROM** at startup,
so the repository and the APKs contain no game data.

> 🤖 **AI notice:** this project was developed with the help of AI tools
> (Anthropic's Claude), which wrote most of the code and documentation under
> my direction and review.

## 📸 Screenshots

<p align="center">
  <img src="docs/images/gameplay-1.png" width="49%" alt="Battle animation">
  <img src="docs/images/gameplay-2.png" width="49%" alt="Level up">
  <img src="docs/images/gameplay-3.png" width="49%" alt="Combat forecast">
  <img src="docs/images/gameplay-4.png" width="49%" alt="Dialogue">
</p>

## 🎮 Features

- 📱 **Native Android app**, with a touch gamepad you can rearrange and resize.
- 🖥️ **Second screen** for dual-screen devices like the AYN Thor (a second
  window on PC): the whole battle map, the unit under the cursor with the
  battle forecast, or the army list. Tap the map to move the cursor and select
  units; pick what it shows in the options menu or with its tabs.
- 🖼️ **Widescreen:** more of the battle map on the sides instead of black
  bars (Auto, 16:10, 16:9 or 2:1). The game itself doesn't change; menus,
  battle scenes and cutscenes stay 3:2 in the middle.
- 💾 **Quick saves:** 4 slots with thumbnail (`F5` / `F8`), on top of the normal in-game save.
- 🎨 **Video options:** LCD grid or CRT scanlines filter, smooth or integer scaling, VSync.
- 🌊 **Smoothness options:** match the game speed to 60/120 Hz displays, or frame blending for 90/144 Hz.
- 📳 **Rumble** on every hit in battle, on gamepads and on the device itself.
- ⏩ **Fast-forward** (x2 to x10, hold or toggle).
- 🎮 **Keyboard and gamepad** with remappable buttons.
- 🌍 **Europe and USA** builds. The European version keeps its five languages.
- 🔊 **The original sound engine**, compiled from the decompilation.
- 🚫 **No BIOS and no emulator needed.**

## 📊 Status

- Tested on PC with the European ROM:
  - the whole prologue, compared frame by frame with mGBA (same dialogue,
    combat, damage and level-ups);
  - every chapter of both routes, the Tower of Valni and the Lagdou Ruins,
    started directly and played with random input to find crashes;
  - menus, saving, battle animations and quick saves.
- The US build compiles, but has not been tested with a US ROM yet.

## 💽 You need your own ROM

Each build only accepts its own region's ROM:

| Region | File | SHA-1 |
|---|---|---|
| EU | Fire Emblem - The Sacred Stones (Europe) (En,Fr,De,Es,It) | `51b166dcca231369d7f98000766746dd4b98839c` |
| US | Fire Emblem - The Sacred Stones (USA) | `c25b145e37456171ada4b0d440bf88a19f4d509f` |

## 🚀 Installation (Android)

Download `Fire_Emblem_Sacred_Stones-eu.apk` or `Fire_Emblem_Sacred_Stones-us.apk`
from the **[latest release](https://github.com/kalasyan/fire-emblem-ss-recomp/releases/latest)**,
install the one for your region and pick your ROM when you open it. ▶️

Builds of every change to `main` are also in
**[Actions → build](https://github.com/kalasyan/fire-emblem-ss-recomp/actions/workflows/build.yml)**
(the `fe8-android-apk` artifact of the latest run).

## 🐧 Installation (Linux and Steam Deck)

Each release also has a native AppImage per region
(`Fire_Emblem_Sacred_Stones-eu-x86_64.AppImage` / `-us-`; the
`fe8-linux-appimage-*` artifacts of the Actions runs have the same files). It
contains no game data.

1. Put the `.AppImage` in a folder and copy your `.gba` ROM next to it (any
   file name). Make the AppImage executable (`chmod +x`, or *Properties →
   Permissions* in the file manager).
2. Run it. It finds the ROM, checks it and keeps its own copy, so it never
   asks again (without a ROM next to it, it opens a file picker once).
3. **Steam Deck:** do steps 1-2 in Desktop Mode, then *Add a Non-Steam Game*
   in Steam and pick the AppImage. In Game Mode it starts full screen and the
   Deck's controls work as a gamepad (Select+Start opens the options menu).

Settings, saves and quick saves live in `~/.local/share/fire-emblem-ss-recomp/fe8/`.

## 🕹️ Controls

> 💡 The **options menu** pauses the game and opens with **Esc** (or **F1**,
> **Select+Start** on a gamepad, the menu button on touch screens). From there
> you can remap every button, manage quick saves, change the video filter,
> set the fast-forward speed and edit the touch controls.

| GBA | Keyboard |
| --- | --- |
| A / B | X / Z |
| L / R | A / S |
| Start / Select | Enter / Backspace |
| D-pad | arrow keys |
| Fast-forward | Tab |
| Quick save / load | F5 / F8 |
| Fullscreen | F11 |

## 🔨 Building from source

### Android

You need JDK 17 and the Android SDK with NDK 27.2.12479018:

```sh
cd android
./gradlew assembleEuRelease   # or assembleUsRelease
```

### PC (Linux)

You need CMake 3.22+, Ninja, clang (or gcc) with lld, and Python 3. SDL3 and
Dear ImGui are downloaded automatically if they are not installed.

```sh
cmake -B build -G Ninja -DFE8_REGION=eu    # or us
ninja -C build
./build/fe8 [path/to/rom.gba]
```

Without a path, the program looks for `fe8_eu.gba` / `fe8_us.gba` next to the
executable (or any matching `.gba` next to the AppImage) or opens a file
picker, and remembers the ROM afterwards.
`packaging/linux/build-appimage.sh` packs a build made with
`-DSDL_STATIC=ON -DSDL_SHARED=OFF` into an AppImage.
`-DFE8_HOST=headless` builds `fe8_headless`, a windowless version for testing
that dumps screenshots.

### Testing

[docs/PORT.md](docs/PORT.md) explains how the port works and the `FE8_WARP`
mode, which starts a new game on any chapter. That is how every chapter has
been tested without playing up to it.

### GBA ROM (decompilation)

The tree is still the fireemblem8u decompilation, and the port's changes to
the game code are under `#ifdef PLATFORM_PC`. The instructions for building
the original GBA ROM are in [docs/UPSTREAM_README.md](docs/UPSTREAM_README.md).

## 🙏 Credits and licenses

- Decompilation: [FireEmblemUniverse/fireemblem8u](https://github.com/FireEmblemUniverse/fireemblem8u)
  and its contributors ([FE Decomp Portal](https://laqieer.github.io/fe-decomp-portal/)).
- Port layer: GPL-3.0 ([port/LICENSE](port/LICENSE)).
- Software PPU: [Project Picori](https://github.com/999sian/tmc).
- Sister projects: [metroid-zm-recomp](https://github.com/kalasyan/metroid-zm-recomp)
  (Metroid: Zero Mission) and [metroid-fs-recomp](https://github.com/kalasyan/metroid-fs-recomp)
  (Metroid Fusion).

Fire Emblem is a trademark of Nintendo and Intelligent Systems. This project is
not affiliated with them and does not distribute any game data.
