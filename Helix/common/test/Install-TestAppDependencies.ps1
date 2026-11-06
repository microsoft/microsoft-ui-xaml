# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding()]
param(
    [String]$Platform = $env:testbuildplatform,

    [String]$PayloadRoot = $PSScriptRoot
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Get-NormalizedAppxArchitecture
{
    param(
        [AllowNull()]
        [Object]$Architecture
    )

    if($null -eq $Architecture -or [String]::IsNullOrWhiteSpace([String]$Architecture))
    {
        return "neutral"
    }

    $normalizedArchitecture = ([String]$Architecture).Trim().ToLowerInvariant()
    if($normalizedArchitecture -eq "amd64")
    {
        return "x64"
    }

    return $normalizedArchitecture
}

function Get-AppxPackageIdentity
{
    param(
        [Parameter(Mandatory = $true)]
        [String]$Path
    )

    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $archive = $null
    try
    {
        $archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
        $manifestEntry = @($archive.Entries | Where-Object {
            [String]::Equals($_.FullName, "AppxManifest.xml", [StringComparison]::OrdinalIgnoreCase)
        }) | Select-Object -First 1

        if($null -eq $manifestEntry)
        {
            throw "The package does not contain AppxManifest.xml."
        }

        $manifestStream = $manifestEntry.Open()
        try
        {
            $manifestReader = New-Object System.IO.StreamReader($manifestStream)
            try
            {
                [xml]$manifest = $manifestReader.ReadToEnd()
            }
            finally
            {
                $manifestReader.Dispose()
            }
        }
        finally
        {
            $manifestStream.Dispose()
        }

        $identityNode = $manifest.SelectSingleNode(
            "/*[local-name()='Package']/*[local-name()='Identity']")
        if($null -eq $identityNode)
        {
            throw "AppxManifest.xml does not contain a package identity."
        }

        $name = $identityNode.GetAttribute("Name")
        $publisher = $identityNode.GetAttribute("Publisher")
        $versionText = $identityNode.GetAttribute("Version")
        $architecture = Get-NormalizedAppxArchitecture `
            -Architecture $identityNode.GetAttribute("ProcessorArchitecture")

        foreach($requiredAttribute in @{
            Name = $name
            Publisher = $publisher
            Version = $versionText
        }.GetEnumerator())
        {
            if([String]::IsNullOrWhiteSpace([String]$requiredAttribute.Value))
            {
                throw "AppxManifest.xml package identity is missing '$($requiredAttribute.Key)'."
            }
        }

        $version = $null
        if(![Version]::TryParse($versionText, [ref]$version))
        {
            throw "AppxManifest.xml package identity has invalid version '$versionText'."
        }

        return [pscustomobject]@{
            Name = $name
            Publisher = $publisher
            Architecture = $architecture
            Version = $version
        }
    }
    catch
    {
        throw "Failed to read the package identity from '$Path': $($_.Exception.Message)"
    }
    finally
    {
        if($null -ne $archive)
        {
            $archive.Dispose()
        }
    }
}

function Get-HighestInstalledAppxPackageVersion
{
    param(
        [Parameter(Mandatory = $true)]
        [Object]$Identity
    )

    $highestInstalledVersion = $null
    foreach($installedPackage in @(Get-AppxPackage -Name $Identity.Name))
    {
        if(![String]::Equals(
            [String]$installedPackage.Name,
            [String]$Identity.Name,
            [StringComparison]::OrdinalIgnoreCase))
        {
            continue
        }

        if(![String]::Equals(
            [String]$installedPackage.Publisher,
            [String]$Identity.Publisher,
            [StringComparison]::OrdinalIgnoreCase))
        {
            continue
        }

        $installedArchitecture = Get-NormalizedAppxArchitecture `
            -Architecture $installedPackage.Architecture
        if($installedArchitecture -ne $Identity.Architecture)
        {
            continue
        }

        $installedVersion = $null
        if(![Version]::TryParse([String]$installedPackage.Version, [ref]$installedVersion))
        {
            throw "Installed package '$($installedPackage.PackageFullName)' has invalid version '$($installedPackage.Version)'."
        }

        if($null -eq $highestInstalledVersion -or
           $installedVersion.CompareTo($highestInstalledVersion) -gt 0)
        {
            $highestInstalledVersion = $installedVersion
        }
    }

    return $highestInstalledVersion
}

if(!$Platform)
{
    $Platform = "x86"
    $buildConfigPath = Join-Path $PayloadRoot "buildconfig.json"
    if(Test-Path -LiteralPath $buildConfigPath)
    {
        $buildConfig = Get-Content -LiteralPath $buildConfigPath | Out-String | ConvertFrom-Json
        $Platform = $buildConfig.Platform
    }
}

$redistPlatform = $Platform
if($Platform -eq "arm64ec")
{
    $redistPlatform = "x64"
}

$packagesToInstall = @(
    "Microsoft.VCLibs.$redistPlatform.Debug.14.00.appx",
    "Microsoft.VCLibs.$redistPlatform.14.00.appx",
    "Microsoft.VCLibs.$redistPlatform.Debug.14.00.Desktop.appx",
    "Microsoft.VCLibs.$redistPlatform.14.00.Desktop.appx",
    "Microsoft.NET.CoreRuntime.1.1.appx",
    "Microsoft.NET.CoreRuntime.2.2.appx",
    "Microsoft.NET.CoreFramework.Debug.2.2.appx",
    "Microsoft.NET.Native.Framework.2.2.appx",
    "Microsoft.NET.Native.Framework.Debug.2.2.appx",
    "Microsoft.NET.Native.Runtime.2.2.appx"
)

$testPackageRoot = Join-Path $PayloadRoot "Test"
foreach($package in $packagesToInstall)
{
    $packagePath = Join-Path $testPackageRoot $package
    if(Test-Path -LiteralPath $packagePath -PathType Leaf)
    {
        $packageIdentity = Get-AppxPackageIdentity -Path $packagePath
        $installedVersion = Get-HighestInstalledAppxPackageVersion -Identity $packageIdentity
        if($null -ne $installedVersion -and
           $installedVersion.CompareTo($packageIdentity.Version) -ge 0)
        {
            $skipMessage = ("Skipping {0}. {1} {2} version {3} is already registered for " +
                "the current user and satisfies payload version {4}.") -f
                $package,
                $packageIdentity.Name,
                $packageIdentity.Architecture,
                $installedVersion,
                $packageIdentity.Version
            Write-Host $skipMessage
            continue
        }

        Write-Host ("Installing {0} ({1} {2} version {3})." -f
            $package,
            $packageIdentity.Name,
            $packageIdentity.Architecture,
            $packageIdentity.Version)
        $installErrors = @()
        Add-AppxPackage -Path $packagePath -ErrorVariable installErrors -ErrorAction SilentlyContinue
        foreach($errorRecord in @($installErrors))
        {
            # A same or higher package version is already sufficient for discovery and execution.
            if(($errorRecord.Exception.Message -match "0x80073D06") -or
               ($errorRecord.Exception.Message -match "0x80073CFB"))
            {
                Write-Host "The same or higher version of this package is already installed."
            }
            else
            {
                Write-Error -ErrorRecord $errorRecord
            }
        }
    }
}
