# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# New-CoverageHtmlReport.ps1 - Render one self-contained HTML coverage report with a module
# drop-down (Combined / each instrumented DLL), from the output of Repair-CodeCoverage.ps1.
#
# Each view shows KPI cards and a collapsible folder tree (folders roll up the covered/total
# lines of everything beneath them; files show their own numbers), colored green >= 75%,
# yellow 40-75%, red < 40%, with a name filter. Everything (styles, script, data) is inline,
# so the single file can be opened straight from the downloaded pipeline artifact.
#
# EXPERIMENTAL QUALITY.

[CmdletBinding()]
param(
    # Output directory of Repair-CodeCoverage.ps1 (contains coverage-summary.json and the
    # per-view corrected-per-file.csv files).
    [Parameter(Mandatory = $true)]
    [string]$ReportDir,

    [string]$OutputFile = "",

    [string]$Title = "WinUI Code Coverage",

    # Optional context line (e.g. build number / link) shown under the title.
    [string]$Subtitle = "",

    # Display-only trim of the build-agent prefix (C:\__w\1\s\, D:\a\1\s\, ...). Paths that do
    # not match are shown unchanged.
    [string]$StripPathPrefix = '^[A-Za-z]:[\\/]+(__w|a|_work)[\\/]+\d+[\\/]+s[\\/]+'
)

$ErrorActionPreference = 'Stop'

$summaryPath = Join-Path $ReportDir 'coverage-summary.json'
if (-not (Test-Path $summaryPath)) { throw "Not found: $summaryPath (run Repair-CodeCoverage.ps1 first)." }
if (-not $OutputFile) { $OutputFile = Join-Path $ReportDir 'coverage-report.html' }

# Piped through ForEach-Object so Windows PowerShell 5.1 (which emits a JSON array as a single
# object) still yields one item per view.
$views = @((Get-Content $summaryPath -Raw | ConvertFrom-Json) | ForEach-Object { $_ })
$stripRegex = if ($StripPathPrefix) { New-Object System.Text.RegularExpressions.Regex($StripPathPrefix, 'IgnoreCase') } else { $null }

function ConvertTo-JsString {
    param([string]$s)
    # Escape for a JS string literal; < > & are escaped so data can never close the <script> block.
    $s = $s -replace '\\', '\\' -replace '"', '\"' -replace '<', '\u003c' -replace '>', '\u003e' -replace '&', '\u0026' -replace '[\x00-\x1f]', ''
    return '"' + $s + '"'
}

function New-Node { param([string]$Name) @{ Name = $Name; Covered = 0L; Total = 0L; Folders = @{}; Files = New-Object System.Collections.ArrayList } }

# Tree node JSON: {n:name, c:covered, t:total, f:1 (files only), k:[children] (folders only)}.
function Write-NodeJson {
    param($Node, [System.Text.StringBuilder]$Sb)
    [void]$Sb.Append('{"n":').Append((ConvertTo-JsString $Node.Name)).Append(',"c":').Append($Node.Covered).Append(',"t":').Append($Node.Total).Append(',"k":[')
    $first = $true
    foreach ($key in ($Node.Folders.Keys | Sort-Object)) {
        if (-not $first) { [void]$Sb.Append(',') }
        $first = $false
        Write-NodeJson -Node $Node.Folders[$key] -Sb $Sb
    }
    foreach ($file in ($Node.Files | Sort-Object Name)) {
        if (-not $first) { [void]$Sb.Append(',') }
        $first = $false
        [void]$Sb.Append('{"n":').Append((ConvertTo-JsString $file.Name)).Append(',"c":').Append($file.Covered).Append(',"t":').Append($file.Total).Append(',"f":1}')
    }
    [void]$Sb.Append(']}')
}

$sb = New-Object System.Text.StringBuilder
[void]$sb.Append('[')
$firstView = $true
foreach ($view in $views) {
    $csvPath = Join-Path $ReportDir $view.PerFileCsv
    if (-not (Test-Path $csvPath)) { throw "Per-file CSV for view '$($view.View)' not found: $csvPath" }

    $root = New-Node ''
    foreach ($row in (Import-Csv $csvPath)) {
        $path = $row.File
        if ($stripRegex) { $path = $stripRegex.Replace($path, '', 1) }
        $segments = @(($path -replace '/', '\').Split('\') | Where-Object { $_ })
        if ($segments.Count -eq 0) { continue }
        $covered = [long]$row.CoveredLines
        $total = [long]$row.TotalLines

        $node = $root
        $node.Covered += $covered; $node.Total += $total
        for ($i = 0; $i -lt $segments.Count - 1; $i++) {
            $seg = $segments[$i]
            if (-not $node.Folders.ContainsKey($seg)) { $node.Folders[$seg] = New-Node $seg }
            $node = $node.Folders[$seg]
            $node.Covered += $covered; $node.Total += $total
        }
        [void]$node.Files.Add([pscustomobject]@{ Name = $segments[-1]; Covered = $covered; Total = $total })
    }

    if (-not $firstView) { [void]$sb.Append(',') }
    $firstView = $false
    [void]$sb.Append('{"id":').Append((ConvertTo-JsString $view.View))
    [void]$sb.Append(',"label":').Append((ConvertTo-JsString $view.Label))
    [void]$sb.Append(',"files":').Append([long]$view.Files)
    [void]$sb.Append(',"covered":').Append([long]$view.CoveredLines)
    [void]$sb.Append(',"total":').Append([long]$view.TotalLines)
    [void]$sb.Append(',"tree":')
    Write-NodeJson -Node $root -Sb $sb
    [void]$sb.Append('}')
    Write-Host ("  View '{0}': {1} files" -f $view.Label, $view.Files)
}
[void]$sb.Append(']')

$template = @'
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8" />
<title>__TITLE__</title>
<style>
  :root { --green:#2ea043; --yellow:#b08800; --red:#cf222e; --border:#d0d7de; --muted:#57606a; --hover:#f6f8fa; --accent:#0969da; }
  * { box-sizing: border-box; }
  body { font-family: -apple-system, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; margin: 0; padding: 24px; color: #1f2328; background: #fff; }
  h1 { font-size: 20px; margin: 0 0 4px 0; }
  .subtitle { color: var(--muted); font-size: 13px; margin-bottom: 16px; }
  .picker { display: flex; align-items: center; gap: 10px; margin-bottom: 16px; font-size: 14px; }
  .picker select { font-size: 14px; padding: 6px 10px; border: 1px solid var(--border); border-radius: 6px; min-width: 360px; }
  table.views { border-collapse: collapse; margin-bottom: 20px; font-size: 13px; }
  table.views th, table.views td { border: 1px solid var(--border); padding: 6px 12px; text-align: right; font-variant-numeric: tabular-nums; }
  table.views th:first-child, table.views td:first-child { text-align: left; }
  table.views th { background: var(--hover); font-weight: 600; }
  table.views tr.sel td { background: #ddf4ff; }
  table.views tbody tr { cursor: pointer; }
  table.views tbody tr:hover td { background: var(--hover); }
  .kpis { display: flex; gap: 16px; margin-bottom: 20px; flex-wrap: wrap; }
  .kpi { border: 1px solid var(--border); border-radius: 8px; padding: 14px 18px; min-width: 140px; }
  .kpi .label { font-size: 12px; color: var(--muted); text-transform: uppercase; letter-spacing: .03em; }
  .kpi .value { font-size: 26px; font-weight: 600; margin-top: 4px; }
  .legend { font-size: 12px; color: var(--muted); margin-bottom: 12px; display: flex; gap: 16px; align-items: center; flex-wrap: wrap; }
  .swatch { display: inline-block; width: 10px; height: 10px; border-radius: 2px; margin-right: 5px; vertical-align: middle; }
  #searchBox { width: 100%; max-width: 480px; padding: 8px 10px; border: 1px solid var(--border); border-radius: 6px; font-size: 13px; margin-bottom: 12px; }
  .tree { border: 1px solid var(--border); border-radius: 8px; overflow: hidden; }
  ul.kids { list-style: none; margin: 0; padding: 0 0 0 22px; }
  .tree > ul.kids { padding-left: 0; }
  ul.kids.collapsed { display: none; }
  li.node { border-top: 1px solid var(--border); }
  .tree > ul.kids > li.node:first-child { border-top: none; }
  .row { display: flex; align-items: center; padding: 7px 10px; }
  .row.folder { cursor: pointer; }
  .row.folder:hover { background: var(--hover); }
  .caret { width: 14px; text-align: center; color: var(--muted); font-size: 11px; user-select: none; flex: 0 0 auto; }
  .name { flex: 1 1 auto; font-size: 13px; font-family: Consolas, "Cascadia Mono", monospace; overflow-wrap: anywhere; padding-right: 12px; }
  .row.folder .name { font-weight: 600; }
  .bar { flex: 0 0 140px; height: 8px; background: #eaeef2; border-radius: 4px; overflow: hidden; margin-right: 10px; }
  .fill { display: block; height: 100%; }
  .pct { flex: 0 0 62px; text-align: right; font-size: 12px; font-weight: 600; font-variant-numeric: tabular-nums; }
  .lines { flex: 0 0 130px; text-align: right; font-size: 11px; color: var(--muted); font-variant-numeric: tabular-nums; padding-left: 10px; }
  .hi { color: var(--green); } .fill.hi { background: var(--green); }
  .mid { color: var(--yellow); } .fill.mid { background: var(--yellow); }
  .lo { color: var(--red); } .fill.lo { background: var(--red); }
  .hidden { display: none !important; }
  footer { margin-top: 20px; font-size: 11px; color: var(--muted); max-width: 900px; }
</style>
</head>
<body>
<h1>__TITLE__</h1>
<div class="subtitle">__SUBTITLE__</div>

<div class="picker">
  <label for="viewSelect"><b>Module:</b></label>
  <select id="viewSelect"></select>
</div>

<table class="views" id="viewsTable">
  <thead><tr><th>Module</th><th>Line coverage</th><th>Covered lines</th><th>Total lines</th><th>Files</th></tr></thead>
  <tbody></tbody>
</table>

<div class="kpis" id="kpis"></div>

<div class="legend">
  <span><span class="swatch" style="background:var(--green)"></span>&ge; 75%</span>
  <span><span class="swatch" style="background:var(--yellow)"></span>40&ndash;75%</span>
  <span><span class="swatch" style="background:var(--red)"></span>&lt; 40%</span>
  <span style="margin-left:auto">Click a folder to expand/collapse it</span>
</div>

<input id="searchBox" type="text" placeholder="Filter by file or folder name..." />
<div class="tree" id="tree"></div>

<footer>
  Line coverage, deduplicated by (source file, line number) to correct a duplicate-line defect in the
  native coverage merge engine; these numbers match the Azure DevOps Code Coverage tab.
  "Combined" counts a source file compiled into more than one DLL (e.g. a shared header) once, so the
  per-DLL totals need not add up to the combined total.
</footer>

<script>
const VIEWS = __VIEWS_JSON__;

function pct(c, t) { return t > 0 ? 100 * c / t : 0; }
function cls(p) { return p >= 75 ? 'hi' : (p >= 40 ? 'mid' : 'lo'); }
function fmt(n) { return n.toLocaleString('en-US'); }
function el(tag, className, text) {
  const e = document.createElement(tag);
  if (className) e.className = className;
  if (text !== undefined) e.textContent = text;
  return e;
}

function renderTable(current) {
  const tbody = document.querySelector('#viewsTable tbody');
  tbody.textContent = '';
  VIEWS.forEach((v, i) => {
    const p = pct(v.covered, v.total);
    const tr = el('tr', i === current ? 'sel' : '');
    tr.appendChild(el('td', '', v.label));
    tr.appendChild(el('td', cls(p), p.toFixed(2) + '%'));
    tr.appendChild(el('td', '', fmt(v.covered)));
    tr.appendChild(el('td', '', fmt(v.total)));
    tr.appendChild(el('td', '', fmt(v.files)));
    tr.addEventListener('click', () => selectView(i));
    tbody.appendChild(tr);
  });
}

function renderKpis(v) {
  const kpis = document.getElementById('kpis');
  kpis.textContent = '';
  [['Line coverage', pct(v.covered, v.total).toFixed(2) + '%'], ['Covered lines', fmt(v.covered)],
   ['Total lines', fmt(v.total)], ['Files', fmt(v.files)]].forEach(([label, value]) => {
    const d = el('div', 'kpi');
    d.appendChild(el('div', 'label', label));
    d.appendChild(el('div', 'value', value));
    kpis.appendChild(d);
  });
}

function buildNode(node) {
  const li = el('li', 'node');
  const p = pct(node.c, node.t);
  const row = el('div', 'row ' + (node.f ? 'file' : 'folder'));
  const caret = el('span', 'caret', node.f ? '' : '\u25B8');
  row.appendChild(caret);
  row.appendChild(el('span', 'name', node.n));
  const bar = el('span', 'bar');
  const fill = el('span', 'fill ' + cls(p));
  fill.style.width = Math.min(p, 100) + '%';
  bar.appendChild(fill);
  row.appendChild(bar);
  row.appendChild(el('span', 'pct ' + cls(p), p.toFixed(1) + '%'));
  row.appendChild(el('span', 'lines', fmt(node.c) + ' / ' + fmt(node.t)));
  li.appendChild(row);
  if (!node.f && node.k && node.k.length) {
    const ul = el('ul', 'kids collapsed');
    node.k.forEach(k => ul.appendChild(buildNode(k)));
    li.appendChild(ul);
    row.addEventListener('click', () => {
      const collapsed = ul.classList.toggle('collapsed');
      caret.textContent = collapsed ? '\u25B8' : '\u25BE';
    });
  }
  return li;
}

function renderTree(v) {
  const tree = document.getElementById('tree');
  tree.textContent = '';
  const ul = el('ul', 'kids');
  (v.tree.k || []).forEach(k => ul.appendChild(buildNode(k)));
  tree.appendChild(ul);
}

// Matching rows stay visible and their ancestors are expanded; clearing restores the collapsed tree.
function applyFilter(term) {
  const lis = document.querySelectorAll('#tree li.node');
  if (!term) {
    lis.forEach(li => {
      li.classList.remove('hidden');
      const ul = li.querySelector(':scope > ul.kids');
      if (ul) { ul.classList.add('collapsed'); li.querySelector(':scope > .row .caret').textContent = '\u25B8'; }
    });
    return;
  }
  const lower = term.toLowerCase();
  lis.forEach(li => {
    li.dataset.match = li.querySelector(':scope > .row .name').textContent.toLowerCase().includes(lower) ? '1' : '0';
  });
  lis.forEach(li => {
    const descendant = li.querySelector('li.node[data-match="1"]') !== null;
    const show = li.dataset.match === '1' || descendant;
    li.classList.toggle('hidden', !show);
    const ul = li.querySelector(':scope > ul.kids');
    if (show && ul && descendant) { ul.classList.remove('collapsed'); li.querySelector(':scope > .row .caret').textContent = '\u25BE'; }
  });
}

function selectView(i) {
  const v = VIEWS[i];
  document.getElementById('viewSelect').value = String(i);
  renderTable(i);
  renderKpis(v);
  renderTree(v);
  applyFilter(document.getElementById('searchBox').value.trim());
  // Keep the selection in the URL so a link reopens the same view (best-effort; some viewers forbid it).
  try { history.replaceState(null, '', '#view=' + encodeURIComponent(v.id)); } catch (e) { }
}

const select = document.getElementById('viewSelect');
VIEWS.forEach((v, i) => {
  const o = el('option', '', v.label + '  \u2014  ' + pct(v.covered, v.total).toFixed(2) + '%');
  o.value = String(i);
  select.appendChild(o);
});
select.addEventListener('change', e => selectView(Number(e.target.value)));
document.getElementById('searchBox').addEventListener('input', e => applyFilter(e.target.value.trim()));

const m = /view=([^&]+)/.exec(location.hash);
const initial = m ? VIEWS.findIndex(v => v.id === decodeURIComponent(m[1])) : -1;
selectView(initial >= 0 ? initial : 0);
</script>
</body>
</html>
'@

$subtitleText = if ($Subtitle) { $Subtitle } else { "Generated $((Get-Date).ToString('yyyy-MM-dd HH:mm'))" }
$html = $template.Replace('__TITLE__', [System.Net.WebUtility]::HtmlEncode($Title))
$html = $html.Replace('__SUBTITLE__', [System.Net.WebUtility]::HtmlEncode($subtitleText))
$html = $html.Replace('__VIEWS_JSON__', $sb.ToString())

[System.IO.File]::WriteAllText($OutputFile, $html, [System.Text.UTF8Encoding]::new($false))
Write-Host ("Wrote HTML report ({0:N1} MB): {1}" -f ((Get-Item $OutputFile).Length / 1MB), $OutputFile)
