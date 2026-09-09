[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$outputPath = Join-Path $env:TEMP "winui-system-component-inventory-test.json"

try
{
    & (Join-Path $PSScriptRoot "Get-ComponentInventory.ps1") -OutputPath $outputPath | Out-Null
    $inventory = Get-Content $outputPath -Raw | ConvertFrom-Json

    $composition = $inventory.components | Where-Object id -eq "composition"
    $dispatching = $inventory.components | Where-Object id -eq "dispatching"

    if ($null -eq $composition -or $composition.winui.idl.count -lt 20)
    {
        throw "The WinUI Composition IDL inventory is unexpectedly incomplete."
    }

    if ($composition.latestOs.systemIdlFiles.Count -eq 0)
    {
        throw "System Composition IDL was not found in the latest OS source."
    }

    if ($composition.windows10Os.systemIdlFiles.Count -eq 0)
    {
        throw "System Composition IDL was not found in the Windows 10 OS source."
    }

    if ($null -eq $dispatching -or $dispatching.winui.idl.count -lt 5)
    {
        throw "The WinUI Dispatching IDL inventory is unexpectedly incomplete."
    }

    if ($dispatching.latestOs.systemIdlFiles.Count -eq 0)
    {
        throw "Windows.System.DispatcherQueue IDL was not found in the latest OS source."
    }

    if ($dispatching.windows10Os.systemIdlFiles.Count -eq 0)
    {
        throw "Windows.System.DispatcherQueue IDL was not found in the Windows 10 OS source."
    }

    Write-Output "Component inventory tests passed."
}
finally
{
    Remove-Item $outputPath -ErrorAction SilentlyContinue
}
