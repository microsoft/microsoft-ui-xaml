[CmdLetBinding()]
Param(
    [string]$BuildOutputDir,
    [string]$PublishDir,
    [string]$Platform,
    [string]$Configuration
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
        Copy-Item -Force $source $destinationDir
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
PublishFile -IfExists $BuildOutputDir\$Configuration\AnyCPU\MUXControls.Test.TAEF\MUXControls.Test.dll $FullPublishDir\Test\
PublishFile -IfExists $BuildOutputDir\$Configuration\AnyCPU\MUXControls.ReleaseTest.TAEF\MuxControls.ReleaseTest.dll $FullPublishDir\Test\
PublishFile -IfExists $FullBuildOutput\FrameworkPackage\*.* $FullPublishDir\FrameworkPackage

# Publish pdbs:
$symbolsOutputDir = "$($FullPublishDir)\Symbols\"
PublishFile -IfExists $FullBuildOutput\Microsoft.UI.Xaml\Microsoft.UI.Xaml.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\IXMPTestApp\IXMPTestApp.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXTestUtilities\MUXTestUtilities.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXControls.Test\MUXControls.Test.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXControlsTestApp\MUXControlsTestApp.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXControlsTestAppForIslands\MUXControlsTestAppForIslands.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\MUXControlsTestAppWPF\MUXControlsTestAppWPF.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\TestAppCX\TestAppCX.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\NugetPackageTestApp\NugetPackageTestApp.pdb $symbolsOutputDir
PublishFile -IfExists $FullBuildOutput\NugetPackageTestAppCX\NugetPackageTestAppCX.pdb $symbolsOutputDir