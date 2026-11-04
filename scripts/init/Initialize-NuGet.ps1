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
