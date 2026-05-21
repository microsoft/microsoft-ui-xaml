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

# If this is PowerShell 5, remove PowerShell 7 module paths from PSModulePath to avoid module loading conflicts.
# The pwsh7 path can cause an error inside the credprovider install via missing Get-FileHash command.
$originalPSModulePath = $env:PSModulePath
if ($PSVersionTable.PSVersion.Major -eq 5)
{
    if ($env:PSModulePath -like "*powershell\7\Modules*")
    {
        $paths = $env:PSModulePath -split ';'
        $filteredPaths = $paths | Where-Object { $_ -notlike "*powershell\7\Modules*" }
        $env:PSModulePath = $filteredPaths -join ';'
    }
}

try
{
    Invoke-Expression "& { $(Invoke-RestMethod https://aka.ms/install-artifacts-credprovider.ps1) } -AddNetfx"
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
