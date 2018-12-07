$Flavor = "Debug"
$Platform = "x86"

$Release = 0

$deviceDir = "HelixPayload"

$muxDllFile = Get-Item "BuildOutput\$Flavor\$Platform\Microsoft.UI.Xaml\Microsoft.UI.Xaml.dll"

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

# If(!(test-path $binplaceDir))
# {
#     New-Item -ItemType Directory -Force -Path $binplaceDir
# }

copy "$nugetPackagesDir\microsoft.windows.apps.test\1.0.181203002\lib\netcoreapp2.1\*.dll" $binplaceDir
copy "$nugetPackagesDir\taef.redist.wlk\10.31.180822002\build\Binaries\$platform\*" $binplaceDir
copy "$nugetPackagesDir\taef.redist.wlk\10.31.180822002\build\Binaries\$platform\CoreClr\*" $binplaceDir
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app\2.1.0\runtimes\win-$platform\lib\netcoreapp2.1\*" $binplaceDir
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app\2.1.0\runtimes\win-$platform\native\*" $binplaceDir

mkdir "$binplaceDir\.NETCoreApp2.1\"
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app\2.1.0\runtimes\win-$platform\lib\netcoreapp2.1\*" "$binplaceDir\.NETCoreApp2.1\"
copy "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app\2.1.0\runtimes\win-$platform\native\*" "$binplaceDir\.NETCoreApp2.1\"




# Always copy over the test files.

$repoDirectory = Split-Path -Parent $script:MyInvocation.MyCommand.Path
if(!$Release)
{
    $testDllOutputDir = Join-Path $repoDirectory "BuildOutput\$flavor\AnyCPU\MUXControls.Test.TAEF"
    $testAppOutputDir = Join-Path $repoDirectory "BuildOutput\$flavor\$platform\MUXControlsTestApp.TAEF"
    $ixmpAppOutputDir = Join-Path $repoDirectory "BuildOutput\$flavor\$platform\IXMPTestApp.TAEF"
    $testAppWPFXamlIslandsOutputDir = Join-Path $repoDirectory "BuildOutput\$flavor\$platform\MUXControlsTestAppWPFPackage"

    $appxPath = Join-Path $testAppOutputDir "AppPackages\MUXControlsTestApp_Test"
    $dependenciesPath = Join-Path $testAppOutputDir "AppPackages\MUXControlsTestApp_Test\Dependencies\$platform"

    copy (Join-Path $testDllOutputDir "MUXControls.Test.dll") $deviceDir
    copy "$appxPath\*" $deviceDir
    copy "$dependenciesPath\MUXControlsTestApp*" $deviceDir
    copy (Join-Path $ixmpAppOutputDir "AppPackages\IXMPTestApp_Test\IXMPTestApp*") $deviceDir
    copy (Join-Path $ixmpAppOutputDir "AppPackages\IXMPTestApp_Test\Dependencies\$platform\*") $deviceDir

    copy (Join-Path $testAppWPFXamlIslandsOutputDir "AppPackages\MUXControlsTestAppWPFPackage_Test\MUXControlsTestAppWPFPackage*") $deviceDir
    # app package dependencies are missing. Following up on email
    #$putDOutput += putd (Join-Path $testAppWPFXamlIslandsOutputDir "AppPackages\MUXControlsTestAppWPFPackage_Test\Dependencies\$platform\*") $deviceDir
}
else
{
    $testDllOutputDir = Join-Path $repoDirectory "BuildOutput\$flavor\AnyCPU\MUXControls.Test.TAEF"
    $releaseTestDllOutputDir = Join-Path $repoDirectory "BuildOutput\$flavor\AnyCPU\MUXControls.ReleaseTest.TAEF"
    $testAppOutputDir = Join-Path $repoDirectory "BuildOutput\$flavor\$platform\NugetPackageTestApp"
    $testAppCxOutputDir = Join-Path $repoDirectory "BuildOutput\$flavor\$platform\NugetPackageTestAppCX"

    copy (Join-Path $testDllOutputDir "MUXControls.Test.dll") $deviceDir
    copy (Join-Path $releaseTestDllOutputDir "MUXControls.ReleaseTest.dll") $deviceDir
    copy (Join-Path $testAppOutputDir "AppPackages\NugetPackageTestApp_Test\NugetPackageTestApp*") $deviceDir
    copy (Join-Path $testAppOutputDir "AppPackages\NugetPackageTestApp_Test\Dependencies\$platform\*") $deviceDir
    copy (Join-Path $testAppCxOutputDir "AppPackages\NugetPackageTestAppCX_Test\NugetPackageTestAppCX*") $deviceDir
    copy (Join-Path $testAppCxOutputDir "AppPackages\NugetPackageTestAppCX_Test\Dependencies\$platform\*") $deviceDir
}

copy runtests.cmd $deviceDir