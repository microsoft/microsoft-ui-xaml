---
name: winui-profiler-scenario
description: 'Discuss, refine, and run ControlDemo performance scenarios through Copilot chat. Use when the user describes an app flow to profile, wants to talk through a scenario, or asks to change which actions the profiler harness executes.'
---

# Conversational profiler scenarios

Copilot chat is the user interface. The user describes the flow in ordinary
language, can revise it across turns, reviews readable steps, and confirms when
to run. Do not ask the user to write JSON, copy commands, or type proposal hashes.
Do not launch a separate agent or ADE workflow just to understand the scenario.
Copilot provides the model reasoning; the existing harness validates and executes
only bounded action data. This is not a standalone MAF chat service.

## Discuss the flow

If the user simply says "launch", clarify whether they mean startup measurement
or opening the app to explore. Startup measurement is not supported by this
runner. For an explicit request just to open ControlDemo, use
`Invoke-AdePoc.ps1 -Action Explore -Scenario Control`: it builds and opens the
default 40-button demo without ETW, results, markers, or automatic closure.
Run it in a detached background shell when the window must outlive the response,
verify the printed PID/HWND is live and responsive, and leave it until the user
closes it. Opening the app is not approval for any measurement or fix.

1. Use the app already identified in the conversation. If none is identified,
   ask which app. Currently only ControlDemo has an action runner. Explain that
   limitation for another app instead of silently substituting ControlDemo.
2. Read `tools\WinUIProfiler.Harness\Poc\ControlDemo\scenario-capabilities.json`
   and the harness README before proposing actions.
3. Keep the requested order and distinguish measured actions from initial setup.
   The panel starts empty unless the user explicitly requests prepopulation;
   never hide requested measured population in setup.
4. Ask only for missing necessary details using `ask_user` when available.
   Combine related questions. For example, "make it larger" needs exact physical
   pixel dimensions; "add buttons" needs a count; scroll needs start or end.
   Retain previous answers instead of restarting the conversation.
5. Explain unsupported actions or metrics before execution. Do not drop them,
   guess replacements, invent hooks, or fall back to the default 40-button flow.
   Do not generate arbitrary automation code. This runner supports populate,
   clear, resize and scroll through app-owned hooks, not physical input.

## Compile and review, without running the app

Save only scenario-related text and explicit clarifications in a new
`scenario-request.json` in the session's artifact directory:

```json
{"text": "The original scenario, followed by its explicit clarifications."}
```

Create a separate `scenario-plan.json` there. Never modify the app or the
repository's default scenario to describe a flow. Use this exact shape:

```json
{
  "schema": "winui-profiler-control-scenario/v1",
  "status": "Ready",
  "request": "The exact text from scenario-request.json.",
  "name": "A short scenario name",
  "initialButtonCount": 0,
  "steps": [
    {"action": "populate", "count": 60},
    {"action": "resize", "width": 800, "height": 600},
    {"action": "scroll", "position": "end"},
    {"action": "clear"}
  ]
}
```

The example is not a default; use only the actual requested actions.
Validate from the repository root, substituting the actual artifact paths:

```powershell
python -B .\tools\WinUIProfiler.Harness\Poc\ControlDemo\scenario_plan.py review `
    --root .\tools\WinUIProfiler.Harness\Poc\ControlDemo `
    --plan '<scenario-plan.json>' --request '<scenario-request.json>'
```

Only a Ready result can proceed. Show the user its actual ordered actions,
parameters, any unmeasured setup, final expected tree, and the ten-run scope.
State that startup is excluded, actions use app hooks, and results are
measurements, not a passed performance budget. Ask for confirmation before
building or running. A request to discuss a scenario is not permission to run it.
Keep JSON and the returned `planSha256` behind the scenes unless requested.

If the user revises anything, update the plan and scenario-related request,
revalidate, and obtain confirmation of the new readable plan. Unsupported or
ambiguous requests remain a conversation, not an executable fallback.

## Run only the confirmed plan

Invoke the existing adapter yourself, binding it to the reviewed bytes:

```powershell
& .\tools\WinUIProfiler.Harness\Poc\Invoke-AdePoc.ps1 `
    -Action Prepare -Scenario Control -ScenarioPlan '<reviewed plan path>' `
    -ScenarioPlanSha256 '<planSha256 returned by review>'
```

Use the printed `POC-STATE`, never an automatically selected latest state.
`Prepare` freezes the exact plan, builds the app and measures ten fresh processes.
If the plan changes after review, stop and review again; do not retry without
the hash. Respect the repository's build skill and capture prerequisites.
Do not install dependencies speculatively or bypass capture failures.

If the user asks to see the app, use that measurement state's `Show` action with
an agreed display duration. This replays the flow and then holds the final
window open; it is not a recording or another ten-run acceptance check.

Report the actual measurement and evidence paths. Per-run `app-result.json`
contains `executedSteps`, actual dimensions/offsets/counts, and the plan hash.
Custom flows are measurement/investigation-only: do not claim a performance
pass or reuse the seeded fix gate. Investigation or code changes require their
own separately approved ADE workflow; scenario confirmation does not approve
them. Automatic hypothesis loops, other apps, VM execution, and the full
three-agent MAF orchestrator remain separate work.
