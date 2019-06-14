[CmdletBinding()]
param(
    [switch]$NoBuild,
    [switch]$Release,
    [String]$Flavor = "Debug",
    [String]$Platform = "x86"
)

function DoesTaefAppXNeedBuild
{
    param(
        [System.IO.FileSystemInfo]$MuxDllFile,
        [string]$ProjectName,
        [string]$Platform,
        [string]$Flavor
    )

    $projectFileName = "$($ProjectName).TAEF"

    return DoesAppXNeedBuild -MuxDllFile $MuxDllFile -ProjectName $ProjectName -Platform $Platform -Flavor $Flavor -AppXPath "$projectFileName\AppPackages\$($ProjectName)_Test\$ProjectName.appx" -ExePath "$projectFileName\$ProjectName.exe"
}

function DoesAppXNeedBuild
{
    param(
        [System.IO.FileSystemInfo]$MuxDllFile,
        [string]$AppXPath,
        [string]$ExePath,
        [string]$ProjectName,
        [string]$BuildTarget,
        [string]$Platform,
        [string]$Flavor
    )

    $appxFullPath = "$PSScriptRoot\BuildOutput\$Flavor\$Platform\$AppXPath"
    $testAppxFile = Get-Item $appxFullPath -ErrorAction Ignore
    $testExeFile = Get-Item "$PSScriptRoot\BuildOutput\$Flavor\$Platform\$ExePath" -ErrorAction Ignore

    if((!$testAppxFile) -or (!$testExeFile) -or ($testExeFile.LastWriteTime -gt $testAppxFile.LastWriteTime) -or ($muxDllFile.LastWriteTime -gt $testAppxFile.LastWriteTime))
    {
        if ($testAppxFile)
        {
            Write-Host "$testAppxFile LastWriteTime = $($testAppxFile.LastWriteTime)"
        }
        else
        {
            Write-Host "No appx at $appxFullPath"
        }
        if ($testExeFile)
        {
            Write-Host "$testExeFile LastWriteTime = $($testExeFile.LastWriteTime)"
        }
        Write-Host "$muxDllFile LastWriteTime = $($muxDllFile.LastWriteTime)"

        return $true
    }
    else
    {
        return $false
    }
}


# Clean up artifacts and HelixPayload directories:
$artifactsDropDir = "$PSScriptRoot\Artifacts\drop"
$helixpayloadDir = "$PSScriptRoot\HelixPayload\$Flavor\$Platform"
if(Test-Path $artifactsDropDir)
{
    Remove-Item $artifactsDropDir -Force -Recurse
}

if(Test-Path $helixpayloadDir)
{
    Remove-Item $helixpayloadDir -Force -Recurse
}

# Determine if we need to build the test binaries:
$muxDllFile = Get-Item "$PSScriptRoot\BuildOutput\$Flavor\$Platform\Microsoft.UI.Xaml\Microsoft.UI.Xaml.dll"

$shouldBuild = $false;
$buildCmd = "";
if(!$Release)
{   
    $shouldBuild = $shouldBuild -Or (DoesTaefAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "MUXControlsTestApp" -Platform $Platform -Flavor $Flavor)
    $shouldBuild = $shouldBuild -Or (DoesTaefAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "IXMPTestApp" -Platform $Platform -Flavor $Flavor)
    $shouldBuild = $shouldBuild -Or (DoesAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "MUXControlsTestAppWPFPackage" -Platform $Platform -Flavor $Flavor -AppXPath "MUXControlsTestAppWPFPackage\AppPackages\MUXControlsTestAppWPFPackage_Test\MUXControlsTestAppWPFPackage.appx" -ExePath "MUXControlsTestAppWPF\MUXControlsTestAppWPF.exe" -ProjectPath "MUXControlsTestAppWPFPackage\MUXControlsTestAppWPFPackage.wapproj")
    if($shouldBuild)
    {
        $buildCmd = "$PSScriptRoot\build.cmd $($Platform.ToLower()) $($Flavor.ToLower()) /target test\MUXControlsTestApp\MUXControlsTestApp_TAEF:Publish /target test\IXMPTestApp\IXMPTestApp_TAEF:Publish /target test\MUXControlsTestAppWPF\MUXControlsTestAppWPFPackage:Publish"
    }
}
else
{
    $shouldBuild = $shouldBuild -Or (DoesAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "NugetPackageTestApp" -Platform $Platform -Flavor $Flavor -AppXPath "NugetPackageTestApp\AppPackages\NugetPackageTestApp_Test\NugetPackageTestApp.appx" -ExePath "NugetPackageTestApp\NugetPackageTestApp.exe" -ProjectPath "MUXControlsReleaseTest\NugetPackageTestApp\NugetPackageTestApp.csproj")
    $shouldBuild = $shouldBuild -Or (DoesAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "NugetPackageTestAppCX" -Platform $Platform -Flavor $Flavor -AppXPath "NugetPackageTestAppCX\AppPackages\NugetPackageTestAppCX_Test\NugetPackageTestAppCX.appx" -ExePath "NugetPackageTestAppCX\NugetPackageTestAppCX.exe" -ProjectPath "MUXControlsReleaseTest\NugetPackageTestAppCX\NugetPackageTestAppCX.csproj")
    if($shouldBuild)
    {
        $buildCmd = "$PSScriptRoot\build.cmd $($Platform.ToLower()) $($Flavor.ToLower()) /project D:\microsoft-ui-xaml\test\MUXControlsReleaseTest\MUXControlsReleaseTest.sln"
    }
}

if($shouldBuild)
{
    if(!$NoBuild)
    {
        Write-Host $buildCmd
        Invoke-Expression $buildCmd
    }
    else
    {
        Write-Warning "Test binaries are out of date, but -NoBuild flag was specified so skipping build"
    }
}


& .\build\CopyFilesToStagingDir.ps1 -BuildOutputDir "$PSScriptRoot\BuildOutput\" -PublishDir $artifactsDropDir -Platform $Platform -Configuration $Flavor -PublishAppxFiles
& .\Tools\NugetWrapper.cmd restore build\Helix\packages.config -PackagesDirectory build\Helix\packages
& .\build\Helix\PrepareHelixPayload.ps1 -Platform $Platform -Configuration $Flavor

Write-Host "Test binaries dir created in $helixpayloadDir"