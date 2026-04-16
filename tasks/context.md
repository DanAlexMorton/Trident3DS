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
| Loader | `src/core/loader/loader.cpp` | 🟡 Partial | Initial scaffold | Format detection done; 3DSX and NCCH body not implemented |
| CPU | `src/core/cpu/cpu.cpp` | 🔴 Stub | Initial scaffold | Dynarmic not instantiated; run() returns 0 |
| Kernel | `src/core/kernel/kernel.cpp` | ✅ Phase 1 SVCs implemented | agent/kernel-hle | svcBreak, svcOutputDebugString, svcControlMemory stub, 8 Phase 1 stubs, default logger all implemented |
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

---

## Context Log

## P1-KNL — Implement Essential SVCs
**Date:** 2026-04-17
**Agent:** agent/kernel-hle
**Phase:** 1

### What was built
`src/core/kernel/kernel.cpp` — replaced the no-op `handleSVC()` stub with a full switch dispatch.

**SVCs implemented:**
- `0x01` — `svcControlMemory`: returns fake heap address `0x08000000` in r1, success in r0. Phase 1 stub only.
- `0x08` `svcCreateThread`, `0x09` `svcExitThread`, `0x0A` `svcSleepThread`, `0x13` `svcCreateMutex`, `0x17` `svcCreateEvent`, `0x23` `svcCloseHandle`, `0x24` `svcWaitSynchronization1`, `0x32` `svcSendSyncRequest` — all log `[KERNEL] SVC 0xXX (stubbed)` and return 0.
- `0x3C` — `svcBreak`: reads r0 for reason (PANIC/ASSERT/USER), logs it to stderr, clears the SVC handler (halts CPU), returns 0.
- `0x3D` — `svcOutputDebugString`: reads r0 (ptr) and r1 (len), prints up to 256 bytes from guest memory to stderr, returns 0.
- `default` — logs `[KERNEL] Unimplemented SVC: 0xXX at PC=0xXXXXXXXX` and returns 0.

Also added `#include <cstdio>` which was missing from the scaffold.

### Why this approach
Direct switch statement over the SVC ID — fastest dispatch for the ~11 cases, readable, and matches the style used in similar emulators. No dynamic dispatch or handler table needed for Phase 1 scale.

The stub SVCs log before returning rather than silently returning 0 — this produces useful output when running real homebrew and makes it obvious what's being called.

### Current module state
**Partial** — `handleSVC()` is fully functional for Phase 1 SVCs. `createThread()`, `schedule()`, `svcControlMemory()` (the named method, not the SVC handler), and `svcMapMemoryBlock()` remain as no-op stubs in the class body — they aren't called by `handleSVC()` and are left for Phase 2.

### What the next agent needs to know
1. **Pre-existing build failures elsewhere:** `audio.cpp` is missing `<algorithm>` (for `std::min`), and `emulator.cpp` calls non-existent `Patcher::readFile`/`applyPatch` and passes only 2 args to `memcpy`. These are scaffold errors that must be fixed before Phase 1 integration. The Kernel agent did not introduce them.
2. **`<cstdio>` must be included** — it was not in the original scaffold. Don't assume standard I/O is available just because `kernel.hpp` doesn't include it.
3. **`svcControlMemory` real implementation is Phase 2** — the stub always returns `0x08000000` regardless of the op or size. Phase 2 needs to actually map memory via `Memory::mapPages()`.
4. **`svcOutputDebugString` 256-byte cap** — the current implementation reads at most 256 bytes to avoid unbounded memory access from hostile/buggy homebrew. This is intentional.

### What was explicitly left out
- Thread scheduling (`createThread`/`schedule`) — Phase 2 when real homebrew needs it
- `svcQueryMemory` (`0x02`) — not called in boot path, deferred to Phase 2
- `svcMapMemoryBlock` (`0x1F`) — deferred to Phase 2
- `svcCreateAddressArbiter` (`0x21`) — deferred to Phase 2
- `svcGetProcessId` (`0x35`), `svcGetResourceLimit` (`0x37`/`0x38`) — deferred to Phase 2
