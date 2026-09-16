<#
.SYNOPSIS
  Dependency-free PowerShell test runner for PR performance result comparison.

.DESCRIPTION
  Discovers tests-*.ps1 files, dot-sources each, and invokes every new Test-*
  function. Each test throws on failure; the runner reports pass/fail counts
  and exits non-zero if any test failed.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$testDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$testFiles = @(Get-ChildItem -Path $testDir -Filter 'tests-*.ps1' | Sort-Object Name)

if ($testFiles.Count -eq 0) {
    Write-Host "[FAIL] No tests-*.ps1 files found in $testDir" -ForegroundColor Red
    exit 1
}

$results = @{
    pass = 0
    fail = 0
    failures = @()
}

foreach ($file in $testFiles) {
    Write-Host ""
    Write-Host "=== $($file.Name) ===" -ForegroundColor Cyan

    $beforeFunctions = @((Get-ChildItem function: | Where-Object Name -like 'Test-*').Name)
    try {
        . $file.FullName
    } catch {
        Write-Host "  [FAIL] (load) $($file.Name)" -ForegroundColor Red
        Write-Host "         $($_.Exception.Message)" -ForegroundColor Red
        $results.fail++
        $results.failures += "$($file.Name) (load) - $($_.Exception.Message)"
        continue
    }

    $afterFunctions = @((Get-ChildItem function: | Where-Object Name -like 'Test-*').Name)
    $newFunctions = @($afterFunctions | Where-Object { $beforeFunctions -notcontains $_ })

    foreach ($functionName in $newFunctions) {
        try {
            & $functionName
            Write-Host "  [PASS] $functionName" -ForegroundColor Green
            $results.pass++
        } catch {
            Write-Host "  [FAIL] $functionName" -ForegroundColor Red
            Write-Host "         $($_.Exception.Message)" -ForegroundColor Red
            $results.fail++
            $results.failures += "$functionName - $($_.Exception.Message)"
        }
    }

    foreach ($functionName in $newFunctions) {
        Remove-Item "function:$functionName" -ErrorAction SilentlyContinue
    }
}

Write-Host ""
Write-Host "=========================================="
Write-Host "  pass=$($results.pass) fail=$($results.fail)"
if ($results.fail -gt 0) {
    Write-Host "  FAILED:" -ForegroundColor Red
    foreach ($failure in $results.failures) {
        Write-Host "    - $failure" -ForegroundColor Red
    }
    exit 1
}

Write-Host "  ALL GREEN" -ForegroundColor Green
exit 0
