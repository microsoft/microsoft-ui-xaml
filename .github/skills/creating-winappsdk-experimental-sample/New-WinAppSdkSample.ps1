#Requires -Version 5.1
<#
.SYNOPSIS
    Scaffolds, builds, and (optionally) launches a minimal self-contained WinUI 3
    desktop app that references a specific experimental Windows App SDK release.

.DESCRIPTION
    Windows App SDK ships an "experimental" channel roughly once a month
    (for example 2.3.2-experimentalA). This script produces a tiny, known-good
    WinUI 3 sample wired to that exact package version so the release can be
    smoke-tested end to end (restore -> build -> launch).

    It is resilient to two common environment problems:
      1. A broken / private-only NuGet setup - it writes an isolated nuget.config
         that uses only nuget.org and neutralizes NUGET_PLUGIN_PATHS.
      2. A machine with no Windows SDK targeting pack installed - the .NET SDK
         refuses to pull Microsoft.Windows.SDK.NET.Ref from nuget.org and only
         looks in the offline "library-packs" folder. When the pack is missing,
         the script downloads the nupkg and materializes a valid v3 *fallback
         package folder*, then registers it in nuget.config.

.PARAMETER Version
    Windows App SDK package version, e.g. '2.3.2-experimentalA'.
    Default: the newest '*experimental*' version published on nuget.org.

.PARAMETER OutputDir
    Folder to generate the app into. Default: $HOME\WinAppSdkSamples\WinUISample-<Version>.
    Keep this OUTSIDE any repo that uses Central Package Management / Directory.Build.props.

.PARAMETER Platform
    x64 (default), x86, or arm64.

.PARAMETER TargetFramework
    Default 'net9.0-windows10.0.22621.0'.

.PARAMETER WindowsSdkPackageVersion
    Windows SDK projection pack version. Default: newest stable pack matching the
    TFM's platform band (e.g. 10.0.22621.*) on nuget.org.

.PARAMETER Launch
    Launch the built app.

.PARAMETER NoBuild
    Only generate files; skip restore/build.

.EXAMPLE
    # Recommended: auto-detect and build/launch the latest monthly experimental release.
    .\New-WinAppSdkSample.ps1 -Launch

.EXAMPLE
    # Pin an explicit release id (any experimental version published on nuget.org):
    .\New-WinAppSdkSample.ps1 -Version <MAJOR.MINOR.PATCH-experimentalX> -Launch
#>
[CmdletBinding()]
param(
    [string]$Version,
    [string]$OutputDir,
    [ValidateSet('x64', 'x86', 'arm64')]
    [string]$Platform = 'x64',
    [string]$TargetFramework = 'net9.0-windows10.0.22621.0',
    [string]$TargetPlatformMinVersion = '10.0.17763.0',
    [string]$WindowsSdkPackageVersion,
    [switch]$Launch,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$AppName = 'WinAppSdkSample'
$feed = 'https://api.nuget.org/v3/index.json'
$flat = 'https://api.nuget.org/v3-flatcontainer'

function Get-Versions([string]$id) {
    (Invoke-RestMethod "$flat/$id/index.json" -UseBasicParsing).versions
}

function Select-LatestVersion([string[]]$versions) {
    # Order-independent, SemVer-aware "highest version" selection. Does NOT rely on
    # the order the feed returns. Sorts by the numeric base version first, then by
    # the prerelease label; a release with no prerelease outranks the same base with
    # one (SemVer rule), achieved by mapping "no prerelease" to a label that sorts last.
    ($versions | ForEach-Object {
            $dash = $_.IndexOf('-')
            if ($dash -ge 0) { $base = $_.Substring(0, $dash); $pre = $_.Substring($dash + 1) }
            else { $base = $_; $pre = [char]0xFFEF }  # sorts after any real prerelease label
            [pscustomobject]@{ Raw = $_; Base = [version]$base; Pre = $pre }
        } | Sort-Object -Property Base, Pre)[-1].Raw
}

# --- Resolve the Windows App SDK version --------------------------------------
if (-not $Version) {
    Write-Host 'Detecting latest experimental Windows App SDK version...'
    $exp = Get-Versions 'microsoft.windowsappsdk' | Where-Object { $_ -match '-experimental' }
    if (-not $exp) { throw 'No experimental Microsoft.WindowsAppSDK versions found on nuget.org.' }
    $Version = Select-LatestVersion $exp
}
Write-Host "Windows App SDK version: $Version"

# --- Derive the platform band and resolve the ref-pack version ----------------
if ($TargetFramework -match 'windows(\d+\.\d+\.\d+)\.\d+') { $band = $Matches[1] } else { $band = '10.0.22621' }
if (-not $WindowsSdkPackageVersion) {
    $stable = Get-Versions 'microsoft.windows.sdk.net.ref' | Where-Object { $_ -like "$band.*" -and $_ -notmatch 'preview' }
    if (-not $stable) { throw "No stable Microsoft.Windows.SDK.NET.Ref $band.* versions found on nuget.org." }
    $WindowsSdkPackageVersion = Select-LatestVersion $stable
}
Write-Host "Windows SDK projection pack: $WindowsSdkPackageVersion"

# --- Resolve paths ------------------------------------------------------------
if (-not $OutputDir) { $OutputDir = Join-Path $HOME "WinAppSdkSamples\WinUISample-$Version" }
$rid = @{ x64 = 'win-x64'; x86 = 'win-x86'; arm64 = 'win-arm64' }[$Platform]
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
Write-Host "Output directory: $OutputDir"

# --- Ensure the Windows SDK ref pack is resolvable ----------------------------
# The .NET SDK treats Microsoft.Windows.SDK.NET.Ref as a targeting pack and only
# looks in the offline library-packs fallback folder. If it is not installed,
# materialize a local v3 fallback folder from the nuget.org nupkg.
$fallbackRoot = Join-Path $OutputDir '.fallback'
$id = 'microsoft.windows.sdk.net.ref'
$installedPack = Test-Path (Join-Path $env:ProgramFiles "dotnet\packs\Microsoft.Windows.SDK.NET.Ref\$WindowsSdkPackageVersion")
$inLibPacks = Test-Path (Join-Path $env:ProgramFiles "dotnet\library-packs\$id.$WindowsSdkPackageVersion.nupkg")
$useFallback = -not ($installedPack -or $inLibPacks)

if ($useFallback) {
    $dir = Join-Path $fallbackRoot "$id\$WindowsSdkPackageVersion"
    $sha512File = Join-Path $dir "$id.$WindowsSdkPackageVersion.nupkg.sha512"
    if (-not (Test-Path $sha512File)) {
        Write-Host 'No Windows SDK targeting pack installed - building local fallback folder...'
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $nupkg = Join-Path $dir "$id.$WindowsSdkPackageVersion.nupkg"
        Invoke-WebRequest "$flat/$id/$WindowsSdkPackageVersion/$id.$WindowsSdkPackageVersion.nupkg" -OutFile $nupkg -UseBasicParsing
        $zip = "$nupkg.zip"; Copy-Item $nupkg $zip -Force
        Expand-Archive -Path $zip -DestinationPath $dir -Force; Remove-Item $zip -Force
        $sha = [System.Security.Cryptography.SHA512]::Create()
        $hashB64 = [Convert]::ToBase64String($sha.ComputeHash([System.IO.File]::ReadAllBytes($nupkg)))
        Set-Content -Path $sha512File -Value $hashB64 -NoNewline -Encoding ascii
        Set-Content -Path (Join-Path $dir '.nupkg.metadata') -Value ('{"version":2,"contentHash":"' + $hashB64 + '","source":"' + $feed + '"}') -NoNewline -Encoding ascii
    }
}

# --- File templates (placeholders are replaced below) -------------------------
$csproj = @'
<Project Sdk="Microsoft.NET.Sdk">

  <PropertyGroup>
    <OutputType>WinExe</OutputType>
    <TargetFramework>__TFM__</TargetFramework>
    <TargetPlatformMinVersion>__MINVER__</TargetPlatformMinVersion>
    <WindowsSdkPackageVersion>__WINSDKPKG__</WindowsSdkPackageVersion>
    <EnableWindowsTargeting>true</EnableWindowsTargeting>
    <RootNamespace>WinAppSdkSample</RootNamespace>
    <ApplicationManifest>app.manifest</ApplicationManifest>
    <Platforms>__PLATFORM__</Platforms>
    <RuntimeIdentifiers>__RID__</RuntimeIdentifiers>
    <UseWinUI>true</UseWinUI>
    <Nullable>enable</Nullable>
    <LangVersion>latest</LangVersion>
    <WindowsPackageType>None</WindowsPackageType>
    <WindowsAppSDKSelfContained>true</WindowsAppSDKSelfContained>
  </PropertyGroup>

  <ItemGroup>
    <PackageReference Include="Microsoft.WindowsAppSDK" Version="__VERSION__" />
  </ItemGroup>

</Project>
'@

$nugetFallback = @'
  <fallbackPackageFolders>
    <add key="local-winsdk-ref" value="__FALLBACK__" />
  </fallbackPackageFolders>
'@

$nugetConfig = @'
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <clear />
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" />
  </packageSources>
__FALLBACKBLOCK__</configuration>
'@

$manifest = @'
<?xml version="1.0" encoding="utf-8"?>
<assembly manifestVersion="1.0" xmlns="urn:schemas-microsoft-com:asm.v1">
  <assemblyIdentity version="1.0.0.0" name="WinAppSdkSample.app" />
  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}" />
    </application>
  </compatibility>
  <application xmlns="urn:schemas-microsoft-com:asm.v3">
    <windowsSettings>
      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/pm</dpiAware>
      <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2, PerMonitor</dpiAwareness>
    </windowsSettings>
  </application>
</assembly>
'@

$appXaml = @'
<?xml version="1.0" encoding="utf-8"?>
<Application
    x:Class="WinAppSdkSample.App"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
    <Application.Resources>
        <ResourceDictionary>
            <ResourceDictionary.MergedDictionaries>
                <XamlControlsResources xmlns="using:Microsoft.UI.Xaml.Controls" />
            </ResourceDictionary.MergedDictionaries>
        </ResourceDictionary>
    </Application.Resources>
</Application>
'@

$appCs = @'
using Microsoft.UI.Xaml;

namespace WinAppSdkSample;

public partial class App : Application
{
    private Window? _window;

    public App()
    {
        InitializeComponent();
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        _window = new MainWindow();
        _window.Activate();
    }
}
'@

$mainXaml = @'
<?xml version="1.0" encoding="utf-8"?>
<Window
    x:Class="WinAppSdkSample.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    Title="WinAppSDK Experimental Sample">
    <StackPanel HorizontalAlignment="Center" VerticalAlignment="Center" Spacing="12">
        <TextBlock
            Text="Hello from Windows App SDK __VERSION__!"
            FontSize="24"
            HorizontalAlignment="Center" />
        <Button
            x:Name="myButton"
            Click="myButton_Click"
            Content="Click Me"
            HorizontalAlignment="Center" />
    </StackPanel>
</Window>
'@

$mainCs = @'
using Microsoft.UI.Xaml;

namespace WinAppSdkSample;

public sealed partial class MainWindow : Window
{
    private int _count;

    public MainWindow()
    {
        InitializeComponent();
    }

    private void myButton_Click(object sender, RoutedEventArgs e)
    {
        _count++;
        myButton.Content = $"Clicked {_count} time(s)";
    }
}
'@

# --- Substitute placeholders and write files ----------------------------------
$csproj = $csproj.Replace('__TFM__', $TargetFramework).Replace('__MINVER__', $TargetPlatformMinVersion).Replace('__WINSDKPKG__', $WindowsSdkPackageVersion).Replace('__PLATFORM__', $Platform).Replace('__RID__', $rid).Replace('__VERSION__', $Version)
$fallbackBlock = if ($useFallback) { $nugetFallback.Replace('__FALLBACK__', $fallbackRoot) } else { '' }
$nugetConfig = $nugetConfig.Replace('__FALLBACKBLOCK__', $fallbackBlock)
$mainXaml = $mainXaml.Replace('__VERSION__', $Version)

Set-Content -Path (Join-Path $OutputDir "$AppName.csproj") -Value $csproj -Encoding utf8
Set-Content -Path (Join-Path $OutputDir 'nuget.config')     -Value $nugetConfig -Encoding utf8
Set-Content -Path (Join-Path $OutputDir 'app.manifest')     -Value $manifest -Encoding utf8
Set-Content -Path (Join-Path $OutputDir 'App.xaml')         -Value $appXaml -Encoding utf8
Set-Content -Path (Join-Path $OutputDir 'App.xaml.cs')      -Value $appCs -Encoding utf8
Set-Content -Path (Join-Path $OutputDir 'MainWindow.xaml')  -Value $mainXaml -Encoding utf8
Set-Content -Path (Join-Path $OutputDir 'MainWindow.xaml.cs') -Value $mainCs -Encoding utf8
Write-Host 'Generated project files.'

if ($NoBuild) { Write-Host 'Skipping build (-NoBuild).'; return }

# --- Restore + build ----------------------------------------------------------
# Neutralize a broken/absent NuGet credential provider for this restore only. Save the caller's
# value first and restore it in finally so later restores in the same session keep their provider.
$prevNugetPluginPaths = $env:NUGET_PLUGIN_PATHS
$env:NUGET_PLUGIN_PATHS = ''
Push-Location $OutputDir
try {
    Write-Host 'Building...'
    & dotnet build -c Debug -p:Platform=$Platform -p:EnableWindowsTargeting=true
    if ($LASTEXITCODE -ne 0) { throw "dotnet build failed with exit code $LASTEXITCODE." }
}
finally {
    # Assigning $null (when it was unset) removes the variable, faithfully restoring the prior state.
    $env:NUGET_PLUGIN_PATHS = $prevNugetPluginPaths
    Pop-Location
}

$exe = Join-Path $OutputDir "bin\$Platform\Debug\$TargetFramework\$AppName.exe"
if (-not (Test-Path $exe)) { throw "Build reported success but the exe was not found at: $exe" }
Write-Host "Build succeeded: $exe"

if ($Launch) {
    $p = Start-Process -FilePath $exe -PassThru
    Start-Sleep -Seconds 5
    if (Get-Process -Id $p.Id -ErrorAction SilentlyContinue) {
        Write-Host "Launched (PID $($p.Id)) - app is running."
    }
    else {
        throw "App exited immediately (exit code $($p.ExitCode))."
    }
}
