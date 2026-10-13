# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Build-CoverageTreeReport.ps1
#
# Renders a SINGLE, self-contained HTML file with a collapsible folder-tree view of code
# coverage - as opposed to reportgenerator's default `Html` report type, which emits one .html
# page PER SOURCE FILE/CLASS (thousands of files for a full WinUI build). This script performs
# no new coverage computation of its own - it is a pure rendering step on top of data already
# produced (and already verified to match Azure DevOps's own Code Coverage tab exactly) by
# Repair-CoverageDuplicates.ps1: `corrected-per-file.csv` and `corrected-summary.json`.
#
# OUTPUT: exactly one .html file. No external .css/.js/.svg assets, no per-file pages, no CDN
# dependencies - everything (styles, script, and the coverage data itself) is embedded inline,
# so the file can be opened directly from disk (or emailed, attached to a work item, etc.) with
# no network access and no companion folder.
#
# TREE UI: folders are collapsed by default at every level (only top-level root folders are
# visible on first load); clicking a folder row expands/collapses just that folder's immediate
# children (subfolders and files) one level at a time. Each folder shows a coverage percentage
# that is the SUM of covered/total lines across every file nested anywhere beneath it (computed
# bottom-up); each file shows its own exact percentage. Both use the same 3-tier ADO-style
# color scale: green >= 75%, yellow 40-75%, red < 40%.

[CmdletBinding()]
param(
    # Path to corrected-per-file.csv, as produced by Repair-CoverageDuplicates.ps1.
    [Parameter(Mandatory = $true)]
    [string]$InputCsv,

    # Path to corrected-summary.json (for the overall KPI header). Optional - if omitted or
    # missing, the KPI header is computed from the per-file rows instead.
    [string]$InputSummaryJson = "",

    # Folder to write the single HTML report into. Created if it doesn't exist. Kept separate
    # from reportgenerator's own (multi-file) `html\` output folder by convention.
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Report title shown at the top of the page.
    [string]$Title = "Code Coverage Report",

    # Regex matched against the START of each file path and stripped before display (the
    # "boilerplate" build-agent prefix). Default matches the OneBranch convention
    # (e.g. C:\__w\1\s\, D:\a\3\s\ etc. - a drive letter, "__w" or "a", a numeric leg id, then
    # "s\") case-insensitively, on either slash direction. If a given path doesn't match, it is
    # left completely unchanged - this is a display-only trim, never a hard requirement.
    [string]$StripPathPrefix = '^[A-Za-z]:[\\/]+(__w|a)[\\/]+\d+[\\/]+s[\\/]+',

    # Output file name.
    [string]$OutputFileName = 'coverage-tree.html',

    # Name(s) of the instrumented product DLL(s) this coverage measures (e.g.
    # "Microsoft.ui.xaml.dll"). Shown at the top of the report next to the title. If not
    # supplied, falls back to the "InstrumentedModules" field in -InputSummaryJson (as written
    # by Repair-CoverageDuplicates.ps1's -ModuleNames parameter); if that's also unavailable,
    # the report simply omits the module line rather than guessing.
    [string[]]$ModuleNames = @()
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $InputCsv)) {
    throw "Input CSV not found: $InputCsv"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Write-Host "Building single-file folder-tree coverage report from $InputCsv ..."
$sw = [System.Diagnostics.Stopwatch]::StartNew()

$rows = Import-Csv $InputCsv
if ($rows.Count -eq 0) {
    throw "No rows found in $InputCsv - nothing to render."
}

$stripRegex = $null
if ($StripPathPrefix) {
    try { $stripRegex = New-Object System.Text.RegularExpressions.Regex($StripPathPrefix, 'IgnoreCase') }
    catch { Write-Host "WARNING: -StripPathPrefix is not a valid regex ('$StripPathPrefix'); paths will be shown unmodified. $($_.Exception.Message)" -ForegroundColor Yellow }
}

# ---------------------------------------------------------------------------------------------
# Build an in-memory folder tree. Each node: @{ Name; IsFile; Covered; Total; Children (ordered
# dictionary, folders/files keyed by name) }. Folder Covered/Total are computed bottom-up after
# every file has been inserted.
# ---------------------------------------------------------------------------------------------
function New-TreeNode {
    param([string]$Name, [bool]$IsFile)
    [pscustomobject]@{
        Name     = $Name
        IsFile   = $IsFile
        Covered  = 0
        Total    = 0
        Children = [ordered]@{}
    }
}

$root = New-TreeNode -Name '' -IsFile $false
$strippedCount = 0
$totalCount = 0

foreach ($row in $rows) {
    $totalCount++
    $path = $row.File
    if ($stripRegex -and $stripRegex.IsMatch($path)) {
        $path = $stripRegex.Replace($path, '', 1)
        $strippedCount++
    }
    $path = $path -replace '/', '\'
    $segments = $path.Split('\') | Where-Object { $_ -ne '' }
    if ($segments.Count -eq 0) { continue }

    $covered = [int]$row.CoveredLines
    $total = [int]$row.TotalLines

    $node = $root
    for ($i = 0; $i -lt $segments.Count; $i++) {
        $seg = $segments[$i]
        $isLeaf = ($i -eq $segments.Count - 1)
        if (-not $node.Children.Contains($seg)) {
            $node.Children[$seg] = New-TreeNode -Name $seg -IsFile $isLeaf
        }
        $node = $node.Children[$seg]
    }
    # $node is now the leaf (file) node.
    $node.Covered = $covered
    $node.Total = $total
}

Write-Host ("Stripped the boilerplate prefix from {0}/{1} paths." -f $strippedCount, $totalCount)

# Bottom-up rollup: a folder's Covered/Total = sum of its children's (already-rolled-up) values.
function Get-Rollup {
    param($Node)
    if ($Node.IsFile) { return }
    $c = 0; $t = 0
    foreach ($child in $Node.Children.Values) {
        Get-Rollup -Node $child
        $c += $child.Covered
        $t += $child.Total
    }
    $Node.Covered = $c
    $Node.Total = $t
}
Get-Rollup -Node $root

# ---------------------------------------------------------------------------------------------
# Convert to a compact, JSON-serializable shape: folders sorted before files, then alphabetical.
# Keys are kept short (n/t/c/f/k) since this blob is embedded directly in the HTML and a full
# WinUI build can have several thousand files.
# ---------------------------------------------------------------------------------------------
function ConvertTo-JsonNode {
    param($Node)
    $pct = if ($Node.Total -gt 0) { [math]::Round(100.0 * $Node.Covered / $Node.Total, 2) } else { 0 }
    $obj = [ordered]@{
        n = $Node.Name
        c = $Node.Covered
        t = $Node.Total
        p = $pct
    }
    if ($Node.IsFile) {
        $obj['f'] = 1
    } else {
        $sortedChildren = $Node.Children.Values | Sort-Object @{Expression = { $_.IsFile } }, @{Expression = { $_.Name } }
        $obj['k'] = @($sortedChildren | ForEach-Object { ConvertTo-JsonNode -Node $_ })
    }
    return $obj
}

$treeData = @($root.Children.Values | Sort-Object @{Expression = { $_.IsFile } }, @{Expression = { $_.Name } } | ForEach-Object { ConvertTo-JsonNode -Node $_ })
$treeJson = $treeData | ConvertTo-Json -Depth 100 -Compress
if ($treeData.Count -eq 1) {
    # ConvertTo-Json unwraps single-element arrays unless -AsArray is available (PS 6.1+); force array shape.
    $treeJson = "[$treeJson]"
}

# ---------------------------------------------------------------------------------------------
# Overall KPI header - prefer the already-computed summary (exact figures verified against ADO);
# fall back to summing the per-file rows if the summary file wasn't supplied/found.
# ---------------------------------------------------------------------------------------------
$summary = $null
if ($InputSummaryJson -and (Test-Path $InputSummaryJson)) {
    $summary = Get-Content $InputSummaryJson -Raw | ConvertFrom-Json
}
if (-not $summary) {
    $totalLines = ($rows | Measure-Object -Property TotalLines -Sum).Sum
    $coveredLines = ($rows | Measure-Object -Property CoveredLines -Sum).Sum
    $summary = [pscustomobject]@{
        Modules      = $rows.Count
        TotalLines   = $totalLines
        CoveredLines = $coveredLines
        LineCoverage = if ($totalLines -gt 0) { [math]::Round(100.0 * $coveredLines / $totalLines, 2) } else { 0 }
    }
}

# Resolve which DLL(s) this report is measuring: an explicit -ModuleNames wins; otherwise fall
# back to the "InstrumentedModules" field already recorded in corrected-summary.json (as passed
# through from Get-PipelineCodeCoverage.ps1's -Module parameter); if neither is available the
# report simply omits the module line rather than guessing at a DLL name.
$resolvedModuleNames = @($ModuleNames)
if ($resolvedModuleNames.Count -eq 0 -and $summary.PSObject.Properties.Name -contains 'InstrumentedModules') {
    $resolvedModuleNames = @($summary.InstrumentedModules)
}
$moduleDisplay = if ($resolvedModuleNames -and $resolvedModuleNames.Count -gt 0) { ($resolvedModuleNames -join ', ') } else { '' }

# Make sure SUMMARY (embedded in the HTML for the KPI cards) reflects the resolved module list
# regardless of which path it came from (explicit -ModuleNames vs. corrected-summary.json).
if ($summary.PSObject.Properties.Name -contains 'InstrumentedModules') {
    $summary.InstrumentedModules = @($resolvedModuleNames)
} else {
    $summary | Add-Member -NotePropertyName 'InstrumentedModules' -NotePropertyValue @($resolvedModuleNames)
}

$summaryJson = $summary | ConvertTo-Json -Compress

# ---------------------------------------------------------------------------------------------
# HTML template. Single-quoted here-string (no PowerShell variable expansion) with placeholder
# tokens swapped in afterward - this keeps every '$' in the embedded JavaScript (template
# literals, etc.) completely literal and avoids any PowerShell-escaping gymnastics.
# ---------------------------------------------------------------------------------------------
$htmlTemplate = @'
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8" />
<title>__TITLE__</title>
<style>
  :root {
    --green: #2ea043; --green-bg: #e6f4ea;
    --yellow: #b08800; --yellow-bg: #fff8e1;
    --red: #cf222e; --red-bg: #fdecea;
    --border: #d0d7de; --muted: #57606a; --bg: #ffffff; --row-hover: #f6f8fa;
  }
  * { box-sizing: border-box; }
  body { font-family: -apple-system, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; margin: 0; padding: 24px; background: var(--bg); color: #1f2328; }
  h1 { font-size: 20px; margin: 0 0 4px 0; }
  .subtitle { color: var(--muted); font-size: 13px; margin-bottom: 20px; }
  .kpis { display: flex; gap: 16px; margin-bottom: 24px; flex-wrap: wrap; }
  .kpi { border: 1px solid var(--border); border-radius: 8px; padding: 14px 18px; min-width: 140px; }
  .kpi .label { font-size: 12px; color: var(--muted); text-transform: uppercase; letter-spacing: .03em; }
  .kpi .value { font-size: 26px; font-weight: 600; margin-top: 4px; }
  .legend { font-size: 12px; color: var(--muted); margin-bottom: 12px; display: flex; gap: 16px; align-items: center; flex-wrap: wrap; }
  .legend .swatch { display: inline-block; width: 10px; height: 10px; border-radius: 2px; margin-right: 5px; vertical-align: middle; }
  #search { margin-bottom: 12px; }
  #searchBox { width: 100%; max-width: 480px; padding: 8px 10px; border: 1px solid var(--border); border-radius: 6px; font-size: 13px; }
  .tree { border: 1px solid var(--border); border-radius: 8px; overflow: hidden; }
  ul.node-children { list-style: none; margin: 0; padding: 0; }
  ul.node-children.collapsed { display: none; }
  li.node { border-top: 1px solid var(--border); }
  li.node:first-child { border-top: none; }
  .row { display: flex; align-items: center; padding: 7px 10px; cursor: default; }
  .row.folder { cursor: pointer; }
  .row.folder:hover { background: var(--row-hover); }
  .caret { width: 14px; display: inline-block; text-align: center; color: var(--muted); font-size: 11px; user-select: none; flex: 0 0 auto; }
  .row.file .caret { visibility: hidden; }
  .name { flex: 1 1 auto; font-size: 13px; font-family: Consolas, "Cascadia Mono", monospace; overflow-wrap: anywhere; padding-right: 12px; }
  .row.folder .name { font-weight: 600; }
  .bar-wrap { flex: 0 0 140px; height: 8px; background: #eaeef2; border-radius: 4px; overflow: hidden; margin-right: 10px; }
  .bar-fill { height: 100%; }
  .pct { flex: 0 0 62px; text-align: right; font-size: 12px; font-weight: 600; font-variant-numeric: tabular-nums; }
  .lines { flex: 0 0 110px; text-align: right; font-size: 11px; color: var(--muted); font-variant-numeric: tabular-nums; padding-left: 10px; }
  .pct-hi, .bar-fill.hi { color: var(--green); }
  .bar-fill.hi { background: var(--green); }
  .pct-mid, .bar-fill.mid { color: var(--yellow); }
  .bar-fill.mid { background: var(--yellow); }
  .pct-lo, .bar-fill.lo { color: var(--red); }
  .bar-fill.lo { background: var(--red); }
  ul.node-children { padding-left: 22px; }
  .tree > ul.node-children { padding-left: 0; }
  .hidden-by-search { display: none !important; }
  .module-line { font-size: 13px; color: #1f2328; margin-bottom: 2px; }
  .module-line .module-badge { display: inline-block; background: #ddf4ff; color: #0969da; border: 1px solid #b6e3ff; border-radius: 12px; padding: 2px 10px; font-family: Consolas, "Cascadia Mono", monospace; font-size: 12px; margin-left: 6px; }
  footer { margin-top: 20px; font-size: 11px; color: var(--muted); }
</style>
</head>
<body>
<h1>__TITLE__</h1>
__MODULE_LINE__
<div class="subtitle">Generated __GENERATED_AT__ &middot; boilerplate path prefix stripped from every entry below</div>

<div class="kpis" id="kpis"></div>

<div class="legend">
  <span><span class="swatch" style="background:var(--green)"></span>&ge; 75%</span>
  <span><span class="swatch" style="background:var(--yellow)"></span>40&ndash;75%</span>
  <span><span class="swatch" style="background:var(--red)"></span>&lt; 40%</span>
  <span style="margin-left:auto">Click a folder row to expand/collapse it</span>
</div>

<div id="search">
  <input id="searchBox" type="text" placeholder="Filter by file or folder name..." />
</div>

<div class="tree" id="tree"></div>

<footer>Self-contained report - no external files. Coverage data is bug-fixed for a verified duplicate-line defect in the native (dotnet-coverage / Microsoft.CodeCoverage.Console) merge engine; these numbers match the Azure DevOps Code Coverage tab.</footer>

<script>
const TREE_DATA = __TREE_JSON__;
const SUMMARY = __SUMMARY_JSON__;

function pctClass(p) {
  if (p >= 75) return 'hi';
  if (p >= 40) return 'mid';
  return 'lo';
}

function fmtInt(n) {
  return n.toLocaleString('en-US');
}

function renderKpis() {
  const kpis = document.getElementById('kpis');
  const items = [
    ['Line coverage', (SUMMARY.LineCoverage !== undefined ? SUMMARY.LineCoverage : 0) + '%'],
    ['Covered lines', fmtInt(SUMMARY.CoveredLines || 0)],
    ['Total lines', fmtInt(SUMMARY.TotalLines || 0)],
    ['Files', fmtInt(SUMMARY.Modules || 0)]
  ];
  items.forEach(([label, value]) => {
    const div = document.createElement('div');
    div.className = 'kpi';
    const l = document.createElement('div'); l.className = 'label'; l.textContent = label;
    const v = document.createElement('div'); v.className = 'value'; v.textContent = value;
    div.appendChild(l); div.appendChild(v);
    kpis.appendChild(div);
  });
}

function buildRow(node) {
  const row = document.createElement('div');
  row.className = 'row ' + (node.f ? 'file' : 'folder');

  const caret = document.createElement('span');
  caret.className = 'caret';
  caret.textContent = node.f ? '' : '\u25B8';
  row.appendChild(caret);

  const name = document.createElement('span');
  name.className = 'name';
  name.textContent = node.n;
  row.appendChild(name);

  const barWrap = document.createElement('span');
  barWrap.className = 'bar-wrap';
  const barFill = document.createElement('span');
  barFill.className = 'bar-fill ' + pctClass(node.p);
  barFill.style.width = Math.min(node.p, 100) + '%';
  barWrap.appendChild(barFill);
  row.appendChild(barWrap);

  const pct = document.createElement('span');
  pct.className = 'pct pct-' + pctClass(node.p);
  pct.textContent = node.p.toFixed(1) + '%';
  row.appendChild(pct);

  const lines = document.createElement('span');
  lines.className = 'lines';
  lines.textContent = fmtInt(node.c) + ' / ' + fmtInt(node.t);
  row.appendChild(lines);

  return { row, caret };
}

function buildNodeLi(node) {
  const li = document.createElement('li');
  li.className = 'node';
  li.dataset.name = node.n.toLowerCase();

  const { row, caret } = buildRow(node);
  li.appendChild(row);

  if (!node.f && node.k && node.k.length) {
    const childUl = document.createElement('ul');
    childUl.className = 'node-children collapsed';
    node.k.forEach(child => childUl.appendChild(buildNodeLi(child)));
    li.appendChild(childUl);

    row.addEventListener('click', () => {
      const collapsed = childUl.classList.toggle('collapsed');
      caret.textContent = collapsed ? '\u25B8' : '\u25BE';
    });
  }
  return li;
}

function renderTree() {
  const container = document.getElementById('tree');
  const rootUl = document.createElement('ul');
  rootUl.className = 'node-children';
  TREE_DATA.forEach(node => rootUl.appendChild(buildNodeLi(node)));
  container.appendChild(rootUl);
}

// Filter: matching folders/files stay visible (and their ancestor chain is force-expanded);
// non-matching leaves are hidden. Clearing the box restores the normal collapsed tree.
function applyFilter(term) {
  const allLis = document.querySelectorAll('#tree li.node');
  if (!term) {
    allLis.forEach(li => {
      li.classList.remove('hidden-by-search');
      const ul = li.querySelector(':scope > ul.node-children');
      if (ul) ul.classList.add('collapsed');
      const caret = li.querySelector(':scope > .row .caret');
      if (caret && !li.querySelector(':scope > .row.file')) caret.textContent = '\u25B8';
    });
    return;
  }
  const lower = term.toLowerCase();
  allLis.forEach(li => {
    const nameEl = li.querySelector(':scope > .row .name');
    const isMatch = nameEl && nameEl.textContent.toLowerCase().includes(lower);
    li.dataset.match = isMatch ? '1' : '0';
  });
  allLis.forEach(li => {
    const anyDescendantMatches = li.querySelector('li.node[data-match="1"]') !== null;
    const isSelfMatch = li.dataset.match === '1';
    const show = isSelfMatch || anyDescendantMatches;
    li.classList.toggle('hidden-by-search', !show);
    if (show) {
      const ul = li.querySelector(':scope > ul.node-children');
      if (ul && anyDescendantMatches) {
        ul.classList.remove('collapsed');
        const caret = li.querySelector(':scope > .row .caret');
        if (caret) caret.textContent = '\u25BE';
      }
    }
  });
}

renderKpis();
renderTree();
document.getElementById('searchBox').addEventListener('input', (e) => applyFilter(e.target.value.trim()));
</script>
</body>
</html>
'@

$outFile = Join-Path $OutputDir $OutputFileName
$moduleLineHtml = if ($moduleDisplay) {
    '<div class="module-line">Code coverage measured for: <span class="module-badge">' + [System.Net.WebUtility]::HtmlEncode($moduleDisplay) + '</span></div>'
} else {
    ''
}
$html = $htmlTemplate.Replace('__TITLE__', [System.Net.WebUtility]::HtmlEncode($Title))
$html = $html.Replace('__MODULE_LINE__', $moduleLineHtml)
$html = $html.Replace('__GENERATED_AT__', (Get-Date).ToString('yyyy-MM-dd HH:mm:ss'))
$html = $html.Replace('__TREE_JSON__', $treeJson)
$html = $html.Replace('__SUMMARY_JSON__', $summaryJson)

Set-Content -Path $outFile -Value $html -Encoding UTF8

Write-Host ("`nWrote single-file tree report in {0:N1}s -> {1}" -f $sw.Elapsed.TotalSeconds, $outFile)
if ($moduleDisplay) { Write-Host ("  Module(s):        {0}" -f $moduleDisplay) }
Write-Host ("  Overall: {0}% ({1} / {2} lines across {3} files)" -f $summary.LineCoverage, $summary.CoveredLines, $summary.TotalLines, $summary.Modules)
