---
name: create-automation-test
description: Use when a natural-language Windows application scenario needs a portable UI Automation test for the harness.
---

# Create automation test

Invoke:

```powershell
& <harness>\Invoke-ProfilerHarness.ps1 `
  -Stage create-automation-test `
  -Config <config.json> `
  -Engine agency `
  -ScenarioPrompt "<scenario>" `
  -ScenarioPath <harness>\.artifacts\generated-scenario.json
```

Complete the [prerequisites](../../README.md#prerequisites) and obtain explicit VM consent. The selected Agency or GitHub Copilot engine runs only in the pinned guest, with nonediting tool permissions. Its response must be JSON using the bounded action vocabulary; the controller validates it before saving under harness `.artifacts`. Never run an agent or scenario on the devbox.

Ask for the intended natural-language scenario and prefer stable UI Automation identifiers. Install/purchase actions need explicit user intent; do not invent them. A scenario never launches processes or executes arbitrary scripts. Review generated selectors before capture.

The DSL is portable across IXP and plain Hyper-V guests and can describe third-party apps exposing UI Automation. Private WinUI profiling additionally requires a compatible target runtime and successful readiness probe; UI Automation support alone does not establish that capability. Devbox execution remains blocked.
