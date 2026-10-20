<#
.SYNOPSIS
    One command: code coverage of a WinUI component from ONLY the tests you choose, run on a local
    Hyper-V VM.

.DESCRIPTION
    relink (instrumentable copy) -> instrument + broadcast into TestPayload -> collector + tests on VM
    -> download .coverage -> restore payload/VM -> component report (optionally vs. a baseline).
    * The payload and the VM are restored in a finally block even if the run fails. If a previous
      run was killed before it could restore, this script restores that first.
    * A restore failure fails the command (after the report is written, if coverage was collected).
    * Only the generated TestPayload\<flavor> folder and the VM's deployed payload are modified;
      BuildOutput and repo sources are never touched.

.EXAMPLE
    # TitleBar coverage from the TitleBar tests only (output: BuildOutput\Coverage\amd64chk\TitleBar)
    .\Invoke-WinUIVmCoverage.ps1 -RepoRoot C:\src\microsoft-ui-xaml -VMName MyTestVM `
        -TestFilter '*.TitleBarTests.*' -ComponentName TitleBar `
        -ComponentPathRegex '\\controls\\dev\\TitleBar\\|\\Generated\\TitleBar|\\generated\\Controls\\TitleBar' `
        -MoveSessionToConsole

.EXAMPLE
    # TableView lives in Microsoft.UI.Xaml.Controls.Tabular.dll
    .\Invoke-WinUIVmCoverage.ps1 -RepoRoot C:\src\microsoft-ui-xaml -VMName MyTestVM `
        -TestFilter '*.TableViewSortingTests.*' -ComponentName TableView -Module Microsoft.UI.Xaml.Controls.Tabular.dll `
        -ComponentPathRegex '\\controls\\dev\\TableView\\'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepoRoot,
    [Parameter(Mandatory)][string]$VMName,
    # One or more runtests filters; each runs with its own collector session, then all are merged.
    [Parameter(Mandatory)][string[]]$TestFilter,
    # Work + report folder. Default: <RepoRoot>\BuildOutput\Coverage\<Flavor>\<ComponentName> (git-ignored).
    [string]$OutputDir = '',
    [string]$ComponentName = 'TitleBar',
    [string]$ComponentPathRegex = '\\controls\\dev\\TitleBar\\|\\Generated\\TitleBar|\\generated\\Controls\\TitleBar',
    [ValidateSet('Microsoft.UI.Xaml.Controls.dll', 'Microsoft.UI.Xaml.Controls.Tabular.dll', 'Microsoft.ui.xaml.dll')]
    [string]$Module = 'Microsoft.UI.Xaml.Controls.dll',
    [string]$Flavor = 'amd64chk',
    [string]$BaselinePerFileCsv = '',
    [string]$BaselineLabel = 'Baseline',
    # Root of the checkout that produced -BaselinePerFileCsv, if it is not a build-agent root.
    [string]$BaselineRepoRoot = '',
    [PSCredential]$Credential,
    [switch]$MoveSessionToConsole,
    [switch]$RefreshPayload,
    [switch]$ForceReinstrument,
    # Skip the pre-check that lists the matching tests in the payload before instrumenting.
    [switch]$SkipTestCheck,
    [switch]$SkipFunctionCoverage
)
$ErrorActionPreference = 'Stop'
$scripts = $PSScriptRoot
$RepoRoot = (Resolve-Path $RepoRoot).Path
if (-not $OutputDir) { $OutputDir = Join-Path $RepoRoot "BuildOutput\Coverage\$Flavor\$ComponentName" }
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path
$work = Join-Path $OutputDir 'work'
$plat = if ($Flavor -match '^(amd64|x86|arm64)') { @{ amd64 = 'x64'; x86 = 'x86'; arm64 = 'arm64' }[$Matches[1]] } else { 'x64' }
$cfg = if ($Flavor -match 'fre$') { 'fre' } else { 'chk' }
$payloadFlavor = "$plat$cfg"
$payloadDir = Join-Path $RepoRoot "TestPayload\$payloadFlavor"
# Native link project of each module that needs the Step 1 relink (MUX already links with CV/FIXUP).
$relinkProject = @{ 'Microsoft.UI.Xaml.Controls.dll' = 'controls\dev\dll'; 'Microsoft.UI.Xaml.Controls.Tabular.dll' = 'controls\dev\dll-tabular' }
$runLabel = Get-Date -Format 'yyyyMMdd-HHmmss'
$transcript = Join-Path $OutputDir "coverage-run-$runLabel.log"
Start-Transcript $transcript | Out-Null
Write-Host "== Output: $OutputDir"
# Outer try/finally: the transcript is stopped whatever happens.
try {
    $sw = [Diagnostics.Stopwatch]::StartNew()

    # A previous run that was killed before Step 4 leaves instrument-state.json behind: restore first.
    if (Test-Path (Join-Path $work 'instrument-state.json')) {
        Write-Host "== A previous run did not finish restoring the payload; restoring it first"
        & (Join-Path $scripts 'Step4-Restore-Payload.ps1') -RepoRoot $RepoRoot -WorkDir $work -VMName $VMName -Credential $Credential
    }

    $runs = @()
    $runError = $null
    $restoreError = $null
    try {
        if ($RefreshPayload) {
            Write-Host "== Refreshing TestPayload\$payloadFlavor from BuildOutput"
            Push-Location $RepoRoot
            try {
                & .\initrun.ps1 -Flavor $Flavor .\test\CreateTestPayload.ps1 -Platform $plat -Configuration $cfg -Mode DevTestSuite -SkipSymbols -Quiet
                $code = $LASTEXITCODE
            } finally { Pop-Location }
            if ($code -ne 0) {
                throw "CreateTestPayload failed (exit code $code); see $payloadDir\CreateTestPayload.log. Fix the build or payload, then retry."
            }
        }
        $missing = @('runtests.cmd', 'te.exe' | Where-Object { -not (Test-Path (Join-Path $payloadDir $_)) })
        if (-not (Test-Path $payloadDir) -or $missing) {
            throw "Test payload $payloadDir is missing or incomplete ($($missing -join ', ')). Build the repo, then rerun with -RefreshPayload."
        }
        if (-not (Get-ChildItem $payloadDir -Recurse -Filter $Module -File | Select-Object -First 1)) {
            throw "No copy of $Module in $payloadDir. Build the repo, then rerun with -RefreshPayload."
        }

        # Fail fast, before the long instrumentation, if the payload that will be deployed has no
        # tests matching a filter (for example because the build predates the component's tests).
        # Mirrors the test files and Desktop selection that runtests.ps1 uses on the VM.
        if (-not $SkipTestCheck) {
            Push-Location $payloadDir
            # TAEF needs /select:"<query>" passed verbatim; PowerShell 7's default quoting turns the
            # query into a file name (TAEF then lists every test).
            $savedArgPassing = $PSNativeCommandArgumentPassing
            $PSNativeCommandArgumentPassing = 'Legacy'
            try {
                $testFiles = @(Get-ChildItem Test -Filter 'Microsoft.UI.Xaml.Tests.*.dll' -EA SilentlyContinue | ForEach-Object { "Test\$($_.Name)" }) +
                    @('Test\MUXControls.Test.dll', 'Test\UnpackagedApps\MUXControlsTestApp\MUXControlsTestApp.dll', 'Test\IXMPTestApp.appx' | Where-Object { Test-Path $_ })
                foreach ($f in $TestFilter) {
                    $select = "@Name='*$f' and (@TestPass:IncludeOnlyOn='*Desktop*' OR (NOT(@TestPass:IncludeOnlyOn=*) AND NOT(@TestPass:ExcludeOn='*Desktop*')))"
                    $nameRx = '^' + ([regex]::Escape("*$f") -replace '\\\*', '.*') + '$'
                    $listing = @(& .\te.exe @testFiles /list "/select:`"$select`"" 2>&1 | ForEach-Object { "$_".Trim() })
                    if ($LASTEXITCODE -ne 0) {
                        throw "Listing tests with te.exe failed (exit code $LASTEXITCODE) for '$f':`n$(($listing | Where-Object { $_ } | Select-Object -Last 10) -join "`n")"
                    }
                    $count = @($listing | Where-Object { $_ -match $nameRx }).Count
                    Write-Host "== Test check: '$f' matches $count test(s) in TestPayload\$payloadFlavor"
                    if (-not $count) {
                        throw "No tests in TestPayload\$payloadFlavor match '$f'. Check the filter, and that the build is current and contains the component's tests (rebuild, then use -RefreshPayload). Use -SkipTestCheck to bypass this check."
                    }
                }
            }
            finally { $PSNativeCommandArgumentPassing = $savedArgPassing; Pop-Location }
        }

        Write-Host "== Step 1: instrumentable $Module"
        if ($relinkProject.ContainsKey($Module)) {
            $rl = & (Join-Path $scripts 'Step1-New-InstrumentableDll.ps1') -RepoRoot $RepoRoot -OutputDir (Join-Path $work 'relink') -Flavor $Flavor `
                -ObjSubDir $relinkProject[$Module] -ProjectSubDir $relinkProject[$Module] -ModuleName $Module
            $srcDll = $rl.dll
        } else {
            # MUX already links with /debugtype:cv,fixup,pdata; stage its DLL+PDB side by side.
            $d = Join-Path $work 'mux'; New-Item -ItemType Directory -Force $d | Out-Null
            Copy-Item (Join-Path $RepoRoot "BuildOutput\bin\$Flavor\Product\Microsoft.ui.xaml.dll") $d -Force
            Copy-Item (Get-ChildItem (Join-Path $RepoRoot "BuildOutput\bin\$Flavor") -Recurse -Filter Microsoft.ui.xaml.pdb | Select-Object -First 1).FullName $d -Force
            $srcDll = Join-Path $d 'Microsoft.ui.xaml.dll'
        }

        Write-Host "== Step 2: instrument + broadcast into payload"
        $st = & (Join-Path $scripts 'Step2-Instrument-Payload.ps1') -RepoRoot $RepoRoot -WorkDir $work -SourceDll $srcDll -PayloadFlavor $payloadFlavor -ForceReinstrument:$ForceReinstrument

        Write-Host "== Step 3: collector + tests on $VMName"
        for ($i = 0; $i -lt $TestFilter.Count; $i++) {
            $label = if ($TestFilter.Count -gt 1) { "$runLabel-$($i + 1)" } else { $runLabel }
            Write-Host "   filter $($i + 1)/$($TestFilter.Count): $($TestFilter[$i])"
            $runs += & (Join-Path $scripts 'Step3-Invoke-VmCoverageRun.ps1') -RepoRoot $RepoRoot -WorkDir $work -VMName $VMName -TestFilter $TestFilter[$i] `
                -Flavor $Flavor -Credential $Credential -RunLabel $label -MoveSessionToConsole:$MoveSessionToConsole
        }
    }
    catch { $runError = $_; throw }
    finally {
        Write-Host "== Step 4: restore payload + VM"
        try { & (Join-Path $scripts 'Step4-Restore-Payload.ps1') -RepoRoot $RepoRoot -WorkDir $work -VMName $VMName -Credential $Credential }
        catch {
            $restoreError = $_
            Write-Warning "RESTORE FAILED: $_`n  instrument-state.json was kept. Rerun this script (it restores first) or run:`n  $scripts\Step4-Restore-Payload.ps1 -RepoRoot '$RepoRoot' -WorkDir '$work' -VMName $VMName"
            # The original failure (if any) is rethrown after this block; only note the restore error here.
            if ($runError) { Write-Warning "The run itself had already failed: $runError" }
        }
    }

    Write-Host "== Step 5: $ComponentName report"
    $rep = & (Join-Path $scripts 'Step5-Get-ComponentCoverage.ps1') -CoverageFiles @($runs.coverageFile) -OutputDir (Join-Path $OutputDir "report-$runLabel") -RepoRoot $RepoRoot `
        -Module ([IO.Path]::GetFileNameWithoutExtension($Module)) -ComponentPathRegex $ComponentPathRegex -ComponentName $ComponentName `
        -BaselinePerFileCsv $BaselinePerFileCsv -BaselineLabel $BaselineLabel -BaselineRepoRoot $BaselineRepoRoot `
        -RunLabel "tests '$($TestFilter -join "', '")'" -SkipFunctionCoverage:$SkipFunctionCoverage
    $testSummary = ($runs | ForEach-Object testSummary) -join ' | '
    Write-Host ("== DONE in {0:hh\:mm\:ss}: {1} {2}/{3} lines = {4} %  | tests: {5}" -f $sw.Elapsed, $ComponentName, $rep.coveredLines, $rep.totalLines, $rep.lineCoveragePct, $testSummary)
    Write-Host "   summary: $($rep.summary)"
    if ($restoreError) {
        throw "Coverage report written ($($rep.summary)), but restoring the payload/VM FAILED: $restoreError. The payload may still be instrumented; rerun this script (it restores first) or run Step4-Restore-Payload.ps1 -WorkDir '$work'."
    }
    $rep | Add-Member testSummary $testSummary -PassThru | Add-Member coverageFile @($runs.coverageFile) -PassThru
}
finally { Stop-Transcript | Out-Null }
