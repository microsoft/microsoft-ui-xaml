<#
.SYNOPSIS
    One command: code coverage of a WinUI component from ONLY the tests you choose, run on a local
    Hyper-V VM.

.DESCRIPTION
    relink (instrumentable copy) -> instrument + broadcast into TestPayload -> collector + tests on VM
    -> download .coverage -> restore payload/VM -> component report (optionally vs. a CI baseline).
    The payload and the VM are restored in a finally block even if the run fails.

.EXAMPLE
    # TitleBar coverage from the TitleBar tests only, compared to CI build 157731159's numbers
    .\Invoke-WinUIVmCoverage.ps1 -RepoRoot C:\Repos\WinUiGitHubRepo\microsoft-ui-xaml -VMName WinUiTestVM1 `
        -TestFilter '*.TitleBarTests.*' -ComponentName TitleBar `
        -ComponentPathRegex '\\controls\\dev\\TitleBar\\|\\Generated\\TitleBar|\\generated\\Controls\\TitleBar' `
        -OutputDir C:\cov\TitleBar -MoveSessionToConsole `
        -BaselinePerFileCsv C:\...\CodeCoverageOutput_157731159\PerModule\Microsoft.UI.Xaml.Controls\corrected-per-file.csv
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
try {
    $sw = [Diagnostics.Stopwatch]::StartNew()
    if ($RefreshPayload) {
        Write-Host "== Refreshing TestPayload\$payloadFlavor from BuildOutput"
        Push-Location $RepoRoot
        try { & .\initrun.ps1 -Flavor $Flavor .\test\CreateTestPayload.ps1 -Platform $plat -Configuration $cfg -Mode DevTestSuite -SkipSymbols -Quiet }
        finally { Pop-Location }
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
try {
    Write-Host "== Step 5: $ComponentName report"
    $rep = & (Join-Path $scripts 'Step5-Get-ComponentCoverage.ps1') -CoverageFiles @($runs.coverageFile) -OutputDir (Join-Path $OutputDir "report-$runLabel") `
        -Module ([IO.Path]::GetFileNameWithoutExtension($Module)) -ComponentPathRegex $ComponentPathRegex -ComponentName $ComponentName `
        -BaselinePerFileCsv $BaselinePerFileCsv -BaselineLabel $BaselineLabel -RunLabel "tests '$($TestFilter -join "', '")'" -SkipFunctionCoverage:$SkipFunctionCoverage
    $testSummary = ($runs | ForEach-Object testSummary) -join ' | '
    Write-Host ("== DONE in {0:hh\:mm\:ss}: {1} {2}/{3} lines = {4} %  | tests: {5}" -f $sw.Elapsed, $ComponentName, $rep.coveredLines, $rep.totalLines, $rep.lineCoveragePct, $testSummary)
    Write-Host "   summary: $($rep.summary)"
    $rep | Add-Member testSummary $testSummary -PassThru | Add-Member coverageFile @($runs.coverageFile) -PassThru
}
finally { Stop-Transcript | Out-Null }
