# Trident3DS — Agent-Based Development Plan

> **Last updated:** April 17, 2026
>
> **Paradigm:** Each component is an isolated `git worktree` with its own `AGENT.md`.
> Agents work in parallel without file conflicts. PRs merge back to `main` at defined checkpoints.
> This file is the source of truth for what exists, what is stubbed, and what each agent owns.

---

## Current State of the Codebase

Everything compiles. Nothing executes yet. Here is the honest status of each module:

| Module | File | State | What's Done | What's Missing |
|---|---|---|---|---|
| Memory bus | `src/core/memory/` | ✅ Solid | Full page-table VM, read/write 8/16/32/64, RA regions | Nothing — this is complete |
| ROM Patcher | `src/core/patcher/` | ✅ Complete | IPS, UPS, BPS with CRC32 | Nothing |
| Loader | `src/core/loader/` | 🟡 Partial | NCSD→NCCH dispatch, format detection, partial NCCH magic check | `load3DSX()` unimplemented; `loadNCCH()` stubbed (no ExeFS/code segment) |
| CPU | `src/core/cpu/` | 🔴 Stub | Correct interface, pimpl skeleton | Dynarmic never instantiated; `run()` returns 0 |
| Kernel | `src/core/kernel/` | 🔴 Stub | `handleSVC()` exists with TODO list of 39 SVCs | No SVC actually does anything |
| GPU | `src/core/gpu/` | 🔴 Stub | Dual-screen layout, `renderFrame()` signature | No PICA200 commands processed |
| Audio | `src/core/audio/` | 🔴 Stub | 32768 Hz, stereo PCM signature | No DSP emulation |
| Services | `src/core/services/` | 🟡 Skeleton | 28 services registered | None handle actual IPC requests |
| AI Upscaler | `src/core/ai/` | 🟡 Stub | Nearest-neighbor fallback wired | No neural network model |
| Dynarmic | `third_party/dynarmic/` | ✅ Present | Already a git submodule; already in CMakeLists.txt | Not instantiated in `cpu.cpp` |
| libretro frontend | `src/frontend/libretro/` | ✅ Complete | Full `retro_*` API, RetroAchievements memory map | — |
| CI | `.github/workflows/` | 🔴 Missing | — | No GitHub Actions yet |

---

## The Architecture That Already Exists

```
Trident3DS/
├── CMakeLists.txt                      # Cross-platform, Dynarmic already wired in
├── third_party/dynarmic/               # ARM11 JIT (git submodule, Panda3DS fork)
├── include/trident/libretro.h
├── ios/Trident.entitlements
└── src/
    ├── core/
    │   ├── emulator.hpp/.cpp           # Orchestrator — owns all subsystems
    │   ├── memory/memory.hpp/.cpp      # ✅ COMPLETE — page-table VM, all widths
    │   ├── cpu/cpu.hpp/.cpp            # 🔴 STUB — wire Dynarmic here
    │   ├── kernel/kernel.hpp/.cpp      # 🔴 STUB — implement SVCs here
    │   ├── gpu/gpu.hpp/.cpp            # 🔴 STUB — PICA200 pipeline here
    │   ├── audio/audio.hpp/.cpp        # 🔴 STUB — DSP here
    │   ├── loader/loader.hpp/.cpp      # 🟡 PARTIAL — complete ExeFS + 3DSX
    │   ├── patcher/patcher.hpp/.cpp    # ✅ COMPLETE
    │   ├── services/service_manager.hpp/.cpp  # 🟡 SKELETON — implement IPC
    │   └── ai/texture_upscaler.hpp/.cpp       # 🟡 STUB
    └── frontend/
        └── libretro/libretro_core.cpp  # ✅ COMPLETE
```

---

## Agent Model and Golden Rules

```
main branch  ← Integration. CI must pass. Human reviews all PRs.
  │
  ├── agent/cpu-jit        → ../trident-agent-cpu/
  ├── agent/loader         → ../trident-agent-loader/
  ├── agent/kernel-hle     → ../trident-agent-kernel/
  ├── agent/ci-testing     → ../trident-agent-ci/
  ├── agent/gpu-pica        → ../trident-agent-gpu/        (Phase 2)
  ├── agent/services-hle   → ../trident-agent-services/   (Phase 2)
  └── agent/audio-dsp      → ../trident-agent-audio/      (Phase 2)
```

**Rules every agent must follow:**

1. **Never edit headers shared between components.** `cpu.hpp`, `memory.hpp`, `kernel.hpp`, `gpu.hpp`, `loader.hpp` are contracts. Read them; never change them without a human PR.
2. **Every commit must keep `cmake --build build` green.** Don't break the build for other agents.
3. **Open a PR when success criteria are met.** Don't push to `main` directly.
4. **Read your `AGENT.md` first.** It contains the exact brief, constraints, and success criteria.

---

## Phase 1 — Proof of Life: Boot Homebrew

**Goal:** A `.3dsx` homebrew binary runs, hits `svcBreak` (SVC `0x3C`), emulator logs it cleanly and exits.
**Critical path:** `cpu-jit` → `kernel-hle` + `loader` run in parallel → integration.
**Duration:** 2–4 weeks. Four agents run simultaneously.

### Agent: CPU (`agent/cpu-jit`)

**The single most important task in the entire project.**
Dynarmic is already a git submodule. CMakeLists.txt already links it. All that remains is implementing `cpu.cpp`.

**Owns:** `src/core/cpu/cpu.cpp` only. Do not touch `cpu.hpp`.

**Exact Dynarmic API to implement** (from `third_party/dynarmic/src/dynarmic/interface/A32/config.h`):

```cpp
// The full implementation shape for cpu.cpp

#include "cpu.hpp"
#include "../memory/memory.hpp"

#ifdef TRIDENT_USE_DYNARMIC
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/A32/config.h"
#endif

namespace Trident {

#ifdef TRIDENT_USE_DYNARMIC
// Step 1: Implement UserCallbacks — bridges Dynarmic to Memory
struct DynarmicCallbacks : public Dynarmic::A32::UserCallbacks {
    Memory* mem;
    CPU::SVCCallback svcHandler;
    uint64_t ticksRemaining = 0;

    // Memory reads — forward to Memory bus
    uint8_t  MemoryRead8 (uint32_t vaddr) override { return mem->read8(vaddr); }
    uint16_t MemoryRead16(uint32_t vaddr) override { return mem->read16(vaddr); }
    uint32_t MemoryRead32(uint32_t vaddr) override { return mem->read32(vaddr); }
    uint64_t MemoryRead64(uint32_t vaddr) override { return mem->read64(vaddr); }

    // Memory writes — forward to Memory bus
    void MemoryWrite8 (uint32_t vaddr, uint8_t  v) override { mem->write8(vaddr, v); }
    void MemoryWrite16(uint32_t vaddr, uint16_t v) override { mem->write16(vaddr, v); }
    void MemoryWrite32(uint32_t vaddr, uint32_t v) override { mem->write32(vaddr, v); }
    void MemoryWrite64(uint32_t vaddr, uint64_t v) override { mem->write64(vaddr, v); }

    // SVC — dispatch to Kernel
    void CallSVC(uint32_t swi) override {
        if (svcHandler) svcHandler(swi);
    }

    // Exception handler — log and halt for now
    void ExceptionRaised(uint32_t pc, Dynarmic::A32::Exception e) override {
        // TODO: handle UnpredictableInstruction, UndefinedInstruction, etc.
        (void)pc; (void)e;
    }

    // Interpreter fallback — for unimplemented instructions
    void InterpreterFallback(uint32_t pc, size_t num) override {
        // TODO: minimal interpreter for rare instructions
        (void)pc; (void)num;
    }

    // Timing
    void AddTicks(uint64_t ticks) override {
        ticksRemaining = (ticks > ticksRemaining) ? 0 : ticksRemaining - ticks;
    }
    uint64_t GetTicksRemaining() override { return ticksRemaining; }
};
#endif

struct CPU::Impl {
    Memory* memory = nullptr;
    SVCCallback svcHandler;

#ifdef TRIDENT_USE_DYNARMIC
    std::unique_ptr<DynarmicCallbacks> callbacks;
    std::unique_ptr<Dynarmic::A32::Jit> jit;
#endif

    // Fallback register state (used when Dynarmic not available)
    uint32_t regs[16] = {};
    uint32_t cpsr = 0;
};

CPU::CPU() : impl(std::make_unique<Impl>()) {}
CPU::~CPU() = default;

bool CPU::init(Memory& memory) {
    impl->memory = &memory;

#ifdef TRIDENT_USE_DYNARMIC
    // Step 2: Wire up callbacks
    impl->callbacks = std::make_unique<DynarmicCallbacks>();
    impl->callbacks->mem = &memory;

    // Step 3: Configure Dynarmic
    Dynarmic::A32::UserConfig config;
    config.callbacks = impl->callbacks.get();
    config.arch_version = Dynarmic::A32::ArchVersion::v6K; // ARM11 = ARMv6K

    // Step 4: Instantiate the JIT
    impl->jit = std::make_unique<Dynarmic::A32::Jit>(config);
#endif

    reset();
    return true;
}

void CPU::reset() {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) {
        impl->jit->Regs().fill(0);
        impl->jit->SetCpsr(0x00000010); // User mode, ARM state
    }
#endif
    std::fill(std::begin(impl->regs), std::end(impl->regs), 0);
    impl->cpsr = 0;
}

void CPU::setPC(uint32_t pc) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) { impl->jit->Regs()[15] = pc; return; }
#endif
    impl->regs[15] = pc;
}

uint32_t CPU::getPC() const {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) return impl->jit->Regs()[15];
#endif
    return impl->regs[15];
}

void CPU::setReg(unsigned index, uint32_t value) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit && index < 16) { impl->jit->Regs()[index] = value; return; }
#endif
    if (index < 16) impl->regs[index] = value;
}

uint32_t CPU::getReg(unsigned index) const {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit && index < 16) return impl->jit->Regs()[index];
#endif
    return (index < 16) ? impl->regs[index] : 0;
}

void CPU::setSP(uint32_t sp) { setReg(13, sp); }
uint32_t CPU::getSP() const { return getReg(13); }
void CPU::setCPSR(uint32_t cpsr) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) { impl->jit->SetCpsr(cpsr); return; }
#endif
    impl->cpsr = cpsr;
}
uint32_t CPU::getCPSR() const {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) return impl->jit->Cpsr();
#endif
    return impl->cpsr;
}

uint64_t CPU::run(uint64_t cycles) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) {
        impl->callbacks->svcHandler = impl->svcHandler; // keep in sync
        impl->callbacks->ticksRemaining = cycles;
        impl->jit->Run();
        return cycles - impl->callbacks->ticksRemaining;
    }
#endif
    (void)cycles;
    return 0;
}

void CPU::setSVCHandler(SVCCallback callback) {
    impl->svcHandler = std::move(callback);
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->callbacks) impl->callbacks->svcHandler = impl->svcHandler;
#endif
}

void CPU::invalidateCacheRange(uint32_t start, uint32_t size) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) impl->jit->InvalidateCacheRange(start, size);
#endif
    (void)start; (void)size;
}

void CPU::clearCache() {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) impl->jit->ClearCache();
#endif
}

} // namespace Trident
```

**Success criteria:**
- [ ] `cmake --build build` succeeds with Dynarmic compiled in
- [ ] A test that calls `cpu.init(mem)` then sets PC to a NOP sled address and calls `cpu.run(100)` returns non-zero cycles
- [ ] SVC callback fires when a SVC instruction is executed

---

### Agent: Loader (`agent/loader`)

**Owns:** `src/core/loader/loader.cpp` only. Do not touch `loader.hpp`.

**Task 1: Complete `load3DSX()`**

3DSX binary format:
```
Offset  Size  Description
0x00    4     Magic "3DSX"
0x04    2     Header size (usually 0x20 or 0x2C for extended header)
0x06    2     Relocation header size (always 0x0C)
0x08    4     Format version
0x0C    4     Flags
0x10    4     Code segment size (bytes)
0x14    4     RO (read-only) segment size
0x18    4     BSS region size
0x1C    4     Data segment size (includes BSS)
0x20    4     SMDI (smdh icon) offset (extended header only, if header_size >= 0x2C)
0x24    4     SMDI size

After header: code segment, then ro segment, then data segment (raw bytes)
After segments: relocation tables

Relocation entry (4 bytes each):
  Bits 31-16: skip count (words to skip before patching)
  Bits 15-0:  patch count (words to patch after skipping)
  Type ABS (cross-segment absolute): add segment base
  Type REL (relative): subtract PC
```

Load address: code at `0x00100000`, ro at `code_end`, data at `ro_end`.

**Task 2: Complete `loadNCCH()` — ExeFS parsing**

```
NCCH structure at offset 0x000 from partition start:
  0x000  0x100  RSA-2048 signature (skip for homebrew)
  0x100  0x004  Magic "NCCH"
  0x104  0x004  Content size (media units = × 0x200)
  0x108  0x008  Partition ID
  ...
  0x180  0x004  ExeFS offset (media units from partition start)
  0x184  0x004  ExeFS size
  0x1A0  0x004  RomFS offset
  0x1A4  0x004  RomFS size

ExeFS header at ExeFS offset:
  8 file entries, each 16 bytes:
    0x00  0x08  File name (null-padded, e.g. ".code\0\0\0")
    0x08  0x04  File offset (from ExeFS + 0x200)
    0x0C  0x04  File size
  Hash table follows (8 × SHA-256, not needed for basic loading)

.code section: load at 0x00100000
  May be LZ-compressed (check ExHeader flag bit 1 of "system control info")
  For initial implementation: assume uncompressed; add LZ11 decompression later
```

**Success criteria:**
- [ ] `load3DSX()` loads a real homebrew 3DSX, maps segments into memory, returns correct entry point
- [ ] `loadNCCH()` reads ExeFS, finds `.code`, maps it to `0x00100000`

---

### Agent: Kernel HLE (`agent/kernel-hle`)

**Owns:** `src/core/kernel/kernel.cpp` only. Do not touch `kernel.hpp`.

**Phase 1 SVC implementation priority** — implement these in order:

```cpp
void Kernel::handleSVC(uint32_t svc) {
    switch (svc) {
    case 0x01: svc_ControlMemory();          break; // Alloc heap
    case 0x08: svc_CreateThread();           break; // Spawn thread
    case 0x09: svc_ExitThread();             break; // Thread suicide
    case 0x0A: svc_SleepThread();            break; // Yield
    case 0x13: svc_CreateMutex();            break; // Mutex (stub OK: return handle 0)
    case 0x17: svc_CreateEvent();            break; // Event (stub OK: return handle 0)
    case 0x23: svc_CloseHandle();            break; // No-op for stubs
    case 0x24: svc_WaitSynchronization1();   break; // Block on object (stub: return 0)
    case 0x32: svc_SendSyncRequest();        break; // IPC (calls ServiceManager)
    case 0x35: svc_GetProcessId();           break; // Return fake PID
    case 0x37: svc_GetResourceLimit();       break; // Return fake handle
    case 0x3C: svc_Break();                  break; // IMPORTANT: log + halt
    case 0x3D: svc_OutputDebugString();      break; // Print to log
    default:
        // Log unimplemented SVCs — crucial for debugging
        fprintf(stderr, "UNIMPLEMENTED SVC: 0x%02X at PC=0x%08X\n",
                svc, impl->cpu->getPC());
        break;
    }
}
```

SVC register convention (ARM AAPCS):
- Arguments in r0–r3
- Return values in r0 (result code), r1 (output value if needed)
- r0 = 0 means success; negative = error code

**The most important SVC for Phase 1:**
```cpp
void Kernel::svc_Break() {
    // SVC 0x3C — called when homebrew hits an unrecoverable error
    // r0 = reason (0 = panic, 1 = assert, 2 = user)
    uint32_t reason = impl->cpu->getReg(0);
    fprintf(stderr, "[KERNEL] svcBreak: reason=%u\n", reason);
    // Signal emulator to halt
    impl->cpu->setSVCHandler(nullptr); // Stop executing
}
```

**Success criteria:**
- [ ] `svcBreak` is handled — emulator logs the reason and stops cleanly
- [ ] `svcOutputDebugString` prints to stderr
- [ ] `svcControlMemory` maps a heap region (can use `Memory::mapPages` directly)

---

### Agent: CI (`agent/ci-testing`)

**Owns:** `.github/workflows/`, `tests/` (create if not present).

**Task 1: GitHub Actions CI**

Create `.github/workflows/build.yml`:
```yaml
name: Build

on:
  push:
    branches: ['main', 'agent/**']
  pull_request:
    branches: ['main']

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive   # Pulls in third_party/dynarmic

      - name: Configure (Unix)
        if: runner.os != 'Windows'
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Configure (Windows)
        if: runner.os == 'Windows'
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022"

      - name: Build
        run: cmake --build build --config Release -j4

      - name: Test
        run: ctest --test-dir build --build-config Release --output-on-failure
```

**Task 2: Basic smoke tests** — add `tests/CMakeLists.txt` and at minimum:
- Memory read/write round-trip test for each region
- Loader format detection test
- CPU init/reset test (just checks it doesn't crash without Dynarmic)

**Success criteria:**
- [ ] CI badge green on all three platforms on `main`
- [ ] PR checks block merge if build fails

---

## Phase 1 Integration Checkpoint

Before merging the four Phase 1 agent PRs, verify in order:
1. `agent/ci-testing` PR merged first (sets up CI so other merges are validated)
2. `agent/loader` merges (3DSX loading works)
3. `agent/cpu-jit` merges (Dynarmic runs)
4. `agent/kernel-hle` merges (SVCs handled)
5. **Integration test:** Run the emulator against a homebrew `.3dsx`. Expected output:
   ```
   [LOADER] Loaded 3DSX: entry=0x00100000
   [CPU] Starting execution at 0x00100000
   [KERNEL] svcBreak: reason=0
   Emulator halted cleanly.
   ```

---

## Phase 2 — First Triangle

**Prerequisite:** Phase 1 checkpoint passed.
**Goal:** A GPU-using homebrew renders a coloured clear screen (no geometry needed yet).
**New worktrees:** `agent/gpu-pica`, `agent/services-hle`, `agent/audio-dsp`

### Agent: GPU (`agent/gpu-pica`)

**Owns:** `src/core/gpu/gpu.cpp` only.

PICA200 GPU is command-driven. Games write command lists via the GSP service.
Each command is a 32-bit register write to the PICA200 register space at `0x1EF00000`.

Priority registers for Phase 2:
```
0x1EF00111  FRAMEBUFFER_TOP_LEFT_1   — physical address of top screen framebuffer
0x1EF00117  FRAMEBUFFER_FORMAT       — pixel format (0 = RGBA8, 1 = RGB8, etc.)
0x1EF001C0  MEMORY_FILL_START_1      — fill color (clear screen)
0x1EF001C2  MEMORY_FILL_END_1        — fill end address
0x1EF001C4  MEMORY_FILL_CONTROL_1    — trigger fill
```

Phase 2 success = respond to a memory fill command and write the correct color to VRAM.

### Agent: Services HLE (`agent/services-hle`)

**Owns:** `src/core/services/service_manager.cpp` only.

Priority services for Phase 2 (the ones every GPU homebrew calls within 50ms of boot):
```
srv:        — GetServiceHandle (dispatch to registered services)
APT:U       — Initialize, Enable, GetSharedFont, ReceiveParameter
gsp::Gpu    — RegisterInterruptRelayQueue, SubmitGxCommand, WriteHWRegs
HID:USER    — GetIPCHandles (stub: return dummy shared mem handle)
fs:USER     — Initialize (stub: return success)
cfg:u       — GetRegionCode, GetSystemModel (stub: return EUR, O3DS)
```

IPC message format (all 3DS IPC uses this):
```
TLS offset 0x80: IPC command header
  Bits 31-16: Command ID
  Bits 15-6:  Normal param count
  Bits 5-0:   Translate param count
r4 = pointer to IPC buffer in TLS
```

### Agent: Audio (`agent/audio-dsp`)

**Owns:** `src/core/audio/audio.cpp` only.

Phase 2 goal: stub DSP enough that games calling `dsp::DSP Initialize` don't crash.
The real DSP (CEVA TeakLite II) can come in Phase 3. For now, return success from all DSP SVCs and produce silence (zeroed PCM samples).

---

## Phase 3 — Commercial Titles Boot

**Prerequisite:** Phase 2 checkpoint. GPU clears screen, services don't crash on init.
**Strategy:** Run → crash → identify missing service or SVC → stub → repeat.

A new `MISSING_SERVICES.md` file will track gaps found during compat testing.
Each title gets its own `agent/compat/<title>` worktree for isolated bring-up work.

New agents in Phase 3:
- **Debug Agent** (`agent/debugger`) — GDB stub, instruction trace ring buffer, service call logger
- **Compat Agents** — one per title, report bugs against Phase 2 agent code via issues

---

## Phase 4 — AI Differentiation (Ongoing Research Track)

Runs in parallel from Phase 2 onward. Does not block Phase 3.

### AI Shader Transpiler (`agent/ai-shader`)

**Goal:** Translate PICA200 shader bytecode → GLSL/MSL/WGSL without hand-writing a decoder.

Track A (ship first): Rule-based transpiler covering all ~30 PICA200 VS opcodes.
Track B (research): Seq2seq transformer model trained on open-source homebrew shaders.

Training data: ~5,000 shader pairs available from open-source homebrew (`.shbin` files ship with source).
Model target: ~10M parameters, ONNX export, C++ inference via ONNX Runtime.

**Owns:** `src/core/ai/` (extend) + new `tools/shader_transpiler/` directory.

### Neural Texture Upscaler (`agent/ai-upscaler`)

The stub is already in `src/core/ai/texture_upscaler.cpp`. The hook is wired.
Phase 4 goal: replace nearest-neighbor with ESPCN or FSRCNN running as a GLSL compute shader (GLES 3.1 on Android, MSL on iOS via Metal).

---

## Worktree Setup

### PowerShell (Windows — run from repo root)

Save as `scripts/setup-worktrees.ps1`:

```powershell
$RepoRoot = git rev-parse --show-toplevel
$ParentDir = Split-Path $RepoRoot -Parent

$Worktrees = @(
    @{ Name = "cpu-jit";      Branch = "agent/cpu-jit" },
    @{ Name = "loader";       Branch = "agent/loader" },
    @{ Name = "kernel-hle";   Branch = "agent/kernel-hle" },
    @{ Name = "ci-testing";   Branch = "agent/ci-testing" },
    @{ Name = "gpu-pica";     Branch = "agent/gpu-pica" },
    @{ Name = "services-hle"; Branch = "agent/services-hle" },
    @{ Name = "audio-dsp";    Branch = "agent/audio-dsp" },
    @{ Name = "debugger";     Branch = "agent/debugger" },
    @{ Name = "ai-shader";    Branch = "agent/ai-shader" }
)

foreach ($wt in $Worktrees) {
    $Path = Join-Path $ParentDir "trident-agent-$($wt.Name)"
    if (Test-Path $Path) {
        Write-Host "⚠️  Skipping $($wt.Name) — already exists at $Path"
        continue
    }
    # Create branch if it doesn't exist
    $branchExists = git show-ref --quiet "refs/heads/$($wt.Branch)"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Creating branch: $($wt.Branch)"
        git branch $wt.Branch
    }
    Write-Host "Adding worktree: $Path"
    git worktree add $Path $wt.Branch

    # Drop AGENT.md
    $AgentMd = @"
# Agent Brief: $($wt.Name)

Branch: ``$($wt.Branch)``
See PLAN.md for full brief, success criteria, and constraints.

## Rules
1. Never edit shared headers — only your src/ subtree.
2. Every commit must keep ``cmake --build build`` green.
3. Open a PR against ``main`` when success criteria are met.

## Build locally
``````
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
``````
"@
    Set-Content -Path (Join-Path $Path "AGENT.md") -Value $AgentMd
    Write-Host "  ✓ $($wt.Name) ready"
}

Write-Host ""
Write-Host "All worktrees set up. List with: git worktree list"
```

Run it:
```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\scripts\setup-worktrees.ps1
```

---

## Immediate Next Steps (Do Today)

In priority order:

1. **Run the worktree setup script** — creates all agent branches in one shot.

2. **Spawn the CPU agent** in `../trident-agent-cpu/`
   - Give it: this PLAN.md, the CPU section above, `cpu.hpp`, `memory.hpp`, Dynarmic's `config.h`
   - The implementation code above is essentially complete — it just needs to be written into `cpu.cpp`

3. **Spawn the Loader agent** in `../trident-agent-loader/`
   - 3DSX format is fully documented above; this is a parsing task, not systems work

4. **Spawn the Kernel agent** in `../trident-agent-kernel/`
   - `svcBreak` and `svcOutputDebugString` are the only two needed to hit the Phase 1 checkpoint

5. **Spawn the CI agent** in `../trident-agent-ci/`
   - GitHub Actions YAML is essentially written above; just needs committing

These four can all run in parallel right now.
