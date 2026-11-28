# Merges WinUI's activatable class registrations into a generated AppxManifest.xml.
# Needed because a self-contained msix built against the local WinUI nupkg doesn't get
# InProcessServer entries for Microsoft.UI.Xaml.dll / Microsoft.UI.Xaml.Controls.dll etc.
# Skips dlls that are already registered and classes that are already declared, so it's
# safe to run more than once.
param(
    [Parameter(Mandatory)] [string] $ManifestPath,
    [Parameter(Mandatory)] [string] $RegistrationsPath
)

$ns = 'http://schemas.microsoft.com/appx/manifest/foundation/windows10'
$extXPath = "//*[local-name()='Extension'][@Category='windows.activatableClass.inProcessServer']"

[xml]$manifest = Get-Content -Raw $ManifestPath
[xml]$registrations = Get-Content -Raw $RegistrationsPath

$existingPaths = @{}
$existingClasses = @{}
foreach ($ext in $manifest.SelectNodes($extXPath)) {
    $p = $ext.SelectSingleNode("*[local-name()='InProcessServer']/*[local-name()='Path']")
    if ($p) { $existingPaths[$p.InnerText.Trim()] = $true }
    foreach ($c in $ext.SelectNodes(".//*[local-name()='ActivatableClass']")) {
        $existingClasses[$c.GetAttribute('ActivatableClassId')] = $true
    }
}

$extensions = $manifest.DocumentElement.SelectSingleNode("*[local-name()='Extensions']")
if (-not $extensions) {
    $extensions = $manifest.CreateElement('Extensions', $ns)
    $manifest.DocumentElement.AppendChild($extensions) | Out-Null
}

$added = @()
foreach ($ext in $registrations.SelectNodes($extXPath)) {
    $path = $ext.SelectSingleNode("*[local-name()='InProcessServer']/*[local-name()='Path']").InnerText.Trim()
    if ($existingPaths.ContainsKey($path)) { continue }

    $imported = $manifest.ImportNode($ext, $true)
    foreach ($c in @($imported.SelectNodes(".//*[local-name()='ActivatableClass']"))) {
        $id = $c.GetAttribute('ActivatableClassId')
        if ($existingClasses.ContainsKey($id)) { $c.ParentNode.RemoveChild($c) | Out-Null }
        else { $existingClasses[$id] = $true }
    }
    if (-not $imported.SelectSingleNode(".//*[local-name()='ActivatableClass']")) { continue }

    $extensions.AppendChild($imported) | Out-Null
    $existingPaths[$path] = $true
    $added += $path
}

if ($added.Count -gt 0) {
    $manifest.Save($ManifestPath)
    Write-Host "AddLiftedClassRegistrations: added $($added -join ', ')"
} else {
    Write-Host "AddLiftedClassRegistrations: nothing to add"
}
