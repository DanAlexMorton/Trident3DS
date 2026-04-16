# Trident3DS — Agent Context Log

This is the living memory of the project. Every agent updates this file when they complete a task.
Its purpose is continuity: a new agent reading this file should be able to understand **what has been built,
why each decision was made, and what the current state of each module is** — without having to read the entire git history.

Read this file after `tasks/lessons.md` and before writing any code.

---

## How to Update This File

At the end of every completed task, append a new entry to the **Context Log** section below.
You are writing for the next developer agent, not for Daniel. Assume they have read `CLAUDE.md` but
have not seen this project before today.

A good entry answers:
- **What did I build or change?** Be specific about files and functions.
- **Why this approach?** What alternatives did you consider and rule out?
- **What is the current state?** Leave the module status honest — working, partial, stubbed.
- **What should the next agent in this area know?** Traps, non-obvious patterns, deferred decisions.
- **What was explicitly left out and why?**

A bad entry says "implemented the CPU" and nothing else.

---

## Context Entry Template

```
## [Task ID] — [Short Title]
**Date:** YYYY-MM-DD
**Agent:** which worktree / branch
**Phase:** 1 / 2 / 3 / 4

### What was built
Specific description of what changed — files, functions, behaviours.

### Why this approach
Rationale for key decisions. What was tried first, what was ruled out, why this solution.

### Current module state
Honest status of every module touched: working | partial | stubbed | broken.

### What the next agent needs to know
Non-obvious patterns, gotchas, deferred decisions, open questions.
Anything that would have saved time if you'd known it at the start.

### What was explicitly left out
Scope decisions — what was deferred to a later phase and why.
```

---

## Module Status Snapshot

This section is a single source of truth for the current state of every module.
**Agents must update the relevant row(s) when their task completes.**

| Module | File | Status | Last updated by | Notes |
|--------|------|--------|-----------------|-------|
| Memory bus | `src/core/memory/memory.cpp` | ✅ Complete | Initial scaffold | Page-table VM, all widths, RA regions |
| ROM Patcher | `src/core/patcher/patcher.cpp` | ✅ Complete | Initial scaffold | IPS, UPS, BPS with CRC32 |
| Loader | `src/core/loader/loader.cpp` | ✅ Complete (Phase 1) | agent/loader | 3DSX and NCCH ExeFS loading implemented; ELF stub unchanged |
| CPU | `src/core/cpu/cpu.cpp` | 🔴 Stub | Initial scaffold | Dynarmic not instantiated; run() returns 0 |
| Kernel | `src/core/kernel/kernel.cpp` | 🔴 Stub | Initial scaffold | handleSVC() exists but does nothing |
| GPU | `src/core/gpu/gpu.cpp` | 🔴 Stub | Initial scaffold | No PICA200 commands processed |
| Audio | `src/core/audio/audio.cpp` | 🔴 Stub | Initial scaffold | generateSamples() returns silence |
| Services | `src/core/services/service_manager.cpp` | 🟡 Skeleton | Initial scaffold | 28 services registered; none handle IPC |
| AI Upscaler | `src/core/ai/texture_upscaler.cpp` | 🟡 Stub | Initial scaffold | Nearest-neighbour fallback only |
| libretro frontend | `src/frontend/libretro/libretro_core.cpp` | ✅ Complete | Initial scaffold | Full retro_* API, RA memory map |
| CI | `.github/workflows/` | 🔴 Missing | — | No GitHub Actions yet |

---

## Context Log

*(No entries yet — this project is just starting. The first entry will be written when Phase 1 agents complete their tasks.)*

---

## P1-LDR — Complete the Loader
**Date:** 2026-04-17
**Agent:** trident-agent-loader / agent/loader
**Phase:** 1

### What was built
`src/core/loader/loader.cpp` — two functions replaced:

- **`load3DSX()`** — full implementation:
  - Parses main header (headerSize, codeSize, rodataSize, bssSize, dataSize)
  - Reads 3 × `RelocHeader` structs at `data[headerSize]` (12 bytes each: numAbs, numRel, reserved)
  - Computes page-aligned load addresses: code @ `0x00100000`, rodata @ next 4 KB boundary, data @ next 4 KB boundary
  - Maps FCRAM pages with `Memory::mapPages` (code = `PageFlags::All`, data/rodata = `ReadWrite`)
  - Copies segment bytes with `Memory::writeBlock`; BSS tail stays zero from `Memory::init()`
  - Applies 3 × 2 relocation tables (ABS then REL per segment). Each entry = `{skip:16, patch:16}`. Word encoding: `tSeg = word >> 28`, `tOfs = word & 0x0FFFFFFF`. ABS: `segBase[tSeg] + tOfs`. REL: `(segBase[tSeg] + tOfs) - patch_addr`
  - Returns `{ success=true, entryPoint=0x00100000 }`

- **`loadNCCH()`** — ExeFS parsing added:
  - Reads ExeFS offset from **NCCH+0x180** (u32, in media units × 0x200). NOTE: the brief said 0x1A0 — that is wrong; 0x1A0 is the RomFS size field. See Lesson 1.
  - Scans 8 ExeFS file entries searching for `".code\0\0\0"` (8-byte name)
  - Loads `.code` bytes at `VADDR_CODE_START` (0x00100000) via `mapPages` + `writeBlock`
  - Comment: `// TODO(Phase 2): add LZ11 decompression for compressed .code sections`
  - Returns `{ success=true, entryPoint=0x00100000, programId }`

No other files were modified. `loader.hpp` is unchanged.

### Why this approach
- **mapPages + writeBlock over direct memcpy:** The Memory API uses a page table; bypassing it (writing directly to `getFCRAM()`) would leave the page table empty and future reads via `read32/readBlock` would return zero. Going through the public API keeps the virtual address space consistent.
- **FCRAM at VADDR_CODE_START offset 0:** We map virtual 0x00100000 → `fcram[0]`. This is the simplest stable convention for Phase 1 homebrew. Phase 2 may need a true physical-to-virtual mapping when the kernel allocates heap at 0x08000000.
- **Segment-indexed relocation (`word >> 28`):** The 3DSX format encodes the target segment index in the upper 4 bits of each pointer word, NOT the source segment. This is required for correct cross-segment references (e.g., code pointing to rodata).

### Current module state
`load3DSX()` — **working** (full relocation pipeline).  
`loadNCCH()` — **working** (ExeFS parse, no LZ11 decompression).  
`loadELF()` — **stubbed** (unchanged, still returns failure).  
`detectFormat()`, `loadFile()`, `load()`, `loadNCSD()` — **working** (unchanged from scaffold).

### What the next agent needs to know
1. **ExeFS offset is 0x180, not 0x1A0.** The task brief had it wrong. Check Lesson 1 before touching any NCCH parsing.
2. **3DSX ABS reloc = `segBase[word >> 28] + (word & 0x0FFFFFFF)`.** Not a simple "add code base". See Lesson 2.
3. **Always `pageAlignSize()` before `mapPages`.** `mapPages` silently maps zero pages if size is not a 4 KB multiple. See Lesson 3.
4. **Pre-existing build errors in `audio.cpp` and `emulator.cpp` are unrelated to loader.** They existed before this task. Details: `audio.cpp:15 'min' not found` (missing `#include <algorithm>`); `emulator.cpp:89 Patcher::readFile not found` (API mismatch in Patcher usage).
5. **Phase 2 must add LZ11 decompression** for `.code` sections that have the compression flag set in the NCCH ExHeader (bit 1 of system control info flags).

### What was explicitly left out
- LZ11 decompression for NCCH `.code` — deferred to Phase 2 via TODO comment
- ELF loading — not in Phase 1 scope
- BSS explicit zeroing — relies on `Memory::init()` zero-filling FCRAM, which is documented
- ExHeader parsing (stack size, stack address, memory type) — not needed to reach `svcBreak` in Phase 1

---

## Key Architectural Decisions (Running Record)

This section records significant decisions that shaped the architecture.
Add an entry whenever a decision is made that a future agent might otherwise question or reverse.

---

### Decision 1 — Dynarmic over interpreter for Phase 1
**Date:** 2026-04-17
**Decision:** Use Dynarmic JIT for the ARM11 from day one, not an interpreter.
**Rationale:** An interpreter would be faster to bootstrap but would need to be thrown away. Dynarmic is already a git submodule, already linked in CMake, and has a clean callback API that works well for Phase 1 without a page table. The Phase 1 implementation uses callbacks only (slower but correct); Phase 2 will add the page-table fast path.
**Alternatives considered:** A simple fetch-decode-execute interpreter using a switch statement on the opcode. Ruled out because it would delay JIT work and Dynarmic's Phase 1 cost is just one `.cpp` file.
**Consequences:** The CPU agent's first task is slightly harder (Dynarmic API vs trivial switch), but all subsequent phases benefit from a real JIT from the start.

---

### Decision 2 — Callbacks for Phase 1 memory, page table for Phase 2
**Date:** 2026-04-17
**Decision:** Phase 1 CPU uses Dynarmic's `MemoryRead*/Write*` callbacks. Phase 2 will wire the page table.
**Rationale:** Dynarmic supports two memory access modes: callbacks (correct but slower) and a host page table (fast). `Memory::pageTable` uses `PageEntry{ptr, flags}` structs, but Dynarmic's page table wants plain `uint8_t*`. They're incompatible without an extraction step. Phase 1 doesn't need the performance, so callbacks are cleaner.
**Consequences:** Phase 2 GPU/services agent will need to maintain a parallel `uint8_t*` array that shadows `Memory::pageTable` for Dynarmic. Document this when it's implemented.

---

### Decision 3 — pimpl pattern everywhere
**Date:** 2026-04-17
**Decision:** All core modules use the pimpl (pointer-to-implementation) idiom — `struct Impl` inside the `.cpp`, hidden behind `std::unique_ptr<Impl>`.
**Rationale:** Keeps headers stable. Agents can change implementation internals without causing recompiles across the codebase. This is especially important in an agent/worktree model where multiple branches are in flight simultaneously.
**Consequences:** Slightly more boilerplate per class. Every agent must follow this pattern — don't put `Dynarmic::A32::Jit` in a header, put it in the `Impl` struct.
