# Code coverage for WinUI tests (focused pass)

EXPERIMENTAL. This is a minimal, focused code-coverage pass - not the full nightly-wide flow.

We use Visual Studio **static instrumentation** (the `Microsoft.CodeCoverage.Console` /
`dotnet-coverage` engine). It is fast at run time and it handles our out-of-process TAEF setup,
where the product DLLs load in a separate packaged app process.

**What gets instrumented:** both `Microsoft.ui.xaml.dll` (the framework/core, "MUX") and
`Microsoft.UI.Xaml.Controls.dll` (the controls, "MUXC"). Each gets its **own independent session
id**, so its own dedicated named pipe, collector process, and raw `.coverage` files - there is
no shared listener between the two, and no possibility of one module's data mixing into the
other's report. The merge step reports each DLL on its own **and** both combined - see
[Viewing the results](#viewing-the-results).

**What gets exercised:** only `DevTestSuite`, on **Win10-RS5, x86, chk** - the single OS/flavor
combination with the largest Dev test population in the nightly matrix (~6,838 unique tests /
~6,853 executions). No `ScenarioTestSuite`, no other OS or architecture, no sample apps.

## Run it

The pipeline entry point, `build\WinUI-CodeCoverage.yml`, is not in this repository: it is kept in
Microsoft's internal mirror of this repo because it references internal resources (the `.scc`
converter, package feeds, the daily schedule). That mirror receives every change merged here, so
each run covers the latest `main`. The entry point extends the shared stages below with
`collectCodeCoverage: true`; nothing else in this repository turns that switch on. It:

1. Builds x86 chk only (`WinUI-BuildWinUI-Stage.yml` with `collectCodeCoverage: true`, which
   collapses the build matrix to a single flavor).
2. Builds the test payload for a single OS leg (`WinUI-CreateTestPayload-Job.yml` with
   `collectCodeCoverage: true`), instruments both `Microsoft.ui.xaml.dll` and
   `Microsoft.UI.Xaml.Controls.dll` in that payload (`Instrument-CoveragePayload.ps1`) - each
   with its own freshly-generated, independent session id - and bundles the VS coverage
   collector tool into the payload so the lab test agent (no VS/dotnet installed) can collect on
   its own.
3. Runs the Win10-RS5 Dev test slices with **one collector per instrumented module** attached
   (`RunTestPassSliceOnBuildAgent.ps1 -CollectCodeCoverage`, which auto-discovers every module's
   session from the payload), producing one `.coverage` file **per module, per slice**.
4. Merges every slice's `.coverage` file for every module in one pass
   (`WinUI-MergeCodeCoverage-Job.yml` + `Merge-CodeCoverage.ps1`), corrects the merged data
   (`Repair-CodeCoverage.ps1`, see [Why the numbers are corrected](#why-the-numbers-are-corrected)),
   and publishes Combined / MUX / MUXC results - see [Viewing the results](#viewing-the-results).
5. Converts the same slices' coverage into StackSampler sampled-coverage `.scc` files
   (`WinUI-ConvertCoverageToScc-Job.yml` + `Convert-CoverageToScc.ps1`) - see
   [StackSampler (.scc) output](#stacksampler-scc-output). The entry point can turn this off
   (`convertCoverageToScc: false` on `WinUI-RunTests-Stage.yml`).

## Session ids: independent, auto-generated, auto-discovered

Every instrumented module gets its **own** session id, generated fresh (a random 16-hex-char
id) by `Instrument-CoveragePayload.ps1` at instrument time - there is no shared or
`Build.BuildId`-derived id passed through the pipeline. Each module's id is recorded in its own
discovery file in the payload: `_coverage-session-id-<ModuleStem>.txt` (e.g.
`_coverage-session-id-Microsoft.ui.xaml.txt` and
`_coverage-session-id-Microsoft.UI.Xaml.Controls.txt`).

`RunTestPassSliceOnBuildAgent.ps1` auto-discovers every such file in the payload and starts one
collector per module - own named pipe (`\\.\pipe\CodeCoverage.pipe.<session-id>`), own
`--output` file (`coverage-<machine>-slice<N>-<ModuleStem>.coverage`), own log files
(`_coverage-collector-<ModuleStem>.log`/`.err`). A fresh random id per instrument run can never
collide across builds; `RunTestPassSliceOnBuildAgent.ps1` still probes for a pre-existing
collector pipe before starting its own, as cheap defense against a stale collector left behind
by a retried attempt of the *same* build. Each module's collector setup/shutdown is wrapped in
its own try/catch, so a problem with one module's collector never prevents the other module's
collector from starting or being cleaned up (fail-open, per module).

## Viewing the results

Every run reports three views: **Combined** (all instrumented DLLs together), **MUX**
(`Microsoft.ui.xaml.dll`) and **MUXC** (`Microsoft.UI.Xaml.Controls.dll`).

- **Code Coverage tab** - the **Files** view has one top-level folder per DLL:
  `MUX - Microsoft.ui.xaml.dll` and `MUXC - Microsoft.UI.Xaml.Controls.dll`. Each folder shows
  that DLL's own coverage (expand it for its `dxaml\`, `controls\`, `BuildOutput\`, ... tree), and
  the **Overview** at the top is all DLLs together. Files outside the build's sources root (OS/SDK
  sources, intermediate directories) sit under `<DLL folder>\_outside-sources\` instead of being
  dropped by the tab.
- **`coverage-report.html`** in the `MergeCodeCoverage_<attempt>` artifact - a single
  self-contained page with a **Module drop-down** (Combined / MUX / MUXC), a comparison table,
  and a collapsible, searchable folder tree for the selected view. Append `#view=<ModuleStem>`
  (e.g. `#view=Microsoft.UI.Xaml.Controls`) to open a specific view.
- The merge job's log (`Merge and correct code coverage`) prints all three numbers.

The same artifact (`out\CodeCoverage\`) also contains, per view, `corrected-per-file.csv` and
`corrected-summary.json` (under `Combined\` and `<ModuleStem>\`), a per-DLL
`corrected.cobertura.xml` with real source paths (for `reportgenerator` or other Cobertura
tools), `coverage-summary.csv`, and `merged.coverage` (binary, opens in Visual Studio).

A source file compiled into more than one DLL (typically a shared header) is counted **once** in
Combined but once **per DLL** in the per-DLL views, so the per-DLL totals need not add up to the
combined total. Both are correct for what they measure. The tab's Overview is the sum of the two
DLL folders, so it counts such files once per DLL; on one run that was 74.77% versus
74.83% for the deduplicated Combined view.

### Why the tab is laid out by folder

Azure DevOps recomputes the tab's numbers from the published per-file data, merging entries that
share a path, so publishing the DLLs as separate Cobertura packages still renders as one mixed
tree. The tab's other ways to split results don't work for this pipeline:

- the **Details** view's "Build configuration | Build platform" drop-down: `PublishCodeCoverageResults@2`
  always publishes an empty configuration/platform;
- the tab's **HTML report** toggle: the task generates it, but OneBranch does not let it attach
  the `Code Coverage Report_<buildId>` artifact;
- a build-summary section: OneBranch blocks `##vso[task.uploadsummary]` in this job.

Re-rooting each DLL's files under its own folder is what keeps them apart. The cost is that the
tab's paths no longer match the repository layout. Queue the pipeline with
`CoverageTabLayout: Combined` to publish real paths instead (all DLLs merged into one tree,
Overview = deduplicated Combined).

## Why the numbers are corrected

The native coverage merge engine (shared by `dotnet-coverage` and
`Microsoft.CodeCoverage.Console.exe`) does not consolidate duplicate `(file, line)` entries when
merging `.coverage` files from separate collector processes for the same DLL - exactly our
one-collector-per-slice setup. It appends another copy of each class's `<line>` elements per
input file, so anything that sums them (the tool's own printed summary, the merged Cobertura's
root `line-rate`, `reportgenerator`) overcounts and understates coverage. For example, one run's
raw merge claimed 65.6% for MUX while the Code Coverage tab showed the true 76.4%.

`Repair-CodeCoverage.ps1` applies the same dedup the ADO tab does: it groups every `<line>` by
`(source file, line number)` across all classes that map to that file (C++ template
instantiations create many classes per file) and keeps the highest hit count. It does this per
DLL (Cobertura package) and globally for Combined. Only corrected numbers are logged and
published.

## StackSampler (.scc) output

The `ConvertCoverageToScc` job turns the slices' `.coverage` files into the `.scc` (SQLite) format
used by the Windows sampled-coverage service, so WinUI coverage can be uploaded with
`StackSampler.exe upload`. It wraps an existing converter rather than re-implementing it: a Python
package (`common_coverage`, entry point `cli\coverage_to_scc.py`) that lives outside this
repository. The entry-point pipeline declares a repository resource for it and passes, through
`WinUI-RunTests-Stage.yml`, the resource alias (`sccConverterRepository`), the folder holding the
package (`sccConverterSubdirectory`), and optionally the package feeds to use (`sccNuGetSources`,
`sccPipIndexUrls`).

How the inputs line up with what the converter needs:

- **Original DLL/PDB.** The converter maps covered source lines to RVAs, which needs the
  *uninstrumented* DLL and its own PDB - instrumentation changes the PDB signature (GUID +1, age 2)
  and the RVA layout. `Instrument-CoveragePayload.ps1 -BinariesOutDir` saves, per module, the
  original pair (`<dll>.orig` left by the instrumenter, and the PDB it instrumented with) and the
  instrumented pair (`<dll>`, `<stem>.instr.pdb`). The CreateTestPayload job zips them into
  `CoverageBinaries\coverage-binaries.zip` in its `TestPayload_<OS>_<arch>` artifact - outside
  `TestPayload\`, so test slices never download it, and zipped, so BinSkim/code-sign validation
  don't treat the unsigned or instrumented DLLs as shipping binaries.
- **Test host `BuildLabEx`.** Required for upload, and it has to come from the machine that ran
  the tests. Each RS5 slice writes `_coverage-host-os.json` (BuildLabEx, ProductName, ReleaseId,
  UBR, ...) next to its `.coverage` files.
- **All slices in one file, plus one per DLL.** `Convert-CoverageToScc.ps1` groups the slices by
  `BuildLabEx` (StackSampler groups uploads by it), merges each group to coverage XML, and runs
  `coverage_to_scc.py` with both original PDBs from a folder holding the original DLLs (so the
  `.scc` records their checksum/timestamp/size). From the same XML it then writes one `.scc` per
  DLL (`--filter`), named `-MUX` and `-MUXC`. Host fields are only written when every slice in
  the group agrees on them.
- **Checks.** Before converting, each original DLL's CodeView signature must match its PDB, and the
  PDB must have line info. After converting, each `.scc` must pass SQLite `integrity_check`, carry
  the group's `BuildLabEx`, have each expected module present with hits and the original PDB
  signature, and contain no other modules.
- **Tools.** dotnet-coverage (or the VS console tool), a 64-bit Python 3.9+ (else the official
  `python` NuGet package), `comtypes` (pip, into a private folder) and `msdia140.dll` from the VS
  DIA SDK. Feeds default to the public NuGet/PyPI feeds; the entry point can pass its own.

The `ConvertCoverageToScc_<attempt>` artifact contains, under `Scc\`:

| File | Contents |
| --- | --- |
| `winui-<buildId>-Win10-RS5-x86chk.scc` | Both DLLs (combined) |
| `winui-<buildId>-Win10-RS5-x86chk-MUX.scc` | `Microsoft.ui.xaml.dll` only |
| `winui-<buildId>-Win10-RS5-x86chk-MUXC.scc` | `Microsoft.UI.Xaml.Controls.dll` only |
| `UPLOAD.md` | The StackSampler commands to run |
| `scc-summary.json` | Hits per file and module, host metadata, converter commit, tools |
| `CoverageBinaries\coverage-binaries.zip` (+ manifest) | Original and instrumented DLL/PDB |

For a given build, upload **either** the combined file **or** the two per-DLL files, not both: the
combined file already contains every per-DLL hit, and all three share the same `BuildLabEx`, so
uploading both sets records the same coverage twice. Uploading needs an authorized account, so it
is a manual step. With StackSampler extracted to `.\StackSampler`:

```powershell
# Option A - combined
.\StackSampler\StackSampler.exe upload --file-pattern "<artifact>\Scc\winui-<buildId>-Win10-RS5-x86chk.scc" --name "WinUI <buildNumber> (<buildId>) - Win10-RS5 x86chk"
# Option B - per DLL
.\StackSampler\StackSampler.exe upload --file-pattern "<artifact>\Scc\winui-<buildId>-Win10-RS5-x86chk-MUX.scc" --name "WinUI <buildNumber> (<buildId>) - Win10-RS5 x86chk - MUX"
.\StackSampler\StackSampler.exe upload --file-pattern "<artifact>\Scc\winui-<buildId>-Win10-RS5-x86chk-MUXC.scc" --name "WinUI <buildNumber> (<buildId>) - Win10-RS5 x86chk - MUXC"
```

`.scc` is hit-only: covered and partially covered lines become RVA hits; uncovered lines and hit
counts are not represented, so it is not a substitute for the corrected line-coverage numbers above.
On a full run (20 slices, both DLLs) the conversion produced about 1.74 million unique RVA hits
(MUX about 1.18 million, MUXC about 0.57 million) in a few minutes.

To run it locally against a downloaded build:

```powershell
.\Helix\common\pipeline\coverage\Convert-CoverageToScc.ps1 `
    -InputDir <folder with the RunTestsStage_RunTestsRS5_* artifacts' CodeCoverage folders> `
    -CoverageBinaries <coverage-binaries.zip, or a folder with <Stem>\original\> `
    -ConverterRoot <folder that contains common_coverage\> `
    -OutputDir .\scc -Modules Microsoft.ui.xaml.dll,Microsoft.UI.Xaml.Controls.dll
```

## Wiring into the existing shared templates

Unlike a dedicated coverage-only copy of the RunTests stage, this pass reuses the **same**
`WinUI-RunTests-Stage.yml`, `WinUI-CreateTestPayload-Job.yml`,
`WinUI-RunTestPassOnPipeline-Job.yml`, and `WinUI-BuildWinUI-Stage.yml` that every other WinUI
pipeline (Nightly, PR, GitHubPR, Official, ValidateWindowsAppSDK) already uses. A single
`collectCodeCoverage` parameter (default `false` everywhere) gates every coverage-specific
behavior additively - every other pipeline that doesn't pass it sees no change at all.

## Files

| File | What it does |
|------|--------------|
| `build\WinUI-CodeCoverage.yml` (internal mirror only) | Pipeline entry point; turns `collectCodeCoverage` on for the shared stages, lists the modules, declares the converter repository resource, and schedules the daily run |
| `build\AzurePipelinesTemplates\WinUI-BuildWinUI-Stage.yml` | `collectCodeCoverage: true` collapses the build matrix to a single x86 chk leg |
| `build\AzurePipelinesTemplates\WinUI-CreateTestPayload-Job.yml` | `collectCodeCoverage: true` collapses the payload matrix to a single OS leg, downloads product PDBs, and calls `CreateTestPayload.ps1 -CodeCoverage -CoverageModules ...`; zips the saved original/instrumented DLL/PDB into `CoverageBinaries\coverage-binaries.zip` |
| `test\CreateTestPayload.ps1` | `-CodeCoverage` instruments the payload after building it (calls `Instrument-CoveragePayload.ps1`) |
| `Helix\common\pipeline\coverage\Instrument-CoveragePayload.ps1` | Instruments every copy of each configured module, one independent session id per module, and records each in its own discovery file; `-BinariesOutDir` saves each module's original and instrumented DLL/PDB |
| `build\AzurePipelinesTemplates\WinUI-RunTestPassOnPipeline-Job.yml` | `collectCodeCoverage: true` appends `-CollectCodeCoverage` to the slice runner and stages every module's raw `.coverage`/collector logs, plus the test host's `_coverage-host-os.json`, inside the job's existing artifact |
| `Helix\common\pipeline\RunTestPassSliceOnBuildAgent.ps1` | `-CollectCodeCoverage` auto-discovers every module's session from the payload and stands up one independent collector per module around the slice's `te.exe` runs |
| `Helix\common\pipeline\coverage\Set-CoveragePipeAcl.ps1` | Opens each collector's pipe (NULL DACL) so low-integrity `te.processhost` can connect; retries for ~15s if the pipe can't be opened yet |
| `build\AzurePipelinesTemplates\WinUI-RunTests-Stage.yml` | `collectCodeCoverage: true` runs only the RS5 job and appends the merge job; threads `coverageModuleStems` |
| `build\AzurePipelinesTemplates\WinUI-MergeCodeCoverage-Job.yml` + `Helix\common\pipeline\coverage\Merge-CodeCoverage.ps1` | Downloads every module's slice `.coverage` files, merges them in one pass, and publishes to the Code Coverage tab (one folder per DLL by default) |
| `Helix\common\pipeline\coverage\Repair-CodeCoverage.ps1` | Dedups the raw merge by `(file, line)` per DLL and combined; writes `published.cobertura.xml` (layout per `-PublishLayout`), per-view CSVs/summaries, and per-DLL Cobertura files |
| `Helix\common\pipeline\coverage\New-CoverageHtmlReport.ps1` | Renders `coverage-report.html` (Combined / per-DLL drop-down, folder tree) from `Repair-CodeCoverage.ps1`'s output |
| `build\AzurePipelinesTemplates\WinUI-ConvertCoverageToScc-Job.yml` + `Helix\common\pipeline\coverage\Convert-CoverageToScc.ps1` | Converts the slices' `.coverage` to StackSampler `.scc` with the converter supplied by the entry point and publishes it with the original/instrumented DLL/PDB (see [StackSampler (.scc) output](#stacksampler-scc-output)) |

## Known limitations / follow-ups

- Adding a third module requires updating `CoverageModules` (DLL filenames, as a YAML list) AND
  `CoverageModuleStems` (matching stems, as a YAML list) together on `WinUI-CodeCoverage.yml` -
  these two lists must stay in sync manually.
- `CoverageModules` must be a YAML **list**, never a space-separated string. It is joined with
  commas before being passed to `CreateTestPayload.ps1 -CoverageModules`, because PowerShell only
  binds multiple values to a `[string[]]` parameter when they are comma-separated. A
  space-separated value binds just the first DLL and silently spills the rest onto the next
  positional parameter (`-CoverageSessionIds`), which instruments one module and bakes a DLL name
  in as its session id. `CreateTestPayload.ps1` and `Instrument-CoveragePayload.ps1` now both
  reject that shape up front.
- Only `DevTestSuite` on Win10-RS5/x86/chk runs. No `ScenarioTestSuite`, no other OS/arch.
- **`Microsoft.UI.Xaml.Controls.dll` (MUXC) needs extra link options to be instrumentable.**
  Static native instrumentation requires the CV + FIXUP debug streams. MUXC's **Debug** (`chk`)
  configuration did not emit them, so the instrumenter refused the binary with:

  ```
  Unable to obtain debug information. Link with the /PROFILE linker switch.
  ```

  and then **exited 0**, leaving the image untouched - a fully green run that produced a
  `.coverage` file with no data and a merged report with zero lines (observed on earlier runs
  from two different branches).

  MUX (`Microsoft.ui.xaml.dll`) was never affected because `dxaml\Xaml.NTLegacy.Cpp.targets`
  passes `/debugtype:cv,fixup,pdata` unconditionally. MUXC only passed `/debugtype:cv,fixup` in
  its **Release** configuration; Debug was the gap.

  Fixed by `/p:EnableCodeCoverageLinkOptions=true`, which the coverage build leg now sets
  (`coverageLinkMSBuildArg` in `WinUI-BuildWinUI-Stage.yml`). When set, MUXC's Debug link adds
  `/debugtype:cv,fixup` and disables incremental linking (the two are incompatible). It is
  **opt-in** rather than always-on so the local Debug inner loop keeps incremental linking and
  Nightly/PR/Official builds keep their current linker flags. To reproduce locally, build MUXC
  with `/p:EnableCodeCoverageLinkOptions=true` before running `CreateTestPayload.ps1 -CodeCoverage`.

  Both failure modes are also *loud* now: `Instrument-CoveragePayload.ps1` treats an
  exit-0-but-unsuccessful instrument as a module failure, and `Merge-CodeCoverage.ps1` warns when
  a module's corrected report contains no lines at all.
- `controls\dev\dll-chart` and `controls\dev\dll-tabular` follow the same Release-only
  `/debugtype:cv,fixup` pattern, so they would need the same treatment before they could be added
  to `CoverageModules`.
- No local-VM coverage path (out of scope for this pass).
- Fail-open: collector/pipe/slice problems log an ADO warning and skip coverage for that
  module/slice rather than failing the test run. The one exception is `CreateTestPayload.ps1`,
  which hard-fails (`exit 1`) if instrumentation itself fails at payload-build time - a broken
  instrumentation there would otherwise silently ship an uninstrumented/broken payload.

