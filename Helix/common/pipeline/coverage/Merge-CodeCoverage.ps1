# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Merge-CodeCoverage.ps1 - Merge every per-slice, per-module .coverage file into corrected
# per-module and combined coverage reports.
#
# Runs in the MergeCodeCoverage job after every RunTests slice has published its
# coverage-<machine>-slice<n>-<ModuleStem>.coverage files. Steps:
#   1. One `merge` of all inputs -> raw Cobertura (one <package> per DLL) + merged.coverage (VS).
#   2. Repair-CodeCoverage.ps1 -> dedup by (file, line), per DLL and combined, and
#      published.cobertura.xml for the Code Coverage tab (one top-level folder per DLL). The raw
#      Cobertura's own totals are inflated by a duplicate-line defect in the merge engine and are
#      never reported.
#   3. New-CoverageHtmlReport.ps1 -> coverage-report.html with a Combined/MUX/MUXC drop-down.
#
# EXPERIMENTAL QUALITY.

[CmdletBinding()]
param(
    # Root to search (recursively) for *.coverage files - usually the downloaded artifacts dir.
    [Parameter(Mandatory = $true)]
    [string]$InputDir,

    # Where to write the reports (published as the MergeCodeCoverage artifact).
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Explicit tool path; auto-resolved (dotnet-coverage preferred here - merge is portable).
    [string]$CoverageToolPath = "",

    # Scratch folder for the raw (large, uncorrected) merged Cobertura. Kept out of $OutputDir so
    # it is not published.
    [string]$WorkDir = "",

    # Shape of published.cobertura.xml (see Repair-CodeCoverage.ps1).
    [ValidateSet('PerModuleFolders', 'Combined')]
    [string]$PublishLayout = 'PerModuleFolders'
)

$ErrorActionPreference = "Stop"

function Resolve-CoverageTool {
    param([string]$Explicit)
    if ($Explicit -and (Test-Path $Explicit)) { return $Explicit }
    $dc = Get-Command "dotnet-coverage" -ErrorAction SilentlyContinue
    if ($dc) { return $dc.Source }
    $vs = Get-ChildItem -Path @(
        "${env:ProgramFiles}\Microsoft Visual Studio",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
    ) -Recurse -Filter "Microsoft.CodeCoverage.Console.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($vs) { return $vs.FullName }
    Write-Host "dotnet-coverage not found; installing the global tool..."
    & dotnet tool install --global dotnet-coverage --add-source https://api.nuget.org/v3/index.json --ignore-failed-sources | Write-Host
    $env:PATH = "$env:PATH;$env:USERPROFILE\.dotnet\tools"
    $dc = Get-Command "dotnet-coverage" -ErrorAction SilentlyContinue
    if ($dc) { return $dc.Source }
    throw "Could not find or install a code coverage tool."
}

$files = @(Get-ChildItem -Path $InputDir -Recurse -Filter "*.coverage" -ErrorAction SilentlyContinue |
           Where-Object { $_.Length -gt 0 } | ForEach-Object { $_.FullName })

Write-Host "Found $($files.Count) coverage file(s) under $InputDir."
if ($files.Count -eq 0) {
    Write-Host "##vso[task.logissue type=warning]No .coverage files found to merge."
    Write-Host "Nothing to merge; exiting without a report."
    exit 0
}
$files | ForEach-Object { Write-Host "  $_" }

$covTool = Resolve-CoverageTool -Explicit $CoverageToolPath
Write-Host "Coverage tool: $covTool"

if (-not $WorkDir) {
    $tempRoot = if ($env:AGENT_TEMPDIRECTORY) { $env:AGENT_TEMPDIRECTORY } else { $env:TEMP }
    $WorkDir = Join-Path $tempRoot "CodeCoverageMerge"
}
New-Item -ItemType Directory $OutputDir -Force | Out-Null
New-Item -ItemType Directory $WorkDir -Force | Out-Null
$rawCobertura = Join-Path $WorkDir "merged.raw.cobertura.xml"
$coverageOut  = Join-Path $OutputDir "merged.coverage"

Write-Host "`nMerging to raw Cobertura: $rawCobertura"
& $covTool merge @files -o $rawCobertura -f cobertura | Write-Host
if ($LASTEXITCODE -ne 0) { throw "Cobertura merge failed (exit $LASTEXITCODE)." }

Write-Host "Merging to binary .coverage (open in Visual Studio): $coverageOut"
& $covTool merge @files -o $coverageOut -f coverage | Write-Host
if ($LASTEXITCODE -ne 0) { throw "Binary merge failed (exit $LASTEXITCODE)." }

Write-Host ""
& (Join-Path $PSScriptRoot "Repair-CodeCoverage.ps1") -InputCobertura $rawCobertura -OutputDir $OutputDir -PublishLayout $PublishLayout

# Piped through ForEach-Object so Windows PowerShell 5.1 still yields one item per view.
$views = @((Get-Content (Join-Path $OutputDir 'coverage-summary.json') -Raw | ConvertFrom-Json) | ForEach-Object { $_ })
foreach ($v in $views) {
    if ([long]$v.TotalLines -le 0) {
        # An instrumented-but-never-hit module still has real line counts; zero lines means the
        # module was never successfully instrumented.
        Write-Host "##vso[task.logissue type=warning]Coverage for '$($v.Label)' contains no lines at all - it was almost certainly never successfully instrumented. Check the instrumentation step in CreateTestPayload."
    }
}

$subtitle = if ($env:BUILD_BUILDNUMBER) { "Build $($env:BUILD_BUILDNUMBER) ($($env:BUILD_BUILDID)) - $($env:BUILD_SOURCEBRANCHNAME)" } else { "" }
Write-Host ""
& (Join-Path $PSScriptRoot "New-CoverageHtmlReport.ps1") -ReportDir $OutputDir -Subtitle $subtitle

# (No ##vso[task.uploadsummary] build-summary table: OneBranch blocks that logging command in
# this job - "not allowed in this step due to policy restrictions".)
Write-Host "`nReports in $($OutputDir):"
Write-Host "  published.cobertura.xml  (Code Coverage tab: one top-level folder per DLL)"
Write-Host "  coverage-report.html     (drop-down: Combined / each DLL)"
Write-Host "  coverage-summary.csv     (one row per view)"
Write-Host "  <Module>\, Combined\     (per-file CSVs, summaries, per-DLL Cobertura with real paths)"
Write-Host "  merged.coverage          (binary, for Visual Studio)"
