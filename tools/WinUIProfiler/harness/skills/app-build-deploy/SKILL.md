---
name: app-build-deploy
description: Build and deploy the configured application to the selected local, Hyper-V, or Ixp target.
---

# App build and deploy

Invoke:

```powershell
pwsh <harness>\Invoke-ProfilerHarness.ps1 -Stage app-build-deploy -Config <config.json>
```

Commands come from the app repository's documented build process and are executed as argument arrays, not shell text. Remote apps can provide a payload directory plus provider-specific installation commands. This stage is reusable by every optimization attempt.

