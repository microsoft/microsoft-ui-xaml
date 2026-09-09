[CmdletBinding()]
param(
    [string]$WinUIPath = "C:\microsoft-ui-xaml"
)

$ErrorActionPreference = "Stop"

$configurationPath = Join-Path $PSScriptRoot "wuc-closure.json"
$configuration = Get-Content $configurationPath -Raw | ConvertFrom-Json
$externalBinaries = Get-Content (
    Join-Path $PSScriptRoot "..\externalbinaries.targets"
) -Raw

foreach ($module in $configuration.forbiddenRuntimeModules)
{
    if ($externalBinaries.IndexOf($module, [StringComparison]::OrdinalIgnoreCase) -lt 0)
    {
        throw "Payload filtering does not mention forbidden module '$module'."
    }
}
if ($externalBinaries.IndexOf(
    'Name="AssertSystemComponentPayload"',
    [StringComparison]::Ordinal
) -lt 0)
{
    throw "The forbidden payload build assertion is missing."
}

$package = Get-ChildItem (Join-Path $WinUIPath "packages") `
    -Directory `
    -Filter "Microsoft.WindowsAppSDK.InteractiveExperiences.*" |
    Where-Object {
        Test-Path (Join-Path $_.FullName "runtimes-framework\package.appxfragment")
    } |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if ($null -eq $package)
{
    throw "A restored InteractiveExperiences package was not found."
}

$filteredManifest = Join-Path $env:TEMP "system-component-filtered-ixp.xml"
try
{
    & (Join-Path $PSScriptRoot "Filter-IxpAppxFragment.ps1") `
        -InputPath (Join-Path $package.FullName "runtimes-framework\package.appxfragment") `
        -OutputPath $filteredManifest `
        -ConfigurationPath $configurationPath

    [xml]$manifest = Get-Content $filteredManifest
    $namespaceManager = [Xml.XmlNamespaceManager]::new($manifest.NameTable)
    $namespaceManager.AddNamespace(
        "appx",
        "http://schemas.microsoft.com/appx/manifest/foundation/windows10"
    )
    $servers = @(
        $manifest.SelectNodes("//appx:InProcessServer", $namespaceManager) |
            ForEach-Object { [string]$_.Path }
    )

    foreach ($module in $configuration.forbiddenRuntimeModules)
    {
        if ($module -in $servers)
        {
            throw "Forbidden activation server '$module' remains in the filtered manifest."
        }
    }
    if ("Microsoft.UI.Input.dll" -notin $servers)
    {
        throw "The filtered manifest removed an unrelated retained IXP server."
    }
}
finally
{
    Remove-Item $filteredManifest -ErrorAction SilentlyContinue
}

$winuiPackage = Join-Path (
    Join-Path $WinUIPath "PackageStore"
) "Microsoft.WindowsAppSDK.WinUI.3.0.0-dev.nupkg"
if (Test-Path $winuiPackage -PathType Leaf)
{
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($winuiPackage)
    try
    {
        $forbiddenEntries = @(
            $archive.Entries |
                Where-Object {
                    [IO.Path]::GetFileName($_.FullName) -in
                        $configuration.forbiddenRuntimeModules
                } |
                ForEach-Object { $_.FullName }
        )
        if ($forbiddenEntries.Count -ne 0)
        {
            throw "Forbidden WinUI package payload remains: $($forbiddenEntries -join ', ')"
        }
    }
    finally
    {
        $archive.Dispose()
    }
}

Write-Output "System component payload tests passed."
