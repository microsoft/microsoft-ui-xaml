# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.SYNOPSIS
    Keeps only the latest attempt of each test slice's coverage so a re-run slice is never merged
    twice.

.DESCRIPTION
    Each coverage test slice stages its raw .coverage files under CodeCoverage\Attempt<N>\ inside its
    test-output artifact, <stage>_<job>_<slice>_<status> (see WinUI-RunTestPassOnPipeline-Job.yml).
    When a re-run changes a slice's status, the build holds one artifact per status, so the slice's
    coverage appears more than once. (A re-run that ends with the same status does not republish, so
    the earlier attempt's coverage - a complete run of the same slice - is the one available.)

    This script finds every CodeCoverage\Attempt<N> folder under -DownloadDir, groups them by module
    folder + <stage>_<job>_<slice> (ignoring the status), and copies the .coverage files of the highest
    attempt that produced coverage into -OutputDir\<module folder>\<stage>_<job>_<slice>_Attempt<N>.

.PARAMETER DownloadDir
    Folder the test-output artifacts were downloaded into: <module folder>\<artifact name>\...

.PARAMETER OutputDir
    Folder that receives the selected .coverage files; pass it to Merge-CodeCoverage.ps1 -InputDir.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$DownloadDir,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir
)

$ErrorActionPreference = 'Stop'

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
if (-not (Test-Path $DownloadDir)) {
    Write-Host "##vso[task.logissue type=warning]Coverage download folder '$DownloadDir' does not exist; nothing to select."
    return
}

# <stage>_<job>_<slice position>_<status>, e.g. RunTestsStage_RunTestsRS5_3_Succeeded.
$artifactPattern = '^(?<slice>.+_\d+)_(?<status>[A-Za-z]+)$'
$candidates = @()
foreach ($attemptDir in @(Get-ChildItem -Path $DownloadDir -Directory -Recurse | Where-Object { $_.Name -match '^Attempt\d+$' -and $_.Parent.Name -eq 'CodeCoverage' })) {
    $artifactDir = $attemptDir.Parent
    while ($artifactDir -and $artifactDir.Name -notmatch $artifactPattern) { $artifactDir = $artifactDir.Parent }
    if (-not $artifactDir) {
        Write-Host "##vso[task.logissue type=warning]Could not find the test-output artifact folder above '$($attemptDir.FullName)'; ignoring it."
        continue
    }
    $slice = [regex]::Match($artifactDir.Name, $artifactPattern).Groups['slice'].Value
    $candidates += [pscustomobject]@{
        Key     = "{0}|{1}" -f $artifactDir.Parent.FullName, $slice
        Module  = $artifactDir.Parent.Name
        Slice   = $slice
        Attempt = [int]($attemptDir.Name -replace '^Attempt', '')
        Dir     = $attemptDir
        Files   = @(Get-ChildItem -Path $attemptDir.FullName -File -Filter '*.coverage' | Where-Object { $_.Length -gt 0 })
    }
}
if ($candidates.Count -eq 0) {
    Write-Host "##vso[task.logissue type=warning]No CodeCoverage\Attempt<N> folders found under '$DownloadDir'."
    return
}

$groups = @($candidates | Group-Object Key)
$selectedCount = 0
foreach ($group in $groups) {
    $ordered = @($group.Group | Sort-Object Attempt -Descending)
    $chosen = $ordered | Where-Object { $_.Files.Count -gt 0 } | Select-Object -First 1
    if (-not $chosen) {
        Write-Host "##vso[task.logissue type=warning]No attempt of slice '$($ordered[0].Slice)' ($($ordered[0].Module)) produced coverage."
        continue
    }
    if ($chosen.Attempt -ne $ordered[0].Attempt) {
        Write-Host "##vso[task.logissue type=warning]Attempt $($ordered[0].Attempt) of slice '$($chosen.Slice)' ($($chosen.Module)) has no coverage; using attempt $($chosen.Attempt)."
    }
    $ignored = @($ordered | Where-Object { $_ -ne $chosen } | ForEach-Object { "attempt $($_.Attempt)" })
    $ignoredText = if ($ignored.Count -gt 0) { " (ignoring $($ignored -join ', '))" } else { '' }
    Write-Host "Selected $($chosen.Module) / $($chosen.Slice) attempt $($chosen.Attempt): $($chosen.Files.Count) file(s)$ignoredText"

    $destination = Join-Path $OutputDir (Join-Path $chosen.Module ("{0}_Attempt{1}" -f $chosen.Slice, $chosen.Attempt))
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    foreach ($file in $chosen.Files) {
        Copy-Item -Path $file.FullName -Destination (Join-Path $destination $file.Name) -Force
        $selectedCount++
    }
}

Write-Host "Selected $selectedCount coverage file(s) from $($groups.Count) slice(s)."
