---
name: profile-and-apply-changes
description: Use when comparing WinUI performance candidates against a repeatable baseline using harness snapshots and frame evidence.
---

# Profile and apply changes

Invoke:

```powershell
& <harness>\Invoke-ProfilerHarness.ps1 `
  -Stage profile-and-apply-changes `
  -Config <config.json> `
  -Engine agency
```

**The legacy stage name does not authorize host patch application.** Follow the [VM prerequisites](../../README.md#prerequisites); no host agents, builds, tests, deployment, profiling, or automatic `git apply`.

Rebuild the baseline in the guest. Require an application-specific reset before each capture. Take 3-10 baseline samples, with first-frame and post-scenario snapshots, SQLite/frame/object-span exports, and clean ETW evidence. Analyze all baseline samples through the chosen guest engine; distinguish observed event spans from proven object lifetime.

Propose edits in guest worktrees. Reject binary, empty, or duplicate patches. Validate at most five distinct candidates through `app-build-deploy` and equally sized capture sets; every paired sample must meet the threshold with valid evidence. Stop immediately on build/deploy/capture/diagnostic errors for manual checkpoint recovery.

On consistent improvement, export the source patch/report and stop for **manual devbox review**. Never apply it automatically. Otherwise stop at five candidates or the bounded proposal budget. Retain guest worktrees, tasks, and evidence; do not claim transactional rollback or statistical proof from a single p95 sample.
