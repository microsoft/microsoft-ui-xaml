---
name: app-build-deploy
description: Use when an application needs building or redeployment for a WinUI profiling run or optimization candidate.
---

# App build and deploy

Invoke:

```powershell
& <harness>\Invoke-ProfilerHarness.ps1 -Stage app-build-deploy -Config <config.json>
```

Follow the [prerequisites](../../README.md#prerequisites) and explicit VM consent contract. Devbox execution is prohibited; neither build scripts nor package registration may run on the host.

Read the application repository's README/build instructions and configure argument-array commands with `scope: "guest"`. Source snapshots and `user/<alias>/profiler_harness` branches belong in the verified guest. Missing repositories are cloned there using guest authentication. Payload preparation, signing, SDK installation, and deployment also run there. Never copy a built DLL back over a devbox DLL.

`-SkipBuild` reuses existing **guest** outputs only. Optimization attempts build edited guest worktrees without restaging over their edits. Any build/deploy error stops the loop for manual inspection/checkpoint recovery; do not reinstall a baseline or use a host fallback.
