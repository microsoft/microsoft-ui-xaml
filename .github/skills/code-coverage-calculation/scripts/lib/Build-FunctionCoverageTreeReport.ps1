# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Build-FunctionCoverageTreeReport.ps1
#
# Renders a SINGLE, self-contained HTML file that extends Build-CoverageTreeReport.ps1's
# collapsible folder/file tree with one more level: expand a FILE row to reveal the individual
# C++ functions defined in it (from Get-FunctionCoverage.ps1's corrected-per-function.csv), each
# with its own covered/total lines and %.
#
# The folder/file levels (and their rollup percentages/coloring) are built from
# corrected-per-file.csv exactly like Build-CoverageTreeReport.ps1 - i.e. they still show the
# Azure-DevOps-verified, authoritative file/folder totals. The function level underneath a file
# is a separate, ADDITIONAL data source (corrected-per-function.csv) laid on top purely for
# drill-down - it is NOT summed into the file/folder rollup, and deliberately so: in C++, many
# distinct compiled functions (template instantiations, inlined helpers, macro expansions) can
# legitimately share the same physical source line, so summing per-function totals does not
# reproduce a file's true unique-line total (see Get-FunctionCoverage.ps1's header comment for
# the full explanation and a worked example). This report keeps both views honest by computing
# them independently and never mixing one into the other.
#
# PERFORMANCE: a full WinUI build can have 250k+ functions. Folder/file rows are built eagerly
# (same as Build-CoverageTreeReport.ps1 - a few thousand nodes, cheap), but each file's function
# list is only built into the DOM the first time that file row is expanded (lazy), so the report
# stays fast to load even though the underlying embedded JSON contains every function.
#
# OUTPUT: exactly one .html file - no external assets, works fully offline.
#
# EXAMPLE
#   .\Build-FunctionCoverageTreeReport.ps1 `
#       -InputPerFileCsv CorrectedReport\corrected-per-file.csv `
#       -InputPerFunctionCsv FunctionCoverage\corrected-per-function.csv `
#       -InputSummaryJson CorrectedReport\corrected-summary.json `
#       -OutputDir FunctionCoverage\TreeReport `
#       -ModuleNames Microsoft.ui.xaml.dll

[CmdletBinding()]
param(
    # Path to corrected-per-file.csv, as produced by Repair-CoverageDuplicates.ps1 /
    # Split-CoverageByModule.ps1. Drives the folder/file levels of the tree (authoritative
    # totals - matches the Azure DevOps Code Coverage tab).
    [Parameter(Mandatory = $true)]
    [string]$InputPerFileCsv,

    # Path to corrected-per-function.csv, as produced by Get-FunctionCoverage.ps1. Drives the
    # function-level drill-down nested under each file row.
    [Parameter(Mandatory = $true)]
    [string]$InputPerFunctionCsv,

    # Path to corrected-summary.json (for the overall KPI header). Optional - if omitted or
    # missing, the KPI header is computed from the per-file rows instead.
    [string]$InputSummaryJson = "",

    # Folder to write the single HTML report into. Created if it doesn't exist.
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Report title shown at the top of the page.
    [string]$Title = "Function Coverage Report",

    # Regex matched against the START of each file path and stripped before display - same
    # convention/default as Build-CoverageTreeReport.ps1. Applied identically to both input CSVs
    # so file paths line up between them.
    [string]$StripPathPrefix = '^[A-Za-z]:[\\/]+(__w|a)[\\/]+\d+[\\/]+s[\\/]+',

    # Output file name.
    [string]$OutputFileName = 'function-coverage-tree.html',

    # Name(s) of the instrumented product DLL(s) this coverage measures. Same semantics as
    # Build-CoverageTreeReport.ps1's -ModuleNames.
    [string[]]$ModuleNames = @()
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $InputPerFileCsv)) { throw "Input per-file CSV not found: $InputPerFileCsv" }
if (-not (Test-Path $InputPerFunctionCsv)) { throw "Input per-function CSV not found: $InputPerFunctionCsv" }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Write-Host "Building single-file function-drill-down tree report..."
Write-Host "  Per-file (authoritative):   $InputPerFileCsv"
Write-Host "  Per-function (drill-down):  $InputPerFunctionCsv"
$sw = [System.Diagnostics.Stopwatch]::StartNew()

$fileRows = Import-Csv $InputPerFileCsv
if ($fileRows.Count -eq 0) { throw "No rows found in $InputPerFileCsv - nothing to render." }
$functionRows = Import-Csv $InputPerFunctionCsv
Write-Host ("Loaded {0:N0} file row(s) and {1:N0} function row(s)." -f $fileRows.Count, $functionRows.Count)

$stripRegex = $null
if ($StripPathPrefix) {
    try { $stripRegex = New-Object System.Text.RegularExpressions.Regex($StripPathPrefix, 'IgnoreCase') }
    catch { Write-Host "WARNING: -StripPathPrefix is not a valid regex ('$StripPathPrefix'); paths will be shown unmodified. $($_.Exception.Message)" -ForegroundColor Yellow }
}

function Get-NormalizedPath {
    param([string]$Path)
    $p = $Path
    if ($stripRegex -and $stripRegex.IsMatch($p)) { $p = $stripRegex.Replace($p, '', 1) }
    return ($p -replace '/', '\')
}

# ---------------------------------------------------------------------------------------------
# 1. Group corrected-per-function.csv by (normalized) file path, sorting each file's own
#    function list worst-covered-first (most useful for spotting gaps) - ties broken by name.
#    Kept as plain arrays of compact records; NOT summed into anything - purely for drill-down.
# ---------------------------------------------------------------------------------------------
Write-Host "Grouping functions by file..."
$functionsByFile = @{}
foreach ($fr in $functionRows) {
    $key = Get-NormalizedPath -Path $fr.File
    if (-not $functionsByFile.ContainsKey($key)) { $functionsByFile[$key] = New-Object System.Collections.Generic.List[object] }
    $total = [int]$fr.TotalLines
    $covered = [int]$fr.CoveredLines
    $pct = if ($total -gt 0) { [math]::Round(100.0 * $covered / $total, 2) } else { 0 }
    $displayName = if ($fr.Type -and $fr.Type -ne '') { "$($fr.Type)::$($fr.Function)" } else { $fr.Function }
    $tooltip = "$($fr.Namespace)::$($fr.Type)::$($fr.Function)" -replace '^::+', ''
    [void]$functionsByFile[$key].Add([pscustomobject]@{
        n = $displayName
        q = $tooltip
        c = $covered
        t = $total
        p = $pct
    })
}
foreach ($k in @($functionsByFile.Keys)) {
    $functionsByFile[$k] = @($functionsByFile[$k] | Sort-Object p, n)
}
Write-Host ("Grouped functions for {0:N0} distinct file(s)." -f $functionsByFile.Count)

# ---------------------------------------------------------------------------------------------
# 2. Build the folder/file tree from corrected-per-file.csv - identical logic/rollup to
#    Build-CoverageTreeReport.ps1 - then attach each file node's function list (if any) as a
#    separate 'fk' (function-kids) property, kept apart from folder 'k' children.
# ---------------------------------------------------------------------------------------------
function New-TreeNode {
    param([string]$Name, [bool]$IsFile)
    [pscustomobject]@{
        Name      = $Name
        IsFile    = $IsFile
        Covered   = 0
        Total     = 0
        Functions = $null
        Children  = [ordered]@{}
    }
}

$root = New-TreeNode -Name '' -IsFile $false
$strippedCount = 0
$totalCount = 0

foreach ($row in $fileRows) {
    $totalCount++
    $rawPath = $row.File
    $path = Get-NormalizedPath -Path $rawPath
    if ($path -ne $rawPath) { $strippedCount++ }
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
    $node.Covered = $covered
    $node.Total = $total
    $fns = $null
    if ($functionsByFile.ContainsKey($path)) { $fns = $functionsByFile[$path] }
    $node.Functions = $fns
}

Write-Host ("Stripped the boilerplate prefix from {0}/{1} file paths." -f $strippedCount, $totalCount)

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
        if ($Node.Functions -and $Node.Functions.Count -gt 0) { $obj['fk'] = $Node.Functions }
    } else {
        $sortedChildren = $Node.Children.Values | Sort-Object @{Expression = { $_.IsFile } }, @{Expression = { $_.Name } }
        $obj['k'] = @($sortedChildren | ForEach-Object { ConvertTo-JsonNode -Node $_ })
    }
    return $obj
}

$treeData = @($root.Children.Values | Sort-Object @{Expression = { $_.IsFile } }, @{Expression = { $_.Name } } | ForEach-Object { ConvertTo-JsonNode -Node $_ })
$treeJson = $treeData | ConvertTo-Json -Depth 100 -Compress
if ($treeData.Count -eq 1) { $treeJson = "[$treeJson]" }

# ---------------------------------------------------------------------------------------------
# 3. KPI header - file-level KPIs from the (already-verified) summary/per-file rows, plus new
#    function-level KPIs computed straight from corrected-per-function.csv.
# ---------------------------------------------------------------------------------------------
$summary = $null
if ($InputSummaryJson -and (Test-Path $InputSummaryJson)) {
    $summary = Get-Content $InputSummaryJson -Raw | ConvertFrom-Json
}
if (-not $summary) {
    $totalLines = ($fileRows | Measure-Object -Property TotalLines -Sum).Sum
    $coveredLines = ($fileRows | Measure-Object -Property CoveredLines -Sum).Sum
    $summary = [pscustomobject]@{
        Modules      = $fileRows.Count
        TotalLines   = $totalLines
        CoveredLines = $coveredLines
        LineCoverage = if ($totalLines -gt 0) { [math]::Round(100.0 * $coveredLines / $totalLines, 2) } else { 0 }
    }
}

$totalFunctions = $functionRows.Count
$functionsWithCoverage = @($functionRows | Where-Object { [int]$_.CoveredLines -gt 0 }).Count
$functionsFullyCovered = @($functionRows | Where-Object { [int]$_.TotalLines -gt 0 -and [int]$_.CoveredLines -eq [int]$_.TotalLines }).Count

$resolvedModuleNames = @($ModuleNames)
if ($resolvedModuleNames.Count -eq 0 -and $summary.PSObject.Properties.Name -contains 'InstrumentedModules') {
    $resolvedModuleNames = @($summary.InstrumentedModules)
}
$moduleDisplay = if ($resolvedModuleNames -and $resolvedModuleNames.Count -gt 0) { ($resolvedModuleNames -join ', ') } else { '' }

$summaryObj = [ordered]@{
    LineCoverage           = $summary.LineCoverage
    CoveredLines           = $summary.CoveredLines
    TotalLines             = $summary.TotalLines
    Modules                = $summary.Modules
    TotalFunctions         = $totalFunctions
    FunctionsWithCoverage  = $functionsWithCoverage
    FunctionsFullyCovered  = $functionsFullyCovered
}
$summaryJson = ($summaryObj | ConvertTo-Json -Compress)

# ---------------------------------------------------------------------------------------------
# HTML template. Single-quoted here-string (no PowerShell variable expansion).
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
    --fn-bg: #fbfcfd;
  }
  * { box-sizing: border-box; }
  body { font-family: -apple-system, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; margin: 0; padding: 24px; background: var(--bg); color: #1f2328; }
  h1 { font-size: 20px; margin: 0 0 4px 0; }
  .subtitle { color: var(--muted); font-size: 13px; margin-bottom: 20px; }
  .kpis { display: flex; gap: 16px; margin-bottom: 20px; flex-wrap: wrap; }
  .kpi { border: 1px solid var(--border); border-radius: 8px; padding: 14px 18px; min-width: 140px; }
  .kpi .label { font-size: 12px; color: var(--muted); text-transform: uppercase; letter-spacing: .03em; }
  .kpi .value { font-size: 26px; font-weight: 600; margin-top: 4px; }
  .note { font-size: 12px; color: var(--muted); background: #fff8e1; border: 1px solid #f0e2b3; border-radius: 6px; padding: 10px 12px; margin-bottom: 16px; }
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
  .row.folder, .row.file.expandable { cursor: pointer; }
  .row.folder:hover, .row.file.expandable:hover { background: var(--row-hover); }
  .caret { width: 14px; display: inline-block; text-align: center; color: var(--muted); font-size: 11px; user-select: none; flex: 0 0 auto; }
  .row.file:not(.expandable) .caret { visibility: hidden; }
  .row.function .caret { visibility: hidden; }
  .name { flex: 1 1 auto; font-size: 13px; font-family: Consolas, "Cascadia Mono", monospace; overflow-wrap: anywhere; padding-right: 12px; }
  .row.folder .name { font-weight: 600; }
  .row.function .name { color: #57606a; }
  .row.function .name .fn-icon { color: #8250df; margin-right: 5px; }
  .row.function { background: var(--fn-bg); }
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

<div class="note">Folder/file percentages and line totals are the authoritative, Azure-DevOps-verified numbers. Expand a file to see its individual functions - each function's own covered/total lines are shown for that function alone, and are <strong>not</strong> summed into the file total above it (in C++, distinct functions such as template instantiations can legitimately share the same physical source line, so a naive sum would double-count).</div>

<div class="legend">
  <span><span class="swatch" style="background:var(--green)"></span>&ge; 75%</span>
  <span><span class="swatch" style="background:var(--yellow)"></span>40&ndash;75%</span>
  <span><span class="swatch" style="background:var(--red)"></span>&lt; 40%</span>
  <span><span class="fn-icon" style="color:#8250df">&#402;</span> = function (click a file with a caret to reveal its functions)</span>
  <span style="margin-left:auto">Search filters folders/files only</span>
</div>

<div id="search">
  <input id="searchBox" type="text" placeholder="Filter by file or folder name..." />
</div>

<div class="tree" id="tree"></div>

<footer>Self-contained report - no external files. File/folder coverage matches the Azure DevOps Code Coverage tab exactly. Function-level detail comes from the coverage merge engine's native (non-Cobertura) output format, which preserves per-function boundaries that Cobertura collapses away.</footer>

<script>
const TREE_DATA = __TREE_JSON__;
const SUMMARY = __SUMMARY_JSON__;

function pctClass(p) {
  if (p >= 75) return 'hi';
  if (p >= 40) return 'mid';
  return 'lo';
}

function fmtInt(n) {
  return (n || 0).toLocaleString('en-US');
}

function renderKpis() {
  const kpis = document.getElementById('kpis');
  const items = [
    ['Line coverage', (SUMMARY.LineCoverage !== undefined ? SUMMARY.LineCoverage : 0) + '%'],
    ['Covered lines', fmtInt(SUMMARY.CoveredLines)],
    ['Total lines', fmtInt(SUMMARY.TotalLines)],
    ['Files', fmtInt(SUMMARY.Modules)],
    ['Functions', fmtInt(SUMMARY.TotalFunctions)],
    ['Functions w/ coverage', fmtInt(SUMMARY.FunctionsWithCoverage)],
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

function buildRow(node, kind) {
  const row = document.createElement('div');
  row.className = 'row ' + kind;

  const caret = document.createElement('span');
  caret.className = 'caret';
  caret.textContent = kind === 'function' ? '' : '\u25B8';
  row.appendChild(caret);

  const name = document.createElement('span');
  name.className = 'name';
  if (kind === 'function') {
    const icon = document.createElement('span');
    icon.className = 'fn-icon';
    icon.textContent = '\u0192';
    name.appendChild(icon);
    const text = document.createTextNode(node.n);
    name.appendChild(text);
    if (node.q) { name.title = node.q; }
  } else {
    name.textContent = node.n;
  }
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

function buildFunctionLi(fn) {
  const li = document.createElement('li');
  li.className = 'node';
  const { row } = buildRow(fn, 'function');
  li.appendChild(row);
  return li;
}

function buildNodeLi(node) {
  const li = document.createElement('li');
  li.className = 'node';
  li.dataset.name = node.n.toLowerCase();

  const hasFolderChildren = !node.f && node.k && node.k.length;
  const hasFunctionChildren = !!(node.f && node.fk && node.fk.length);
  const kind = node.f ? 'file' : 'folder';
  const { row, caret } = buildRow(node, kind);
  if (hasFunctionChildren) { row.classList.add('expandable'); caret.textContent = '\u25B8'; }
  li.appendChild(row);

  if (hasFolderChildren) {
    const childUl = document.createElement('ul');
    childUl.className = 'node-children collapsed';
    node.k.forEach(child => childUl.appendChild(buildNodeLi(child)));
    li.appendChild(childUl);
    row.addEventListener('click', () => {
      const collapsed = childUl.classList.toggle('collapsed');
      caret.textContent = collapsed ? '\u25B8' : '\u25BE';
    });
  } else if (hasFunctionChildren) {
    // Lazy: build the (potentially large) function list only on first expand.
    let built = false;
    let childUl = null;
    row.addEventListener('click', () => {
      if (!built) {
        childUl = document.createElement('ul');
        childUl.className = 'node-children collapsed';
        node.fk.forEach(fn => childUl.appendChild(buildFunctionLi(fn)));
        li.appendChild(childUl);
        built = true;
      }
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

// Filter over folder/file rows only (function rows are lazily built and not indexed here).
function applyFilter(term) {
  const allLis = document.querySelectorAll('#tree li.node');
  if (!term) {
    allLis.forEach(li => {
      li.classList.remove('hidden-by-search');
    });
    return;
  }
  const lower = term.toLowerCase();
  allLis.forEach(li => {
    const nameEl = li.querySelector(':scope > .row .name');
    const isFn = li.querySelector(':scope > .row.function') !== null;
    const isMatch = !isFn && nameEl && nameEl.textContent.toLowerCase().includes(lower);
    li.dataset.match = isMatch ? '1' : '0';
  });
  allLis.forEach(li => {
    const anyDescendantMatches = li.querySelector('li.node[data-match="1"]') !== null;
    const isSelfMatch = li.dataset.match === '1';
    const isFn = li.querySelector(':scope > .row.function') !== null;
    const show = isFn ? true : (isSelfMatch || anyDescendantMatches);
    li.classList.toggle('hidden-by-search', !show);
    if (show && !isFn) {
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

Write-Host ("`nWrote single-file function-drill-down tree report in {0:N1}s -> {1} ({2:N1} MB)" -f $sw.Elapsed.TotalSeconds, $outFile, ((Get-Item $outFile).Length / 1MB))
if ($moduleDisplay) { Write-Host ("  Module(s):        {0}" -f $moduleDisplay) }
Write-Host ("  File-level: {0}% ({1} / {2} lines across {3} files)" -f $summary.LineCoverage, $summary.CoveredLines, $summary.TotalLines, $summary.Modules)
Write-Host ("  Functions:  {0:N0} total, {1:N0} with any coverage, {2:N0} fully covered" -f $totalFunctions, $functionsWithCoverage, $functionsFullyCovered)
