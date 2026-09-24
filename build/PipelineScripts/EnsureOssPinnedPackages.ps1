# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
<#
.SYNOPSIS
Ensures the safe OSS-pinned dependency packages are available in the shine-oss feed.
#>

[CmdletBinding()]
param(
    [ValidateSet("CheckFetch", "Push")]
    [string]$Mode = "CheckFetch",

    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,

    [string]$PackagesDirectory = (Join-Path (Join-Path $RepositoryRoot "out") "packages-to-push"),

    [string]$SummaryPath = (Join-Path (Join-Path $RepositoryRoot "out") "oss-pinned-package-summary.json"),

    [string]$NuGetOrgSource = "https://api.nuget.org/v3/index.json",

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$InternalFeedSource,

    [string]$ShineOssFeedSource = "https://pkgs.dev.azure.com/shine-oss/microsoft-ui-xaml/_packaging/WinUI-Dependencies/nuget/v3/index.json",

    [string]$ShineOssLocalViewSource = "https://pkgs.dev.azure.com/shine-oss/microsoft-ui-xaml/_packaging/WinUI-Dependencies@Local/nuget/v3/index.json",

    [switch]$SkipFeedPresenceCheck,

    [switch]$ResolveOnly
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.IO.Compression.FileSystem

function Invoke-NuGet {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [switch]$AllowFailure,

        [switch]$Quiet
    )

    $displayArguments = @($Arguments | ForEach-Object {
        if ($_ -match "\s") {
            "`"$_`""
        } else {
            $_
        }
    })
    Write-Host "nuget $($displayArguments -join ' ')"

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = & nuget @Arguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if (-not $Quiet) {
        $output | ForEach-Object { Write-Host $_ }
    }

    if ($exitCode -ne 0 -and -not $AllowFailure) {
        throw "nuget $($Arguments[0]) failed with exit code $exitCode."
    }

    [pscustomobject]@{
        ExitCode = $exitCode
        Output = $output
    }
}

function Remove-DirectoryIfPresent {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }
}

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

function Get-PackagesConfigVersion {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PackageId
    )

    $packagesConfigPath = Join-Path $RepositoryRoot "packages.config"
    $xml = Get-XmlDocument -Path $packagesConfigPath
    $nodes = @($xml.packages.package | Where-Object { $_.id -eq $PackageId })

    if ($nodes.Count -ne 1) {
        throw "Expected exactly one '$PackageId' entry in '$packagesConfigPath', found $($nodes.Count)."
    }

    $version = $nodes[0].version
    if ([string]::IsNullOrWhiteSpace($version)) {
        throw "Package '$PackageId' in '$packagesConfigPath' has an empty version."
    }

    $version
}

function Get-RequiredPackages {
    @(
        [pscustomobject]@{
            Id = "Microsoft.Internal.WinUILocalizationResources"
            Version = Get-PackagesConfigVersion -PackageId "Microsoft.Internal.WinUILocalizationResources"
            SourceName = "WinUI.Dependencies"
            SourceUrl = $InternalFeedSource
        }
        [pscustomobject]@{
            Id = "Microsoft.WindowsAppSDK.Foundation"
            Version = Get-OssVersionsPropsValue -PropertyName "FoundationPackageVersion"
            SourceName = "WinUI.Dependencies"
            SourceUrl = $InternalFeedSource
        }
        [pscustomobject]@{
            Id = "Microsoft.WindowsAppSDK.Base"
            Version = Get-OssVersionsPropsValue -PropertyName "BasePackageVersion"
            SourceName = "WinUI.Dependencies"
            SourceUrl = $InternalFeedSource
        }
    )
}

function New-TemporaryDirectory {
    $path = Join-Path ([System.IO.Path]::GetTempPath()) ("oss-pinned-package-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $path | Out-Null
    $path
}

function Get-NupkgMetadata {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $zip = [System.IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $nuspecEntry = @($zip.Entries | Where-Object { $_.FullName -like "*.nuspec" }) | Select-Object -First 1
        if ($null -eq $nuspecEntry) {
            throw "Package '$Path' does not contain a nuspec."
        }

        $stream = $nuspecEntry.Open()
        try {
            $reader = [System.IO.StreamReader]::new($stream)
            try {
                [xml]$nuspec = $reader.ReadToEnd()
            } finally {
                $reader.Dispose()
            }
        } finally {
            $stream.Dispose()
        }

        $idNode = $nuspec.SelectSingleNode("/*[local-name()='package']/*[local-name()='metadata']/*[local-name()='id']")
        $versionNode = $nuspec.SelectSingleNode("/*[local-name()='package']/*[local-name()='metadata']/*[local-name()='version']")
        if ($null -eq $idNode -or $null -eq $versionNode) {
            throw "Package '$Path' nuspec is missing id or version metadata."
        }

        [pscustomobject]@{
            Id = $idNode.InnerText.Trim()
            Version = $versionNode.InnerText.Trim()
        }
    } finally {
        $zip.Dispose()
    }
}

function Test-PackageAvailableInShineOss {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Package
    )

    if ($SkipFeedPresenceCheck) {
        return $false
    }

    $tempPath = New-TemporaryDirectory
    try {
        $result = Invoke-NuGet -Arguments @(
            "install", $Package.Id,
            "-Version", $Package.Version,
            "-Source", $ShineOssLocalViewSource,
            "-OutputDirectory", $tempPath,
            "-NonInteractive",
            "-NoHttpCache",
            "-DirectDownload",
            "-ExcludeVersion",
            "-DependencyVersion", "Ignore",
            "-Verbosity", "quiet"
        ) -AllowFailure -Quiet

        if ($result.ExitCode -eq 0) {
            return $true
        }

        $outputText = ($result.Output | Out-String)
        if ($outputText -match "Unable to find version|Unable to find package|No packages found|not found on source") {
            return $false
        }

        throw "Failed to query shine-oss @Local for '$($Package.Id)' '$($Package.Version)'. NuGet output: $outputText"
    } finally {
        Remove-DirectoryIfPresent -Path $tempPath
    }
}

function Download-Package {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Package
    )

    $tempPath = New-TemporaryDirectory
    try {
        Invoke-NuGet -Arguments @(
            "install", $Package.Id,
            "-Version", $Package.Version,
            "-Source", $Package.SourceUrl,
            "-OutputDirectory", $tempPath,
            "-NonInteractive",
            "-NoHttpCache",
            "-DirectDownload",
            "-DependencyVersion", "Ignore"
        ) | Out-Null

        $downloadedPackages = @(Get-ChildItem -LiteralPath $tempPath -Recurse -File -Filter "*.nupkg" | Where-Object {
            $_.Name -notlike "*.symbols.nupkg"
        })
        if ($downloadedPackages.Count -ne 1) {
            throw "Expected exactly one package for '$($Package.Id)' '$($Package.Version)' from '$($Package.SourceName)', found $($downloadedPackages.Count)."
        }

        $metadata = Get-NupkgMetadata -Path $downloadedPackages[0].FullName
        if ($metadata.Id -ne $Package.Id -or $metadata.Version -ne $Package.Version) {
            throw "Downloaded '$($downloadedPackages[0].FullName)' has nuspec '$($metadata.Id)' '$($metadata.Version)', expected '$($Package.Id)' '$($Package.Version)'."
        }

        New-Item -ItemType Directory -Path $PackagesDirectory -Force | Out-Null
        $destinationPath = Join-Path $PackagesDirectory $downloadedPackages[0].Name
        Copy-Item -LiteralPath $downloadedPackages[0].FullName -Destination $destinationPath -Force
        $destinationPath
    } finally {
        Remove-DirectoryIfPresent -Path $tempPath
    }
}

function Write-Summary {
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Entries
    )

    $summaryDirectory = Split-Path -Parent $SummaryPath
    New-Item -ItemType Directory -Path $summaryDirectory -Force | Out-Null
    ConvertTo-Json -InputObject @($Entries) -Depth 5 | Set-Content -LiteralPath $SummaryPath -Encoding UTF8
    Write-Host "Wrote OSS pinned package summary to '$SummaryPath'."
}

function Invoke-CheckFetch {
    $requiredPackages = @(Get-RequiredPackages)
    $summary = @()

    Remove-DirectoryIfPresent -Path $PackagesDirectory
    New-Item -ItemType Directory -Path $PackagesDirectory -Force | Out-Null

    foreach ($package in $requiredPackages) {
        Write-Host "Required OSS package pin: $($package.Id) $($package.Version)."

        if ($ResolveOnly) {
            $summary += [pscustomobject]@{
                Id = $package.Id
                Version = $package.Version
                SourceName = $package.SourceName
                Action = "ResolvedOnly"
                PackagePath = $null
            }
            continue
        }

        $isAvailable = Test-PackageAvailableInShineOss -Package $package
        if ($isAvailable) {
            Write-Host "$($package.Id) $($package.Version) already exists in the shine-oss @Local view."
            $summary += [pscustomobject]@{
                Id = $package.Id
                Version = $package.Version
                SourceName = $package.SourceName
                Action = "AlreadyAvailable"
                PackagePath = $null
            }
            continue
        }

        Write-Host "$($package.Id) $($package.Version) is missing from shine-oss @Local. Fetching from $($package.SourceName)."
        $packagePath = Download-Package -Package $package
        $summary += [pscustomobject]@{
            Id = $package.Id
            Version = $package.Version
            SourceName = $package.SourceName
            Action = "FetchedForPush"
            PackagePath = $packagePath
        }
    }

    Write-Summary -Entries $summary
}

function Invoke-Push {
    if ($ResolveOnly) {
        throw "-ResolveOnly is only valid with -Mode CheckFetch."
    }

    $requiredPackages = @(Get-RequiredPackages)
    $requiredById = @{}
    foreach ($package in $requiredPackages) {
        $requiredById[$package.Id.ToLowerInvariant()] = $package
    }

    if (-not (Test-Path -LiteralPath $PackagesDirectory)) {
        Write-Host "Package directory '$PackagesDirectory' does not exist; no missing pinned packages were staged."
        return
    }

    $packagesToPush = @(Get-ChildItem -LiteralPath $PackagesDirectory -Recurse -File -Filter "*.nupkg" | Where-Object {
        $_.Name -notlike "*.symbols.nupkg"
    })
    if ($packagesToPush.Count -eq 0) {
        Write-Host "No missing pinned packages were staged; nothing to push."
        return
    }

    $validatedPackages = @()
    foreach ($packageFile in $packagesToPush) {
        $metadata = Get-NupkgMetadata -Path $packageFile.FullName
        $key = $metadata.Id.ToLowerInvariant()
        if (-not $requiredById.ContainsKey($key)) {
            throw "Package '$($packageFile.FullName)' has ID '$($metadata.Id)', which is not configured for automatic safe OSS promotion."
        }

        $expected = $requiredById[$key]
        if ($metadata.Version -ne $expected.Version) {
            throw "Package '$($packageFile.FullName)' has version '$($metadata.Version)', expected pinned version '$($expected.Version)' for '$($metadata.Id)'."
        }

        $validatedPackages += [pscustomobject]@{
            File = $packageFile
            Metadata = $metadata
            RequiredPackage = $expected
        }
    }

    foreach ($validated in $validatedPackages) {
        Invoke-NuGet -Arguments @(
            "push", $validated.File.FullName,
            "-Source", $ShineOssFeedSource,
            "-ApiKey", "AzureDevOps",
            "-NonInteractive",
            "-SkipDuplicate"
        ) | Out-Null

        if (-not $SkipFeedPresenceCheck -and -not (Test-PackageAvailableInShineOss -Package $validated.RequiredPackage)) {
            throw "Post-push verification failed for '$($validated.Metadata.Id)' '$($validated.Metadata.Version)' in shine-oss @Local."
        }
    }
}

Write-Host "Repository root: $RepositoryRoot"
Write-Host "Packages directory: $PackagesDirectory"
Write-Host "Summary path: $SummaryPath"

switch ($Mode) {
    "CheckFetch" { Invoke-CheckFetch }
    "Push" { Invoke-Push }
}
