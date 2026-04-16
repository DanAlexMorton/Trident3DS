# Trident3DS — Active Task Plans

Agents write their plan here before writing code. One task at a time. Mark items complete as you go.

---

## How to Use This File

1. Before starting any non-trivial task, write a plan section below
2. State the task ID (from `agent-briefs.md`), what you will change, and what you will NOT touch
3. Check off items as you complete them
4. Mark the whole plan DONE when the verification checklist passes
5. Leave completed plans here for history — don't delete them

---

## Plan Template

```
## [Task ID] — [Task Name]
**Agent:** [which worktree]
**Status:** IN PROGRESS / DONE / BLOCKED
**Date started:** YYYY-MM-DD

### What I will change
- file1.cpp: reason
- file2.cpp: reason

### What I will NOT touch
- cpu.hpp: it's a contract
- memory.cpp: not in my scope

### Steps
- [ ] Step 1
- [ ] Step 2
- [ ] Step 3

### Verification
- [ ] cmake --build passes
- [ ] No warnings
- [ ] No header edits
- [ ] Success criteria from AGENT.md met
```

---

## Active Plans

*(No active plans yet — agents will add plans here when Phase 1 work begins.)*

---

## Sprint Plan — Phase 1

### How sprints work

Sprints are batches of agent work you kick off together.
`(Parallel)` tasks can all run at the same time in separate VS Code windows.
`(Sequential)` tasks must wait for a preceding merge before starting.
At the end of each sprint, you run the integration checkpoint before moving on.

---

### Sprint P1-S0 — CI Foundation
**Goal:** Green CI on all three platforms before any feature work lands.
Merge this sprint's single PR before starting P1-S1.

| Task ID | Agent / Worktree | Type | Description |
|---------|-----------------|------|-------------|
| P1-S0-1 | `agent/ci-testing` | Sequential | Set up GitHub Actions — Ubuntu + Windows + macOS build matrix, Catch2 smoke tests |

**Brief to paste:**
> Use the brief for `P1-CI` from `tasks/agent-briefs.md`.

**Integration checkpoint:**
- CI is green on all three platforms for an empty build
- At least one smoke test passes (memory read/write round-trip)

---

### Sprint P1-S1 — Four Agents in Parallel
**Goal:** Wire the CPU, load a 3DSX, handle SVCs. All four tasks run simultaneously.
Requires Sprint P1-S0 to be merged first (so CI catches regressions immediately).

| Task ID | Agent / Worktree | Type | Description |
|---------|-----------------|------|-------------|
| P1-S1-1 | `agent/cpu-jit` | Parallel | Wire Dynarmic: `DynarmicCallbacks` + `Dynarmic::A32::Jit` in `cpu.cpp`, `run()` returns real cycle count |
| P1-S1-2 | `agent/loader` | Parallel | Implement `load3DSX()`: parse header, map segments to 0x00100000, apply ABS+REL relocations |
| P1-S1-3 | `agent/loader` | Parallel (same worktree as P1-S1-2) | Implement `loadNCCH()`: parse ExeFS, load `.code` segment |
| P1-S1-4 | `agent/kernel-hle` | Parallel | Implement essential SVCs: `svcBreak` (halt + log), `svcOutputDebugString`, `svcControlMemory`, plus stubs for `svcCreateThread`, `svcSleepThread`, `svcSendSyncRequest` |

> **Note on P1-S1-2 and P1-S1-3:** The loader agent can do both in one session. Start with 3DSX (simpler format, needed for integration test), then NCCH. Both go on the same `agent/loader` branch.

**Integration checkpoint (after all four PRs merge):**
- `cmake --build` passes on all platforms with zero warnings
- CPU: `run(4468530)` returns a non-zero cycle count (Dynarmic executed something)
- Loader: `loadFile("test.3dsx", mem)` returns `success=true` and a non-zero `entryPoint`
- Kernel: `handleSVC(0x3C)` logs the break reason to stderr and sets a halt flag

---

### Sprint P1-S2 — Integration & Boot
**Goal:** Wire emulator.cpp so a `.3dsx` boots to `svcBreak` and exits cleanly.
All P1-S1 PRs must be merged before this sprint starts.
This is done by Daniel in the main repo (or a dedicated agent/worktree if preferred).

| Task ID | Agent / Worktree | Type | Description |
|---------|-----------------|------|-------------|
| P1-S2-1 | `agent/cpu-jit` or main | Sequential | Update `emulator.cpp`: call `loader.loadFile()`, set `cpu.setPC(entryPoint)`, run the main loop calling `cpu.run(CYCLES_PER_FRAME)` |
| P1-S2-2 | `agent/kernel-hle` | Sequential | Ensure `svcBreak` sets an `Emulator::halt()` flag that the main loop checks to exit |

**Integration checkpoint — Phase 1 Complete:**
```
□ Load a real .3dsx homebrew (e.g. the Homebrew Launcher or a hello-world binary)
□ Emulator boots to svcBreak
□ svcBreak reason is logged to stderr
□ Process exits cleanly with expected output
□ CI is green
□ tasks/context.md is up to date with honest module status
```

Once all boxes are ticked, you are ready for Phase 2.

---

## Backlog

### Phase 2 (After Phase 1 integration checkpoint passes)

First, create the Phase 2 worktrees:
```bash
git branch agent/gpu-pica
git branch agent/services-hle
git branch agent/audio-dsp
git worktree add ../trident-agent-gpu-pica      agent/gpu-pica
git worktree add ../trident-agent-services-hle  agent/services-hle
git worktree add ../trident-agent-audio-dsp     agent/audio-dsp
```

| Task ID | Agent / Worktree | Type | Description |
|---------|-----------------|------|-------------|
| P2-GPU-1 | `agent/gpu-pica` | Parallel | PICA200 command processor — parse register writes, execute memory fill commands |
| P2-GPU-2 | `agent/gpu-pica` | Sequential (after P2-GPU-1) | Framebuffer output — XRGB8888, correct top/bottom screen dimensions |
| P2-SVC-1 | `agent/services-hle` | Parallel | GSP service: `RegisterInterruptRelayQueue`, `SubmitGxCommand` |
| P2-SVC-2 | `agent/services-hle` | Parallel | APT service: `Initialize`, `Enable`, `GetSharedFont` |
| P2-SVC-3 | `agent/services-hle` | Parallel | FS service: `Initialize`, `OpenFile`, `ReadFile` (stub SD card) |
| P2-AUD-1 | `agent/audio-dsp` | Parallel | DSP stub: `Initialize` returns success, `generateSamples()` returns silence |

**Phase 2 integration checkpoint:** First rendered triangle via PICA200 GPU.

---

### Phase 3 (After Phase 2 integration checkpoint passes)

| Task ID | Agent / Worktree | Type | Description |
|---------|-----------------|------|-------------|
| P3-DBG-1 | `agent/debugger` | Parallel | GDB stub on port 1234 — halt, step, breakpoints, register read/write |
| P3-DBG-2 | `agent/debugger` | Parallel | Instruction trace ring buffer (last 1000 instructions, circular) |
| P3-DBG-3 | `agent/debugger` | Parallel | Service call logger with arguments decoded from IPC buffers |

**Phase 3 integration checkpoint:** Commercial title boots to title screen (approximate).

---

### Phase 4 (AI Differentiators — Ongoing Research)

| Task ID | Agent / Worktree | Type | Description |
|---------|-----------------|------|-------------|
| P4-SHD-1 | `agent/ai-shader` | Parallel | Rule-based PICA200 shader transpiler — all ~30 vertex shader opcodes → GLSL ES 3.0 |
| P4-SHD-2 | `agent/ai-shader` | Sequential (after P4-SHD-1) | Seq2seq neural shader model: PICA200 bytecode tokens → GLSL tokens |
| P4-TEX-1 | `agent/ai-shader` | Parallel | Neural texture upscaler — ESPCN/FSRCNN implemented as GLSL compute shader |
| P4-FRM-1 | `agent/ai-shader` | Sequential (after P4-TEX-1) | Frame interpolation — optical flow, synthesise intermediate frames (30fps → 60fps) |

**Phase 4 integration checkpoint:** AI-transpiled shaders produce correct output on a known title.
