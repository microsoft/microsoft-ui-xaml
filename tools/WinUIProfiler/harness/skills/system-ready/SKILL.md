---
name: system-ready
description: Use when preparing an isolated VM for the WinUI profiling harness or checking its prerequisites.
---

# System ready

Invoke:

```powershell
& <harness>\Invoke-ProfilerHarness.ps1 -Stage system-ready -Config <config.json>
```

**VM only.** Never execute builds, installers, deployment, tests, profiling, or agents on the devbox. A worktree is not a sandbox.

Show the [README prerequisites](../../README.md#prerequisites) first and confirm them individually. Ask for IXP VM, local Hyper-V VM (default), or devbox; devbox execution is blocked, not a fallback. For IXP, direct the user to the separate manual `scripts\Initialize-IxpVm.ps1` wizard. Never run that wizard or import IXPTools automatically.

Use schema 2, pinned VM ID/guest MachineGuid/computer name, an existing running VM, and a clean post-setup checkpoint. Explicit execution consent is required. Read app/profiler repository build documentation; existing source is copied, missing repositories cloned, and topic branches created **inside the VM**.

The controller checks independent guest Agency/GitHub Copilot installations, builds/deploys the profiler before the app, then verifies launch, a first drawn frame, and two private snapshots. Authentication is manual in the guest; never copy devbox tokens. A failed prerequisite, build, deployment, or probe stops readiness. Preserve evidence and require manual checkpoint recovery; package metadata is not a full rollback.
