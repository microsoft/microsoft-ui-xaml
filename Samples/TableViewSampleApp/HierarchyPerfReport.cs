using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;

namespace TableViewSampleApp;

// Builds a self-contained HTML report over one or more hierarchy perf results.json documents. The
// documents are embedded as-is; the page can also load more results.json files (picker or drag-drop)
// so runs from different branches, builds or machines can be compared side by side against a
// chosen baseline.
internal static class HierarchyPerfReport
{
    public static string Build(string title, IEnumerable<string> jsonDocuments)
    {
        // "</" would end the <script> element early; "<\/" is the same string to JavaScript.
        var data = "[" + string.Join(",\n", jsonDocuments.Select(d => d.Replace("</", "<\\/"))) + "]";
        return Template
            .Replace("__TITLE__", WebUtility.HtmlEncode(title))
            .Replace("__GENERATED__", DateTimeOffset.Now.ToString("yyyy-MM-dd HH:mm:ss zzz"))
            .Replace("__DATA__", data);
    }

    private const string Template = """
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>__TITLE__</title>
<style>
body { font: 13px "Segoe UI", system-ui, sans-serif; margin: 16px; color: #222; }
h1 { font-size: 20px; margin: 0 0 4px; }
h2 { font-size: 16px; margin: 20px 0 6px; }
table { border-collapse: collapse; margin: 4px 0; }
th, td { border: 1px solid #ddd; padding: 3px 6px; text-align: right; white-space: nowrap; }
th { background: #f3f3f3; position: sticky; top: 0; }
td.l, th.l { text-align: left; }
.better { color: #0a7d22; font-weight: 600; }
.worse { color: #c62828; font-weight: 600; }
.err { color: #c62828; }
.muted { color: #777; font-weight: normal; }
.bar { background: linear-gradient(to right, #dbe9ff var(--w), transparent var(--w)); }
.controls { margin: 8px 0; }
.controls label { margin-right: 12px; }
#drop { border: 2px dashed #aaa; padding: 8px; margin: 8px 0; }
</style>
</head>
<body>
<h1>__TITLE__</h1>
<div class="muted" id="gen"></div>
<div id="drop">Drop more <b>results.json</b> files here, or <input type="file" id="file" multiple accept=".json">. They are added to the comparison.</div>

<h2>Runs <span class="muted">(pick the baseline; ratios are run / baseline)</span></h2>
<table id="runs"></table>

<div class="controls">
  <label>Metric <select id="metric"></select></label>
  <label>Shape <select id="fshape"></select></label>
  <label>Key <select id="fkey"></select></label>
  <label>Step contains <input id="fstep" list="steps" size="16"></label><datalist id="steps"></datalist>
  <label>Highlight beyond &plusmn;<input id="thr" type="number" value="5" style="width:4em">%</label>
  <button id="csv">Download comparison CSV</button>
</div>

<h2>Summary vs baseline <span class="muted">(geometric mean of per-row ratios; &lt; 1 is faster / smaller; &darr; better, &uarr; worse rows)</span></h2>
<table id="summary"></table>

<h2>Comparison</h2>
<div class="controls"><label><b>Rows (n)</b> <select id="rows"></select></label></div>
<table id="cmp"></table>

<h2>All metrics for <select id="drun"></select></h2>
<table id="details"></table>

<script>
const EMBEDDED = __DATA__;
const SCHEMA = "tableview-hierarchy-perf/1";
const METRICS = [
  ["frame", "Frame ms (to next frame)"], ["layout", "Layout ms (call + layout)"], ["call", "Call ms (API only)"],
  ["frameMax", "Frame max ms"], ["keyCalls", "Key selector calls"], ["parentCalls", "Parent / children selector calls"],
  ["otherCalls", "Other selector calls"], ["managedKb", "Managed delta KB"], ["privateKb", "Private delta KB"],
  ["rows", "Visible rows"]];
const NEUTRAL = new Set(["rows"]);
const runs = [];
let baseline = 0;

const $ = id => document.getElementById(id);
const esc = s => String(s ?? "").replace(/[&<>"]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
const fmt = v => v == null || isNaN(v) ? "&ndash;" : Math.abs(v) >= 100 ? v.toFixed(0) : Math.abs(v) >= 10 ? v.toFixed(1) : v.toFixed(2);
const rowKey = r => r.n + "|" + r.shape + "|" + r.key + "|" + r.step;
const shapeStep = r => r.n + "|" + r.shape + "|" + r.step;
const NO_KEY = "n/a";
const STEP_TIERS = [
  ["interaction", ["S5 expand root 0 (one level)", "S5 collapse root 0 (expanded subtree)"]],
  ["edit", ["S9 append leaf", "S9 insert leaf (middle)", "S9 remove leaf (middle)", "S9 reparent leaf (middle)", "S9 append root"]],
  ["shaping", ["S10 expanded: Sort", "S10 expanded: Filter", "S10 expanded: GroupBy"]],
  ["bulk", ["S6 ExpandAllRows", "S6 CollapseAllRows"]],
  ["shaping", ["S4 collapsed: Sort", "S4 collapsed: Filter", "S4 collapsed: GroupBy"]],
  ["shaping", ["S2 flat: Sort"]],
  ["setup", ["S3 declare hierarchy", "S10 expanded: clear hierarchy", "S2 flat: KeyBy", "S1 flat: assign source"]]
];
const STEP_ORDER = STEP_TIERS.flatMap(([tier, patterns]) => patterns.map(pattern => ({ tier, pattern })));

function stepInfo(step) {
  const rank = STEP_ORDER.findIndex(x => step.startsWith(x.pattern));
  return rank >= 0 ? { rank, tier: STEP_ORDER[rank].tier } : { rank: STEP_ORDER.length, tier: "other" };
}

function orderedRows(entries) {
  return entries.map((entry, index) => ({ entry, index, rank: stepInfo(entry[1].step).rank }))
    .sort((a, b) => a.rank - b.rank || a.index - b.index)
    .map(x => x.entry);
}

function orderedSteps(rows) {
  const seen = new Map();
  for (const x of rows) if (!seen.has(x.step)) seen.set(x.step, x);
  return orderedRows([...seen.values()].map(x => [rowKey(x), x])).map(([, x]) => x.step);
}

function addRun(doc) {
  if (!doc || doc.schema !== SCHEMA) { alert("Not a hierarchy perf results.json (schema " + SCHEMA + ")."); return; }
  const base = doc.variant + " @ " + String(doc.started || "").replace("T", " ").slice(0, 19);
  let label = base, i = 2;
  while (runs.some(r => r.label === label)) label = base + " #" + (i++);
  doc.label = label;
  doc.index = new Map(doc.results.map(r => [rowKey(r), r]));
  runs.push(doc);
}

function setOptions(sel, values) {
  const cur = sel.value;
  sel.innerHTML = values.map(v => `<option>${esc(v)}</option>`).join("");
  if (values.includes(cur)) sel.value = cur;
}

function setRowsOptions(ns) {
  const sel = $("rows"), cur = sel.value, values = ns.map(String);
  sel.innerHTML = values.map(v => `<option value="${esc(v)}">${esc(v)}</option>`).join("") + '<option value="all">All</option>';
  sel.value = values.includes(cur) || cur === "all" ? cur : (values.length ? values[values.length - 1] : "all");
}

// A run without row keys (the children-selector API) records key "n/a"; it is listed only when no
// run has keyed rows for the same n/shape/step, and otherwise compared against every key type.
function allRows() {
  const keyed = new Set();
  for (const r of runs) for (const x of r.results) if (x.key !== NO_KEY) keyed.add(shapeStep(x));
  const seen = new Map();
  for (const r of runs) for (const x of r.results) {
    if (x.key === NO_KEY && keyed.has(shapeStep(x))) continue;
    const k = rowKey(x);
    if (!seen.has(k)) seen.set(k, x);
  }
  return [...seen.entries()];
}

function rowOf(run, key) {
  const r = run.index.get(key);
  if (r) return r;
  const [n, shape, , ...step] = key.split("|");
  return run.index.get(n + "|" + shape + "|" + NO_KEY + "|" + step.join("|"));
}

function filtered() {
  const n = $("rows").value, s = $("fshape").value, k = $("fkey").value, t = $("fstep").value.toLowerCase();
  return orderedRows(allRows().filter(([, x]) =>
    (n === "all" || String(x.n) === n) && (s === "all" || x.shape === s) &&
    (k === "all" || x.key === k) && (!t || x.step.toLowerCase().includes(t))));
}

// Errored steps (e.g. an edit the control never applied) have no comparable cost.
function value(run, key, metric) { const r = rowOf(run, key); return r && r.median && !r.median.error ? r.median[metric] ?? null : null; }
function ratio(a, b) { return a != null && b != null && b > 0 ? a / b : null; }
function geomean(xs) {
  xs = xs.filter(x => x != null && isFinite(x) && x > 0);
  return xs.length ? Math.exp(xs.reduce((s, x) => s + Math.log(x), 0) / xs.length) : null;
}
function cls(q, metric) {
  const t = (+$("thr").value || 0) / 100;
  if (NEUTRAL.has(metric) || q == null || !isFinite(q)) return "";
  return q < 1 - t ? "better" : q > 1 + t ? "worse" : "";
}
const header = () => runs.map((r, i) => `<th>${esc(r.label)}${i === baseline ? " (base)" : ""}</th>`).join("");

function renderRuns() {
  $("runs").innerHTML = "<tr><th>Base</th><th class=l>Variant</th><th class=l>API</th><th class=l>Status</th><th class=l>Started</th><th class=l>Sample</th>" +
    "<th class=l>Tabular SHA-256</th><th class=l>Machine</th><th>CPUs</th><th>Rows</th><th></th></tr>" +
    runs.map((r, i) => {
      const e = r.environment || {};
      return `<tr><td><input type=radio name=base ${i === baseline ? "checked" : ""} data-i=${i}></td>` +
        `<td class=l>${esc(r.variant)}</td><td class=l>${esc(e.hierarchyApi)}</td><td class=l>${esc(r.status)}</td><td class=l>${esc(r.started)}</td>` +
        `<td class=l>${esc(e.sampleConfiguration)}</td><td class=l title="${esc(e["Microsoft.UI.Xaml.Controls.Tabular.dll"])}">${esc(String(e.tabularSha256 || "").slice(0, 16))}</td>` +
        `<td class=l>${esc(e.machine)}</td><td>${esc(e.cpus)}</td><td>${r.results.length}</td>` +
        `<td><button data-rm=${i}>remove</button></td></tr>`;
    }).join("");
  document.querySelectorAll("input[name=base]").forEach(b => b.onchange = () => { baseline = +b.dataset.i; render(); });
  document.querySelectorAll("button[data-rm]").forEach(b => b.onclick = () => {
    runs.splice(+b.dataset.rm, 1);
    if (baseline >= runs.length) baseline = 0;
    render();
  });
}

function renderSummary(metric, list) {
  const base = runs[baseline];
  let h = "<tr><th class=l>Tier</th><th class=l>Step</th>" + header() + "</tr>";
  for (const s of [...new Set(list.map(([, x]) => x.step)), "ALL"]) {
    const info = s === "ALL" ? { tier: "" } : stepInfo(s);
    h += `<tr><td class=l>${esc(info.tier)}</td><td class=l>${esc(s)}</td>` + runs.map((r, i) => {
      if (i === baseline) return "<td>1.00x</td>";
      const qs = list.filter(([, x]) => s === "ALL" || x.step === s).map(([k]) => ratio(value(r, k, metric), value(base, k, metric)));
      const g = geomean(qs);
      const better = qs.filter(q => cls(q, metric) === "better").length, worse = qs.filter(q => cls(q, metric) === "worse").length;
      return `<td class="${cls(g, metric)}">${g == null ? "&ndash;" : g.toFixed(2) + "x"} <span class=muted>(${better}&darr; ${worse}&uarr;)</span></td>`;
    }).join("") + "</tr>";
  }
  $("summary").innerHTML = h;
}

function renderComparison(metric, list) {
  let h = "<tr><th>n</th><th class=l>shape</th><th class=l>key</th><th class=l>tier</th><th class=l>step</th>" + header() + "</tr>";
  for (const [k, x] of list) {
    const vals = runs.map(r => value(r, k, metric));
    const max = Math.max(0, ...vals.filter(v => v != null && isFinite(v)));
    const b = vals[baseline];
    h += `<tr><td>${x.n}</td><td class=l>${esc(x.shape)}</td><td class=l>${esc(x.key)}</td><td class=l>${esc(stepInfo(x.step).tier)}</td><td class=l>${esc(x.step)}</td>` +
      runs.map((r, i) => {
        const v = vals[i], row = rowOf(r, k), err = row && row.median && row.median.error;
        const w = max > 0 && v != null ? Math.max(0, v / max * 100) : 0;
        let delta = "";
        if (i !== baseline && v != null && b != null && b !== 0) {
          const q = v / b;
          delta = ` <span class="${cls(q, metric)}">(${q >= 1 ? "+" : ""}${((q - 1) * 100).toFixed(0)}%)</span>`;
        }
        return `<td class=bar style="--w:${w.toFixed(1)}%" title="${esc(err || "")}">${fmt(v)}${delta}${err ? " <span class=err>&#9888;</span>" : ""}</td>`;
      }).join("") + "</tr>";
  }
  $("cmp").innerHTML = h;
}

function renderDetails() {
  const r = runs.find(x => x.label === $("drun").value) || runs[0];
  if (!r) { $("details").innerHTML = ""; return; }
  let h = "<tr><th>n</th><th class=l>shape</th><th class=l>key</th><th class=l>tier</th><th class=l>step</th>" +
    METRICS.map(([k]) => `<th>${k}</th>`).join("") + "<th>frame spread</th><th>runs</th><th class=l>error</th></tr>";
  for (const [, x] of orderedRows(r.results.map(x => [rowKey(x), x]))) {
    const md = x.median || {};
    const frames = (x.runs || []).map(s => s.frame).filter(v => v != null);
    const spread = frames.length > 1 && md.frame ? ((Math.max(...frames) - Math.min(...frames)) / md.frame * 100).toFixed(0) + "%" : "&ndash;";
    h += `<tr><td>${x.n}</td><td class=l>${esc(x.shape)}</td><td class=l>${esc(x.key)}</td><td class=l>${esc(stepInfo(x.step).tier)}</td><td class=l>${esc(x.step)}</td>` +
      METRICS.map(([k]) => `<td>${fmt(md[k])}</td>`).join("") +
      `<td>${spread}</td><td>${(x.runs || []).length}</td><td class="l err">${esc(md.error || "")}</td></tr>`;
  }
  $("details").innerHTML = h;
}

function render() {
  if (!runs.length) { $("runs").innerHTML = "<tr><td>No runs loaded.</td></tr>"; return; }
  renderRuns();
  const rows = allRows().map(e => e[1]);
  setRowsOptions([...new Set(rows.map(r => +r.n))].sort((a, b) => a - b));
  setOptions($("fshape"), ["all", ...new Set(rows.map(r => r.shape))]);
  setOptions($("fkey"), ["all", ...new Set(rows.map(r => r.key))]);
  setOptions($("drun"), runs.map(r => r.label));
  $("steps").innerHTML = orderedSteps(rows).map(s => `<option value="${esc(s)}"></option>`).join("");
  const metric = $("metric").value, list = filtered();
  renderSummary(metric, list);
  renderComparison(metric, list);
  renderDetails();
}

function csvCell(v) { v = String(v ?? ""); return /[",\n]/.test(v) ? '"' + v.replace(/"/g, '""') + '"' : v; }
function toCsv() {
  const metric = $("metric").value, base = runs[baseline];
  const head = ["metric", "n", "shape", "key", "tier", "step", ...runs.flatMap((r, i) => i === baseline ? [r.label] : [r.label, r.label + " ratio"])];
  const lines = [head.map(csvCell).join(",")];
  for (const [k, x] of filtered()) {
    const b = value(base, k, metric);
    lines.push([metric, x.n, x.shape, x.key, stepInfo(x.step).tier, x.step, ...runs.flatMap((r, i) => {
      const v = value(r, k, metric);
      return i === baseline ? [v ?? ""] : [v ?? "", v != null && b ? (v / b).toFixed(4) : ""];
    })].map(csvCell).join(","));
  }
  return lines.join("\n");
}

function readFiles(files) {
  let pending = files.length;
  for (const f of files) {
    const reader = new FileReader();
    reader.onload = () => {
      try { addRun(JSON.parse(reader.result)); } catch (e) { alert(f.name + ": " + e); }
      if (--pending === 0) render();
    };
    reader.readAsText(f);
  }
}

$("metric").innerHTML = METRICS.map(([k, l]) => `<option value="${k}">${l}</option>`).join("");
for (const id of ["metric", "rows", "fshape", "fkey", "thr"]) $(id).onchange = render;
$("fstep").oninput = render;
$("drun").onchange = renderDetails;
$("file").onchange = e => readFiles([...e.target.files]);
$("drop").ondragover = e => e.preventDefault();
$("drop").ondrop = e => { e.preventDefault(); readFiles([...e.dataTransfer.files]); };
$("csv").onclick = () => {
  const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([toCsv()], { type: "text/csv" }));
  a.download = "hierarchy-perf-compare.csv";
  a.click();
};
$("gen").textContent = "Generated __GENERATED__. Lower is better for every metric except visible rows. Frame includes the next frame tick, so it is the user-visible cost.";
EMBEDDED.forEach(addRun);
render();
</script>
</body>
</html>
""";
}
