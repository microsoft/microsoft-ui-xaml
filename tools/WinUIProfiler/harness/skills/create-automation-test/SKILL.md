---
name: create-automation-test
description: Convert a natural-language desktop scenario into the validated portable harness UI Automation JSON format.
---

# Create automation test

Invoke:

```powershell
pwsh <harness>\Invoke-ProfilerHarness.ps1 `
  -Stage create-automation-test `
  -Config <config.json> `
  -Engine agency `
  -ScenarioPrompt "<scenario>" `
  -ScenarioPath <output.json>
```

The selected engine may only propose JSON. The controller parses and validates the bounded action vocabulary before saving it. The resulting scenario is target-independent and can run locally, on a plain Hyper-V VM, or on an Ixp VM.

