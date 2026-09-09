[CmdletBinding()]
param(
    [string]$LatestOsRoot
)

$ErrorActionPreference = "Stop"

$configuration = Get-Content (
    Join-Path $PSScriptRoot "system-build-inputs.json"
) -Raw | ConvertFrom-Json
$folderPaths = Get-Content (
    Join-Path $PSScriptRoot "..\folderpaths.props"
) -Raw

foreach ($property in @(
    "SystemComponentOsRoot",
    "SystemCompositionInternalIncludePath",
    "SystemCompositionPrivateIncludePath",
    "SystemCompositionMetadataPath",
    "SystemDispatcherQueueIncludePath",
    "SystemDispatcherQueueMetadataPath",
    "SystemCoreMessagingLibPath"
))
{
    if (-not $folderPaths.Contains("<$property>", [StringComparison]::Ordinal) -and
        -not $folderPaths.Contains("<$property ", [StringComparison]::Ordinal))
    {
        throw "The build path property '$property' is not wired in eng\folderpaths.props."
    }
}

if (-not $LatestOsRoot)
{
    $LatestOsRoot = $configuration.latestOsRoot
}

foreach ($artifact in $configuration.artifacts)
{
    $path = Join-Path $LatestOsRoot $artifact.relativePath
    if (-not (Test-Path $path -PathType Leaf))
    {
        throw "Required system build input '$($artifact.id)' was not found at '$path'."
    }

    $actualHash = (Get-FileHash $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -ne $artifact.sha256)
    {
        throw "Hash mismatch for system build input '$($artifact.id)' at '$path'."
    }
}

Write-Output "System build input tests passed."
