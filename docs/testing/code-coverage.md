# Runtime code coverage (experimental)

Code coverage is an opt-in Azure Pipelines test mode. It instruments only
`Microsoft.ui.xaml.dll` (MUX) and `Microsoft.UI.Xaml.Controls.dll` (MUXC).
This experimental mode covers loose DLLs in the test payload, not runtime copies
inside APPX/MSIX packages such as `IXMPTestApp.appx`.
It does not instrument test binaries, sample apps, or Windows App SDK dependencies.
It does not set a coverage threshold or add a required PR check.

## Pipelines

GitHub hosts the source, but the full build and runtime test flow uses Azure Pipelines.
`build/WinUI-GitHub-PR.yml` is the GitHub-facing Azure Pipelines entry point.
The registered pipelines are **WinUI-GitHub-PR (OneBranch)** (195405) and
**WinUI-GitHub-Nightly** (199443), both backed by this GitHub repository.
The GitHub Actions `PR Build` workflow is a separate, currently disabled, product-only
build. This change does not enable or modify it.

| Azure Pipelines YAML | Scope | Coverage default |
| --- | --- | --- |
| `build/WinUI-CodeCoverage.yml` | Manual x86 Debug build and DevTestSuite on Win11-25H2; no sample, scenario, or static test stages | Off |
| `build/WinUI-GitHub-PR.yml` | Existing GitHub PR validation; coverage applies to DevTestSuite | Off |
| `build/WinUI-Nightly.yml` | Existing nightly; coverage applies to DevTestSuite | Off |

Set **CollectCodeCoverage** to **true** when manually queuing a run.
For the GitHub PR pipeline, leave **runFullValidation** enabled.
When coverage is off, the coverage steps and merge job are omitted during template
expansion. Existing build matrices, test commands, publishing steps, and triggers
are unchanged.

The focused YAML has both CI and PR triggers disabled. Adding the file does not
register a pipeline, schedule a run, or grant resource access.

## Which tests contribute

Coverage follows the binary that executes the product code, not the test's source
directory. The following expectations come from the
[test-job definitions](../../build/AzurePipelinesTemplates/WinUI-CreateTestPayload-Job.yml)
and their deployment paths. They still need confirmation in a WinUI lab run with
a working collector.

| Test family | Expected contribution when coverage is enabled |
| --- | --- |
| Controls API tests under `controls/dev/.../APITests` | Yes. They compile into the loose `MUXControlsTestApp.dll`, which TAEF runs against instrumented runtime copies. |
| Controls interaction tests under `controls/dev/.../InteractionTests` | Yes. `MUXControls.Test.dll` drives the unpackaged `MUXControlsTestApp.exe`. Product execution in the app counts; the test driver's own code does not. |
| Other loose controls test apps, such as `TabViewTearOutApp` | Yes, when selected. Their co-located MUX/MUXC copies are instrumented too. |
| Native integration tests under `dxaml/test/native/external` | Yes, for the selected UAP, WPF, and Win32Explicit groups. Their hosts consume loose payload files. |
| Managed integration tests under `dxaml/test/managed` | Yes, for the selected WPF group. The pipeline does not schedule UAP-only managed tests. |
| `Microsoft.UI.Xaml.Tests.Isolated.*` unit tests, including tests under `dxaml/xcp/components/.../unittests` | They run, but product code statically linked into their test DLLs is not instrumented. Only calls into an instrumented runtime DLL can contribute. |
| `controls/test/IXMPTestApp` | No coverage for its embedded runtime copies. The self-contained `IXMPTestApp.appx` is left unchanged. |
| Controls scenario/sample/Gallery tests | No. They run in the separate `ScenarioTestSuite` flow, which does not enable collection in this port. |

A packaged host is not necessarily excluded. Controls API tests use TAEF's
`PackagedCWA` host with loose payload files. The
[native dxaml host project](../../dxaml/test/infra/taefhostapp/taefhostapp.vcxproj)
also deploys its executable and dependencies as loose files, rather than deploying
its built APPX. The uncovered case is a runtime copy inside an already-built
APPX/MSIX package.

Controls tests can cover both MUXC and the underlying MUX implementation. However,
the metric includes only those two DLLs: test assemblies, compiler binaries,
dependencies, and the separate `Microsoft.UI.Xaml.Controls.Tabular.dll` are not
instrumented. Executing a statically linked copy of the same source code does not
count as executing the instrumented runtime copy.

Tests must also be built, enabled, and selected for the run's architecture, OS,
hosting mode, and test query. The current coverage test jobs use x86 Debug.
Ignored tests and tests filtered out for that configuration do not contribute.
Collection is per slice, not per individual test, so a merged report does not
identify which test covered a particular line.

## Run coverage manually

Use the existing
[WinUI-GitHub-PR (OneBranch) pipeline](https://dev.azure.com/microsoft/WinUI/_build?definitionId=195405) (MS internal).
It already reads this GitHub repository and does not require a new pipeline registration.

1. Select **Run pipeline**, then choose the GitHub branch containing the coverage
   change. Before the PR merges, choose its source branch, not `main`.
2. Enable **Collect runtime code coverage (experimental)** (`CollectCodeCoverage`).
3. Leave **Run full WinUI PR validation stages** (`runFullValidation`) enabled.
   Keep the other parameters at their defaults and leave the stages selected.
4. Select **Run**. This queues a real build and lab test pass, not a YAML-only preview.
5. Follow the `Build` and `RunTests` stages. Within `RunTests`, expect payload
   instrumentation, the OS test jobs, and a final `MergeCodeCoverage` job.
6. Open the run's **Code Coverage** tab for the summary, or download
   `CodeCoverage/merged.cobertura.xml` and `CodeCoverage/merged.coverage` from the
   **MergeCodeCoverage** artifact. Inspect slice warnings as well as the summary.

If the coverage checkbox is missing, confirm that the selected branch contains
the updated `build/WinUI-GitHub-PR.yml`. Use **Run pipeline** to select new parameters;
rerunning failed jobs from an old run does not select this change or enable coverage.
If Azure DevOps requests resource authorization, a maintainer must authorize the
required resource; do not change pipeline defaults or bypass the approval.

Alternatively, with Azure CLI and the Azure DevOps extension authenticated to the
Microsoft-internal WinUI project:

```powershell
az pipelines run `
    --org https://dev.azure.com/microsoft `
    --project WinUI `
    --id 195405 `
    --branch "<branch-containing-this-change>" `
    --parameters CollectCodeCoverage=true runFullValidation=true
```

The option applies only to this queued run. Normal PR validation and nightly
defaults remain unchanged. This is the full PR pipeline, not the smaller focused
pipeline described below.

## Validate the initial port

1. Use the manual steps above with **CollectCodeCoverage=false** first. Compare
   the expanded stages/jobs and test task inputs with a base-branch run. There
   should be no coverage build variable, instrumentation, collector, coverage
   symbol download, or merge job.
2. Queue the same commit with **CollectCodeCoverage=true**. Confirm instrumentation
   finds both runtime DLLs, updates every loose copy, distributes `static_covrun*.dll`,
   and bundles the collector.
3. Check for nonempty per-slice `coverage-<machine>-slice<N>.coverage` files and
   merged reports with source-line data for both MUX and MUXC, not test assemblies.
   Collector warnings indicate incomplete results even if tests pass. Confirm no
   collector remains after each slice. Open the binary report in Visual Studio
   if needed, and account for the test-family limitations above.
4. Repeat the default-off comparison for **WinUI-GitHub-Nightly**. For coverage-on
   runs, verify that the merge includes slices from all three OS jobs.

Do not change pipeline defaults, schedules, or branch protection to test this feature.
The existing **WinUI-CodeCoverage-Experimental** pipeline (197783) is backed by the
internal ADO repository, not this GitHub repository. Running it does not test this port.

### Smaller optional run

For a smaller test, register `build/WinUI-CodeCoverage.yml` as a new GitHub-backed
Azure pipeline and select the PR branch. An authorized maintainer must grant it
access to the same template repositories, variable groups, feeds, and test pools
used by GitHub PR validation. Leave automatic triggers and schedules disabled.
Run it once with coverage off and once with coverage on. It builds only x86 Debug
and runs the Win11-25H2 test slices.

Build-output reuse requires a coverage-enabled build and its matching product symbols.
Standalone [script regression tests](../../Helix/common/pipeline/coverage/tests/README.md)
can run without creating or queuing a pipeline.

## Implementation and limitations

When enabled, the build adds linker fixup metadata to Debug MUXC so that VS can
instrument it. MUX already emits this metadata. The option also disables incremental
linking for MUXC; normal builds keep their existing linker settings.

The build agent creates the normal symbol-free test payload, then instruments it
as a separate opt-in step. Matching product PDBs are downloaded from the same build.
Only test payload copies are instrumented; shipped packages and build outputs are not.
All payloads in a run use the same session ID because test jobs overlay OS payloads.
Retry markers record original and instrumented hashes so a retry can repair
same-build copies without replacing binaries from another build.

The scripts leave signed test packages unchanged and warn about their coverage
gap. Packaged tests still run, but their embedded runtime DLLs do not contribute
to the report. Instrumenting them would require a separate package deployment or
repackaging/re-signing path; that is outside this initial port.

Instrumentation uses Visual Studio's `Microsoft.CodeCoverage.Console.exe`.
`dotnet-coverage` alone does not support native instrumentation. The script bundles
the VS collector and native dependencies because test agents do not have VS or
the .NET CLI installed. Build and merge images must provide the VS coverage tool.

Each slice starts a collector, invokes the unchanged test runner, and shuts down
the collector in `finally`, including when tests throw. Shutdown has a 60-second
wall-clock budget covering both the client and collector; stalled owned processes
are terminated without replacing the test result. Collector failures produce
warnings and do not prevent test execution. Missing or empty merge input fails the
coverage job rather than publishing an empty success report. A partial report can
still be published when only some slices produce data; inspect all slice warnings.
The merge converts each input separately and requires source-line data before
combining slices, because the VS CLI can otherwise silently skip corrupt files.
This adds one conversion per slice and temporary space for one Cobertura report.
Slices with no source-line data also fail validation; inspect their collector logs.

The port retains the original experimental NULL DACL workaround for TAEF's
access to the collector's named pipe.
It allows all local users to access that pipe until the collector exits. Run this
mode only on isolated, disposable test agents, not shared developer machines.
This changes discretionary permissions, not integrity labels. Actual lab tokens,
IPC permissions, and collector privileges still need end-to-end verification.

Coverage increases payload size, artifact storage, and run time. A local VM runner,
additional runtime modules, and per-PR automatic coverage are outside this change.
