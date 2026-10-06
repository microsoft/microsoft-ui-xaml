# Static source/XML checks only. These do NOT compile XAML, resolve WinUI types,
# launch the sample, query UIA, or establish any accessibility/runtime pass.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Assert-That([bool] $condition, [string] $message) {
    if (-not $condition) { throw "FAIL (static): $message" }
}
function Read-Sample([string] $relativePath) {
    Get-Content -Raw -LiteralPath (Join-Path $root $relativePath)
}

$xamlFiles = @(Get-ChildItem -LiteralPath $root -Recurse -Filter '*.xaml' -File |
    Where-Object { $_.FullName -notmatch '\\(bin|obj)\\' })
$tableCount = 0
foreach ($file in $xamlFiles) {
    [xml] $xml = Get-Content -Raw -LiteralPath $file.FullName
    $ids = @($xml.SelectNodes('//*[@AutomationProperties.AutomationId]') |
        ForEach-Object { $_.GetAttribute('AutomationProperties.AutomationId') })
    Assert-That (@($ids | Group-Object | Where-Object Count -gt 1).Count -eq 0) "Duplicate static AutomationId in $($file.Name)"
    foreach ($table in $xml.SelectNodes('//*[local-name()="TableView"]')) {
        $tableCount++
        Assert-That (-not [string]::IsNullOrWhiteSpace($table.GetAttribute('AutomationProperties.Name'))) "App-owned table Name missing in $($file.Name)"
        Assert-That (-not [string]::IsNullOrWhiteSpace($table.GetAttribute('AutomationProperties.AutomationId'))) "Table AutomationId missing in $($file.Name)"
    }
}
Write-Output "PASS (static): $($xamlFiles.Count) XML documents; $tableCount tables with metadata; no duplicate static IDs within a file"

[xml] $shell = Read-Sample 'MainWindow.xaml'
$destinations = @($shell.SelectNodes('//*[local-name()="NavigationViewItem"][@Tag]'))
Assert-That ($destinations.Count -eq 30) 'Exactly one destination was added to the existing 29'
$shellCode = Read-Sample 'MainWindow.xaml.cs'
foreach ($destination in $destinations) {
    $tag = $destination.GetAttribute('Tag')
    Assert-That ($shellCode.Contains('["' + $tag + '"] = typeof(')) "Destination $tag has a page map entry"
}
Write-Output 'PASS (static): all 30 destinations have page-map entries, including existing Groups and Shaping'

[xml] $fixture = Read-Sample 'Pages\AccessibilityRegressionPage.xaml'
$fixtureCode = Read-Sample 'Pages\AccessibilityRegressionPage.xaml.cs'
Assert-That ($null -ne $fixture.SelectSingleNode('//*[local-name()="SamplePresenter"]')) 'Fixture reuses SamplePresenter'
Assert-That (@($fixture.SelectNodes('//*[local-name()="ComboBox"][@Header="Assessment scenario"]/*')).Count -eq 4) 'Four focused scenarios are declared'
Assert-That ($fixtureCode.Contains('ClearValue(AutomationProperties.NameProperty)')) 'An explicit unlabeled-table baseline exists'
Assert-That (-not ($fixtureCode -match '\.BeginEdit\s*\(|CurrentCell|OnCreateAutomationPeer')) 'Fixture must not invent edit APIs or override providers'
foreach ($node in $fixture.SelectNodes('//*')) {
    foreach ($attribute in $node.Attributes) {
        if ($attribute.LocalName -in @('Click', 'Toggled', 'SelectionChanged', 'BeginningEdit', 'CellEditEnding')) {
            Assert-That ($fixtureCode -match ('\b' + [regex]::Escape($attribute.Value) + '\s*\(')) "Fixture handler $($attribute.Value) is defined"
        }
    }
}
Write-Output 'PASS (static): fixture registration, four scenarios, handler names, public-API guard, and default-name case'

$keyboard = Read-Sample 'Pages\KeyboardNavPage.xaml'
$keyboardCode = Read-Sample 'Pages\KeyboardNavPage.xaml.cs'
Assert-That (-not (($keyboard + $keyboardCode) -match 'MajorText|UIA RowOrColumnMajor|mirrors those same values')) 'Keyboard counters must not pretend to be UIA evidence'
Assert-That ($keyboard.Contains('KeyboardNavBeforeTable') -and $keyboard.Contains('KeyboardNavAfterTable')) 'Ordinary keyboard entry/exit targets exist'
Write-Output 'PASS (static): keyboard instructions/counters no longer claim peer measurements'

$sort = (Read-Sample 'Pages\SortPage.xaml') + (Read-Sample 'Pages\SortPage.xaml.cs') + (Read-Sample 'Snippets\Sort.cs.txt')
Assert-That (-not ($sort -match 'Teams\.Take\(5\)|TopRowsPreviewText|SortedColumns|SortRequested')) 'Stale source-order preview/API explanations were removed'
Assert-That ($sort.Contains('Programmatic sort setup') -and $sort.Contains('Source objects')) 'Sort setup and source counters are labeled'
Write-Output 'PASS (static): sorting readout does not present source order as displayed order'

$empty = Read-Sample 'Pages\EmptyStatePage.xaml'
foreach ($tag in @('Populated', 'Null', 'Collection', 'Filtered')) {
    Assert-That ($empty.Contains('Tag="' + $tag + '"')) "Empty-state mode $tag exists"
}
Assert-That ((Read-Sample 'Pages\EmptyStatePage.xaml.cs').Contains('_source.Filter(_ => false)')) 'Filter-to-empty uses the real public shaping API'
Write-Output 'PASS (static): distinct populated/null/empty-collection/filter-to-empty setup'

$groups = Read-Sample 'Pages\GroupsPage.xaml.cs'
Assert-That (-not $groups.Contains('new Random')) 'Grouping mutation is not random'
Assert-That ($groups.Contains('_originalDepartments') -and $groups.Contains('OnResetClick') -and $groups.Contains('.GroupBy(')) 'Grouping retains its real API and reset snapshot'
Write-Output 'PASS (static): repeatable grouping mutation/reset wiring'

[xml] $app = Read-Sample 'App.xaml'
$hc = $app.SelectSingleNode('//*[@*[local-name()="Key"]="HighContrast"]')
Assert-That ($null -ne $hc) 'Sample-owned high-contrast brush fallback exists'
Assert-That (@($hc.ChildNodes | Where-Object NodeType -eq Element).Count -eq 2) 'Only the two sample-owned banding brushes are added'
$unsafeEventPattern = '\bHighContrastChanged\s*[+-]='
# Positive control copied from the registration identified in the startup dump.
Assert-That ('_accessibilitySettings.HighContrastChanged += OnHighContrastChanged;' -match $unsafeEventPattern) 'The regression guard detects the crash-causing registration'
Assert-That (-not ($shellCode -match $unsafeEventPattern)) 'Unsupported AccessibilitySettings.HighContrastChanged must not be subscribed or revoked'
Assert-That (-not ($shellCode -match '\bOnHighContrastChanged\b')) 'The unsupported high-contrast event handler is removed'
$appearanceHandlers = [ordered]@{
    ColorValuesChanged = 'OnSystemColorValuesChanged'
    Activated = 'OnWindowActivated'
    ActualThemeChanged = 'OnRootActualThemeChanged'
}
foreach ($hook in $appearanceHandlers.GetEnumerator()) {
    Assert-That ($shellCode.Contains($hook.Key + ' += ' + $hook.Value + ';') -and
        $shellCode.Contains($hook.Key + ' -= ' + $hook.Value + ';')) "$($hook.Key) has a matching named subscription/revocation"
}
Assert-That ($shellCode -match '(?s)private void OnWindowActivated\(object sender, WindowActivatedEventArgs args\)\s*\{\s*if \(!_closed\) UpdateCaptionButtonColors\(\);\s*\}') 'Window activation refreshes caption colors with a closed-window guard'
Assert-That ($shellCode.Contains('if (_accessibilitySettings.HighContrast)') -and $shellCode.Contains('titleBar.ButtonForegroundColor = null')) 'Real HC clears sample caption overrides'
Assert-That ((Read-Sample 'Pages\SettingsPage.xaml').Contains('Contrast themes')) 'Settings has real Windows Contrast theme directions'
Write-Output 'PASS (static): unsafe HC event excluded; ColorValuesChanged/Activated/root-theme lifetime pairs, HC query, and caption reset retained'

# Use PowerShell 7's already-installed Roslyn parser. Parse only: no compilation,
# type binding, source generation, output assembly, MSBuild, or package restore.
Add-Type -Path (Join-Path $PSHOME 'Microsoft.CodeAnalysis.dll')
Add-Type -Path (Join-Path $PSHOME 'Microsoft.CodeAnalysis.CSharp.dll')
$csharpFiles = @(Get-ChildItem -LiteralPath $root -Recurse -Filter '*.cs' -File |
    Where-Object { $_.FullName -notmatch '\\(bin|obj)\\' })
foreach ($file in $csharpFiles) {
    $tree = [Microsoft.CodeAnalysis.CSharp.CSharpSyntaxTree]::ParseText(
        [string](Get-Content -Raw -LiteralPath $file.FullName))
    $errors = @($tree.GetDiagnostics() | Where-Object Severity -eq Error)
    Assert-That ($errors.Count -eq 0) "C# syntax in $($file.Name): $($errors -join '; ')"
}
Write-Output "PASS (syntax only): $($csharpFiles.Count) C# files parsed; types and XAML-generated members were NOT resolved"
Write-Output '9 static source/XML/syntax checks passed. Full compile and UI/runtime assessment remain pending.'
