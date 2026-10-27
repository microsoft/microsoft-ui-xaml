# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

param(
    [string]$AppDirectory = $PSScriptRoot,
    [ValidateSet('all', 'added', 'preload', 'recovery', 'path')]
    [string]$Mode = 'all'
)

$ErrorActionPreference = 'Stop'
$executable = Join-Path $AppDirectory 'ActivationSearchTests.exe'
if (-not (Test-Path -LiteralPath $executable)) {
    throw "Build or deploy the activation regression app first: $executable"
}
$modes = if ($Mode -eq 'all') { @('added', 'preload', 'recovery', 'path') } else { @($Mode) }
$results = foreach ($case in $modes) {
    $log = Join-Path $AppDirectory "$case.log"
    $errorLog = Join-Path $AppDirectory "$case.err"
    $process = Start-Process -FilePath $executable -ArgumentList $case -WorkingDirectory $AppDirectory `
        -RedirectStandardOutput $log -RedirectStandardError $errorLog -PassThru
    # Keep a handle so Windows PowerShell can read the exit code after the process exits.
    $null = $process.Handle
    $timedOut = -not $process.WaitForExit(45000)
    if ($timedOut) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }
    $exitCode = $process.ExitCode
    $process.Dispose()
    Get-Content -LiteralPath $log | Write-Host
    Get-Content -LiteralPath $errorLog | Write-Host
    [pscustomobject]@{
        mode = $case
        passed = -not $timedOut -and $exitCode -eq 0
        timedOut = $timedOut
        exitCode = $exitCode
    }
}
$results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $AppDirectory 'results.json')
$results | Format-Table | Out-Host
if ($results.passed -contains $false) {
    throw 'WebView2 activation regression failed. See results.json and the per-process logs.'
}
