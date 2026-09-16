# Runtime code coverage (experimental)

Runtime code coverage records execution of `Microsoft.ui.xaml.dll` (MUX) and
`Microsoft.UI.Xaml.Controls.dll` (MUXC) during Azure Pipelines runtime tests.
Instrumentation adds probes to loose copies of these DLLs in the test payload.
The pipeline merges their execution data into reports for Visual Studio and
the Azure DevOps **Code Coverage** tab.

Coverage is opt-in and defaults to off. Enabling it does not change which build
flavors, OSes, or tests run, and does not instrument shipped packages.
The coverage flow does not enforce a percentage threshold.

> Run coverage only on isolated, disposable test agents. The collector wrapper
> relaxes the permissions on its named pipe. See [Collector access](#collector-access).

## Pipelines

The coverage entry points are Azure Pipelines definitions backed by this GitHub
repository, not GitHub Actions workflows.

| Pipeline | YAML entry point | Coverage scope |
| --- | --- | --- |
| [WinUI-GitHub-PR (OneBranch)](https://dev.azure.com/microsoft/WinUI/_build?definitionId=195405) (MS internal) | [WinUI-GitHub-PR.yml](../../build/WinUI-GitHub-PR.yml) | DevTestSuite |
| [WinUI-GitHub-Nightly](https://dev.azure.com/microsoft/WinUI/_build?definitionId=199443) (MS internal) | [WinUI-Nightly.yml](../../build/WinUI-Nightly.yml) | DevTestSuite |

Both expose the boolean **CollectCodeCoverage** parameter, defaulting to
**false**. They pass it to shared build and runtime-test templates as
`collectCodeCoverage`. When it is false, template expansion omits coverage-specific
build settings, payload preparation, instrumentation, symbol downloads, and merging.
The test task invokes the test runner without the collector wrapper.

The [RunTests stage](../../build/AzurePipelinesTemplates/WinUI-RunTests-Stage.yml)
selects Win10-RS5, Win11-23H2, and Win11-25H2 for these entry points.
The [payload matrix](../../build/AzurePipelinesTemplates/WinUI-CreateTestPayload-Job.yml)
selects x86 Debug (`x86chk`), and the
[test-job template](../../build/AzurePipelinesTemplates/WinUI-RunTestPassOnPipeline-Job.yml)
splits each OS pass into 20 parallel jobs, called *slices*. Other build flavors,
static tests, and scenario tests retain their normal pipeline behavior; they do
not add data to this coverage report.

## How coverage is wired

| Phase | Work | Source |
| --- | --- | --- |
| Build | Pass `WinUICollectCodeCoverage=true` to MSBuild for the coverage-enabled build. | [WinUI-BuildWinUI-Stage.yml](../../build/AzurePipelinesTemplates/WinUI-BuildWinUI-Stage.yml) |
| Prepare | Create the test payload, replace loose MUX/MUXC copies with final product DLLs, and download their two matching PDBs. | [WinUI-CreateTestPayload-Job.yml](../../build/AzurePipelinesTemplates/WinUI-CreateTestPayload-Job.yml) |
| Instrument | Instrument the two runtime DLLs, distribute them and `static_covrun*.dll` to their payload locations, and bundle the collector. | [Instrument-CoveragePayload.ps1](../../Helix/common/pipeline/coverage/Instrument-CoveragePayload.ps1) |
| Collect | Start one collector per slice, invoke the normal test runner, and shut down the collector to write its report. | [Invoke-WithCodeCoverage.ps1](../../Helix/common/pipeline/coverage/Invoke-WithCodeCoverage.ps1) |
| Merge and publish | Download slice reports, validate and merge them, and publish Cobertura to the Code Coverage tab. | [WinUI-MergeCodeCoverage-Job.yml](../../build/AzurePipelinesTemplates/WinUI-MergeCodeCoverage-Job.yml) |

### Build metadata and matching symbols

Native instrumentation needs linker fixup information in addition to the symbols
used for debugging. The coverage build settings in
[Microsoft.UI.Xaml.Common.targets](../../controls/dev/dll/Microsoft.UI.Xaml.Common.targets)
add `/DEBUGTYPE:cv,fixup` to Debug MUXC and disable incremental linking for MUXC.
MUX already emits the required fixup information. Coverage-off builds retain
their normal linker settings.

Test apps can carry component-package DLLs from an earlier link than the final
product output. Even when the machine code is identical, the DLLs can identify
different PDBs. As with debugging, the DLL's embedded PDB GUID and age must match
the PDB used for instrumentation; a matching source commit or file version is
not sufficient.

The **Prepare final runtime copies for code coverage** task replaces every loose
MUX/MUXC copy with the DLL from `drop\<buildFlavor>\Product`. The next download
selects only `Symbols/Product/Microsoft.ui.xaml.pdb` and
`Symbols/Product/Microsoft.UI.Xaml.Controls.pdb` from the same build.
Instrumentation temporarily stages each PDB beside its DLL, then removes the
staged PDB from the test payload. Product artifacts and signed test packages
remain unchanged.

If build-output reuse is configured, the binaries and symbols must come from the
same coverage-enabled build. Queue-time variable permissions still apply.

### Instrumentation and collection

[Get-CoverageTool.ps1](../../Helix/common/pipeline/coverage/Get-CoverageTool.ps1)
locates Visual Studio's `Microsoft.CodeCoverage.Console.exe`.
Build and merge agents must provide the native coverage tool. `dotnet-coverage`
alone does not support native instrumentation. The payload bundles the collector
from the instrumentation agent and its dependencies so test agents do not need a
Visual Studio or .NET CLI installation.

[coverage.config](../../Helix/common/pipeline/coverage/coverage.config) enables
static native instrumentation and disables dynamic native and managed
instrumentation. The script instruments only MUX and MUXC, not every DLL in the
payload. Hash markers track original and instrumented copies to detect
inconsistent inputs and allow instrumentation retries for the same session.

All OS payloads in a run use the session ID `winui-<BuildId>` because the test
jobs copy the OS payloads into a common directory on each agent. Each slice starts
its own local collector for that session and invokes
[RunTestPassSliceOnBuildAgent.ps1](../../Helix/common/pipeline/RunTestPassSliceOnBuildAgent.ps1)
with the normal arguments.

The wrapper waits up to 30 seconds for the collector pipe. It attempts shutdown
in `finally`, including when tests throw, with a default 60-second wall-clock
budget for the shutdown client and collector. Stalled processes started by the
wrapper are terminated. Except for the preflight failures below, collector setup
and shutdown errors produce warnings without replacing the test runner's exit
code or exception.

Before loading the session, the wrapper removes any previous report and generated
settings at the slice's output path. If it cannot check or remove those files,
it stops before running tests and suppresses the slice's entire test-output
artifact, including test logs.
This prevents an old report from being published as data from the current attempt.

Before starting a collector, the wrapper checks whether the session pipe already
exists. If it does, the wrapper fails before running tests, changing pipe
permissions, or sending a shutdown command. A launched collector that has already
exited is also not sent a shutdown command by session name.

### Collector access

The wrapper uses the supported
[`AllowedUsers` setting](https://github.com/microsoft/codecoverage/blob/main/docs/configuration.md)
to grant named accounts access to the collector's shared memory and pipes.
It does not set a NULL DACL or change integrity labels.

Each slice generates `<slice-report>.config` from the checked-in instrumentation
settings. It includes the wrapper process's owner and, if present, the logged-in
console user reported by `Win32_ComputerSystem.UserName`. Duplicate names are
removed. Process ownership comes from `Win32_Process.GetOwner`, rather than the
PowerShell thread identity, which can be impersonated during remoting.
Each account must resolve to a SID before the collector starts. Account names
and SIDs are logged, and the generated settings are kept beside the report.
The checked-in settings are not modified.

If no console user is logged in, the wrapper logs that fact and allows only the
collector account. This does not provision a desktop or make interactive tests
runnable. Account-discovery, SID-resolution, and settings-write failures use the
existing coverage-setup warning policy: tests still run, but no collector starts.
There is no retry with default permissions or a NULL DACL.
Remote Desktop users and other test accounts are not enumerated automatically.

A separate VM comparison used the same payload and versions described under
[Hosting-mode limitations](#hosting-mode-limitations), with one representative
from each of the five contributing desktop paths. It kept the instrumented DLLs
and hosting modes unchanged and used a fresh collector for each test.

| Collector permissions | Result across the five desktop representatives |
| --- | --- |
| Previous NULL-DACL workaround | All tests passed; all five reports covered MUX and MUXC. |
| Default permissions, without the pipe ACL edit | All tests passed; all five reports were empty. |
| Explicit `AllowedUsers`, without the pipe ACL edit | All tests passed; all five reports covered MUX and MUXC. |

Repeating the controls API and interaction cases with both permission
configurations produced identical covered MUXC source-line sets between the
repeats and small MUX differences: 20 differing lines for the API test and eight
for the interaction test. The initial runs also varied in telemetry and UI paths.
These results establish collection from the tested hosts, not identical execution
on every run.

The VM's `AllowedUsers` configuration named the actual test account, whose SID
matched the observed medium-integrity product hosts. The pipeline candidate
replaces the previous NULL DACL with per-agent account discovery. It still needs
lab validation with the actual pipeline accounts and OS matrix; the VM comparison
did not validate that discovery on lab agents.

## Which tests contribute

Coverage follows the binary that executes the product code, not the test's source
directory. This table describes the contribution paths in the
[test-job definitions](../../build/AzurePipelinesTemplates/WinUI-CreateTestPayload-Job.yml)
and [payload layout](../../test/CreateTestPayload.ps1). A test contributes only
when it runs an instrumented runtime copy that can communicate with the collector.
The table describes eligibility, not a guarantee that every selected host contributes.
An eligible test can pass without contributing coverage if its host cannot
communicate with the collector. See the measured hosting-mode boundary below.

| Test family | Coverage eligibility |
| --- | --- |
| Controls API tests under `controls/dev/.../APITests` | Eligible. They compile into the loose `MUXControlsTestApp.dll`, which TAEF runs against instrumented runtime copies. |
| Controls interaction tests under `controls/dev/.../InteractionTests` | Eligible. `MUXControls.Test.dll` drives the unpackaged `MUXControlsTestApp.exe`. Product execution in the app can count; the test driver's own code does not. |
| Other loose controls test apps, such as `TabViewTearOutApp` | Eligible when selected. Their co-located MUX/MUXC copies are instrumented too. |
| Native integration tests under `dxaml/test/native/external` | Eligible for the selected UAP, WPF, and Win32Explicit groups. Their hosts consume loose payload files. |
| Managed integration tests under `dxaml/test/managed` | Eligible for the selected WPF group. The pipeline does not schedule UAP-only managed tests. |
| `Microsoft.UI.Xaml.Tests.Isolated.*` unit tests, including tests under `dxaml/xcp/components/.../unittests` | They run, but product code statically linked into their test DLLs is not instrumented. Only calls into an instrumented runtime DLL can contribute. |
| `controls/test/IXMPTestApp` | No coverage for its embedded runtime copies. The self-contained `IXMPTestApp.appx` is left unchanged. |
| Controls scenario/sample/Gallery tests | No. The separate `ScenarioTestSuite` flow does not enable coverage collection. |

A packaged host is not necessarily excluded. Controls API tests use TAEF's
`PackagedCWA` host with loose payload files. The
[native dxaml host project](../../dxaml/test/infra/taefhostapp/taefhostapp.vcxproj)
also deploys its executable and dependencies as loose files, rather than deploying
its built APPX. Runtime copies inside an already-built APPX/MSIX package are not
instrumented. Loose deployment avoids that exclusion but does not establish
collector access for the host.

Controls tests can cover both MUXC and the underlying MUX implementation. However,
the metric includes only those two DLLs: test assemblies, compiler binaries,
dependencies, and the separate `Microsoft.UI.Xaml.Controls.Tabular.dll` are not
instrumented. Executing a statically linked copy of the same source code does not
count as executing the instrumented runtime copy.

Tests must also be built, enabled, and selected for the run's architecture, OS,
hosting mode, and test query.
Ignored tests and tests filtered out for that configuration do not contribute.
Collection is per slice, not per individual test, so a merged report does not
identify which test covered a particular line.

### Hosting-mode limitations

Successful MUX/MUXC pipeline reports establish collection in aggregate. They do not
establish coverage independently for WPF, Win32Explicit, UAP, or `PackagedCWA`.
Measure representative tests separately to establish that boundary. A hosting-mode
name or package identity alone does not establish the process's account,
integrity level, or AppContainer restrictions.

Representative checks with `Microsoft.CodeCoverage.Console` 17.14.3, an
instrumented x86 Debug WinUI payload, and Windows 11 build 26678 produced the
following results. Each test used a fresh collector and separate report with the
NULL-DACL workaround. The tests retained their declared hosting modes.

| Execution path | Representative test | Observed runtime coverage |
| --- | --- | --- |
| Controls API, `PackagedCWA` | `ThemeResourcesTests.VerifyOverrides` | MUX and MUXC |
| Controls interaction, unpackaged app | `RatingControlTests.BasicInteractionTest` | MUX and MUXC |
| Native WPF | `BasicPointerTests::ProtectedCursorOnNonLiveElement` | MUX and MUXC |
| Managed WPF | `WPFTests.VerifyPLMHandlerNoException` | MUX and MUXC |
| Native Win32Explicit | `XamlIslandTests::IslandWithMuxcDoesntPoisonThread` | MUX and MUXC |
| Native UAP | `BasicPointerTests::VisualTreeHelperHitTest` | None, although the test passed |

All representatives in this table passed. The contributing product hosts ran at
medium integrity without AppContainer restrictions. The API and dxaml desktop
hosts had package identity; the interaction app did not.

The native UAP host, `taefhostapp.exe`, ran at low integrity in an AppContainer.
It loaded the instrumented MUX and MUXC DLLs and the native coverage runtime,
executed the WinUI hit-test assertions, and produced an empty report. Repeating
that test reproduced the result. A separate WPF test using the same instrumented
DLL paths produced coverage in both DLLs. Native UAP is therefore a demonstrated
coverage gap for this configuration, not just an untested deployment path.

These checks characterize the tested payload, collector version, architecture,
and OS; they are not a guarantee for every test or pipeline OS. A different
Win32Explicit test, `WindowlessXamlIslandTests::ValidateUiaTree`, timed out.
Two enabled `XamlIslandTests` alternatives passed and contributed coverage.
In particular, `WindowsXamlManagerCreationScenarios` loaded and covered MUX alone;
absence of MUXC from that report was not a collector-access failure.

In isolated x86 and x64 native fixture checks with
`Microsoft.CodeCoverage.Console` 17.14.3, low-integrity and no-capabilities
AppContainer processes executed the instrumented code but produced no coverage
with the NULL-DACL workaround. An x64 TAEF `RunAs:LowIL` fixture had the same
result. Enabling `AllowLowIntegrityProcesses` and explicit `AllowedUsers` did not
resolve those cases. The desktop `AllowedUsers` comparison described under
[Collector access](#collector-access) does not establish collection from
low-integrity or AppContainer hosts. These checks do not establish behavior for
other collector versions.

To establish a hosting-mode boundary, run a representative test separately using
the real WinUI payload and its intended host. Record the OS, build flavor,
collector version, and host's security context. Check for executed source lines in
each runtime DLL the test is expected to exercise. Keep the payload and hosting
mode unchanged when comparing collector configurations; do not change the host's
security context merely to obtain coverage.

## Run coverage manually

For a PR validation run, open **WinUI-GitHub-PR (OneBranch)** from the pipeline
table above.

1. Select **Run pipeline**, then choose the GitHub branch to measure.
2. Enable **Collect runtime code coverage (experimental)** (`CollectCodeCoverage`).
3. Leave **Run full WinUI PR validation stages** (`runFullValidation`) enabled.
   This option controls whether the product build and test stages run.
4. Select **Run**. This queues a real build and lab test pass, not a YAML-only preview.
5. Follow the `Build` and `RunTests` stages. Within `RunTests`, expect payload
   instrumentation, the OS test jobs, and a final `MergeCodeCoverage` job.
6. Open the run's **Code Coverage** tab for the summary and inspect the slice
   warnings and artifacts described below.

If the coverage checkbox is missing, confirm that the selected branch contains
the parameter in `build/WinUI-GitHub-PR.yml`. Use **Run pipeline** to select new
parameters; retrying jobs retains the original run's source version and parameters.
If Azure DevOps requests resource authorization, a maintainer must authorize the
required resource.

Alternatively, with Azure CLI and the Azure DevOps extension authenticated to the
Microsoft-internal WinUI project:

```powershell
az pipelines run `
    --org https://dev.azure.com/microsoft `
    --project WinUI `
    --id 195405 `
    --branch "<branch>" `
    --parameters CollectCodeCoverage=true runFullValidation=true
```

For nightly coverage, use **WinUI-GitHub-Nightly** with `CollectCodeCoverage=true`.
This enables collection in DevTestSuite without skipping nightly stages or changing
other nightly parameters, including signing and publishing.
Enabling coverage for a queued run does not change pipeline defaults or schedules.

### Retrying failed test jobs

`MergeCodeCoverage` depends on the selected runtime test-job groups and uses
`succeeded()`. A failed or canceled dependency skips the merge rather than
publishing coverage from an unfinished test pass.

Use **Rerun failed jobs** for `RunTests` to retry the failed test jobs and their
dependent merge job. Successful test jobs keep their existing artifacts. Once
the dependencies succeed, the merge downloads reports only from artifacts whose
names end in `_Succeeded`, including successful retries. Reports in `_Failed`
and `_Canceled` artifacts remain available for diagnosis but are not merged.
This prevents a failed attempt's corrupt or partial report from interfering with
a successful retry.

The existing test-output publisher reuses a `_Succeeded` artifact if it already
exists, rather than replacing it with a later attempt. The merge filter selects
successful-status artifacts, not the latest attempt of each slice.

Start a new coverage-enabled run to try a pipeline change; retrying an older run
retains its original YAML and parameters. These changes support retrying failed
tests before coverage is published. They do not replace an already-published
`MergeCodeCoverage` artifact; rerunning a merge after publication can still
encounter an artifact-name collision.

### Validate collector account access in the lab

Use a coverage-enabled PR run with normal full validation and the normal test
matrix. No additional pipeline parameter is needed for `AllowedUsers`.

1. Check the `Run Tests` log for `Coverage collector account`, `Coverage console
   account`, and `Coverage allowed user` entries. Confirm the named accounts cover
   the actual test hosts. A missing console user or a different test account
   requires investigation; do not broaden access to all local users.
2. Inspect each slice's `.coverage.config`, `.coverage.log`, and `.coverage.err`.
   Look for setup warnings, identity-mapping errors, and collector failures.
3. Check that the expected slices arrived and that both MUX and MUXC have executed
   source lines. Compare representative desktop hosting modes, not just the
   merged percentage. Passing tests alone do not establish collector access.
4. Confirm collector shutdown and the accepted native UAP coverage limitation.
   Do not change TAEF hosting modes to make collection pass.

## Reports and percentages

The **MergeCodeCoverage** artifact contains the merged reports. Per-slice reports
and collector logs are in the test-output artifacts, under the OS and build-flavor
directories.

| File | Purpose |
| --- | --- |
| `CodeCoverage/merged.cobertura.xml` | Source-line data passed to `PublishCodeCoverageResults@2` for the Azure Code Coverage tab. |
| `CodeCoverage/merged.coverage` | Binary coverage report for Visual Studio. |
| `coverage-<machine>-slice<N>.coverage` | A slice's input to the merge. |
| `<slice-report>.config` | Exact per-slice collection settings, including the account allowlist. |
| `<slice-report>.log` and `<slice-report>.err` | Collector standard output and standard error, where `<slice-report>` includes the `.coverage` extension. |
| `<slice-report>.shutdown.log` and `<slice-report>.shutdown.err` | Shutdown-client standard output and standard error. |

### Why Azure and Cobertura percentages can differ

Use the top-level **Code Coverage** summary when comparing results in Azure DevOps.
Its source-line percentage can differ from the `line-rate` attribute at the root
of the native collector's Cobertura XML:

- The native XML root and package summaries count method-level line entries.
  A source line can appear in multiple methods or C++ template instantiations,
  and in both runtime DLLs.
- The Azure summary combines entries by source-file path and line number. A
  location counts as covered if any of its entries has nonzero hits.

For example, suppose a report contains these four method-level entries:

| Source location | Compiled instance | Has hits |
| --- | --- | --- |
| `Example.cpp`, line 10 | A | Yes |
| `Example.cpp`, line 10 | B | No |
| `Example.cpp`, line 11 | A | Yes |
| `Example.cpp`, line 12 | A | No |

The method-entry calculation is **2 covered entries out of 4: 50%**.
The unique-source-line calculation is **2 covered locations out of 3: 66.67%**.
Both describe the same execution data. A covered source line does not imply
that every compiled instance of it was exercised.

For the unique-line metric, the denominator is the set of source locations
represented in the report, not every line in the repository.
A percentage for MUX and MUXC is not a percentage
for all WinUI binaries or test configurations. Do not average slice percentages
to obtain the total: the same locations can be exercised in multiple slices.
Shared source locations can also appear in both modules, so adding their counts
need not equal the combined unique-line count.

When comparing runs, use the same counting rule and check the source revision,
test selection, and report completeness. Do not label the raw XML root percentage
as the Azure summary.

### Report completeness and failures

Test outcomes and coverage collection are separate. Failed tests can leave useful
coverage data, and passing tests do not prove that collection worked.
The merge job waits for successful test dependencies and selects only
`_Succeeded` test-output artifacts. Cancellation can interrupt collection,
upload, or merging. See [Retrying failed test jobs](#retrying-failed-test-jobs).

[Merge-CodeCoverage.ps1](../../Helix/common/pipeline/coverage/Merge-CodeCoverage.ps1)
converts every downloaded slice report separately and requires source-line data
before merging. This detects corrupt reports that the VS tool can otherwise skip
while returning success. The merge job fails if there are no input files, if an
input file is empty, if a conversion fails, or if a report has no source-line data.

Validation checks the files that arrived; it does not enforce an expected slice
count. A missing slice can leave a valid but incomplete aggregate. A canceled or
interrupted collector can also produce a valid report containing only part of a
test pass, so even a complete file count does not prove complete execution.
Likewise, data from other processes in a slice can produce a valid report even
when one hosting mode contributes nothing.

Before relying on the percentage, check that the intended slices finished,
inspect collector warnings, and confirm source-line data for both MUX and MUXC.
For a failed slice, inspect its collector and shutdown logs alongside the test
results. Do not infer report completeness from a successful publish task alone.

## Maintaining the coverage flow

Use the standalone
[script regression suite and native smoke test](../../Helix/common/pipeline/coverage/tests/README.md)
when changing coverage helpers. The smoke test exercises native instrumentation
and merging; it does not exercise TAEF host permissions.

For pipeline changes, compare coverage-off template expansion with the baseline
for both PR and nightly entry points. Verify that coverage-specific settings and
steps are absent and that normal matrices, test arguments, and publishing remain
unchanged. With coverage enabled, verify payload preparation, both runtime modules,
slice outputs, collector cleanup, and merge/publication without reducing the
normal test scope. Changes to collector versions or permissions also require
representative lab runs across the selected hosting modes.

Coverage adds symbol downloads, payload and report storage, and instrumentation
time. Merge validation performs one conversion per slice and uses temporary space
for one Cobertura report before producing the final reports.
