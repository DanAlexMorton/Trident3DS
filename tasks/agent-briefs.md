# Trident3DS — All Agent Briefs

Complete task list across all phases. One brief per agent worktree.

---

## MANDATORY AGENT START SEQUENCE — Read These Files Before Writing Any Code

### Step 1 — Read `tasks/lessons.md`
Real mistakes made during development. Each has a Rule. If your task touches an affected area, apply the rule.

### Step 2 — Read `CLAUDE.md`
Full project memory: tech stack, module status, hardware constants, memory map, interface contracts, gotchas.

### Step 3 — Read `PLAN.md`
Strategic roadmap. Know your phase, what was built before you, and what comes after.

### Step 4 — Read your `AGENT.md` (in your worktree root)
Your specific mission, file ownership, and success criteria.

### After any correction
Append a lesson to `tasks/lessons.md` immediately. Be specific.

---

## HOW TO USE THESE BRIEFS

**Step 1 — Create worktrees (once, from repo root in Git Bash):**
```bash
git branch agent/cpu-jit && git worktree add ../trident-agent-cpu-jit agent/cpu-jit
git branch agent/loader && git worktree add ../trident-agent-loader agent/loader
git branch agent/kernel-hle && git worktree add ../trident-agent-kernel-hle agent/kernel-hle
git branch agent/ci-testing && git worktree add ../trident-agent-ci-testing agent/ci-testing
```

**Step 2 — Open each worktree in a separate VS Code window:**
```
code C:\Users\donal\Work\Games\trident-agent-cpu-jit
code C:\Users\donal\Work\Games\trident-agent-loader
code C:\Users\donal\Work\Games\trident-agent-kernel-hle
code C:\Users\donal\Work\Games\trident-agent-ci-testing
```

**Step 3 — In each window, open Copilot Chat (`Ctrl+Shift+I`), switch to Agent mode, paste the brief below.**

**Step 4 — Check status (Git Bash from repo root):**
```bash
git worktree list
git log --oneline agent/cpu-jit ^main
```

**Step 5 — Review and merge when all four Phase 1 agents are done:**
```bash
git diff main..agent/cpu-jit       # review changes
git checkout main
git merge --no-ff agent/cpu-jit    # merge in order: ci → loader → cpu → kernel
```

Tasks marked **(Parallel)** run simultaneously. Tasks marked **(Sequential)** wait for a preceding merge.

---
---

# PHASE 1 — PROOF OF LIFE
# Goal: A .3dsx homebrew boots, hits svcBreak, emulator logs it and exits cleanly.
# All four Phase 1 agents run in PARALLEL.

---

## BRIEF P1-CPU: Wire Dynarmic into cpu.cpp

**Worktree:** `../trident-agent-cpu-jit` | **Branch:** `agent/cpu-jit` | **(Parallel)**

⚠️ START SEQUENCE: Read `tasks/lessons.md` → `CLAUDE.md` → `PLAN.md §CPU Agent` → `AGENT.md`. Do this before writing any code.

**Owns:** `src/core/cpu/cpu.cpp` only. Do not touch `cpu.hpp` or any other file.

**Context:**
- Dynarmic is already a git submodule at `third_party/dynarmic/`
- CMakeLists.txt already links it — no CMake changes needed
- The Dynarmic API is at `third_party/dynarmic/src/dynarmic/interface/A32/config.h`
- Read that header before starting

**What to implement:**

1. Add `DynarmicCallbacks` struct inside `cpu.cpp` (not in any header) that inherits `Dynarmic::A32::UserCallbacks` and forwards all `MemoryRead*/Write*` calls to the `Memory*` pointer.

2. `CallSVC(uint32_t swi)` in the callbacks should call `impl->svcHandler(swi)` if set.

3. `arch_version` must be `Dynarmic::A32::ArchVersion::v6K` — the 3DS is ARM11 (ARMv6K), not v8.

4. On `reset()`, set CPSR to `0x00000010` (User mode, ARM state) — not zero.

5. `CPU::run(cycles)` should set `callbacks->ticksRemaining = cycles`, call `jit->Run()`, and return `cycles - callbacks->ticksRemaining`.

6. `CPU::invalidateCacheRange` and `CPU::clearCache` should call the matching Dynarmic methods.

**The full implementation shape is in `PLAN.md §CPU Agent`.** Read it — the code is essentially written for you.

**VERIFICATION CHECKLIST:**
- [ ] `cmake --build build --config Debug` succeeds with Dynarmic compiled in
- [ ] `CPU::init(mem)` does not crash — Jit is constructed
- [ ] `CPU::run(100)` returns > 0 when PC points at a NOP sled mapped in memory
- [ ] SVC callback fires when the JIT executes a SVC instruction
- [ ] `cpu.hpp` is unchanged
- [ ] No other files outside `src/core/cpu/cpu.cpp` were modified

---

## BRIEF P1-LDR: Complete the Loader

**Worktree:** `../trident-agent-loader` | **Branch:** `agent/loader` | **(Parallel)**

⚠️ START SEQUENCE: Read `tasks/lessons.md` → `CLAUDE.md` → `PLAN.md §Loader Agent` → `AGENT.md`.

**Owns:** `src/core/loader/loader.cpp` only. Do not touch `loader.hpp`.

**Task 1: Implement `load3DSX()`**

3DSX header layout (all little-endian u32 unless noted):
```
0x00  magic "3DSX" (4 bytes)
0x04  header_size (u16)
0x06  reloc_header_size (u16) — always 0x0C
0x08  format_version (u32)
0x0C  flags (u32)
0x10  code_size (u32)      ← bytes of code segment
0x14  rodata_size (u32)
0x18  bss_size (u32)
0x1C  data_size (u32)      ← includes BSS
```
Raw segments follow the header in order: code, rodata, data.
Load addresses: code at `0x00100000`, rodata at `code_end` (page-aligned), data at `rodata_end` (page-aligned).

After segments: relocation tables. Each relocation is a 4-byte entry:
- Bits 31–16: skip count (number of words to skip)
- Bits 15–0: patch count (number of words to patch after skipping)
- ABS relocations: add the segment's load address to each patched word
- REL relocations: subtract the patch word's own address (make PC-relative)

Two relocation tables per segment (ABS then REL). Three segments = six tables total.

Entry point = code load address (`0x00100000`).

**Task 2: Complete `loadNCCH()`**

The current stub detects the magic but doesn't load anything. Add ExeFS parsing:

```
ExeFS offset in NCCH header: at NCCH+0x1A0 (u32, in media units × 0x200)
ExeFS header layout (8 × 16-byte file entries):
  0x00  file name (8 bytes, null-padded, e.g. ".code\0\0\0")
  0x08  file offset (u32, from ExeFS header + 0x200)
  0x0C  file size (u32)
Find the ".code" entry. Load it at VADDR_CODE_START (0x00100000).
Entry point = VADDR_CODE_START.
```

Phase 1 assumption: `.code` is uncompressed. Add a comment: `// TODO(Phase 2): add LZ11 decompression for compressed .code sections`.

**VERIFICATION CHECKLIST:**
- [ ] `load3DSX()` successfully parses a small synthetic 3DSX (create one in tests if needed)
- [ ] Returned `LoadResult.success == true` and `entryPoint == 0x00100000`
- [ ] Segments are correctly written into Memory at the right virtual addresses
- [ ] `loadNCCH()` finds the ExeFS, locates `.code`, maps it at `0x00100000`
- [ ] `loader.hpp` is unchanged
- [ ] No other files modified

---

## BRIEF P1-KNL: Implement Essential SVCs

**Worktree:** `../trident-agent-kernel-hle` | **Branch:** `agent/kernel-hle` | **(Parallel)**

⚠️ START SEQUENCE: Read `tasks/lessons.md` → `CLAUDE.md` → `PLAN.md §Kernel Agent` → `AGENT.md`.

**Owns:** `src/core/kernel/kernel.cpp` only. Do not touch `kernel.hpp`.

**The SVC register convention (ARM AAPCS):**
- Arguments in `r0`–`r3` — read with `impl->cpu->getReg(0)` etc.
- Return in `r0` (result code: 0 = success, negative = error code)
- Second output in `r1` (e.g. a handle)

**Implement these SVCs in order of priority:**

### SVC 0x3C — svcBreak (MOST IMPORTANT)
```cpp
// r0 = reason (0=PANIC, 1=ASSERT, 2=USER)
uint32_t reason = impl->cpu->getReg(0);
const char* reasons[] = {"PANIC", "ASSERT", "USER"};
fprintf(stderr, "[KERNEL] svcBreak: %s\n",
        reason < 3 ? reasons[reason] : "UNKNOWN");
// Signal halt by clearing the SVC handler so CPU stops calling back
impl->cpu->setSVCHandler(nullptr);
impl->cpu->setReg(0, 0); // return success
```

### SVC 0x3D — svcOutputDebugString
```cpp
// r0 = string ptr (virtual address), r1 = length
uint32_t ptr = impl->cpu->getReg(0);
uint32_t len = impl->cpu->getReg(1);
// Read bytes from memory and print them
for (uint32_t i = 0; i < len && i < 256; i++) {
    fputc(impl->memory->read8(ptr + i), stderr);
}
fputc('\n', stderr);
impl->cpu->setReg(0, 0);
```

### SVC 0x01 — svcControlMemory (stub)
```cpp
// r0 = op, r1 = addr0, r2 = addr1, r3 = size, (stack) = perms
// For Phase 1: just return success. Real heap mapping comes in Phase 2.
impl->cpu->setReg(1, 0x08000000); // return a fake heap address in r1
impl->cpu->setReg(0, 0);          // success
```

### SVCs 0x08, 0x09, 0x0A, 0x13, 0x17, 0x23, 0x24, 0x32 — stubs
These are all called by most homebrew on boot. Return success (r0=0) for all.
Log them: `fprintf(stderr, "[KERNEL] SVC 0x%02X (stubbed)\n", svc);`

### Unimplemented SVCs
```cpp
default:
    fprintf(stderr, "[KERNEL] Unimplemented SVC: 0x%02X at PC=0x%08X\n",
            svc, impl->cpu->getPC());
    impl->cpu->setReg(0, 0); // return success to avoid crashes
    break;
```

**VERIFICATION CHECKLIST:**
- [ ] `svcBreak` prints the reason and halts cleanly (SVC handler cleared)
- [ ] `svcOutputDebugString` prints to stderr
- [ ] `svcControlMemory` returns without crashing
- [ ] All other Phase 1 SVCs return 0 with a log message
- [ ] Unknown SVCs log and return 0 — never crash
- [ ] `kernel.hpp` is unchanged
- [ ] No other files modified

---

## BRIEF P1-CI: GitHub Actions CI + Smoke Tests

**Worktree:** `../trident-agent-ci-testing` | **Branch:** `agent/ci-testing` | **(Parallel)**

⚠️ START SEQUENCE: Read `tasks/lessons.md` → `CLAUDE.md` → `PLAN.md §CI Agent` → `AGENT.md`.

**Owns:** `.github/workflows/`, `tests/` (create if not present), `CMakeLists.txt` (tests section only).

**Task 1: Create `.github/workflows/build.yml`**

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
      fail-fast: false
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DTRIDENT_USE_DYNARMIC=ON

      - name: Build
        run: cmake --build build --config Release -j4

      - name: Test
        run: ctest --test-dir build --build-config Release --output-on-failure
```

**Task 2: Add test infrastructure**

1. Add `FetchContent` for Catch2 in CMakeLists.txt (inside a `if(BUILD_TESTING)` guard)
2. Create `tests/test_memory.cpp` — verify read/write round-trip for each width (8, 16, 32, 64)
3. Create `tests/test_loader.cpp` — verify `detectFormat()` returns correct enum for each magic
4. Create `tests/test_cpu.cpp` — verify `CPU::init()` succeeds and `getPC()` returns 0 after reset

Keep tests minimal — they just need to compile and not crash. Real integration tests come later.

**VERIFICATION CHECKLIST:**
- [ ] CI YAML is valid (check with a YAML linter or push and see if Actions runs)
- [ ] CI runs on all three platforms when pushed to `agent/ci-testing`
- [ ] Tests compile and pass on your local machine
- [ ] `cmake --build build` still works for other agents (don't change existing targets)
- [ ] No production source files modified

---
---

# PHASE 2 — FIRST TRIANGLE
# Prerequisite: Phase 1 integration checkpoint passed.
# Goal: GPU homebrew renders a coloured clear screen.
# Three new agents. Run in parallel after Phase 1 merges.

---

## BRIEF P2-GPU: PICA200 GPU Bring-Up

**Worktree:** `../trident-agent-gpu-pica` | **Branch:** `agent/gpu-pica`

⚠️ START SEQUENCE: Read `tasks/lessons.md` → `CLAUDE.md` → `PLAN.md §GPU Agent` → `AGENT.md`.

**Owns:** `src/core/gpu/gpu.cpp` only.

**Phase 2 goal:** Respond to a GPU memory-fill command (clear screen) and write the correct colour to VRAM at `0x1F000000`. Confirmed by screenshot test comparing framebuffer pixel colour.

PICA200 is command-driven. Games write 32-bit register writes to `0x1EF00000` via the GSP service. The GPU agent receives these via `submitCommandList(addr, size)`.

Priority registers:
```
0x1EF001C0  MEMORY_FILL_START_1   — start address of fill region
0x1EF001C2  MEMORY_FILL_END_1     — end address
0x1EF001C4  MEMORY_FILL_CONTROL_1 — fill value and trigger bit
0x1EF00111  FRAMEBUFFER_TOP_LEFT  — physical address of top screen FB
0x1EF00117  FRAMEBUFFER_FORMAT    — 0=RGBA8, 1=RGB8
```

Phase 2 success = receive a fill command, write the colour to the correct VRAM region, `renderFrame()` returns a valid pointer.

**VERIFICATION CHECKLIST:**
- [ ] `submitCommandList()` processes a memory-fill sequence without crashing
- [ ] The correct colour appears at the top-screen framebuffer address in VRAM
- [ ] `getTopFramebuffer()` returns a non-null pointer
- [ ] `gpu.hpp` unchanged, no other files modified

---

## BRIEF P2-SVC: HLE Services (APT, GSP, FS, CFG, HID)

**Worktree:** `../trident-agent-services-hle` | **Branch:** `agent/services-hle`

⚠️ START SEQUENCE: Read `tasks/lessons.md` → `CLAUDE.md` → `PLAN.md §Services Agent` → `AGENT.md`.

**Owns:** `src/core/services/service_manager.cpp` only.

**Priority services** (called within the first 50ms of any GPU homebrew):

| Service | Key commands | Phase 2 requirement |
|---|---|---|
| `srv:` | GetServiceHandle | Route to registered services |
| `APT:U` | Initialize, Enable, GetSharedFont | Return success; return null for shared font |
| `gsp::Gpu` | RegisterInterruptRelayQueue, SubmitGxCommand | Forward GxCommands to GPU |
| `HID:USER` | GetIPCHandles | Return dummy shared memory handle |
| `fs:USER` | Initialize | Return success |
| `cfg:u` | GetSystemModel, GetRegionCode | Return O3DS (0), EUR (2) |

IPC message format: TLS offset `0x80` holds the IPC buffer. `r4` points to it. Command ID is in bits 31–16 of the first word.

**VERIFICATION CHECKLIST:**
- [ ] `srv:GetServiceHandle` routes to correct service handler without crash
- [ ] `APT:Initialize` returns 0
- [ ] `gsp::Gpu:SubmitGxCommand` calls `gpu->submitCommandList()`
- [ ] `cfg:u:GetSystemModel` returns 0 (O3DS)
- [ ] `service_manager.hpp` unchanged

---

## BRIEF P2-AUD: DSP Audio Stub

**Worktree:** `../trident-agent-audio-dsp` | **Branch:** `agent/audio-dsp`

⚠️ START SEQUENCE: Read `tasks/lessons.md` → `CLAUDE.md` → `PLAN.md §Audio Agent` → `AGENT.md`.

**Owns:** `src/core/audio/audio.cpp` only.

**Phase 2 goal:** `dsp::DSP:Initialize` returns success. `generateSamples()` returns correctly-sized silence (zeroed PCM buffer at 32768 Hz stereo).

Real DSP emulation (CEVA TeakLite II) is Phase 3. Phase 2 just needs the audio system to not crash when games init it.

**VERIFICATION CHECKLIST:**
- [ ] DSP Initialize SVC returns 0
- [ ] `generateSamples()` fills buffer with zeroed PCM at correct size
- [ ] No audio-related crash in a GPU homebrew boot sequence
- [ ] `audio.hpp` unchanged

---
---

# PHASE 3 — COMMERCIAL TITLES BOOT
# Prerequisite: Phase 2 integration. GPU clears screen, services don't crash.
# Strategy: Run → crash → identify gap → stub → repeat.
# New file: MISSING_SERVICES.md tracks gaps discovered during compat work.

---

## BRIEF P3-DBG: Debugger and Inspector Tools

**Worktree:** `../trident-agent-debugger` | **Branch:** `agent/debugger`

**Owns:** `src/debug/` (create), `tools/inspector/` (create).

**Deliverables:**
- GDB stub on port 1234 — lets Copilot/Claude Code debug the ARM11 guest via GDB protocol
- Instruction trace ring buffer (last 1000 instructions, logged on crash)
- Service call logger with arguments (reads from kernel SVC dispatch)
- PICA200 command list logger (reads from GPU submitCommandList)

---
---

# PHASE 4 — AI DIFFERENTIATION
# Runs in parallel from Phase 2 onward. Does not block Phase 3.

---

## BRIEF P4-SHD: AI Shader Transpiler

**Worktree:** `../trident-agent-ai-shader` | **Branch:** `agent/ai-shader`

**Owns:** `src/core/ai/` (extend), `tools/shader_transpiler/` (create).

**Track A (ship first):** Rule-based transpiler — hand-map all ~30 PICA200 VS opcodes to GLSL. Gives correct output immediately. Used as ground truth for Track B.

PICA200 VS opcode list: `ADD, DP3, DP4, DPH, DST, EX2, LG2, LIT, MUL, SGE, SLT, FLR, MAX, MIN, RCP, RSQ, MOVA, MOV, NOP, END, EMIT, SETEMIT, JMP, JMPC, JMPU, CALL, CALLC, CALLU, IFU, IFC, LOOP, BREAK, BREAKC`

**Track B (research):** Seq2seq transformer (~10M params). Input: PICA200 bytecode tokens. Output: GLSL tokens. Training data: open-source homebrew `.shbin` files paired with source GLSL. Export to ONNX for C++ inference.

---

## BRIEF P4-TEX: Neural Texture Upscaler

**Worktree:** (extend `agent/ai-shader` or new branch)
**Owns:** `src/core/ai/texture_upscaler.cpp`

Replace the current nearest-neighbour stub with ESRCNN or FSRCNN running as a GLSL compute shader (GLES 3.1 on Android, MSL compute on iOS via Metal). The hook point is already wired in `texture_upscaler.cpp`.
