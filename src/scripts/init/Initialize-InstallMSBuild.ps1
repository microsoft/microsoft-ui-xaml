param(
    [string]$installDir,
    [string]$Verbosity = 'quiet',
    [string]$logsDir,
    [switch]$ignoreInstalledVS, # set this flag if you never want to use the installed VS from program files
    [ValidateSet("BuildTools169", "Latest16x", "Release17x", "Preview17x")][string]$VsVersionToInstall
)


Import-Module -Name $PSScriptRoot\..\MSBuildFunctions.psm1 -DisableNameChecking

#is VS installed on this machine?
if (Test-Path -Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio\") {
    $msBuildInstallations = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -requires Microsoft.Component.MSBuild -property InstallationPath -prerelease

    Write-Host "MSBuild installations on this machine:"
    Write-Host

    foreach ($msBuildInstallation in $msBuildInstallations) {
        $msBuildPath = (Join-Path $msBuildInstallation "MSBuild\Current\Bin\amd64\msbuild.exe")
        if (Test-Path $msBuildPath) {
            $msBuildVersion = & $msBuildPath -version -noLogo
            Write-Host "$msBuildPath (v$msBuildVersion)"

            # If we weren't run from an init'd command prompt that defines VSINSTALLDIR, Use the first valid VS installation as a fallback
            if ($null -eq $vsInstallDirFallback) { $vsInstallDirFallback = $msBuildInstallation }
        }
        else {
            Write-Host "Path not found: '$msBuildPath'"
        }
    }
}



Write-Host

$useVsBuildTools = $false

# if $ignoreInstalledVS is set, we ignore the globally installed Visual Studio
if ($ignoreInstalledVS) {
    $msbuildUpToDate = $false
}
else {
    if ($null -eq $env:VSINSTALLDIR) {
        # We want to use VSINSTALLDIR if possible, but it will be null if not from a VS dev command prompt.
        # If we didn't, use the VS installation path we found while searching VS instances
        $vsInstallDir = $vsInstallDirFallback
    }
    else {
        # default to using globally installed VS
        $vsInstallDir = $env:VSINSTALLDIR.TrimEnd('\')
    }

    # Check if global VS is valid
    $msbuildUpToDate = Test-MSBuild (Join-Path $vsInstallDir "MSBuild\Current\Bin\amd64\msbuild.exe")
}

# If globally installed VS is out of date, or we're ignoring it, switch to using VS build tools
if (-not $msbuildUpToDate) {
    $useVsBuildTools = $true
    $vsInstallDir = $installDir;
    # Check if VS build tools is valid
    $msbuildUpToDate = Test-MSBuild (Join-Path $vsInstallDir "MSBuild\Current\Bin\amd64\msbuild.exe")
}

if ($useVsBuildTools) {
    $vsInstaller = Join-Path $env:TEMP "vs_buildtools.exe"
}
else {
    $vsInstaller = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vs_installer.exe"
}

# Microsoft.VisualStudio.Component.VC.ATL.ARM64 is not installed by default, 
# so use it as a rough signal that "vs_installer .vsconfig_buildtools ..." needs to be run.
$hasAtl = Test-Path (Join-Path $vsInstallDir "VC\Tools\MSVC\*\atlmfc\lib\arm64")
$needsDownload = -not $msbuildUpToDate 

if (-not $msbuildUpToDate) {
    Write-Host "VS build tools need to be installed."
}
elseif (-not $hasAtl) {
    Write-Host "VS build tools need to be updated."
}

if ($needsDownload) {
    Download-MSBuild -OutFile $vsInstaller
}

if (-not $msbuildUpToDate) {
    Install-MSBuild $vsInstallDir $vsInstaller $logsDir $useVsBuildTools
}
elseif (-not $hasAtl) {
    Modify-MSBuild $vsInstallDir $vsInstaller $logsDir $useVsBuildTools
}

if ($needsDownload) {
    Remove-Item $vsInstaller
}
else {
    Write-Host "VS installer is already up-to-date. No installation required."
}