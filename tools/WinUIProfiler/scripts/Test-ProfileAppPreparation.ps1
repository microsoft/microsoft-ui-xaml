#requires -Version 7.4
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ArtifactDirectory
)
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'Private-runtime preparation tests require Windows.' }
$tokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Join-Path $PSScriptRoot 'Profile-App.ps1'), [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw ($parseErrors.Message -join [Environment]::NewLine) }
foreach ($name in @('Get-LocalPath', 'Get-FileIdentity', 'Get-MuxCompanions', 'Write-Host')) {
    $definition = $ast.Find({
        param($node)
        $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name
    }, $true)
    if (-not $definition) { throw "Preparation function missing: $name" }
    . ([scriptblock]::Create($definition.Extent.Text))
}

$originalError = [Console]::Error
$diagnostics = [IO.StringWriter]::new()
try {
    [Console]::SetError($diagnostics)
    $pipeline = @(Write-Host 'preparation diagnostic' -ForegroundColor Cyan)
}
finally { [Console]::SetError($originalError) }
if ($pipeline.Count -ne 0 -or $diagnostics.ToString().Trim() -ne 'preparation diagnostic') {
    throw 'Headless preparation diagnostics must accept positional messages and write only to stderr.'
}
$diagnostics.Dispose()

$root = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ArtifactDirectory)
$fixture = Join-Path $root "preparation-$([guid]::NewGuid().ToString('N'))"
$null = New-Item -ItemType Directory -Path $fixture
try {
    $imports = @('Microsoft.Internal.FrameworkUdk.dll', 'Microsoft.UI.Windowing.Core.dll', 'CoreMessagingXP.dll')
    foreach ($name in @('Microsoft.UI.Xaml.dll', 'unrelated.dll') + $imports) {
        [IO.File]::WriteAllBytes((Join-Path $fixture $name), [byte[]]@(1, 2, 3, 4))
    }
    $companions = @(Get-MuxCompanions $fixture $false)
    if ($companions.Count -ne $imports.Count -or @($imports | Where-Object { $_ -notin $companions.name }).Count) {
        throw 'Private runtime staging must retain the app-local import closure without copying the original core or unrelated DLLs.'
    }
}
finally {
    Remove-Item -LiteralPath $fixture -Recurse -Force
}
Write-Output 'Passed: headless stderr diagnostics and private-runtime app-local import staging.'
