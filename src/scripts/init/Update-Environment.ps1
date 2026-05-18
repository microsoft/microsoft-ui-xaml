param(
    [string] $repoRoot
)

$ErrorActionPreference = "Stop"

if (!$repoRoot -or !(Test-Path $repoRoot)) {
    Write-Host "The option -repoRoot was not specified or was not found, assuming current working directory"
    $repoRoot = (Resolve-Path .\).Path
}

$packagesDir = "$repoRoot\packages"

# Prefer the absolute path of the environment's nuget.exe if it exists, otherwise use nuget.exe on the path
$nugetExe = "$repoRoot\.tools\VSS.NuGet\nuget.exe"
if(!(Test-Path $nugetExe)) {
    # Check if there's one on the path\
    $nugetExe = (Get-Command -ErrorAction SilentlyContinue -Name nuget).Path
    if (! $nugetExe) {
        Write-Error "Failed to find nuget.exe.  Please ensure a nuget.exe is on the path."
    }
}

# Get current version
$packageName = "Microsoft.VisualStudio.Services.NuGet.Bootstrap"
$versionMarkFilePath = "$repoRoot\scripts\init\.version"
$currentVersion = (Get-Content $versionMarkFilePath)
$nugetSource = "https://nuget.org/api/v2/"
Write-Host "Current version is $currentVersion"

# Get newest available version
$listOutput = & $nugetExe list -source $nugetSource -NonInteractive $packageName

if ($LastExitCode -ne 0 -or (! $listOutput)) {
    Write-Error "Unable to find $packageName"
}

$latestLine = $listOutput | Select-String -Pattern $packageName | Select-Object -First 1
if ((! $latestLine) ) {
    Write-Error "Error parsing available versions of $packageName"
}

$newestVersion = ($latestLine -split ' ')[1]
Write-Host "Newest version is $newestVersion"

# Note: Doesn't actually check that it's newer.
# Instead of implementing a semver check here, just leave it up to the user to confirm they actually want to "update"
if ($currentVersion -ne $newestVersion) {
    $confirmation = Read-Host "Update from $currentVersion to $($newestVersion)? [y/n]"
    if ($confirmation -eq "y") {

        # Download the newest version
        & $nugetExe install $packageName -Version $newestVersion -OutputDirectory $packagesDir -source $nugetSource

        # Bootstrap
        $bootstrapPath = Resolve-Path "$repoRoot\packages\$packageName.$newestVersion\tools\Bootstrap.ps1"
        & $bootstrapPath -RepoRoot $repoRoot
    }
}