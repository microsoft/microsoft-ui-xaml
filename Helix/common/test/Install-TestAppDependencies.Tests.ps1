# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

function New-TestAppxPackage
{
    param(
        [Parameter(Mandatory = $true)]
        [String]$Path,

        [String]$Name = "Microsoft.VCLibs.140.00",

        [String]$Publisher = "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US",

        [String]$Architecture = "x64",

        [String]$Version = "14.0.33519.0",

        [Switch]$OmitArchitecture,

        [Switch]$OmitVersion
    )

    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $packageDirectory = Join-Path $TestDrive ([Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $packageDirectory | Out-Null

    $versionAttribute = if($OmitVersion) { "" } else { " Version=`"$Version`"" }
    $architectureAttribute = if($OmitArchitecture) { "" } else { " ProcessorArchitecture=`"$Architecture`"" }
    @"
<?xml version="1.0" encoding="utf-8"?>
<Package xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10">
  <Identity Name="$Name" Publisher="$Publisher"$architectureAttribute$versionAttribute />
</Package>
"@ | Set-Content -LiteralPath (Join-Path $packageDirectory "AppxManifest.xml") -Encoding UTF8

    [System.IO.Compression.ZipFile]::CreateFromDirectory($packageDirectory, $Path)
}

function New-InstalledTestPackage
{
    param(
        [String]$Name = "Microsoft.VCLibs.140.00",

        [String]$Publisher = "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US",

        [String]$Architecture = "X64",

        [String]$Version = "14.0.33519.0"
    )

    [pscustomobject]@{
        Name = $Name
        Publisher = $Publisher
        Architecture = $Architecture
        Version = [Version]$Version
        PackageFullName = "${Name}_${Version}_${Architecture}__test"
    }
}

Describe "Install-TestAppDependencies package applicability" {
    BeforeEach {
        $script:subject = Join-Path $PSScriptRoot "Install-TestAppDependencies.ps1"
        $script:payloadRoot = Join-Path $TestDrive ([Guid]::NewGuid().ToString("N"))
        $script:testPackageRoot = Join-Path $script:payloadRoot "Test"
        New-Item -ItemType Directory -Path $script:testPackageRoot | Out-Null

        $script:packagePath = Join-Path $script:testPackageRoot "Microsoft.VCLibs.x64.14.00.appx"
        New-TestAppxPackage -Path $script:packagePath

        $script:state = @{
            InstalledPackages = @()
        }
        $state = $script:state
        Mock Get-AppxPackage ({
            $state.InstalledPackages
        }.GetNewClosure())
        Mock Add-AppxPackage {}
        Mock Write-Host {}
    }

    It "skips a matching package at the same version" {
        $script:state.InstalledPackages = @(New-InstalledTestPackage)

        & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot

        Assert-MockCalled Get-AppxPackage -Times 1 -Exactly -Scope It -ParameterFilter {
            $Name -eq "Microsoft.VCLibs.140.00" -and !$AllUsers
        }
        Assert-MockCalled Add-AppxPackage -Times 0 -Exactly -Scope It
        Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter {
            $Object -eq ("Skipping Microsoft.VCLibs.x64.14.00.appx. " +
                "Microsoft.VCLibs.140.00 x64 version 14.0.33519.0 is already registered " +
                "for the current user and satisfies payload version 14.0.33519.0.")
        }
    }

    It "skips a matching package at a higher version" {
        $script:state.InstalledPackages = @(New-InstalledTestPackage -Version "14.0.40000.0")

        & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot

        Assert-MockCalled Add-AppxPackage -Times 0 -Exactly -Scope It
    }

    It "uses the highest version among matching registrations" {
        $script:state.InstalledPackages = @(
            (New-InstalledTestPackage -Version "14.0.30000.0"),
            (New-InstalledTestPackage -Version "14.0.40000.0"),
            (New-InstalledTestPackage -Version "14.0.35000.0")
        )

        & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot

        Assert-MockCalled Add-AppxPackage -Times 0 -Exactly -Scope It
        Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter {
            [String]$Object -match "version 14\.0\.40000\.0"
        }
    }

    It "installs when the matching package is older" {
        $script:state.InstalledPackages = @(New-InstalledTestPackage -Version "14.0.30000.0")

        & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot

        Assert-MockCalled Add-AppxPackage -Times 1 -Exactly -Scope It -ParameterFilter {
            $Path -eq $script:packagePath
        }
    }

    It "installs when the package is not registered for the current user" {
        & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot

        Assert-MockCalled Add-AppxPackage -Times 1 -Exactly -Scope It
    }

    It "treats an omitted payload architecture as neutral" {
        Remove-Item -LiteralPath $script:packagePath
        New-TestAppxPackage -Path $script:packagePath -OmitArchitecture
        $script:state.InstalledPackages = @(
            New-InstalledTestPackage -Architecture "Neutral"
        )

        & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot

        Assert-MockCalled Add-AppxPackage -Times 0 -Exactly -Scope It
    }

    It "does not accept a different publisher or architecture" {
        $script:state.InstalledPackages = @(
            (New-InstalledTestPackage -Publisher "CN=Different Publisher" -Version "99.0.0.0"),
            (New-InstalledTestPackage -Architecture "X86" -Version "99.0.0.0")
        )

        & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot

        Assert-MockCalled Add-AppxPackage -Times 1 -Exactly -Scope It
    }

    It "fails explicitly when the manifest identity is malformed" {
        Remove-Item -LiteralPath $script:packagePath
        New-TestAppxPackage -Path $script:packagePath -OmitVersion

        { & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot } |
            Should Throw "missing 'Version'"
        Assert-MockCalled Add-AppxPackage -Times 0 -Exactly -Scope It
    }

    It "fails explicitly when the package cannot be read" {
        Set-Content -LiteralPath $script:packagePath -Value "not an appx package"

        { & $script:subject -Platform x64 -PayloadRoot $script:payloadRoot } |
            Should Throw "Failed to read the package identity"
        Assert-MockCalled Add-AppxPackage -Times 0 -Exactly -Scope It
    }
}
