# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$tokens = $null
$errors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile("$repoRoot\test\CreateTestPayload.ps1", [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw "Payload parser errors: $errors" }
$selection = $ast.Find({
    param($node)
    $node -is [System.Management.Automation.Language.IfStatementAst] -and
    $node.Extent.Text.StartsWith('if ($UseSharedTestProjections -and ') -and
    $node.Extent.Text.Contains('$managedTestRoot =')
}, $false)
$scrub = $ast.Find({
    param($node)
    $node -is [System.Management.Automation.Language.IfStatementAst] -and
    $node.Extent.Text.StartsWith('if ($UseSharedTestProjections)') -and
    $node.Extent.Text.Contains('$sharedAssemblyPath')
}, $true)
if (!$selection -or !$scrub) { throw 'Shared staging blocks not found' }
$selection = [scriptblock]::Create($selection.Extent.Text)
$scrub = [scriptblock]::Create($scrub.Extent.Text)

function Test-Staging([string]$Mode, [bool]$UseSharedTestProjections, [bool]$sdkOnly, [string]$missing = '') {
    $outpath = Join-Path ([IO.Path]::GetTempPath()) ('WinUISharedTestProjections-' + [guid]::NewGuid().ToString('N'))
    $binpath = "$outpath\build"
    $active = $UseSharedTestProjections -and $Mode -in @('DevTestSuite', 'ScenarioTestSuite')
    try {
        New-Item -ItemType Directory -Path @(
            "$outpath\Test\UnpackagedApps\ApiApp", "$outpath\localDotNet",
            "$binpath\Test\SharedTestProjections", "$binpath\Test\MUXTestInfra",
            "$outpath\Test\Microsoft.UI.Xaml.Tests.Managed.Enterprise.Moco\localDotNet",
            "$outpath\Test\MUXControls.Test\localDotNet", "$outpath\Test\SharedTestProjections"
        ) -Force | Out-Null
        foreach ($module in 'Microsoft.UI.Xaml.Tests.Managed.Enterprise.Moco', 'MUXControls.Test') {
            [IO.File]::WriteAllText("$outpath\Test\$module.dll", 'current-module')
            [IO.File]::WriteAllText("$outpath\Test\$module\$module.dll", 'obsolete-module')
            [IO.File]::WriteAllText("$outpath\Test\$module\localDotNet\coreclr.dll", 'obsolete-runtime')
        }
        foreach ($file in 'WinRT.Interop.dll', 'WinRT.Projection.dll', 'WinRT.Component.dll', 'WinRT.Sdk.Projection.dll') {
            [IO.File]::WriteAllText("$outpath\Test\$file", 'obsolete-flat')
            [IO.File]::WriteAllText("$outpath\localDotNet\$file", 'app-runtime-map')
            [IO.File]::WriteAllText("$outpath\Test\UnpackagedApps\ApiApp\$file", 'app-owned')
            if ($file -ne $missing -and (!$sdkOnly -or $file -in @('WinRT.Interop.dll', 'WinRT.Sdk.Projection.dll'))) {
                [IO.File]::WriteAllText("$binpath\Test\SharedTestProjections\$file", 'shared-' + $file)
            }
        }
        [IO.File]::WriteAllText("$binpath\Test\MUXTestInfra\MUXTestInfra.dll", 'v3-helper')
        [IO.File]::WriteAllText("$outpath\Test\MUXTestInfra.dll", 'stale-helper')
        [IO.File]::WriteAllText("$outpath\localDotNet\coreclr.dll", 'runtime')
        foreach ($file in 'sample.msix', 'test.cer', 'WinUIGalleryTestData.xml') {
            [IO.File]::WriteAllText("$outpath\Test\$file", 'shared-data')
        }
        $failed = $false
        try {
            if ($Mode -in @('DevTestSuite', 'ScenarioTestSuite')) { & $scrub }
            & $selection
        } catch {
            if (!$missing -or !$_.Exception.Message.Contains($missing)) { throw }
            $failed = $true
        }
        if ($missing) {
            if (!$failed) { throw "Missing $missing was accepted" }
            Write-Output "PASS: missing required $missing rejected"
            return
        }
        foreach ($file in 'WinRT.Interop.dll', 'WinRT.Projection.dll', 'WinRT.Component.dll', 'WinRT.Sdk.Projection.dll') {
            if ((Get-Content -LiteralPath "$outpath\Test\UnpackagedApps\ApiApp\$file" -Raw) -ne 'app-owned') { throw 'App bundle changed' }
            $present = !$active -or !$sdkOnly -or $file -in @('WinRT.Interop.dll', 'WinRT.Sdk.Projection.dll')
            if ((Test-Path -LiteralPath "$outpath\Test\$file") -ne $present) { throw "Wrong sidecar presence: $file" }
            if ($present) {
                $expected = if ($active) { 'shared-' + $file } else { 'obsolete-flat' }
                if ((Get-Content -LiteralPath "$outpath\Test\$file" -Raw) -ne $expected) { throw "Wrong sidecar: $file" }
            }
        }
        foreach ($module in 'Microsoft.UI.Xaml.Tests.Managed.Enterprise.Moco', 'MUXControls.Test') {
            if ((Test-Path -LiteralPath "$outpath\Test\$module") -eq $active) { throw 'Obsolete module cleanup incorrect' }
            if ((Get-Content -LiteralPath "$outpath\Test\$module.dll" -Raw) -ne 'current-module') { throw 'Flat test removed' }
        }
        if ($active) {
            if ((Get-Content -LiteralPath "$outpath\Test\MUXTestInfra.dll" -Raw) -ne 'v3-helper') { throw 'Stale helper selected' }
            foreach ($file in 'WinRT.Interop.dll', 'WinRT.Projection.dll', 'WinRT.Component.dll') {
                if (Test-Path -LiteralPath "$outpath\localDotNet\$file") { throw 'App map contaminated shared runtime' }
            }
            if ((Get-Content -LiteralPath "$outpath\localDotNet\WinRT.Sdk.Projection.dll" -Raw) -ne 'shared-WinRT.Sdk.Projection.dll') { throw 'Wrong SDK projection in runtime profile' }
            if (Test-Path -LiteralPath "$outpath\Test\SharedTestProjections") { throw 'Redundant staging bundle retained' }
            & $selection
        }
        foreach ($file in 'sample.msix', 'test.cer', 'WinUIGalleryTestData.xml') {
            if ((Get-Content -LiteralPath "$outpath\Test\$file" -Raw) -ne 'shared-data') { throw 'Shared data changed' }
        }
        if ((Get-Content -LiteralPath "$outpath\localDotNet\coreclr.dll" -Raw) -ne 'runtime') { throw 'Runtime changed' }
        Write-Output "PASS: mode=$Mode shared=$UseSharedTestProjections sdkOnly=$sdkOnly"
    } finally {
        Remove-Item -LiteralPath $outpath -Recurse -Force
    }
}

Test-Staging DevTestSuite $true $false
Test-Staging ScenarioTestSuite $true $false
Test-Staging ScenarioTestSuite $true $true
Test-Staging DevTestSuite $false $false
Test-Staging PGO $true $false
Test-Staging DevTestSuite $true $false 'WinRT.Interop.dll'
Test-Staging ScenarioTestSuite $true $true 'WinRT.Sdk.Projection.dll'
