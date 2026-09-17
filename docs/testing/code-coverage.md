# Runtime code coverage (experimental)

Coverage records execution of loose `Microsoft.ui.xaml.dll` (MUX) and
`Microsoft.UI.Xaml.Controls.dll` (MUXC) copies in the runtime-test payload.
Here, *loose* means DLL files outside an APPX/MSIX package, even if the host has
package identity.
Reports are available in Visual Studio format and Azure's **Code Coverage** tab.
**Reports and downloadable coverage artifacts are currently MS internal only.**

**`CollectCodeCoverage` defaults to false.** Enabling it does not change test
selection, build/OS matrices, triggers, or schedules. No coverage percentage
threshold is enforced.

## Pipelines

These pipelines are **MS internal only**:

- WinUI-GitHub-PR (OneBranch)
- WinUI-GitHub-Nightly

## How coverage is wired

| Phase | Implementation |
| --- | --- |
| Build | [Build template](../../build/AzurePipelinesTemplates/WinUI-BuildWinUI-Stage.yml) sets `WinUICollectCodeCoverage=true`. Debug MUXC gains linker fixup metadata; MUXC incremental linking is disabled. MUX already has fixups. |
| Prepare | [Payload template](../../build/AzurePipelinesTemplates/WinUI-CreateTestPayload-Job.yml) replaces loose runtime copies with final product DLLs and downloads their two matching PDBs. |
| Instrument | [Instrumentation script](../../Helix/common/pipeline/coverage/Instrument-CoveragePayload.ps1) distributes instrumented DLLs, `static_covrun*.dll`, and the bundled VS collector. |
| Collect | [Collector wrapper](../../Helix/common/pipeline/coverage/Invoke-WithCodeCoverage.ps1) starts a collector per parallel test job (*slice*) around the normal test runner. |
| Merge | [Merge job](../../build/AzurePipelinesTemplates/WinUI-MergeCodeCoverage-Job.yml) validates and merges successful-slice reports, populates Azure's **Code Coverage** tab, and saves downloadable reports. |

Coverage instrumentation requires each DLL's matching PDB. Matching source code
is not enough: a DLL from an earlier link must not be paired with a later link's PDB.
Preparation therefore replaces all loose MUX/MUXC copies from test apps with the
final product DLLs and uses their matching PDBs from the same build.
When reusing outputs, choose a coverage-enabled build and keep those pairs together.
PDBs temporarily placed beside DLLs for instrumentation are removed afterward.

Instrumentation and merge agents require Visual Studio's native
`Microsoft.CodeCoverage.Console.exe`; `dotnet-coverage` alone is insufficient.
Test agents use the bundled collector. Only static native MUX/MUXC instrumentation
is enabled. Hash markers detect inconsistent payloads and support same-session
instrumentation retries.

Each agent uses a local collector with the run's `winui-<BuildId>` session ID.
After launching the collector, the wrapper waits up to 30 seconds for its pipe.
After tests return or throw, the wrapper requests shutdown. Its waits for the
shutdown client and collector share a 60-second budget. Cleanup terminates remaining
processes started by the wrapper. Forced job cancellation can interrupt cleanup.
Setup/shutdown problems normally warn without replacing test results.

Two preflight failures stop tests: an existing collector session, which is left
untouched, and inability to check or remove stale reports/settings. The latter
suppresses the entire slice artifact, including test logs, to prevent stale data
from being published.

### Collector access

The wrapper uses the supported
[`AllowedUsers` setting](https://github.com/microsoft/codecoverage/blob/main/docs/configuration.md)
to grant named accounts access to collector shared memory and pipes.

Generated `<slice-report>.config` allows the collector process owner
(`Win32_Process.GetOwner`) and, if present, the console user
(`Win32_ComputerSystem.UserName`). Names are deduplicated, resolved to SIDs, and
logged. Querying the process owner avoids using an impersonated thread's identity.
Checked-in settings stay unchanged.

Without a console user, only the collector account is allowed; this does not
provision an interactive desktop. Remote Desktop and other test accounts are not
automatically included. Account discovery, SID resolution, or settings failures
prevent collector startup but still allow tests to run. The wrapper does not retry
with broader permissions.

## Which tests contribute

Only execution in an instrumented MUX/MUXC copy that can reach the collector
counts. These paths come from the [payload layout](../../test/CreateTestPayload.ps1):

| Test family | Does coverage work? |
| --- | --- |
| Controls API and interaction tests | **Yes.** Verified for representative controls API and unpackaged interaction tests using loose MUX/MUXC copies. The test driver's code does not count. |
| Other selected loose apps, including `TabViewTearOutApp` | **Yes, if** they execute instrumented MUX/MUXC copies and reach the collector. This path is eligible but has not been separately verified for every app. |
| Native/managed dxaml desktop integration tests | **Yes.** Verified for representative native WPF, managed WPF, and native Win32Explicit tests. |
| Native UAP tests | **No in the verified cases.** Tests passed but produced empty reports; see the hosting-mode limitation below. |
| Isolated unit tests: statically linked product code | **No.** That code is not part of the instrumented runtime DLLs. Calls into instrumented MUX/MUXC DLLs can still contribute. |
| `IXMPTestApp` and other embedded APPX/MSIX runtime copies | **No.** Embedded runtime copies are not instrumented; packages are neither rebuilt nor re-signed. |
| Scenario/sample/Gallery tests | **No.** These run separately without coverage collection. |

Coverage can work even when Windows treats the test app as a packaged app.
For example, controls API tests use TAEF's `PackagedCWA` mode but load instrumented
DLLs from the test payload, rather than uninstrumented copies inside an APPX/MSIX
package. Tests must still be built, enabled, and selected.
Test assemblies and other product binaries, including Tabular, are not measured.
Collection is per slice, not per test.

### Hosting-mode limitations

VM checks using collector 17.14.3, x86 Debug WinUI, and Windows 11 build 26678
confirmed MUX/MUXC coverage with `AllowedUsers` for controls API, unpackaged
interaction, native WPF, managed WPF, and native Win32Explicit representatives.
Default collector permissions produced empty reports. Hosting modes and
instrumented binaries were unchanged between comparisons.

**Native UAP remains a coverage gap.** Representative tests passed but produced
empty reports despite loading both instrumented DLLs and the coverage runtime.
That host ran at low integrity in an AppContainer. The desktop comparison does not
establish UAP support; UAP tests still run unchanged.

A combined report can contain coverage from desktop tests even when UAP tests
contribute nothing.

## Run coverage in Azure Pipelines

1. Open **WinUI-GitHub-PR (OneBranch)** above, select **Run pipeline**, and choose
   the branch containing the coverage changes.
2. Enable **Collect runtime code coverage (experimental)** and leave
   **Run full WinUI PR validation stages** (`runFullValidation`) enabled.
3. Queue the run. Follow `Build`, then `RunTests` and `MergeCodeCoverage`.
4. Inspect the **Code Coverage** tab, slice artifacts, and collector warnings.

With Azure CLI and its DevOps extension authenticated, substitute your organization
URL, project, and pipeline ID:

```powershell
az pipelines run `
    --org "<organization-url>" `
    --project "<project>" `
    --id "<pipeline-id>" `
    --branch "<branch>" `
    --parameters CollectCodeCoverage=true runFullValidation=true
```

Nightly uses `CollectCodeCoverage=true` without changing other nightly parameters,
including signing/publishing. Missing resources require maintainer authorization.
If the checkbox is absent, check that the selected branch has the YAML parameter.

### Retrying failed test jobs

If flaky tests require **Rerun failed jobs**, the final coverage report uses only
reports published with a successful status. Reports labeled failed or canceled are
excluded. Already-published successful reports are reused, not replaced by a later
attempt. Merging waits until the runtime-test jobs succeed.

This supports retries before coverage publication. Rerunning a merge after it
has published can still fail because the `MergeCodeCoverage` artifact already exists.

## Reports and percentages

Merged reports are in **MergeCodeCoverage**; slice files are in test-output
artifacts under OS/build-flavor directories.

| File | Purpose |
| --- | --- |
| `CodeCoverage/merged.cobertura.xml` | Input to the Azure coverage publisher. |
| `CodeCoverage/merged.coverage` | Visual Studio report. |
| `coverage-<machine>-slice<N>.coverage` | Slice report. |
| `<slice-report>.config` | Generated settings and account allowlist. |
| `<slice-report>.log`, `.err`, `.shutdown.log`, `.shutdown.err` | Collector and shutdown output; `<slice-report>` includes `.coverage`. |

### What is Cobertura?

Cobertura is an XML format for code-coverage reports. It records source files,
line numbers, and whether those lines executed during tests. Here, it is an output
format, not a separate tool that runs the tests or collects coverage.

The merge step uses the Visual Studio coverage tool to generate
`merged.cobertura.xml` from the collected `.coverage` files. Azure's
`PublishCodeCoverageResults@2` task reads that XML to populate the pipeline's
**Code Coverage** tab.

### Comparing line counts

The native Cobertura XML root counts method-level line entries, which can repeat
across templates, methods, and DLLs. Independent source-line deduplication instead
counts each normalized file/line location once, covered if any entry has hits.
See [Open issues](#open-issues) for the unresolved Azure summary discrepancy.

Use the same counting rule; check revision, test selection, and completeness.
Do not average slice percentages or add module totals: source locations overlap.
Coverage is not a measure of every repository line or every compiled instance.

### Report completeness and failures

[Merge-CodeCoverage.ps1](../../Helix/common/pipeline/coverage/Merge-CodeCoverage.ps1)
converts each downloaded report separately and requires source-line data because
the VS tool can silently skip corrupt inputs. No inputs, empty reports, failed
conversions, and reports without source-line data fail the merge.

Validation does not enforce the expected slice count or nonzero hits in each
DLL. Missing individual reports can go unnoticed, and a valid report may be partial.
Passing tests or publication alone do not prove coverage. Check all expected slices,
collector account/SID logs, setup/shutdown warnings, and actual executed lines in
both DLLs. Investigate account mismatches rather than broadening access to all users.

## Open issues

**Azure summary counting:** In buildId `157708100` (MS internal),
Azure reported **61.67%**, while independent source-line
deduplication gave **74.42%**. Those counts still differ, although the latest run
below agrees at **74.43%**. The earlier difference remains unexplained; do not assume
the metrics always agree or infer a collection failure from this alone.

**Collector error policy:** Setup/shutdown errors still normally warn without
failing tests. Fail-fast behavior for coverage-enabled runs has been proposed but
is not implemented or validated.

**Local build and VM coverage:** Collection has been demonstrated with custom
orchestration, but the init/build scripts, [run-tests-on-vm.ps1](../../tools/run-tests-on-vm.ps1),
and the [`test-on-vm` skill](../../.github/skills/test-on-vm/SKILL.md) have no dedicated
coverage option. A local workflow needs a build-time opt-in, not just a test switch:
set the existing `WinUICollectCodeCoverage=true` property and ensure the affected
DLLs are relinked. This enables Debug MUXC fixup metadata and disables MUXC incremental
linking; MUX already emits fixups. These settings prepare binaries for instrumentation
but do not add probes. The workflow must then instrument matching DLL/PDB payload
copies, deploy them, collect inside the VM, and retrieve reports using the existing
coverage helpers.

## Validation status

Most recent coverage test: buildId `157773727` (MS internal),
with `CollectCodeCoverage=true` and
`runFullValidation=true`, **succeeded after retries**. All 60 runtime slices passed:
20 per OS, with 58 retaining their first-attempt results. Two slices passed on retry
after an agent failed to connect and a separate job reached its 100-minute limit.

The merge was skipped on the unsuccessful attempt. After the retries, it validated
and merged 60 successful-slice reports, excluding the old canceled report.
Publication succeeded. Setup/shutdown log checks found no collector warnings in
the 60 selected slices. Independent
analysis confirmed executed source lines in both MUX and MUXC:
**436,329 / 586,231 unique file/line locations (74.43%)** combined, matching Azure's
summary. This was a coverage-on run, not a coverage-off lab validation.

The collector-exit warning fix added after this run has passed local regression
tests but has not yet been exercised in a pipeline.

## Maintaining the coverage flow

Use the local [Pester suite and optional native smoke test](../../Helix/common/pipeline/coverage/tests/README.md)
for script changes. Local validation passed 99 tests per PowerShell host and 54
template comparisons. These local checks do not exercise Azure's retry scheduling
or every TAEF host's collector access.

Compare coverage-off PR/nightly expansion with baseline; keep enabled test scope.
Collector changes need representative host checks. Coverage adds instrumentation,
downloads, storage, and one validation conversion per slice.
