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
| `tools\WinUIProfiler.Harness\Poc` | Sample, protected checks, and thin ADE command adapter |
| `eng\ade\ui-profiler` | Generated plugin and separate synthetic/control packs |
| `%LOCALAPPDATA%\ade\runtime\0.1.0` | Shared ADE runtime, not checked in |
| `%LOCALAPPDATA%\WinUIProfilerHarness\Poc` | Unique staged sample and state per preparation |
| `%LOCALAPPDATA%\WinUIProfilerHarness\Builds` | Unique short-path control builds referenced by capture manifests |
| `%LOCALAPPDATA%\ade\runs` | ADE working copies, proposals, patches, logs, and reports |

Both packs target their small staged samples only. Do not point them at the full
mux repository: their checks and protected paths are sample-relative.

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

The package owns generated plugin plumbing. This repository owns `pack` and `control-pack`.
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

Preparation stages only the demo and protected support files outside mux. It
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

ADE uses a model for read-only planning and the approved code change. The model
receives a task already narrowed to redundant layout work, the small scenario's
source, and the selected control knowledge. It identifies the per-button
`UpdateLayout()` call as avoidable work.

The measurements are not model-generated: `metrics.py` uses fixed SQLite
queries over WinUIProfiler output. The live demonstration measured 40 layout
passes before the change and one afterward in each of ten runs, while preserving
all 40 buttons and the loaded framework binary. That validates this guided
app-level diagnosis. It is not autonomous root-cause discovery from an arbitrary
trace, and the POC does not connect an analysis agent to the profiler's live/MCP
query interface.

## Run focused checks

From the mux root:

```powershell
python -B -m unittest discover -s tools\WinUIProfiler.Harness\Poc\sample\tests -t tools\WinUIProfiler.Harness\Poc\sample
python -B -m unittest discover -s tools\WinUIProfiler.Harness\Poc\tests
python -B -m unittest discover -s tools\WinUIProfiler.Harness\Poc\ControlDemo\tests -t tools\WinUIProfiler.Harness\Poc\ControlDemo
```

The sample tests protect API behavior. The harness tests prove that the checker
rejects repeated scanning, accepts an indexed implementation, and refuses to
report incorrect results as performance success. The test implementation of an
index is not a pre-applied fix to the seed. Control extractor tests use synthetic
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
