<#
.SYNOPSIS
    One command: code coverage of a WinUI component from ONLY the tests you choose, run on a local
    Hyper-V VM.

.DESCRIPTION
    relink (instrumentable copy) -> instrument + broadcast into TestPayload -> collector + tests on VM
    -> download .coverage -> restore payload/VM -> component report (optionally vs. a baseline).
    The payload and the VM are restored in a finally block even if the run fails.

.EXAMPLE
    # TitleBar coverage from the TitleBar tests only, compared to a baseline from another coverage run
    .\Invoke-WinUIVmCoverage.ps1 -RepoRoot C:\src\microsoft-ui-xaml -VMName MyTestVM `
        -TestFilter '*.TitleBarTests.*' -ComponentName TitleBar `
        -ComponentPathRegex '\\controls\\dev\\TitleBar\\|\\Generated\\TitleBar|\\generated\\Controls\\TitleBar' `
        -OutputDir C:\src\microsoft-ui-xaml\CodeCoverageOutput\TitleBar -MoveSessionToConsole `
        -BaselinePerFileCsv C:\cov\baseline\PerModule\Microsoft.UI.Xaml.Controls\corrected-per-file.csv
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepoRoot,
    [Parameter(Mandatory)][string]$VMName,
    # One or more runtests filters; each runs with its own collector session, then all are merged.
    [Parameter(Mandatory)][string[]]$TestFilter,
    [Parameter(Mandatory)][string]$OutputDir,
    [string]$ComponentName = 'TitleBar',
    [string]$ComponentPathRegex = '\\controls\\dev\\TitleBar\\|\\Generated\\TitleBar|\\generated\\Controls\\TitleBar',
    [ValidateSet('Microsoft.UI.Xaml.Controls.dll', 'Microsoft.ui.xaml.dll')][string]$Module = 'Microsoft.UI.Xaml.Controls.dll',
    [string]$Flavor = 'amd64chk',
    [string]$BaselinePerFileCsv = '',
    [string]$BaselineLabel = 'Baseline',
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
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path
$work = Join-Path $OutputDir 'work'
$plat = if ($Flavor -match '^(amd64|x86|arm64)') { @{ amd64 = 'x64'; x86 = 'x86'; arm64 = 'arm64' }[$Matches[1]] } else { 'x64' }
$cfg = if ($Flavor -match 'fre$') { 'fre' } else { 'chk' }
$payloadFlavor = "$plat$cfg"
$runLabel = Get-Date -Format 'yyyyMMdd-HHmmss'
$transcript = Join-Path $OutputDir "coverage-run-$runLabel.log"
Start-Transcript $transcript | Out-Null
# Outer try/finally: the transcript is stopped even when Steps 1-3 fail (Step 4 still runs first).
try {
    try {
        $sw = [Diagnostics.Stopwatch]::StartNew()
        if ($RefreshPayload) {
            Write-Host "== Refreshing TestPayload\$payloadFlavor from BuildOutput"
            Push-Location $RepoRoot
            try { & .\initrun.ps1 -Flavor $Flavor .\test\CreateTestPayload.ps1 -Platform $plat -Configuration $cfg -Mode DevTestSuite -SkipSymbols -Quiet }
            finally { Pop-Location }
        }

        # Fail fast, before the long instrumentation, if the payload that will be deployed has no
        # tests matching a filter (for example because the build predates the component's tests).
        # Uses the same selection as runtests.ps1 on the VM.
        $payloadDir = Join-Path $RepoRoot "TestPayload\$payloadFlavor"
        if (-not $SkipTestCheck -and (Test-Path (Join-Path $payloadDir 'te.exe'))) {
            Push-Location $payloadDir
            # TAEF needs /select:"<query>" passed verbatim; PowerShell 7's default quoting turns the
            # query into a file name (TAEF then lists every test).
            $savedArgPassing = $PSNativeCommandArgumentPassing
            $PSNativeCommandArgumentPassing = 'Legacy'
            try {
                $testDlls = @(Get-ChildItem Test -Filter 'Microsoft.UI.Xaml.Tests.*.dll' -EA SilentlyContinue | ForEach-Object { "Test\$($_.Name)" }) +
                    @('Test\MUXControls.Test.dll', 'Test\UnpackagedApps\MUXControlsTestApp\MUXControlsTestApp.dll' | Where-Object { Test-Path $_ })
                foreach ($f in $TestFilter) {
                    $select = "@Name='*$f' and (@TestPass:IncludeOnlyOn='*Desktop*' OR (NOT(@TestPass:IncludeOnlyOn=*) AND NOT(@TestPass:ExcludeOn='*Desktop*')))"
                    $nameRx = '^' + ([regex]::Escape("*$f") -replace '\\\*', '.*') + '$'
                    $count = @(& .\te.exe @testDlls /list "/select:`"$select`"" 2>&1 | ForEach-Object { "$_".Trim() } | Where-Object { $_ -match $nameRx }).Count
                    Write-Host "== Test check: '$f' matches $count test(s) in TestPayload\$payloadFlavor"
                    if (-not $count) {
                        throw "No tests in TestPayload\$payloadFlavor match '$f'. Check the filter, and that the build is current and contains the component's tests (rebuild, then use -RefreshPayload). Use -SkipTestCheck to bypass this check."
                    }
                }
            }
            finally { $PSNativeCommandArgumentPassing = $savedArgPassing; Pop-Location }
        }

        Write-Host "== Step 1: instrumentable $Module"
        if ($Module -eq 'Microsoft.UI.Xaml.Controls.dll') {
            $rl = & (Join-Path $scripts 'Step1-New-InstrumentableDll.ps1') -RepoRoot $RepoRoot -OutputDir (Join-Path $work 'relink') -Flavor $Flavor
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
        $runs = @()
        for ($i = 0; $i -lt $TestFilter.Count; $i++) {
            $label = if ($TestFilter.Count -gt 1) { "$runLabel-$($i + 1)" } else { $runLabel }
            Write-Host "   filter $($i + 1)/$($TestFilter.Count): $($TestFilter[$i])"
            $runs += & (Join-Path $scripts 'Step3-Invoke-VmCoverageRun.ps1') -RepoRoot $RepoRoot -WorkDir $work -VMName $VMName -TestFilter $TestFilter[$i] `
                -Flavor $Flavor -Credential $Credential -RunLabel $label -MoveSessionToConsole:$MoveSessionToConsole
        }
    }
    finally {
        Write-Host "== Step 4: restore payload + VM"
        try { & (Join-Path $scripts 'Step4-Restore-Payload.ps1') -RepoRoot $RepoRoot -WorkDir $work -VMName $VMName -Credential $Credential }
        catch { Write-Warning "RESTORE FAILED - fix manually: $_" }
    }
    Write-Host "== Step 5: $ComponentName report"
    $rep = & (Join-Path $scripts 'Step5-Get-ComponentCoverage.ps1') -CoverageFiles @($runs.coverageFile) -OutputDir (Join-Path $OutputDir "report-$runLabel") -RepoRoot $RepoRoot `
        -Module ([IO.Path]::GetFileNameWithoutExtension($Module)) -ComponentPathRegex $ComponentPathRegex -ComponentName $ComponentName `
        -BaselinePerFileCsv $BaselinePerFileCsv -BaselineLabel $BaselineLabel -RunLabel "tests '$($TestFilter -join "', '")'" -SkipFunctionCoverage:$SkipFunctionCoverage
    $testSummary = ($runs | ForEach-Object testSummary) -join ' | '
    Write-Host ("== DONE in {0:hh\:mm\:ss}: {1} {2}/{3} lines = {4} %  | tests: {5}" -f $sw.Elapsed, $ComponentName, $rep.coveredLines, $rep.totalLines, $rep.lineCoveragePct, $testSummary)
    Write-Host "   summary: $($rep.summary)"
    $rep | Add-Member testSummary $testSummary -PassThru | Add-Member coverageFile @($runs.coverageFile) -PassThru
}
finally { Stop-Transcript | Out-Null }
