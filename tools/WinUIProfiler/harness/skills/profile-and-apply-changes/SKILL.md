---
name: profile-and-apply-changes
description: Capture before/after profiler evidence and run the bounded evidence-gated app optimization loop.
---

# Profile and apply changes

Invoke:

```powershell
pwsh <harness>\Invoke-ProfilerHarness.ps1 `
  -Stage profile-and-apply-changes `
  -Config <config.json> `
  -Engine agency
```

The controller captures a baseline, creates isolated app worktrees, rejects empty or duplicate proposals, calls `app-build-deploy`, profiles each candidate, and applies only a verified improvement to the configured app topic branch. It stops immediately after success for manual review, or after five distinct unsuccessful suggestions for manual intervention.

