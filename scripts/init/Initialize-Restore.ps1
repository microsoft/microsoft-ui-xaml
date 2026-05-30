param(
    [Parameter(Mandatory=$true)] [string] $repoRoot,
    [string]$Verbosity = 'quiet'
)

$startTime = [datetime]::Now
$ErrorActionPreference = "Stop"
# Suppress Write-Progress rendering which can crash PowerShell 5.1 in some terminal environments.
if ($Verbosity -eq 'quiet') {
    $ProgressPreference = "SilentlyContinue"
}

$scriptsDir = $PSScriptRoot
$customScriptsDir = "$repoRoot\scripts\"
$preInitScript = "$customScriptsDir\PreInit.ps1"
$preRestoreToolsScript = "$customScriptsDir\PreRestoreTools.ps1"
$postInitScript = "$customScriptsDir\PostInit.ps1"

# Run PreInit.ps1 if it exists
if (Test-Path $preInitScript) {
    . $preInitScript -RepoRoot $repoRoot
}

# Enable long path support if necessary
. "$scriptsDir\Initialize-CheckLongPathSupport.ps1"

# Download the NuGet tools if necessary
. "$scriptsDir\Initialize-NuGet.ps1" -RepoRoot $repoRoot -Verbosity $Verbosity

# Download the MSBuild tools if necessary
. "$scriptsDir\Initialize-InstallMSBuild.ps1" -InstallDir "$repoRoot\.buildtools" -Verbosity $Verbosity

# Run PreRestoreTools.ps1 if it exists
if (Test-Path $preRestoreToolsScript) {
    . $preRestoreToolsScript -RepoRoot $repoRoot
}

# Restores tool packages (if .nuget\tools\packages.config exists)
. "$scriptsDir\Restore-ToolPackages.ps1" -RepoRoot $repoRoot -Verbosity $Verbosity

# Run PostInit.ps1 if it exists
if (Test-Path $postInitScript) {
    . $postInitScript -RepoRoot $repoRoot -Verbosity $Verbosity
}

if (Test-Path "$env:reporoot\docs\init-known-issues.md") {
    type "$env:reporoot\docs\init-known-issues.md"
}

$duration = (([datetime]::Now - $startTime).TotalSeconds).ToString("N2")
Write-Host Initialized environment for $env:_BuildArch $env:_BuildType `($duration s`)
