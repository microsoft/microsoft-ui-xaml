# Synthetic discovery-evidence bindings only; no profiler, app or model is launched.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$temp = Join-Path ([IO.Path]::GetTempPath()) ('profiler-supplement-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory $temp)
function Expect-Failure([scriptblock]$Action) {
    $failed = $false
    try { & $Action | Out-Null } catch { $failed = $true }
    if (!$failed) { throw 'Expected supplemental evidence validation to reject this input.' }
}
try {
    $database = Join-Path $temp 'original.sqlite'
    $measurementPath = Join-Path $temp 'measurement.json'
    [IO.File]::WriteAllText($database, 'synthetic original database')
    $measurement = @{ database = @{ path = $database; sha256 = Get-Sha256 $database } }
    Write-JsonFile $measurementPath $measurement
    $directory = Join-Path $temp 'supplement'
    [void](New-Item -ItemType Directory $directory)
    $inputPath = Join-Path $directory 'native-output.json'
    [IO.File]::WriteAllText($inputPath, '{"synthetic":true}')
    $path = Join-Path $directory 'supplement.json'
    $content = @{
        schema = 'ui-profiler/supplemental-evidence/v1'; provider = 'MUX WinUIProfiler'; captures = 0
        originalMeasurement = @{ path = $measurementPath; sha256 = Get-Sha256 $measurementPath }
        originalDatabase = $measurement.database
        inputs = @(@{ path = $inputPath; sha256 = Get-Sha256 $inputPath })
    }
    Write-JsonFile $path $content
    $binding = Get-SupplementalEvidenceBinding $path $measurementPath $measurement
    Assert-SupplementalEvidenceBinding $binding

    foreach ($value in @('another profiler', '')) {
        $content.provider = $value
        Write-JsonFile $path $content
        Expect-Failure { Get-SupplementalEvidenceBinding $path $measurementPath $measurement }
    }
    $content.provider = 'MUX WinUIProfiler'
    foreach ($value in @(1, $false, '0')) {
        $content.captures = $value
        Write-JsonFile $path $content
        Expect-Failure { Get-SupplementalEvidenceBinding $path $measurementPath $measurement }
    }
    $content.captures = 0
    $content.originalMeasurement.sha256 = '0' * 64
    Write-JsonFile $path $content
    Expect-Failure { Get-SupplementalEvidenceBinding $path $measurementPath $measurement }
    $content.originalMeasurement.sha256 = Get-Sha256 $measurementPath
    $content.originalDatabase = @{ path = $database; sha256 = '0' * 64 }
    Write-JsonFile $path $content
    Expect-Failure { Get-SupplementalEvidenceBinding $path $measurementPath $measurement }
    $content.originalDatabase = $measurement.database
    $content.inputs = @(@{ path = $database; sha256 = Get-Sha256 $database })
    Write-JsonFile $path $content
    Expect-Failure { Get-SupplementalEvidenceBinding $path $measurementPath $measurement }
    $content.inputs = @(@{ path = $inputPath; sha256 = Get-Sha256 $inputPath })
    $content.inputs += $content.inputs[0]
    Write-JsonFile $path $content
    Expect-Failure { Get-SupplementalEvidenceBinding $path $measurementPath $measurement }
    $content.inputs = @()
    Write-JsonFile $path $content
    Expect-Failure { Get-SupplementalEvidenceBinding $path $measurementPath $measurement }
    $content.inputs = @(@{ path = $inputPath; sha256 = Get-Sha256 $inputPath })
    Write-JsonFile $path $content
    $binding = Get-SupplementalEvidenceBinding $path $measurementPath $measurement
    [IO.File]::AppendAllText($path, ' ')
    Expect-Failure { Assert-SupplementalEvidenceBinding $binding }
    Write-JsonFile $path $content
    $binding = Get-SupplementalEvidenceBinding $path $measurementPath $measurement
    [IO.File]::AppendAllText($inputPath, ' changed')
    Expect-Failure { Assert-SupplementalEvidenceBinding $binding }
    [IO.File]::WriteAllText($inputPath, '{"synthetic":true}')
    $binding = Get-SupplementalEvidenceBinding $path $measurementPath $measurement
    [IO.File]::AppendAllText($database, ' changed')
    Expect-Failure { Assert-SupplementalEvidenceBinding $binding }
    'Supplemental discovery evidence checks passed: original binding, provider, zero captures, containment, duplicates, emptiness and manifest/input drift.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
