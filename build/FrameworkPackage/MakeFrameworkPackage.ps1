[CmdLetBinding()]
Param(
    [string]$inputDirectory,
    [string]$outputDirectory,
    [string]$Platform,
    [string]$Configuration,
    [string]$Publisher = "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US",
    [string]$VersionOverride,
    [int]$Subversion = "000",
    [string]$builddate_yymm,
    [string]$builddate_dd,
    [string]$WindowsSdkBinDir,
    [string]$BasePackageName,
    [string]$PackageNameSuffix)

Import-Module $PSScriptRoot\..\..\tools\Utils.psm1 -DisableNameChecking

function Copy-IntoNewDirectory {
    Param($source, $destinationDir, [switch]$IfExists = $false)

    if ((-not $IfExists) -or (Test-Path $source))
    {
        Write-Verbose "Copy from '$source' to '$destinationDir'"
        if (-not (Test-Path $destinationDir))
        {
            $ignore = New-Item -ItemType Directory $destinationDir
        }
        Copy-Item -Recurse -Force $source $destinationDir
    }
}

pushd $PSScriptRoot

$fullOutputPath = [IO.Path]::GetFullPath($outputDirectory)
Write-Host "MakeFrameworkPackage: $fullOutputPath" -ForegroundColor Magenta 
# Write-Output "Path: $env:PATH"

mkdir -Force $fullOutputPath\PackageContents | Out-Null
mkdir -Force $fullOutputPath\Resources | Out-Null

Copy-IntoNewDirectory FrameworkPackageContents\* $fullOutputPath\PackageContents

Copy-IntoNewDirectory PriConfig\* $fullOutputPath

$KitsRoot10 = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots" -Name KitsRoot10).KitsRoot10
$WindowsSdkBinDir = Join-Path $KitsRoot10 "bin\x86"
# If this path is not found, construct one using Program Files (x86). VS2019 hosted agents seem to have the wrong path populated in the registry.
if (-not (Test-Path $WindowsSdkBinDir))
{
    Write-Host "Not found: $WindowsSdkBinDir"
    $KitsRoot10 =  "${env:ProgramFiles(x86)}\Windows Kits\10"
    $WindowsSdkBinDir = Join-Path $KitsRoot10 "bin\x86"
}

$ActivatableTypes = ""

# Copy over and add to the manifest file list the .dll, .winmd for the inputs. Also copy the .pri
# but don't list it because it will be merged together.

Write-Output "Input: $inputDirectory"
$inputBaseFileName = "Microsoft.UI.Xaml"
$inputBasePath = $inputDirectory

Copy-IntoNewDirectory "$inputBasePath\$inputBaseFileName.dll" $fullOutputPath\PackageContents
Copy-IntoNewDirectory "$inputBasePath\$inputBaseFileName.pri" $fullOutputPath\Resources
Copy-IntoNewDirectory "$inputBasePath\sdk\$inputBaseFileName.winmd" $fullOutputPath\PackageContents

Write-Verbose "Copying $inputBasePath\Themes"
Copy-IntoNewDirectory -IfExists $inputBasePath\Themes $fullOutputPath\PackageContents\Microsoft.UI.Xaml

#Find the latest available sdk
function Get-SDK-References-Path
{
    [xml]$sdkPropsContent = Get-Content $PSScriptRoot\..\..\sdkversion.props
    $sdkVersions = $sdkPropsContent.SelectNodes("//*[contains(local-name(), 'SDKVersion')]") | Sort-Object -Property '#text' -Descending 
    foreach ($version in $sdkVersions)
    {
        $sdkReferencesPath = Join-Path (Join-Path $kitsRoot10 "References\") ($version.'#text')
        Write-Verbose "Checking $sdkReferencesPath ..."
        if (Test-Path $sdkReferencesPath)
        {
            Write-Verbose "Found $sdkReferencesPath"
            return $sdkReferencesPath
        }
    }
    return ''
}

$sdkReferencesPath = Get-SDK-References-Path
$WindowsSdkBinDir = Join-Path $sdkReferencesPath.Replace("References", "bin") "x64"
Write-Verbose "SdkReferencesPath = $sdkReferencesPath"
Write-Verbose "WindowsSdkBinDir = $WindowsSdkBinDir"
$foundationWinmdPath = Get-ChildItem -Recurse $sdkReferencesPath"\Windows.Foundation.FoundationContract" -Filter "Windows.Foundation.FoundationContract.winmd" | Select-Object -ExpandProperty FullName
$universalWinmdPath = Get-ChildItem -Recurse $sdkReferencesPath"\Windows.Foundation.UniversalApiContract" -Filter "Windows.Foundation.UniversalApiContract.winmd" | Select-Object -ExpandProperty FullName
$refrenceWinmds = $foundationWinmdPath + ";" + $universalWinmdPath
Write-Verbose "Calling Get-ActivatableTypes with '$inputBasePath\sdk\$inputBaseFileName.winmd' '$refrenceWinmds'"
$classes = Get-ActivatableTypes $inputBasePath\sdk\$inputBaseFileName.winmd  $refrenceWinmds  | Sort-Object -Property FullName
Write-Host $classes.Length Types found.
@"
"$inputBaseFileName.dll" "$inputBaseFileName.dll"
"$inputBaseFileName.winmd" "$inputBaseFileName.winmd" 
"@ | Out-File -Append -Encoding "UTF8" $fullOutputPath\PackageContents\FrameworkPackageFiles.txt

    $ActivatableTypes += @"
    <Extension Category="windows.activatableClass.inProcessServer">
      <InProcessServer>
        <Path>$inputBaseFileName.dll</Path>

"@

ForEach ($class in $classes)
{
    $className = $class.fullname
    #Write-Host "Activatable type : $className"
    $ActivatableTypes += "        <ActivatableClass ActivatableClassId=`"$className`" ThreadingModel=`"both`" />`r`n"
}

$ActivatableTypes += @"
      </InProcessServer>
    </Extension>

"@

Copy-IntoNewDirectory ..\..\dev\Materials\Acrylic\Assets\NoiseAsset_256x256_PNG.png $fullOutputPath\Assets

$customPropsFile = "$PSScriptRoot\..\..\version.props"
Write-Verbose "Looking in $customPropsFile"

if (-not (Test-Path $customPropsFile))
{
    Write-Error "Expected '$customPropsFile' to exist"
    Exit 1
}
[xml]$customProps = (Get-Content $customPropsFile)
$versionMajor = $customProps.GetElementsByTagName("MUXVersionMajor").'#text'
$versionMinor = $customProps.GetElementsByTagName("MUXVersionMinor").'#text'

Write-Verbose "CustomProps = $customProps, VersionMajor = '$versionMajor', VersionMinor = '$versionMinor'"

if ((!$versionMajor) -or (!$versionMinor))
{
    Write-Error "Expected MUXVersionMajor and MUXVersionMinor tags to be in version.props file"
    Exit 1
}

if (-not $PackageNameSuffix)
{
    $PackageNameSuffix = "$($versionMajor).$($versionMinor)"
}

# Calculate the version the same as our nuget package.

if ($VersionOverride)
{
    $version = $VersionOverride
}
else
{    
    $pstZone = [System.TimeZoneInfo]::FindSystemTimeZoneById("Pacific Standard Time")
    $pstTime = [System.TimeZoneInfo]::ConvertTimeFromUtc((Get-Date).ToUniversalTime(), $pstZone)
    # Split version into yyMM.dd because appx versions can't be greater than 65535
    # Also store submission requires that the revision field stay 0, so our scheme is:
    #    A.ByyMM.ddNNN
    # compared to nuget version which is:
    #    A.B.yyMMddNN
    # We could consider dropping the B part out of the minor section if we need to because
    # it's part of the package name, but we'll try to keep it in for now. We also omit the "B"
    # part when it's 0 because the appx version doesn't allow the minor version to have a 0 prefix.
    # Also note that we trim 0s off the beginning of the day because appx versions can't start with 0.
    if ($versionMinor -eq 0)
    {
        $versionMinor = ""
    }

    if (-not $builddate_yymm)
    {
        $builddate_yymm = ($pstTime).ToString("yyMM")
    }
    if (-not $builddate_dd)
    {
        $builddate_dd = ($pstTime).ToString("dd")
    }

    # Pad subversion up to 3 digits
    $subversionPadded = $subversion.ToString("000")

    $version = "${versionMajor}.${versionMinor}" + $builddate_yymm + "." + $builddate_dd.TrimStart("0") + "$subversionPadded.0"

    Write-Verbose "Version = $version"
}

Write-Host "Version = $version" -ForegroundColor Green

$versionPropsFile = 
@"
<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <MicrosoftUIXamlAppxVersion>$version</MicrosoftUIXamlAppxVersion>
  </PropertyGroup>
</Project>
"@

Set-Content -Value $versionPropsFile $fullOutputPath\MicrosoftUIXamlVersion.props


# Also copy in some loose files 

$PackageName = $BasePackageName
if ($PackageNameSuffix)
{
    $PackageName += ".$PackageNameSuffix"
}
if ($Configuration -ilike "debug")
{
    # NOTE: Having the package be named differently between debug and release is possible (and is what other frameworks do)
    # but in general our customers don't care to be running debug versions of MUX bits when their app is debug. So by default
    # we use the "release" appx in their Debug builds. For testing purposes a Debug build produces an appx of the same name
    # as the release version so that you can in-place upgrade to a debug version on your own machine for debugging MUX.
    #$PackageName += ".Debug"
}

# AppxManifest needs some pieces generated per-flavor.
$manifestContents = Get-Content $fullOutputPath\PackageContents\AppxManifest.xml
$manifestContents = $manifestContents.Replace('$(PackageName)', "$PackageName")
$manifestContents = $manifestContents.Replace('$(Platform)', "$Platform")
$manifestContents = $manifestContents.Replace('$(Publisher)', "$Publisher")
$manifestContents = $manifestContents.Replace('$(ActivatableTypes)', "$ActivatableTypes")
$manifestContents = $manifestContents.Replace('$(Version)', "$Version")
Set-Content -Value $manifestContents $fullOutputPath\PackageContents\AppxManifest.xml

$manifestContents = $manifestContents.Replace("$PackageName", "Microsoft.UI.Xaml.CBS")
$manifestContents = $manifestContents.Replace('FrameworkPackageDetector', "CBSPackageDetector")
Set-Content -Value $manifestContents $fullOutputPath\CBSAppxManifest.xml

# Call GetFullPath to clean up the path -- makepri is very picky about double slashes in the path.
$priConfigPath = [IO.Path]::GetFullPath("$fullOutputPath\priconfig.xml")
$priOutputPath = [IO.Path]::GetFullPath("$fullOutputPath\resources.pri")
$priCBSOutputPath = [IO.Path]::GetFullPath("$fullOutputPath\CBSresources.pri")
$noiseAssetPath = [IO.Path]::GetFullPath("$fullOutputPath\Assets\NoiseAsset_256x256_PNG.png")
$resourceContents = [IO.Path]::GetFullPath("$fullOutputPath\Resources")
$pfxPath = [IO.Path]::GetFullPath("..\MSTest.pfx")

pushd $fullOutputPath\PackageContents

$xbfFilesPath = "$fullOutputPath\PackageContents\Microsoft.UI.Xaml"
if (($Configuration -ilike "debug") -and (Test-Path $xbfFilesPath))
{
    Get-ChildItem -Recurse $xbfFilesPath -File | Resolve-Path -Relative | %{ $_.Replace(".\", "") } |
%{ @"
"$_" "$_"
"@ } | Out-File -Append -Encoding "UTF8" $fullOutputPath\PackageContents\FrameworkPackageFiles.txt
}

# Append output path of resources.pri as well
@"
"$priOutputPath" "resources.pri"
"$noiseAssetPath" "Microsoft.UI.Xaml\Assets\NoiseAsset_256x256_PNG.png"
"@ | Out-File -Append -Encoding "UTF8" $fullOutputPath\PackageContents\FrameworkPackageFiles.txt

$makepriNew = "`"" + (Join-Path $WindowsSdkBinDir "makepri.exe") + "`" new /pr $fullOutputPath /cf $priConfigPath /of $priOutputPath /in $PackageName /o"
Write-Host $makepriNew
cmd /c $makepriNew
if ($LastExitCode -ne 0) { Exit 1 }

$makepriNew = "`"" + (Join-Path $WindowsSdkBinDir "makepri.exe") + "`" new /pr $fullOutputPath /cf $priConfigPath /of $priCBSOutputPath /in Microsoft.UI.Xaml.CBS /o"
Write-Host $makepriNew
cmd /c $makepriNew
if ($LastExitCode -ne 0) { Exit 1 }

$outputAppxFileFullPath = Join-Path $fullOutputPath "$PackageName.appx"
$outputAppxFileFullPath = [IO.Path]::GetFullPath($outputAppxFileFullPath)

$makeappx = "`"" + (Join-Path $WindowsSdkBinDir "makeappx.exe") + "`" pack /o /p $outputAppxFileFullPath /f FrameworkPackageFiles.txt"
Write-Host $makeappx
cmd /c $makeappx
if ($LastExitCode -ne 0) { Exit 1 }

if ($env:TFS_ToolsDirectory -and ($env:BUILD_DEFINITIONNAME -match "release") -and $env:UseSimpleSign)
{
    # From MakeAppxBundle in the XES tools
    $signToolPath = $env:TFS_ToolsDirectory + "\bin\SimpleSign.exe"
    if (![System.IO.File]::Exists($signToolPath))
    {
       $signToolPath = "SimpleSign.exe"
    }

    # From here: https://osgwiki.com/wiki/Package_ES_Appx_Bundle#Code_sign_Appx_Bundle
    $signCert = "136020001"

    $signtool = "`"$signToolPath`" -i:`"$outputAppxFileFullPath`" -c:$signCert -s:`"CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US`""
}
else
{
    $signtool = "`"" + (Join-Path $WindowsSdkBinDir "signtool.exe") + "`" sign /a /v /fd SHA256 /f $pfxPath $outputAppxFileFullPath"
}
Write-Host $signtool
cmd /c $signtool
if ($LastExitCode -ne 0) { Exit 1 }

popd
popd
