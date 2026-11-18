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
Neither mode implements the three profiler subagents.

## What belongs where

| Location | Ownership |
| --- | --- |
| `Poc\Invoke-AdePoc.ps1` | Thin ADE command adapter and explicit staging |
| `Poc\helpers` | Shared capture, extraction, profiler queries, verification, and evidence comparison |
| `Poc\ControlDemo` | Test app, its build script, providers/markers, and protected `scenario.json` |
| `Poc\sample` | Synthetic Python demonstration |
| `Poc\tests` | Harness/helper tests |
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
closes. The title confirms the 40-button correctness check. This is separate
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
