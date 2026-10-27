# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..\..").Path
$objects = "$root\BuildOutput\obj\amd64chk"
$bin = "$root\BuildOutput\bin\amd64chk"
$output = "$bin\Test\UnpackagedApps\WebView2ActivationTestApp"
$generated = "$objects\CompilerTests\Basic\CppWinRT\Simple\generated"
$sdkManifest = "$objects\CompilerTests\Basic\CppWinRT\Simple\Manifests\WindowsAppSDK.manifest"
$native = "$root\BuildOutput\packaging\Debug\runtimes\win10-x64\native"
$runtime = "$bin\Test\UnpackagedApps\MUXControlsTestApp\Microsoft.WindowsAppRuntime.dll"

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw 'Run this script through initrun.ps1 in the amd64chk build environment.'
}
foreach ($required in @("$generated\winrt\Microsoft.UI.Xaml.Controls.h", $sdkManifest, $native, $runtime)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Missing full-build prerequisite: $required"
    }
}

$coreName = 'Microsoft.Web.WebView2.Core.dll'
New-Item -ItemType Directory -Force $output, "$output\plugin", "$objects\WebView2ActivationTestApp" | Out-Null
Get-ChildItem -LiteralPath $native -File | Where-Object Name -ne $coreName |
    Copy-Item -Destination $output -Force
Copy-Item -LiteralPath $runtime, "$bin\Product\Microsoft.ui.xaml.dll", "$bin\Product\Microsoft.UI.Xaml.Controls.dll" -Destination $output -Force
[xml]$packages = Get-Content "$root\packages.config" -Raw
$webview = $packages.packages.package | Where-Object id -eq 'Microsoft.Web.WebView2'
$core = "$root\packages\$($webview.id).$($webview.version)\runtimes\win-x64\native_uap\$coreName"
Copy-Item -LiteralPath $core -Destination "$output\plugin" -Force
if (Test-Path -LiteralPath "$output\$coreName") {
    throw "The dependency must exist only in the plugin directory: $output\$coreName"
}

[xml]$manifest = Get-Content -LiteralPath $sdkManifest -Raw
$coreRegistration = @($manifest.SelectNodes("//*[local-name()='file' and @name='$coreName']"))
if ($coreRegistration.Count -ne 1) {
    throw 'Expected exactly one WebView2 registration in the generated SDK manifest.'
}
# Core must use DLL-name fallback, not the SDK manifest's redirection.
$null = $coreRegistration[0].ParentNode.RemoveChild($coreRegistration[0])
$manifestPath = "$objects\WebView2ActivationTestApp\ActivationSearchTests.manifest"
$manifest.Save($manifestPath)

$arguments = @(
    '/nologo', '/std:c++20', '/EHsc', '/MD', '/O2', '/DUNICODE', '/D_UNICODE',
    "/I$generated", "$PSScriptRoot\ActivationSearchTests.cpp",
    "/Fo$objects\WebView2ActivationTestApp\ActivationSearchTests.obj",
    "/Fe$output\ActivationSearchTests.exe",
    '/link', 'windowsapp.lib', 'user32.lib', '/MANIFEST:EMBED', "/MANIFESTINPUT:$manifestPath"
)
& cl.exe @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Activation probe compilation failed: $LASTEXITCODE"
}
Copy-Item -LiteralPath "$PSScriptRoot\Run.ps1" -Destination $output -Force
Write-Output "Built activation regression app: $output"
