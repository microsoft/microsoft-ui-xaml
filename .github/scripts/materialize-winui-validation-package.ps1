param(
    [string]$PackageStore,
    [string]$PackagesDirectory
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

if (-not $PackageStore) {
    $PackageStore = Join-Path $repoRoot 'PackageStore'
}
if (-not $PackagesDirectory) {
    $PackagesDirectory = Join-Path $repoRoot 'packages'
}

$packageId = 'Microsoft.WindowsAppSDK.WinUI'
$available = @(Get-ChildItem -LiteralPath $PackageStore -Filter "$packageId.*.nupkg" -File -ErrorAction Stop)
if ($available.Count -ne 1) {
    throw "Expected exactly one $packageId validation package in '$PackageStore', found $($available.Count): $($available.Name -join ', ')"
}

$nupkg = $available[0]
$version = $nupkg.BaseName.Substring($packageId.Length + 1)
if ([string]::IsNullOrWhiteSpace($version)) {
    throw "Unable to determine the WinUI package version from '$($nupkg.Name)'."
}

$destination = Join-Path $PackagesDirectory "$packageId.$version"
$requiredProps = Join-Path $destination 'build\native\Microsoft.WindowsAppSDK.WinUI.props'
$requiredTargets = Join-Path $destination 'build\native\Microsoft.WindowsAppSDK.WinUI.targets'

if (-not (Test-Path -LiteralPath $requiredProps) -or -not (Test-Path -LiteralPath $requiredTargets)) {
    if (Test-Path -LiteralPath $destination) {
        Remove-Item -LiteralPath $destination -Recurse -Force
    }

    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory($nupkg.FullName, $destination)
}

if (-not (Test-Path -LiteralPath $requiredProps) -or -not (Test-Path -LiteralPath $requiredTargets)) {
    throw "WinUI validation package '$($nupkg.FullName)' did not contain the expected native build imports under '$destination'."
}

Write-Host "Materialized $packageId $version at '$destination'."
