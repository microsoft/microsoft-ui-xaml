[CmdLetBinding()]
Param(
    [string]$BuildOutputDir,
    [string]$PublishDir,
    [string]$Platform,
    [string]$Configuration,
    [switch]$PublishAppxFiles=$false
)

$FullBuildOutput = "$($BuildOutputDir)\$($Configuration)\$($Platform)"
$FullPublishDir = "$($PublishDir)\$($Configuration)\$($Platform)"

if (!(Test-Path $FullPublishDir)) { mkdir $FullPublishDir }


function PublishFile {
    Param($source, $destinationDir, [switch]$IfExists = $false)

    if ((-not $IfExists) -or (Test-Path $source))
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
        Write-Host "Not copying '$source' to $destinationDir because it did not exist"
    }
}

PublishFile -IfExists $FullBuildOutput\Microsoft.UI.Xaml\Microsoft.UI.Xaml.dll $FullPublishDir\Microsoft.UI.Xaml\
PublishFile -IfExists $FullBuildOutput\Microsoft.UI.Xaml\Microsoft.UI.Xaml.pri $FullPublishDir\Microsoft.UI.Xaml\
PublishFile -IfExists $FullBuildOutput\Microsoft.UI.Xaml\sdk\Microsoft.UI.Xaml.winmd $FullPublishDir\Microsoft.UI.Xaml\sdk\
PublishFile -IfExists $FullBuildOutput\Microsoft.UI.Xaml\Generic.xaml $FullPublishDir\Microsoft.UI.Xaml\
PublishFile -IfExists $FullBuildOutput\Microsoft.UI.Xaml.Design\Microsoft.UI.Xaml.Design.dll $FullPublishDir\Microsoft.UI.Xaml.Design\
PublishFile -IfExists $BuildOutputDir\$Configuration\AnyCPU\Microsoft.UI.Xaml.FrameworkPackagePRI\Microsoft.UI.Xaml.pri $FullPublishDir\Microsoft.UI.Xaml.FrameworkPackagePRI\
PublishFile -IfExists $BuildOutputDir\$Configuration\AnyCPU\MUXControls.Test.TAEF\MUXControls.Test.dll $FullPublishDir\Test\
PublishFile -IfExists $BuildOutputDir\$Configuration\AnyCPU\MUXControls.Test.TAEF\MUXTestInfra.TAEF.dll $FullPublishDir\Test\
PublishFile -IfExists $BuildOutputDir\$Configuration\AnyCPU\MUXControls.ReleaseTest.TAEF\MUXControls.ReleaseTest.dll $FullPublishDir\Test\
PublishFile -IfExists $BuildOutputDir\$Configuration\AnyCPU\MUXExperimental.Test.TAEF\MUXExperimental.Test.dll $FullPublishDir\Test\

PublishFile -IfExists $FullBuildOutput\Microsoft.Experimental.UI.Xaml\Microsoft.Experimental.UI.Xaml.dll $FullPublishDir\Microsoft.Experimental.UI.Xaml\
PublishFile -IfExists $FullBuildOutput\Microsoft.Experimental.UI.Xaml\Microsoft.Experimental.UI.Xaml.pri $FullPublishDir\Microsoft.Experimental.UI.Xaml\
PublishFile -IfExists $FullBuildOutput\Microsoft.Experimental.UI.Xaml\sdk\Microsoft.Experimental.UI.Xaml.winmd $FullPublishDir\Microsoft.Experimental.UI.Xaml\sdk\
PublishFile -IfExists $FullBuildOutput\Microsoft.Experimental.UI.Xaml\Generic.xaml $FullPublishDir\Microsoft.Experimental.UI.Xaml\

# pgosweep and vcruntime are required to run pgo instrumented test run. They are placed from the
# cx test app instead of releasetest.dll since these are architecture specific and the ReleaseTest assembly is AnyCPU.
PublishFile -IfExists $FullBuildOutput\NugetPackageTestAppCX\pgosweep.exe $FullPublishDir\Test\
PublishFile -IfExists $FullBuildOutput\NugetPackageTestAppCX\vcruntime140.dll $FullPublishDir\Test\

PublishFile -IfExists $FullBuildOutput\FrameworkPackage\*.* $FullPublishDir\FrameworkPackage

if ($PublishAppxFiles)
{
    $AppxPackagesDir = "$FullPublishDir\AppxPackages"

    PublishFile -IfExists $FullBuildOutput\MUXControlsTestApp.TAEF\AppPackages\MUXControlsTestApp_Test\ $AppxPackagesDir
    PublishFile -IfExists $FullBuildOutput\IXMPTestApp.TAEF\AppPackages\IXMPTestApp_Test\ $AppxPackagesDir
    PublishFile -IfExists $FullBuildOutput\MUXControlsTestAppWPFPackage\AppPackages\MUXControlsTestAppWPFPackage_Test\ $AppxPackagesDir

    PublishFile -IfExists $FullBuildOutput\NugetPackageTestApp\AppPackages\NugetPackageTestApp_Test\ $AppxPackagesDir
    PublishFile -IfExists $FullBuildOutput\NugetPackageTestAppCX\AppPackages\NugetPackageTestAppCX_Test\ $AppxPackagesDir
    PublishFile -IfExists $FullBuildOutput\AppThatUsesMUXIndirectly\AppPackages\AppThatUsesMUXIndirectly_Test\ $AppxPackagesDir
    PublishFile -IfExists $FullBuildOutput\WpfApp.Package\AppPackages\WpfApp_Test\ $AppxPackagesDir
    PublishFile -IfExists $FullBuildOutput\MUXExperimentalTestApp\AppPackages\MUXExperimentalTestApp_Test\ $AppxPackagesDir
}

$UnpackagedAppsDir = "$FullPublishDir\UnpackagedApps"

PublishFile -IfExists $FullBuildOutput\WpfApp\ $UnpackagedAppsDir

# Publish pdbs:
$symbolsOutputDir = "$($FullPublishDir)\Symbols\"
PublishFile -IfExists $FullBuildOutput\Microsoft.UI.Xaml\Microsoft.UI.Xaml.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\IXMPTestApp.TAEF\IXMPTestApp.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXControls.Test\MUXControls.Test.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXControlsTestApp.TAEF\MUXControlsTestApp.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXControlsTestAppForIslands\MUXControlsTestAppForIslands.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXControlsTestAppWPF\MUXControlsTestAppWPF.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\TestAppCX\TestAppCX.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\NugetPackageTestApp\NugetPackageTestApp.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\AppThatUsesMUXIndirectly\AppThatUsesMUXIndirectly.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\NugetPackageTestAppCX\NugetPackageTestAppCX.pdb $symbolsOutputDir
PublishFile -IfExists $BuildOutputDir\$Configuration\AnyCPU\MUXExperimental.Test.TAEF\MUXExperimental.Test.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXExperimentalTestApp\MUXExperimentalTestApp.pdb $symbolsOutputDir