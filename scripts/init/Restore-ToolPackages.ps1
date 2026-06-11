Param(
    [Parameter(Mandatory=$true)] [string] $repoRoot,
    [string] $Verbosity = 'quiet'
)

$ErrorActionPreference = "Stop"

$toolsPackagesConfigPath = "$repoRoot\.nuget\tools\packages.config"
$packagesDirectory = "$repoRoot\packages"

# Restore NuGet tools packages, unless we're on a build machine
if((Test-Path env:\BUILD_BUILDNUMBER) -eq $true)
{
    return;
}

if (Test-Path $toolsPackagesConfigPath) {
    Write-Host "Restoring tool packages..."
    nuget restore -PackagesDirectory $packagesDirectory $toolsPackagesConfigPath -Verbosity $Verbosity
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to restore tool packages."
    } else {
        Write-Host "Restored tool packages"
    }
}