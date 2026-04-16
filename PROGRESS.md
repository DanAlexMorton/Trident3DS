# Trident3DS — Project Progress

_Last updated: April 17, 2026_

---

## Background

Started from DanAlexMorton/3dsTrident — a fork of Panda3DS wrapped as a libretro core.
After submitting to the libretro buildbot, received feedback from warmenhoven (libretro gatekeeper):

> "A couple minor tweaks and one half-implemented feature isn't enough to put a fork of an existing core on the buildbot."

Decision: **build a new 3DS emulator from scratch** under the Trident name, with genuine differentiators:
- AI-based texture upscaling
- Built-in ROM patching (IPS/UPS/BPS)
- First-class iOS support
- RetroAchievements from day one

---

## Repository Map

| Repo | Location | Status |
|------|----------|--------|
| [DanAlexMorton/3dsTrident](https://github.com/DanAlexMorton/3dsTrident) | `C:\Users\donal\Work\Games\3dsEmulator` | Maintained (iOS fixes pushed) |
| [DanAlexMorton/Trident3DS](https://github.com/DanAlexMorton/Trident3DS) | `C:\Users\donal\Work\Games\Trident3DS` | Active development |
| [DanAlexMorton/libretro-core-info fork](https://github.com/DanAlexMorton/libretro-core-info) | `C:\Users\donal\Work\Games\libretro-core-info` | PR #76 open |

---

## What We Have Done

### 3dsTrident (fork of Panda3DS)

- [x] Fixed `is_experimental = false` in `trident_libretro.info` (commit `71fc485`)
- [x] Opened PR #76 to [libretro/libretro-core-info](https://github.com/libretro/libretro-core-info)
- [x] Confirmed libretro-super PR #1964 already merged (recipes + info)
- [x] Fixed iOS GLES shader version: `#version 310 es` → `#version 300 es` (Panda3DS fork)
- [x] Added `ios/Trident.entitlements` with `com.apple.security.cs.allow-jit` + `dynamic-codesigning`
- [x] Updated GitHub Actions workflow with entitlements (commit `b97b0be`)
- [x] Drafted Discord message for GitLab mirror request

### Trident3DS (new from-scratch emulator)

#### Infrastructure
- [x] GitHub repo created: `DanAlexMorton/Trident3DS`
- [x] Project directory scaffolded with full source tree
- [x] CMake build system (C++20, shared lib, cross-platform)
- [x] Dynarmic submodule added (`third_party/dynarmic` from Panda3DS-emu fork)
- [x] `.gitignore` updated
- [x] Initial commit `8b02ff6` pushed to main

#### Core Modules
- [x] **Memory bus** (`src/core/memory/`) — Full page-table virtual addressing, read/write ops for 8/16/32/64-bit widths, `getContiguousMappedRegions()` for RetroAchievements memory descriptors
- [x] **ROM Patcher** (`src/core/patcher/`) — Complete IPS, UPS, and BPS implementation with CRC32 validation
- [x] **ROM Loader** (`src/core/loader/`) — NCSD/NCCH/3DSX/ELF format detection and partial NCSD→NCCH partition extraction
- [x] **CPU wrapper** (`src/core/cpu/`) — Dynarmic integration point stub with `run()`, `reset()`, `setPC/SP`, `setSVCHandler()`
- [x] **Kernel** (`src/core/kernel/`) — SVC handler stub with all 39 SVCs documented (0x01–0x3D)
- [x] **GPU** (`src/core/gpu/`) — PICA200 stub; dual-screen layout (400×240 top + 320×240 bottom), `renderFrame()` producing XRGB8888
- [x] **Audio** (`src/core/audio/`) — DSP stub at 32768 Hz, `generateSamples()` producing stereo PCM
- [x] **Service Manager** (`src/core/services/`) — HLE framework with 28 services registered (srv:, APT, gsp::Gpu, hid:USER, fs:USER, cfg:u, dsp::DSP, and more)
- [x] **AI Texture Upscaler** (`src/core/ai/`) — Stub with nearest-neighbor fallback; wired for 2× and 4× neural network modes
- [x] **Emulator orchestrator** (`src/core/emulator.*`) — Owns and initialises all subsystems, `runFrame()`, input routing, HID register mapping

#### Frontend
- [x] **libretro_core.cpp** (`src/frontend/libretro/`) — Full `retro_*` API: `retro_init/deinit`, `retro_load_game`, `retro_run`, `retro_get_memory_data`, input descriptors, RetroAchievements memory map, core options

#### Assets / Platform
- [x] `ios/Trident.entitlements` — JIT entitlements for sideloaded iOS apps
- [x] `README.md` — Build instructions, architecture diagram, feature table

---

## What We Want to Achieve

### Short-Term (Next Steps)

- [ ] **Wire Dynarmic into the CPU wrapper** — instantiate `Dynarmic::A32::Jit`, implement memory callbacks via `Dynarmic::A32::UserCallbacks`, connect SVC handler
- [ ] **Implement basic NCCH loading** — decrypt ExeFS, load `.code` section into memory at correct virtual address, parse `ExHeader` for stack/BSS sizes
- [ ] **Boot to `svcBreak`** — get a simple homebrew `.3dsx` to reach the entry point and throw a known SVC without crashing
- [ ] **Stub essential SVCs** — `SVC 0x08` (CreateThread), `SVC 0x13` (CreateEvent), `SVC 0x24` (SendSyncRequest), `SVC 0x2A` (GetSystemInfo) are the minimum for most homebrew
- [ ] **Handle `srv:` and `APT:U`** — these are called by almost every 3DS title within the first 50ms; getting them past init unlocks many games
- [ ] **GPU: clear-colour framebuffer** — even a black screen with correct dimensions confirms the render pipeline works end-to-end
- [ ] **Set up GitHub Actions CI** — CMake build on Ubuntu, Windows, and macOS; fail on build errors

### Medium-Term

- [ ] **PICA200 GPU rendering** — implement the command processor, rasteriser, and texture units; target: render simple geometry (triangles/quads)
- [ ] **GSP service** — GPU service proxy; games write GPU commands through GSP, not directly
- [ ] **fs:USER** — virtual SD card and ROM filesystem access; needed for most commercial titles and many homebrew
- [ ] **DSP audio** — implement the CEVA TeakLite DSP or use an LLE DSP binary; target 32768 Hz stereo output
- [ ] **N3DS support** — enable N3DS mode (256MB OCRAM, extra CPU cores), wire up Dynarmic for the extra ARM11 cores
- [ ] **Save states** — implement `retro_serialize` / `retro_unserialize` once core state is stable
- [ ] **Cheats** — implement `retro_cheat_set` for Action Replay codes

### Long-Term

- [ ] **RetroAchievements** — finalize memory map alignment with [rcheevos PR #446](https://github.com/RetroAchievements/rcheevos/pull/446); pass RA test suite
- [ ] **Neural network texture upscaler** — replace nearest-neighbor stub with a lightweight ESPCN or FSRCNN model, running on the GPU via GLSL compute shaders (GLES 3.1 on Android, Metal on iOS via MSL transpile)
- [ ] **IPS/UPS/BPS patcher UI** — expose patch file path via libretro core option so users can patch ROMs without external tools
- [ ] **Android build** — CMake + Gradle integration, GLES 3.1, test on RetroArch Android
- [ ] **iOS build** — CMake, Xcode, MAP_JIT + `pthread_jit_write_protect_np` (Oaknut already supports this), Aerinstaller/AltStore distribution
- [ ] **libretro buildbot** — once the emulator can boot commercial software, re-submit to libretro-core-info and request buildbot inclusion
- [ ] **GitLab mirror** — request mirror on `git.libretro.com` via the Discord #core-development channel

### Aspirational / Differentiators

- [ ] **ROM hack support** — built-in patcher means users load original ROM + patch file; no pre-patched ROMs needed
- [ ] **Texture packs** — hook the GPU texture unit to allow hi-res texture replacement
- [ ] **Debug view** — optional overlay showing CPU registers, GPU state, active services (useful for homebrew developers)
- [ ] **Multiplayer via netplay** — libretro netplay support; 3DS local wireless over UDP tunnel

---

## Architecture Reference

```
Trident3DS/
├── include/trident/libretro.h          # libretro API header
├── ios/Trident.entitlements            # iOS JIT entitlements
├── third_party/dynarmic/               # ARM11 JIT (submodule)
└── src/
    ├── core/
    │   ├── emulator.hpp / .cpp         # Top-level orchestrator
    │   ├── memory/                     # Virtual memory bus
    │   ├── cpu/                        # ARM11 (Dynarmic wrapper)
    │   ├── kernel/                     # SVC dispatcher
    │   ├── gpu/                        # PICA200 GPU
    │   ├── audio/                      # CEVA DSP
    │   ├── loader/                     # ROM format loaders
    │   ├── patcher/                    # IPS/UPS/BPS patcher
    │   ├── services/                   # HLE service manager
    │   └── ai/                         # Texture upscaler
    └── frontend/
        └── libretro/libretro_core.cpp  # libretro API impl
```

### Key Constants

| Symbol | Value | Description |
|--------|-------|-------------|
| `CPU_CLOCK_RATE` | 268,111,856 Hz | ARM11 MPCore @ 268 MHz |
| `CYCLES_PER_FRAME` | 4,468,530 | @ 60 fps |
| `FCRAM_SIZE` | 128 MB (256 MB N3DS) | Main RAM |
| `VRAM_SIZE` | 6 MB | GPU memory |
| `AUDIO_SAMPLE_RATE` | 32,768 Hz | DSP output |
| `TOP_SCREEN` | 400 × 240 | Top screen |
| `BOTTOM_SCREEN` | 320 × 240 | Touch screen |

### Memory Map (Virtual)

| Range | Description |
|-------|-------------|
| `0x00100000–0x07FFFFFF` | Application code |
| `0x08000000–0x0FFFFFFF` | Heap |
| `0x14000000–0x1BFFFFFF` | VRAM |
| `0x1FF00000–0x1FF7FFFF` | DSP memory |
| `0x1FF80000–0x1FFFFFFF` | AXI WRAM |
| `0x10000000–0x10FFFFFF` | IO registers |
