# Trident3DS — Project Memory

Trident3DS is a from-scratch 3DS emulator targeting libretro (RetroArch), iOS (via JIT entitlements), and Android. It started as a fork of Panda3DS (3dsTrident) but was rejected from the libretro buildbot for not being original enough. This repo is the original rewrite. The differentiators are: AI shader transpiler (PICA200 → GLSL/MSL), neural texture upscaling, and first-class iOS support.

**GitHub:** `https://github.com/DanAlexMorton/Trident3DS`

---

## Agent Instructions

**Read these three files before starting any task — in this order:**
1. `tasks/lessons.md` — real mistakes and rules; if your area is affected, the rule applies to you
2. `tasks/context.md` — what has actually been built, current module status, key decisions made
3. This file (`CLAUDE.md`) — the target architecture, constants, contracts, and gotchas

**For any non-trivial task (3+ steps):**
1. Write a plan in `tasks/todo.md` before writing any code
2. State which files will change, which will NOT be touched, and why
3. Get confirmation before executing
4. Mark items complete as you go
5. Run the verification checklist before calling anything done
6. **Run the end-of-task self-reflection before opening a PR** (see below)
7. **Update `tasks/context.md`** — module status table + context log entry (see below)
8. If corrected by Daniel: append a lesson to `tasks/lessons.md` immediately

**End-of-task self-reflection (mandatory before every PR):**

Answer these five questions after the verification checklist passes:
1. What was harder than I expected, and why?
2. What assumption did I make that turned out to be wrong or risky?
3. What would I warn the next agent working in this area?
4. What did I discover that isn't obvious from reading the headers?
5. Did I almost break anything? What saved it?

Rate each non-trivial answer **1–10** (10 = would have caused a build break or wrong hardware behaviour; 5 = subtle bug avoided; 1 = minor style note). Write any answer rated 4+ as a lesson in `tasks/lessons.md`. If nothing is notable, write one line: `Reflection [N] — [date] — [agent] — No new lessons. Task was straightforward.`

**Context update (mandatory after every completed task):**
1. Open `tasks/context.md`
2. Update the **Module Status Snapshot** table for every module you touched — status, branch name, and notes
3. Append a new **Context Log** entry using the template in that file
4. If you made a significant architectural decision, add it to the **Key Architectural Decisions** section
The context log is how the next agent knows what you built and why. Without it they reverse-engineer from git blame.

**Verification checklist (required before marking any task done):**
- [ ] `cmake --build build` succeeds with zero errors and zero new warnings
- [ ] No edits made to any file in `include/` or any other agent's `src/core/` subtree
- [ ] The change compiles on the agent's own branch — do NOT push broken code
- [ ] If you added a new function, there is a corresponding test or a manual test description
- [ ] Check: "Would a senior emulator dev approve this PR?" If not, fix it first.

**Code standards:**
- Follow the existing pimpl pattern used in `cpu.cpp`, `kernel.cpp`, etc.
- Use `uint32_t`, `uint16_t`, `uint8_t` — not `int`, `short`, `char` for hardware values
- All hardware constants go in headers, not in `.cpp` files
- Namespace everything under `Trident::`
- No `printf` — use `fprintf(stderr, "[MODULE] message\n")` with module prefix

---

## Tech Stack

| Layer | Technology |
|---|---|
| Language | C++20 |
| Build | CMake 3.16+, cross-platform |
| CPU JIT | Dynarmic (ARM11 ARMv6K, git submodule at `third_party/dynarmic`) |
| GPU | PICA200 stub → OpenGL 4.1 / GLES 3.1 / Metal (Phase 2) |
| Audio | CEVA TeakLite II DSP stub → HLE / LLE (Phase 3) |
| Frontend | libretro API (`include/trident/libretro.h`) |
| AI | Neural shader transpiler (Phase 4), texture upscaler stub (Phase 2) |
| iOS JIT | `MAP_JIT` + `pthread_jit_write_protect_np`, entitlements in `ios/` |
| Platform | Windows (primary dev), macOS, Linux, Android, iOS |

---

## Repository Layout

```
Trident3DS/
├── CMakeLists.txt                  # Cross-platform build — Dynarmic already wired in
├── CLAUDE.md                       # This file — read every session
├── PLAN.md                         # Strategic roadmap — read for architecture context
├── PROGRESS.md                     # Project history and status log
├── third_party/dynarmic/           # ARM11 JIT submodule (Panda3DS fork)
├── include/trident/libretro.h      # libretro API header
├── ios/Trident.entitlements        # iOS JIT entitlements
├── tasks/
│   ├── lessons.md                  # READ THIS FIRST — accumulated mistake log
│   ├── todo.md                     # Active task plans
│   └── agent-briefs.md             # All phase agent briefs
└── src/core/
    ├── emulator.hpp/.cpp           # Orchestrator — owns all subsystems
    ├── memory/memory.hpp/.cpp      # ✅ COMPLETE — page-table VM, all read/write widths
    ├── cpu/cpu.hpp/.cpp            # 🔴 STUB — Dynarmic not wired yet (Phase 1 unlock)
    ├── kernel/kernel.hpp/.cpp      # 🔴 STUB — SVCs not implemented
    ├── gpu/gpu.hpp/.cpp            # 🔴 STUB — PICA200 not implemented
    ├── audio/audio.hpp/.cpp        # 🔴 STUB — DSP not implemented
    ├── loader/loader.hpp/.cpp      # 🟡 PARTIAL — 3DSX and NCCH body not implemented
    ├── patcher/patcher.hpp/.cpp    # ✅ COMPLETE — IPS/UPS/BPS
    ├── services/service_manager.hpp/.cpp  # 🟡 SKELETON — 28 services registered, none handle IPC
    └── ai/texture_upscaler.hpp/.cpp       # 🟡 STUB — nearest-neighbour fallback only
```

---

## Critical Path

**Nothing works until Dynarmic is wired.** The entire Phase 1 unblocks with one file: `src/core/cpu/cpu.cpp`. Dynarmic is already a git submodule. CMakeLists.txt already links it. The only missing piece is implementing `DynarmicCallbacks` and instantiating `Dynarmic::A32::Jit`. See `PLAN.md §CPU Agent` for the complete implementation.

---

## Key Hardware Constants

| Symbol | Value | Notes |
|---|---|---|
| `CPU_CLOCK_RATE` | 268,111,856 Hz | ARM11 MPCore |
| `CYCLES_PER_FRAME` | 4,468,530 | @ 60 fps |
| `FCRAM_SIZE` | 128 MB | 256 MB on New 3DS |
| `VRAM_SIZE` | 6 MB | GPU memory |
| `AUDIO_SAMPLE_RATE` | 32,768 Hz | DSP output |
| `TOP_SCREEN` | 400 × 240 | Top display |
| `BOTTOM_SCREEN` | 320 × 240 | Touch display |
| `PAGE_BITS` | 12 | 4KB pages — matches Dynarmic exactly |

---

## Memory Map (ARM11 virtual)

| Range | Region |
|---|---|
| `0x00100000–0x03FFFFFF` | Application code |
| `0x08000000–0x0FFFFFFF` | Heap |
| `0x10000000–0x10FFFFFF` | IO / MMIO registers |
| `0x14000000–0x1BFFFFFF` | Linear heap |
| `0x1F000000–0x1F5FFFFF` | VRAM |
| `0x1FF00000–0x1FF7FFFF` | DSP memory |
| `0x1FF80000–0x1FFFFFFF` | AXI WRAM |
| `0x1FF82000–0x1FFFFFFF` | TLS area |

---

## Interface Contracts (Public Headers — NEVER EDIT)

These headers define component boundaries. Agents read them to understand what to implement, but **no agent may change them**. A header change requires a human PR review.

**`src/core/cpu/cpu.hpp`** — CPU wrapper; `init(Memory&)`, `run(cycles)`, `setSVCHandler(cb)`, `setPC/getReg` etc.

**`src/core/memory/memory.hpp`** — Full page-table VM; `read8/16/32/64`, `write8/16/32/64`, `mapPages`, `getPointer`, `getFCRAM`.

**`src/core/kernel/kernel.hpp`** — HLE kernel; `init(CPU&, Memory&)`, `handleSVC(svc_id)`.

**`src/core/gpu/gpu.hpp`** — GPU stub; `init(Memory&)`, `submitCommandList(addr, size)`, `vSync()`.

**`src/core/loader/loader.hpp`** — ROM loader; `load(data, Memory&)`, `loadFile(path, Memory&)`, returns `LoadResult{success, entryPoint, programId}`.

---

## Dynarmic Integration Notes

The Dynarmic API lives at `third_party/dynarmic/src/dynarmic/interface/A32/config.h`.

Key points:
- Implement `Dynarmic::A32::UserCallbacks` — forward all `MemoryRead*/Write*` to `Memory::read*/write*`
- `CallSVC(swi)` → call the `SVCCallback` set via `cpu.setSVCHandler()`
- `arch_version = Dynarmic::A32::ArchVersion::v6K` — ARM11 is ARMv6K
- `PAGE_BITS = 12` in Dynarmic exactly matches `PAGE_BITS = 12` in `memory.hpp` — they're compatible
- Do NOT try to share Memory's `PageEntry` array directly with Dynarmic — `PageEntry` has flags, Dynarmic wants `uint8_t*`. Use callbacks for Phase 1; add the fast-path page table in Phase 2.

---

## SVC Register Convention

ARM AAPCS calling convention for SVCs:
- Arguments: `r0`–`r3`
- Return value: `r0` (result code — 0 = success, negative = error)
- Second return value: `r1` (e.g. handle output)
- Read args with `cpu->getReg(0)` etc.; write results with `cpu->setReg(0, result)`

---

## Essential SVCs for Phase 1

| SVC | ID | Notes |
|---|---|---|
| `svcBreak` | `0x3C` | Log reason (r0) and halt — most critical |
| `svcOutputDebugString` | `0x3D` | Print r0=ptr, r1=len to stderr |
| `svcControlMemory` | `0x01` | Map heap; op in r0, addr in r1, size in r3 |
| `svcCreateThread` | `0x08` | Stub OK for Phase 1 — return fake handle |
| `svcSleepThread` | `0x0A` | No-op stub OK |
| `svcSendSyncRequest` | `0x32` | Routes IPC to ServiceManager |

---

## 3DSX Loading (Homebrew Format)

```
Header offsets (all little-endian):
  0x00  magic "3DSX"
  0x04  header_size (u16) — usually 0x20 or 0x2C
  0x10  code_size (u32)
  0x14  rodata_size (u32)
  0x18  bss_size (u32)
  0x1C  data_size (u32)

Segments loaded at:
  code:   0x00100000
  rodata: 0x00100000 + code_size (aligned to page)
  data:   rodata_end (aligned to page)

After segments: relocation tables (skip/patch pairs for ABS and REL relocations)
```

---

## Gotchas and Rules

- **Never edit `include/` or another agent's `.hpp` file** — these are contracts
- **Namespace everything** — all code must be inside `namespace Trident { }`
- **Use hardware types** — `uint32_t` not `int` for addresses, registers, and sizes
- **Dynarmic arch version** — must be `v6K`, not the default `v8` — the 3DS is ARM11
- **Memory is little-endian** — the 3DS ARM11 is little-endian; Dynarmic handles byte-swap for you
- **CPSR on reset** — set to `0x00000010` (User mode, ARM state) not 0
- **Never push broken code** — each agent branch must always compile cleanly
- **Log unimplemented SVCs** — don't silently ignore them; `fprintf(stderr, "[KERNEL] Unimplemented SVC: 0x%02X\n", svc)` at minimum

---

## Worktree Structure

| Worktree path | Branch | Phase |
|---|---|---|
| `../trident-agent-cpu-jit` | `agent/cpu-jit` | 1 |
| `../trident-agent-loader` | `agent/loader` | 1 |
| `../trident-agent-kernel-hle` | `agent/kernel-hle` | 1 |
| `../trident-agent-ci-testing` | `agent/ci-testing` | 1 |
| `../trident-agent-gpu-pica` | `agent/gpu-pica` | 2 |
| `../trident-agent-services-hle` | `agent/services-hle` | 2 |
| `../trident-agent-audio-dsp` | `agent/audio-dsp` | 2 |
| `../trident-agent-debugger` | `agent/debugger` | 3 |
| `../trident-agent-ai-shader` | `agent/ai-shader` | 4 |
