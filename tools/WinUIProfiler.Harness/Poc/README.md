# WinUI profiler ADE POCs

These are local proofs of ADE's execution engine and its connection to
WinUIProfiler, not the production harness. POC source lives in mux; installations, staged
samples, ADE working copies, and reports live outside the repository.

| Mode | Workload | Acceptance evidence |
| --- | --- | --- |
| `Synthetic` (default) | Python catalog | Protected normalization-work count |
| `Control` | Real WinUI Button/StackPanel app | ETL -> WinUIProfiler SQLite -> scenario-scoped layout-pass count |

The synthetic sample's case-insensitive catalog repeatedly normalizes every label for
every query. Correctness tests pass, but a protected check fails a deterministic
work budget across ten independent iterations. ADE proposes a fix, waits for a
person to approve its exact hash, edits its own copy, verifies the result, and
produces a patch and evidence. The seed remains intentionally inefficient.

For the synthetic mode, no VM, WinUI product build, ETW capture, or extra Python package is required.
Reduced normalization counts do not prove a wall-clock or WinUI improvement.
The Control mode now composes measurement, ADE research and ADE code execution
into an approval-gated investigation loop. These are not three standalone
production profiler-agent services.

## What belongs where

| Location | Ownership |
| --- | --- |
| `Poc\Invoke-AdePoc.ps1` | Thin ADE command adapter and explicit staging |
| `Poc\helpers` | Shared capture, extraction, queries, flow orchestration, validation and reporting |
| `Poc\ControlDemo` | Test app, bounded action runner/validator/catalog, build script, providers/markers, and protected `scenario.json` |
| `Poc\sample` | Synthetic Python demonstration |
| `Poc\tests` | Harness/helper tests |
| `.github\skills\winui-profiler-scenario` | Copilot conversational scenario intake, clarification and confirmation |
| `eng\ade\ui-profiler` | Generated plugin and separate synthetic, control, and investigation packs |
| `%LOCALAPPDATA%\ade\runtime\0.1.0` | Shared ADE runtime, not checked in |
| `%LOCALAPPDATA%\WinUIProfilerHarness\Poc` | Unique staged sample and state per preparation |
| `%LOCALAPPDATA%\WinUIProfilerHarness\Builds` | Unique short-path control builds referenced by capture manifests |
| `%LOCALAPPDATA%\ade\runs` | ADE working copies, proposals, patches, logs, and reports |

The packs target their small staged samples only. Do not point them at the full
mux repository: their checks and protected paths are sample-relative.

ControlDemo consumes the shared helpers; it does not own them. Its `scenario.json`
defines the build output, launch arguments, measured source files, marker provider
and payloads, correctness expectations, editable files, and layout budget. Shared
helpers take an explicit scenario/source root instead of assuming 40 buttons or a
particular app project. Another app still needs its own build/scenario integration;
the entry point currently exposes only the two POC modes.

## Prerequisites

- Windows, PowerShell 7, Git, and Python 3.10 or later on PATH.
- ADE platform package 0.1.0, installed using its setup script.
- Agency/Copilot authentication for the live coding-agent execution.

If the runtime is not installed, run the supplied platform setup script from
the mux root, substituting your package path:

```powershell
pwsh -NoProfile -File '<package>\setup.ps1' `
    -Team ui-profiler `
    -DisplayName 'WinUI Profiler' `
    -OutDir "$PWD\eng\ade\ui-profiler"
```

The package owns generated plugin plumbing. This repository owns `pack`,
`control-pack`, and `investigation-pack`.
An existing runtime with conflicting bytes at the same version must be
resolved explicitly; do not force-replace a shared installation automatically.

## Synthetic engine walkthrough

Run from the mux root:

```powershell
$poc = '.\tools\WinUIProfiler.Harness\Poc\Invoke-AdePoc.ps1'
& $poc -Action Prepare
```

Preparation checks ADE, stages only the small sample outside mux, verifies the
passing correctness tests and the expected failing work-budget check, and
prints `POC-STATE: <path>`. Set that exact path:

```powershell
$state = '<path printed by Prepare>'
& $poc -Action Propose -StatePath $state
& $poc -Action Report -StatePath $state
```

Read the proposal and its baseline checks, selected knowledge, protected paths,
and SHA-256 hash. **Stop here until the person approves this exact proposal.**
Generating a proposal is not permission to execute it.

Only after explicit approval:

```powershell
& $poc -Action Approve -StatePath $state -ProposalSha256 '<approved hash>'
& $poc -Action Run -StatePath $state
& $poc -Action Report -StatePath $state
& $poc -Action Verify -StatePath $state
```

`Status` is available for inspection. `Run` invokes the shared ADE engine; this
script implements no separate coding-agent or repair loop. ADE owns approval,
execution, verification, and the maximum of two execution rounds.

Expected successful evidence:

- The correctness tests remain passing.
- Each work-count iteration meets the unchanged budget.
- `diff.patch` changes only `workload.py` in ADE's working copy.
- The original sample in mux and the staged source remain unchanged.
- The ADE report names the outcome; failures are not converted into success.
- `Verify` checks the evidence hash chain, not the quality of the fix or a
  cryptographic signature.

The before/after workload includes construction and all queries. Elapsed times
are informational, not the acceptance signal.

## Real WinUIProfiler walkthrough

This mode adds a genuine WinUI control, ETL capture, profiler import, and
SQLite-backed verification. It uses a deliberately inefficient **app workload**,
not a newly discovered framework bug. See [ControlDemo](ControlDemo/README.md)
for the metric definition, permissions, and privacy boundaries.

Additional prerequisites: an interactive Windows x64 desktop, .NET 10 SDK,
enabled ETW capture permissions, and the repository's initialized build
environment. If `initrun.ps1` requests a full init, run `.\init.ps1 amd64chk`
before continuing.

Build the existing profiler CLI from a fresh PowerShell session. Using the
system .NET host explicitly keeps this .NET 10 tool separate from the product's
repository-local SDK. Substitute your host path for a nonstandard installation:

```powershell
$dotnet = Join-Path $env:ProgramFiles 'dotnet\dotnet.exe'
pwsh -NoProfile -Command {
    $root = $PWD.Path
    $dotnet = Join-Path $env:ProgramFiles 'dotnet\dotnet.exe'
    & "$root\initrun.ps1" -Command @(
        $dotnet, 'build', "$root\tools\WinUIProfiler\WinUIProfiler.Cli\WinUIProfiler.Cli.csproj",
        '--configuration', 'Release', '--verbosity', 'quiet', '--nologo'
    )
}

$poc = '.\tools\WinUIProfiler.Harness\Poc\Invoke-AdePoc.ps1'
& $poc -Action Prepare -Scenario Control -DotNet $dotnet
$state = '<new Control POC-STATE path>'
& $poc -Action Propose -StatePath $state
& $poc -Action Report -StatePath $state
```

Preparation stages the demo at the working root, shared code under `helpers`,
and its protected helper tests under `tests`, all outside mux. It
builds the staged app and performs ten fresh capture/import iterations; all
forty-button correctness checks must pass, while the initial layout budget must
fail. Every run retains its ETL, SQLite session, app result, and runtime identity.

Read and approve this **new** proposal's exact hash before using the same
`Approve`, `Run`, `Report`, and `Verify` actions shown above. A previous synthetic
approval does not authorize this run. ADE can edit only `ScenarioWorkload.cs`;
it reruns the real ten-iteration verifier after the change. The budget is at most
four complete layout passes per iteration. Scenario durations are also reported,
but are not a statistical acceptance criterion.

The initial capture backend is **logman**, not WPR. No `.wprp`, VM support,
TestGenerator, live resource analysis, or MAF orchestration is implemented.
The framework DLL is the locked self-contained public package, not local mux
product-build output. Raw provider-wide ETLs can include other apps; only the
owned demo PID is imported, and traces must remain local.

### Where model reasoning happens

The original guided path uses a model for read-only planning and the approved code change. The model
receives a task already narrowed to redundant layout work, the small scenario's
source, and the selected control knowledge. It identifies the per-button
`UpdateLayout()` call as avoidable work.

The measurements are not model-generated: `helpers\metrics.py` uses fixed SQLite
queries over WinUIProfiler output with the app's scenario contract. The live demonstration measured 40 layout
passes before the change and one afterward in each of ten runs, while preserving
all 40 buttons and the loaded framework binary. That validates this guided
app-level diagnosis. It is not autonomous root-cause discovery from an arbitrary
trace.

### Profiler-driven investigation

The opt-in investigation path reuses ADE's document/research workflow rather
than introducing another agent runtime. It supplies a problem statement, the
baseline capture, and a source snapshot matching the measured hashes. It does
not supply the original scenario README, an existing fix, or a proposed cause.

After preparing a new Control state, propose the investigation:

```powershell
& $poc -Action Investigate -StatePath $state `
    -Problem 'Investigate the cost of constructing the control tree.'
& $poc -Action Report -Phase Investigation -StatePath $state
```

Read this proposal and obtain explicit approval of its exact hash. Approval of
an earlier fix or another investigation does not authorize it:

```powershell
& $poc -Action Approve -Phase Investigation -StatePath $state -ProposalSha256 '<approved investigation hash>'
& $poc -Action Run -Phase Investigation -StatePath $state
& $poc -Action Report -Phase Investigation -StatePath $state
& $poc -Action Verify -Phase Investigation -StatePath $state
```

The model must query the real profiler before inspecting app source, record
initial observations, select a drilldown, and correlate the evidence with code.
`helpers\profiler_query.py` exposes CLI overview, tick, region, event, element, and object
history queries. Its scenario query reads the existing SQLite `EventDetails`
view within the exact UI-thread/marker interval. This matters when an operation
runs outside an instrumented tick; startup regions cannot stand in for it.

ADE writes `ade-output\investigation.md` and `decision.json` without changing
source. Input hashes and query receipts are checked after execution. They
establish which evidence was queried, not that the diagnosis is correct or
that these files are cryptographically authenticated.

If the decision is `RecommendationReady`, the usual `Propose` action now uses
that diagnosis instead of the original guided task. Review and approve the new
fix proposal separately before running it. `NeedsMeasurement` blocks a fix
proposal and lists the extra evidence required; automatic recapture/narrowing
cycles remain future work.

This is a controlled seeded scenario, not an unseen diagnosis benchmark.
The model can propose a source-level cause from telemetry plus source, but
causality still needs the later approved change and remeasurement. This does
not implement a general MAF investigation graph or the profiler's live/MCP API.

Inside a staged working copy, the shared entry points are:

```powershell
python -B -m helpers.verify --root .
python -B -m helpers.profiler_query overview --root . --iteration 1
```

The app's `scenario.json` and the `helpers` directory are protected in both ADE
workflows. Investigation staging includes only its configured measured source
snapshots, scenario contract, and query/extraction helpers, not the demo README.

### Natural-language ControlDemo scenarios

Describe and refine the flow **in Copilot chat**. The repository's
`winui-profiler-scenario` skill guides the conversation:

> **User:** Profile ControlDemo: populate 60 buttons, make the window larger,
> scroll to the end, then clear it.
>
> **Copilot:** What window width and height in physical pixels?
>
> **User:** 800 by 600.
>
> **Copilot:** Start empty; measure populate 60, resize to 800x600, scroll to
> the end, then clear. Repeat in ten fresh processes after window load.
> These are app-owned actions, not physical mouse input. Run this scenario?

The user can revise the flow before confirming. They do not write JSON, run
commands, or type hashes. Merely discussing a scenario never starts the app.
Copilot itself supplies the model reasoning; scenario intake does not launch
a separate agent/ADE approval cycle for every conversational turn.
If the user just wants to open the app, `Explore -Scenario Control` builds and
opens its default 40-button window without ETW or measurement. It stays open
until closed. This is separate from measuring startup, which is not supported
by the current action runner.

Behind the scenes, Copilot retains the scenario-related request and explicit
clarifications in session artifacts, compiles a bounded action plan, and runs
`scenario_plan.py review`. The deterministic validator checks request identity,
exact fields and parameter bounds. The user confirms a readable plan because
syntax validation cannot establish semantic faithfulness. Unsupported actions
or missing necessary parameters are explained or clarified, never silently
dropped or replaced with the default flow.

Only after confirmation does Copilot invoke `Prepare -Scenario Control
-ScenarioPlan <path> -ScenarioPlanSha256 <reviewed hash>`. The hash is supplied
by the agent, not the user, and prevents executing different plan bytes after
review. This is a Copilot skill over the existing local harness, not a standalone
MAF conversational service.

`Prepare` freezes a validated plan in a new external sample, compiles matching
marker/correctness expectations, builds once and captures ten fresh runs.
Each run verifies the frozen plan's SHA-256, executed step count, final button
tree and every step's action outcome. Per-step ETW markers and
`executedSteps` in each `app-result.json` retain actual counts, window
dimensions and scroll offsets. The plan is included in measured source hashes
and investigation snapshots; changing it during a measurement invalidates it.

Supported actions are `populate` (replace with 1-200 buttons), `clear`,
`resize` (400-1600 by 300-1000 physical pixels), and `scroll` to `start`/`end`.
A plan has at most 12 ordered steps. The app rejects scrolling content that
does not actually overflow. `initialButtonCount` is unmeasured setup, only
when explicitly requested; ordinary population remains inside the markers.
Steps use app-owned hooks, not physical mouse/keyboard input. Clicks, typing,
navigation, arbitrary waits/loops, other controls/apps and launch measurements
are not supported in this milestone.

`-Problem` still only asks an investigation question; it does not change actions.
Custom flows report **Measured**, not a performance pass: no layout budget is
invented. Use the integrated flow below for diagnosis and an evidence-backed
fix-acceptance contract. Direct seeded fix `Propose` remains blocked for custom
plans without that contract. A legitimate zero-layout interval needs
explicit scenario opt-in and positive XAML layout-provider evidence from the
same process/UI thread elsewhere in the complete trace. Lost, missing-provider
or unbalanced telemetry is still invalid.

Omitting `-ScenarioPlan` preserves the original 40-button fail-to-pass demo.
Other apps and arbitrary scenario/test-code generation remain future work.

### Integrated investigation and validation loop

After a confirmed scenario has been prepared/measured, Copilot normally starts
this flow instead of stopping at a measurement table. It also accepts an explicit
previous Control state without changing its retained evidence:

```powershell
& $poc -Action StartFlow -StatePath $state `
    -Problem 'Does this measured scenario contain avoidable layout work?'
# Retain the exact POC-FLOW printed above.
$flow = '<flow.json printed by StartFlow>'
& $poc -Action FlowStatus -FlowPath $flow
& $poc -Action FlowReport -FlowPath $flow
```

StartFlow stages an isolated source snapshot and proposes the first research
run. **It does not approve or execute it.** Read the pending ADE report and obtain
human approval of that exact proposal. Copilot supplies the hash on the user's
behalf only after that confirmation:

```powershell
& $poc -Action ContinueFlow -FlowPath $flow -ProposalSha256 '<approved pending hash>'
```

Each approval permits only its pending phase. The driver resumes automatically
until the next approval or terminal outcome:

1. Query profiler evidence, correlate source, and return a structured diagnosis.
2. If inconclusive, repeat ten unchanged runs with a narrower question, then
   propose another approved analysis. New instrumentation/scenarios stop as NeedsInput.
3. If a fix is justified, propose a separately approved ADE `perf-optimize` run.
   The recommendation specifies a meaningful per-run layout-count reduction.
   Its protected contract seals baseline evidence and preserves the exact plan,
   setup, correctness, markers, runtime and profiler. The unchanged baseline
   fails without being recaptured. Only configured app source is editable.
4. ADE builds, captures, checks and repairs in its working copy. A passing ADE
   run triggers a separate harness-owned build and ten fresh captures. The
   harness independently recounts both SQLite sets and checks every paired run.
5. A separately approved post-fix analysis evaluates whether that controlled
   change supports the proposed cause. Only then can the flow report Accepted.

No evidence of a problem is **NotDemonstrated**, not an invitation to manufacture
a fix. Other explicit outcomes are Rejected, Inconclusive, NeedsInput,
FixNotValidated, EnvironmentFailure, RecoveryRequired and TimedOut. Capture or
build failures never establish that a hypothesis was false.
Passing a count gate supports an app-level work reduction, not a statistical
latency improvement or a general framework root cause.

`-MaxAnalysisCycles` defaults to 3 (including post-fix analysis);
`-FlowTimeoutSeconds` defaults to 3600 and includes approval waits. The deadline
is checked at safe phase boundaries, not by killing active builds/captures.
ADE retains its bounded execution/repair/check timeouts. If interrupted during
an operation, the driver refuses to replay it automatically: inspect the named
ADE run/log before starting a new flow. A per-flow file lock prevents concurrent
advancement. Wrong or stale approval hashes cannot start execution.

`flow.json` and `report.md` retain the hypothesis/refinement history, inputs,
decisions, invocation logs, native ADE results and per-run validation comparison.
Evidence and controller files are hash-sealed across gates. Each ADE run retains
its own detailed report, diff and hash-chained tool log. This is the POC's unified
report, not the production LogMaster service.

The orchestrator never applies a patch to mux, commits, pushes or edits a PR.
Code/binaries remain in the explicitly reported ADE working copy/build outputs.
Legacy Synthetic and manually staged Control operations remain available.

### See the app and audit the result

The default app closes immediately after writing its result, so the benchmark
window can disappear before it is noticed. With a newly prepared Control state:

```powershell
& $poc -Action Show -StatePath $state -DisplaySeconds 30
# After a successful fix run:
& $poc -Action Show -StatePath $state -Variant Candidate -DisplaySeconds 30
```

This builds the staged baseline or successful ADE candidate and performs one observation capture. The window
stays open for the requested duration **after** the scenario Stop marker, then
closes. The title confirms the final button-count correctness check. This is separate
from the ten-iteration acceptance check, and is not a replay of an old binary.
States staged before the shared-helper layout need a fresh `Prepare` for
measurement, investigation, or fix actions. Existing ADE runs/workspaces are not
rewritten; `Status`, `Verify`, and `Report` remain available for those runs.

To independently recount a retained baseline/candidate pair:

```powershell
& $poc -Action Evidence -StatePath $state `
    -CandidateManifest '<exact candidate capture-manifest.json path from the ADE target-check log>'
```

This reopens every session read-only, checks the process/thread/marker and
capture-health requirements, and prints each run's counts and both manifest
and executable paths. It refuses duplicate iterations, different framework
hashes, or source changes outside the workload. Existing binaries are checked
against the recorded hashes. No "latest" capture is selected automatically.
New captures also retain `build.log` and the managed app DLL hash; older
manifests recorded only the executable hash.

## Run focused checks

From the mux root:

```powershell
python -B -m unittest discover -s tools\WinUIProfiler.Harness\Poc\sample\tests -t tools\WinUIProfiler.Harness\Poc\sample
python -B -m unittest discover -s tools\WinUIProfiler.Harness\Poc\tests -t tools\WinUIProfiler.Harness\Poc
```

The sample tests protect API behavior. The harness tests prove that the checker
rejects repeated scanning, accepts an indexed implementation, and refuses to
report incorrect results as performance success. The test implementation of an
index is not a pre-applied fix to the seed. Shared extractor tests use synthetic
SQLite fixtures to test process/thread/marker scoping and reject incomplete data;
the real acceptance command imports actual ETLs through WinUIProfiler.

## Boundaries and next PRs

ADE's working copy is not a sandbox. Its shell runs with the engineer's rights.
The tool only stages explicit sample files, and it rejects artifact roots
inside mux. Pack tools and knowledge are trusted inputs, not an OS security
boundary. Review the patch; this adapter never applies it to mux.

After evaluating this POC, implement the planned production work as separate
PRs: contracts/skeleton, a real control scenario, automated local capture,
extraction, comparison, framework deployment, ADE integration, and agents.
Do not treat these bounded POC checkers as qualified production performance
verifiers.
