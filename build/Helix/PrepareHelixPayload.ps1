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
$muxTestInfraHelixVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"Microsoft.Internal.MUXTestInfra.Helix`"]").version
$mitaVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"microsoft.windows.apps.test`"]").version
$taefVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"TAEF.Redist.Wlk`"]").version
$muxcustomBuildTasksVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"MUXCustomBuildTasks`"]").version
$netCoreAppVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"runtime.win-$Platform.microsoft.netcore.app`"]").version

Write-Host "muxTestInfraHelixVer = $muxTestInfraHelixVer"
Write-Host "mitaVer = $mitaVer"
Write-Host "taefVer = $taefVer"
Write-Host "muxcustomBuildTasksVer = $muxcustomBuildTasksVer"
Write-Host "netCoreAppVer = $netCoreAppVer"

# Copy files from nuget packages
Copy-Item "$nugetPackagesDir\Microsoft.Internal.MUXTestInfra.Helix.$muxTestInfraHelixVer\scripts\test\*" $payloadDir
Copy-Item "$nugetPackagesDir\microsoft.windows.apps.test.$mitaVer\lib\netcoreapp2.1\*.dll" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk.$taefVer\build\Binaries\$Platform\*" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk.$taefVer\build\Binaries\$Platform\CoreClr\*" $payloadDir
New-Item -ItemType Directory -Force -Path "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app.$netCoreAppVer\runtimes\win-$Platform\lib\netcoreapp2.1\*" "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app.$netCoreAppVer\runtimes\win-$Platform\native\*" "$payloadDir\.NETCoreApp2.1\"
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

# Copy files from the 'drop' artifact dir
Copy-Item "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\MUXControls.Test.dll" $payloadDir
Copy-Item "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\MUXTestInfra.TAEF.dll" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\MUXControlsTestApp_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\MUXControlsTestApp_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\AppThatUsesMUXIndirectly_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\AppThatUsesMUXIndirectly_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\IXMPTestApp_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\IXMPTestApp_Test\Dependencies\$Platform\*" $payloadDir

# Copy files from the 'NugetPkgTestsDrop' or 'FrameworkPkgTestsDrop' artifact dir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\MUXControls.ReleaseTest.dll" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\pgosweep.exe" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\Test\vcruntime140.dll" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\NugetPackageTestApp_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\NugetPackageTestApp_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\NugetPackageTestAppCX_Test\*" $payloadDir
Copy-If-Exists "$repoDirectory\Artifacts\$ArtifactName\$Configuration\$Platform\AppxPackages\NugetPackageTestAppCX_Test\Dependencies\$Platform\*" $payloadDir

# Copy files from the repo
New-Item -ItemType Directory -Force -Path "$payloadDir"
# Copy-Item "build\helix\ConvertWttLogToXUnit.ps1" "$payloadDir"
# Copy-Item "build\helix\OutputFailedTestQuery.ps1" "$payloadDir"
# Copy-Item "build\helix\OutputSubResultsJsonFiles.ps1" "$payloadDir"
# Copy-Item "build\helix\HelixTestHelpers.cs" "$payloadDir"
# Copy-Item "build\helix\runtests.cmd" $payloadDir
Copy-Item "build\helix\*.ps1" "$payloadDir"
Copy-Item "version.props" "$payloadDir"
# Copy-Item "build\Helix\CopyVisualTreeVerificationFiles.ps1" "$payloadDir"
