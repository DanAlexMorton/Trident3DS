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

*(No lessons yet — this project is just starting. The first reflection will be added when Phase 1 agents complete their first tasks.)*

---

> **Note to agents:** An empty lessons file means the project is new, not that there's nothing to learn.
> Read it every session. Lessons accumulate fast. The reflection after your first task
> is the most valuable one — that's when the surprises are freshest.
