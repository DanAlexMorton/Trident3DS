# Trident3DS — Copilot Agent Instructions

These instructions are loaded automatically by GitHub Copilot in every VS Code session.
Follow them before writing a single line of code.

---

## TWO MANDATORY SEQUENCES — Start AND End of every task, no exceptions

---

## MANDATORY END SEQUENCE — Before Opening Any PR

Run this self-reflection **after** the verification checklist passes and **before** opening the PR.
Do not skip it. A missing reflection is a red flag in review.

### Step 1 — Answer these five questions honestly

Ask yourself:
1. **What was harder than I expected, and why?**
2. **What assumption did I make that turned out to be wrong or risky?**
3. **What would I warn the next agent working in this area?**
4. **What did I discover about this codebase that isn't obvious from reading the headers?**
5. **Did I almost break anything? What saved it?**

### Step 2 — Rate each non-trivial answer

Use this scale:
- **8–10 / HIGH** — Would cause a build break, wrong hardware behaviour, or waste another agent's work
- **4–7 / MEDIUM** — Subtle bug avoided or non-obvious pattern discovered
- **1–3 / LOW** — Minor tooling or style insight

### Step 3 — Write a lesson entry for anything rated 4 or above

Append to `tasks/lessons.md` using the template at the top of that file.
Include: impact rating, source (REFLECTION), agent name, what happened, root cause, rule.

### Step 4 — If nothing is notable, write a single line

```
Reflection [N] — [YYYY-MM-DD] — [agent/worktree] — No new lessons. Task was straightforward.
```

This confirms you ran the reflection. Do not leave it blank.

### Step 5 — Update `tasks/context.md`

This is mandatory. Two updates required:

**Update the Module Status Snapshot table:**
Find the row(s) for every module you touched. Update the Status, "Last updated by" (use your branch name),
and Notes columns to reflect the current reality. Be honest — if it's still partial, say partial.

**Append a Context Log entry:**
Add a new entry at the bottom of the Context Log section using the template in `context.md`.
Write for the next agent, not for Daniel. Answer:
- What specifically was built (files, functions, behaviours)
- Why this approach over alternatives
- Current state of each module you touched
- What the next agent in this area needs to know before starting
- What was explicitly deferred and why

If you made a significant architectural decision, add it to the **Key Architectural Decisions** section too.

A context entry takes 5 minutes and saves the next agent 30 minutes of archaeology.
Do not skip it.

---

## MANDATORY START SEQUENCE — Every Session, Every Task, No Exceptions

### Step 1 — Read `tasks/lessons.md`
Contains accumulated lessons from real mistakes made during Trident3DS development.
Each lesson has a concrete Rule. If your task touches an affected area, that rule applies to you.
Skipping this = repeating known bugs.

### Step 2 — Read `tasks/context.md`
The living project history. Shows what has actually been built, why key decisions were made,
the current honest status of every module, and what previous agents discovered.
This is different from CLAUDE.md — CLAUDE.md describes the target; context.md describes reality.

### Step 3 — Read `CLAUDE.md`
The full project memory: tech stack, module status, hardware constants, memory map,
interface contracts, Dynarmic integration notes, SVC conventions, and critical gotchas.

### Step 4 — Read `PLAN.md`
The strategic roadmap. Understand which phase you are in and what your agent owns.
Know what was built before you and what comes after.

### Step 5 — Read your `AGENT.md`
In your worktree root. Contains your specific mission, success criteria, and constraints.

### After any correction from Daniel
Append a new lesson to `tasks/lessons.md` immediately using the template at the top of that file.
Be specific. Vague lessons don't prevent recurrence.

---

## Plan Before You Build

For ANY task with 3+ steps or architectural decisions:
- Write a plan in `tasks/todo.md` before writing any code
- State exactly which files will be created or modified and why
- State what will NOT be touched and why
- If something goes sideways mid-task: STOP, re-plan, do not keep pushing

For simple single-file changes (under ~30 lines): proceed directly, no plan needed.

---

## The Cardinal Rule: Agents Own Implementations, Not Interfaces

- **Headers (`.hpp` files) are contracts.** Read them to understand what to implement.
- **Never edit a header that belongs to another component.**
- Your assigned files are listed in your `AGENT.md`. Only touch those.
- A header change requires Daniel to review it as a separate PR — do not mix it in.

---

## Verification Checklist (Required Before Marking Any Task Done)

- [ ] `cmake --build build --config Debug` succeeds with zero errors
- [ ] No new compiler warnings introduced
- [ ] No edits to `*.hpp` files outside your assigned subtree
- [ ] If you implemented a function, there is either a test or a described manual test
- [ ] All hardware values use `uint32_t`/`uint16_t`/`uint8_t` — not `int`/`short`/`char`
- [ ] Everything is inside `namespace Trident { }`
- [ ] Would a senior emulator developer approve this PR? If not, fix it first.

---

## Code Standards

- **Types:** `uint32_t` for addresses, registers, sizes. Never `int` or `unsigned int` for hardware values.
- **Namespace:** All code inside `namespace Trident { }` — no exceptions.
- **Logging:** `fprintf(stderr, "[MODULE] message\n")` with the module name prefix. No `printf`.
- **Pattern:** Follow the existing pimpl pattern — `struct Impl` inside each `.cpp` file.
- **Constants:** Hardware constants belong in `.hpp` files, not inline in `.cpp`.
- **No TODOs without a reason:** If you leave a stub, explain why: `// TODO(Phase 2): add LZ11 decompression here`.
- **No silent failures:** If an SVC or format is unimplemented, log it — never silently no-op.

---

## Trident3DS in 100 Words

From-scratch 3DS emulator. libretro frontend + iOS + Android targets. ARM11 CPU via Dynarmic JIT (already submodule, already in CMake — just needs `cpu.cpp` implementing). Memory bus is complete. Everything else is stubbed. Phase 1 goal: boot a `.3dsx` homebrew to `svcBreak`. Phase 2: first rendered triangle via PICA200 GPU. Phase 3: commercial titles. Phase 4: AI shader transpiler (PICA200 → GLSL) and neural texture upscaler. Four agents run in parallel in Phase 1 (CPU, Loader, Kernel, CI) via git worktrees. Each agent owns one `src/core/` subtree. Headers are contracts — never edit them.
