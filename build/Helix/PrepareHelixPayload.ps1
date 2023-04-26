[CmdLetBinding()]
Param(
    [ValidateSet("x86", "x64")]
    [string]$Platform = "x86",

    [ValidateSet("Debug", "Release")]
    [string]$Configuration= "Debug",

    [string]$ArtifactName='drop'
)

$payloadDir = "HelixPayload\$Configuration\$Platform"


$repoDirectory = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "..\..\"
$nugetPackagesDir = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "packages"
 
# Create the payload directory. Remove it if it already exists.
If(test-path $payloadDir)
{
    Remove-Item $payloadDir -Recurse
}
New-Item -ItemType Directory -Force -Path $payloadDir


[xml]$pkgVerData = (Get-Content "$PSScriptRoot\packages.config")
$winuiHelixVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"Microsoft.Internal.WinUI.Helix`"]").version
$mitaVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"microsoft.windows.apps.test`"]").version
$taefVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"TAEF.Redist.Wlk`"]").version
$muxcustomBuildTasksVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"MUXCustomBuildTasks`"]").version
$netCoreAppVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"Microsoft.NETCore.App.Runtime.win-$Platform`"]").version

Write-Host "winuiHelixVer = $winuiHelixVer"
Write-Host "mitaVer = $mitaVer"
Write-Host "taefVer = $taefVer"
Write-Host "muxcustomBuildTasksVer = $muxcustomBuildTasksVer"
Write-Host "netCoreAppVer = $netCoreAppVer"

# Copy files from nuget packages
Copy-Item "$nugetPackagesDir\Microsoft.Internal.WinUI.Helix.$winuiHelixVer\scripts\test\*" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk.$taefVer\build\Binaries\$Platform\*" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk.$taefVer\build\Binaries\$Platform\CoreClr\*" $payloadDir
New-Item -ItemType Directory -Force -Path "$payloadDir\.netcoreapp3.1\"
Copy-Item "$nugetPackagesDir\Microsoft.NETCore.App.Runtime.win-$Platform.$netCoreAppVer\runtimes\win-$Platform\lib\netcoreapp3.1\*" "$payloadDir\.netcoreapp3.1\"
Copy-Item "$nugetPackagesDir\Microsoft.NETCore.App.Runtime.win-$Platform.$netCoreAppVer\runtimes\win-$Platform\native\*" "$payloadDir\.netcoreapp3.1\"
Copy-Item "$nugetPackagesDir\MUXCustomBuildTasks.$muxcustomBuildTasksVer\tools\$platform\WttLog.dll" $payloadDir

function Copy-If-Exists
{
    Param($source, $destinationDir)

    if (Test-Path $source)
    {
        Write-Host "Copy from '$source' to '$destinationDir'"
        Copy-Item -Force $source $destinationDir
    }
    else
    {
        Write-Host "'$source' does not exist."
    }
}

function Copy-Recursively-If-Exists
{
    Param($source, $destinationDir)

    if (Test-Path $source)
    {
        Write-Host "Copy from '$source' to '$destinationDir'"
        if (-not (Test-Path $destinationDir))
        {
            $ignore = New-Item -ItemType Directory $destinationDir
        }
        Copy-Item -Recurse -Force $source $destinationDir
    }
    else
    {
        Write-Host "'$source' does not exist."
    }
}

# Copy files from the 'drop' artifact dir
Copy-Item "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\*.dll" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\MUXExperimental.Test.dll" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\WebView2Loader.dll" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\MUXControlsTestApp_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\MUXControlsTestApp_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\MUXExperimentalTestApp_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\MUXExperimentalTestApp_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\AppThatUsesMUXIndirectly_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\AppThatUsesMUXIndirectly_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\IXMPTestApp_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\IXMPTestApp_Test\Dependencies\$Platform\*" $payloadDir

# Copy files from the 'NugetPkgTestsDrop' or 'FrameworkPkgTestsDrop' artifact dir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\MUXControls.ReleaseTest.dll" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\pgosweep.exe" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\vc_redist.*.exe" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\vcruntime140.dll" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\NugetPackageTestApp_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\NugetPackageTestApp_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\NugetPackageTestAppCX_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\NugetPackageTestAppCX_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\WpfApp_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\WpfApp_Test\Dependencies\$Platform\*" $payloadDir
Copy-Recursively-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\UnpackagedApps\WpfApp\" $payloadDir

# Copy files from the repo
New-Item -ItemType Directory -Force -Path "$payloadDir"
Copy-Item "build\helix\scripts\*" "$payloadDir"
Copy-Item "version.props" "$payloadDir"
