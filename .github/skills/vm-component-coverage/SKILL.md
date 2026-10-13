---
name: vm-component-coverage
description: >-
  EXPERIMENTAL. Calculate code coverage of a WinUI control or component (for example
  "calculate coverage of control TitleBar") by running only that component's tests on a local
  Hyper-V VM against the local build. Reports per-file and per-function line coverage that matches
  the ADO Code Coverage tab. Use for "calculate/measure/get coverage of <control>", "how much do
  <control>'s tests cover", or "coverage before/after my tests" when you have a local build and a VM.
---

# WinUI component coverage from a local VM test run

## Agent workflow (follow in order; every gate is mandatory)

All commands run from the repo root. `<skill>` = `.github\skills\vm-component-coverage`.

### Gate 0 - Prerequisites (check; stop and tell the user on failure)

| Check | How | If missing |
|---|---|---|
| Local build of the flavor (default `amd64chk`) | `BuildOutput\bin\amd64chk\Product\Microsoft.UI.Xaml.Controls.dll` and `BuildOutput\obj\amd64chk\controls\dev\dll\**\link.command.1.tlog` exist | Follow the **build** skill. Ask before starting a full build (10+ min). |
| VS Enterprise code coverage | `Microsoft.CodeCoverage.Console.exe` under `%ProgramFiles%\Microsoft Visual Studio` | Stop; the user must install it. |
| `dotnet-coverage` | `Get-Command dotnet-coverage` | Tell the user: `dotnet tool install --global dotnet-coverage` |
| Hyper-V rights | `Get-VM` succeeds | Follow the permission steps in the **test-on-vm** skill. Never elevate or add group members yourself. |

### Gate 1 - VM details (strict: never start a run without them)

**Before every coverage run, the user must explicitly confirm the VM or enter its details in this
session.** Never pick a VM silently, even if only one credential is cached. Never reuse a VM name,
username or password from memory, an earlier session, or an earlier run without asking again.

1. Look for cached credentials. This shows the VM file and username only, never the password:
   ```powershell
   Get-ChildItem "$env:USERPROFILE\.winui-test\vmcred-*.xml" -EA SilentlyContinue |
       ForEach-Object { [pscustomobject]@{ VM = $_.BaseName -replace '^vmcred-'; UserName = (Import-Clixml $_.FullName).UserName } }
   ```
2. Use `ask_user` every time:
   - **Cached credentials exist:** ask "Is this the VM to use?" Offer one choice per cached VM and
     username, plus **"Enter different VM details"**.
   - **No cache, or the user chooses different details:** ask for the **VM name** and **VM username**
     (the plain account name as shown on the VM, with no `.\` or `VM\` prefix).
   - If the user declines or cancels, stop. Do not continue with a default.
3. Run `<skill>\scripts\Set-VmCredential.ps1 -VMName <vm> -UserName <user>`:
   - For new details, add `-Force`. The script asks for the **password in a separate window** (never in
     chat; chat text is kept in session history) and writes the encrypted per-user cache
     `%USERPROFILE%\.winui-test\vmcred-<vm>.xml`. This is the cache `tools\run-tests-on-vm.ps1` and Steps 3-4 read;
     Step 3 does not pass `-Credential` to the runner, so the cache must exist.
   - For a confirmed cached VM, run it without `-Force`. It validates without prompting.
   - Either way it checks that the VM exists and is Running, that the logon works, and that the user has an
     Active desktop session. A new credential replaces the cache only after it validates.
4. If the script fails (VM missing or stopped, bad password, no active desktop), report the message,
   ask the user to fix it or give new details, and repeat this gate. Do not continue.

### Gate 2 - Resolve the component and its tests

**A. `controls\dev` (MUXC, `Microsoft.UI.Xaml.Controls.dll`)**

1. Folder: match the name case-insensitively against `controls\dev\*` folders. If there's no match,
   search `controls\dev\*\*.idl` for `runtimeclass <Name>` and use that folder (for example
   `ItemsRepeater` resolves to `Repeater`). If there are zero or several candidates, ask the user and offer the candidates.
2. Tests: list the `[TestClass]` classes (with their namespaces) in `controls\dev\<F>\APITests\**\*.cs`
   and `controls\dev\<F>\InteractionTests\**\*.cs`. TAEF test names are `<namespace>.<class>.<method>`,
   and `runtests` selects `@Name='*<filter>'`.
   - Prefer one wildcard that covers every class, usually `*.<F>Tests.*`. It covers both the `ApiTests`
     and `InteractionTests` classes, and namespaces such as `...ApiTests.RepeaterTests.*`.
   - If no single wildcard covers them all, pass several filters (`-TestFilter 'a','b'`). Each runs
     with its own collector, and the results are merged and deduplicated in the report.
   - If the folder has no tests, tell the user and ask which tests to run.
3. `-ComponentPathRegex`: `\\controls\\dev\\<F>\\` plus `\\controls\\dev\\Generated\\(<RC1>|<RC2>|...)\.`
   for each `runtimeclass` in `controls\dev\<F>\*.idl` that has a generated file in `controls\dev\Generated`.
4. Show the resolved plan (folder, test classes, filter(s), regex), then proceed. Ask only if
   something is ambiguous.

**B. Not in `controls\dev`: investigate `dxaml` (MUX, `Microsoft.ui.xaml.dll`) and get approval**

1. Sources: search `dxaml\xcp` for the type, for example `core\core\elements\<Name>.cpp`, `core\inc\<Name>.h`,
   `dxaml\lib\<Name>_Partial.*`, `dxaml\lib\winrtgeneratedclasses\<Name>.g.*`, `components\**`, and
   `core\controls\generated\C<Name>*`. Decide which are in scope (for example whether `ButtonBase` counts for `Button`).
2. Tests: `dxaml\test\native\external\<area>\<name>\` (C++ TAEF; names look like
   `Microsoft::UI::Xaml::Tests::Controls::<Name>::<Class>::<Method>`). Note each class's `Hosting:Mode`;
   `runtests` cannot mix hosting modes in one filter, so use one filter per mode.
3. Use `ask_user` to present the proposed source files and regex, test classes and filter(s), and module
   `Microsoft.ui.xaml.dll`. Warn that MUX is about 71 MB, so the first instrumentation is likely to take
   well over 40 minutes. **Do not run until the user approves.** Revise the plan with their feedback.

**C. Found in neither place:** ask the user which source folder and tests to use.

### Run

- Output: `<repo>\CodeCoverageOutput\<Component>` (already ignored by `.gitignore`).
- Add `-RefreshPayload` if `TestPayload\x64chk` is missing or older than the build output.
- Add `-MoveSessionToConsole` when the host is an RDP or Cloud PC session (`$env:SESSIONNAME -like 'RDP-*'`).
- Tell the user the expected time first (about 30-45 min the first time for MUXC, about 6 min with the cache).
  Run it async with a long wait. **Never stop the process during Step 2 or 3**; the restore runs in `finally`.

Run from the repo root in PowerShell, invoking the script directly. Don't use `pwsh -File`: it turns
`'a','b'` into a single string.

```powershell
& .\.github\skills\vm-component-coverage\scripts\Invoke-WinUIVmCoverage.ps1 `
    -RepoRoot (Get-Location).Path -VMName <vm> -TestFilter '<filter>'[,'<filter2>'] `
    -ComponentName <F> -ComponentPathRegex '<regex>' -Module <Microsoft.UI.Xaml.Controls.dll|Microsoft.ui.xaml.dll> `
    -OutputDir .\CodeCoverageOutput\<F> [-RefreshPayload] [-MoveSessionToConsole]
```

### Report

- Confirm Step 4 restored the payload. If the log shows `RESTORE FAILED`, tell the user prominently.
- From `report-<ts>\coverage-summary.md`, present the component total, the per-file table, and the
  uncovered and partially covered functions. Point to `TreeReport\coverage-tree.html` and the CSVs.
- Check the test summary. If `Total=0`, the filter matched nothing; fix the filter instead of reporting 0 %.

---

# Reference (original skill documentation)

Measures **which lines of one component your selected tests execute**. It doesn't need a pipeline
run: it uses your local build and the same Hyper-V VM and runner (`tools\run-tests-on-vm.ps1`) you
already use for WinUI tests.

It's built from three references:

| Reference | What was reused |
|---|---|
| `microsoft-ui-xaml-lift` PR **16435603** ("Add focused code coverage pipeline") | How to make MUXC instrumentable (`/debugtype:cv,fixup` + `/INCREMENTAL:NO`, commit `2919de62`). Static instrumentation with a per-module session id, broadcast to every payload copy plus `static_covrun*.dll`. A server-mode collector with a NULL-DACL pipe (`Set-CoveragePipeAcl.ps1`, copied unmodified into `scripts\lib`). Shutdown to flush. |
| `WinUITestInventory\SkillToGetCodeCoverageFromPipeline` | The (file, line) dedup fix that makes numbers match the ADO Code Coverage tab (`Split-CoverageByModule.ps1`), the HTML tree report, and function-level coverage. These scripts are copied **unmodified** into `scripts\lib`. |
| `WinUITestInventory\CoverageReportFromScripts` | The same merge flow (`dotnet-coverage merge` to Cobertura), and the note that the PDB is only needed at instrument time. |

## Quick start

```powershell
cd <repo-root>\.github\skills\vm-component-coverage\scripts
.\Invoke-WinUIVmCoverage.ps1 `
    -RepoRoot   <repo-root> `
    -VMName     <vm> `
    -TestFilter '*.TitleBarTests.*' `
    -ComponentName TitleBar `
    -ComponentPathRegex '\\controls\\dev\\TitleBar\\|\\Generated\\TitleBar|\\generated\\Controls\\TitleBar' `
    -OutputDir  <repo-root>\CodeCoverageOutput\TitleBar `
    -MoveSessionToConsole `
    -BaselinePerFileCsv '<pipeline-skill-output>\PerModule\Microsoft.UI.Xaml.Controls\corrected-per-file.csv' `
    -BaselineLabel 'CI build 157731159 (all tests)'
```

Open `<OutputDir>\report-<timestamp>\coverage-summary.md`. Other outputs in that folder:
- `component-per-file.csv`
- `component-functions.csv`
- `TreeReport\coverage-tree.html`
- `PerModule\<Module>\corrected-per-file.csv` (whole module)

## Prerequisites

- **Local build:** a full local build of the flavor you test (default `amd64chk`), so that `BuildOutput\obj\...\link.command.1.tlog`, the objs and the PDB exist. Also an existing `TestPayload\x64chk`, created by any earlier `tools\run-tests-on-vm.ps1` run, or pass `-RefreshPayload`.
- **Visual Studio Enterprise (host):** supplies `Microsoft.CodeCoverage.Console.exe` for instrument, collect and shutdown, and the MSVC toolset that built the objs.
- **`dotnet-coverage`:** `dotnet tool install --global dotnet-coverage`, used for the merge.
- **VM access:**
  - A running Hyper-V VM with the test user logged on, an unlocked interactive desktop, and the WinUI test prerun already done (any earlier `run-tests-on-vm.ps1` run does this).
  - The host account must be a local admin or a member of Hyper-V Administrators.
  - The VM credential must be cached in `%USERPROFILE%\.winui-test\vmcred-<VM>.xml`, the same cache `run-tests-on-vm.ps1` uses.
- **No VS or dotnet needed on the VM:** the collector tools are staged to `C:\WinUICoverage\CoverageTool` automatically.

## What it does (and why)

| Step | Script | Notes |
|---|---|---|
| 1 | `Step1-New-InstrumentableDll.ps1` | **MUXC only.** chk MUXC is not instrumentable: the tool prints "*Unable to obtain debug information. Link with the /PROFILE linker switch.*" and then **exits 0** with the image untouched. The lift PR fixes this with an opt-in `EnableCodeCoverageLinkOptions` MSBuild property, which this checkout may not have, and repo files must not be edited. So the script re-runs the **exact link command MSBuild recorded** (`link.command.1.tlog`) with `/DEBUGTYPE:CV,FIXUP` added and `/INCREMENTAL:NO`. `/OUT`, `/PDB`, `/IMPLIB` and `/MAP` are redirected to `work\relink`, and `/WINMD:NO` is set. It takes about 15 s, and BuildOutput is never touched. MUX (`Microsoft.ui.xaml.dll`) already links with `/debugtype:cv,fixup,pdata` and skips this step. |
| 2 | `Step2-Instrument-Payload.ps1` | Static native instrumentation with a fresh 16-hex session id. **This is slow for a 38 MB chk DLL (about 20 min)**, so the result is cached in `work\instrumented`, keyed by the input DLL's SHA256, and reused on later runs together with its baked-in session id. The script catches the "exit 0 but not instrumented" case. It backs up and then overwrites **every** payload copy of the DLL, plus `static_covrun64.dll`, because each test app loads its own co-located copy (`Test\UnpackagedApps\MUXControlsTestApp\`). |
| 3 | `Step3-Invoke-VmCoverageRun.ps1` | Stages the collector on the VM once. It starts `collect --server-mode --session-id <sid>` as the VM user through an interactive scheduled task, waits for `\\.\pipe\CodeCoverage.pipe.<sid>`, and sets a NULL DACL on it. It then runs `initrun.ps1 .\tools\run-tests-on-vm.ps1 -SkipPayload <filter>`, which deploys the instrumented payload and runs the tests on the desktop. In `finally` it calls `shutdown <sid>` so the collector writes the `.coverage` file, then downloads it. |
| 4 | `Step4-Restore-Payload.ps1` | Always runs, from `finally`. It restores every original DLL, hash-verified, removes `static_covrun*.dll`, and does the same in the VM's deployed payload. Each distinct original is restored to its own copies. |
| 5 | `Step5-Get-ComponentCoverage.ps1` | Merges the coverage to Cobertura (`dotnet-coverage merge -f cobertura`) and applies the per-module (file, line) dedup (`lib\Split-CoverageByModule.ps1`), the same correction that matches the ADO tab. It then filters to the component by `-ComponentPathRegex` and optionally compares with a baseline `corrected-per-file.csv`. Paths are matched repo-relative, so `C:\__w\1\s\...` versus `C:\Repos\...` and `x86chk` versus `amd64chk` don't matter. It also produces function-level coverage (`lib\Get-FunctionCoverage.ps1`) and the HTML tree (`lib\Build-CoverageTreeReport.ps1`). |

You can run the steps one at a time. Each writes its state or results under `<OutputDir>\work`.

## Timing

- **First run:** about 30-45 min, almost all of it Step 2 instrumenting a 38 MB chk MUXC. The rest is a few seconds to relink, then deploy, the tests, flush and the report.
- **Later runs with an unchanged build:** about 6 min. The relink and the instrumented DLL are both reused from `<OutputDir>\work`.
- **Report:** parsing the Cobertura and native XML takes about 1-2 min each; skip the second with `-SkipFunctionCoverage`.

## Verified result (TitleBar, 2026-10-06)

`-TestFilter '*.TitleBarTests.*'` on a local Hyper-V VM (Win11 26688, amd64chk): 24/24 tests passed, and the collector produced an 88 MB `.coverage` file. These are the numbers from the full skill run:

| File | TitleBar tests only (this skill) | CI build 157731159 (all tests, pipeline skill) |
|---|---|---|
| `controls\dev\TitleBar\TitleBar.cpp` | **98.07 %** (507/517) | 10.64 % (55/517) |
| `controls\dev\TitleBar\TitleBarAutomationPeer.cpp` | **100 %** (17/17) | 0 % (0/17) |
| `controls\dev\Generated\TitleBar.properties.cpp` | **90.23 %** (157/174) | 28.74 % (50/174) |
| All 15 TitleBar source files | **81.03 %** (773/954) | 23.69 % (226/954) |

Of 45 functions in the TitleBar `.cpp` files, 40 are fully covered, 4 partially, and 1 not at all (`OnWindowRectChanged`). Afterwards, the payload, the VM and BuildOutput were all verified back at their original hashes.

## Gotchas (learned the hard way)

- **Host session disconnects (Cloud PC / RDP):** if the host's session disconnects, the Hyper-V *enhanced session* stops delivering input. UI tests then fail with `InjectMouseInput ... Access is denied` and "Test window lost focus". Use `-MoveSessionToConsole`, which runs `tscon <id> /dest:console` as SYSTEM inside the VM, so the VM desktop stays interactive. Reconnecting with vmconnect moves the session back.
- **Instrumenting is slow:** about 20 min and about 3 GB RAM for chk MUXC. Don't interrupt it; later runs reuse the cache. Use `-ForceReinstrument` after a rebuild if the hash didn't change but you want a new session id.
- **Session id must match:** the DLL the test process loads must carry the collector's session id. If the coverage file is tiny or empty, check `work\raw\<run>\collector.log` and confirm the app-local copy (`MUXControlsTestApp\Microsoft.UI.Xaml.Controls.dll`) is the instrumented one.
- **Rebuilds overwrite the instrumented copy:** rebuilding `MUXControlsTestApp` re-stages an older Controls DLL into `BuildOutput\...\UnpackagedApps\MUXControlsTestApp`. That doesn't matter here because Step 2 overwrites the *payload* copies, but don't run `run-tests-on-vm.ps1` **without** `-SkipPayload` in between, because that recreates the payload.
- **Expected failures still count toward coverage:** failing or ignored tests still execute product code until they fail, and Ignored tests don't run at all.
- **Known crash at exit (PC-3) is harmless:** TitleBar runs fail-fast at app exit, which shows as cleanup errors. Coverage still flushes because the collector is out of process.
- **Numbers differ from CI:** a local VM run uses a different OS build, flavor (x64 vs CI's x86) and test subset. Compare per file, not the module total.
- **NULL DACL:** the collector pipe is open to every local user until shutdown. That's fine on a test VM, but avoid it on shared machines.
- **`cmd /c` quoting:** starting the collector via `cmd /c "<exe>" ... > "<log>"` silently fails, because cmd strips the outer quotes. Step 3 therefore writes a `.cmd` wrapper. If the collector pipe never appears, check that wrapper and `C:\WinUICoverage\out\collector-<run>.log` on the VM.
- **Same tool for instrument and collect:** the collector must be the same `Microsoft.CodeCoverage.Console.exe` version that instrumented the DLL. Step 2 records the tool, and Step 3 stages that exact version to `C:\WinUICoverage\CoverageTool\<version>`. When several VS installs exist, the highest file version wins; path string order is not used.
- **Relink is not byte-deterministic:** Step 1 therefore reuses its previous output while the recorded link inputs are unchanged. Otherwise the Step 2 cache, keyed by DLL hash, would miss and re-instrument for 45 minutes.
- **Single-file bug in `lib\Get-FunctionCoverage.ps1`:** with a single `.coverage` input, the script's own merge produces an empty `<results/>`. An if-expression unwraps the one-element array, so `@inputFiles` splats nothing. Step 5 works around this by pre-merging (`dotnet-coverage merge -f xml`) and calling the script with `-SkipMerge`. The copied script is left unmodified.
- **Host suspends pause the run:** a Cloud PC host that sleeps or suspends pauses instrumentation; it resumes and finishes afterwards. Keep the host awake for predictable timing.

## Parameters (`Invoke-WinUIVmCoverage.ps1`)

| Parameter | Default | Notes |
|---|---|---|
| `-RepoRoot` | required | WinUI checkout with a completed local build |
| `-VMName` | required | Hyper-V VM |
| `-TestFilter` | required | One or more filters passed to `run-tests-on-vm.ps1`, e.g. `*.TitleBarTests.*`. With several, Step 3 runs once per filter and Step 5 merges the `.coverage` files. |
| `-OutputDir` | required | Work and report root |
| `-ComponentName` / `-ComponentPathRegex` | TitleBar | Regex on the source path selecting the component's files |
| `-Module` | `Microsoft.UI.Xaml.Controls.dll` | or `Microsoft.ui.xaml.dll` |
| `-Flavor` | `amd64chk` | Build flavor; payload `x64chk` |
| `-BaselinePerFileCsv` / `-BaselineLabel` | none | e.g. a CI `corrected-per-file.csv` from the pipeline skill |
| `-MoveSessionToConsole` | off | Recommended on Cloud PC / RDP hosts |
| `-RefreshPayload` | off | Run `test\CreateTestPayload.ps1` first |
| `-ForceReinstrument` | off | Ignore the instrumented-DLL cache |
| `-SkipFunctionCoverage` | off | Skip the function-level merge |

## Files in this skill

| File | Purpose |
|---|---|
| `scripts\Invoke-WinUIVmCoverage.ps1` | One-command orchestrator for Steps 1-5. Step 3 runs once per `-TestFilter`. The payload and VM are restored in `finally`. |
| `scripts\Step1-New-InstrumentableDll.ps1` | Relinks MUXC with `/DEBUGTYPE:CV,FIXUP` from the recorded link command, outside BuildOutput. |
| `scripts\Step2-Instrument-Payload.ps1` | Static instrumentation (cached) and broadcast into `TestPayload\<flavor>`, with backups. |
| `scripts\Step3-Invoke-VmCoverageRun.ps1` | Starts the collector on the VM, runs the tests through `tools\run-tests-on-vm.ps1 -SkipPayload`, shuts the collector down and downloads the coverage file. |
| `scripts\Step4-Restore-Payload.ps1` | Restores the payload and the VM copies, hash-verified. |
| `scripts\Set-VmCredential.ps1` | Gate 1 helper: validates the VM, takes the password in a separate window, writes the encrypted credential cache, and checks the logon and the active desktop. |
| `scripts\Step5-Get-ComponentCoverage.ps1` | Merge, per-module dedup, component filter, baseline comparison, function coverage, HTML tree and `coverage-summary.md`. |
| `scripts\lib\Set-CoveragePipeAcl.ps1` | Unmodified copy from lift PR 16435603 (`Helix\common\pipeline\coverage\`). |
| `scripts\lib\Split-CoverageByModule.ps1`, `Repair-CoverageDuplicates.ps1`, `Build-CoverageTreeReport.ps1`, `Get-FunctionCoverage.ps1`, `Build-FunctionCoverageTreeReport.ps1` | Unmodified copies from `WinUITestInventory\SkillToGetCodeCoverageFromPipeline\scripts`. |

