# Data-only checks. Compiles just the two BCL-only fixture types in memory.
# Does not load WinUI, restore packages, build the sample, or test UI Automation.
$ErrorActionPreference = 'Stop'

# Windows PowerShell 5.1's Add-Type uses the in-box CodeDom C# 5 compiler, which cannot
# parse the fixture sources (file-scoped namespace, nullable reference annotations).
# Re-run under PowerShell 7+, whose Add-Type compiles with Roslyn.
if ($PSVersionTable.PSEdition -ne 'Core') {
    $pwsh = Get-Command pwsh -ErrorAction SilentlyContinue
    if (-not $pwsh) {
        throw 'FAIL: PowerShell 7+ (pwsh) is required to compile the fixture sources.'
    }
    & $pwsh.Source -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath
    exit $LASTEXITCODE
}

$sampleRoot = Split-Path -Parent $PSScriptRoot
$sources = @(
    (Join-Path $sampleRoot 'Models\AccessibilityRow.cs'),
    (Join-Path $sampleRoot 'Data\AccessibilityFixtureData.cs')
)

function Assert-That([bool] $condition, [string] $message) {
    if (-not $condition) { throw "FAIL: $message" }
}

foreach ($source in $sources) {
    Assert-That (Test-Path -LiteralPath $source) "Deterministic fixture source must exist: $source"
}
Add-Type -Path $sources

$data = [TableViewSampleApp.Data.AccessibilityFixtureData]::new()
$originals = @($data.Rows)
Assert-That ($originals.Count -eq 24) 'The fixture has exactly 24 fixed records'
Assert-That (($originals.Id -join ',') -eq ((1..24) -join ',')) 'Record IDs and source order are stable'
Assert-That ($originals[0].DisplayText -eq 'Record 01') 'The first display value is fixed'
Assert-That ($originals[23].DisplayText -eq 'Record 24') 'The last display value is fixed'
Assert-That ((@($originals | Where-Object Department -eq 'Design').Count) -eq 12) 'There are 12 Design records'
Assert-That ((@($originals | Where-Object Department -eq 'Engineering').Count) -eq 12) 'There are 12 Engineering records'
Write-Output 'PASS: fixed 24-record dataset, IDs, source order, and two equal groups'

$another = [TableViewSampleApp.Data.AccessibilityFixtureData]::new()
foreach ($i in 0..23) {
    Assert-That ($originals[$i].DisplayText -eq $another.Rows[$i].DisplayText) 'A new fixture has identical values'
    Assert-That (-not [object]::ReferenceEquals($originals[$i], $another.Rows[$i])) 'Independent fixtures do not share mutable records'
}
Write-Output 'PASS: deterministic independent instances'

$notifications = [System.Collections.Generic.List[string]]::new()
$handler = [System.ComponentModel.PropertyChangedEventHandler] {
    param($sender, $eventArgs)
    $notifications.Add($eventArgs.PropertyName)
}
$originals[0].add_PropertyChanged($handler)
Assert-That ($data.Mutate($originals[0])) 'A present record can be mutated'
$originals[0].remove_PropertyChanged($handler)
Assert-That ($originals[0].DisplayText -eq 'Record 01 (changed)') 'Mutation uses a repeatable display value'
Assert-That ($originals[0].Department -eq 'Engineering') 'Mutation moves the record to the other group'
Assert-That (($notifications -join ',') -eq 'DisplayText,Department') 'Both mutations notify binding listeners'
Assert-That ([object]::ReferenceEquals($data.Rows[0], $originals[0])) 'Mutation preserves object identity'
Write-Output 'PASS: in-place mutation, group-key change, and property notifications'

Assert-That ($originals[0].ExplicitName -eq 'Explicit label for record 01') 'The explicit label is distinct from displayed text'
Assert-That ($data.Mutate($originals[0])) 'A second mutation is supported'
Assert-That ($originals[0].DisplayText -eq 'Record 01') 'Second mutation returns to baseline text'
Assert-That ($originals[0].Department -eq 'Design') 'Second mutation returns to baseline group'
Write-Output 'PASS: repeatable two-state mutation and a separate explicit label'

Assert-That ($data.Remove($originals[0])) 'Remove the first record'
Assert-That ($data.Remove($originals[23])) 'Remove the offscreen candidate, record 24'
Assert-That ($data.Remove($originals[11])) 'Remove a middle record'
Assert-That ($data.Rows.Count -eq 21 -and $data.RemovedCount -eq 3) 'Removal counts describe source membership'
Assert-That (-not $data.Remove($originals[0])) 'Removing an absent record is a no-op'
Assert-That (-not $data.Mutate($originals[23])) 'Mutating an absent record is a no-op'
Write-Output 'PASS: first/middle/last removal and absent-record no-ops'

Assert-That ($data.RestoreRemoved() -eq 3) 'Restore reports how many objects it reinserted'
foreach ($i in 0..23) {
    Assert-That ([object]::ReferenceEquals($data.Rows[$i], $originals[$i])) 'Restore reuses each object in original source order'
}
Assert-That ($data.RestoreRemoved() -eq 0) 'Repeated restore is a no-op'
Write-Output 'PASS: restoration preserves exact identities/order and is idempotent'

Assert-That ($data.Mutate($data.LastRow)) 'Mutate record 24 without selecting or realizing it'
Assert-That ($data.Remove($data.LastRow)) 'Remove mutated record 24'
Assert-That ($data.RestoreRemoved() -eq 1) 'Restore mutated record 24'
Assert-That ($data.LastRow.DisplayText -eq 'Record 24 (changed)') 'Restore preserves edits; it is not reset'
Write-Output 'PASS: last-record operations do not depend on a table or viewport'

$originals[2].DisplayText = 'Manual edit'
$originals[2].Department = 'Custom group'
Assert-That ($data.Remove($originals[5])) 'Prepare a removed record for reset'
$data.Reset()
foreach ($i in 0..23) {
    Assert-That ([object]::ReferenceEquals($data.Rows[$i], $originals[$i])) 'Reset preserves original object identities'
    Assert-That ($data.Rows[$i].DisplayText -eq $another.Rows[$i].DisplayText) 'Reset restores display values'
    Assert-That ($data.Rows[$i].Department -eq $another.Rows[$i].Department) 'Reset restores group keys'
}
Assert-That ($data.RemovedCount -eq 0) 'Reset restores membership'
$data.Reset()
Assert-That ($data.Rows.Count -eq 24) 'Repeated reset does not duplicate rows'
Write-Output 'PASS: reset restores edited values/membership without replacing or duplicating objects'

Assert-That (-not $data.Mutate($null)) 'Null mutation is a no-op'
Assert-That (-not $data.Remove($null)) 'Null removal is a no-op'
Assert-That (-not $data.Mutate($another.Rows[0])) 'Foreign record mutation is rejected'
Assert-That (-not $data.Remove($another.Rows[0])) 'Foreign record removal is rejected'
Write-Output 'PASS: null and foreign-record guards'
Write-Output '9 data-only checks passed. No XAML, UI, UIA, or full sample compilation was tested.'
