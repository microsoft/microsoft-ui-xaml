# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDir,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    [string]$CoverageToolPath
)

$ErrorActionPreference = 'Stop'
$files = @(Get-ChildItem -LiteralPath $InputDir -Recurse -File -Filter 'coverage-*.coverage')
if ($files.Count -eq 0 -or @($files | Where-Object Length -eq 0).Count -gt 0)
{
    throw 'Coverage input is missing or empty. Check the per-slice collector logs.'
}
$tool = & "$PSScriptRoot\Get-CoverageTool.ps1" -CoverageToolPath $CoverageToolPath
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

foreach ($format in @('cobertura', 'coverage'))
{
    $name = if ($format -eq 'cobertura') { 'merged.cobertura.xml' } else { 'merged.coverage' }
    $output = Join-Path $OutputDir $name
    if (Test-Path -LiteralPath $output)
    {
        Remove-Item -LiteralPath $output
    }
    & $tool merge @($files.FullName) --output $output --output-format $format | Out-Host
    if ($LASTEXITCODE -ne 0)
    {
        throw "Coverage merge failed for $format (exit $LASTEXITCODE)."
    }
    if (-not (Test-Path -LiteralPath $output) -or (Get-Item -LiteralPath $output).Length -eq 0)
    {
        throw "Coverage merge did not produce '$output'."
    }
}
Write-Host "Merged $($files.Count) coverage files into '$OutputDir'."
