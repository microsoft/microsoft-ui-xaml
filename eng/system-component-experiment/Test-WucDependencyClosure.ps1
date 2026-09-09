[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$outputPath = Join-Path $env:TEMP "winui-wuc-closure-test.json"

try
{
    & (Join-Path $PSScriptRoot "Get-WucDependencyClosure.ps1") `
        -OutputPath $outputPath |
        Out-Null

    $closure = Get-Content $outputPath -Raw | ConvertFrom-Json

    $requiredSystemNodes = @(
        "composition",
        "composition-core",
        "composition-desktop",
        "composition-effects",
        "composition-interactions",
        "composition-private",
        "dispatching"
    )
    foreach ($nodeId in $requiredSystemNodes)
    {
        $node = $closure.nodes | Where-Object id -eq $nodeId
        if ($null -eq $node -or
            -not $node.latestSystemAvailable -or
            -not $node.windows10SystemAvailable)
        {
            throw "Required system closure node '$nodeId' is unavailable."
        }
    }

    $experimental = $closure.nodes |
        Where-Object id -eq "composition-experimental-property-change"
    if ($null -eq $experimental -or
        $experimental.latestSystemAvailable -or
        ($experimental.winuiEvidence | Measure-Object -Property count -Sum).Sum -eq 0)
    {
        throw "The lifted-only Composition property-change dependency was not detected."
    }

    $dispatching = $closure.nodes | Where-Object id -eq "dispatching"
    $dispatcherEvidence = (
        $dispatching.winuiEvidence |
            Where-Object pattern -eq "msy::IDispatcherQueue3"
    )
    if ($null -eq $dispatcherEvidence -or $dispatcherEvidence.count -eq 0)
    {
        throw "The lifted-only DispatcherQueue lifecycle dependency was not detected."
    }

    $directManipulation = $closure.nodes | Where-Object id -eq "direct-manipulation"
    if (($directManipulation.winuiEvidence | Measure-Object -Property count -Sum).Sum -eq 0)
    {
        throw "The lifted DirectManipulation runtime dependency was not detected."
    }

    if ($closure.compositionSymbolClosure.usedSymbolCount -lt 40)
    {
        throw "The WinUI Composition symbol closure is unexpectedly incomplete."
    }

    $experimentalGap = $closure.compositionSymbolClosure.symbols |
        Where-Object name -eq "IExpCompositionPropertyChanged"
    if ($null -eq $experimentalGap -or $experimentalGap.latestSystemAvailable)
    {
        throw "The lifted-only experimental Composition symbol gap was not preserved."
    }

    Write-Output "WUC dependency closure tests passed."
}
finally
{
    Remove-Item $outputPath -ErrorAction SilentlyContinue
}
