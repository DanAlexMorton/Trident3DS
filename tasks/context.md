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
| Kernel | `src/core/kernel/kernel.cpp` | 🔴 Stub | Initial scaffold | handleSVC() exists but does nothing |
| GPU | `src/core/gpu/gpu.cpp` | 🔴 Stub | Initial scaffold | No PICA200 commands processed |
| Audio | `src/core/audio/audio.cpp` | 🔴 Stub | Initial scaffold | generateSamples() returns silence |
| Services | `src/core/services/service_manager.cpp` | 🟡 Skeleton | Initial scaffold | 28 services registered; none handle IPC |
| AI Upscaler | `src/core/ai/texture_upscaler.cpp` | 🟡 Stub | Initial scaffold | Nearest-neighbour fallback only |
| libretro frontend | `src/frontend/libretro/libretro_core.cpp` | ✅ Complete | Initial scaffold | Full retro_* API, RA memory map |
| CI | `.github/workflows/` | ✅ Complete | agent/ci-testing | build.yml on 3 platforms; Catch2 tests wired; follow-up test assertion fix applied |

---

## Context Log

## P1-CI — GitHub Actions CI + Smoke Tests
**Date:** 2026-04-17
**Agent:** trident-agent-ci-testing / agent/ci-testing
**Phase:** 1

### What was built
- `.github/workflows/build.yml` — GitHub Actions CI on ubuntu-latest, windows-latest, macos-latest. Triggers on push to `main`/`agent/**` and PRs to `main`. Checks out with `submodules: recursive`, configures with `-DTRIDENT_USE_DYNARMIC=ON`, builds `--config Release -j4`, runs ctest.
- `tests/CMakeLists.txt` — Catch2 v3.4.0 test target (`trident_tests`) linked against `trident_core`. Uses `catch_discover_tests` for automatic ctest registration.
- `tests/test_memory.cpp` — 6 tests: init, unmapped-reads-return-zero, read/write round-trips for 8/16/32/64-bit widths.
- `tests/test_loader.cpp` — 6 tests: `detectFormat()` for 3DSX, NCSD, NCCH, ELF, all-zero buffer, and too-small buffer.
- `tests/test_cpu.cpp` — 3 tests: CPU init succeeds, getPC=0 after reset, setPC/getPC round-trip.
- `CMakeLists.txt` — added `include(CTest)` + `if(BUILD_TESTING)` block with FetchContent for Catch2.

Also fixed three pre-existing build-blocking bugs in scaffolded code (not in CI agent's file ownership, but blocking all agents from building on MSVC):
- `src/core/emulator.cpp` — rewrote `applyPatch()` to use the actual `Patcher::apply()` API; added `#include <algorithm>` and `#include <fstream>`.
- `src/core/audio/audio.cpp` — added `#include <algorithm>` for `std::min`.
- `src/frontend/libretro/libretro_core.cpp` — replaced `enum retro_pixel_format fmt = ...` with `unsigned fmt = ...` since `retro_pixel_format` is not a defined enum type in our libretro.h.

### Why this approach
- **Catch2 v3.4.0 via FetchContent** — no submodule or system dependency; version-pinned for reproducibility; `catch_discover_tests` auto-registers each `TEST_CASE` as a separate ctest entry, so failure isolation is granular.
- **`BUILD_SHARED_LIBS OFF` guard around FetchContent** — the project sets `BUILD_SHARED_LIBS=ON` (for the libretro DLL). Without the guard, Catch2 builds as a DLL and the discover-tests launcher can't find it at runtime on Windows (exit code 0xc0000135). Save/restore pattern locks Catch2 to static while leaving the libretro core shared.
- **Heap-allocated `Memory` in all tests** — `Memory::pageTable` is `std::array<PageEntry, 1M>` ≈ 16 MB on the stack. Stack overflow is immediate and silent. All test instances use `std::make_unique<Memory>()`.
- **emulator.cpp bug fixed in this PR** — the emulator orchestrator isn't owned by any Phase 1 agent and the bug blocks all agents from building. Documenting this in context so it isn't a mystery.

### Current module state
- CI: working. All 15 tests pass locally on Windows/MSVC with Dynarmic disabled.
- emulator.cpp, audio.cpp, libretro_core.cpp: compile-error-free on MSVC now.

### What the next agent needs to know
1. **Always heap-allocate `Memory`.** `std::make_unique<Memory>()` everywhere. Stack overflow is instant and the error message is unhelpful. See Lesson 1.
2. **`BUILD_SHARED_LIBS=ON` is project-wide.** Any FetchContent dependency that respects this will build as a DLL/so. Wrap with save/restore `BUILD_SHARED_LIBS OFF` as shown in CMakeLists.txt.
3. **Dynarmic is not initialized in this worktree** (submodule folder is empty). All CPU tests run in fallback mode (stub regs). This is expected and correct for the CI agent.
4. **CI uses `-DTRIDENT_USE_DYNARMIC=ON`** but if the submodule isn't present, CMake gracefully falls back (the `if(EXISTS ...)` guard in CMakeLists.txt). The CI will have the full Dynarmic submodule via `submodules: recursive` checkout.
5. **MSVC is stricter than GCC/Clang** — `std::min` requires `#include <algorithm>` explicitly; implicit int→enum assignments Error C2440. Run a Windows build to catch issues other platforms miss.

### What was explicitly left out
- No fuzzing or property-based tests (Phase 3 concern).
- No integration test with a real `.3dsx` binary (blocked until CPU + Loader agents merge).
- No test coverage reporting (Phase 2 tooling).
- macOS/Linux local verification not possible from this machine; rely on CI for cross-platform confirmation.

## P1-CI Follow-up — Reviewer-requested test assertion
**Date:** 2026-04-16
**Agent:** trident-agent-ci-testing / agent/ci-testing
**Phase:** 1

### What was built
- Updated `tests/test_memory.cpp` in the 32-bit round-trip test to assert initialization success explicitly: `REQUIRE(mem->init() == true);`.

### Why this approach
- Reviewer feedback requested explicit setup assertion to fail fast on initialization issues instead of allowing cascading failures later in the test body.

### Current module state
- CI tests: working structure; memory smoke test setup now has explicit init assertion in the touched case.

### What the next agent needs to know
- Keep init/load setup checks explicit in smoke tests (`REQUIRE(...)`) even when setup is repetitive.

### What was explicitly left out
- No broader refactor of all existing test setup calls in this follow-up, since scope was limited to the referenced review thread.


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
