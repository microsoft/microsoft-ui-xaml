---
name: work
description: Use when asked to do engineering work with the ui-profiler ADE in a WinUI Profiler repository - fix a bug, build a feature, make a change, research a question, or run a team work type such as triage. Runs the shared ADE loop in a copy of the repository after a human approves the proposal. Do not use for quick questions about the code.
user-invocable: true
---

# WinUI Profiler ADE: do any kind of engineering work

The shared ADE runtime (`ade.exe`) does the work. It copies the repository, runs the coding agent in
a Microsoft Agent Framework loop, runs the checks and writes the evidence. Your job is to collect the
inputs, show the proposal to the user, wait for approval, run the loop and explain the result.

Do not edit the repository by hand. Hand edits are not in the run record.

## 1. Find the plugin and check the environment

Run the block below with the shell. Use the shell, not a file-reading tool, because the plugin is
outside your workspace.

```powershell
$root = @($env:AGENCY_PLUGIN_DIR, $env:PLUGIN_ROOT, $env:CLAUDE_PLUGIN_ROOT) |
    Where-Object { $_ -and (Test-Path "$_\scripts\ade.ps1") -and ((Get-Content "$_\pack\ade.team.json" -Raw) -match '"team":\s*"ui-profiler"') } |
    Select-Object -First 1
$pointer = "$env:LOCALAPPDATA\ade\plugins\ui-profiler.txt"
if (-not $root -and (Test-Path $pointer)) { $root = (Get-Content $pointer -Raw).Trim() }
if (-not $root -or -not (Test-Path "$root\scripts\ade.ps1")) { throw "ui-profiler ADE plugin not found. Run setup.ps1 from the ADE platform package." }
& "$root\scripts\ade.ps1" doctor
```

If the result is not `Healthy`, show the problems to the user and stop.
Each shell call is a new process, so repeat the `$root` block in each later call.

## 2. Choose the work type

A work type is one kind of work. It sets the instructions and the done rule (how the runtime decides that the work is done).
List the work types:

```powershell
& "$root\scripts\ade.ps1" types
```

| Done rule | Built-in work types | What the runtime needs |
|---|---|---|
| `fail-to-pass` | `bug`, `feature` | A check command that fails now and must pass at the end. |
| `checks-pass` | `change` | The team checks, and optionally a target check. A file must change. |
| `deliverable` | `research` | No check. The agent writes a document in `ade-output\`. No source file may change. |

The team can add work types, such as `triage`. A team work type with the same name replaces the built-in one.
Pick the work type that matches the request. If two match, ask the user.

## 3. Use a workstream for long work

A workstream is a named, personal context for one piece of work, such as a feature that takes weeks.
It keeps the repository, the default work type, extra checks, notes and a history of runs.
The runtime adds `notes.md` and `history.md` to the knowledge of each run in the workstream.

```powershell
& "$root\scripts\ade.ps1" workstream list
& "$root\scripts\ade.ps1" workstream new --name "<name>" --repo "<repo>" --type feature --goal "<goal>"
```

Suggest a workstream when the user describes multi-step work. Otherwise do not use one.
Tell the user to edit the `notes.md` file that `workstream new` reports with decisions and context.

## 4. Collect the inputs

| Input | When it is needed |
|---|---|
| Task | Always. One or two sentences. |
| Repository | Always, unless the workstream has it. The runtime never writes to it. |
| Work type | Always, unless the workstream has it. |
| Check | Required for `fail-to-pass`. Optional for `checks-pass`. Not allowed for `deliverable`. |

Do not guess a check. For a bug, the check reproduces the bug. For a feature, it is the acceptance test.
If the user has no check, help them write a small failing test first. Show the test to the user.
Then use the command that runs only that test. Protect the test file with `--protect <relative path>`.

## 5. Propose

```powershell
& "$root\scripts\ade.ps1" propose --type <name> --repo "<repo>" --task "<task>" [--check "<command>"] [--protect "<path>"] [--workstream "<name>"]
```

The last line starts with `ADE-RESULT:`. Read `runId`, `proposalSha256` and `report` from it.
Read the proposal with `Get-Content "<report>" -Raw`. Show the user the important parts:
the work type, the baseline checks, the protected files, the selected knowledge and the plan.

If the status is `Stopped`, explain the `stopReason` and stop:

- `CheckAlreadyPasses`: the check passes already, so it does not prove the bug or the missing feature.
- `NoVerification`: a `checks-pass` work type has no check that can verify the work. Add a check.
- `EnvironmentFailure`: a check command could not run. This is not a code failure.

## 6. Wait for approval

Ask the user to approve this exact proposal hash. Approve only after the user says yes to it.
Never approve for the user. If no person can answer, stop and leave the run awaiting approval.

```powershell
& "$root\scripts\ade.ps1" approve --run <runId> --hash <proposalSha256>
```

## 7. Run the loop

```powershell
& "$root\scripts\ade.ps1" run --run <runId>
```

A run can take several minutes. Read `report.md` from the `report` field and summarize it:
the stop reason, the rounds, the checks, the changed files and the cited knowledge.
For a document work type, the `deliverable` field has the path of the document. Summarize it.

| Stop reason | What to tell the user |
|---|---|
| `Passed` | For code, show the diff. The user applies it with the command in the report. For a document, show the summary and the path. |
| `ChecksFailed` | The loop used all rounds. Show the failing check output. |
| `DeliverableIncomplete` | The document is missing or lacks required sections. |
| `NothingChanged` | The agent made no change. Suggest a clearer task or more knowledge. |
| `ProtectedPathChanged` | The agent changed a protected file, or a document work type changed source code. |
| `ApprovalStale` or `InputsChanged` | Something changed after approval, such as workstream notes. Propose again. |
| `TimedOut`, `Cancelled` or `ProviderFailed` | Report it. Propose again when the cause is fixed. |

Do not apply the diff unless the user asks you to.

## 8. Check the record

```powershell
& "$root\scripts\ade.ps1" verify --run <runId>
```

`Intact` means the event log hash chain is unbroken. It is not a signature.