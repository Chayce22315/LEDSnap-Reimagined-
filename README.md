# LEDSnap!

Dynamic LED banner suite. Design pixel-art banners on your iPhone, preview them full-screen on-device, or stream to a PC receiver window.

**Phone-first**: no Mac or PC required to design and present banners. Your iPhone is both the studio and the stage.

## CI

[![LEDSnap CI](../../actions/workflows/build.yml/badge.svg)](../../actions/workflows/build.yml)

Builds are run on GitHub Actions (macOS runner for iOS, Ubuntu for core tests and PC receiver). Push to `main` or open a PR to trigger.

## Architecture

```
core/     Pure C engine (libledsnap) - compositor, tools, text, animation, .lsnap format
ios/      Swift/SwiftUI app linking core via bridging header
pc/       C + SDL2 receiver app linking core via CMake
```

The shared `core/` library handles all rendering math — 3-layer compositing, drawing tools, scrolling text, frame-by-frame animation with onion skinning, and the `.lsnap` binary export format. Both the iOS app and the PC receiver link against it.

## Phone-First Workflow

1. Open the app, tap **+** to create a new banner (pick grid size)
2. Draw on the pixel canvas with pen, bucket fill, or eraser
3. Tap **Preview** to see your banner full-screen in landscape — the phone becomes the LED board
4. Tap **Save** to persist as a `.lsnap` file on-device
5. Reopen any saved banner from the home screen to edit or present again
6. Double-tap to exit full-screen preview

No PC needed. When you do have a PC, export the `.lsnap` and load it in the receiver app.

## Building the Core Library & Tests

```bash
cd core
cmake -B build
cmake --build build
./build/test_compositor
```

## Building the PC Receiver

Requires SDL2 development libraries installed.

### Windows (MSVC + vcpkg)

```bash
vcpkg install sdl2
cd pc
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
./build/Release/ledsnap_receiver.exe
```

### Linux / macOS

```bash
cd pc
cmake -B build
cmake --build build
./build/ledsnap_receiver
```

### PC Receiver Usage

```
ledsnap_receiver [options] [file.lsnap]

Options:
  --eco           30 fps eco mode (default 60 fps)
  --scale N       screen pixels per grid cell (default 12)
  --width N       grid width  (default 64)
  --height N      grid height (default 16)

Keys:
  E       toggle eco mode
  Escape  quit
```

When launched without a `.lsnap` file, the receiver shows a rainbow background with scrolling demo text.

## Building the iOS App

The iOS app builds via GitHub Actions CI (see `.github/workflows/build.yml`). You do not need a Mac locally.

To build manually on a Mac: open the `ios/` folder in Xcode, add the `core/include` and `core/src` files to the Xcode project target, set the bridging header to `LEDSnap/LEDSnap-Bridging-Header.h`, and build for a device or simulator.

## .lsnap Format

Compact binary format for transferring banner data from designer to receiver. See `core/include/ls_lsnap.h` for the full specification. Key properties:

- 16-byte header (magic, version, dimensions, FPS, mode flag, palette size)
- Indexed palette (up to 256 RGB entries)
- Delta-compressed frames (only changed pixels stored per frame)
