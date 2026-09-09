[CmdletBinding()]
param(
    [string]$WinUIPath = "C:\microsoft-ui-xaml"
)

$ErrorActionPreference = "Stop"

$modelPath = Join-Path $WinUIPath "dxaml\xcp\tools\XCPTypesAutoGen\XamlOM\Model"
$idlPath = Join-Path $WinUIPath "dxaml\xcp\dxaml\idl\winrt"

$modelFiles = Get-ChildItem $modelPath -Filter "*.cs" -File
$idlFiles = Get-ChildItem $idlPath -Filter "*.idl" -File -Recurse

$modelText = ($modelFiles | Get-Content -Raw) -join "`n"
$idlText = ($idlFiles | Get-Content -Raw) -join "`n"

foreach ($required in @(
    "Windows.UI.Composition",
    "Windows.System.DispatcherQueue"
))
{
    if ($modelText.IndexOf($required, [StringComparison]::Ordinal) -lt 0 -or
        $idlText.IndexOf($required, [StringComparison]::Ordinal) -lt 0)
    {
        throw "System public contract '$required' is missing from XamlOM or generated IDL."
    }
}

$liftedReferences = @(
    "Microsoft.UI.Dispatching",
    "Microsoft.UI.Composition.AnimationPropertyInfo",
    "Microsoft.UI.Composition.CompositionBrush",
    "Microsoft.UI.Composition.CompositionEasingFunction",
    "Microsoft.UI.Composition.CompositionLight",
    "Microsoft.UI.Composition.CompositionPropertySet",
    "Microsoft.UI.Composition.Compositor",
    "Microsoft.UI.Composition.IAnimationObject",
    "Microsoft.UI.Composition.ICompositionAnimationBase",
    "Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop",
    "Microsoft.UI.Composition.ICompositionSurface",
    "Microsoft.UI.Composition.IVisualElement",
    "Microsoft.UI.Composition.IVisualElement2",
    "Microsoft.UI.Composition.Visual"
)

foreach ($reference in $liftedReferences)
{
    if ($modelText.IndexOf($reference, [StringComparison]::Ordinal) -ge 0 -or
        $idlText.IndexOf($reference, [StringComparison]::Ordinal) -ge 0)
    {
        throw "Lifted public contract reference '$reference' remains."
    }
}

if ($modelText.IndexOf(
    "Microsoft.UI.Composition.SystemBackdrops.SystemBackdropConfiguration",
    [StringComparison]::Ordinal
) -lt 0)
{
    throw "The explicitly retained no-counterpart SystemBackdropConfiguration contract is missing."
}

Write-Output "System public contract tests passed."
