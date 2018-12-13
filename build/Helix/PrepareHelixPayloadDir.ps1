$Flavor = "Debug"
$Platform = "x86"
$payloadDir = "HelixPayload"

$repoDirectory = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "..\..\"
#$nugetPackagesDir = "$env:USERPROFILE\.nuget\packages"
#$nugetPackagesDir = "$repoDirectory\packages\"
$nugetPackagesDir = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "packages"

# Create the payload directory. Remove it if it already exists.
If(test-path $payloadDir)
{
    Remove-Item $payloadDir -Recurse
}
New-Item -ItemType Directory -Force -Path $payloadDir

# Copy files from global nuget packages dir
copy "$nugetPackagesDir\microsoft.windows.apps.test.1.0.181203002\lib\netcoreapp2.1\*.dll" $payloadDir
copy "$nugetPackagesDir\taef.redist.wlk.10.31.180822002\build\Binaries\$platform\*" $payloadDir
copy "$nugetPackagesDir\taef.redist.wlk.10.31.180822002\build\Binaries\$platform\CoreClr\*" $payloadDir
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app.2.1.0\runtimes\win-$platform\lib\netcoreapp2.1\*" $payloadDir
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app.2.1.0\runtimes\win-$platform\native\*" $payloadDir
mkdir "$payloadDir\.NETCoreApp2.1\"
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app.2.1.0\runtimes\win-$platform\lib\netcoreapp2.1\*" "$payloadDir\.NETCoreApp2.1\"
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app.2.1.0\runtimes\win-$platform\native\*" "$payloadDir\.NETCoreApp2.1\"

# Copy files from the repo-level nuget packages dir
copy "$repoDirectory\packages\MUXCustomBuildTasks.1.0.38.tools\$platform\WttLog.dll" $payloadDir

# Copy files from the 'drop' artifact dir
copy "$repoDirectory\Artifacts\drop\$flavor\$platform\Test\MUXControls.Test.dll" $payloadDir
copy "$repoDirectory\Artifacts\drop\$flavor\$platform\AppxPackages\MUXControlsTestApp_Test\*" $payloadDir
copy "$repoDirectory\Artifacts\drop\$flavor\$platform\AppxPackages\MUXControlsTestApp_Test\Dependencies\$platform\*" $payloadDir

# Copy files from the repo
copy "build\helix\runtests.cmd" $payloadDir
copy "build\helix\ConvertWttLogToXUnit.ps1" $payloadDir