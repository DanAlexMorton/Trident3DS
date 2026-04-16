# Trident3DS

A Nintendo 3DS emulator built as a [libretro](https://www.libretro.com/) core.

## Features

- **ARM11 CPU emulation** via [Dynarmic](https://github.com/merryhime/dynarmic) JIT compiler
- **PICA200 GPU** rendering (OpenGL ES 3.0)
- **HLE service emulation** for 28+ 3DS system services
- **RetroAchievements** support with proper memory descriptors
- **ROM patching** — IPS, UPS, and BPS formats with CRC32 validation
- **AI texture upscaling** (placeholder for neural network-based upscaler)
- **Cross-platform** — Windows, Linux, macOS, Android, iOS

## Supported ROM Formats

| Format | Extension | Description |
|--------|-----------|-------------|
| NCSD   | `.3ds`, `.cci` | Cartridge dumps |
| NCCH   | `.cxi`, `.app`, `.ncch` | Individual partitions |
| 3DSX   | `.3dsx` | Homebrew executables |
| ELF    | `.elf`, `.axf` | Standard ELF binaries |

## Building

```bash
# Clone with submodules
git clone --recursive https://github.com/DanAlexMorton/Trident3DS.git
cd Trident3DS

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The output is `trident_libretro.dll` (Windows), `.so` (Linux), or `.dylib` (macOS).

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `TRIDENT_USE_DYNARMIC` | ON | Use Dynarmic JIT for CPU |
| `TRIDENT_ENABLE_AI` | OFF | Enable AI texture upscaling |

## Usage

Load the core in RetroArch or any libretro frontend:

```
retroarch -L trident_libretro.dll game.3ds
```

## Architecture

```
src/
├── core/
│   ├── cpu/        # ARM11 CPU (Dynarmic wrapper)
│   ├── memory/     # Virtual memory bus with page tables
│   ├── kernel/     # SVC handler and threading
│   ├── gpu/        # PICA200 GPU emulation
│   ├── audio/      # DSP audio emulation
│   ├── loader/     # ROM format loaders (NCSD/NCCH/3DSX/ELF)
│   ├── patcher/    # ROM patch engine (IPS/UPS/BPS)
│   ├── services/   # HLE service manager (28+ services)
│   ├── ai/         # AI texture upscaler
│   └── emulator.*  # Top-level orchestrator
└── frontend/
    └── libretro/   # libretro API implementation
```

## License

[GPL-3.0](LICENSE)
