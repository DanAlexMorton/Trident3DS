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

---

## Lesson 1 — 2026-04-17 — Impact: 8 / HIGH
**Source:** REFLECTION
**Agent:** agent/cpu-jit
**What happened:** Dynarmic needs Boost (boost/icl and boost/variant) but Boost is not bundled and not documented as a prerequisite. The project docs said "CMakeLists.txt already links it — no CMake changes needed", implying zero setup was required. In reality, cmake configure fails with "Could NOT find Boost" and the submodule starts empty.
**Root cause:** The PLAN.md and agent brief were written assuming a developer environment where Boost is pre-installed. On a clean machine it is not present.
**Fix:** 1) Run `git submodule update --init --recursive` before building. 2) Download or install Boost and pass `-DBOOST_ROOT=<path>` to cmake. The modular boost cmake zip from GitHub releases needs to be flattened — all `libs/<name>/include/` subdirs merged into one directory. 3) Set `BUILD_SHARED_LIBS=OFF` (or fix in CMakeLists) — Dynarmic does not export DLL symbols so it must be static.
**Rule:** When initialising this project on a new machine: run `git submodule update --init --recursive`, then install Boost headers and pass `BOOST_ROOT` to cmake. Do not assume cmake "just works" out of the box.
**Affected areas:** `third_party/dynarmic/`, `CMakeLists.txt`, any CI configuration

---

## Lesson 2 — 2026-04-17 — Impact: 5 / MEDIUM
**Source:** REFLECTION
**Agent:** agent/cpu-jit
**What happened:** `BUILD_SHARED_LIBS=ON` (the top-level default) caused Dynarmic to be built as a `.dll` without an import `.lib` file, making the final link fail with `LNK1104: cannot open file 'dynarmic.lib'`. The fix was to force `BUILD_SHARED_LIBS OFF` before `add_subdirectory(third_party/dynarmic)`.
**Root cause:** MSVC only generates a `.lib` import library for a DLL if the DLL exports symbols via `__declspec(dllexport)`. Dynarmic does not do this — it is designed as a static library. Inheriting `BUILD_SHARED_LIBS=ON` from the parent scope made it build as a broken DLL.
**Fix:** Added `set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)` immediately before `add_subdirectory(third_party/dynarmic)`. The `trident_libretro` target uses `add_library(... SHARED ...)` explicitly so it is unaffected by this global flag.
**Rule:** Any third-party library that does not export symbols must have `BUILD_SHARED_LIBS` forced OFF before its `add_subdirectory()` call, regardless of the project-level default.
**Affected areas:** `CMakeLists.txt`, `third_party/dynarmic/`

---

> **Note to agents:** An empty lessons file means the project is new, not that there's nothing to learn.
> Read it every session. Lessons accumulate fast. The reflection after your first task
> is the most valuable one — that's when the surprises are freshest.
