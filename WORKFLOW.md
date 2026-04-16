# Trident3DS — Agent Workflow Guide

How to run this project day-to-day using Copilot agents and git worktrees.

---

## The Big Picture

You run multiple Copilot agents in parallel, each in its own VS Code window pointed at its own
git worktree. Each agent owns one component. They never conflict. You review and merge their
PRs at sprint checkpoints.

```
You (Daniel)
│
├── Reviews briefs, answers questions, corrects mistakes
├── Merges PRs at sprint checkpoints
└── Decides when phases are complete

      ↓ paste brief              ↓ paste brief
┌─────────────┐           ┌─────────────┐
│ VS Code #1  │           │ VS Code #2  │        ... and so on
│ cpu-jit     │           │ loader      │
│ Copilot     │           │ Copilot     │
│ Agent mode  │           │ Agent mode  │
└─────────────┘           └─────────────┘
      ↓ commits                 ↓ commits
 agent/cpu-jit            agent/loader
      └──────────────┬──────────┘
                  main (you merge here)
```

---

## One-Time Setup (do this once)

### 1. Create the worktrees

Open Git Bash from `C:\Users\donal\Work\Games\Trident3DS` and run:

```bash
# Phase 1 worktrees (needed now)
git branch agent/ci-testing
git branch agent/cpu-jit
git branch agent/loader
git branch agent/kernel-hle

git worktree add ../trident-agent-ci-testing   agent/ci-testing
git worktree add ../trident-agent-cpu-jit       agent/cpu-jit
git worktree add ../trident-agent-loader        agent/loader
git worktree add ../trident-agent-kernel-hle    agent/kernel-hle
```

Verify:
```bash
git worktree list
# Should show 5 entries: main repo + 4 worktrees
```

### 2. Open VS Code windows (one per active agent)

```bash
# Run these from Git Bash or Windows Run (Win+R)
code "C:\Users\donal\Work\Games\trident-agent-ci-testing"
code "C:\Users\donal\Work\Games\trident-agent-cpu-jit"
code "C:\Users\donal\Work\Games\trident-agent-loader"
code "C:\Users\donal\Work\Games\trident-agent-kernel-hle"
```

Label each window in your taskbar so you know which is which.

---

## Sprint Workflow (repeat for every sprint)

### Step 1 — Give the agent its brief

In each VS Code window:
1. Open Copilot Chat: `Ctrl+Shift+I`
2. Switch to **Agent** mode (dropdown at top of chat panel)
3. Paste the brief from `tasks/agent-briefs.md` for that worktree
4. Send it

**The opening prompt to paste in each window:**

```
You are working in the [AGENT NAME] worktree for Trident3DS.

Before writing any code, follow the mandatory start sequence:
1. Read tasks/lessons.md
2. Read tasks/context.md
3. Read CLAUDE.md
4. Read PLAN.md
5. Read AGENT.md in this worktree root

Then complete this task:
[PASTE THE BRIEF FROM tasks/agent-briefs.md]

When finished:
- Run the verification checklist
- Run the self-reflection (5 questions, log lessons ≥4 to tasks/lessons.md)
- Update tasks/context.md (module status table + context log entry)
- Open a PR against main
```

Replace `[AGENT NAME]` and `[PASTE THE BRIEF]` with the actual values.

### Step 2 — Let agents run

Agents work autonomously. You don't need to babysit them. Check in when:
- Copilot asks you a question (answer in the chat)
- You see a PR opened on GitHub
- Something looks wrong in the chat output

**Healthy signs:** Agent reads the files first, writes a plan in `tasks/todo.md`, codes in small commits, runs the build.

**Warning signs:** Agent starts coding immediately without reading files, edits a `.hpp` file, modifies files outside its subtree.

### Step 3 — Review the PR

When an agent opens a PR, check these things in order:

```
□ 1. CI is green (build passes on all three platforms)
□ 2. Only files in the agent's assigned subtree were changed
□ 3. No .hpp files were edited
□ 4. tasks/lessons.md has a reflection entry (even if "no new lessons")
□ 5. tasks/context.md module status table was updated
□ 6. tasks/context.md has a new context log entry
□ 7. The success criteria in the brief are all ticked
□ 8. Code uses uint32_t not int for hardware values
□ 9. Everything is inside namespace Trident { }
□ 10. No silent no-ops — unimplemented SVCs log to stderr
```

If any box is unchecked, tell the agent what to fix in the Copilot chat window.
Don't merge until all boxes are ticked.

### Step 4 — Merge in sprint order

**Never merge directly to main without reviewing.** Merge in the order listed in the sprint.
Some tasks are marked (Sequential) — they must wait for a preceding merge.

```bash
# From the main repo (C:\Users\donal\Work\Games\Trident3DS)
git checkout main
git merge --no-ff agent/ci-testing     # Always merge CI first
git merge --no-ff agent/cpu-jit
git merge --no-ff agent/loader
git merge --no-ff agent/kernel-hle
git push origin main
```

After merging, clean up the worktree:
```bash
git worktree remove ../trident-agent-ci-testing
git branch -d agent/ci-testing
```

### Step 5 — Run the integration test

After all sprint branches are merged, manually test the integration checkpoint.
Each sprint has a checkpoint description in `tasks/todo.md`. If it fails, create a bug-fix task.

---

## Daily Rhythm

**Morning (~10 mins):**
- Check which agents have open PRs on GitHub
- Review any questions agents asked overnight
- If a PR looks ready, run through the review checklist and merge

**During the day:**
- Answer agent questions when they pop up in Copilot chat
- If an agent gets stuck or goes wrong, correct it in chat and tell it to add a lesson

**When starting a new task:**
- Open the relevant worktree window
- Paste the brief from `tasks/agent-briefs.md`
- Let it run

---

## How to Correct an Agent

When an agent does something wrong:

1. **Tell it exactly what's wrong in plain language.** Don't hint — be direct.
   > "You edited `cpu.hpp`. That's a contract file you're not allowed to touch. Revert that change and implement the same behaviour inside `cpu.cpp` instead."

2. **Tell it to add a lesson immediately.**
   > "Add a lesson to `tasks/lessons.md` for this mistake. Use the template. Rate it."

3. **Ask it to re-run the verification checklist** before trying again.

4. **If the same mistake happens twice,** check `tasks/lessons.md` — the lesson may be too vague.
   Tell the agent to make the rule more specific.

---

## Phase Gate Criteria

### Phase 1 → Phase 2

Don't start Phase 2 until all of these are true:

```
□ agent/ci-testing merged — CI green on Ubuntu + Windows + macOS
□ agent/cpu-jit merged — Dynarmic instantiated, run() returns real cycle count
□ agent/loader merged — load3DSX() loads a real homebrew binary
□ agent/kernel-hle merged — svcBreak handled, emulator logs and halts cleanly
□ Integration test passed — a .3dsx file boots to svcBreak and exits with expected output
□ tasks/context.md updated with honest module status for all Phase 1 modules
```

Once all pass, create the Phase 2 worktrees:
```bash
git branch agent/gpu-pica
git branch agent/services-hle
git branch agent/audio-dsp
git worktree add ../trident-agent-gpu-pica      agent/gpu-pica
git worktree add ../trident-agent-services-hle  agent/services-hle
git worktree add ../trident-agent-audio-dsp     agent/audio-dsp
```

---

## Handling a Stuck Agent

If an agent has been running for a long time without committing anything, or is going in circles:

1. **Read the chat output** — usually it's stuck on a compile error or waiting for a decision
2. **Ask it directly:** "What are you blocked on? State the exact error or question."
3. **If it's a build error,** paste the error message and ask it to fix just that one issue
4. **If it's an architectural question,** make the decision yourself and tell it what to do
5. **If it's repeatedly making the same mistake,** stop it, correct it, and make sure it adds a lesson

---

## Quick Reference — File Purposes

| File | What it's for | Updated by |
|------|--------------|------------|
| `CLAUDE.md` | Project memory — architecture, constants, contracts | Human only (rare) |
| `PLAN.md` | Strategic roadmap — all phases and agent briefs | Human only |
| `WORKFLOW.md` | This file — how to run the project | Human only |
| `tasks/lessons.md` | Mistake log + self-reflection — rules for agents | Agents (end of task) |
| `tasks/context.md` | Living context — what's been built and why | Agents (end of task) |
| `tasks/todo.md` | Active sprint plans and backlog | Agents (start/end of task) |
| `tasks/agent-briefs.md` | Paste-ready briefs for each agent | Human only |
| `AGENT.md` (per worktree) | Specific brief for that worktree's agent | Human only |
| `.github/copilot-instructions.md` | Copilot auto-load — start/end sequences | Human only |

---

## Common Commands

```bash
# Check worktree status
git worktree list

# See what an agent has committed
git log --oneline agent/cpu-jit ^main

# See what files an agent changed
git diff main..agent/cpu-jit --name-only

# See the full diff before merging
git diff main..agent/cpu-jit

# If a worktree gets corrupted
git worktree repair

# Remove a merged worktree
git worktree remove ../trident-agent-cpu-jit
git branch -d agent/cpu-jit
```

---

## What Good Agent Output Looks Like

A well-run agent session looks like this in the chat:

```
[Agent reads tasks/lessons.md]
  → "No lessons yet. Noted."

[Agent reads tasks/context.md]
  → "CPU module is stub. Memory is complete. I own cpu.cpp only."

[Agent reads CLAUDE.md, PLAN.md, AGENT.md]
  → "Understood. Dynarmic is already in CMake. I just need cpu.cpp."

[Agent writes plan to tasks/todo.md]
  → "Plan written. Waiting for confirmation."

[You: "looks good, go ahead"]

[Agent implements DynarmicCallbacks in cpu.cpp]
[Agent runs cmake --build, fixes errors]
[Agent commits: "P1-S1-1: Wire Dynarmic ARM11 JIT into cpu.cpp"]

[Agent runs verification checklist]
  → "All checks pass."

[Agent runs self-reflection]
  → "Lesson 1 added: arch_version must be v6K not v8. Impact: 9/HIGH."

[Agent updates tasks/context.md]
  → "Module status updated. Context log entry added."

[Agent opens PR]
```

If the session doesn't start with file reads, or skips straight to coding, stop it and ask it to follow the start sequence.
