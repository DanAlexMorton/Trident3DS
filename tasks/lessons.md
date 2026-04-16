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

## Lesson 1 — 2026-04-17 — Impact: 6 / MEDIUM
**Source:** REFLECTION (agent self-reported)
**Agent:** agent/kernel-hle
**What happened:** The overall `cmake --build` fails with errors in `audio.cpp` (missing `<algorithm>` for `std::min`) and `emulator.cpp` (calls to non-existent `Patcher::readFile`/`applyPatch`, broken `memcpy` with 2 args). These are pre-existing scaffold errors unrelated to the Kernel agent's changes.
**Root cause:** The initial scaffold was generated with stub code that doesn't compile on Windows/MSVC — `std::min` requires `<algorithm>`, `memcpy` requires 3 arguments, and `Patcher::readFile` was never declared in `patcher.hpp`.
**Fix:** The Kernel agent's `kernel.cpp` compiled cleanly (verified via IDE diagnostics and selective build output). The pre-existing errors will need to be fixed by the agent that owns those files (CI agent or emulator.cpp owner) before Phase 1 integration.
**Rule:** Always verify your specific file compiles without errors using IDE diagnostics (`get_errors`) — don't rely on the overall build exit code when there are known pre-existing failures in other files.
**Affected areas:** `src/core/audio/audio.cpp`, `src/core/emulator.cpp` — both need a fix before Phase 1 integration can succeed.

Reflection 1 — 2026-04-17 — agent/kernel-hle — See Lesson 1 above. `svcBreak` and all required SVCs implemented cleanly. `<cstdio>` header was missing from the scaffold and had to be added.

---

> **Note to agents:** An empty lessons file means the project is new, not that there's nothing to learn.
> Read it every session. Lessons accumulate fast. The reflection after your first task
> is the most valuable one — that's when the surprises are freshest.
