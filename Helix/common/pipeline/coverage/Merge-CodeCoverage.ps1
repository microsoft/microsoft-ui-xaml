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
foreach ($name in @('merged.cobertura.xml', 'merged.coverage'))
{
    $previousOutput = Join-Path $OutputDir $name
    if (Test-Path -LiteralPath $previousOutput)
    {
        Remove-Item -LiteralPath $previousOutput
    }
}

function Merge-CoverageFiles([string[]]$InputFiles, [string]$Format, [string]$Output)
{
    if (Test-Path -LiteralPath $Output)
    {
        Remove-Item -LiteralPath $Output
    }
    & $tool merge @InputFiles --output $Output --output-format $Format | Out-Host
    if ($LASTEXITCODE -ne 0)
    {
        throw "Coverage merge failed for $Format (exit $LASTEXITCODE)."
    }
    if (-not (Test-Path -LiteralPath $Output) -or (Get-Item -LiteralPath $Output).Length -eq 0)
    {
        throw "Coverage merge did not produce '$Output'."
    }
}

# The VS tool silently skips invalid files and can return a nonempty, zero-module
# report even for corrupt-only input. Validate each slice has source lines before
# combining it with valid slices, which would otherwise conceal the missing data.
$validationDir = Join-Path $OutputDir ('.validation-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $validationDir | Out-Null
try
{
    $validationFile = Join-Path $validationDir 'slice.cobertura.xml'
    $readerSettings = [Xml.XmlReaderSettings]::new()
    $readerSettings.DtdProcessing = [Xml.DtdProcessing]::Ignore
    $readerSettings.XmlResolver = $null
    foreach ($file in $files)
    {
        Write-Host "Validating coverage slice '$($file.FullName)'."
        Merge-CoverageFiles -InputFiles @($file.FullName) -Format cobertura -Output $validationFile
        $reader = [Xml.XmlReader]::Create($validationFile, $readerSettings)
        try
        {
            [void]$reader.MoveToContent()
            if ($reader.LocalName -ne 'coverage' -or [long]$reader.GetAttribute('lines-valid') -le 0)
            {
                throw "Coverage slice '$($file.FullName)' is invalid or contains no source-line data."
            }
        }
        finally
        {
            $reader.Dispose()
        }
    }

    Merge-CoverageFiles -InputFiles $files.FullName -Format cobertura -Output (Join-Path $OutputDir 'merged.cobertura.xml')
    Merge-CoverageFiles -InputFiles $files.FullName -Format coverage -Output (Join-Path $OutputDir 'merged.coverage')
}
finally
{
    Remove-Item -LiteralPath $validationDir -Recurse -Force
}
Write-Host "Merged $($files.Count) coverage files into '$OutputDir'."
