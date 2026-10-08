# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdLetBinding()]
Param(
    [string]$PackageRoot =  $(Resolve-Path "$PSScriptRoot\..\..\BuildOutput\packaging\Release"),
    [string]$OutputDir = $(Resolve-Path "$PSScriptRoot\..\..\PackageStore"),
    [string]$VersionOverride,
    [switch]$NoPackageAnalysis,
    [switch]$UseDependencyOverrides,
    [switch]$InstallPackage,
    [string]$Nuspec = "Microsoft.ProjectReunion.WinUI.TransportPackage.nuspec",
    # When supplied (non-empty), pins ALL upstream inter-component dependency versions
    # baked into the produced nupkg (Foundation/IXP/Base/IxpTransport) to this single
    # coherent value, overriding both the eng\Versions.props pins and
    # the -UseDependencyOverrides path. Used by the monobuild Pack step which passes
    # $(WindowsAppSDKFormattedVersion). Empty default preserves standalone behavior.
    # Mirrors Foundation's BuildAll.ps1 -PackageVersion / -ComponentPackageVersion pattern.
    [string]$MonobuildPinnedVersion = ''
)

#
# Version is read from the VERSION file.
#

$scriptDirectory = $script:MyInvocation.MyCommand.Path | Split-Path -Parent

pushd $scriptDirectory

if (!$OutputDir)
{
    $OutputDir = $scriptDirectory
}

if (!$env:NUGETCMD) {

    cmd /c where /Q nuget.exe
    if ($lastexitcode -ne 0) {
        Write-Host "nuget not found on path. Either add it to path or set NUGETCMD environment variable." -ForegroundColor Red
        Exit 1
    }

    $env:NUGETCMD = "nuget.exe"
}

if ($VersionOverride)
{
    $version = $VersionOverride
}
elseif ($MonobuildPinnedVersion)
{
    # Monobuild: a set MonobuildPinnedVersion implies the coherent monobuild version,
    # so callers need not also pass -VersionOverride.
    $version = $MonobuildPinnedVersion
}
else
{
    $version = "$env:versionFinal"

    if (!$version)
    {
        Write-Error "Expected versionFinal environment variable to have been set"
        Exit 1
    }

    Write-Verbose "Version = $version"
}

# Record versions of dependent packages, for Project Reunion validation
$VersionsPropsPath = Join-Path "$scriptDirectory\..\.." "eng\versions.props"
[xml]$VersionsPropsContent = Get-Content $VersionsPropsPath
$versionPropsFilePropertyGroup = $VersionsPropsContent.Project.PropertyGroup[0]
$IXP_Version = ''
$BASE_COMPONENT_VERSION = ''
$FOUNDATION_COMPONENT_VERSION = ''
$IXP_COMPONENT_VERSION = ''
$CsWinRT_Version = $VersionsPropsContent.SelectSingleNode('//MicrosoftCsWinRTPackageVersion').InnerText
$WEBVIEW2_Version = $VersionsPropsContent.SelectSingleNode('//WebView2PackageVersion').InnerText

# Component versions. versions.props pins wrap a ValueOrDefault; resolve to the pinned env var, else the literal fallback.
function Resolve-VersionPin([string]$raw) {
    if ($env:WindowsAppSDKVersionPinned) { return $env:WindowsAppSDKVersionPinned }
    if ($raw -match ",\s*'([^']*)'") { return $Matches[1] }
    return $raw
}
$IXP_Version                  = Resolve-VersionPin $VersionsPropsContent.SelectSingleNode('//IxpTransportPackageVersion').InnerText
$BASE_COMPONENT_VERSION       = Resolve-VersionPin $VersionsPropsContent.SelectSingleNode('//BasePackageVersion').InnerText
$FOUNDATION_COMPONENT_VERSION = Resolve-VersionPin $VersionsPropsContent.SelectSingleNode('//FoundationPackageVersion').InnerText
$IXP_COMPONENT_VERSION        = Resolve-VersionPin $VersionsPropsContent.SelectSingleNode('//IXPPackageVersion').InnerText

if ($UseDependencyOverrides)
{
    $foundationPkgPath = Join-Path "$scriptDirectory\..\.." "packages\microsoft.windowsappsdk.foundation"
    if (!(Test-Path $foundationPkgPath))
    {
        Write-Error "Asked to use dependency overrides, but microsoft.windowsappsdk.foundation package not installed."
        Exit 1
    }

    # Extract the Foundation version to use from the last-known-good value in Versions.props
    $FOUNDATION_COMPONENT_VERSION = $VersionsPropsContent.SelectSingleNode('//FoundationTransportPackageVersion[@Condition]').InnerText

    # Extract the IXP and Base versions from the dependencies in the Foundation package
    $foundationNuspecPath = "$foundationPkgPath\$FOUNDATION_COMPONENT_VERSION\microsoft.windowsappsdk.foundation.nuspec"
    [xml]$FoundationNuspec = Get-Content $foundationNuspecPath 
    $IXP_COMPONENT_VERSION = ($FoundationNuspec.package.metadata.dependencies.dependency | Where-Object { $_.id -eq "Microsoft.WindowsAppSDK.InteractiveExperiences" }).version
    $BASE_COMPONENT_VERSION = ($FoundationNuspec.package.metadata.dependencies.dependency | Where-Object { $_.id -eq "Microsoft.WindowsAppSDK.Base" }).version

    Write-Host "Version overrides:"
    Write-Host "    FOUNDATION_COMPONENT_VERSION: $FOUNDATION_COMPONENT_VERSION"
    Write-Host "    IXP_COMPONENT_VERSION: $IXP_COMPONENT_VERSION"
    Write-Host "    BASE_COMPONENT_VERSION: $BASE_COMPONENT_VERSION"
}

# Monobuild override: a single coherent version pins ALL upstream inter-component
# dependency versions baked into the produced nupkg. Takes precedence over both the
# eng\Versions.props pins and the -UseDependencyOverrides path above. Empty string
# means not supplied -> the resolved Versions.props values apply.
if ($MonobuildPinnedVersion)
{
    $FOUNDATION_COMPONENT_VERSION = $MonobuildPinnedVersion
    $IXP_COMPONENT_VERSION        = $MonobuildPinnedVersion
    $BASE_COMPONENT_VERSION       = $MonobuildPinnedVersion
    $IXP_Version                  = $MonobuildPinnedVersion
    Write-Host "MonobuildPinnedVersion applied -> all 4 inter-component dependency versions pinned to: $MonobuildPinnedVersion"
}

if ($IXP_Version -eq '' -or $BASE_COMPONENT_VERSION -eq '' -or $FOUNDATION_COMPONENT_VERSION -eq '' -or $IXP_COMPONENT_VERSION -eq '')
{
    Write-Error "One of the following is empty, but expected not to be: IXP_Version, BASE_COMPONENT_VERSION, FOUNDATION_COMPONENT_VERSION, IXP_COMPONENT_VERSION"
    Exit 1
}

if (!(Test-Path $OutputDir)) { mkdir $OutputDir }

# Pass NoWarn=NU5100 to silence warnings for all the native binaries being put
# into the "runtime-framework\" folder rather than "lib\" or "runtime\".
$CommonNugetArgs = "-properties `"PackageRoot=$PackageRoot``;Version=$version``;IXP_Version=$IXP_Version``;CsWinRT_Version=$CsWinRT_Version``;WEBVIEW2_Version=$WEBVIEW2_Version``;BASE_COMPONENT_VERSION=$BASE_COMPONENT_VERSION``;FOUNDATION_COMPONENT_VERSION=$FOUNDATION_COMPONENT_VERSION``;IXP_COMPONENT_VERSION=$IXP_COMPONENT_VERSION;NoWarn=NU5100`""

$NugetArgs = "$CommonNugetArgs -OutputDirectory $(Resolve-Path $OutputDir)"

if ($NoPackageAnalysis)
{
    $NugetArgs = "$NugetArgs -NoPackageAnalysis"
}

$NugetCmdLine = "$env:NUGETCMD pack $Nuspec $NugetArgs"
Write-Host $NugetCmdLine
Invoke-Expression $NugetCmdLine

if ($lastexitcode -ne 0)
{
    Write-Host "Nuget returned $lastexitcode"
    Exit $lastexitcode;
}


if ($InstallPackage)
{
    $PackageCache = [System.IO.Path]::GetFullPath((Join-Path "$scriptDirectory\..\.." "packages"))
    $NugetConfigPath = Join-Path "$scriptDirectory\..\.." "NuGet.config"
    $PackageId = "Microsoft.WindowsAppSDK.WinUI"

    function Remove-CachedPackage([string]$CacheRoot)
    {
        $PackagePaths = @(
            (Join-Path $CacheRoot "$PackageId\$version"),
            (Join-Path $CacheRoot "$PackageId.$version")
        )

        foreach ($PackagePath in $PackagePaths)
        {
            if (Test-Path $PackagePath)
            {
                # Delete the `.nupkg.metadata` first in case of error when subsequently
                # deleting the rest of the directory; this will prevent Nuget from
                # erroneously believing that the package is still in a successfully
                # extracted state.
                $MetadataPath = Join-Path $PackagePath ".nupkg.metadata"
                if (Test-Path $MetadataPath)
                {
                    Remove-Item $MetadataPath -Force -ErrorAction Stop
                }

                Write-Host "Removing stale package: $PackagePath" -ForegroundColor Yellow
                Remove-Item $PackagePath -Recurse -Force -ErrorAction Stop
            }
        }
    }

    $PackageCaches = @($PackageCache)
    if ($env:NUGET_PACKAGES)
    {
        if (![System.IO.Path]::IsPathRooted($env:NUGET_PACKAGES))
        {
            # `nuget.exe` will raise a clear error message if `NUGET_PACKAGES` is not 
            # a rooted path. We will simply ignore it as a potential package cache to be 
            # cleared as we are unable to resolve it ourselves and otherwise let 
            # `nuget.exe`'s own validation logic handle this scenario.
            Write-Warning "Skipping cleanup for relative NUGET_PACKAGES path: $env:NUGET_PACKAGES"
        }
        else
        {
            $PackageCaches += [System.IO.Path]::GetFullPath($env:NUGET_PACKAGES)
        }
    }

    $PackageCaches = $PackageCaches | Sort-Object -Unique

    foreach ($CacheRoot in $PackageCaches)
    {
        Remove-CachedPackage $CacheRoot
    }

    Write-Host "nuget install $PackageId -Version $version -OutputDirectory $PackageCache -ConfigFile $NugetConfigPath"
    nuget install $PackageId -Version $version -OutputDirectory $PackageCache -ConfigFile $NugetConfigPath

    if ($lastexitcode -ne 0)
    {
        Write-Host "Nuget Install returned $lastexitcode"
        Exit $lastexitcode;
    }
}


popd
