---
name: profiler-build-deploy
description: Use when WinUI Profiler or the harness capture driver needs building or deployment to an isolated profiling VM.
---

# Profiler build and deploy

Invoke:

```powershell
& <harness>\Invoke-ProfilerHarness.ps1 -Stage profiler-build-deploy -Config <config.json>
```

Complete the [prerequisites](../../README.md#prerequisites), identity pins, clean checkpoint, and explicit VM consent first. Never build, install SDKs, deploy the profiler, or replace runtime DLLs on the devbox.

Stage the profiler source and current harness into the verified guest. Resolve/install the exact SDK from `tools\WinUIProfiler\global.json` there. Run `scripts\Build-App.ps1`, read its canonical guest output marker, publish the self-contained capture driver, and copy complete output directories **guest-to-guest**, including `ConnectionPayloads` and `WinUIProfiler.Native.dll`.

Deployment state is tied to the VM ID and guest MachineGuid. `-SkipBuild` accepts existing guest outputs, never host binaries. Profiler deployment does not establish target-app private diagnostic compatibility; `system-ready` must pass its launch/first-frame/snapshot probe. Failures stop for manual recovery, never host deployment.
