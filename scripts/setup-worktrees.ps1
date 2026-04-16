# Trident3DS — Agent Worktree Setup
# Run from the repo root: .\scripts\setup-worktrees.ps1

$ErrorActionPreference = "Stop"

$RepoRoot = (git rev-parse --show-toplevel).Trim()
$ParentDir = Split-Path $RepoRoot -Parent

$Worktrees = @(
    @{ Name = "cpu-jit";      Branch = "agent/cpu-jit";      Phase = 1 },
    @{ Name = "loader";       Branch = "agent/loader";        Phase = 1 },
    @{ Name = "kernel-hle";   Branch = "agent/kernel-hle";    Phase = 1 },
    @{ Name = "ci-testing";   Branch = "agent/ci-testing";    Phase = 1 },
    @{ Name = "gpu-pica";     Branch = "agent/gpu-pica";      Phase = 2 },
    @{ Name = "services-hle"; Branch = "agent/services-hle";  Phase = 2 },
    @{ Name = "audio-dsp";    Branch = "agent/audio-dsp";     Phase = 2 },
    @{ Name = "debugger";     Branch = "agent/debugger";      Phase = 3 },
    @{ Name = "ai-shader";    Branch = "agent/ai-shader";     Phase = 4 }
)

Write-Host ""
Write-Host "Trident3DS Agent Worktree Setup"
Write-Host "================================"
Write-Host "Repo root : $RepoRoot"
Write-Host "Worktrees : $ParentDir\trident-agent-*"
Write-Host ""

foreach ($wt in $Worktrees) {
    $Path = Join-Path $ParentDir "trident-agent-$($wt.Name)"

    if (Test-Path $Path) {
        Write-Host "  SKIP  $($wt.Name) (Phase $($wt.Phase)) — already exists"
        continue
    }

    # Create branch if it doesn't exist
    $null = git show-ref --quiet "refs/heads/$($wt.Branch)" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  + branch  $($wt.Branch)"
        git branch $wt.Branch | Out-Null
    }

    Write-Host "  + worktree  $($wt.Name) (Phase $($wt.Phase)) -> $Path"
    git worktree add $Path $wt.Branch 2>&1 | Out-Null

    # Write AGENT.md into the worktree
    $AgentContent = @"
# Agent Brief: $($wt.Name)

**Branch:** ``$($wt.Branch)``
**Worktree:** ``$Path``
**Phase:** $($wt.Phase)

## Your Mission

Read **PLAN.md** in the repo root — find the section for ``$($wt.Name)``.
It contains your exact task, the code you need to write, and your success criteria.

## Hard Rules

1. **Never edit shared headers.** You can read ``*.hpp`` files anywhere, but only
   edit ``.cpp`` files inside your assigned ``src/core/`` subtree.
2. **Every commit must build cleanly.** Run the build check below before committing.
3. **Open a PR against ``main``** when your success criteria are all checked off.
   Do not push directly to ``main``.

## Build Check

```
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DTRIDENT_USE_DYNARMIC=ON
cmake --build build --config Debug -j4
```

## Key Files for This Agent

- ``PLAN.md``           — full brief and implementation details
- ``PROGRESS.md``       — project history and context
- ``src/core/``         — all core source (read anything, edit only your subtree)
- ``third_party/dynarmic/src/dynarmic/interface/A32/config.h``  — Dynarmic API (CPU agent)
"@

    Set-Content -Path (Join-Path $Path "AGENT.md") -Value $AgentContent -Encoding UTF8
}

Write-Host ""
Write-Host "Done. Active worktrees:"
git worktree list
Write-Host ""
Write-Host "Spawn agents:"
Write-Host "  Phase 1 (start now — all parallel):"
Write-Host "    CPU agent    -> $(Join-Path $ParentDir 'trident-agent-cpu-jit')"
Write-Host "    Loader agent -> $(Join-Path $ParentDir 'trident-agent-loader')"
Write-Host "    Kernel agent -> $(Join-Path $ParentDir 'trident-agent-kernel-hle')"
Write-Host "    CI agent     -> $(Join-Path $ParentDir 'trident-agent-ci-testing')"
