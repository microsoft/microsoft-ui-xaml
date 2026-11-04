param(
    [Parameter(Mandatory)]
    [string]$OutputDirectory,
    [ValidateSet('amd64chk', 'amd64fre', 'arm64chk', 'arm64fre')]
    [string]$Flavor = 'amd64chk',
    [ValidateSet('Local', 'Before', 'After')]
    [string]$Variant = 'Local',
    [string]$RuntimeDll,
    [ValidateNotNullOrEmpty()]
    [string]$RuntimeLabel = 'Local WinUI build'
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$configuration = if ($Flavor.EndsWith('chk')) { 'Debug' } else { 'Release' }
$architecture = if ($Flavor.StartsWith('amd64')) { 'x64' } else { 'arm64' }
$obj = Join-Path $repo "BuildOutput\obj\$Flavor\Samples\FullscreenPixelLab"
$recipePath = Join-Path $obj 'FullscreenPixelLab.build.appxrecipe'
$native = Join-Path $repo "BuildOutput\packaging\$configuration\runtimes\win10-$architecture\native"
$registrationsPath = Join-Path $repo "BuildOutput\obj\$Flavor\MergedWinMD\TestExtensions.xml"
if (-not $RuntimeDll) {
    $RuntimeDll = Join-Path $repo "BuildOutput\bin\$Flavor\Product\Microsoft.ui.xaml.dll"
}
foreach ($required in @($recipePath, $registrationsPath, $RuntimeDll, $native)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Missing build output: $required. Build the product and sample for $Flavor first."
    }
}

$output = [IO.Path]::GetFullPath($OutputDirectory)
if ((Test-Path -LiteralPath $output) -and (Get-ChildItem -LiteralPath $output -Force)) {
    throw "Output directory must be empty: $output"
}
New-Item -ItemType Directory -Force -Path $output | Out-Null

[xml]$recipe = Get-Content -LiteralPath $recipePath -Raw
$files = $recipe.SelectNodes('/*[local-name()="Project"]/*[local-name()="ItemGroup"]/*[local-name()="AppxPackagedFile" or local-name()="AppXManifest"]')
if ($files.Count -eq 0) {
    throw "No package payload was found in $recipePath"
}
foreach ($file in $files) {
    $destination = [IO.Path]::GetFullPath((Join-Path $output ([string]$file.PackagePath)))
    if (-not $destination.StartsWith($output.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Package path escapes the layout: $($file.PackagePath)"
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $file.Include -Destination $destination -Force
}

# Keep input, windowing, composition, controls, and their resources on the same local build.
Copy-Item -Path (Join-Path $native '*') -Destination $output -Recurse -Force
Copy-Item -LiteralPath $RuntimeDll -Destination (Join-Path $output 'Microsoft.ui.xaml.dll') -Force

$namespace = 'http://schemas.microsoft.com/appx/manifest/foundation/windows10'
[xml]$registrations = "<Extensions xmlns='$namespace'>" +
    (Get-Content -LiteralPath $registrationsPath -Raw) + '</Extensions>'
$manifestPath = Join-Path $output 'AppxManifest.xml'
[xml]$manifest = Get-Content -LiteralPath $manifestPath -Raw
if ($Variant -ne 'Local') {
    $manifest.Package.Identity.SetAttribute('Name',
        $manifest.Package.Identity.GetAttribute('Name') + ".$Variant")
    $displayName = [string]$manifest.Package.Properties.DisplayName + " - $($Variant.ToUpperInvariant())"
    $manifest.Package.Properties.DisplayName = $displayName
    $visualElements = $manifest.SelectSingleNode('//*[local-name()="VisualElements"]')
    $visualElements.SetAttribute('DisplayName', $displayName)
}
$namespaces = [Xml.XmlNamespaceManager]::new($manifest.NameTable)
$namespaces.AddNamespace('p', $namespace)
$extensions = $manifest.SelectSingleNode('/p:Package/p:Extensions', $namespaces)
if (-not $extensions) {
    $extensions = $manifest.CreateElement('Extensions', $namespace)
    [void]$manifest.DocumentElement.AppendChild($extensions)
}

# Include non-Controls namespaces, such as the XAML metadata providers.
foreach ($extension in $registrations.DocumentElement.ChildNodes) {
    $path = [string]$extension.InProcessServer.Path
    if (-not $path) {
        continue
    }
    $existing = $extensions.SelectNodes("p:Extension[p:InProcessServer/p:Path='$path']", $namespaces)
    foreach ($node in $existing) {
        [void]$extensions.RemoveChild($node)
    }
    [void]$extensions.AppendChild($manifest.ImportNode($extension, $true))
}
$manifest.Save($manifestPath)
$RuntimeLabel | Set-Content -LiteralPath (Join-Path $output 'runtime-label.txt') -Encoding utf8
[ordered]@{
    RuntimeLabel = $RuntimeLabel
    PackageIdentity = $manifest.Package.Identity.GetAttribute('Name')
    Flavor = $Flavor
    AppSha256 = (Get-FileHash -LiteralPath (Join-Path $output 'FullscreenPixelLab.dll')).Hash
    RuntimeSha256 = (Get-FileHash -LiteralPath (Join-Path $output 'Microsoft.ui.xaml.dll')).Hash
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'runtime-provenance.json') -Encoding utf8
Write-Output $output
