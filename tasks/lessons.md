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

## Lesson 1 — 2026-04-17 — Impact: 9 / HIGH
**Source:** REFLECTION (agent self-reported)
**Agent:** trident-agent-loader / agent/loader
**What happened:** The task brief stated the NCCH ExeFS offset was at byte `0x1A0` in the NCCH
header. PLAN.md and the real 3DS hardware spec both say `0x180`. Offset `0x1A0` is the RomFS
size field. Trusting the brief would have made `loadNCCH()` read a garbage value as the ExeFS
location on every real ROM.
**Root cause:** The brief was a summarised version of PLAN.md and contained a copy-paste error for
the hardware constant.
**Fix:** Used `0x180` from PLAN.md (the more detailed and hardware-verified source).
**Rule:** When a hardware register offset in a task brief conflicts with PLAN.md or the 3dbrew
wiki, always use PLAN.md / the hardware spec. Never trust the brief alone for hardware constants.
**Affected areas:** `loadNCCH()`, any future agent parsing the NCCH header

---

## Lesson 2 — 2026-04-17 — Impact: 8 / HIGH
**Source:** REFLECTION (agent self-reported)
**Agent:** trident-agent-loader / agent/loader
**What happened:** The task brief described 3DSX ABS relocations as "add the segment's load
address to each patched word", implying a single add of the source-segment base. Real 3DSX
binaries encode the TARGET segment index in bits 31–28 of the patched word (`tSeg = word >> 28`,
`tOfs = word & 0x0FFFFFFF`). The correct ABS patch is `segBase[tSeg] + tOfs`. Using only the
source-segment base would silently corrupt every cross-segment function pointer in homebrew.
**Root cause:** The brief over-simplified the relocation algorithm; the real format uses a
segment-index tag inside each word, as implemented in ctrulib's loader.
**Fix:** Implemented the full `(word >> 28)` segment-dispatch for both ABS and REL tables.
**Rule:** For 3DSX relocations, always decode `tSeg = word >> 28` and look up
`segBase[tSeg]` — do NOT use the segment being patched as the relocation base.
**Affected areas:** `load3DSX()` relocation logic

---

## Lesson 3 — 2026-04-17 — Impact: 6 / MEDIUM
**Source:** REFLECTION (agent self-reported)
**Agent:** trident-agent-loader / agent/loader
**What happened:** `Memory::mapPages(vaddr, ptr, size, flags)` divides `size` by `PAGE_SIZE` using
a right-shift (`size >> PAGE_BITS`). If `size` is not a multiple of 4096, the fractional last page
is silently skipped, leaving those bytes unmapped and write-calls to them silently dropped.
**Root cause:** The page table walk assumes the caller provides a page-aligned size.
**Fix:** Always compute `pageAlignSize(size)` before passing to `mapPages`.
**Rule:** Always round size UP to the nearest 4 KB before calling `Memory::mapPages`.
Any segment size from a ROM header must be page-aligned before mapping.
**Affected areas:** Any code that calls `Memory::mapPages` with a computed size

---

> **Note to agents:** An empty lessons file means the project is new, not that there's nothing to learn.
> Read it every session. Lessons accumulate fast. The reflection after your first task
> is the most valuable one — that's when the surprises are freshest.
