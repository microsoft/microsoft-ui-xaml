---
name: system-ready
description: Prepare the app/profiler repositories, selected target, both agent CLIs, app deployment, and WinUI Profiler deployment.
---

# System ready

Invoke:

```powershell
pwsh <harness>\Invoke-ProfilerHarness.ps1 -Stage system-ready -Config <config.json>
```

The deterministic controller prepares `user/<alias>/profiler_harness` branches, creates or reuses the configured local/Hyper-V/Ixp target, verifies separate Agency and standalone GitHub Copilot installations, then calls the reusable `app-build-deploy` and `profiler-build-deploy` stages.

Do not place VM credentials or agent tokens in configuration files. Hyper-V credentials are read from the environment variable names in the configuration.

