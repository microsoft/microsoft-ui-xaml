[CmdletBinding()]
param(
    [string]$OutputPath = (
        Join-Path $PSScriptRoot "..\..\artifacts\system-component-experiment\environment.json"
    )
)

$ErrorActionPreference = "Stop"

function Get-RepositoryState
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path (Join-Path $Path ".git")))
    {
        throw "Repository not found at '$Path'."
    }

    $branch = (& git -C $Path branch --show-current).Trim()
    $commit = (& git -C $Path rev-parse HEAD).Trim()
    $status = @(& git -C $Path status --short)

    if ($LASTEXITCODE -ne 0)
    {
        throw "Failed to read repository state at '$Path'."
    }

    return [ordered]@{
        path = $Path
        branch = $branch
        commit = $commit
        isClean = ($status.Count -eq 0)
        changes = $status
    }
}

$manifestPath = Join-Path $PSScriptRoot "manifest.json"
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
$os = Get-CimInstance Win32_OperatingSystem

$capture = [ordered]@{
    capturedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    machine = [ordered]@{
        osCaption = $os.Caption
        osVersion = $os.Version
        osBuildNumber = $os.BuildNumber
        architecture = $env:PROCESSOR_ARCHITECTURE
    }
    activeSubtrial = $manifest.activeSubtrial
    activeStage = $manifest.activeStage
    repositories = [ordered]@{
        winui = Get-RepositoryState $manifest.repositories.winui.path
        winuiGallery = Get-RepositoryState $manifest.repositories.winuiGallery.path
        latestOs = Get-RepositoryState $manifest.repositories.latestOs.path
        windows10Os = Get-RepositoryState $manifest.repositories.windows10Os.path
    }
    liftedDependencies = $manifest.liftedDependencies
    osCheckpoints = $manifest.osCheckpoints
}

$resolvedOutputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
    $OutputPath
)
$outputDirectory = Split-Path $resolvedOutputPath -Parent
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$capture | ConvertTo-Json -Depth 10 | Set-Content -Path $resolvedOutputPath -Encoding utf8

Write-Output $resolvedOutputPath
