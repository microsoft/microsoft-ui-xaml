Param(
    [Parameter(Mandatory=$true)] [string] $repoRoot,
    [string]$Verbosity = 'quiet'
)

$ErrorActionPreference = "Stop"

# Create the .tools directory
New-Item -ItemType Directory -Force -Path "$repoRoot\.tools" | Out-Null
$toolsDir = Join-Path -Resolve $repoRoot ".tools"

# Ensure nuget.exe is up-to-date
$nugetDownloadName = "nuget.exe"
$nuget_exe = . "$PSScriptRoot\Initialize-DownloadLatest.ps1" -OutDir $toolsDir -DownloadUrl "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -DownloadName $nugetDownloadName -Unzip $false

# Install the Azure Artifacts Credential Provider without querying the anonymous
# GitHub REST API. GitHub-hosted runners share public egress addresses, so the
# installer's /releases/latest API lookup can exhaust the unauthenticated per-IP
# quota even though the same URL works from a developer machine.
#
# The releases/latest/download route preserves the existing "latest stable"
# behavior while downloading the official release assets directly.
Write-Progress "Downloading the Azure Artifacts Credential Provider"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$credentialProviderReleaseBase = "https://github.com/microsoft/artifacts-credprovider/releases/latest/download"

$hostRid = switch ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture)
{
    ([System.Runtime.InteropServices.Architecture]::X64)   { "win-x64"; break }
    ([System.Runtime.InteropServices.Architecture]::X86)   { "win-x86"; break }
    ([System.Runtime.InteropServices.Architecture]::Arm64) { "win-arm64"; break }
    default { throw "Unsupported Windows host architecture for artifacts-credprovider." }
}

$netfxArchive = "Microsoft.NetFx48.NuGet.CredentialProvider.zip"
$netcoreArchive = "Microsoft.$hostRid.NuGet.CredentialProvider.zip"

$netfxPackage = . "$PSScriptRoot\Initialize-DownloadLatest.ps1" -OutDir $toolsDir -DownloadUrl "$credentialProviderReleaseBase/$netfxArchive" -DownloadName "ArtifactsCredentialProvider.NetFx48" -Unzip $true
$netcorePackage = . "$PSScriptRoot\Initialize-DownloadLatest.ps1" -OutDir $toolsDir -DownloadUrl "$credentialProviderReleaseBase/$netcoreArchive" -DownloadName "ArtifactsCredentialProvider.$hostRid" -Unzip $true

$pluginRoot = Join-Path ([System.Environment]::GetFolderPath([System.Environment+SpecialFolder]::UserProfile)) ".nuget\plugins"
$netfxSource = Join-Path $netfxPackage "plugins\netfx\CredentialProvider.Microsoft"
$netcoreSource = Join-Path $netcorePackage "plugins\netcore\CredentialProvider.Microsoft"
$netfxDestination = Join-Path $pluginRoot "netfx\CredentialProvider.Microsoft"
$netcoreDestination = Join-Path $pluginRoot "netcore\CredentialProvider.Microsoft"

foreach ($path in @($netfxSource, $netcoreSource))
{
    if (!(Test-Path $path))
    {
        throw "Credential Provider package is missing expected path: $path"
    }
}

New-Item -ItemType Directory -Force -Path (Split-Path $netfxDestination -Parent) | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $netcoreDestination -Parent) | Out-Null

if (Test-Path $netfxDestination)
{
    Remove-Item $netfxDestination -Force -Recurse
}
if (Test-Path $netcoreDestination)
{
    Remove-Item $netcoreDestination -Force -Recurse
}

Copy-Item $netfxSource -Destination $netfxDestination -Force -Recurse
Copy-Item $netcoreSource -Destination $netcoreDestination -Force -Recurse

Write-Host "Credential Provider installed successfully"
# Add the tools dir to the path which directly contains NuGet.exe and VSS.NuGet.AuthHelper.exe
if (!($env:Path -like "*$toolsDir;*"))
{
    $env:Path = "$toolsDir;" + $env:Path
}
