---
name: profiler-build-deploy
description: Build WinUI Profiler and publish/deploy the self-contained headless capture driver.
---

# Profiler build and deploy

Invoke:

```powershell
pwsh <harness>\Invoke-ProfilerHarness.ps1 -Stage profiler-build-deploy -Config <config.json>
```

The stage uses `tools\WinUIProfiler\scripts\Build-App.ps1`, reads its canonical output marker, publishes the harness driver for the selected architecture, deploys complete directories including `ConnectionPayloads` and `WinUIProfiler.Native.dll`, and writes target-specific deployment state.

