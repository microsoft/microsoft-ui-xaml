# Runtime code coverage (experimental)

Code coverage is an opt-in Azure Pipelines test mode. It instruments only
`Microsoft.ui.xaml.dll` (MUX) and `Microsoft.UI.Xaml.Controls.dll` (MUXC).
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

## First pipeline test

No new pipeline registration is required to test the port with the existing
**WinUI-GitHub-PR (OneBranch)** pipeline.

1. In Azure DevOps, select **Run pipeline**, choose this PR's GitHub branch, and
   leave **CollectCodeCoverage=false** and **runFullValidation=true**. Compare its
   expanded stages/jobs and test task inputs with a base-branch run. There should
   be no coverage build variable, instrumentation, collector, coverage symbol
   download, or merge job.
2. Queue the same commit again with **CollectCodeCoverage=true**. Confirm payload
   instrumentation finds both runtime DLLs, copies each instrumented DLL and its
   `static_covrun*.dll` runtime to every payload location, and bundles the collector.
3. Check the test-output artifacts for nonempty `coverage-<machine>-slice<N>.coverage`
   files and collector logs. Coverage setup/shutdown warnings indicate an incomplete
   result even if the tests pass. Confirm no collector remains after each slice.
4. Open the run's **Code Coverage** tab and the **MergeCodeCoverage** artifact.
   Verify that `CodeCoverage/merged.cobertura.xml` and `CodeCoverage/merged.coverage`
   are nonempty and contain coverage for both MUX and MUXC, not the test assemblies.
   The binary report can be opened in Visual Studio.
5. Repeat the default-off comparison for **WinUI-GitHub-Nightly**. For coverage-on
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

Instrumentation uses Visual Studio's `Microsoft.CodeCoverage.Console.exe`.
`dotnet-coverage` alone does not support native instrumentation. The script bundles
the VS collector and native dependencies because test agents do not have VS or
the .NET CLI installed. Build and merge images must provide the VS coverage tool.

Each slice starts a collector, invokes the unchanged test runner, and shuts down
the collector in `finally`, including when tests throw. Collector failures produce
warnings and do not prevent test execution. Missing or empty merge input fails the
coverage job rather than publishing an empty success report. A partial report can
still be published when only some slices produce data; inspect all slice warnings.

TAEF's low-integrity process host needs access to the collector's named pipe.
The port retains the experimental NULL DACL workaround from the original change.
It allows all local users to access that pipe until the collector exits. Run this
mode only on isolated, disposable test agents, not shared developer machines.

Coverage increases payload size, artifact storage, and run time. A local VM runner,
additional runtime modules, and per-PR automatic coverage are outside this change.
