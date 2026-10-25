param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$VerifyTargetSelection
)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path (Split-Path (Split-Path $PSScriptRoot)))
$tool = Join-Path $root 'tools\WinUIProfiler'
$app = Join-Path $tool 'WinUIProfiler.App\bin\x64\Release\net10.0-windows10.0.26100.0\win-x64'
$manifest = Join-Path $app 'AppxManifest.xml'
$assets = Join-Path $app '__WinUIProfilerConnectionFixture'
$resources = Join-Path $app 'resources.pri'
$name = 'WinUIProfiler.App'
$previousAumid = $env:WINUI_PROFILER_TEST_AUMID
$registered = $false
$createdManifest = $false
$createdAssets = $false
$createdResources = $false
if (!(Test-Path (Join-Path $app 'WinUIProfiler.App.exe'))) { throw 'Build the Release x64 App first.' }
if ((Test-Path $manifest) -or (Test-Path $assets) -or (Test-Path $resources) -or (Get-AppxPackage -Name $name)) {
    throw 'Packaged fixture paths/identity already exist; refusing to alter another registration or files.'
}
try {
    Copy-Item -LiteralPath (Join-Path $app 'WinUIProfiler.App.pri') -Destination $resources
    $createdResources = $true
    New-Item -ItemType Directory -Path $assets | Out-Null
    $createdAssets = $true
    Add-Type -AssemblyName System.Drawing
    foreach ($size in @(44, 50, 150)) {
        $bitmap = [System.Drawing.Bitmap]::new($size, $size)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try { $graphics.Clear([System.Drawing.Color]::SteelBlue) }
            finally { $graphics.Dispose() }
            $bitmap.Save((Join-Path $assets "Logo$size.png"), [System.Drawing.Imaging.ImageFormat]::Png)
        }
        finally { $bitmap.Dispose() }
    }
    @'
<?xml version="1.0" encoding="utf-8"?>
<Package xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
         xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
         xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
         IgnorableNamespaces="uap rescap">
  <Identity Name="WinUIProfiler.App" Publisher="CN=WinUIProfilerTest" Version="1.0.0.0" ProcessorArchitecture="x64" />
  <Properties>
    <DisplayName>WinUI Profiler Connection Fixture</DisplayName>
    <PublisherDisplayName>WinUI Profiler Test</PublisherDisplayName>
    <Logo>__WinUIProfilerConnectionFixture\Logo50.png</Logo>
  </Properties>
  <Resources><Resource Language="en-US" /></Resources>
  <Dependencies><TargetDeviceFamily Name="Windows.Desktop" MinVersion="10.0.22000.0" MaxVersionTested="10.0.26100.0" /></Dependencies>
  <Applications>
    <Application Id="App" Executable="WinUIProfiler.App.exe" EntryPoint="Windows.FullTrustApplication">
      <uap:VisualElements DisplayName="WinUI Profiler Connection Fixture" Description="Owned profiler activation test"
        BackgroundColor="transparent" Square150x150Logo="__WinUIProfilerConnectionFixture\Logo150.png"
        Square44x44Logo="__WinUIProfilerConnectionFixture\Logo44.png" />
    </Application>
  </Applications>
  <Capabilities><rescap:Capability Name="runFullTrust" /></Capabilities>
</Package>
'@ | Set-Content -LiteralPath $manifest -Encoding utf8
    $createdManifest = $true
    # Packaged WinRT activation uses manifest registrations, not the unpackaged
    # executable's registration-free activation context.
    $registration = Join-Path $tool 'WinUIProfiler.App\obj\x64\Release\net10.0-windows10.0.26100.0\win-x64\Manifests\app.manifest'
    [xml]$nativeManifest = Get-Content -LiteralPath $registration -Raw
    [xml]$packageManifest = Get-Content -LiteralPath $manifest -Raw
    $ns = 'http://schemas.microsoft.com/appx/manifest/foundation/windows10'
    $extensions = $packageManifest.CreateElement('Extensions', $ns)
    foreach ($file in $nativeManifest.SelectNodes("//*[local-name()='file']")) {
        $classes = $file.SelectNodes("*[local-name()='activatableClass']")
        if (!$classes.Count) { continue }
        $extension = $packageManifest.CreateElement('Extension', $ns)
        $extension.SetAttribute('Category', 'windows.activatableClass.inProcessServer')
        $server = $packageManifest.CreateElement('InProcessServer', $ns)
        $path = $packageManifest.CreateElement('Path', $ns)
        $path.InnerText = $file.GetAttribute('name')
        [void]$server.AppendChild($path)
        foreach ($class in $classes) {
            $entry = $packageManifest.CreateElement('ActivatableClass', $ns)
            $entry.SetAttribute('ActivatableClassId', $class.GetAttribute('name'))
            $entry.SetAttribute('ThreadingModel', $class.GetAttribute('threadingModel'))
            [void]$server.AppendChild($entry)
        }
        [void]$extension.AppendChild($server)
        [void]$extensions.AppendChild($extension)
    }
    [void]$packageManifest.Package.AppendChild($extensions)
    $packageManifest.Save($manifest)
    Add-AppxPackage -Register $manifest -ForceApplicationShutdown
    $registered = $true
    $package = Get-AppxPackage -Name $name
    if (!$package) { throw 'Fixture registration did not produce a package identity.' }
    $env:WINUI_PROFILER_TEST_AUMID = "$($package.PackageFamilyName)!App"
    dotnet test (Join-Path $tool 'WinUIProfiler.IntegrationTests\WinUIProfiler.IntegrationTests.csproj') `
        -c $Configuration --no-restore --filter 'Category=PackagedConnection' --logger 'console;verbosity=normal'
    if ($LASTEXITCODE -ne 0) { throw "Packaged connection tests failed: $LASTEXITCODE" }
    if ($VerifyTargetSelection) {
        & (Join-Path $PSScriptRoot 'Test-TargetSelectionUi.ps1') `
            -Executable (Join-Path $app 'WinUIProfiler.App.exe') -ApplicationAumid $env:WINUI_PROFILER_TEST_AUMID
    }
}
finally {
    $env:WINUI_PROFILER_TEST_AUMID = $previousAumid
    if ($registered) {
        $package = Get-AppxPackage -Name $name
        if ($package) { Remove-AppxPackage -Package $package.PackageFullName }
    }
    if ($createdManifest) { Remove-Item -LiteralPath $manifest }
    if ($createdResources) { Remove-Item -LiteralPath $resources }
    if ($createdAssets) {
        foreach ($size in @(44, 50, 150)) { Remove-Item -LiteralPath (Join-Path $assets "Logo$size.png") }
        Remove-Item -LiteralPath $assets
    }
}
