import argparse
import csv
import datetime
import html
import json
import os
import re
import statistics


# Source location for the perf kit (TODO: update to the repo path once the kit is committed).
SOURCE_URL = "https://github.com/microsoft/microsoft-ui-xaml"
SOURCE_NOTE = "the harness (sampler.py + report.py) and the per-control BenchmarkHost"
UNSPECIFIED_CONTROL_VERSION = "unspecified"


NUMERIC_COLUMNS = {
    "rows",
    "cols",
    "run",
    "startup_ms",
    "binary_kb",
    "cpu_avg",
    "cpu_max",
    "priv_mb_avg",
    "priv_mb_max",
    "ws_mb_avg",
    "ws_mb_max",
    "bytes_per_row",
    "handles_avg",
    "handles_max",
    "threads_avg",
    "threads_max",
    "gdi_max",
    "user_max",
    "ctx_switches",
    "gpu_util_avg",
    "gpu_util_max",
    "samples",
    "creation_ms",
    "first_render_ms",
    "fps_avg",
    "frame_p99_ms",
    "jank_pct",
    "frame_drops",
    "realized_rows",
    "total_rows",
    "managed_heap_mb",
    "gc_gen0",
    "gc_gen1",
    "gc_gen2",
    "sort_ms",
    "filter_ms",
    "scenario_ms",
    "realized_cols",
    "realized_cells",
    "frame_p95_ms",
    "frame_max_ms",
    "time_to_interactive_ms",
    "data_gen_ms",
    "control_creation_ms",
    "select_ms",
    "op_ms",
    "setup_expand_ms",
    "visible_rows_before",
    "visible_rows_after",
}

GLOBAL_METRICS = [
    ("Startup ms", "startup_ms", 1),
    ("Binary footprint KB", "binary_kb", 1),
    ("CPU avg %", "cpu_avg", 1),
    ("CPU max %", "cpu_max", 1),
    ("Private MB avg", "priv_mb_avg", 1),
    ("Private MB max", "priv_mb_max", 1),
    ("Working set MB avg", "ws_mb_avg", 1),
    ("Working set MB max", "ws_mb_max", 1),
    ("Bytes per row", "bytes_per_row", 1),
    ("Handles avg", "handles_avg", 0),
    ("Handles max", "handles_max", 0),
    ("Threads avg", "threads_avg", 1),
    ("Threads max", "threads_max", 0),
    ("GDI objects max", "gdi_max", 0),
    ("USER objects max", "user_max", 0),
    ("Context switches", "ctx_switches", 0),
    ("GPU util avg %", "gpu_util_avg", 1),
    ("GPU util max %", "gpu_util_max", 1),
]

BASE_SCENARIO_METRICS = [
    ("Startup ms", "startup_ms", 1),
    ("Time to interactive ms", "time_to_interactive_ms", 1),
    ("Avg CPU %", "cpu_avg", 1),
    ("Avg working set MB", "ws_mb_avg", 1),
    ("Managed heap MB", "managed_heap_mb", 1),
    ("Creation ms", "creation_ms", 1),
    ("First render ms", "first_render_ms", 1),
    ("FPS avg", "fps_avg", 1),
    ("P95 frame ms", "frame_p95_ms", 1),
    ("P99 frame ms", "frame_p99_ms", 1),
    ("Max frame ms", "frame_max_ms", 1),
    ("Jank %", "jank_pct", 1),
    ("Realized / total rows", "realized_total", 0),
    ("Realized cols", "realized_cols", 0),
    ("Realized cells", "realized_cells", 0),
    ("Bytes per row", "bytes_per_row", 1),
]

SCENARIO_DESCRIPTIONS = {
    "load": "Initial load, data generation, control creation, and first-render readiness.",
    "scroll": "Scrolling throughput, frame pacing, jank, and virtualization effectiveness.",
    "sort": "Sort operation latency at each captured dataset size.",
    "filter": "Filter operation latency at each captured dataset size.",
    "resize": "Column-resize responsiveness: frame pacing while a column divider is dragged back and forth.",
    "select": "Selection latency: select-all across the dataset, timed to the fully rendered UI state.",
    "edit": "Editing interaction cost and process-resource behavior.",
    # Hierarchy benches. "-x" = every row expanded (ExpandAllRows before first render); otherwise collapsed.
    "load-x": "Hierarchy: initial load with every row expanded (ExpandAllRows in Loaded, part of first render).",
    "scroll-x": "Hierarchy: scrolling the fully expanded tree; frame pacing and virtualization.",
    "resize-x": "Hierarchy: column-resize frame pacing on the fully expanded tree.",
    "select-x": "Hierarchy: Select(10) on the fully expanded tree, timed to rendered.",
    "declare": "Hierarchy: declaring the tree on a loaded flat source (KeyBy+ParentBy / ParentBy(key,parent) / WithChildren), timed to rendered.",
    "expandall": "Hierarchy: ExpandAllRows from collapsed, timed to rendered.",
    "collapseall": "Hierarchy: CollapseAllRows from fully expanded, timed to rendered.",
    "expand1": "Hierarchy: expanding the first root (automation ExpandCollapse pattern), timed to rendered.",
    "sort": "Sort operation latency at each captured dataset size (hierarchy benches: Sort(Name) on the collapsed tree).",
    "filter": "Filter operation latency at each captured dataset size (hierarchy benches: Filter(Value<10) on the collapsed tree).",
    "group": "Hierarchy: GroupBy(8 groups) on the collapsed tree, timed to rendered.",
    "sort-x": "Hierarchy: Sort(Name) on the fully expanded tree, timed to rendered.",
    "filter-x": "Hierarchy: Filter(Value<10) on the fully expanded tree, timed to rendered.",
    "group-x": "Hierarchy: GroupBy(8 groups) on the fully expanded tree, timed to rendered.",
    "edit-x": "Hierarchy: inserting one leaf under a middle row of the expanded tree, timed to rendered.",
}

# Each scenario shows only the metrics relevant to it (the operation's own measurement up top,
# plus a little process-resource context) — not the full generic dump.
_CONTEXT_METRICS = [
    ("Avg CPU %", "cpu_avg", 1),
    ("Avg working set MB", "ws_mb_avg", 1),
    ("Managed heap MB", "managed_heap_mb", 1),
]
SCENARIO_METRIC_SETS = {
    "load": [
        ("Startup ms", "startup_ms", 1),
        ("Creation ms", "creation_ms", 1),
        ("First render ms", "first_render_ms", 1),
        ("Time to interactive ms", "time_to_interactive_ms", 1),
        ("Avg working set MB", "ws_mb_avg", 1),
        ("Managed heap MB", "managed_heap_mb", 1),
        ("Bytes per row", "bytes_per_row", 1),
    ],
    "scroll": [
        ("FPS avg", "fps_avg", 1),
        ("P95 frame ms", "frame_p95_ms", 1),
        ("P99 frame ms", "frame_p99_ms", 1),
        ("Max frame ms", "frame_max_ms", 1),
        ("Jank %", "jank_pct", 1),
        ("Realized / total rows", "realized_total", 0),
        ("Realized cols", "realized_cols", 0),
        ("Realized cells", "realized_cells", 0),
        ("Avg working set MB", "ws_mb_avg", 1),
    ],
    "sort": [("Sort latency ms", "sort_ms", 1)] + _CONTEXT_METRICS,
    "filter": [("Filter latency ms", "filter_ms", 1)] + _CONTEXT_METRICS,
    "select": [("Selection latency ms", "select_ms", 1)] + _CONTEXT_METRICS,
    "resize": [
        ("FPS avg", "fps_avg", 1),
        ("P95 frame ms", "frame_p95_ms", 1),
        ("Max frame ms", "frame_max_ms", 1),
        ("Jank %", "jank_pct", 1),
    ] + _CONTEXT_METRICS,
}

# Hierarchy benches: the timed operation (op_ms) plus the visible-row change it produced.
_HIER_OP_METRICS = [
    ("Operation latency ms", "op_ms", 1),
    ("Visible rows before", "visible_rows_before", 0),
    ("Visible rows after", "visible_rows_after", 0),
] + _CONTEXT_METRICS
SCENARIO_METRIC_SETS["load-x"] = SCENARIO_METRIC_SETS["load"] + [("ExpandAllRows in Loaded ms", "setup_expand_ms", 1)]
SCENARIO_METRIC_SETS["scroll-x"] = SCENARIO_METRIC_SETS["scroll"]
SCENARIO_METRIC_SETS["resize-x"] = SCENARIO_METRIC_SETS["resize"]
for _s in ("select-x", "declare", "expandall", "collapseall", "expand1", "group", "sort-x", "filter-x", "group-x", "edit-x"):
    SCENARIO_METRIC_SETS[_s] = _HIER_OP_METRICS

GLOSSARY_ROWS = [
    ("CPU%", "psutil cpu_percent", "Per-process, all cores", "Process CPU summed across logical cores — can exceed 100% (e.g. 200% = 2 cores busy). Mean and peak over the run."),
    ("Private Bytes", "psutil", "Process", "Committed private memory that cannot be shared with other processes."),
    ("Working Set", "psutil", "Process", "Resident memory currently in physical RAM (avg and peak sampled during the run)."),
    ("Handles", "psutil num_handles", "Process", "Open kernel handles owned by the process."),
    ("Threads", "psutil num_threads", "Process", "Thread count owned by the process."),
    ("GDI/USER objects", "Win32 GetGuiResources", "Process", "GUI resource object counts for leak and pressure tracking."),
    ("Context switches", "psutil num_ctx_switches", "Process", "Voluntary and involuntary context switches observed during sampling."),
    ("GPU Utilization", "PDH \\GPU Engine", "Process-adjacent", "Average and peak GPU engine utilization where counters are available."),
    ("Startup", "Ready-file timing", "End-to-end", "Process-create to the first composition frame (ready-file written on first CompositionTarget.Rendering). NOT time-to-interactive — see that metric separately."),
    ("Binary footprint", "Filesystem scan", "App directory", "Sum of exe and dll sizes in the app directory (app-level; for the control-specific cost see the Binary footprint section)."),
    ("Creation time", "In-app Stopwatch", "Scenario", "Build data model + construct control tree (split into data-gen and control-creation)."),
    ("First render", "CompositionTarget.Rendering", "Scenario", "Process-start to the first composition frame after the control is loaded."),
    ("FPS/P95/P99/Jank", "CompositionTarget.Rendering", "UI-thread cadence", "UI-thread frame cadence (a PROXY for present rate, not GPU vsync/DWM present). Jank = frames over 16.7 ms; FPS = 1000/mean(interval)."),
    ("Realized rows/cols/cells", "Visual-tree count", "Scenario", "Realized containers / cell-content elements during scroll; the virtualization-effectiveness signal."),
    ("Managed heap", "GC.GetTotalMemory(true)", "Process", "Steady-state LIVE managed heap after a forced GC (collected before measuring, so it excludes uncollected garbage)."),
    ("Sort/Filter/Selection latency", "In-app Stopwatch", "Action → rendered", "Timed from the user action to the FULLY RENDERED UI: data op + measure/arrange (UpdateLayout) + 2 composition frames. NOT just the data-operation time (which would under-report perceived latency)."),
    ("Time to interactive", "Dispatcher Low-priority callback", "End-to-end", "Process-start until the UI thread drains to Low dispatcher priority after first render — i.e. input-ready."),
    ("Resize frame pacing", "CompositionTarget.Rendering", "UI-thread cadence", "Frame intervals while a column divider is oscillated (resize scenario)."),
    ("Bytes per row", "Working set ÷ rows", "Derived", "Average working set divided by total rows — a rough per-row memory cost (includes fixed overhead, so it inflates at small row counts)."),
]

# Control → (framework, language, role). Drives the "Controls under test" legend so the
# WinUI-native vs OSS vs WPF distinction is explicit and the comparison is apples-to-apples.
CONTROL_META = {
    "TableView":      ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "Subject under study (PR #15522127)"),
    "TableViewV1":    ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native, model-first v1", "Subject under study — model-first TableView v1 (PR #15522127)"),
    "ListView":       ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "WinUI-native baseline (list)"),
    "ItemsView":      ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "WinUI-native baseline (modern list)"),
    "WinUITableView": ("WinUI 3 — community / OSS", "C# — derives from ListView", "OSS comparator (w-ahmad/WinUI.TableView)"),
    "DataGrid":       ("WPF (.NET)", "C# — managed", "Cross-framework reference only"),
    "ListBox":        ("WPF (.NET)", "C# — managed", "Cross-framework reference only"),
    "TableViewKeyBy":     ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "Hierarchy variant: KeyBy(k).ParentBy(p) (pkg 3.0.0-perf-keyby2)"),
    "TableViewCombined":  ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "Hierarchy variant: ParentBy(key, parent) (pkg 3.0.0-perf-combined1)"),
    "TableViewChildren3": ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "Hierarchy variant: WithChildren, per-level sort (pkg 3.0.0-perf-children3)"),
    "TableViewChildren4": ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "Hierarchy variant: WithChildren, global sort rank (pkg 3.0.0-perf-children4)"),
    "TableViewFlat":      ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "Flat baseline: same TableView build (keyby2), no hierarchy declared"),
    "ListViewFlat":       ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "Flat baseline: ListView, harness row template, sort/filter = repopulate"),
    "ItemsViewFlat":      ("WinUI 3 (Microsoft.UI.Xaml)", "C++/WinRT — native", "Flat baseline: ItemsView, harness row template, sort/filter = repopulate"),
    "WinUITableViewFlat": ("WinUI 3 — community / OSS", "C# — derives from ListView", "Flat baseline: w-ahmad/WinUI.TableView 1.4.1 on WinAppSDK 1.8"),
}

# Reference docs / source for each control (linked from the "Controls under test" legend).
CONTROL_LINKS = {
    "TableView":      "https://microsoft.visualstudio.com/WinUI/_git/microsoft-ui-xaml-lift/pullrequest/15522127",
    "TableViewV1":    "https://microsoft.visualstudio.com/WinUI/_git/microsoft-ui-xaml-lift/pullrequest/15522127",
    "ListView":       "https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.listview",
    "ItemsView":      "https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.itemsview",
    "WinUITableView": "https://github.com/w-ahmad/WinUI.TableView",
    "DataGrid":       "https://learn.microsoft.com/dotnet/api/system.windows.controls.datagrid",
    "ListBox":        "https://learn.microsoft.com/dotnet/api/system.windows.controls.listbox",
}

# Framework-qualified display names (raw control names are kept for data lookups).
CONTROL_DISPLAY = {
    "TableView": "WinUI TableView",
    "TableView (ColVirt OFF)": "WinUI TableView (ColVirt OFF)",
    "TableView (ColVirt ON)": "WinUI TableView (ColVirt ON)",
    "TableViewV1": "WinUI TableView v1",
    "TableViewV1 (ColVirt OFF)": "WinUI TableView v1 (ColVirt OFF)",
    "TableViewV1 (ColVirt ON)": "WinUI TableView v1 (ColVirt ON)",
    "ListView": "WinUI ListView",
    "ItemsView": "WinUI ItemsView",
    "WinUITableView": "OSS TableView",
    "DataGrid": "WPF DataGrid",
    "ListBox": "WPF ListBox",
    "TableViewKeyBy": "TableView hierarchy — KeyBy",
    "TableViewCombined": "TableView hierarchy — Combined",
    "TableViewChildren3": "TableView hierarchy — Children (per-level sort)",
    "TableViewChildren4": "TableView hierarchy — Children (global sort)",
    "TableViewFlat": "TableView flat (no hierarchy)",
    "ListViewFlat": "WinUI ListView (flat)",
    "ItemsViewFlat": "WinUI ItemsView (flat)",
    "WinUITableViewFlat": "OSS TableView (flat)",
}

HIERARCHY_CONTROLS = ("TableViewKeyBy", "TableViewCombined", "TableViewChildren3", "TableViewChildren4")
FLAT_BASELINE_GRIDS = ("TableViewFlat", "WinUITableViewFlat")

GRID_CONTROLS = {"TableView", "TableView (ColVirt OFF)", "TableView (ColVirt ON)",
                 "TableViewV1", "TableViewV1 (ColVirt OFF)", "TableViewV1 (ColVirt ON)",
                 "WinUITableView", "DataGrid", *HIERARCHY_CONTROLS, *FLAT_BASELINE_GRIDS}   # multi-column data grids — the fair head-to-head
# (ListView, ItemsView, ListBox are single-column list baselines)
# TableView variants: "TableView" (no ColVirt run), "TableView (ColVirt OFF)" / "TableView (ColVirt ON)"
# (the ColVirt A/B from -ColVirt both in run-all.ps1). All count as the WinUI TableView control for verdict purposes.
# "TableViewV1" is the model-first TableView v1 (its own split-binary bench); it is a WinUI TableView subject too.
TV_BASE = "TableView"
TV_VARIANT_OFF = "TableView (ColVirt OFF)"
TV_VARIANT_ON = "TableView (ColVirt ON)"
TV_V1_BASE = "TableViewV1"
TV_V1_VARIANT_OFF = "TableViewV1 (ColVirt OFF)"
TV_V1_VARIANT_ON = "TableViewV1 (ColVirt ON)"
TV_ALL_NAMES = (TV_BASE, TV_VARIANT_OFF, TV_VARIANT_ON, TV_V1_BASE, TV_V1_VARIANT_OFF, TV_V1_VARIANT_ON, *HIERARCHY_CONTROLS)


def tv_variants_in(controls):
    """Return ordered list of TableView variants present in ``controls``."""
    return [c for c in TV_ALL_NAMES if c in controls]


def tv_short_label(name):
    if name in (TV_VARIANT_OFF, TV_V1_VARIANT_OFF):
        return "ColVirt OFF"
    if name in (TV_VARIANT_ON, TV_V1_VARIANT_ON):
        return "ColVirt ON"
    return ""


def display_name(control):
    return CONTROL_DISPLAY.get(control, control)

# Executive-summary ranking plans: (label, field, dataset-key, scenarios, higher_is_better, digits).
# dataset-key "scale" = heaviest by rows, "width" = heaviest by columns (the column-virtualization wall).
VERDICT_PLANS = [
    ("Startup", "startup_ms", "scale", {"load"}, False, 1, "ms"),
    ("Time to interactive", "time_to_interactive_ms", "scale", {"load"}, False, 1, "ms"),
    ("First render", "first_render_ms", "scale", {"load"}, False, 1, "ms"),
    ("Scroll FPS", "fps_avg", "scale", {"scroll"}, True, 1, "FPS"),
    ("Scroll FPS (column wall)", "fps_avg", "width", {"scroll"}, True, 1, "FPS"),
    ("P99 frame (column wall)", "frame_p99_ms", "width", {"scroll"}, False, 1, "ms"),
    ("Working set", "ws_mb_avg", "scale", None, False, 1, "MB"),
    ("Sort latency", "sort_ms", "scale", {"sort"}, False, 1, "ms"),
    ("Filter latency", "filter_ms", "scale", {"filter"}, False, 1, "ms"),
    ("Selection latency", "select_ms", "scale", {"select"}, False, 1, "ms"),
    ("Bytes / row", "bytes_per_row", "scale", None, False, 0, "B"),
    ("Avg CPU", "cpu_avg", "scale", {"scroll"}, False, 1, "%"),
    # Hierarchy benches (op_ms = op -> fully rendered).
    ("Expanded scroll FPS", "fps_avg", "scale", {"scroll-x"}, True, 1, "FPS"),
    ("Expanded scroll P99 frame", "frame_p99_ms", "scale", {"scroll-x"}, False, 1, "ms"),
    ("First render (expanded)", "first_render_ms", "scale", {"load-x"}, False, 1, "ms"),
    ("Declare hierarchy", "op_ms", "scale", {"declare"}, False, 1, "ms"),
    ("ExpandAllRows", "op_ms", "scale", {"expandall"}, False, 1, "ms"),
    ("CollapseAllRows", "op_ms", "scale", {"collapseall"}, False, 1, "ms"),
    ("Expand one root", "op_ms", "scale", {"expand1"}, False, 1, "ms"),
    ("Sort (collapsed)", "op_ms", "scale", {"sort"}, False, 1, "ms"),
    ("Sort (expanded)", "op_ms", "scale", {"sort-x"}, False, 1, "ms"),
    ("Filter (collapsed)", "op_ms", "scale", {"filter"}, False, 1, "ms"),
    ("Filter (expanded)", "op_ms", "scale", {"filter-x"}, False, 1, "ms"),
    ("GroupBy (collapsed)", "op_ms", "scale", {"group"}, False, 1, "ms"),
    ("GroupBy (expanded)", "op_ms", "scale", {"group-x"}, False, 1, "ms"),
    ("Insert leaf (expanded)", "op_ms", "scale", {"edit-x"}, False, 1, "ms"),
    ("Select (expanded)", "op_ms", "scale", {"select-x"}, False, 1, "ms"),
]


class Raw(str):
    """Pre-rendered HTML that esc() emits verbatim (e.g. links inside table cells)."""


def esc(value):
    if isinstance(value, Raw):
        return str(value)
    return html.escape(str(value), quote=True)


def parse_number(value):
    if value is None:
        return None
    text = str(value).strip()
    if text == "":
        return None
    try:
        return float(text)
    except ValueError:
        return None


def number_to_int(value):
    if value is None:
        return None
    return int(value)


def read_results(path):
    try:
        with open(path, newline="", encoding="utf-8-sig") as handle:
            reader = csv.DictReader(handle)
            if not reader.fieldnames:
                return []
            rows = []
            for raw in reader:
                numbers = {name: parse_number(raw.get(name)) for name in NUMERIC_COLUMNS}
                rows.append(
                    {
                        "control": (raw.get("control") or "Unknown").strip() or "Unknown",
                        "scenario": (raw.get("scenario") or "unknown").strip() or "unknown",
                        "rows": number_to_int(numbers.get("rows")),
                        "cols": number_to_int(numbers.get("cols")),
                        "run": number_to_int(numbers.get("run")),
                        "numbers": numbers,
                    }
                )
            return rows
    except (FileNotFoundError, OSError):
        return []


_FRAMEWORK_RANK = {
    "TableView": (0, 0), "ListView": (0, 1), "ItemsView": (0, 2),   # WinUI 3 native (TableView lead)
    "WinUITableView": (1, 0),                                        # WinUI 3 OSS
    "DataGrid": (2, 0), "ListBox": (2, 1),                           # WPF
    "TableViewKeyBy": (0, 10), "TableViewCombined": (0, 11),         # hierarchy variants
    "TableViewChildren3": (0, 12), "TableViewChildren4": (0, 13),
    "TableViewFlat": (0, 9), "ListViewFlat": (0, 14), "ItemsViewFlat": (0, 15), "WinUITableViewFlat": (1, 1),
}


def sort_controls(controls):
    # Group same-framework controls together: WinUI-native, then OSS, then WPF — TableView first.
    def key(name):
        rank = _FRAMEWORK_RANK.get(name)
        return (rank[0], rank[1], name.lower()) if rank is not None else (9, 9, name.lower())
    return sorted(controls, key=key)


def sort_sizes(sizes):
    return sorted(sizes, key=lambda item: ((-1 if item[0] is None else item[0]), (-1 if item[1] is None else item[1])))


def values(rows, field, predicate=None):
    items = []
    for row in rows:
        if predicate is not None and not predicate(row):
            continue
        value = row["numbers"].get(field)
        if value is not None:
            items.append(value)
    return items


def mean_value(rows, field, predicate=None):
    items = values(rows, field, predicate)
    if not items:
        return None
    # Median across repeats (warmup run already dropped) is more robust to outliers/contention than mean.
    return statistics.median(items)


def iqr_value(rows, field, predicate=None):
    items = values(rows, field, predicate)
    if len(items) < 2:
        return None
    ordered = sorted(items)
    try:
        q1, q3 = statistics.quantiles(ordered, n=4)[0], statistics.quantiles(ordered, n=4)[2]
        return q3 - q1
    except statistics.StatisticsError:
        return None


def drop_warmup(rows):
    # When a cell was measured multiple times, discard run 1 (cold/JIT/disk warmup) so aggregates
    # reflect steady state. Cells measured once are kept unchanged.
    groups = {}
    for row in rows:
        key = (row["control"], row["scenario"], row["rows"], row["cols"])
        groups.setdefault(key, []).append(row)
    kept = []
    for group in groups.values():
        runs = {row.get("run") for row in group}
        if len(runs) > 1:
            kept.extend(row for row in group if row.get("run") != 1)
        else:
            kept.extend(group)
    return kept


def format_number(value, digits=1):
    if value is None:
        return "—"
    if digits == 0:
        return str(int(round(value)))
    return f"{value:.{digits}f}"


def mean_text(rows, field, predicate=None, digits=1):
    return format_number(mean_value(rows, field, predicate), digits)


def realized_total_text(rows, predicate=None):
    realized = mean_value(rows, "realized_rows", predicate)
    total = mean_value(rows, "total_rows", predicate)
    if realized is None or total is None:
        return "—"
    return f"{int(round(realized))} / {int(round(total))}"


def row_count_label(value):
    if value is None:
        return "unknown rows"
    return f"{value} rows"


def size_label(size):
    rows, cols = size
    row_text = "?" if rows is None else str(rows)
    col_text = "?" if cols is None else str(cols)
    return f"{row_text}×{col_text}"


def largest_row_for(rows, scenario_names=None):
    found = []
    lowered = None
    if scenario_names is not None:
        lowered = {name.lower() for name in scenario_names}
    for row in rows:
        if row["rows"] is None:
            continue
        if lowered is not None and row["scenario"].lower() not in lowered:
            continue
        found.append(row["rows"])
    if not found:
        return None
    return max(found)


def second_largest_row(rows):
    counts = sorted({row["rows"] for row in rows if row["rows"] is not None})
    if len(counts) < 2:
        return None
    return counts[-2]


def heaviest_size(rows, key):
    sizes = {(row["rows"], row["cols"]) for row in rows if row["rows"] is not None and row["cols"] is not None}
    if not sizes:
        return None
    if key == "width":
        return max(sizes, key=lambda s: (s[1], s[0]))
    return max(sizes, key=lambda s: (s[0], s[1]))


def rank_controls(rows, controls, field, size, scenarios, higher_better):
    lowered = {name.lower() for name in scenarios} if scenarios is not None else None
    scored = []
    for control in controls:
        def predicate(row, control=control, size=size, lowered=lowered):
            if row["control"] != control:
                return False
            if size is not None and (row["rows"], row["cols"]) != size:
                return False
            if lowered is not None and row["scenario"].lower() not in lowered:
                return False
            return True
        value = mean_value(rows, field, predicate)
        if value is not None:
            scored.append((control, value))
    if not scored:
        return None
    scored.sort(key=lambda item: item[1], reverse=higher_better)
    return scored


def render_table(headers, body_rows, numeric_from=1):
    out = ["<table>", "<thead><tr>"]
    for index, header in enumerate(headers):
        cls = " class=\"num\"" if index >= numeric_from else ""
        out.append(f"<th{cls}>{esc(header)}</th>")
    out.append("</tr></thead><tbody>")
    for body_row in body_rows:
        out.append("<tr>")
        for index, cell in enumerate(body_row):
            cls = " class=\"num\"" if index >= numeric_from else ""
            out.append(f"<td{cls}>{esc(cell)}</td>")
        out.append("</tr>")
    out.append("</tbody></table>")
    return "\n".join(out)


def render_scorecard(primary_display, cards):
    out = [
        "<section>",
        "<h2>Headline scorecard</h2>",
        f"<p class=\"desc\">Key numbers for <strong>{esc(primary_display)}</strong> — the control under study. These are {esc(primary_display)} alone (<em>not</em> an average across controls); size-dependent cards note their dataset. For the cross-control comparison see the Executive summary above.</p>",
        "<div class=\"scorecard\">",
    ]
    for label, value in cards:
        out.append("<div class=\"score\">")
        out.append(f"<div class=\"score-value\">{esc(value)}</div>")
        out.append(f"<div class=\"score-label\">{esc(label)}</div>")
        out.append("</div>")
    out.append("</div></section>")
    return "\n".join(out)


def scorecard(rows):
    controls = sort_controls({row["control"] for row in rows})
    primary = "TableView" if "TableView" in controls else (controls[0] if controls else "")
    largest = largest_row_for(rows)
    second = second_largest_row(rows)
    sort_largest = largest_row_for(rows, {"sort"})
    filter_largest = largest_row_for(rows, {"filter"})
    realized_largest = largest_row_for(rows, {"scroll", "load"})

    def primary_only(extra=None):
        if extra is None:
            return lambda row: row["control"] == primary
        return lambda row: row["control"] == primary and extra(row)

    cards = [
        (f"Time to interactive ms @ {row_count_label(largest)}", mean_text(rows, "time_to_interactive_ms", primary_only(lambda r, s=largest: r["scenario"].lower() == "load" and r["rows"] == s), 1) if largest is not None else "—"),
        (f"Creation time ms @ {row_count_label(largest)}", mean_text(rows, "creation_ms", primary_only(lambda r, s=largest: r["scenario"].lower() == "load" and r["rows"] == s), 1) if largest is not None else "—"),
        (f"First render ms @ {row_count_label(largest)}", mean_text(rows, "first_render_ms", primary_only(lambda r, s=largest: r["scenario"].lower() == "load" and r["rows"] == s), 1) if largest is not None else "—"),
        (
            f"Working set MB @ {row_count_label(largest)}",
            mean_text(rows, "ws_mb_avg", primary_only(lambda r, s=largest: r["rows"] == s), 1) if largest is not None else "—",
        ),
        (
            f"Working set MB @ {row_count_label(second)}",
            mean_text(rows, "ws_mb_avg", primary_only(lambda r, s=second: r["rows"] == s), 1) if second is not None else "—",
        ),
        ("Scroll FPS avg", mean_text(rows, "fps_avg", primary_only(lambda r: r["scenario"].lower() == "scroll"), 1)),
        ("P99 frame time ms", mean_text(rows, "frame_p99_ms", primary_only(lambda r: r["scenario"].lower() == "scroll"), 1)),
        (
            f"Sort latency ms @ {row_count_label(sort_largest)}",
            mean_text(rows, "sort_ms", primary_only(lambda r, s=sort_largest: r["scenario"].lower() == "sort" and r["rows"] == s), 1)
            if sort_largest is not None
            else "—",
        ),
        (
            f"Filter latency ms @ {row_count_label(filter_largest)}",
            mean_text(rows, "filter_ms", primary_only(lambda r, s=filter_largest: r["scenario"].lower() == "filter" and r["rows"] == s), 1)
            if filter_largest is not None
            else "—",
        ),
        (
            f"Realized rows / total @ {row_count_label(realized_largest)}",
            realized_total_text(
                rows,
                primary_only(lambda r, s=realized_largest: r["scenario"].lower() in {"scroll", "load"} and r["rows"] == s),
            )
            if realized_largest is not None
            else "—",
        ),
    ]
    return display_name(primary), cards


def render_global_summary(rows, controls):
    # Restrict to one dataset size so size-sensitive metrics aren't blended across 100->100000-row
    # tiers; average across scenarios at that size for a typical process-resource profile.
    size = heaviest_size(rows, "scale")
    scoped = [r for r in rows if (r["rows"], r["cols"]) == size] if size is not None else rows
    body = []
    for label, field, digits in GLOBAL_METRICS:
        cells = [mean_text(scoped, field, lambda row, control=control: row["control"] == control, digits) for control in controls]
        if all(c == "—" for c in cells):
            continue  # drop metrics with no data for any control (e.g. GPU when not sampled)
        body.append([label] + cells)
    where = f" at {esc(size_label(size))}" if size is not None else ""
    return "\n".join(
        [
            "<section>",
            "<h2>Resource profile</h2>",
            f"<p class=\"desc\">External-sampler process metrics, averaged across all scenarios{where} "
            "(a single dataset size, so values are not blended across sizes — see the per-scenario sections for per-size detail).</p>",
            render_table(["Metric"] + [display_name(c) for c in controls], body),
            "</section>",
        ]
    )


def scenario_metrics(rows, size, scenario):
    s = scenario.lower()
    metrics = list(SCENARIO_METRIC_SETS.get(s, BASE_SCENARIO_METRICS))
    size_rows = [row for row in rows if (row["rows"], row["cols"]) == size]

    def has_data(field):
        op_scenario = {"sort_ms": "sort", "filter_ms": "filter", "select_ms": "select"}.get(field, s)
        scoped = [row for row in size_rows if row["scenario"].lower() == op_scenario]
        return bool(values(scoped, field))

    # Drop rows where no control has a value for this dataset (avoids all-"—" noise).
    return [(label, field, digits) for (label, field, digits) in metrics if has_data(field)]


def render_scenarios(rows, controls):
    scenarios = sorted({row["scenario"] for row in rows}, key=lambda item: item.lower())
    out = ["<section>", "<h2>Per-scenario sections</h2>"]
    for scenario in scenarios:
        description = SCENARIO_DESCRIPTIONS.get(scenario.lower(), "Benchmark measurements captured for this scenario.")
        inner = [f"<p class=\"desc\">{esc(description)}</p>"]
        sizes = sort_sizes({(row["rows"], row["cols"]) for row in rows if row["scenario"] == scenario})
        for size in sizes:
            inner.append(f"<h4>Dataset: {esc(size_label(size))}</h4>")
            body = []
            for label, field, digits in scenario_metrics(rows, size, scenario):
                cells = [label]
                # Sort/Filter latency are always measured in their own scenario, so pull those rows
                # directly rather than the section's scenario — keeps every dataset table complete.
                field_scenario = {"sort_ms": "sort", "filter_ms": "filter", "select_ms": "select"}.get(field, scenario)
                for control in controls:
                    predicate = (
                        lambda row, control=control, field_scenario=field_scenario, size=size: row["control"] == control
                        and row["scenario"].lower() == field_scenario.lower()
                        and (row["rows"], row["cols"]) == size
                    )
                    if field == "realized_total":
                        cells.append(realized_total_text(rows, predicate))
                    else:
                        cells.append(mean_text(rows, field, predicate, digits))
                body.append(cells)
            inner.append(render_table(["Metric"] + [display_name(c) for c in controls], body))
        out.append(f"<details class=\"sec\"><summary class=\"h3\">{esc(scenario)}</summary>" + "".join(inner) + "</details>")
    out.append("</section>")
    return "\n".join(out)


def render_methodology():
    preamble = (
        "Measurements come from two layers: an <strong>external sampler</strong> (Python · psutil + Win32 GetGuiResources + PDH) "
        "for things the control can't measure about itself (startup, CPU, working set, handles, GPU), and "
        "<strong>in-app instrumentation</strong> (Stopwatch · CompositionTarget.Rendering · VisualTreeHelper) for control-internal metrics. "
        "Conventions: latency = <strong>user action → fully rendered</strong> (data op + UpdateLayout + 2 composition frames), not just the data op; "
        "FPS is UI-thread frame cadence (a proxy, not GPU/DWM present rate); managed heap is the steady-state live set after a forced GC; "
        "multi-run cells report the median of runs 2..N (run 1 discarded as warmup)."
    )
    source = (
        f"<p class=\"desc\">📦 <strong>Source:</strong> all capture logic lives in {esc(SOURCE_NOTE)} — "
        f"<a class=\"srclink\" href=\"{esc(SOURCE_URL)}\" target=\"_blank\">view the code ↗</a> "
        f"<span class=\"muted\">(placeholder link — it will point to the kit's repo path once the code is committed).</span></p>"
    )
    return "\n".join(
        [
            "<section>",
            "<h2>Methodology — how each metric was captured</h2>",
            f"<p class=\"desc\">{preamble}</p>",
            render_table(["Metric", "Source", "Scope", "How captured"], GLOSSARY_ROWS, numeric_from=99),
            source,
            "</section>",
        ]
    )


def render_controls_legend(controls):
    def name_cell(control):
        name = display_name(control)
        url = CONTROL_LINKS.get(control)
        if url:
            return Raw(f"<a class=\"srclink\" href=\"{esc(url)}\" target=\"_blank\">{esc(name)}</a>")
        return name
    body = [[name_cell(control), *CONTROL_META.get(control, ("—", "—", "—"))] for control in controls]
    note = (
        "Apples-to-apples within WinUI: <strong>WinUI TableView</strong> (C++/WinRT native) vs the "
        "<strong>OSS TableView</strong> (w-ahmad WinUI.TableView, C#, derives from ListView); <strong>WinUI ListView</strong> and "
        "<strong>WinUI ItemsView</strong> are WinUI-native baselines. <strong>WPF DataGrid</strong> / "
        "<strong>WPF ListBox</strong> are a different framework (.NET WPF) shown for cross-framework reference only. "
        "<br><strong>Note on samples:</strong> every benchmark host app is <strong>C#</strong> — each control is driven identically from a C# harness. "
        "The \u201cControl implementation\u201d column below is the language of the <em>control library itself</em>: the WinUI framework controls are native C++/WinRT (accessed from C# via the WinRT projection), the OSS control and WPF controls are C#."
    )
    return "\n".join(
        [
            "<section>",
            "<h2>Controls under test</h2>",
            f"<p class=\"desc\">{note}</p>",
            render_table(["Control", "Framework", "Control implementation", "Role"], body, numeric_from=99),
            "</section>",
        ]
    )


def render_verdict(rows, controls):
    sizes = {"scale": heaviest_size(rows, "scale"), "width": heaviest_size(rows, "width")}
    if sizes["scale"] is None:
        return ""
    grid_controls = [control for control in controls if control in GRID_CONTROLS]
    verdict_controls = grid_controls if len(grid_controls) >= 2 else controls

    # Which TableView variants are in scope for the rightmost column(s)?
    # If multiple variants are present they each get their own column.
    tv_names = tv_variants_in(verdict_controls)
    if not tv_names:
        # Nothing TableView-shaped to show; keep a single placeholder column for layout stability.
        tv_keys, tv_headers = [TV_BASE], ["WinUI TableView"]
    elif len(tv_names) == 1:
        tv_keys, tv_headers = tv_names, [display_name(tv_names[0])]
    else:
        tv_keys = tv_names
        tv_headers = [display_name(n) for n in tv_names]

    computed = []
    win_counts = {control: 0 for control in verdict_controls}
    for label, field, size_key, scenarios, higher, digits, unit in VERDICT_PLANS:
        size = sizes.get(size_key)
        if size is None:
            continue
        ranked = rank_controls(rows, verdict_controls, field, size, scenarios, higher)
        if not ranked:
            continue
        if len(ranked) >= 2:
            win_counts[ranked[0][0]] += 1
        computed.append((label, size, higher, digits, ranked, unit))
    if not computed:
        return ""

    body = []
    tv_wins = {k: [] for k in tv_keys}
    tv_losses = {k: [] for k in tv_keys}
    for label, size, higher, digits, ranked, unit in computed:
        names = [control for control, _ in ranked]
        lookup = dict(ranked)
        best_c, best_v = ranked[0]
        worst_c, worst_v = ranked[-1]
        multi = len(ranked) >= 2
        tv_cells = []
        for tv_key in tv_keys:
            if tv_key in lookup:
                pos = names.index(tv_key) + 1
                rank_suffix = f" (#{pos}/{len(names)})" if multi else " (only grid measured)"
                tv_cells.append(f"{format_number(lookup[tv_key], digits)} {unit}{rank_suffix}")
                if multi and best_c == tv_key:
                    tv_wins[tv_key].append(label)
                elif multi and worst_c == tv_key:
                    tv_losses[tv_key].append(label)
            else:
                tv_cells.append("—")
        best_cell = f"{display_name(best_c)} — {format_number(best_v, digits)} {unit}"
        worst_cell = (
            f"{display_name(worst_c)} — {format_number(worst_v, digits)} {unit}"
            if multi
            else "— (only 1 grid measured)"
        )
        body.append(
            [
                f"{label} @ {size_label(size)}",
                best_cell,
                worst_cell,
                *tv_cells,
            ]
        )

    # WinUI apples-to-apples head-to-head: each TableView variant vs the OSS WinUITableView.
    h2h = {k: {"win": 0, "total": 0} for k in tv_keys}
    for _, _, higher, _, ranked, _ in computed:
        lookup = dict(ranked)
        if "WinUITableView" not in lookup:
            continue
        oss = lookup["WinUITableView"]
        for tv_key in tv_keys:
            if tv_key not in lookup:
                continue
            h2h[tv_key]["total"] += 1
            tv = lookup[tv_key]
            if (tv > oss) if higher else (tv < oss):
                h2h[tv_key]["win"] += 1

    leader = max(win_counts, key=lambda control: win_counts[control]) if win_counts else None
    bits = []
    if leader is not None:
        bits.append(f"<strong>Most category wins:</strong> {esc(display_name(leader))} ({win_counts[leader]} of {len(computed)} tracked metrics).")
    for tv_key in tv_keys:
        label_prefix = display_name(tv_key)
        if tv_wins.get(tv_key):
            bits.append(f"<strong>{esc(label_prefix)} leads</strong> on {esc(', '.join(tv_wins[tv_key]))}.")
        if tv_losses.get(tv_key):
            bits.append(f"<strong>{esc(label_prefix)} is the field laggard</strong> on {esc(', '.join(tv_losses[tv_key]))}.")
        h = h2h.get(tv_key, {"win": 0, "total": 0})
        if h["total"]:
            bits.append(
                f"<strong>WinUI apples-to-apples</strong> ({esc(label_prefix)} vs OSS TableView): "
                f"{esc(label_prefix)} wins {h['win']} of {h['total']} tracked metrics."
            )

    # Description text: dynamic when ColVirt variants are present.
    subject_label = " / ".join(tv_headers) if tv_headers else "WinUI TableView"
    if len(tv_keys) > 1 and all(tv_short_label(k) for k in tv_keys):
        desc_extra = (
            " <strong>WinUI TableView is shown as two variants</strong> — ColVirt OFF (today's behavior) and "
            "ColVirt ON (column-virtualization enabled) — both pulled from the same bench, toggled at runtime via "
            "<code>TVPERF_COLVIRT</code>."
        )
    else:
        desc_extra = ""

    return "\n".join(
        [
            "<section>",
            "<h2>Executive summary — who wins at what</h2>",
            "<p class=\"desc\">Each row is ranked at the most stressful dataset where the metric is measured: "
            "🏆 best vs 🐌 worst among the comparable <strong>grid controls</strong> "
            "(the WinUI TableView subject, OSS TableView, WPF DataGrid) — an apples-to-apples head-to-head, "
            f"with <strong>{esc(subject_label)}</strong>'s value and rank. Lower is better except Scroll FPS. "
            "The single-column <strong>list baselines</strong> (WinUI ListView/ItemsView, WPF ListBox) "
            "are shown in the detailed sections below but excluded here because they don't provide grid functionality "
            "(columns/headers/selection), so they're not cost-comparable." + desc_extra + "</p>",
            f"<p>{' '.join(bits)}</p>",
            render_table(["Metric (dataset)", "🏆 Best", "🐌 Worst", *tv_headers], body, numeric_from=99),
            "</section>",
        ]
    )


def stylesheet():
    return """
<style>
:root {
  --accent:#0078d4;
  --bg:#1e1e1e;
  --fg:#d4d4d4;
  --card:#252526;
  --border:#3e3e42;
  --heading:#e8e8e8;
}
* { box-sizing: border-box; }
body {
  margin: 0 auto;
  max-width: 1200px;
  padding: 2rem;
  background: var(--bg);
  color: var(--fg);
  font-family: 'Segoe UI', system-ui, sans-serif;
  line-height: 1.5;
}
h1, h2, h3, h4 { color: var(--heading); margin-bottom: 0.5rem; }
h1 { font-size: 2rem; margin-top: 0; }
h2 { border-bottom: 1px solid var(--border); padding-bottom: 0.35rem; margin-top: 2rem; }
h2.divider { border-bottom: 1px dashed var(--border); color: #8b949e; font-size: 1.05rem; text-transform: uppercase; letter-spacing: 0.06em; margin-top: 2.6rem; padding-bottom: 0.4rem; }
h3 { margin-top: 1.5rem; }
h4 { margin-top: 1rem; color: var(--fg); }
.meta, .desc { color: #b8b8b8; }
.srclink { color: #58a6ff; text-decoration: none; }
.srclink:hover { text-decoration: underline; }
.muted { color: #6e7681; font-size: 0.85em; }
.callout { background: rgba(88,166,255,0.08); border-left: 3px solid #58a6ff; border-radius: 6px; padding: 0.6rem 0.9rem; margin: 0.7rem 0 1.1rem; color: var(--fg); line-height: 1.55; }
.callout strong { color: #fff; }
.scorecard {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(190px, 1fr));
  gap: 1rem;
  margin-top: 1rem;
}
.score {
  background: var(--card);
  border: 1px solid var(--border);
  border-radius: 10px;
  padding: 1rem;
  min-height: 110px;
}
.score-value {
  color: var(--accent);
  font-size: 2rem;
  font-weight: 700;
  font-variant-numeric: tabular-nums;
  line-height: 1.1;
  word-break: break-word;
}
.score-label { margin-top: 0.5rem; font-size: 0.9rem; color: #c8c8c8; }
table {
  width: 100%;
  border-collapse: collapse;
  margin: 0.75rem 0 1.25rem;
  background: var(--card);
  border: 1px solid var(--border);
}
th, td { padding: 0.55rem 0.65rem; border-bottom: 1px solid var(--border); vertical-align: top; }
th { text-align: left; color: var(--heading); border-bottom: 2px solid var(--accent); }
tbody tr:nth-child(even) { background: rgba(255,255,255,0.025); }
.num { text-align: right; font-family: Consolas, 'Cascadia Mono', monospace; font-variant-numeric: tabular-nums; }
section { margin-top: 1.5rem; }
.no-results {
  margin-top: 2rem;
  padding: 1rem;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: var(--card);
}
details.sec { border: 1px solid var(--border); border-radius: 10px; background: var(--card); margin-top: 1rem; padding: 0 1rem; }
details.sec > summary { cursor: pointer; list-style: none; padding: 0.7rem 0; font-weight: 600; color: var(--heading); user-select: none; }
details.sec > summary.h2 { font-size: 1.3rem; }
details.sec > summary.h3 { font-size: 1.05rem; color: #9ecbff; }
details.sec > summary::-webkit-details-marker { display: none; }
details.sec > summary::before { content: "▸ "; color: var(--accent); font-weight: 700; }
details.sec[open] > summary::before { content: "▾ "; }
details.sec[open] > summary { border-bottom: 1px solid var(--border); }
details.sec[open] { padding-bottom: 1rem; }
details.sec details.sec { background: rgba(255,255,255,0.02); }
</style>
""".strip()


def page(title, body):
    return "\n".join(
        [
            "<!doctype html>",
            "<html lang=\"en\">",
            "<head>",
            "<meta charset=\"utf-8\">",
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">",
            f"<title>{esc(title)}</title>",
            stylesheet(),
            "</head>",
            "<body>",
            body,
            "</body>",
            "</html>",
        ]
    )


def load_sidecar(csv_path, name):
    try:
        path = os.path.join(os.path.dirname(os.path.abspath(csv_path)), name)
        if os.path.exists(path):
            with open(path, encoding="utf-8-sig") as handle:
                return json.load(handle)
    except Exception:
        pass
    return None


def control_version_label(env):
    if not env:
        return UNSPECIFIED_CONTROL_VERSION
    return (
        env.get("control_version")
        or env.get("controlVersion")
        or env.get("control_label")
        or env.get("label")
        or UNSPECIFIED_CONTROL_VERSION
    )


def control_version_detail(env):
    if not env:
        return ""
    version = env.get("control_file_version")
    binary = env.get("control_binary")
    details = []
    if version:
        details.append(f"DLL FileVersion {version}")
    if binary:
        details.append(str(binary))
    return " · ".join(details)


ENV_FIELDS = [
    ("Measured control", "control_version"),
    ("Control DLL FileVersion", "control_file_version"),
    ("Control DLL", "control_binary"),
    ("CPU", "cpu"),
    ("Logical cores", "cores"),
    ("RAM", "ram"),
    ("GPU", "gpu"),
    ("OS", "os"),
    ("Build", "build"),
    (".NET", "dotnet"),
    ("Windows App SDK", "winappsdk"),
    ("Captured", "captured"),
]


def render_env(env):
    env = env or {}
    body = [["Measured control", control_version_label(env)]]
    body.extend([[label, str(env.get(key, "—"))] for label, key in ENV_FIELDS if key != "control_version" and env.get(key)])
    if not body:
        return ""
    return "\n".join(
        [
            "<section>",
            "<h2>Test environment</h2>",
            "<p class=\"desc\">All measurements were captured on this machine/configuration. Absolute numbers are only comparable within the same environment.</p>",
            render_table(["Property", "Value"], body, numeric_from=99),
            "</section>",
        ]
    )


def render_binary_impact(footprint, controls):
    if not footprint:
        return ""
    mux = footprint.get("_mux_tableview")
    if not mux:
        return ""
    fw_dll = footprint.get("TableView", {}).get("framework_dll_kb")
    oss = footprint.get("WinUITableView", {}).get("app_carried_kb")
    rows = [
        ["Consumer app delta (with vs without TableView)", "≈ 0 KB — WinUI TableView ships inside the Windows App SDK runtime, so referencing it adds no extra shipped bytes to a consumer app."],
        ["Control source size", f"{format_number(mux.get('source_kb'), 0)} KB · {mux.get('files', '—')} files · {format_number(mux.get('source_loc'), 0)} LOC"],
        ["Source breakdown", f"{mux.get('cpp', '—')} .cpp + {mux.get('h', '—')} .h (C++/WinRT impl) · {mux.get('idl', '—')} .idl (WinRT surface) · {mux.get('xaml', '—')} .xaml (themeresources/templates)"],
        ["Framework DLL it ships in", f"Microsoft.UI.Xaml.Controls.dll = {format_number(fw_dll, 0)} KB (bundles every WinUI control — TableView is one of many)"],
    ]
    if oss is not None:
        rows.append(["OSS alternative (for contrast only)", f"{format_number(oss, 0)} KB app-carried (WinUI.TableView.dll) — the OSS TableView ships in the app, so it is itself the with/without delta."])
    return "\n".join(
        [
            "<section>",
            "<h2>WinUI TableView — binary footprint</h2>",
            "<p class=\"desc\">Footprint is shown for the WinUI TableView control only (a cross-control app-size comparison isn't meaningful — WinUI/WPF controls are runtime-shipped, so the numbers are all 0). Because the framework <code>Microsoft.UI.Xaml.Controls.dll</code> bundles every control, referencing TableView adds <strong>~0 bytes</strong> to a consumer app. The control's own weight is its <strong>source size</strong>; the exact linked-DLL delta would need an A/B framework build (no symbol-size tool was available on this box).</p>",
            render_table(["Aspect", "Value"], rows, numeric_from=99),
            "</section>",
        ]
    )


def render_virtualization(rows, controls):
    targets = []
    seen = set()
    for name, size in (("Scale stress", heaviest_size(rows, "scale")), ("Column wall", heaviest_size(rows, "width"))):
        if size is not None and size not in seen:
            seen.add(size)
            targets.append((name, size))
    if not targets:
        return ""
    out = [
        "<section>",
        "<h2>Virtualization effectiveness</h2>",
        "<p class=\"desc\">Realized element counts during scroll. <strong>Realized cells</strong> = cell-content elements materialized. "
        "If realized cols ≈ total cols (and cells ≈ realized rows × total cols), columns are <strong>not</strong> being virtualized — "
        "every column of every visible row is built. Row virtualization keeps realized rows small regardless.</p>",
    ]
    for name, size in targets:
        total_cols = size[1] or 0
        per = {}
        for control in controls:
            def predicate(row, control=control, size=size):
                return row["control"] == control and row["scenario"].lower() == "scroll" and (row["rows"], row["cols"]) == size
            r_rows = mean_value(rows, "realized_rows", predicate)
            r_cols = mean_value(rows, "realized_cols", predicate)
            r_cells = mean_value(rows, "realized_cells", predicate)
            virt = "—"
            if r_cols is not None and total_cols:
                virt = "No (all cols)" if r_cols >= 0.9 * total_cols else f"Yes ({int(round(r_cols))}/{total_cols})"
            per[control] = (format_number(r_rows, 0), f"{format_number(r_cols, 0)} / {total_cols}", format_number(r_cells, 0), virt)
        metric_rows = [
            ["Realized rows"] + [per[c][0] for c in controls],
            ["Realized cols / total"] + [per[c][1] for c in controls],
            ["Realized cells"] + [per[c][2] for c in controls],
            ["Column virtualization"] + [per[c][3] for c in controls],
        ]
        out.append(f"<h4>{esc(name)} — {esc(size_label(size))}</h4>")
        out.append(render_table(["Metric"] + [display_name(c) for c in controls], metric_rows))
    out.append("</section>")
    return "\n".join(out)


def render_startup_breakdown(rows, controls):
    size = heaviest_size(rows, "scale")
    if size is None:
        return ""
    stages = [
        ("Data gen ms", "data_gen_ms"),
        ("Control creation ms", "control_creation_ms"),
        ("Creation ms (total)", "creation_ms"),
        ("First render ms", "first_render_ms"),
        ("Time to interactive ms", "time_to_interactive_ms"),
    ]
    body = []
    any_value = False
    for label, field in stages:
        cells = [label]
        for control in controls:
            def predicate(row, control=control, size=size):
                return row["control"] == control and row["scenario"].lower() == "load" and (row["rows"], row["cols"]) == size
            text = mean_text(rows, field, predicate, 1)
            if text != "—":
                any_value = True
            cells.append(text)
        body.append(cells)
    if not any_value:
        return ""
    return "\n".join(
        [
            "<section>",
            "<h2>Startup breakdown / time-to-interactive</h2>",
            f"<p class=\"desc\">Load scenario at {esc(size_label(size))}. Stages: data generation → control construction → first render → input-ready (time-to-interactive). Creation = data gen + control creation.</p>",
            render_table(["Stage"] + [display_name(c) for c in controls], body),
            "</section>",
        ]
    )


def render_memory_breakdown(rows, controls):
    size = heaviest_size(rows, "scale")
    if size is None:
        return ""
    metrics = [
        ("Working set MB (avg)", "ws_mb_avg", 1),
        ("Working set MB (max)", "ws_mb_max", 1),
        ("Private MB (avg)", "priv_mb_avg", 1),
        ("Managed heap MB", "managed_heap_mb", 1),
        ("Bytes per row", "bytes_per_row", 0),
        ("GC gen0", "gc_gen0", 0),
        ("GC gen1", "gc_gen1", 0),
        ("GC gen2", "gc_gen2", 0),
    ]
    body = []
    for label, field, digits in metrics:
        cells = [label]
        for control in controls:
            def predicate(row, control=control, size=size):
                return row["control"] == control and row["scenario"].lower() == "scroll" and (row["rows"], row["cols"]) == size
            cells.append(mean_text(rows, field, predicate, digits))
        if all(c == "—" for c in cells[1:]):
            continue
        body.append(cells)
    return "\n".join(
        [
            "<section>",
            "<h2>Memory breakdown</h2>",
            f"<p class=\"desc\">Scroll scenario at {esc(size_label(size))}. <strong>Working set</strong> is total resident memory — the real footprint to compare. "
            "<strong>Managed heap</strong> is only the .NET object graph: it is tiny for the C++/WinRT native controls "
            "(WinUI TableView/ListView/ItemsView), whose visual tree lives in native memory — so do not compare managed heap across "
            "frameworks, compare working set. GC gen counts show managed-collection pressure (low for the native controls).</p>",
            render_table(["Metric"] + [display_name(c) for c in controls], body),
            "</section>",
        ]
    )


def render_stability(rows, controls):
    size = heaviest_size(rows, "scale")
    if size is None:
        return ""
    metrics = [
        ("Scroll FPS", "fps_avg", {"scroll"}, 1),
        ("Startup ms", "startup_ms", {"load"}, 1),
        ("Working set MB", "ws_mb_avg", {"scroll"}, 1),
        ("Sort ms", "sort_ms", {"sort"}, 1),
    ]
    body = []
    has_spread = False
    for label, field, scenarios, digits in metrics:
        cells = [label]
        for control in controls:
            def predicate(row, control=control, scenarios=scenarios, size=size):
                return row["control"] == control and row["scenario"].lower() in scenarios and (row["rows"], row["cols"]) == size
            median = mean_value(rows, field, predicate)
            iqr = iqr_value(rows, field, predicate)
            if median is None:
                cells.append("—")
            elif iqr is not None:
                has_spread = True
                cells.append(f"{format_number(median, digits)} ±{format_number(iqr, digits)}")
            else:
                cells.append(format_number(median, digits))
        body.append(cells)
    if not has_spread:
        return ""
    return "\n".join(
        [
            "<section>",
            "<h2>Run stability (median ± IQR)</h2>",
            f"<p class=\"desc\">Spread across repeats at {esc(size_label(size))} (warmup run discarded). A small IQR means the measurement is stable; a large IQR (e.g., from box contention) means the absolute number should be treated with caution.</p>",
            render_table(["Metric"] + [display_name(c) for c in controls], body),
            "</section>",
        ]
    )


DETAIL_SECTION_TITLES = {
    "Virtualization effectiveness",
    "Startup breakdown / time-to-interactive",
    "Memory breakdown",
    "Resource profile",
    "Per-scenario sections",
    "Methodology — how each metric was captured",
}


def make_collapsible(body):
    # Wrap the detailed sections in <details> (collapsed by default); headline sections stay open.
    def repl(match):
        title = match.group(1).strip()
        if title in DETAIL_SECTION_TITLES:
            return f"<details class=\"sec\"><summary class=\"h2\">{title}</summary>{match.group(2)}</details>"
        return match.group(0)
    return re.sub(r"<section>\s*<h2>(.*?)</h2>(.*?)</section>", repl, body, flags=re.DOTALL)


def render_report(rows, title, env=None, footprint=None):
    generated = datetime.datetime.now(datetime.timezone.utc).replace(microsecond=0, tzinfo=None).isoformat() + "Z"
    control_version = control_version_label(env)
    control_detail = control_version_detail(env)
    control_callout = f"<strong>Measured control version:</strong> {esc(control_version)}"
    if control_detail:
        control_callout += f"<br><span class=\"muted\">{esc(control_detail)}</span>"
    if not rows:
        body = "\n".join(
            [
                f"<h1>{esc(title)}</h1>",
                f"<p class=\"meta\">Generated: {esc(generated)}</p>",
                f"<div class=\"callout\">{control_callout}</div>",
                "<div class=\"no-results\">No results</div>",
            ]
        )
        return page(title, body)

    controls = sort_controls({row["control"] for row in rows})
    run_values = {row.get("run") for row in rows if row.get("run")}
    runs_n = max(run_values) if run_values else 1
    rows = drop_warmup(rows)
    if runs_n > 1:
        runs_summary = f"{runs_n} runs/cell (median of 2–{runs_n}, run 1 discarded as warmup)"
        agg_note = (
            f"<strong>Iterations:</strong> each cell was measured <strong>{runs_n}×</strong>. "
            f"Run 1 (cold/JIT/disk warmup) is discarded; the reported value is the <strong>median of runs 2–{runs_n}</strong> "
            f"(per-metric spread is shown as ± IQR in the Run stability section)."
        )
    else:
        runs_summary = "1 run/cell (preliminary — no repeats)"
        agg_note = "<strong>Iterations:</strong> a <strong>single run per cell</strong> (preliminary — no repeats, so absolute numbers are indicative only)."
    scenarios = {row["scenario"] for row in rows}
    sizes = {(row["rows"], row["cols"]) for row in rows}
    summary = f"Measured control: {control_version}; Scenarios: {len(scenarios)} × {len(sizes)} dataset sizes × {runs_summary}; Controls: {', '.join(display_name(c) for c in controls)}"
    page_title = f"{title} — {runs_n} run{'s' if runs_n != 1 else ''}/cell"
    body = "\n".join(
        [
            f"<h1>{esc(title)}</h1>",
            f"<p class=\"meta\">Generated: {esc(generated)} · <strong>{esc(runs_summary)}</strong></p>",
            f"<p class=\"meta\">{esc(summary)}.</p>",
            f"<div class=\"callout\">{control_callout}</div>",
            f"<div class=\"callout\">{agg_note}</div>",
            f"<p class=\"meta\">📦 Source &amp; methodology: <a class=\"srclink\" href=\"{esc(SOURCE_URL)}\" target=\"_blank\">capture code ↗</a> · full detail in the Methodology section at the end. <span class=\"muted\">(link updates once the kit is committed)</span></p>",
            render_env(env),
            render_controls_legend(controls),
            render_verdict(rows, controls),
            render_scorecard(*scorecard(rows)),
            render_stability(rows, controls),
            render_binary_impact(footprint, controls),
            "<h2 class=\"divider\">Detailed sections — expand for the full data tables</h2>",
            render_virtualization(rows, controls),
            render_startup_breakdown(rows, controls),
            render_memory_breakdown(rows, controls),
            render_global_summary(rows, controls),
            render_scenarios(rows, controls),
            render_methodology(),
        ]
    )
    return page(page_title, make_collapsible(body))


def parse_args():
    parser = argparse.ArgumentParser(description="Render a self-contained dark-themed HTML performance report from results.csv.")
    parser.add_argument("--in", dest="in_path", required=True, help="Input long-form results CSV.")
    parser.add_argument("--out", dest="out_path", required=True, help="Output HTML report path.")
    parser.add_argument("--title", default="UI Study — Multi-Benchmark Report", help="Report title.")
    return parser.parse_args()


def main():
    args = parse_args()
    rows = read_results(args.in_path)
    env = load_sidecar(args.in_path, "env.json")
    footprint = load_sidecar(args.in_path, "footprint.json")
    output = render_report(rows, args.title, env, footprint)
    with open(args.out_path, "w", encoding="utf-8", newline="") as handle:
        handle.write(output)


if __name__ == "__main__":
    main()
