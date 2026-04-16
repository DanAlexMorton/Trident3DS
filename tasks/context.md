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
| CPU | `src/core/cpu/cpu.cpp` | ✅ Complete | agent/cpu-jit | Dynarmic JIT instantiated, ARMv6K, callbacks wired, SVC dispatch, cache invalidation |
| Kernel | `src/core/kernel/kernel.cpp` | 🔴 Stub | Initial scaffold | handleSVC() exists but does nothing |
| GPU | `src/core/gpu/gpu.cpp` | 🔴 Stub | Initial scaffold | No PICA200 commands processed |
| Audio | `src/core/audio/audio.cpp` | 🔴 Stub | Initial scaffold | generateSamples() returns silence |
| Services | `src/core/services/service_manager.cpp` | 🟡 Skeleton | Initial scaffold | 28 services registered; none handle IPC |
| AI Upscaler | `src/core/ai/texture_upscaler.cpp` | 🟡 Stub | Initial scaffold | Nearest-neighbour fallback only |
| libretro frontend | `src/frontend/libretro/libretro_core.cpp` | ✅ Complete | Initial scaffold | Full retro_* API, RA memory map |
| CI | `.github/workflows/` | 🔴 Missing | — | No GitHub Actions yet |

---

## Context Log

---

## P1-CPU — Wire Dynarmic into cpu.cpp
**Date:** 2026-04-17
**Agent:** agent/cpu-jit
**Phase:** 1

### What was built
`src/core/cpu/cpu.cpp` fully implemented. `DynarmicCallbacks` struct inside the `.cpp` (not in any header) inherits `Dynarmic::A32::UserCallbacks` and forwards all `MemoryRead*/Write*` to `Memory*`. `CallSVC` dispatches to `impl->svcHandler`. `AddTicks`/`GetTicksRemaining` implement the cycle budget. `CPU::init()` creates a `Dynarmic::A32::Jit` configured with `arch_version = v6K`. `CPU::reset()` sets CPSR to `0x00000010` (User mode, ARM state). `CPU::run(cycles)` sets `ticksRemaining`, calls `jit->Run()`, returns cycles consumed. `invalidateCacheRange` and `clearCache` delegate to Dynarmic. All code guarded with `#ifdef TRIDENT_USE_DYNARMIC` fallback stubs for non-JIT builds.

`CMakeLists.txt` fixed: removed the top-level `option(BUILD_SHARED_LIBS ON)` which caused Dynarmic to be built as a DLL (no import lib → link error). Added `set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)` before `add_subdirectory(third_party/dynarmic)`. The `trident_libretro SHARED` target is unaffected (explicit keyword overrides the flag).

Pre-existing scaffold bug fixes committed alongside (these were blocking compilation before my changes): `audio.cpp` missing `<algorithm>`, `emulator.cpp` calling non-existent `Patcher::readFile/applyPatch` API, `libretro_core.cpp` wrong `enum` type for `retro_pixel_format`.

### Why this approach
Phase 1 uses Dynarmic callback-mode only (no page-table fast path). This is simpler, correct, and matches the Phase 1 plan. The `DynarmicCallbacks` struct is hidden inside `cpu.cpp` so the callback machinery never leaks into the public API.

`arch_version = v6K` is critical — the 3DS is ARM11 (ARMv6K), not v8. Using the wrong version would silently misbehave on ARMv6K-specific instructions.

### Current module state
CPU: ✅ Working — Dynarmic JIT instantiated, all required callbacks implemented, build passes.

### What the next agent needs to know
1. **Boost is a hard build dependency of Dynarmic.** On any fresh machine, install Boost headers and pass `-DBOOST_ROOT=<path>` to cmake. See `tasks/lessons.md` Lesson 1 for the exact steps.
2. **The submodule starts empty** — `git submodule update --init --recursive` required.
3. **Dynarmic's optional deps (fmt, mcl, xbyak, robin-map, Zydis) are all bundled** in `third_party/dynarmic/externals/`. The "Could NOT find X" cmake warnings are harmless.
4. **Phase 2 CPU work**: add the Dynarmic page-table fast path (`config.page_table_pointer`) to avoid the per-access callback overhead. See Decision 2 in this file.
5. `ExceptionRaised` and `InterpreterFallback` currently just log to stderr. Phase 2 should handle `UnpredictableInstruction` and `UndefinedInstruction` properly.

### What was explicitly left out
- Dynarmic page-table fast path (Phase 2 — see Decision 2)
- Proper exception handling for `UnpredictableInstruction`, `WFI`, `Breakpoint` (Phase 2/3)
- Exclusive monitor (`MemoryWriteExclusive*`) — default returns false (always fails), safe for Phase 1

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
