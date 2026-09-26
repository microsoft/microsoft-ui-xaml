# triage-switcher-failures.ps1  (UNTRACKED dev helper — do not commit)
#
# Classifies failures from a /p:SwitcherMode=true master-backed test run into:
#   NEEDS_SWITCHER_BASELINE - test failed and NO .master.switcher.* exists for it.
#                             Under SwitcherMode the infra fell back to the normal
#                             .master.* and the switcher tree legitimately differs.
#                             => Action: generate/record a .master.switcher.* baseline.
#   SWITCHER_REGRESSION      - test failed even though a .master.switcher.* baseline
#                             DOES exist => the tree differs from the recorded switcher
#                             baseline. => Action: investigate as a real switcher change.
#   INFRA_OR_OTHER           - Blocked / crash / setup failure (not a master diff).
#
# Usage:
#   .\tools\triage-switcher-failures.ps1 -LogPath C:\path\to\testrun-output.log
#   .\tools\triage-switcher-failures.ps1 -LogPath .\run-wpf.log -OutCsv .\triage-wpf.csv
#
# To capture a log: either redirect the host console output of run-tests-on-vm.ps1 to a
# file (... *> run-wpf.log), or copy the VM-side log back:
#   $cred = Import-Clixml "$env:USERPROFILE\.winui-test\vmcred-switcher_ge_vm2.xml"
#   $s = New-PSSession -VMName switcher-ge-vm2 -Credential $cred
#   Copy-Item "C:\TestPayload\$env:COMPUTERNAME-microsoft-ui-xaml-lift\testrun-output.log" .\run-wpf.log -FromSession $s
#   Remove-PSSession $s

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$LogPath,

    [string]$MastersDir = "C:\work\microsoft-ui-xaml-lift\dxaml\test\resources\masters",

    [string]$OutCsv = "",

    [switch]$Quiet
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $LogPath)) { throw "Log not found: $LogPath" }
if (-not (Test-Path $MastersDir)) { throw "Masters dir not found: $MastersDir" }

# --- Normalize the log to clean text (te.exe redirected output can be UTF-16 with NULs) ---
$bytes = [System.IO.File]::ReadAllBytes($LogPath) | Where-Object { $_ -ne 0 }
if ($bytes.Count -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) { $bytes = $bytes[3..($bytes.Count-1)] }
$text  = [System.Text.Encoding]::UTF8.GetString([byte[]]$bytes)
$lines = $text -split "`r?`n"

# --- Build a fast lookup of master base-names that have a switcher baseline ---
# A test's master files are named <Area>_<Class>_<Method>[.<variation>].master[.switcher].<ext>.
# We index the prefix (everything before ".master") of every *.master.switcher.* file.
$switcherPrefixes = New-Object System.Collections.Generic.HashSet[string] ([StringComparer]::OrdinalIgnoreCase)
$normalPrefixes   = New-Object System.Collections.Generic.HashSet[string] ([StringComparer]::OrdinalIgnoreCase)
foreach ($f in Get-ChildItem $MastersDir -Filter '*.master*.xml' -File -ErrorAction SilentlyContinue) {
    $idx = $f.Name.IndexOf('.master', [StringComparison]::OrdinalIgnoreCase)
    if ($idx -lt 1) { continue }
    $prefix = $f.Name.Substring(0, $idx)
    if ($f.Name -match '\.master\.switcher\.') { [void]$switcherPrefixes.Add($prefix) }
    else { [void]$normalPrefixes.Add($prefix) }
}

# Map a fully-qualified TAEF test name to its master prefix.
#   Microsoft::UI::Xaml::Tests::Foundation::Graphics::SwitcherTests::CompNode1...
#     -> Foundation_Graphics_SwitcherTests_CompNode1...
function Get-MasterPrefix([string]$testName) {
    $n = $testName -replace '^Microsoft::UI::Xaml::Tests::', ''
    return ($n -replace '::', '_')
}

# A test "has a switcher baseline" if any switcher master prefix starts with the test's
# prefix (covers per-variation masters like <prefix>.gvi_Dark.master.switcher.xml).
function Test-HasSwitcherBaseline([string]$prefix) {
    if ($switcherPrefixes.Contains($prefix)) { return $true }
    foreach ($p in $switcherPrefixes) { if ($p.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { return $true } }
    return $false
}
function Test-HasNormalBaseline([string]$prefix) {
    if ($normalPrefixes.Contains($prefix)) { return $true }
    foreach ($p in $normalPrefixes) { if ($p.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { return $true } }
    return $false
}

# --- Parse the log for non-passing tests ---
# Primary signal: 'EndGroup: <name> [<Status>]'. Fallback: the 'Summary of Non-passing
# Tests:' block ('    <name> [<Status>]'). We de-dupe by test name, preferring EndGroup.
$results = @{}   # name -> status
foreach ($ln in $lines) {
    if ($ln -match 'EndGroup:\s+(?<name>[\w:]+::[\w:]+)\s+\[(?<st>Failed|Blocked|Skipped)\]') {
        $results[$Matches.name] = $Matches.st
    }
}
if ($results.Count -eq 0) {
    # Fallback to the summary block.
    $inSummary = $false
    foreach ($ln in $lines) {
        if ($ln -match 'Summary of Non-passing Tests:') { $inSummary = $true; continue }
        if ($inSummary) {
            if ($ln -match '^\s+(?<name>[\w:]+::[\w:]+)\s+\[(?<st>Failed|Blocked|Skipped)\]') { $results[$Matches.name] = $Matches.st }
            elseif ($ln -match '^\s*Summary:') { break }
        }
    }
}

# --- Overall TAEF summary line (for sanity) ---
$summaryLine = ($lines | Where-Object { $_ -match 'Summary:\s+Total=\d+' } | Select-Object -Last 1)

# --- Classify ---
$rows = foreach ($name in ($results.Keys | Sort-Object)) {
    $status = $results[$name]
    $prefix = Get-MasterPrefix $name
    $hasSw  = Test-HasSwitcherBaseline $prefix
    $hasNm  = Test-HasNormalBaseline $prefix

    $class =
        if ($status -ne 'Failed') { 'INFRA_OR_OTHER' }          # Blocked / Skipped => not a master diff
        elseif ($hasSw)           { 'SWITCHER_REGRESSION' }     # failed vs its own switcher baseline
        elseif ($hasNm)           { 'NEEDS_SWITCHER_BASELINE' } # fell back to normal master, tree differs
        else                      { 'NO_MASTER_FOUND' }         # couldn't map to any master (name/derivation issue)

    [pscustomobject]@{
        Test              = $name
        Status            = $status
        MasterPrefix      = $prefix
        HasSwitcherMaster = $hasSw
        HasNormalMaster   = $hasNm
        Classification    = $class
    }
}

# --- Report ---
if (-not $Quiet) {
    Write-Host ""
    Write-Host "==================== Switcher failure triage ====================" -ForegroundColor Cyan
    Write-Host "Log:        $LogPath"
    Write-Host "MastersDir: $MastersDir"
    if ($summaryLine) { Write-Host "TAEF:       $($summaryLine.Trim())" -ForegroundColor Gray }
    Write-Host "Non-passing tests found: $($rows.Count)" -ForegroundColor Yellow
    Write-Host ""
    $rows | Group-Object Classification | Sort-Object Name | ForEach-Object {
        $color = switch ($_.Name) {
            'SWITCHER_REGRESSION'      { 'Red' }
            'NEEDS_SWITCHER_BASELINE'  { 'Yellow' }
            'INFRA_OR_OTHER'           { 'DarkYellow' }
            default                    { 'Gray' }
        }
        Write-Host ("  {0,-24} {1}" -f $_.Name, $_.Count) -ForegroundColor $color
    }
    Write-Host ""
    $reg = $rows | Where-Object Classification -eq 'SWITCHER_REGRESSION'
    if ($reg) {
        Write-Host "SWITCHER_REGRESSION (investigate — differ from an existing switcher baseline):" -ForegroundColor Red
        $reg | ForEach-Object { Write-Host "    $($_.Test)" }
        Write-Host ""
    }
}

if (-not $OutCsv) {
    $OutCsv = Join-Path (Split-Path $LogPath -Parent) ("triage-" + [IO.Path]::GetFileNameWithoutExtension($LogPath) + ".csv")
}
$rows | Sort-Object Classification, Test | Export-Csv -Path $OutCsv -NoTypeInformation -Encoding UTF8
if (-not $Quiet) { Write-Host "CSV written: $OutCsv" -ForegroundColor Green }

return $rows
