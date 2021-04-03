[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [String]$Flavor = "Debug",

    [ValidateSet("x86", "x64")]
    [String]$Platform = "x86",

    [switch]$NoBuild,
    [String]$BuildId,
    
    [ValidateSet("DevTest", "NugetPkgTests", "FrameworkPkgTests")]
    [string]$TestSuite = "DevTest"
)

if(!$BuildId -and $TestSuite -eq "FrameworkPkgTests")
{
    Write-Error "-TestSuite='FrameworkPkgTests' is only valid when using a -BuildId. Use -TestSuite='NugetPkgTests' for testing locally."
    exit 1
}

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

function New-TemporaryDirectory {
    $parent = [System.IO.Path]::GetTempPath()
    $name = [System.IO.Path]::GetRandomFileName()
    New-Item -ItemType Directory -Path (Join-Path $parent $name)
}


# Clean up artifacts and HelixPayload directories:
$artifactsDir = "$PSScriptRoot\Artifacts"
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

if($BuildId)
{
    $artifactName = "drop"
    if($TestSuite -eq "NugetPkgTests")
    {
        $artifactName = "NugetPkgTestsDrop"
    }
    elseif($TestSuite -eq "FrameworkPkgTests")
    {
        $artifactName = "FrameworkPkgTestsDrop"
    }

    $artifactTargetDir = "$artifactsDir\$artifactName"
    if(Test-Path $artifactTargetDir)
    {
        Remove-Item $artifactTargetDir -Force -Recurse
    }

    $tempDir = New-TemporaryDirectory
    $tempDirPath = $tempDir.FullName

    $downloadFileName = $artifactName + ".zip"
    $downloadFilePath = Join-Path $tempDirPath $downloadFileName 

    $dropData = Invoke-RestMethod -Uri "https://dev.azure.com/ms/microsoft-ui-xaml/_apis/build/builds/$buildId/artifacts?artifactName=$artifactName&api-version=4.1" -Method Get

    # Invoke-WebRequest is orders of magnitude slower when the progress indicator is being displayed. So temporarily disable it.
    $ProgressPreferenceOld = $ProgressPreference
    $ProgressPreference = "SilentlyContinue"
    try
    {    
        Write-Host "Downloading '$downloadFileName'. Please wait, this will take a few moments..."
        Invoke-WebRequest -Uri $dropData.resource.downloadUrl -OutFile $downloadFilePath
    }
    finally
    {
        $ProgressPreference = $ProgressPreferenceOld    
    }

    Write-Host "Done!"
    Write-Host "Downloaded file to $downloadFilePath"

    Write-Host "Extracting files to $artifactsDir"
    Expand-Archive -Path $downloadFilePath -DestinationPath $artifactsDir

    & .\Tools\NugetWrapper.cmd restore build\Helix\packages.config -PackagesDirectory build\Helix\packages
    & .\build\Helix\PrepareHelixPayload.ps1 -Platform $Platform -Configuration $Flavor -ArtifactName $artifactName 

    Write-Verbose "Removing temp dir '$tempDirPath'"
    Remove-Item -Force -Recurse $tempDirPath

    Write-Host ""
    Write-Host "Test binaries dir created in $helixpayloadDir"
}
else
{
    Write-Host "Creating test binaries dir from local build."

    # Determine if we need to build the test binaries:
    $muxDllFile = Get-Item "$PSScriptRoot\BuildOutput\$Flavor\$Platform\Microsoft.UI.Xaml\Microsoft.UI.Xaml.dll"

    $shouldBuild = $false;
    $buildCmd = "";
    if($TestSuite -eq "DevTest")
    {   
        $shouldBuild = $shouldBuild -Or (DoesTaefAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "MUXControlsTestApp" -Platform $Platform -Flavor $Flavor)
        $shouldBuild = $shouldBuild -Or (DoesTaefAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "IXMPTestApp" -Platform $Platform -Flavor $Flavor)
        if($shouldBuild)
        {
            $buildCmd = "$PSScriptRoot\build.cmd $($Platform.ToLower()) $($Flavor.ToLower()) /target test\MUXControlsTestApp\MUXControlsTestApp_TAEF:Publish /target test\IXMPTestApp\IXMPTestApp_TAEF:Publish /target test\MUXControlsTestAppWPF\MUXControlsTestAppWPFPackage:Publish"
        }
    }
    else
    {
        $shouldBuild = $shouldBuild -Or (DoesAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "NugetPackageTestApp" -Platform $Platform -Flavor $Flavor -AppXPath "NugetPackageTestApp\AppPackages\NugetPackageTestApp_Test\NugetPackageTestApp.appx" -ExePath "NugetPackageTestApp\NugetPackageTestApp.exe" -ProjectPath "MUXControlsReleaseTest\NugetPackageTestApp\NugetPackageTestApp.csproj")
        $shouldBuild = $shouldBuild -Or (DoesAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "NugetPackageTestAppCX" -Platform $Platform -Flavor $Flavor -AppXPath "NugetPackageTestAppCX\AppPackages\NugetPackageTestAppCX_Test\NugetPackageTestAppCX.appx" -ExePath "NugetPackageTestAppCX\NugetPackageTestAppCX.exe" -ProjectPath "MUXControlsReleaseTest\NugetPackageTestAppCX\NugetPackageTestAppCX.csproj")
        $shouldBuild = $shouldBuild -Or (DoesAppXNeedBuild -MuxDllFile $muxDllFile -ProjectName "WpfApp" -Platform $Platform -Flavor $Flavor -AppXPath "WpfApp.Package\AppPackages\WpfApp_Test\WpfApp.msix" -ExePath "WpfApp\WpfApp.exe" -ProjectPath "MUXControlsReleaseTest\XamlIslandsTestApp\WpfApp\WpfApp.Package.wapproj")
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


    & .\build\CopyFilesToStagingDir.ps1 -BuildOutputDir "$PSScriptRoot\BuildOutput\" -PublishDir $artifactsDropDir -Platform $Platform -Configuration $Flavor -PublishApps
    & .\Tools\NugetWrapper.cmd restore build\Helix\packages.config -PackagesDirectory build\Helix\packages
    & .\build\Helix\PrepareHelixPayload.ps1 -Platform $Platform -Configuration $Flavor

    Write-Host ""
    Write-Host "Test binaries dir created in $helixpayloadDir"
}
