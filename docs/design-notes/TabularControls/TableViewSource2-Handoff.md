# TableViewSource2 redesign: session handoff

This note carries the working context of the TableViewSource2 ("v2") shaping redesign so that a new
session, on another machine or for another variation of the design, can continue without the
original conversation. The design itself lives in the documents listed below; this note covers
lineage, workflow, build and test recipes, hard-won lessons and open work.

## Reading order

1. `Shaping-Architecture-Redesign.md`: the frozen spec (reviewed headers and design). Do not edit.
2. `Shaping-Architecture-Redesign-2.md`: the working copy of the spec, kept in sync with what was
   actually built on `tvs-3-implementation-2`.
3. `TableViewSource2-Header-Changes-2.md`: the drift log. Every departure from the frozen spec or
   the reviewed headers, rows #1 to #138, each with the old wording, what was built, why, and how
   it compares with the first implementation. Read this before changing any component.
4. `TableViewSource2-Header-Changes.md`: the drift log of the first implementation (referenced as
   "old #N" in the `-2` log).
5. Supporting specs: `Shaping-Architecture-Layers.md`, `Hierarchical-Data-Rows-API-Spec.md`,
   `Hierarchical-Data-Rows-Parent-Key-Design.md`,
   `Hierarchical-Rows-and-Live-Shaping-Test-Plan.md`, `TableView-Data-Pipeline-Current-Structure.md`,
   `TableView-Row-Identity-Reuse-Spec.md`, `TableView-dev-spec.md`, `TableView-functional-spec.md`.
6. `Shaping-Architecture-Redesign-Original.md`: the pre-review spec, for history only.

## Lineage

| Branch | Role |
|---|---|
| `user/dipesh/tableview/tvs-3-implementation` | First implementation. Reference for comparison ("old" in the drift log). |
| `user/dipesh/tableview/tvs-3-implementation-2` | Second implementation: each component re-implemented fresh from the frozen spec and headers, compared against the first, with every departure logged. |

Key commits on `tvs-3-implementation-2` (oldest first):

| Commit | Content |
|---|---|
| `d5ea25c4a8` .. `5f6617e792` | API, headers and spec design and review (header declarations, no implementation). |
| `caa9a2d818` | Drift log copied; implementation starts here. |
| `147963955c` | `CollectionAccessor2`. |
| `aab9218bea`, `abfc580414`, `7da9913263` | `FenwickTree`, `ShapedNode`, `SourceRecord`, flat `SourceTable`. |
| `ac5061f7d5` | Shape spec. |
| `a4ce1ca803`, `4ab3e5380c`, `ce3b045b53`, `cd7b6ac310` | Source hierarchy (`ParentBy`, `ChildrenBy`). |
| `0bc7be2167`, `7b30035863` | `ShapedTree`, `GroupExpansion`. |
| `980b9646bb` | `ShapingEngine`. |
| `5e17dd9c49` | `ShapingManager`, view adapters, source wiring. |
| `8aca9b7e77` | Phase 6: `TableView` accepts `TableViewSource2`. |
| `d12324071b` | v2 API tests ported; posted-callback errors reported. |
| `c7dcde2922`, `75fa7175ff` | `StrongRef` for per-record references; `GroupNode::Key` stays `tracker_ref` (#138). |

A new variation should branch from `75fa7175ff` (or from `caa9a2d818` to re-implement from the
headers), start its own drift log copy, and refer to this branch's rows as the baseline.

## Code map

All under `controls\dev\TableViewSource2`:

| Folder | Content |
|---|---|
| `Source\` | `CollectionAccessor2`, `SourceRecord`, `SourceTable`, `ThreadBinding.h` (`ThrowIfOffBoundThread`, `ReportPostedError`). |
| `Source\Hierarchy\` | `SourceHierarchy` strategy, `ParentKeyHierarchy`, `ChildCollectionsHierarchy`. |
| `Spec\` | Shape spec, `ShapeDelegates` registry. |
| `Engine\` | `ShapedTree`, `GroupExpansion`, `ShapeState` (including `StrongRef`), `ShapingEngine`. |
| `Shaping\` | `ShapingManager` coordinator (live drain, `DeferRefresh`, owner arbitration). |
| `View\` | `ShapedCollectionView`, `ShapedRowQueries`, `ShapedRowMetadata`. |

Tests: `controls\dev\TableView\APITests\TableViewSource2_*_APITests.cs` (ten files, 170 tests) and
the shared fixtures in `TableView_Hierarchy_APITests_Common.cs`.

## Workflow rules

- Settle design decisions one at a time, with the options and a recommendation, before writing code.
- Implement one component at a time against the frozen spec and headers; compare with the first
  implementation; log every departure as a new row in the drift log (next free row: #139) and
  update the working spec copy to match. Rows carry a `pending` review status.
- Build and run the v2 API tests after each change.
- Commit only when asked. Use explicit `git add` paths. Never commit
  `controls\dev\dll-tabular\XamlMetadataProviderGenerated.h` or
  `controls\dev\dll\XamlMetadataProviderGenerated.h` (build-generated churn).
- Commit messages: imperative subject prefixed with the component (`TableViewSource2: ...`), body
  explaining why.
- Style: terse chat replies; normal, complete prose in code comments, docs and commit messages.
- `Header-Changes-2.md` uses LF line endings and new rows go directly before `## Headers`;
  `Shaping-Architecture-Redesign-2.md` uses CRLF.

## Build and test

Run from the repository root. Never run a full build, interaction tests or test-machine setup;
only API tests, locally.

Build the tabular DLL (about five minutes):

```powershell
.\initrun.ps1 msb /q "controls\dev\dll-tabular\Microsoft.UI.Xaml.Controls.Tabular.vcxproj" *> build.log
```

Build the test app only when test files change:

```powershell
.\initrun.ps1 msb /q "controls\test\MUXControlsTestApp\MUXControlsTestApp.csproj" *> cs.log
```

Deploy:

```powershell
$tab = 'BuildOutput\bin\amd64chk\Product\Microsoft.UI.Xaml.Controls.Tabular.dll'
foreach ($d in 'TestPayload\x64chk', 'TestPayload\x64chk\Test',
               'TestPayload\x64chk\Test\UnpackagedApps\MUXControlsTestApp') { Copy-Item $tab $d -Force }
Copy-Item 'BuildOutput\bin\amd64chk\Test\MUXControlsTestApp.dll' 'TestPayload\x64chk\Test' -Force
Copy-Item 'BuildOutput\bin\amd64chk\Test\UnpackagedApps\MUXControlsTestApp\MUXControlsTestApp.dll' `
          'TestPayload\x64chk\Test\UnpackagedApps\MUXControlsTestApp' -Force
```

Run (15 to 20 minutes for the suite; baseline is 170/170):

```powershell
Push-Location TestPayload\x64chk
.\runtests.cmd "*TableViewSource2*" *> full.log
Pop-Location
```

Search the log for `Summary: `, `[Failed]` and `Error:`. A single test uses
`"*TableViewSource2Tests.<Name>"`; the selector does not accept regex alternation. If every test is
`Blocked` with a test host crash in `ApiTestAssemblyHandling.AssemblyInitialize`, the run is
transient: rerun it.

## Lessons learned

- **Posted callbacks.** An exception escaping a `DispatcherQueue` callback fail-fasts the process
  directly (`RoFailFastWithErrorContext`); `Application.UnhandledException` is never raised. Every
  posted callback (live drain, `SourceTable` re-read, marshalled `PropertyChanged`) therefore
  catches and calls `ReportPostedError()`, which reports through `RoReportUnhandledError` so XAML
  raises `Application.UnhandledException` and the process fails fast only if the app leaves it
  unhandled (#7, #105).
- **Failed live drain.** It leaves the engine stale; the next change rebuilds and raises one `Reset`.
- **Tests and the dispatcher.** `RunOnUIThread.Execute` rethrows action exceptions into the
  dispatcher, so tests catch inside the action. Tests that expect a posted error use the
  `CatchDispatcherErrors` helper in the Live test file (`UnhandledException` with `Handled = true`).
- **Reference tracking cost.** Each `tracker_ref` set or destroy scans the owner's tracker handle
  list (O(N)), so per-record `tracker_ref`s made load, `Reset`, `Refresh` and rebuild O(N^2).
  `SourceRecord::Item`, `ShapeState::SortKeys` and `ParentBy` object keys hold a `StrongRef`
  instead (#138). Trade-off: an item or key that references the source or view model leaks until
  removed or cleared.
- **Group keys stay tracked.** `GroupNode::Key` must remain `tracker_ref`: object group keys commonly
  reference the view model, and `VerifyDelegatesDoNotLeakAfterSourceCollected` fails otherwise.
  Groups are few, so the cost is O(groups).
- **`IObservableVector` resets.** Read `args.Index()` only for item changes; it is undefined on
  `Reset` for some vectors (#137).
- **`ParentBy` orphans.** `OnRemoving` receives the leaving record's former root index so orphans land
  at their source-order slot (#136).

## Main departures from the frozen spec

The drift log is authoritative; the most consequential rows are: native `IItemsSourceView` (#118);
the error model (#5, #7, #94, #105); the `OnRemoving` signature (#8, #136); surgical reparenting
(#93); sort level reuse (#95); `clearOverlay` owned by the caller (#96); live changes folded into the
shaping verbs (#106); `ResyncSource` may throw (#116); hold generations (#110); expansion semantics
(#52 to #55, #89); type-in-place not wired (#128); header click as `DeferRefresh` (#129); level
numbering (#123); 64-bit ids (#76); `StrongRef` (#138).

## Open work

- Type-in-place for the v2 shaping verbs (#128).
- Native unit tests for the shaping engine.
- Run the v1 TableView API suites against a `TableViewSource2` source.
