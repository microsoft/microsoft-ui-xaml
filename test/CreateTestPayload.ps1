# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding()]
param(
    [String]$BinSourceRoot = "",

    [ValidateSet("x86", "x64", "arm64", "arm64ec")]
    [string]$Platform = "$env:BUILDPLATFORM",

    [ValidateSet("chk", "fre")]
    [string]$Configuration = "$env:_BuildType",

    [string][ValidateSet("DevTestSuite", "ScenarioTestSuite", "PGO", IgnoreCase = $false)]
    $Mode = "DevTestSuite",

    [switch]$ShowPayload,

    [switch]$SkipSymbols,

    [switch]$Clean,

    [switch]$Quiet
)

if (-not $Platform)
{
    $Platform = "x86"
}

if (-not $Configuration)
{
    $Configuration = "chk"
}

$Arch = $Platform
if($Platform -eq "x64")
{
    $Arch = "amd64"
}

$repoRoot = Split-Path -Parent $PSScriptRoot

if(!$BinSourceRoot)
{
    $BinSourceRoot = "$repoRoot\BuildOutput\bin"
}

$outpath = "$repoRoot\TestPayload\$Platform$Configuration"

$binpath = "$BinSourceRoot\$Arch$Configuration"
$genxbfPath = "$BinSourceRoot\GenXBF\$Platform"

function Print-Config
{
    if ($Quiet) { return }
    $modes = [System.Collections.ArrayList]::new()
    [void]$modes.Add('DevTestSuite')
    [void]$modes.Add('ScenarioTestSuite')
    [void]$modes.Add('PGO')
    $modes.Remove($Mode)
    Write-Host "==================================================="
    Write-Host "outpath:                           $outpath"
    Write-Host "binpath:                           $binpath"
    Write-Host "genxbfPath:                        $genxbfPath"
    Write-Host "BinSourceRoot:                     $BinSourceRoot"
    Write-Host "Platform:                          $Platform"
    Write-Host "Configuration:                     $Configuration"
    Write-Host "Skip Symbols:                      $SkipSymbols"
    Write-Host "Show Payload:                      $ShowPayload"
    Write-Host "Mode set (manually set by -mode):  $Mode"
    Write-Host "Modes not set [X] :                $modes"
    Write-Host "==================================================="

}

Print-Config

if ($Clean -and (Test-Path $outpath))
{
    Get-ChildItem "$outpath\*" -Recurse -File | ForEach-Object { Remove-Item $_.FullName -Force }
    Get-ChildItem "$outpath\*" -Directory | ForEach-Object { Remove-Item $_.FullName -Force -Recurse }
}

$buildconfig = @{
    Platform = $Platform
    Configuration = $Configuration
}

if (-not (Test-Path $outpath))
{
    $ignore = New-Item -ItemType Directory $outpath
}

$buildconfig | ConvertTo-Json | Out-File (Join-Path $outpath "buildconfig.json")

$logFile = (Join-Path $outpath "CreateTestPayload.log")

if (Test-Path $logFile)
{
    Remove-Item $logFile
}

function Publish-Item {
    Param($source, $destinationDir, [switch]$IfExists = $false)

    if ((-not $IfExists) -or (Test-Path $source))
    {
        if (-not $Quiet) { Write-Host "Copy from '$source' to '$destinationDir'" }

        $sourceDirectory = [System.IO.Path]::GetDirectoryName($source).Trim()
        $sourceFileName = [System.IO.Path]::GetFileName($source).Trim()

        # If there was a file name specified, we don't want to copy recursively.
        # Otherwise, we do.
        if ($sourceFileName)
        {
            & robocopy "$sourceDirectory" "$destinationDir" "$sourceFileName" /XX /LOG+:$logFile | Out-Null
        }
        else
        {
            & robocopy "$sourceDirectory" "$destinationDir" "*" /E /XX /LOG+:$logFile | Out-Null
        }

        "$([Environment]::NewLine)Exit code: $global:LASTEXITCODE" | Out-File $logFile -Encoding utf8 -Append

        # robocopy returns a bunch of different exit codes to indicate exactly what happened during copy,
        # many of which indicate success.  Only exit codes 8 or above indicate failure.
        # This is kind of annoying, since most other things interpret a non-zero exit code to mean failure.
        # We'll set $LASTEXITCODE to 0 to avoid this if the exit code indicates success.
        #
        # Inquiring minds can find the full list of exit codes here:
        #
        # https://learn.microsoft.com/en-us/troubleshoot/windows-server/backup-and-storage/return-codes-used-robocopy-utility
        #
        if ($global:LASTEXITCODE -lt 8)
        {
            $global:LASTEXITCODE = 0
        }
        else
        {
            exit $global:LASTEXITCODE
        }
    }
    else
    {
        if (-not $Quiet) { Write-Host "Not copying '$source' to '$destinationDir' because it did not exist." }
    }
}

$redistPlatform = $Platform
if($Platform -eq "arm64ec")
{
    $redistPlatform = "x64"
}

if ($Mode -eq "DevTestSuite" -or $Mode -eq "ScenarioTestSuite")
{
    Publish-Item "$binpath\TAEF\*" "$outpath"
    Publish-Item "$binpath\TAEF\NetFx4.5\*" "$outpath"
    Publish-Item "$binpath\TestDependencies\localDotNet\*" "$outpath\localDotNet"
    Publish-Item "$binpath\TestDependencies\localDotNet\WinRT.Runtime.dll" "$outpath\"
    Publish-Item "$binpath\TestDependencies\WinUITest.cer" "$outpath"
    Publish-Item "$binpath\TestDependencies\*.cer" "$outpath\Test"
    Publish-Item "$binpath\TestDependencies\crt\vc_redist.$redistPlatform.exe" "$outpath"

    # Publish items from repo:
    Publish-Item "$repoRoot\Test\scripts\*" "$outpath"
    Publish-Item "$repoRoot\Helix\scripts\*" "$outpath"
    Publish-Item "$repoRoot\Helix\common\test\*" "$outpath"
    Publish-Item "$repoRoot\controls\tools\EnableMUXControlsTestAppManagedDebugging.*" "$outpath"
    
    Publish-Item "$binpath\TestDependencies\dotnet-windowsdesktop-runtime-installer.exe" "$outpath"
}

if ($Mode -eq "DevTestSuite")
{
    Publish-Item "$binpath\Product\*.dll" "$outpath"
    Publish-Item "$binpath\Product\*.dll" "$outpath\Test"
    Publish-Item "$binpath\Product\*.winmd" "$outpath\Test"
    Publish-Item "$binpath\Product\en-US\*.dll.mui" "$outpath\Test\en-US"
    Publish-Item "$binpath\Product\Microsoft.Ui.Xaml\*" "$outpath\Test\Microsoft.UI.Xaml"
    Publish-Item "$binpath\Test\" "$outpath\Test"
    Publish-Item "$binpath\TAEF\EtwProcessor.dll" "$outpath\Test"
    Publish-Item "$binpath\TAEF\TE.AppxUnitTestClient.dll" "$outpath\Test"
    Publish-Item "$binpath\TAEF\Microsoft.VisualStudio.TestPlatform.TestExecutor.WinRTCore.winmd" "$outpath\Test"
    Publish-Item "$binpath\TestDependencies\*.cmd" "$outpath"
    Publish-Item "$binpath\TestDependencies\*.dll" "$outpath"
    Publish-Item "$binpath\TestDependencies\*.exe" "$outpath"
    Publish-Item "$binpath\TestDependencies\*.winmd" "$outpath"
    Publish-Item "$binpath\TestDependencies\localDotNet\D3DCompiler_47_cor3.dll" "$outpath\Test"
    Publish-Item "$binpath\TestDependencies\localDotNet\Microsoft.Windows.SDK.NET.dll" "$outpath\Test"
    Publish-Item "$binpath\TestDependencies\crtforwarders\*.dll" "$outpath\Test"
    Publish-Item "$binpath\TestDependencies\AppX\*" "$outpath\Test"
    Publish-Item "$binpath\Test\XamlDiagnosticsTap.dll" "$outpath"
    Publish-Item "$binpath\Test\GenericXaml\generic.Xaml" "$outpath\Test\GenXbfValidation\"
    Publish-Item "$binpath\Test\GenericXaml\split\*" "$outpath\Test\GenXbfValidation\"
    Publish-Item "$genxbfPath\genxbf.dll" "$outpath\Test\GenXbfValidation\"
    Publish-Item "$binpath\product\Microsoft.WinUI.dll" "$outpath\Test\PrivateAPITests\"
    Publish-Item "$binpath\test\private\Microsoft.WinUI.dll" "$outpath\Test\"
    Publish-Item "$binpath\test\private\Microsoft.WinUI.dll" "$outpath\"

    # Since API tests run within the context of TE.ProcessHost.exe, we need to make sure we use the one modified to have WinUI 3 WinRT types in its manifest.
    Publish-Item "$binpath\Test\UnpackagedApps\MUXControlsTestApp\TE.ProcessHost.exe" "$outpath\"
    
    # MUXC WebView2 test infra gets CoreWebView2 SDK Version from this DLL
    Publish-Item "$binpath\Test\UnpackagedApps\MUXControlsTestApp\Microsoft.Web.WebView2.Core.dll" "$outpath\Test"

    if (!$SkipSymbols)
    {
        Publish-Item "$binpath\Symbols\Product\Microsoft.ui.xaml.pdb" "$outpath\Test"
        Publish-Item "$binpath\Symbols\Product\Microsoft.ui.xaml.controls.pdb" "$outpath\Test"
    }
    else
    {
        # The recursive copy of Test\ picks up PDBs from UnpackagedApps/. Remove them.
        Get-ChildItem "$outpath\Test" -Recurse -Filter *.pdb -ErrorAction SilentlyContinue | Remove-Item -Force
    }
}

if ($Mode -eq "ScenarioTestSuite")
{
    # This check for ValidateReunion/ WindowsAppSDK is temporary to limit test run only for Gallery against WAP. This shall no longer be needed
    # TODO: once pipeline is ready to run all sample apps, remove this check
    if (-not ($env:BUILD_DEFINITIONNAME -and ($env:BUILD_DEFINITIONNAME.Contains("ValidateReunion") -or $env:BUILD_DEFINITIONNAME.Contains("WindowsAppSDK"))))
    {
        Publish-Item "$binpath\Samples\WinUICsDesktopSampleApp_Test\*.msix*" "$outpath\Test\"
        Publish-Item "$binpath\Samples\WinUICppDesktopSampleApp_Test\*.msix*" "$outpath\Test\"
    }
    # The Windows App SDK MSIX tooling stamps platform/version/config into the test-package directory name,
    # e.g. WinUIGallery_x86_1.0.0.0_Debug_Test. Discover it dynamically rather than hard-coding the suffix.
    $galleryTestDir = Get-ChildItem -Path "$binpath\Samples" -Directory -Filter "WinUIGallery*Test" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $galleryTestDir)
    {
        throw "WinUIGallery test package directory not found under '$binpath\Samples' (expected a folder matching 'WinUIGallery*Test')."
    }
    Publish-Item "$($galleryTestDir.FullName)\WinUIGallery*.msix*" "$outpath\Test\"
    Get-ChildItem -Path "$outpath\Test" -Filter "WinUIGallery*.msix*" -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Extension -in @(".msix", ".msixbundle") } |
        ForEach-Object {
            $target = Join-Path $_.DirectoryName ("WinUIGallery$($_.Extension)")
            if ($_.FullName -ne $target) { Move-Item -Path $_.FullName -Destination $target -Force }
        }

    Publish-Item "$binpath\Test\MUXControls.Test.dll" "$outpath\Test\"
    Publish-Item "$binpath\Test\MUXTestInfra.dll" "$outpath\Test\"
    Publish-Item "$binpath\Test\Microsoft.Windows.Apps.Test.*.dll" "$outpath\Test\"
    Publish-Item "$binpath\Test\WinUIGalleryTestData.xml" "$outpath\Test\"

    # TODO: Remove below check
    if (-not ($env:BUILD_DEFINITIONNAME -and ($env:BUILD_DEFINITIONNAME.Contains("ValidateReunion") -or $env:BUILD_DEFINITIONNAME.Contains("WindowsAppSDK"))))
    {
        Publish-Item "$binpath\Samples\WinUICppDesktopSampleApp_Test\Dependencies\$redistPlatform\*.appx" "$outpath\Test\"
    }

    # TODO: Remove below check
    if ($env:BUILD_DEFINITIONNAME -and ($env:BUILD_DEFINITIONNAME.Contains("ValidateReunion") -or $env:BUILD_DEFINITIONNAME.Contains("WindowsAppSDK")))
    {
        Publish-Item "$($galleryTestDir.FullName)\Dependencies\$redistPlatform\*.msix" "$outpath\Test\"
    }

    Publish-Item "$repoRoot\Samples\TestAutomation\scripts\*" "$outpath"
}

if ($Mode -eq "PGO")
{
    # Publish items from build:
    Publish-Item "$binpath\Product\*.dll" "$outpath\Test"
    Publish-Item "$binpath\Product\*.winmd" "$outpath\Test"
    Publish-Item "$binpath\Product\en-US\*.dll.mui" "$outpath\Test\en-US"
    Publish-Item "$binpath\Product\Microsoft.Ui.Xaml\*" "$outpath\Test\Microsoft.UI.Xaml"
    Publish-Item "$binpath\TAEF\*" "$outpath"
    Publish-Item "$binpath\TAEF\NetFx4.5\*" "$outpath"
    Publish-Item "$binpath\Test\" "$outpath\Test"
    Publish-Item "$binpath\TestDependencies\WinUITest.cer" "$outpath"
    Publish-Item "$binpath\TestDependencies\crt\vc_redist.$redistPlatform.exe" "$outpath"
    Publish-Item "$binpath\TestDependencies\pgosweep.exe" "$outpath"

    # Publish items from repo:
    Publish-Item "$repoRoot\Helix\scripts\*" "$outpath"
    Publish-Item "$repoRoot\Helix\common\test\*" "$outpath"
}

if ($ShowPayload)
{
    Get-ChildItem *.* -Recurse
}

if (-not $Quiet) {
    Write-Host
    Write-Host "Log file: $logFile"
    Write-Host
}

Print-Config

if (-not $Quiet) {
    Write-Host
    Write-Host "Exit code: $global:LASTEXITCODE"
    Write-Host
}

exit $global:LASTEXITCODE
