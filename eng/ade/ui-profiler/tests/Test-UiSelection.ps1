# Exercise the actual replay branch with inert UIA doubles; no windows or input.
#Requires -Version 5.1
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
$tokens = $null
$errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    "$PSScriptRoot\..\pack\tools\Invoke-Scenario.ps1", [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw 'Scenario runner has syntax errors.' }
$dispatch = $ast.Find({
    param($node)
    $node -is [Management.Automation.Language.SwitchStatementAst] -and
        $node.Condition.Extent.Text -eq '$step.action'
}, $true)
$clause = @($dispatch.Clauses | Where-Object { $_.Item1.Value -eq 'select' })
if ($clause.Count -ne 1) { throw 'Missing unique selection replay branch.' }
$body = $clause[0].Item2.Extent.Text
$action = [scriptblock]::Create($body.Substring(1, $body.Length - 2))
function Expect([bool]$Condition, [string]$Message) {
    if (!$Condition) { throw $Message }
}
function Refuses([scriptblock]$Body, [string]$Message) {
    $failure = $null
    try { & $Body } catch { $failure = $_.Exception.Message }
    Expect ($null -ne $failure -and $failure.Contains($Message)) "Expected failure: $Message"
}
function Fixture([bool]$Selected, [string]$Mode = 'complete') {
    $pattern = [pscustomobject]@{
        Current = [pscustomobject]@{ IsSelected = $Selected }; mode = $Mode; calls = 0
    }
    $pattern | Add-Member ScriptMethod Select {
        $this.calls++
        if ($this.mode -eq 'reject') { throw 'Fixture selection rejected.' }
        if ($this.mode -eq 'complete') { $this.Current.IsSelected = $true }
    }
    $element = [pscustomobject]@{ pattern = $pattern }
    $element | Add-Member ScriptMethod GetCurrentPattern {
        param($id)
        if ($id -ne [System.Windows.Automation.SelectionItemPattern]::Pattern) { throw 'Wrong UIA pattern.' }
        if ($this.pattern.mode -eq 'missing') { throw 'Fixture missing selection pattern.' }
        $this.pattern
    }
    $element
}
$wait = [pscustomobject]@{ Elapsed = [pscustomobject]@{ TotalSeconds = 6 } }
$wait | Add-Member ScriptMethod Restart {}
$entry = @{}
$element = Fixture $false
& $action
Expect ($element.pattern.calls -eq 1 -and $entry.from -ceq 'False' -and $entry.to -ceq 'True') `
    'Selection did not invoke the pattern and retain actual before/after state.'
$element = Fixture $true
Refuses { & $action } 'no-op'
Expect ($element.pattern.calls -eq 0) 'An already selected item was invoked.'
$element = Fixture $false 'reject'
Refuses { & $action } 'Fixture selection rejected.'
$element = Fixture $false 'missing'
Refuses { & $action } 'Fixture missing selection pattern.'
$element = Fixture $false 'unchanged'
Refuses { & $action } 'UI item did not become selected.'
Expect ($entry.to -ceq 'False') 'Failed selection was reported as selected.'
Write-Output 'PASS UIA selection: transition, no-op, provider failure, missing pattern and completion failure.'
