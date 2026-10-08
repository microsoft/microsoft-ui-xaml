# HierarchyBench

CLI benchmark host for the TableView hierarchy variants, driven by the external TableView perf
harness (originally `docs/design-notes/TableView/files/perf/` on
`user/hik/tableview/model-first-design` in microsoft-ui-xaml-lift). One process per matrix cell:
the harness launches the exe, samples CPU/memory/handles from outside, the app writes an L2
metrics CSV (`--emit`) and exits.

## Modes

The control name passed with `--control` selects the mode:

| Control | Mode | What runs |
|---|---|---|
| `TableViewKeyBy` / `TableViewCombined` / `TableViewChildren3` | tree | TableView with the hierarchy API compiled in (`HierarchyApi` = `KeyBy`, `ParentKey2`, `Children`) |
| `TableViewFlat` | flat | Same TableView build, same rows, no hierarchy declared |
| `ListViewFlat` / `ItemsViewFlat` | list | ListView / ItemsView with the harness row template; sort/filter = LINQ + repopulate |
| `WinUITableViewFlat` | OSS | `perf\OssBench` (w-ahmad/WinUI.TableView 1.4.1), same data and scenarios |

Scenarios: `load`, `load-x`, `scroll-x`, `resize-x`, `select-x`, `declare`, `expandall`,
`collapseall`, `expand1`, `sort[-x]`, `filter[-x]`, `group[-x]`, `edit-x` (insert one row),
`value-x` (change one property). Tree-only scenarios throw `NotSupported` in flat/list modes.
Timed ops also report `op_sync_ms` / `layout_ms` / `render_ms` (op call, `UpdateLayout`, two
composition frames).

## Running

1. Build each WinUI variant package (`3.0.0-perf-keyby2`, `-combined1`, `-children3`) into the
   repo PackageStore.
2. `perf\build-bench.ps1` builds one exe per variant into `C:\perf\hikbench\<Control>`.
3. OSS bench: copy `perf\OssBench` outside the repo (the repo's Directory.Build/Packages props
   break a standalone build), `dotnet build -c Release -o C:\perf\hikbench\WinUITableView`.
4. Run the matrix, for example:

   ```powershell
   perf\harness\run-all.ps1 -Controls TableViewFlat,ListViewFlat,ItemsViewFlat,WinUITableViewFlat,TableViewKeyBy,TableViewCombined,TableViewChildren3 `
     -Scenarios load-x,scroll-x,resize-x,select-x,sort-x,filter-x,edit-x,value-x -Tiers h50k -Runs 3 `
     -Out C:\perf\results\results.csv -ReportOut C:\perf\results\report.html
   ```

   Hierarchy tiers: `h10k`, `h50k` (rows) x 10 columns; `-Shape wide|deep|balanced|shallow`.

## Baseline (h50k, 10 cols, balanced, median of 3, ms unless noted)

| Metric | TV flat | ListView | ItemsView | OSS TV | KeyBy | Combined | Children |
|---|---|---|---|---|---|---|---|
| first render (tree fully expanded) | 390 | 263 | 314 | 1159 | 870 | 888 | 935 |
| private MB | 160 | 125 | 122 | 220 | 263 | 268 | 265 |
| scroll fps (VM-capped ~32) | 32 | 30 | 16 | 16 | 32 | 32 | 32 |
| sort-x | 1086 | 452 | 9378 | 555 | 350 | 1470 | 825 |
| filter-x | 179 | 80 | 966 | 1172 | 162 | 555 | 624 |
| edit-x (insert one row) | 42 | 59 | 29 | 91 | 575 | 622 | 637 |
| value-x (property change) | 48 | 45 | 40 | 83 | 40 | 54 | 49 |

Notes: list/OSS sort and filter rebuild the list; the tree filter keeps ancestors of matches
(8,746 rows vs 5,000 flat). Hierarchy `edit-x` is >500 ms synchronous because a single insert
rebuilds the parent-key index and the adapter's flattened rows.
