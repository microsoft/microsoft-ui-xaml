$Flavor = "Debug"
$Platform = "x86"

$Release = 0

$deviceDir = "HelixPayload"

$muxDllFile = Get-Item "Artifacts\drop\$Flavor\$Platform\Microsoft.UI.Xaml\Microsoft.UI.Xaml.dll"

#
# Deploy TAEF, .NETCoreApp, and Microsoft.Windows.Apps.Test. These are the ones that we grabbed off nuget
# so that they match the ones we linked against/packaged in our appx.
#

$nugetPackagesDir = "$env:USERPROFILE\.nuget\packages"
$binplaceDir = "HelixPayload"

If(test-path $binplaceDir)
{
    Remove-Item $binplaceDir -Recurse
}
New-Item -ItemType Directory -Force -Path $binplaceDir

copy "$nugetPackagesDir\microsoft.windows.apps.test\1.0.181203002\lib\netcoreapp2.1\*.dll" $binplaceDir
copy "$nugetPackagesDir\taef.redist.wlk\10.31.180822002\build\Binaries\$platform\*" $binplaceDir
copy "$nugetPackagesDir\taef.redist.wlk\10.31.180822002\build\Binaries\$platform\CoreClr\*" $binplaceDir
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app\2.1.0\runtimes\win-$platform\lib\netcoreapp2.1\*" $binplaceDir
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app\2.1.0\runtimes\win-$platform\native\*" $binplaceDir

mkdir "$binplaceDir\.NETCoreApp2.1\"
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app\2.1.0\runtimes\win-$platform\lib\netcoreapp2.1\*" "$binplaceDir\.NETCoreApp2.1\"
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app\2.1.0\runtimes\win-$platform\native\*" "$binplaceDir\.NETCoreApp2.1\"


$repoDirectory = Split-Path -Parent $script:MyInvocation.MyCommand.Path 
$repoDirectory = Join-Path $repoDirectory "..\..\"

copy "$repoDirectory\packages\MUXCustomBuildTasks.1.0.38-test\build\WttLog.dll" $binplaceDir

$testDllOutputDir = Join-Path $repoDirectory "Artifacts\drop\$flavor\$platform\Test"
$testAppOutputDir = Join-Path $repoDirectory "Artifacts\drop\$flavor\$platform\AppxPackages\MUXControlsTestApp_Test"

$appxPath = $testAppOutputDir
$dependenciesPath = Join-Path $testAppOutputDir "Dependencies\$platform"

copy (Join-Path $testDllOutputDir "MUXControls.Test.dll") $deviceDir
copy "$appxPath\*" $deviceDir
copy "$dependenciesPath\*" $deviceDir



copy "build\helix\runtests.cmd" $deviceDir
copy "build\helix\ConvertWttLogToXUnit.ps1" $deviceDir