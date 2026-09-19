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
    [string]$Nuspec = "Microsoft.ProjectReunion.WinUI.TransportPackage.nuspec"
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

$VersionsDetailsPath = Join-Path "$scriptDirectory\..\.." "eng\Version.Details.xml"
[xml]$versionDetails = Get-Content -Path $VersionsDetailsPath

# Set up Map from the dependencies to its versions to reference later
foreach ($dependency in $versionDetails.Dependencies.ProductDependencies.Dependency)
{
  if ($dependency.name -eq "Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage")
  {
    $IXP_Version = $dependency.version
  }

  if ($dependency.name -eq "Microsoft.WindowsAppSDK.Base")
  {
    $BASE_COMPONENT_VERSION = $dependency.version
  }

  if ($dependency.name -eq "Microsoft.WindowsAppSDK.Foundation")
  {
    $FOUNDATION_COMPONENT_VERSION = $dependency.version
  }

  if ($dependency.name -eq "Microsoft.WindowsAppSDK.InteractiveExperiences")
  {
    $IXP_COMPONENT_VERSION = $dependency.version
  }
}

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

    $EffectivePackageCache = $PackageCache
    if ($env:NUGET_PACKAGES)
    {
        if (![System.IO.Path]::IsPathRooted($env:NUGET_PACKAGES))
        {
            Write-Warning "Skipping cleanup for relative NUGET_PACKAGES path: $env:NUGET_PACKAGES"
        }
        else
        {
            $EffectivePackageCache = [System.IO.Path]::GetFullPath($env:NUGET_PACKAGES)
        }
    }

    $PackageCaches = @($PackageCache, $EffectivePackageCache) | Sort-Object -Unique

    if ($env:NUGET_FALLBACK_PACKAGES)
    {
        $StaleFallbackPaths = @()
        foreach ($FallbackRoot in $env:NUGET_FALLBACK_PACKAGES.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries))
        {
            if (![System.IO.Path]::IsPathRooted($FallbackRoot))
            {
                Write-Warning "Skipping cleanup check for relative NUGET_FALLBACK_PACKAGES path: $FallbackRoot"
                continue
            }

            $FallbackRoot = [System.IO.Path]::GetFullPath($FallbackRoot)
            $FallbackPackagePaths = @(
                (Join-Path $FallbackRoot "$PackageId\$version"),
                (Join-Path $FallbackRoot "$PackageId.$version")
            )

            $StaleFallbackPaths += $FallbackPackagePaths | Where-Object { Test-Path $_ }
        }

        if ($StaleFallbackPaths.Count -gt 0)
        {
            $StaleFallbackPaths = $StaleFallbackPaths | Sort-Object -Unique
            $Paths = $StaleFallbackPaths -join [Environment]::NewLine
            $RemovalCommands = ($StaleFallbackPaths | ForEach-Object {
                "Remove-Item `"$_`" -Recurse -Force"
            }) -join [Environment]::NewLine
            Write-Error @"
The fixed-version development package exists in an active NuGet fallback folder:
$Paths

Close Visual Studio and any active builds, run:
$RemovalCommands

Then rerun pack.component.cmd. If a fallback is shared or read-only, disable it for this
build or ask its owner to remove the stale package.
"@
            Exit 1
        }
    }

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
