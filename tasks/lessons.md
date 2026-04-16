# Trident3DS — Agent Lessons

This file is the self-improvement loop. It is updated in two ways:
1. **After any correction from Daniel** — reactive lesson, always HIGH impact
2. **After every completed task** — proactive self-reflection, any impact level

**Every agent reads this file at the start of every session.**

---

## Impact Scale

| Rating | Meaning |
|--------|---------|
| **10 / HIGH** | Caused or would cause a build break, data corruption, wrong hardware behaviour, or wasted another agent's work |
| **7–9 / HIGH** | Non-obvious pattern that any agent would likely get wrong on first attempt |
| **4–6 / MEDIUM** | Saved time or avoided a subtle bug — worth knowing but not critical |
| **1–3 / LOW** | Minor style, naming, or tooling insight — useful but not urgent |

---

## Lesson Template

```
## Lesson [N] — [YYYY-MM-DD] — Impact: [1–10 / HIGH|MEDIUM|LOW]
**Source:** CORRECTION (Daniel caught this) | REFLECTION (agent self-reported)
**Agent:** which worktree this came from
**What happened:** What went wrong, or what was harder than expected
**Root cause:** Why — wrong assumption, missing context, surprising API behaviour, etc.
**Fix:** What was done to correct it (or what the right approach is)
**Rule:** One concrete, actionable sentence that prevents this recurring
**Affected areas:** Which files, modules, or phases this applies to
```

---

## End-of-Task Reflection Prompt

After completing every task and before opening a PR, agents must answer these five questions
and write any non-trivial answer as a lesson entry:

1. **What was harder than I expected, and why?**
2. **What assumption did I make that turned out to be wrong or risky?**
3. **What would I warn the next agent working in this area?**
4. **What did I discover about this codebase that isn't obvious from reading the headers?**
5. **Did I almost break anything? What saved it?**

If the honest answer to all five is "nothing notable" — write a single line:
`Reflection [N] — [date] — [agent] — No new lessons. Task was straightforward.`

Do not skip the reflection. A blank reflection looks like the agent forgot.

---

## Lessons

## Lesson 1 — 2026-04-17 — Impact: 8 / HIGH
**Source:** REFLECTION (agent self-reported)
**Agent:** trident-agent-ci-testing / agent/ci-testing
**What happened:** `Memory` has a `std::array<PageEntry, 1'048'576>` page table (~16 MB). Declaring `Memory mem;` on the stack in a test (or any function) causes an immediate SIGSEGV / stack overflow at runtime. The test compiled fine but crashed before any assertion ran.
**Root cause:** `std::array` allocates inline on the stack. A 1M-entry array of 16-byte `PageEntry` structs = 16 MB, far exceeding the default Windows stack (1 MB) and typical Linux default (8 MB).
**Fix:** Use `auto mem = std::make_unique<Memory>();` in every test and every standalone function that needs a `Memory` instance. The test suite was rewritten to heap-allocate.
**Rule:** Always heap-allocate `Memory` (and any object containing it) — `std::make_unique<Memory>()` everywhere, never `Memory mem;` on the stack.
**Affected areas:** `tests/`, any future diagnostic tool, Phase 3 compat agents, any agent writing standalone programs.

## Lesson 2 — 2026-04-17 — Impact: 7 / HIGH
**Source:** REFLECTION (agent self-reported)
**Agent:** trident-agent-ci-testing / agent/ci-testing
**What happened:** The project's top-level `option(BUILD_SHARED_LIBS ... ON)` caused Catch2 (pulled via FetchContent) to build as a DLL on Windows. The CI test-discovery step (`catch_discover_tests`) runs the binary post-build; the missing DLL triggered exit code `0xc0000135` and failed the build.
**Root cause:** `BUILD_SHARED_LIBS` is a global CMake variable that FetchContent-pulled projects inherit. Having it ON for the libretro core propagated silently to Catch2.
**Fix:** Save/restore `BUILD_SHARED_LIBS` around `FetchContent_MakeAvailable(Catch2)` in CMakeLists.txt, forcing Catch2 to build as a static library regardless of the project setting.
**Rule:** Before calling `FetchContent_MakeAvailable` for any test or tool dependency, save and set `BUILD_SHARED_LIBS OFF`, then restore — always.
**Affected areas:** `CMakeLists.txt` testing block, any future FetchContent-based dependency added to the project.

## Lesson 3 — 2026-04-17 — Impact: 7 / HIGH
**Source:** REFLECTION (agent self-reported)
**Agent:** trident-agent-ci-testing / agent/ci-testing
**What happened:** Three scaffolded `.cpp` files had compile errors on MSVC that were invisible on GCC/Clang: (1) `emulator.cpp` called non-existent `Patcher::readFile` and `Patcher::applyPatch` (mismatched against the actual `Patcher::apply` API); (2) `audio.cpp` used `std::min` without `#include <algorithm>`; (3) `libretro_core.cpp` used `enum retro_pixel_format fmt = ...` where `retro_pixel_format` is not defined as an enum type in the header — MSVC rejects the implicit int→enum conversion.
**Root cause:** The scaffold was developed and partially tested on Linux/macOS where GCC/Clang silently accept C-style enum declarations, unqualified `std::min` (leaked through other includes), and stale API references that were never actually compiled.
**Fix:** Added `#include <algorithm>` to audio.cpp; rewrote `emulator.cpp::applyPatch` to use `Patcher::apply(romData, patchPath)`; replaced `enum retro_pixel_format` with `unsigned` in libretro_core.cpp.
**Rule:** Run a MSVC build locally (or push to `agent/ci-testing`) before declaring any module "complete" — MSVC's stricter conformance exposes latent bugs that GCC/Clang silently accept.
**Affected areas:** `src/core/emulator.cpp`, `src/core/audio/audio.cpp`, `src/frontend/libretro/libretro_core.cpp`, all future `.cpp` files.

## Lesson 4 — 2026-04-16 — Impact: 8 / HIGH
**Source:** CORRECTION (Daniel caught this)
**Agent:** trident-agent-ci-testing / agent/ci-testing
**What happened:** A smoke test (`tests/test_memory.cpp`, 32-bit read/write case) called `mem->init()` without asserting the return value, which can hide setup failures and produce misleading downstream assertions.
**Root cause:** I treated repeated test setup calls as boilerplate and failed to enforce explicit success checks consistently in each test case.
**Fix:** Updated the test to use `REQUIRE(mem->init() == true);` so initialization failure is surfaced immediately at setup.
**Rule:** Every test setup call that returns a status (`init()`, `load()`, etc.) must be asserted with `REQUIRE` before continuing.
**Affected areas:** `tests/test_memory.cpp`, `tests/test_cpu.cpp`, and future smoke/integration tests.

---

> **Note to agents:** An empty lessons file means the project is new, not that there's nothing to learn.
> Read it every session. Lessons accumulate fast. The reflection after your first task
> is the most valuable one — that's when the surprises are freshest.
