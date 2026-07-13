<#
.SYNOPSIS
Restores native packages whose OSS versions are pinned in eng\Versions.props.
#>

[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,

    [string]$PackagesDirectory = (Join-Path $RepositoryRoot "packages"),

    [string]$NuGetConfigPath = (Join-Path $RepositoryRoot "nuget.config"),

    [string]$Verbosity = "quiet",

    [switch]$Force
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = "Stop"

function Get-XmlDocument {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Required file '$Path' does not exist."
    }

    [xml](Get-Content -LiteralPath $Path -Raw)
}

function Get-OssVersionsPropsValue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PropertyName
    )

    $versionsPropsPath = Join-Path $RepositoryRoot "eng\Versions.props"
    $xml = Get-XmlDocument -Path $versionsPropsPath
    $ossCondition = "'`$(IsInternalWinUIBuild)' != 'true'"
    $nodes = @($xml.Project.PropertyGroup.ChildNodes | Where-Object {
        $_.NodeType -eq [System.Xml.XmlNodeType]::Element -and
        $_.Name -eq $PropertyName -and
        $_.HasAttribute("Condition") -and
        $_.GetAttribute("Condition") -eq $ossCondition
    })

    if ($nodes.Count -ne 1) {
        throw "Expected exactly one OSS-pinned '$PropertyName' value in '$versionsPropsPath', found $($nodes.Count)."
    }

    $value = $nodes[0].InnerText.Trim()
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "OSS-pinned '$PropertyName' in '$versionsPropsPath' is empty."
    }

    $value
}

function Restore-NativePackage {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PackageId,

        [Parameter(Mandatory = $true)]
        [string]$Version
    )

    Write-Host "Restoring OSS-pinned native package $PackageId $Version..."
    & nuget install $PackageId `
        -Version $Version `
        -OutputDirectory $PackagesDirectory `
        -ConfigFile $NuGetConfigPath `
        -NonInteractive `
        -NoHttpCache `
        -DependencyVersion Ignore `
        -Verbosity $Verbosity

    if ($LASTEXITCODE -ne 0) {
        throw "Failed to restore OSS-pinned native package '$PackageId' '$Version'."
    }

    $expectedPackagePath = Join-Path $PackagesDirectory "$PackageId.$Version"
    if (-not (Test-Path -LiteralPath $expectedPackagePath)) {
        throw "Restore completed, but expected package path '$expectedPackagePath' does not exist."
    }
}

if ((Test-Path -LiteralPath (Join-Path $RepositoryRoot ".azuredevops")) -and -not $Force) {
    Write-Host "Skipping OSS-pinned native package restore for internal WinUI build."
    return
}

$packages = @(
    [pscustomobject]@{
        Id = "Microsoft.WindowsAppSDK.Foundation"
        PropertyName = "FoundationPackageVersion"
    }
    [pscustomobject]@{
        Id = "Microsoft.WindowsAppSDK.InteractiveExperiences"
        PropertyName = "IXPPackageVersion"
    }
    [pscustomobject]@{
        Id = "Microsoft.WindowsAppSDK.Base"
        PropertyName = "BasePackageVersion"
    }
)

foreach ($package in $packages) {
    Restore-NativePackage -PackageId $package.Id -Version (Get-OssVersionsPropsValue -PropertyName $package.PropertyName)
}
