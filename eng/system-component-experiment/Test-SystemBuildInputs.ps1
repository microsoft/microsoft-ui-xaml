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
$productMetadata = Get-Content (
    Join-Path $PSScriptRoot "..\productmetadata.props"
) -Raw
$midlTargets = Get-Content (
    Join-Path $PSScriptRoot "..\midl.targets"
) -Raw
$generatorProject = Get-Content (
    Join-Path $PSScriptRoot "..\gencompheadersandidl\gencompheadersandidl.vcxproj"
) -Raw

foreach ($property in @(
    "SystemComponentOsRoot",
    "SystemCompositionInternalIncludePath",
    "SystemCompositionPrivateIncludePath",
    "SystemCompositionMetadataPath",
    "SystemCompositionIdlPath",
    "SystemDispatcherQueueIncludePath",
    "SystemDispatcherQueueMetadataPath",
    "SystemDispatcherQueueIdlPath",
    "SystemCoreMessagingLibPath",
    "SystemWindowsContractsIdlPath",
    "SystemComponentGeneratedIdlPath"
))
{
    if (-not $folderPaths.Contains("<$property>", [StringComparison]::Ordinal) -and
        -not $folderPaths.Contains("<$property ", [StringComparison]::Ordinal))
    {
        throw "The build path property '$property' is not wired in eng\folderpaths.props."
    }
}

if (($productMetadata | Select-String -Pattern "<IxpWinMDs " -AllMatches).Matches.Count -ne 3 -or
    ($productMetadata | Select-String -Pattern "<Merge>false</Merge>" -AllMatches).Matches.Count -lt 3)
{
    throw "All three retained IXP WinMDs must be reference-only."
}
if (-not $midlTargets.Contains(
    'Name="_CopyReferenceOnlyIxpMetadata"',
    [StringComparison]::Ordinal
))
{
    throw "Reference-only IXP metadata is not copied beside merged XAML outputs."
}
if (-not $generatorProject.Contains(
    "Prepare-SystemComponentIdl.ps1",
    [StringComparison]::Ordinal
))
{
    throw "Generated system IDLs are not normalized for the WinUI SDK toolchain."
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
