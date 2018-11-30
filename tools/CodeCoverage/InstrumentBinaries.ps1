param(
[Parameter(Mandatory=$true, HelpMessage="Build platform to be tested.")]
[string]$BuildPlatform,
[Parameter(Mandatory=$true, HelpMessage="Build configuration to be tested.")]
[string]$BuildConfiguration
)

if ($env:PROCESSOR_ARCHITECTURE -ilike "AMD64")
{
    $hostArchitecture = "x64"
}
else
{
    $hostArchitecture = $env:PROCESSOR_ARCHITECTURE
}

if ($hostArchitecture -ilike $BuildPlatform)
{
    $varSuiteString = $BuildPlatform
}
else
{
    $varSuiteString = "$($hostArchitecture)_$($BuildPlatform)"
}

Write-Host
Write-Host "Host architecture: $hostArchitecture"
Write-Host "Target architecture: $BuildPlatform"
Write-Host "Linking environment: $varSuiteString"

# GetFullPath discards any ".." directions and just returns the path itself.
$repoRoot = [System.IO.Path]::GetFullPath("$PSScriptRoot\..\..")

if ($env:BUILD_BINARIESDIRECTORY.Length -gt 0)
{
    $muxPath = "$env:BUILD_BINARIESDIRECTORY\$BuildConfiguration\$BuildPlatform\Microsoft.UI.Xaml"
    $appxPath = "$env:BUILD_BINARIESDIRECTORY\$BuildConfiguration\$BuildPlatform\MUXControlsTestApp\AppPackages\MUXControlsTestApp_Test"
}
else
{
    $muxPath = "$repoRoot\BuildOutput\$BuildConfiguration\$BuildPlatform\Microsoft.UI.Xaml"
    $appxPath = "$repoRoot\BuildOutput\$BuildConfiguration\$BuildPlatform\MUXControlsTestApp\AppPackages\MUXControlsTestApp_Test"
}

$muxObjPath = Get-Content "$muxPath\IntermediateDirectoryLocation.txt"
$appxObjPath = Get-Content "$appxPath\..\..\IntermediateDirectoryLocation.txt"

$magellanPath = "$env:USERPROFILE\.nuget\packages\microsoft.internal.magellan\5.4.170227001-pkges"

$vsBuildToolsPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build"
$vsMsvcToolsPath = "$(Get-Content "$muxPath\VCToolsInstallDirectoryLocation.txt")bin\Host$hostArchitecture\$BuildPlatform"

Write-Host
Write-Host "Beginning instrumentation for code coverage."
Write-Host
Write-Host "Microsoft.UI.Xaml.dll location: $muxPath"
Write-Host "MUXControlsTestApp.appx location: $appxPath"
Write-Host "Microsoft.UI.Xaml.dll intermediate objects path: $muxObjPath"
Write-Host "MUXControlsTestApp.appx intermediate objects path: $appxObjPath"
Write-Host "Magellan location: $magellanPath"
Write-Host "VS build tools location: $vsBuildToolsPath"
Write-Host "VS MSVC tools location: $vsMsvcToolsPath"

if (-not (Test-Path $magellanPath))
{
    Write-Host
    Write-Host "Magellan not detected. Installing..."
    Write-Host

    Invoke-Expression "$PSScriptRoot\..\NugetWrapper.cmd install Microsoft.Internal.Magellan -Source `"https://microsoft.pkgs.visualstudio.com/DefaultCollection/_packaging/Xes/nuget/v3/index.json`" -Version 5.4.170227001-pkges -Prerelease"
}

Write-Host
Write-Host "Saving the uninstrumented version of Microsoft.UI.Xaml.dll..."
Write-Host

if (-not (Test-Path "$muxPath\Microsoft.UI.Xaml.dll.orig"))
{
    Rename-Item "$muxPath\Microsoft.UI.Xaml.dll" "$muxPath\Microsoft.UI.Xaml.dll.orig"
}

Write-Host
Write-Host "Re-linking Microsoft.UI.Xaml.dll for code coverage..."
Write-Host

$linkCommand = "`"$vsMsvcToolsPath\link.exe`" /OUT:`"$muxPath\Microsoft.UI.Xaml.dll`" /MANIFEST:NO /PROFILE /LTCG:incremental /NXCOMPAT /PDB:`"$muxPath\Microsoft.UI.Xaml.pdb`" /DYNAMICBASE `"dxguid.lib`" `"WindowsApp.lib`" /DEF:`"$repoRoot\dev\dll\Microsoft.UI.Xaml.def`" /IMPLIB:`"$muxPath\Microsoft.UI.Xaml.lib`" /DEBUG:FULL /DLL /MACHINE:$BuildPlatform /WINMD /APPCONTAINER /OPT:REF /INCREMENTAL:NO /WINMDFILE:`"$muxPath\Microsoft.UI.Xaml.winmd`" /SUBSYSTEM:CONSOLE /OPT:ICF /ERRORREPORT:PROMPT /NOLOGO /TLBID:1 $muxObjPath\*.obj`""
Write-Host $linkCommand

$LinkBatchFileName = "$env:TEMP\LinkMUX.bat"
Out-File -FilePath "$LinkBatchFileName" -Encoding ascii -InputObject "pushd `"$vsBuildToolsPath`""
Out-File -FilePath "$LinkBatchFileName" -Encoding ascii -Append -InputObject "call vcvarsall.bat $varSuiteString"
Out-File -FilePath "$LinkBatchFileName" -Encoding ascii -Append -InputObject $linkCommand
Out-File -FilePath "$LinkBatchFileName" -Encoding ascii -Append -InputObject "popd"
Invoke-Expression "cmd /c `"$LinkBatchFileName`""
Remove-Item $LinkBatchFileName

Write-Host
Write-Host "Instrumenting Microsoft.UI.Xaml.dll..."
Write-Host

pushd "$magellanPath\tools\$BuildPlatform"
Invoke-Expression ".\BBCover.exe /UserMode /block /i `"$muxPath\Microsoft.UI.Xaml.dll`" /covsym Microsoft.UI.Xaml.dll.covsym /block /outputdir `"$muxPath\instr`" /o Microsoft.UI.Xaml.dll"
popd

Copy-Item "$muxPath\instr\Microsoft.UI.Xaml.dll" "$muxPath\Microsoft.UI.Xaml.dll"

Write-Host
Write-Host "Creating the appx..."
Write-Host

if (-not (Test-Path "$appxPath\MUXControlsTestApp.appx.orig"))
{
    Rename-Item "$appxPath\MUXControlsTestApp.appx" "$appxPath\MUXControlsTestApp.appx.orig"
}

$makeAppxCommand = "MakeAppx.exe pack /l /h sha256 /f $appxObjPath\package.map.txt /o /p $appxPath\MUXControlsTestApp.appx"
$signingCommand = "signtool.exe sign /f $repoRoot\build\MSTest.pfx /fd sha256 $appxPath\MUXControlsTestApp.appx"

Write-Host $makeAppxCommand
Write-Host $signingCommand

$MakeAppxBatchFileName = "$env:TEMP\MakeAppx.bat"
Out-File -FilePath "$MakeAppxBatchFileName" -Encoding ascii -InputObject "setlocal enabledelayedexpansion"
Out-File -FilePath "$MakeAppxBatchFileName" -Encoding ascii -Append -InputObject "pushd `"$vsBuildToolsPath`""
Out-File -FilePath "$MakeAppxBatchFileName" -Encoding ascii -Append -InputObject "call vcvarsall.bat $varSuiteString"
Out-File -FilePath "$MakeAppxBatchFileName" -Encoding ascii -Append -InputObject "popd"
Out-File -FilePath "$MakeAppxBatchFileName" -Encoding ascii -Append -InputObject "pushd `"!WindowsSdkDir!bin\!WindowsSDKVersion!`""
Out-File -FilePath "$MakeAppxBatchFileName" -Encoding ascii -Append -InputObject $makeAppxCommand
Out-File -FilePath "$MakeAppxBatchFileName" -Encoding ascii -Append -InputObject $signingCommand
Out-File -FilePath "$MakeAppxBatchFileName" -Encoding ascii -Append -InputObject "popd"
Invoke-Expression "cmd /c `"$MakeAppxBatchFileName`""
Remove-Item $MakeAppxBatchFileName

Write-Host
Write-Host "Putting back the original DLL..."
Write-Host

if (Test-Path "$muxPath\Microsoft.UI.Xaml.dll.orig")
{
	Remove-Item "$muxPath\Microsoft.UI.Xaml.dll"
	Rename-Item "$muxPath\Microsoft.UI.Xaml.dll.orig" "$muxPath\Microsoft.UI.Xaml.dll"
}

Write-Host
Write-Host "Instrumentation complete!"
Write-Host