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

# See: https://github.com/microsoft/artifacts-credprovider
Write-Progress "Downloading the Azure Artifacts Credential Provider"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# If this is Windows PowerShell 5, prefer its built-in modules over inherited PowerShell 7 modules.
# PowerShell 7 paths can otherwise resolve an incompatible Microsoft.PowerShell.Utility module.
$originalPSModulePath = $env:PSModulePath
if ($PSVersionTable.PSVersion.Major -eq 5)
{
    $systemModulePath = Join-Path $PSHOME "Modules"
    $modulePaths = @(
        $env:PSModulePath -split ';' |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    $firstModulePath = if ($modulePaths.Count -gt 0) { $modulePaths[0] } else { $null }
    if ($firstModulePath -ne $systemModulePath)
    {
        $modulePaths = @($systemModulePath) + @(
            $modulePaths | Where-Object { $_ -ne $systemModulePath }
        )
        $env:PSModulePath = $modulePaths -join ';'
    }
}

try
{
    $pluginDirectory = Join-Path $env:USERPROFILE ".nuget\plugins"
    $providers = @(
        @{ Runtime = "netfx"; EntryPoint = "CredentialProvider.Microsoft.exe" }
        @{ Runtime = "netcore"; EntryPoint = "CredentialProvider.Microsoft.dll" }
    )
    $missingProviders = @($providers | Where-Object {
        !(Test-Path -PathType Leaf (Join-Path $pluginDirectory "$($_.Runtime)\CredentialProvider.Microsoft\$($_.EntryPoint)"))
    })

    if ($missingProviders.Count -gt 0)
    {
        # Download the release asset directly to avoid GitHub API rate limits.
        $providerArchive = . "$PSScriptRoot\Initialize-DownloadLatest.ps1" -OutDir $toolsDir -DownloadUrl "https://github.com/microsoft/artifacts-credprovider/releases/latest/download/Microsoft.NetFx48.NuGet.CredentialProvider.zip" -DownloadName "AzureArtifactsCredentialProvider" -Unzip $true

        foreach ($provider in $missingProviders)
        {
            $source = Join-Path $providerArchive "plugins\$($provider.Runtime)\CredentialProvider.Microsoft"
            if (!(Test-Path -PathType Leaf (Join-Path $source $provider.EntryPoint)))
            {
                throw "The Azure Artifacts Credential Provider archive is missing $($provider.Runtime)\$($provider.EntryPoint)."
            }

            $destination = Join-Path $pluginDirectory $provider.Runtime
            New-Item -ItemType Directory -Force -Path $destination | Out-Null
            Copy-Item -Path $source -Destination $destination -Recurse -Force
        }
    }
}
finally
{
    # Restore original PSModulePath
    $env:PSModulePath = $originalPSModulePath
}

# Add the tools dir to the path which directly contains NuGet.exe and VSS.NuGet.AuthHelper.exe
if (!($env:Path -like "*$toolsDir;*"))
{
    $env:Path = "$toolsDir;" + $env:Path
}
