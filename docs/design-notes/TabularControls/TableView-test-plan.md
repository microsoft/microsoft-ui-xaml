# TableView test plan

## Problem and approach

Create TableView tests from current repository state, using PR `!15971489` only as reference material. Do not port files wholesale. Define coverage categories first, then add tests area-by-area against the current public surface in `controls\dev\TableView\TableView.idl`, `controls\dev\TableView\TableViewSource.idl`, and internal primitives used by TableView (`ResizeGripper`, `SortIndicator`).

Current state:

- TableView product code exists under `controls\dev\TableView`.
- TableView API tests are wired into `controls\test\MUXControlsTestApp` via `controls\dev\TableView\APITests\TableView_APITests.projitems`.
- No interaction tests or TestUI pages exist for TableView yet.
- Current TableView API is narrower than PR `!15971489`: only `None`/`Single` selection, `Select`/`Deselect`/`IsSelected`/`DeselectAll`, read-only `SelectedItem`/`SelectedIndex`, cell-scoped `CommitEdit`/`CancelEdit`, no public row edit API, no public multiple selection, no public column reorder/autosize.

## Test files

- `controls\dev\TableView\APITests\TableView_APITests.projitems` — done
- `controls\dev\TableView\APITests\TableViewTests.cs` — done (initialization tests)
- `controls\dev\TableView\APITests\TableView_Columns_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_DataBinding_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Selection_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Sorting_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableViewSource_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Grouping_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Editing_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_AutomationPeer_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Theming_APITests.cs` — planned
- `controls\dev\TableView\InteractionTests\TableViewTests.cs` — later, only for real input/UIA flows
- `controls\dev\TableView\TestUI\TableViewPage.xaml` + `.xaml.cs` — later, required by interaction tests

## Legend

- `[x]` written and passing
- `[!]` written but failing or blocked
- `[ ]` not written yet
- `(deferred)` blocked on API that does not exist in this repo today; tracked, not written

---

## 1. Initialization, defaults, and XAML activation

Status: in progress. File: `TableViewTests.cs`.

### 1.1 Construction smoke

- [x] `VerifyTableViewConstructs` — `new TableView()` does not throw; derives from `Control`; `Columns` is created by the constructor and empty.
- [x] `VerifyTextColumnConstructs` — `new TableViewTextColumn()` does not throw; derives from `TableViewColumn`; starts unbound.
- [x] `VerifyTemplateColumnConstructs` — `new TableViewTemplateColumn()` does not throw; derives from `TableViewColumn`; no cell template.
- [x] `VerifyColumnConstructs` — `new TableViewColumn()` does not throw; derives from `DependencyObject`. Pins the derivation contract the IDL calls out.
- [x] `VerifyRowConstructs` — `new TableViewRow()` does not throw standalone; derives from `Control`; `IsSelected` false with no owner.
- [x] `VerifyCellsPanelConstructs` — `new TableViewCellsPanel()` does not throw standalone; derives from `Panel`; hosts no children.

### 1.2 Default property values

- [x] `VerifyDefaultPropertyValues` — TableView defaults for `ItemsSource`, `Columns`, `HeadersVisibility`, `GridLinesVisibility`, `CanUserResizeColumns`, `CanUserSortColumns`, `IsReadOnly`, `IsEditing`, `SelectionMode`, `SelectedItem`, `SelectedIndex`, `Density`, `EmptyTemplate`, `GroupHeaderTemplate`, `RowBackground`, `AlternatingRowBackground`.
- [x] `VerifyColumnDefaultPropertyValues` — `TableViewColumn` defaults for `Header`, `HeaderTemplate`, `HeaderTemplateSelector`, `HeaderToolTip`, `Width`, `MinWidth`, `MaxWidth`, `ActualWidth`, `CanResize`, `CanSort`, `SortCycle`, `SortDirection`, `SortMemberPath`, `CustomSortComparer`, `IsReadOnly`, `Visibility`, `FrozenEdge`, `CellEditingTemplate`, `CellToolTipBinding`.
- [x] `VerifyTextColumnDefaultPropertyValues` — `Binding` and `CellToolTipBinding` default to null, and the base column defaults are inherited.
- [x] `VerifyTemplateColumnDefaultPropertyValues` — `CellTemplate` and `CellEditingTemplate` default to null, and the base column defaults are inherited.
- [x] `VerifyRowDefaultPropertyValues` — a standalone `TableViewRow` reports `IsSelected` false through both the CLR property and `IsSelectedProperty`.

### 1.3 Dependency property identity

- [x] `VerifyDependencyProperties` — every public TableView DP exposes a non-null static identifier.
- [x] `VerifyDependencyPropertyBacking` — every settable TableView DP round-trips both ways (`SetValue` is visible to the CLR getter, and the CLR setter is visible to `GetValue`); read-only `SelectedItem`/`SelectedIndex` agree with their CLR getters and `Columns` is reachable through `ColumnsProperty`.
- [x] `VerifyColumnDependencyProperties` — every `TableViewColumn`, `TableViewTemplateColumn`, `TableViewRow`, and `TableViewGroupHeader` DP exposes a non-null static identifier.
- [x] `VerifyColumnDependencyPropertyBacking` — every settable column, group header, and row DP round-trips both ways; read-only `ActualWidth`, `SortDirection`, and `TableViewRow.IsSelected` agree with their CLR getters.

### 1.4 XAML activation

- [x] `VerifyXamlActivationWithInlineColumns` — XAML load with inline columns does not recurse or crash, and covers both `[contentproperty]` declarations: children land in `Columns` with no property-element wrapper, and a column's child content sets `Header`.
- [x] `VerifyXamlActivationWithoutColumns` — a bare `<TableView />` parses, creates an empty `Columns`, and still applies its template.
- [x] `VerifyXamlActivationWithTemplateColumn` — `TableViewTemplateColumn` with an inline `CellTemplate` parses and retains the template through load.

These use `XamlReader.Load`, which exercises the **runtime** XAML parser only. The markup-compile path (XamlTypeInfo generation, `x:Bind`, compile-time content-property resolution) is not covered by any API test and needs a compiled TestUI `.xaml` page. Tracked as a gap; fold into the TestUI work for category 11.

### 1.5 Template application

- [x] `VerifyTemplatePartsAfterTemplateApplication` — required named parts resolve from the default template. This is a real API contract: `TableView.idl` documents that re-templates must supply these parts. It also subsumes default-style resolution — parts cannot resolve unless the style from `TabularControlsResources` applied.
- `(dropped)` `VerifyDefaultStyleResolves` — redundant with the above. Genuine style coverage for `TableViewRow`, `TableViewGroupHeader`, `SortIndicator`, and `ResizeGripper` lives in 13.1, where nothing covers it today.
- `(dropped)` `VerifyApplyTemplateIsIdempotent` — near-vacuous as specified: `FrameworkElement.ApplyTemplate()` returns false and no-ops once a template is applied, so calling it twice asserts nothing. The path that can actually re-enter `OnApplyTemplate` is a `Template`/`Style` swap; the repo covers that only through interaction tests with dedicated TestUI pages (`ProgressBarReTemplatePage` + `ProgressBarTests.ReTemplateChangeStateTest`). Revisit as an interaction test if re-templating becomes a supported scenario worth guarding.

Repo precedent reviewed for this area: no control has a "default style resolves" test. The closest equivalent is snapshot verification via `VisualTreeTestHelper.VerifyVisualTree(root:, verificationFileNamePrefix:)` against a master in `controls\test\MUXControlsTestApp\verification\` (46 masters; used by ColorPicker, AutoSuggestBox, ComboBox, TreeView, PersonPicture, NavigationView, CalendarView). Deferred for TableView: the template is still changing, and every edit would force regenerating the master. Revisit once the template stabilizes by adding `verification\TableView.xml`.

### 1.6 Load, unload, reload

- [x] `ValidateLoadUnload` — unload, reload, and reparent the control; template parts and columns survive, and the selection stashed by `OnTableViewUnloaded` is restored on reload. Also asserts `Loaded`/`Unloaded` alternate rather than nest. Named after `Repeater\APITests\ViewportTests.ValidateLoadUnload`, which it is modeled on.
- [x] `VerifyRepeatedLoadUnloadDoesNotLeakSubscriptions` — five load/unload cycles against a `CountingItemsSource` (an instrumented `IList, INotifyCollectionChanged` with explicit event accessors); the listener count never exceeds one, never goes negative, and ends where it started. The count tracks live `ItemsSourceView` instances, since XAML's view is what actually subscribes.
- [x] `VerifyUnloadWhileItemsSourcePendingDoesNotCrash` — five create/load/unload/collect cycles that tear down before first layout settles, exercising `OnTableViewUnloaded` against a control whose rows pipeline still has queued work. Modeled on `InkCanvasTests.InkCanvasDestroyedBeforePendingCallbacksDoesNotCrash`, which exists for a use-after-free in the same family as the 1.7 assert.

Repo precedent reviewed for this area: strong, and **all API tests**. `Repeater\APITests\ViewportTests.ValidateLoadUnload` is the gold standard (reparents between hosts, counts load/unload, bounds subscriber count). `InkCanvasTests.InkCanvasDestroyedBeforePendingCallbacksDoesNotCrash` and `InkCanvasLoadUnloadDoesNotCrash` cover teardown-during-pending-work. Leak coverage uses a `WeakReference` plus a bounded GC loop (`RepeaterTests.VerifyRepeaterDoesNotLeakItemContainers`, `ScrollViewerAdapterTests.CheckLeaks`). The only interaction-test unload precedent is `TeachingTipTests.TargetUnloadingClosesTeachingTip`, and that is there because it observes a popup, not because unload needs UIA.

Note: `IdleSynchronizer.Wait()` ticks the UI thread, which can mask teardown races. `ViewportTests:613` uses `Task.Delay(16 * 3).Wait()` instead for exactly that reason. If these tests pass but a teardown crash is still suspected, retry with an untickied wait before concluding the path is clean.

### 1.7 Empty source and EmptyTemplate

`TableView::UpdateEmptyState` (`TableView.cpp:1084`) has exactly three branches, and every test below is pinned to one of them:

1. `EmptyTemplate == null` → presenter collapsed, presenter `ContentTemplate` **nulled**, repeater visible. Unconditional opt-out.
2. `EmptyTemplate != null` and the row count is 0 (**including when there is no `ItemsSourceView` at all** — `isEmpty` defaults to `true`) → presenter takes the template, gets non-null content so the `ContentControl` inflates, becomes visible; repeater collapsed.
3. `EmptyTemplate != null` and the row count is non-zero → presenter collapsed, repeater visible. Note this branch does **not** clear `ContentTemplate`, so no test may assert that it does.

Re-evaluation is driven from four call sites only: `RefreshRowsPipeline` (which `OnApplyTemplate`, repeater `Loaded`, and shaping verbs all route through), `OnEmptyTemplatePropertyChanged`, `OnEmptyStateItemsSourceCollectionChanged`, and `TableView_Sort.cpp:554`. The collection-changed subscription is wired **only while `EmptyTemplate` is non-null**, so the live-update tests are also subscription-lifetime tests.

- [!] `VerifyEmptyTemplateShowsForEmptyItemsSource` — branch 2 with an empty `List<Person>`. **Crashes with `0xC0000420` assert in `Microsoft.UI.Xaml.Controls.Tabular.dll`; under investigation.**
- [x] `VerifyNullItemsSourceShowsEmptyTemplate` — branch 2 reached via a null source rather than an empty collection, covering the `isEmpty` default-true path.
- [x] `VerifyEmptyTemplateHiddenWhenItemsPresent` — branch 3; two items present collapse the presenter and show the repeater.
- [x] `VerifyEmptyTemplateAppearsWhenSourceBecomesEmpty` — removing the last item from an `ObservableCollection` drives `OnEmptyStateItemsSourceCollectionChanged` from branch 3 to branch 2.
- [x] `VerifyEmptyTemplateDisappearsWhenItemAdded` — the reverse direction, branch 2 to branch 3. Kept separate from the above because the subscription is conditional and a regression can break one direction only.
- [x] `VerifySettingEmptyTemplateWhileEmptyShowsIt` — assigning `EmptyTemplate` **after** load on an already-empty source; covers `OnEmptyTemplatePropertyChanged`, which is a different entry point from the set-before-load case.
- [x] `VerifyClearingEmptyTemplateRestoresRows` — branch 1 reached by clearing a previously-set template; the only test that asserts `ContentTemplate` is nulled, so a stale template cannot sit inflated behind a collapsed presenter.
- [x] `VerifyNullItemsSourceWithoutEmptyTemplateShowsNothing` — branch 1 for a control that never had a template; no empty presenter, no crash.

All eight assert through the shared `VerifyEmptyStateVisibility` helper, which checks the contract **two-sided** — presenter visible implies repeater collapsed and vice versa. Asserting only the presenter would pass while the control showed both surfaces at once.

Repo precedent reviewed for this area: **none exists.** No other control in `controls\dev` has an empty-state, placeholder, or no-results template, and no test file outside TableView mentions one. TableView is the first, so there is no established shape to follow; the tests are derived from the product code instead. Placement is **API tests**: every part of the contract is observable through `Visibility`, `ContentTemplate`, and the inflated visual tree, with no input, hit-testing, or UIA involved, so an interaction test would add app-navigation flake for nothing.

- `(covered)` `VerifyEmptyTemplateSetAfterLoadAppliesLive` — leftover from the first draft of this checklist; `VerifySettingEmptyTemplateWhileEmptyShowsIt` above is the same test against the same `OnEmptyTemplatePropertyChanged` path.

**Category 1 is complete.** 30 tests in `TableViewTests.cs` across 1.1-1.7, minus the two dropped 1.5 items. One known failure remains: `VerifyEmptyTemplateShowsForEmptyItemsSource` (`0xC0000420`).

---

## 2. Data binding and data source projection

File: `TableView_DataBinding_APITests.cs`.

### 2.1 ItemsSource shapes

- [ ] `VerifyItemsSourceRoundtripsNull` — `ItemsSource` accepts null and reads back null with no rows realized.
- [ ] `VerifyItemsSourceRoundtripsList` — `List<T>` renders one row per item.
- [ ] `VerifyItemsSourceRoundtripsObservableCollection` — An `ObservableCollection<T>` reads back by reference and renders one row per item.
- [ ] `VerifyItemsSourceRoundtripsTableViewSource` — A `TableViewSource` reads back by reference and drives row projection.
- [ ] `VerifyItemsSourceSwapReplacesAllRows` — Assigning a different source discards the old rows and builds rows for the new items.
- [ ] `VerifyItemsSourceSetToNullClearsRows` — Setting `ItemsSource` back to null removes all realized rows.

### 2.2 Text column binding

- [ ] `VerifyTextColumnBindingPopulatesCellText` — bound property appears in the generated `TextBlock`.
- [ ] `VerifyTextColumnDottedBindingPath` — nested path resolves.
- [ ] `VerifyTextColumnNullBindingRendersEmptyCell` — no `ToString()` fallback.
- [ ] `VerifyTextColumnInvalidBindingPathRendersEmptyCell` — bad path does not crash.
- [ ] `VerifyTextColumnBindingChangeAfterLoadUpdatesCells` — Replacing `Binding` on a loaded column re-projects all realized cells.

### 2.3 Template column binding

- [ ] `VerifyTemplateColumnCellTemplateReceivesRowItem` — DataContext is the row item.
- [ ] `VerifyTemplateColumnNullCellTemplateRendersEmptyCell` — A template column with no `CellTemplate` renders an empty cell.
- [ ] `VerifyTemplateColumnCellTemplateChangeRebuildsCells` — Assigning a new `CellTemplate` after load rebuilds the cell content.

### 2.4 Live source updates

- [ ] `VerifyObservableAddInsertsRow` — Appending to an observable source adds one row at the end.
- [ ] `VerifyObservableInsertAtIndexPlacesRowInOrder` — Inserting at an index places the new row at that visual position.
- [ ] `VerifyObservableRemoveRemovesRow` — Removing an item removes exactly its row and shifts the rest up.
- [ ] `VerifyObservableReplaceUpdatesRowInPlace` — Replacing an item updates that row's cells without disturbing neighbours.
- [ ] `VerifyObservableMoveReordersRows` — A move notification reorders rows without rebuilding the whole list.
- [ ] `VerifyObservableResetRebuildsAllRows` — A reset notification discards stale rows and rebuilds from the current items.
- [ ] `VerifyRapidObservableMutationsSettleToExpectedRows` — A burst of adds/removes in one frame settles to the correct final row set.

### 2.5 Item property change notification

- [ ] `VerifyInpcUpdatesBoundTextCell` — An `INotifyPropertyChanged` raise on a realized item updates its bound text cell.
- [ ] `VerifyInpcOnUnrealizedItemDoesNotCrash` — A property change on an item with no realized row is safely ignored.
- [ ] `VerifyInpcAfterSourceSwapDoesNotUpdateStaleRows` — Items from a discarded source no longer drive cell updates after a source swap.

### 2.6 Error and edge inputs

- [ ] `VerifyUnsupportedItemsSourceTypeBehavesPerContract` — A non-enumerable `ItemsSource` fails per the documented contract instead of corrupting state.
- [ ] `VerifyThrowingEnumeratorLeavesControlStateIntact` — A source whose enumerator throws leaves the control usable for a subsequent valid source.
- [ ] `VerifyDuplicateItemInstancesAreHandled` — The same object instance appearing twice in the source produces two independent rows.

---

## 3. Columns and headers

File: `TableView_Columns_APITests.cs`.

### 3.1 Columns collection

- [ ] `VerifyColumnsCollectionIsObservable` — `Columns` raises collection-changed notifications that the control observes.
- [ ] `VerifyAddColumnAddsHeaderAndCells` — Adding a column creates its header and a cell in every realized row.
- [ ] `VerifyInsertColumnPlacesHeaderAtIndex` — Inserting a column places its header and cells at the requested position.
- [ ] `VerifyRemoveColumnRemovesHeaderAndCells` — Removing a column removes its header and its cell from every row.
- [ ] `VerifyClearColumnsRemovesAllHeaders` — `Columns.Clear()` removes every header and leaves rows with no cells.
- [ ] `VerifyReplaceColumnSwapsHeaderAndCells` — Replacing a column at an index swaps header and cells at that position only.
- [ ] `VerifyColumnsChangedBeforeLoadAppliesOnLoad` — Column edits made before the control loads are reflected on first layout.

### 3.2 Column ownership

No public owning-TableView accessor exists in `TableView.idl` today, so ownership is only observable indirectly.

- [ ] `VerifyColumnAddedToTwoTableViewsBehavesPerContract` — adding one column instance to two controls.
- [ ] `VerifyRemovedColumnStopsAffectingRows` — a removed column no longer generates cells or participates in sort.
- [ ] `VerifyRemovedColumnCanBeReAdded` — re-adding a previously removed column rebuilds its header and cells.
- `(deferred)` `GetOwningTableView()` round-trip tests. PR `!15971489` tests this; the accessor is not in this repo's IDL.

### 3.3 Header content

- [ ] `VerifyStringHeaderRenders` — A string `Header` appears as header text.
- [ ] `VerifyObjectHeaderRenders` — A non-string `Header` object renders through normal content presentation.
- [ ] `VerifyNullHeaderRendersEmptyHeader` — A null `Header` renders an empty header cell rather than crashing.
- [ ] `VerifyHeaderChangeAfterLoadUpdatesLive` — Changing `Header` on a loaded column updates the rendered header text.
- [ ] `VerifyHeaderTemplateApplies` — `HeaderTemplate` content replaces the default header presentation.
- [ ] `VerifyHeaderTemplateSelectorApplies` — `HeaderTemplateSelector` is consulted when no `HeaderTemplate` is set.
- [ ] `VerifyHeaderTemplateTakesPrecedenceOverSelector` — With both set, `HeaderTemplate` wins and the selector is not consulted.

### 3.4 Headers visibility

- [ ] `VerifyHeadersVisibilityColumnShowsHeaderRow` — `HeadersVisibility.Column` renders a visible header row.
- [ ] `VerifyHeadersVisibilityNoneCollapsesHeaderRow` — `HeadersVisibility.None` collapses the header row and reclaims its height.
- [ ] `VerifyHeadersVisibilityChangeAfterLoadUpdatesLive` — Toggling `HeadersVisibility` on a loaded control updates the header row immediately.

### 3.5 Column visibility

- [ ] `VerifyCollapsedColumnRemovesCellsFromLayout` — A collapsed column contributes no width and shows no cells.
- [ ] `VerifyCollapsedColumnPreservesColumnState` — A collapsed column keeps its width, sort, and header state while hidden.
- [ ] `VerifyRestoringColumnVisibilityRestoresCells` — Setting `Visibility` back to `Visible` restores the header and cells with prior state.
- [ ] `VerifyHeadersStayAlignedWithCellsAfterVisibilityChange` — Header offsets still match cell offsets after a visibility toggle.

---

## 4. Column sizing, layout, resize, and frozen columns

File: `TableView_Columns_APITests.cs`.

### 4.1 Width property surface

- [ ] `VerifyColumnWidthRoundtripsPixelValue` — A pixel `GridLength` reads back with the same value and `GridUnitType.Pixel`.
- [ ] `VerifyColumnWidthRoundtripsAuto` — `GridLength.Auto` reads back as `GridUnitType.Auto`.
- [ ] `VerifyColumnWidthRoundtripsStar` — A star `GridLength` reads back with the same weight and `GridUnitType.Star`.
- [ ] `VerifyColumnMinWidthRoundtrip` — `MinWidth` reads back the value that was assigned.
- [ ] `VerifyColumnMaxWidthRoundtrip` — `MaxWidth` reads back the value that was assigned.

### 4.2 ActualWidth resolution

- [ ] `VerifyActualWidthFollowsPixelWidth` — `ActualWidth` equals a fixed pixel `Width` after layout.
- [ ] `VerifyActualWidthClampsToMinWidth` — A `Width` below `MinWidth` resolves to `MinWidth`.
- [ ] `VerifyActualWidthClampsToMaxWidth` — A `Width` above `MaxWidth` resolves to `MaxWidth`.
- [ ] `VerifyActualWidthFollowsMinWidthChange` — Raising `MinWidth` on a loaded column widens `ActualWidth`.
- [ ] `VerifyActualWidthFollowsMaxWidthChange` — Lowering `MaxWidth` on a loaded column narrows `ActualWidth`.
- [ ] `VerifyMinWidthGreaterThanMaxWidthIsDeterministic` — An inverted min/max pair resolves to a documented, stable value rather than undefined clamping.
- [ ] `VerifyNonFiniteWidthIsIgnoredOrClamped` — `NaN` and infinity widths are ignored or clamped instead of poisoning layout.

### 4.3 Auto and star sizing

- [ ] `VerifyAutoWidthFitsWidestRealizedCell` — An auto column sizes to the widest realized cell content.
- [ ] `VerifyAutoWidthAccountsForHeaderWidth` — An auto column is at least as wide as its own header.
- [ ] `VerifyAutoWidthGrowsAsWiderRowsRealize` — Scrolling a wider row into view grows the auto column.
- [ ] `VerifyStarWidthDividesRemainingViewport` — Star columns split leftover viewport width in proportion to their weights.
- [ ] `VerifyStarWidthRespectsFixedColumnOverflow` — Fixed columns wider than the viewport leave star columns at their minimum.
- [ ] `VerifyStarWidthInUnboundedHostFallsBackDeterministically` — In an infinite-width host, star columns fall back to a defined finite size.

### 4.4 Layout invalidation

- [ ] `VerifyDensityChangeRemeasuresRowsAndHeaders` — Changing `Density` invalidates measure so row and header heights update.
- [ ] `VerifyHeaderChangeRemeasuresAutoColumn` — A longer header re-measures and widens an auto column.
- [ ] `VerifyCellTemplateChangeRemeasuresAutoColumn` — A wider `CellTemplate` re-measures and widens an auto column.

### 4.5 Resize gating and gesture

- [ ] `VerifyCanUserResizeColumnsFalseHidesGripper` — `CanUserResizeColumns = false` removes the resize affordance from every header.
- [ ] `VerifyPerColumnCanResizeFalseHidesGripper` — `CanResize = false` removes the affordance from that column only.
- [ ] `VerifyResizeDragUpdatesColumnWidth` — A drag delta on the gripper changes the column width by that delta.
- [ ] `VerifyResizeDragRespectsMinWidth` — Dragging narrower than `MinWidth` stops at `MinWidth`.
- [ ] `VerifyResizeDragRespectsMaxWidth` — Dragging wider than `MaxWidth` stops at `MaxWidth`.
- [ ] `VerifyResizeCancelRestoresAuthoredWidth` — Cancelling a drag restores the `GridLength` that was authored before the gesture.
- [ ] `VerifyResizeConvertsStarOrAutoToPixelWidth` — Resizing an auto or star column converts it to an explicit pixel width.
- [ ] `VerifyKeyboardResizeAdjustsWidth` — The header keyboard resize path changes width by the standard step.
- [ ] `VerifyKeyboardResizeShiftMultiplierAppliesLargerDelta` — Holding Shift applies the larger keyboard resize step.
- [ ] `VerifyKeyboardResizeMirrorsInRightToLeft` — Under RTL, the keyboard resize direction is mirrored.

### 4.6 Frozen columns

- [ ] `VerifyLeadingFrozenColumnStaysPinnedDuringHorizontalScroll` — A `FrozenEdge.Leading` column keeps its viewport offset while other columns scroll.
- [ ] `VerifyMultipleLeadingFrozenColumnsStayPinnedInOrder` — Several leading frozen columns stay pinned side by side in declaration order.
- [ ] `VerifyNonContiguousLeadingFrozenOnlyFreezesPrefix` — Only the contiguous leading prefix freezes; a later frozen column scrolls normally.
- [ ] `VerifyTrailingFrozenEdgeIsReservedNoOp` — `FrozenEdge.Trailing` is accepted but has no layout effect in the current implementation.
- [ ] `VerifyFrozenColumnsMirrorInRightToLeft` — Under RTL, leading frozen columns pin to the right edge.

---

## 5. Rows, cells, and visual states

File: `TableView_Columns_APITests.cs` / row-specific file if it grows.

### 5.1 Row structure

- [ ] `VerifyRowTemplatePartsExist` — The default row template exposes its required named parts after `OnApplyTemplate`.
- [ ] `VerifyRowCellCountMatchesVisibleColumns` — A realized row holds exactly one cell per visible column.
- [ ] `VerifyRowCellOrderMatchesColumnOrder` — Cell order in the row matches the order of `Columns`.
- [ ] `VerifyRowDataContextIsRowItem` — A realized row's `DataContext` is the backing item from the source.

### 5.2 Cell content

- [ ] `VerifyTextCellIsVerticallyCentered` — Generated text cell content is vertically centred within the row.
- [ ] `VerifyTextCellTrimsOverflowText` — Text wider than the column is trimmed rather than wrapped or clipped mid-glyph.
- [ ] `VerifyTextCellUsesDensityPadding` — Text cell padding follows the current `Density` value.
- [ ] `VerifyTemplateCellHostsTemplateContent` — The template column's realized content is a child of the cell wrapper.

### 5.3 Recycling

- [ ] `VerifyRecycledRowUpdatesDataContext` — A recycled row's `DataContext` points at the new item, never the previous one.
- [ ] `VerifyRecycledRowUpdatesTextCells` — Recycled text cells show the new item's values with no stale text.
- [ ] `VerifyRecycledRowUpdatesTemplateCells` — Recycled template cells re-bind to the new item.
- [ ] `VerifyRecycledRowUpdatesAlternatingBackground` — Banding follows the new row index after recycling.
- [ ] `VerifyRecycledRowClearsSelectionVisual` — A recycled row does not carry the previous row's selection visual.

### 5.4 Row backgrounds and gridlines

- [ ] `VerifyRowBackgroundApplies` — `RowBackground` paints every row.
- [ ] `VerifyAlternatingRowBackgroundAppliesToOddRows` — `AlternatingRowBackground` paints only the alternating rows.
- [ ] `VerifyRowBackgroundChangeAfterLoadUpdatesLive` — Changing either background brush after load repaints realized rows.
- [ ] `VerifyGridLinesVisibilityAllDrawsBothSeparators` — `All` renders both horizontal and vertical separators.
- [ ] `VerifyGridLinesVisibilityHorizontalDrawsOnlyHorizontal` — `Horizontal` renders row separators and no column separators.
- [ ] `VerifyGridLinesVisibilityVerticalDrawsOnlyVertical` — `Vertical` renders column separators and no row separators.
- [ ] `VerifyGridLinesVisibilityNoneDrawsNoSeparators` — `None` renders no separators at all.

### 5.5 Visual states

- [ ] `VerifyRowSelectedVisualState` — A selected row enters the selected visual state.
- [ ] `VerifyRowPointerOverVisualState` — Pointer-over enters the hover visual state and reverts on exit.
- [ ] `VerifyRowFocusVisualOnKeyboardFocus` — Keyboard focus shows the focus visual; pointer focus does not.
- [ ] `VerifyDisabledTableViewVisualState` — `IsEnabled = false` puts the control and its rows into the disabled state.

---

## 6. Selection

File: `TableView_Selection_APITests.cs`.

### 6.1 Defaults and mode

- [ ] `VerifySelectionDefaults` — `Single`, `SelectedIndex == -1`, `SelectedItem == null`.
- [ ] `VerifySelectionModeRoundtrip` — `SelectionMode` reads back the value that was assigned.
- [ ] `VerifySelectionModeNoneClearsExistingSelection` — Switching to `None` clears any current selection.
- [ ] `VerifySelectionModeNoneIgnoresSelectCall` — `Select` is a no-op while `SelectionMode` is `None`.
- [ ] `VerifySwitchingModeAfterLoadIsHonored` — A mode change after load takes effect without a reload.

### 6.2 Single-selection API

- [ ] `VerifySelectByIndexUpdatesSelectedItemAndIndex` — `Select(index)` sets both `SelectedIndex` and the matching `SelectedItem`.
- [ ] `VerifySelectingSecondIndexClearsFirst` — In single mode, selecting another row deselects the previous one.
- [ ] `VerifyDeselectByIndexClearsSelection` — `Deselect(index)` resets `SelectedIndex` to -1 and `SelectedItem` to null.
- [ ] `VerifyDeselectUnselectedIndexIsNoOp` — Deselecting a row that is not selected leaves selection unchanged.
- [ ] `VerifyDeselectAllClearsSelection` — `DeselectAll()` clears the current selection.
- [ ] `VerifyDeselectAllIsIdempotent` — Calling `DeselectAll()` twice is safe and raises no second change.
- [ ] `VerifyIsSelectedReflectsSelection` — `IsSelected(index)` is true only for the selected index.
- [ ] `VerifyIsSelectedToleratesInvalidIndex` — `IsSelected` returns false for out-of-range indices instead of throwing.
- [ ] `VerifySelectOutOfRangeIsTolerated` — `Select` past the end behaves per contract and leaves state coherent.
- [ ] `VerifySelectNegativeIndexIsTolerated` — `Select(-1)` behaves per contract and leaves state coherent.

### 6.3 Selection and source mutation

- [ ] `VerifyInsertBeforeSelectedRowKeepsSelectedItem` — Inserting above the selection keeps `SelectedItem` and shifts `SelectedIndex`.
- [ ] `VerifyRemoveBeforeSelectedRowKeepsSelectedItem` — Removing above the selection keeps `SelectedItem` and shifts `SelectedIndex`.
- [ ] `VerifyRemovingSelectedItemClearsSelection` — Removing the selected item clears selection rather than selecting a neighbour silently.
- [ ] `VerifyItemsSourceSwapClearsSelection` — Assigning a new source resets selection to the empty state.
- [ ] `VerifyResetClearsSelection` — A collection reset clears selection.

### 6.4 Selection events

- [ ] `VerifySelectionChangedFiresWithAddedItems` — Selecting a row raises `SelectionChanged` with that item in `AddedItems`.
- [ ] `VerifySelectionChangedFiresWithRemovedItems` — Replacing a selection reports the old item in `RemovedItems`.
- [ ] `VerifySelectionChangedDoesNotFireForSameSelection` — Re-selecting the already-selected index raises no event.
- [ ] `VerifySelectionChangedDoesNotFireOnNoOpDeselect` — Deselecting an unselected index raises no event.
- [ ] `VerifySelectionChangedFiresOnceForModeChangeClear` — Switching to `None` raises exactly one clearing event.

### 6.5 Row selection surface

- [ ] `VerifyRowIsSelectedFollowsOwnerSelection` — `TableViewRow.IsSelected` tracks the owning control's selection.
- [ ] `VerifyRowIsSelectedClearedOnDeselectAll` — `DeselectAll()` drives every realized row's `IsSelected` to false.

### 6.6 Deferred

- `(deferred)` Multiple/Extended selection modes, `SelectedItems`, `SelectedItemIndices`, `SelectAll`, `SelectRange`, Ctrl/Shift range gestures. Not in current IDL.

---

## 7. Sorting

File: `TableView_Sorting_APITests.cs`.

### 7.1 Column sort surface

- [ ] `VerifyColumnSortDefaults` — A fresh column reports the documented defaults for `CanSort`, `SortCycle`, `SortMemberPath`, `CustomSortComparer`, and `SortDirection`.
- [ ] `VerifyCanSortRoundtrip` — `CanSort` reads back the value that was assigned.
- [ ] `VerifySortCycleRoundtrip` — `SortCycle` reads back each `TableViewSortCycle` value.
- [ ] `VerifySortMemberPathRoundtrip` — `SortMemberPath` reads back the string that was assigned.
- [ ] `VerifyCustomSortComparerRoundtrip` — `CustomSortComparer` reads back the comparer instance that was assigned.
- [ ] `VerifyGetSortMemberPathReturnsExplicitPath` — `GetSortMemberPathCore()` returns the explicitly set path when one exists.
- [ ] `VerifyGetSortMemberPathFallsBackToTextColumnBindingPath` — With no explicit path, a text column falls back to its `Binding` path.

### 7.2 Programmatic sort state

- [ ] `VerifySortByColumnAscendingSetsDirection` — `SortByColumn(column, Ascending)` sets that column's `SortDirection` to ascending.
- [ ] `VerifySortByColumnDescendingSetsDirection` — `SortByColumn(column, Descending)` sets that column's `SortDirection` to descending.
- [ ] `VerifySortByColumnNoneClearsDirection` — `SortByColumn(column, None)` clears that column's direction.
- [ ] `VerifySortByColumnReplacesPreviousColumnSort` — Sorting a second column clears the first column's sort state.
- [ ] `VerifyToggleSortDirectionCyclesPerSortCycle` — Toggling follows the sequence defined by each `TableViewSortCycle` value.
- [ ] `VerifyToggleSortDirectionRespectsCanSort` — Toggling a column with `CanSort = false` does not change sort state.
- [ ] `VerifyClearSortClearsAllColumnState` — `ClearSort()` resets every column's `SortDirection` and restores source order.
- [ ] `VerifyClearSortIsIdempotent` — Calling `ClearSort()` on an unsorted control is a safe no-op.
- [ ] `VerifyCanUserSortColumnsFalseStillAllowsProgrammaticSort` — `CanUserSortColumns` gates only the header gesture, not the API.

### 7.3 Sort ordering results

- [ ] `VerifySortBySortMemberPathOrdersRows` — Sorting by a member path reorders rows by that property.
- [ ] `VerifySortDescendingReversesOrder` — Descending produces the reverse of the ascending order.
- [ ] `VerifyCustomSortComparerOrdersRows` — A custom comparer drives the row order.
- [ ] `VerifyCustomSortComparerTakesPrecedenceOverSortMemberPath` — With both set, the comparer wins over the member path.
- [ ] `VerifySortIsStableForEqualKeys` — Items with equal sort keys keep their relative source order.
- [ ] `VerifySortSurvivesItemAddition` — Adding an item does not drop the active sort.
- [ ] `VerifySortAppliesToItemAddedAfterSorting` — A newly added item lands at its sorted position, not at the end.

### 7.4 Sort events

- [ ] `VerifySortingEventFiresBeforeStateChange` — `Sorting` is raised while the column still holds its previous direction.
- [ ] `VerifySortingEventCarriesColumnAndDirection` — The `Sorting` args expose the target column and the requested direction.
- [ ] `VerifySortingEventCancelLeavesOrderUnchanged` — `Cancel = true` leaves the row order untouched.
- [ ] `VerifySortingEventCancelLeavesSortStateUnchanged` — `Cancel = true` leaves every column's `SortDirection` untouched.
- [ ] `VerifySortedEventFiresAfterSuccessfulSort` — `Sorted` is raised once after the new order is applied.
- [ ] `VerifySortedEventDoesNotFireWhenCanceled` — A cancelled sort raises no `Sorted` event.

### 7.5 Sort edge cases

- [ ] `VerifyDuplicateSortMemberPathsTrackColumnIdentity` — Two columns with the same path keep independent sort state.
- [ ] `VerifyRemovingSortedColumnClearsSortState` — Removing the sorted column clears the active sort safely.
- [ ] `VerifySortReconcilesWithTableViewSourceSort` — Column sort and `TableViewSource.Sort` resolve with the documented last-writer-wins rule.
- [ ] `VerifyHeaderInvokeTogglesSortAndUpdatesIndicator` — Invoking a header toggles sort and updates the `SortIndicator` and UIA sort metadata.

---

## 8. TableViewSource data shaping

File: `TableViewSource_APITests.cs`.

### 8.1 Construction

- [ ] `VerifyTableViewSourceFromList` — `TableViewSource.From` accepts a `List<T>` and projects every item.
- [ ] `VerifyTableViewSourceFromObservableCollection` — `From` accepts an `ObservableCollection<T>` and tracks its notifications.
- [ ] `VerifyTableViewSourceFromNullBehavesPerContract` — `From(null)` fails or yields an empty source per the documented contract.
- [ ] `VerifyTableViewSourceFromUnsupportedSourceThrows` — An unsupported source type is rejected rather than silently producing no rows.
- [ ] `VerifyTableViewSourceDefaultsAreUnshaped` — A fresh source has no filter, no grouping, and source order preserved.

### 8.2 Filtering

- [ ] `VerifyFilterReducesVisibleRows` — A predicate reduces projected rows to the matching items.
- [ ] `VerifyClearFilterRestoresAllRows` — `ClearFilter()` restores the unfiltered projection.
- [ ] `VerifyFilterChangeReprojectsRows` — Replacing the predicate re-evaluates every item.
- [ ] `VerifyNullFilterPredicateBehavesPerContract` — A null predicate clears filtering or throws per the documented contract.
- [ ] `VerifyThrowingFilterPredicateLeavesSourceIntact` — A predicate that throws leaves the source usable for a later valid filter.
- [ ] `VerifyFilterAppliesToItemsAddedLater` — Items added after filtering are tested against the active predicate.
- [ ] `VerifyFilteredOutItemRemovalDoesNotDisturbRows` — Removing an item that is filtered out leaves visible rows unchanged.

### 8.3 Grouping

- [ ] `VerifyGroupByCreatesGroupHeaders` — `GroupBy` inserts one group header per distinct key.
- [ ] `VerifyGroupByProducesExpectedGroupOrder` — Groups appear in the documented key order.
- [ ] `VerifyGroupByWithIdentitySelectorHandlesReferenceKeys` — An identity selector makes reference-type keys group by value, not instance.
- [ ] `VerifyClearGroupByRestoresFlatRows` — `ClearGroupBy()` removes headers and restores a flat row list.
- [ ] `VerifyGroupKeysOfDifferentTypesDoNotCollapse` — Keys that compare equal only after coercion stay in separate groups.
- [ ] `VerifyGroupKeyChangeMovesItemToNewGroup` — Changing an item's group key moves its row to the correct group.

### 8.4 Sorting

- [ ] `VerifySortByPathOrdersRows` — `Sort(path, direction)` orders the projection by that property path.
- [ ] `VerifySortByKeySelectorOrdersRows` — `Sort(keySelector, direction)` orders by the computed key.
- [ ] `VerifySortDirectionNoneClearsSortAxis` — Passing `None` clears the source-level sort axis.
- [ ] `VerifySourceClearSortRestoresSourceOrder` — `ClearSort()` on the source restores original item order.
- [ ] `VerifySortByKeySelectorClearsColumnHeaderAttribution` — A key-selector sort clears any column header sort indicator.

### 8.5 Composition

- [ ] `VerifyFilterAndSortCompose` — Filter then sort yields sorted rows drawn only from the filtered set.
- [ ] `VerifyGroupAndSortCompose` — Sorting inside grouping orders items within each group, not across groups.
- [ ] `VerifyFilterGroupAndSortCompose` — All three operations applied together yield the documented combined projection.
- [ ] `VerifyLastSortWriterWinsBetweenSourceAndColumn` — A later source sort overrides a column sort and vice versa.

### 8.6 Live updates while shaped

- [ ] `VerifyAddWhileFilteredRespectsPredicate` — An item added while filtered appears only if it matches.
- [ ] `VerifyAddWhileSortedInsertsInOrder` — An item added while sorted lands at its sorted position.
- [ ] `VerifyAddWhileGroupedLandsInCorrectGroup` — An item added while grouped joins the group matching its key.
- [ ] `VerifyRemoveWhileShapedUpdatesRows` — Removal while shaped updates the projection without a full rebuild artefact.
- [ ] `VerifyReplaceWhileShapedUpdatesRows` — Replacement while shaped re-evaluates filter, group, and sort for the new item.
- [ ] `VerifyMoveWhileSortedDoesNotDisturbSortedOrder` — A source move does not change the projected order while a sort is active.
- [ ] `VerifyPropertyChangeUpdatesSortedPosition` — Changing a sort key property moves the row to its new sorted position.
- [ ] `VerifyPropertyChangeUpdatesFilterMembership` — Changing a filtered property adds or removes the row from the projection.
- [ ] `VerifyRapidMutationsWhileShapedSettleCorrectly` — A burst of mutations under shaping settles to the correct final projection.

### 8.7 Identity and state

- [ ] `VerifySelectionReanchorsAcrossReshape` — Selection follows the selected item across a filter, group, or sort change.
- [ ] `VerifyFocusReanchorsAcrossReshape` — Keyboard focus follows the focused item across a reshape.
- [ ] `VerifyDuplicateRowIdentityIsObservable` — Duplicate object identity in a shaped source produces a detectable, documented failure rather than silent corruption.

---

## 9. Grouping and group headers

File: `TableView_Grouping_APITests.cs`.

### 9.1 TableViewGroupInfo

- [ ] `VerifyGroupInfoKeyAndItemCount` — `TableViewGroupInfo.Key` and `ItemCount` match the underlying group.
- [ ] `VerifyGroupInfoLevel` — `Level` reports the group's nesting depth.
- [ ] `VerifyGroupInfoIsExpandableAndIsExpanded` — `IsExpandable` and `IsExpanded` report the correct initial state.
- [ ] `VerifyGroupInfoKeyTextAndItemCountText` — `KeyText` and `ItemCountText` produce the display strings used by the default template.
- [ ] `VerifyGroupInfoRaisesPropertyChangedOnExpandCollapse` — Expanding or collapsing raises `PropertyChanged` for `IsExpanded`.

### 9.2 Group header rendering

- [ ] `VerifyDefaultGroupHeaderShowsKeyAndCount` — The default group header template displays the key and the item count.
- [ ] `VerifyGroupHeaderTemplateApplies` — `GroupHeaderTemplate` replaces the default group header content.
- [ ] `VerifyGroupHeaderTemplateChangeAfterLoadUpdatesLive` — Changing `GroupHeaderTemplate` after load rebuilds realized group headers.
- [ ] `VerifyGroupHeaderSpansAllColumns` — A group header spans the full column set rather than sitting in one column.

### 9.3 Expand and collapse

- [ ] `VerifyExpandAllGroupsIsNoOpOnUngroupedSource` — `ExpandAllGroups()` on a flat source is a safe no-op.
- [ ] `VerifyCollapseAllGroupsIsNoOpOnUngroupedSource` — `CollapseAllGroups()` on a flat source is a safe no-op.
- [ ] `VerifyCollapseAllGroupsHidesDataRowsKeepsHeaders` — Collapsing all groups leaves only group headers visible.
- [ ] `VerifyExpandAllGroupsRestoresDataRows` — Expanding all groups restores every data row.
- [ ] `VerifyToggleSingleGroupUpdatesOnlyThatGroup` — Toggling one group leaves the other groups' expand state untouched.
- [ ] `VerifyGroupHeaderToggleRequestedFires` — Invoking a group header raises `ToggleRequested`.
- [ ] `VerifyCollapsedGroupStatePersistsAcrossSourceUpdate` — A collapsed group stays collapsed after an unrelated source change.

### 9.4 Group edge cases

- [ ] `VerifyEmptyGroupRendersHeaderOnly` — A group with no items renders its header and no data rows.
- [ ] `VerifySingleItemGroupRenders` — A one-item group renders a header and exactly one row.
- [ ] `VerifyNonExpandableGroupIgnoresToggle` — A group with `IsExpandable` false ignores toggle requests.
- [ ] `VerifyGroupRemovedWhenLastItemRemoved` — Removing a group's last item removes the group header.

### 9.5 Group header visual states

- [ ] `VerifyGroupHeaderExpandedCollapsedVisualStates` — The group header enters the expanded or collapsed visual state to match `IsExpanded`.
- [ ] `VerifyGroupHeaderExpandableVisualStates` — The expander affordance is shown only when `IsExpandable` is true.
- [ ] `VerifyGroupHeaderPointerAndPressedVisualStates` — Pointer-over and pressed states apply and revert on the group header.

---

## 10. Editing

File: `TableView_Editing_APITests.cs`.

### 10.1 Gating

- [ ] `VerifyEditingDefaultsAreReadOnly` — `IsReadOnly` defaults true and `IsEditing` defaults false.
- [ ] `VerifyEditGestureIsIgnoredWhenReadOnly` — Edit gestures do nothing while the control is read-only.
- [ ] `VerifyEditEnabledWhenControlAndColumnAreWritable` — Editing starts only when both control and column are writable.
- [ ] `VerifyPerColumnReadOnlyBlocksEditForThatColumnOnly` — A read-only column blocks its own edits while sibling columns stay editable.
- [ ] `VerifyIsEditingReflectsActiveEdit` — `IsEditing` is true only while an edit session is open.

### 10.2 Editors

- [ ] `VerifyTextColumnDoubleClickCreatesTextBox` — Double-clicking an editable text cell swaps in a `TextBox` editor.
- [ ] `VerifyTextColumnF2CreatesTextBox` — F2 on a focused editable text cell swaps in a `TextBox` editor.
- [ ] `VerifyTemplateColumnUsesCellEditingTemplate` — A template column uses `CellEditingTemplate` for its editor.
- [ ] `VerifyEditorReceivesInitialValue` — The editor opens pre-populated with the cell's current value.
- [ ] `VerifyEditorGetsFocusOnBeginEdit` — Focus moves into the editor when editing begins.

### 10.3 Commit and cancel

- [ ] `VerifyCommitEditWritesValueToItem` — `CommitEdit()` writes the edited value back to the bound item property.
- [ ] `VerifyCommitEditReturnsFalseWhenBlocked` — `CommitEdit()` returns false when validation or a handler blocks the commit.
- [ ] `VerifyCancelEditRestoresOriginalValue` — `CancelEdit()` discards the edit and restores the original value.
- [ ] `VerifyEnterKeyCommitsEdit` — Enter commits the active edit and closes the editor.
- [ ] `VerifyEscapeKeyCancelsEdit` — Escape cancels the active edit and closes the editor.
- [ ] `VerifyFocusLossCommitsEdit` — Moving focus out of the editor commits the edit.

### 10.4 Edit events

- [ ] `VerifyBeginningEditFiresWithItemAndColumn` — `BeginningEdit` args expose the item and column being edited.
- [ ] `VerifyBeginningEditCancelPreventsEditor` — Cancelling `BeginningEdit` prevents the editor from being created.
- [ ] `VerifyCellEditEndingFiresWithCommitAction` — A commit raises `CellEditEnding` with the commit action.
- [ ] `VerifyCellEditEndingFiresWithCancelAction` — A cancel raises `CellEditEnding` with the cancel action.
- [ ] `VerifyCellEditEndingCancelPreventsCommit` — Cancelling `CellEditEnding` prevents the value from reaching the item.

### 10.5 Validation and teardown

- [ ] `VerifyValidationErrorBlocksCommit` — An `INotifyDataErrorInfo` error keeps the editor open and blocks commit.
- [ ] `VerifyValidationErrorClearsAfterValidValue` — Entering a valid value clears the error and allows commit.
- [ ] `VerifyItemsSourceResetWhileEditingClosesEditSafely` — A source reset during an edit closes the session without wedging state.
- [ ] `VerifyUnloadWhileEditingClosesEditSafely` — Unloading during an edit closes the session without wedging state.
- [ ] `VerifyReentrantEditCallsLeaveStateCoherent` — Calling begin or commit from inside an edit event leaves coherent state.

### 10.6 Deferred

- `(deferred)` Row-scoped editing, `RowEditEnding`, async deferrals, public `BeginEdit(item, column)`, `CurrentItem`, multi-cell transactions.

---

## 11. Keyboard and pointer interaction

File: `controls\dev\TableView\InteractionTests\TableViewTests.cs` (requires a TestUI page).

- [ ] `DownArrowMovesFocusToNextRow` — Down arrow moves focus to the following row.
- [ ] `UpArrowMovesFocusToPreviousRow` — Up arrow moves focus to the preceding row.
- [ ] `HomeKeyMovesToFirstRow` — Home moves focus to the first row.
- [ ] `EndKeyMovesToLastRow` — End moves focus to the last row.
- [ ] `PageDownMovesByViewport` — Page Down moves focus roughly one viewport down.
- [ ] `PageUpMovesByViewport` — Page Up moves focus roughly one viewport up.
- [ ] `TabMovesFocusOutOfTable` — Tab leaves the table rather than walking every cell.
- [ ] `HeaderEnterTogglesSort` — Enter on a focused header toggles that column's sort.
- [ ] `HeaderKeyboardResizeChangesWidth` — The header keyboard resize path changes the column width.
- [ ] `PointerClickSelectsRow` — Clicking a row selects and focuses it.
- [ ] `PointerDoubleClickBeginsEditWhenEditable` — Double-clicking an editable cell opens its editor.
- [ ] `PointerDoubleClickDoesNothingWhenReadOnly` — Double-clicking a read-only cell opens no editor.
- [ ] `PointerResizeDragChangesColumnWidth` — Dragging the gripper with the pointer resizes the column.
- [ ] `PointerResizeEscapeCancelsResize` — Escape during a pointer drag restores the pre-drag width.
- [ ] `HorizontalScrollKeepsHeaderAligned` — Headers scroll horizontally in lockstep with cells.
- [ ] `VerticalScrollKeepsHeaderSticky` — The header row stays pinned during vertical scroll.
- [ ] `RightToLeftResizeMirrors` — Under RTL, resize drag direction is mirrored.
- [ ] `RightToLeftKeyboardNavigationMirrors` — Under RTL, horizontal keyboard navigation is mirrored.

---

## 12. Accessibility and automation

File: `TableView_AutomationPeer_APITests.cs`.

### 12.1 TableView peer patterns

- [ ] `VerifyGridPatternOnTableViewPeer` — The TableView peer exposes `IGridProvider`.
- [ ] `VerifyTablePatternOnTableViewPeer` — The TableView peer exposes `ITableProvider`.
- [ ] `VerifySelectionPatternOnTableViewPeer` — The TableView peer exposes `ISelectionProvider` with the correct mode flags.
- [ ] `VerifyItemContainerPatternOnTableViewPeer` — The TableView peer exposes `IItemContainerProvider`.
- [ ] `VerifyScrollPatternOnTableViewPeerIfImplemented` — If a scroll pattern is implemented, it reports correct scroll state; otherwise it is absent.
- [ ] `VerifyTableViewPeerClassNameAndControlType` — The peer reports the expected class name and control type.

### 12.2 Grid coordinates

- [ ] `VerifyGridRowAndColumnCountsMatchVisibleGrid` — `RowCount` and `ColumnCount` match the visible rows and columns.
- [ ] `VerifyGridGetItemReturnsCellForRealizedRow` — `GetItem` returns the cell peer for a realized coordinate.
- [ ] `VerifyGridGetItemBehaviorForOffscreenRow` — `GetItem` for an offscreen row either force-realizes or returns the documented result.
- [ ] `VerifyGridGetItemOutOfRangeBehavesPerContract` — Out-of-range coordinates fail per the UIA contract rather than crashing.
- [ ] `VerifyHiddenColumnsAreSkippedInGridCoordinates` — Collapsed columns do not consume UIA column indices.

### 12.3 Cell and row peers

- [ ] `VerifyGridItemPatternOnCellPeer` — The cell peer exposes `IGridItemProvider` with correct row and column.
- [ ] `VerifyTableItemPatternOnCellPeer` — The cell peer exposes `ITableItemProvider` and reports its header.
- [ ] `VerifyValuePatternOnTextCellPeer` — A text cell peer exposes `IValueProvider` with the cell text.
- [ ] `VerifyCellNameIncludesColumnHeaderAndValue` — The cell's automation name combines the column header and the cell value.
- [ ] `VerifyCellNameDoesNotDuplicateHeaderText` — The header text is not repeated twice in the cell name.
- [ ] `VerifySelectionItemPatternOnRowPeer` — The row peer exposes `ISelectionItemProvider`.
- [ ] `VerifyRowPeerReportsIsSelected` — The row peer's `IsSelected` matches the control's selection.
- [ ] `VerifyCellRuntimeIdIsStableAcrossRealization` — A cell's runtime id stays stable across scroll-out and scroll-back where the contract requires it.

### 12.4 Header and group peers

- [ ] `VerifyInvokePatternOnColumnHeaderPeer` — The column header peer exposes `IInvokeProvider`.
- [ ] `VerifyColumnHeaderPeerNameMatchesHeaderText` — The header peer's automation name matches the visible header text.
- [ ] `VerifyColumnHeaderPeerHelpTextIncludesSortState` — Help text reports the current sort direction when the column is sorted.
- [ ] `VerifyColumnHeaderPeerHelpTextIncludesToolTip` — Help text includes `HeaderToolTip` content when present.
- [ ] `VerifyExpandCollapsePatternOnGroupHeaderPeer` — The group header peer exposes `IExpandCollapseProvider`.
- [ ] `VerifyGroupHeaderPeerReportsExpandState` — The group header peer's expand state matches `IsExpanded`.

### 12.5 Primitive peers

- [ ] `VerifySortIndicatorPeerExcludedFromControlAndContentViews` — The decorative sort indicator is hidden from the UIA control and content views.
- [ ] `VerifyResizeGripperPeerNameUsesOwningHeaderText` — The gripper's automation name derives from the header it resizes.
- [ ] `VerifyResizeGripperPeerControlTypeAndAutomationId` — The gripper reports the expected control type and automation id.

---

## 13. Theming, resources, density, and high contrast

File: `TableView_Theming_APITests.cs`.

### 13.1 Style resolution

Owns all default-style coverage, including the `VerifyDefaultStyleResolves` item dropped from 1.5. TableView's own style is already proven by `VerifyTemplatePartsAfterTemplateApplication`; the value here is the types nothing else touches.

- `(covered)` `VerifyTableViewDefaultStyleResolves` — TableView resolves its default style from `TabularControlsResources`. Already implied by `VerifyTemplatePartsAfterTemplateApplication` in 1.5; do not duplicate.
- [ ] `VerifyTableViewRowDefaultStyleResolves` — TableViewRow resolves its default style.
- [ ] `VerifyGroupHeaderDefaultStyleResolves` — TableViewGroupHeader resolves its default style.
- [ ] `VerifySortIndicatorDefaultStyleResolves` — The internal SortIndicator resolves its default style.
- [ ] `VerifyResizeGripperDefaultStyleResolves` — The internal ResizeGripper resolves its default style.

### 13.2 Theme resources

- [ ] `VerifyLightThemeBrushesResolve` — Every TableView theme brush key resolves under the Light theme.
- [ ] `VerifyDarkThemeBrushesResolve` — Every TableView theme brush key resolves under the Dark theme.
- [ ] `VerifyHighContrastBrushesResolve` — Every TableView theme brush key resolves under High Contrast.
- [ ] `VerifyGridLineBrushResourceAffectsSeparators` — Overriding the gridline brush resource changes the rendered separators.
- [ ] `VerifyResizeGripperBrushResourceApplies` — Overriding the gripper brush resource changes the gripper visual.
- [ ] `VerifySortIndicatorForegroundResourceApplies` — Overriding the sort indicator foreground resource changes the glyph colour.
- [ ] `VerifyThemeChangeAfterLoadUpdatesRowsAndHeaders` — Switching theme on a loaded control repaints rows and headers.
- [ ] `VerifyHighContrastVisualStateBrushKeysAreWired` — High Contrast visual states reference valid, resolvable brush keys.

### 13.3 Density

- [ ] `VerifyDensityRoundtrip` — `Density` reads back each `TableViewDensity` value.
- [ ] `VerifyCompactDensityRowHeight` — Compact density yields the documented minimum row height.
- [ ] `VerifyStandardDensityRowHeight` — Standard density yields the documented minimum row height.
- [ ] `VerifyComfortableDensityRowHeight` — Comfortable density yields the documented minimum row height.
- [ ] `VerifyDensityAffectsHeaderHeight` — Density changes the header row height, not just data rows.
- [ ] `VerifyDensityAffectsCellPadding` — Density changes cell padding as well as height.
- [ ] `VerifyDensityChangeAfterLoadUpdatesLive` — Changing `Density` on a loaded control re-lays out immediately.

---

## 14. Tooltips

File: `TableView_Theming_APITests.cs` or a dedicated tooltip file.

- [ ] `VerifyNullCellToolTipBindingCreatesNoToolTip` — With no `CellToolTipBinding`, cells get no `ToolTip` object at all.
- [ ] `VerifyStringCellToolTipAppears` — A string tooltip binding produces a tooltip with that text.
- [ ] `VerifyCellToolTipMapsToAutomationHelpText` — The cell tooltip text is surfaced as UIA help text.
- [ ] `VerifyEmptyStringCellToolTipSuppressesToolTip` — An empty or null tooltip value suppresses both the tooltip and help text.
- [ ] `VerifyNonStringCellToolTipContentAppears` — A non-string tooltip value renders as tooltip content.
- [ ] `VerifyNonStringCellToolTipDoesNotProduceInvalidHelpText` — A non-string tooltip does not push a `ToString()` artefact into help text.
- [ ] `VerifyAuthoredToolTipInCellTemplateIsPreserved` — A tooltip authored inside a `CellTemplate` is not overwritten by the column tooltip.
- [ ] `VerifyRecycledRowDoesNotShowStaleToolTip` — A recycled row shows the new item's tooltip, never the previous one's.
- [ ] `VerifyStringHeaderToolTipAppears` — A string `HeaderToolTip` produces a header tooltip with that text.
- [ ] `VerifyNonStringHeaderToolTipAppears` — A non-string `HeaderToolTip` renders as header tooltip content.
- [ ] `VerifyNullHeaderToolTipSuppressesToolTip` — A null `HeaderToolTip` produces no header tooltip.
- [ ] `VerifyHeaderHelpTextCombinesToolTipAndSortState` — Header help text merges tooltip text with the current sort state.
- [ ] `VerifyHeaderHelpTextDoesNotDuplicateHeaderText` — Header help text does not restate the header name already exposed as the name.

---

## 15. Virtualization and performance safety

File: `TableView_DataBinding_APITests.cs` or dedicated perf file.

- [ ] `VerifyLargeSourceRealizesBoundedRowCount` — A source of thousands of items realizes only a viewport-sized set of rows.
- [ ] `VerifyScrollRealizesTargetRows` — Scrolling to an offset realizes the rows at that offset.
- [ ] `VerifyScrollRecyclesOffscreenRows` — Rows scrolled out of view are recycled rather than retained.
- [ ] `VerifyRealizedRowCountStaysBoundedAfterLongScroll` — A long scroll does not grow the realized row count without bound.
- [ ] `VerifyRecycledCellsUpdateAllVisualState` — Recycling refreshes bindings, content, tooltips, and background together.
- [ ] `VerifyAutoWidthMeasuresOnlyRealizedRows` — Auto sizing measures realized rows only, not the whole source.
- [ ] `VerifyLargeColumnCountRemainsFunctional` — A large but supported column count still lays out and scrolls; columns are not virtualized.
- [ ] `VerifyManyRowsRenderAndScrollWithoutPathologicalRealization` — A few hundred rows render and scroll without repeated full realization passes.

---

## 16. Error handling, edge cases, and reentrancy

File: `TableView_DataBinding_APITests.cs` or dedicated negative-path file.

### 16.1 Invalid input

- [ ] `VerifyNullColumnInsertionBehavesPerContract` — Inserting a null column is rejected or ignored per the collection's contract.
- [ ] `VerifyInvalidBindingPathDoesNotCrash` — An unresolvable binding path yields a blank cell without throwing.
- [ ] `VerifyNonFiniteResizeDeltaIsIgnored` — A `NaN` or infinite resize delta is ignored rather than corrupting width.
- [ ] `VerifyZeroSizedHostDoesNotCrash` — Measuring in a zero-sized host does not crash or divide by zero.

### 16.2 Mutation during callbacks

- [ ] `VerifyColumnRemovalDuringSelectionChangedIsSafe` — Removing a column from inside `SelectionChanged` leaves coherent state.
- [ ] `VerifySourceMutationDuringSortingEventIsSafe` — Mutating the source from inside `Sorting` leaves coherent state.
- [ ] `VerifySourceMutationDuringSortedEventIsSafe` — Mutating the source from inside `Sorted` leaves coherent state.
- [ ] `VerifySourceMutationDuringSelectionChangedIsSafe` — Mutating the source from inside `SelectionChanged` leaves coherent state.
- [ ] `VerifySourceMutationDuringBeginningEditIsSafe` — Mutating the source from inside `BeginningEdit` leaves coherent state.
- [ ] `VerifySourceMutationDuringCellEditEndingIsSafe` — Mutating the source from inside `CellEditEnding` leaves coherent state.
- [ ] `VerifyReentrantSortCallDuringSortingEventIsCoalesced` — A sort requested from inside `Sorting` is coalesced rather than recursing.

### 16.3 Consumer failures

- [ ] `VerifyConsumerThrowInSelectionChangedMatchesContract` — An exception from a `SelectionChanged` handler propagates or is contained exactly as the contract states.
- [ ] `VerifyConsumerThrowInSortingMatchesContract` — An exception from a `Sorting` handler propagates or is contained exactly as the contract states.
- [ ] `VerifyThrowingSourceDuringIterationLeavesControlIntact` — A source that throws mid-enumeration leaves the control usable afterwards.

### 16.4 Teardown

- [ ] `VerifyUnloadDuringPendingResizeLeavesNoWedgedState` — Unloading mid-drag releases pointer capture and leaves no stuck resize state.
- [ ] `VerifyUnloadDuringPendingEditLeavesNoWedgedState` — Unloading mid-edit closes the edit session and leaves no stuck state.
- [ ] `VerifyDisposalDoesNotAssertInTrackerTeardown` — pins the `0xC0000420` regression once root-caused.

---

## Work sequence

1. Establish test wiring and minimal API test shell. **Done.**
2. Add initialization/defaults/XAML-load tests. **In progress** — blocked on the EmptyTemplate crash.
3. Add columns/header/layout/sizing tests.
4. Add data binding/source update tests.
5. Add selection tests for current single-selection API.
6. Add sorting tests.
7. Add TableViewSource shaping tests.
8. Add grouping tests.
9. Add theming/resource/density tests.
10. Add automation tests.
11. Add editing tests.
12. Add tooltip tests.
13. Add interaction TestUI only for behavior that cannot be reliably verified in API tests.

## Notes and constraints

- Keep tests aligned to current public IDL. Do not copy PR `!15971489` names or APIs that do not exist here.
- Prefer API tests first because they are faster, more deterministic, and match existing MUXC patterns.
- Use interaction tests only for real input/UIA flows: pointer resize, keyboard focus, header invoke, tooltip hover, visual state/theme behavior that requires full app navigation.
- Avoid weak tests that skip on missing UI or pass when provider lookup returns null.
- Avoid broad catch blocks in tests unless verifying explicit exception behavior.
- If a behavior is not yet part of current API contract, mark it deferred rather than writing a test that cannot compile.

## Build and run loop

TableView ships in `Microsoft.UI.Xaml.Controls.Tabular.dll`, which reaches the test app through the local `Microsoft.WindowsAppSDK.WinUI` component package. Rebuilding only the vcxproj is not enough by itself; `MUXControlsTestApp` now copies the locally built DLL over the package copy after build (`OverwriteTabularWithLocalBuild`).

```
msbuild controls\dev\dll-tabular\Microsoft.UI.Xaml.Controls.Tabular.vcxproj
msbuild controls\test\MUXControlsTestApp\MUXControlsTestApp.csproj /restore
powershell .\test\CreateTestPayload.ps1 -Platform x64 -Configuration chk
TestPayload\x64chk\runtests.cmd *TableViewTests*
```

When a test crashes the host, snapshot the matching PDB before rebuilding. `Microsoft.UI.Xaml.Controls.Tabular.pdb` is overwritten on every product build, and analyzing a dump against a mismatched PDB produces plausible but wrong symbol names.

---

# Appendix: tests present in PR `!15971489` (reference only)

Captured from the PR working copy. Many target APIs, controls, and namespaces that do not exist in this repo (`ColumnResizeGripper`, `GroupedSourceAdapter`, `HierarchicalSourceAdapter`, `ShapedCollectionView`, multiple selection, column reorder/autosize). Listed for coverage ideas, not as a port target.

## `controls\dev\ColumnResizeGripper\APITests\ColumnResizeGripperTests.cs` (12)

`DefaultsAreCorrect`, `TryResizeUpdatesValueAndRaisesEvent`, `TryResizeClampsBelowMinimum`, `TryResizeClampsAboveMaximum`, `TryResizeNoOpWhenAtTarget`, `DirectValueAssignmentClampsToMaximum`, `BeginResizeAndEndResizeRoundTrip`, `BeginResizeIsIdempotent`, `EndResizeWithoutBeginIsNoOp`, `ChangingMinimumReclampsCurrentValue`, `SettingMinimumAboveMaximumCoercesMaximum`, `SettingMaximumBelowMinimumCoercesMinimum`

## `controls\dev\ColumnResizeGripper\InteractionTests\ColumnResizeGripperTests.cs` (2)

`BeginResize_TryResize_EndResize_LifecycleAndClamping`, `TryResizeWithoutBeginResizeIsIgnored`

## `controls\dev\GroupedSourceAdapter\APITests\GroupedSourceAdapterTests.cs` (20)

`DefaultsAreCorrect`, `FlattensTwoGroupsWithHeaders`, `IncludeGroupHeadersFalseEmitsItemsOnly`, `EmptyGroupsStillEmitHeaders`, `RefreshEventFiresOnSourceChange`, `GroupReferenceMatchesAcrossEntries`, `OuterAddTriggersRebuild`, `OuterRemoveTriggersRebuild`, `InnerGroupChangeTriggersRebuild`, `DefaultIsExpanded_True_EntriesIncludeAllDataRows`, `CollapseGroup_RemovesDataRowsFromEntries_HeaderStays`, `ExpandGroup_RestoresDataRows`, `IsGroupExpanded_ReflectsExplicitState`, `ToggleGroup_FlipsState`, `CollapseAll_GroupHeadersStay_DataRowsGone`, `ExpandAll_AfterCollapseAll_AllGroupsExpand`, `GroupExpansionChanged_FiresWithGroupArg_OnSingleToggle`, `GroupExpansionChanged_FiresWithNullArg_OnExpandAll`, `DefaultIsExpanded_False_GroupsStartCollapsed_ExplicitExpandOverrides`, `Refresh_Manual_PreservesExplicitIntents`

## `controls\dev\HierarchicalSourceAdapter\APITests\HierarchicalSourceAdapterTests.cs` (14)

`DefaultsAreCorrect`, `CollapsedRootShowsOnlyTopLevelNodes`, `DefaultIsExpandedExpandsEntireTree`, `ExpandAndCollapseToggleVisibility`, `ToggleFlipsState`, `ExplicitCollapseOverridesDefaultExpanded`, `RefreshedEventFiresOnSourceChange`, `LeavesAndEmptyParentsAreFlatLeaves`, `B3_RootObservableMutationUpdatesEntries`, `B3_ExpandedChildrenObservableMutationUpdatesEntries`, `ChildrenPropertyName_ReadsViaCustomPropertyProvider`, `ChildrenPropertyName_EmptyFallsBackToIterableSelf`, `ChildrenPropertyName_ChangeRebuildsTree`, `SortDescriptions_OrderSiblingsAtEveryLevel`

## `controls\dev\ShapedCollectionView\APITests\ShapedCollectionViewTests.cs` (20)

`DefaultsAreSane`, `SetSourceMaterializesView`, `SortAscendingByName`, `SortDescendingByAge`, `MultiKeySortAppliesPriorityOrder`, `GroupByDepartmentClustersItems`, `RefreshRebuildsFromCurrentSource`, `ObservableSourceTriggersAutoRefresh`, `RefreshedEventFiresOnRebuild`, `SortDescriptionMutationRebuildsView`, `NoneSortDirectionIsIgnored`, `LiveSorting_ResortsOnObservedPropertyChange`, `LiveSorting_OffMeansNoReshapeOnPropertyChange`, `LiveShapingCoalesce`, `LiveGrouping_RegroupsOnObservedPropertyChange`, `TogglingLiveSortingResubscribesExistingItems`, `B1_GroupKeysOfDifferentTypesDoNotCollapse`, `InpcOnlySource_TriggersAutoRefresh`, `IncrementalAddRemoveProducesSurgicalVectorChanged`, `IncrementalAddHonoursSortOrder`

## `controls\dev\ShapedCollectionView\APITests\ShapedCollectionView_Composition_APITests.cs` (11)

`Composition_SCV_SortOnly`, `Composition_SCV_SortDescending_Replace`, `Composition_SCV_LiveSort_AddRemove`, `Composition_SCV_GSA_Grouped`, `Composition_SCV_GSA_LiveSort_GroupRebuild`, `Composition_SCV_GSA_SourceMutation`, `Composition_SCV_HSA_Hierarchical`, `Composition_SCV_HSA_ExpandCollapse`, `Composition_SCV_HSA_SourceMutation`, `Composition_SCV_GSA_HSA_FullStack`, `Composition_SCV_GSA_HSA_RapidMutation`

## `controls\dev\SortIndicator\APITests\SortIndicatorTests.cs` (8)

`VerifyDefaultDirectionIsNone`, `VerifyDirectionPropertyRoundTrips`, `VerifyDirectionDependencyPropertyExists`, `VerifyAutomationPeerType`, `VerifyAutomationPeerExcludedFromControlAndContentViews`, `VerifyClassName`, `VerifyDefaultStyleResolves`, `VerifyChevronVisibleAfterDirectionChange`

## `controls\dev\SortIndicator\InteractionTests\SortIndicator_InteractionTests.cs` (8)

`SortIndicator_VSM_Ascending`, `SortIndicator_VSM_Descending`, `SortIndicator_VSM_None`, `SortIndicator_VSM_TransitionAscDesc`, `SortIndicator_ThemeSwitchMidState`, `SortIndicator_KeyboardActivation`, `SortIndicator_AutomationPeer_Toggle`, `SortIndicator_AutomationPeer_Metadata`

## `controls\dev\TableView\APITests\TableViewTests.cs` (72)

Defaults and header visibility: `TableView_DefaultsAreCorrect`, `ColumnHeaderVisibility_DefaultIsColumn_ColumnHeaderVisible`, `ColumnHeaderVisibility_None_BothHeadersCollapsed`, `ColumnHeaderVisibility_Column_OnlyColumnHeaderVisible`, `ColumnHeaderVisibility_ChangeAfterLoad_UpdatesLive`

Source and columns: `TableView_ItemsSourceRoundtrip`, `TableView_ColumnsIsObservable`, `TableViewColumn_HeaderRoundtrip`, `TableViewColumn_WidthDefaultsTo120`, `TableViewColumn_ResizeDefaults`, `TableViewColumn_ActualWidthFollowsWidth`, `TableViewColumn_ActualWidthClampsToMinWidth`, `TableViewColumn_ActualWidthClampsToMaxWidth`, `TableViewColumn_ActualWidthFollowsMinWidthChange`, `TableViewColumn_ActualWidthFollowsMaxWidthChange`, `TableViewColumn_PathologicalMinGreaterThanMaxFavoursMin`, `TableViewColumn_CanResizeRoundtrip`, `TableViewTextColumn_BindingRoundtrip`, `TableViewTextColumn_GenerateElementProducesTextBlock`, `TableViewRow_GetOwningTableViewDefaultsNull`, `TableViewColumn_GetOwningTableViewTracksColumnsMembership`, `TableView_XamlLoadDoesNotInfiniteRecurse`

Selection: `TableView_SelectionDefaultsAreCorrect`, `TableView_SelectionModeRoundtrip`, `TableView_SelectByIndexUpdatesDPs`, `TableView_DeselectByIndexClearsDPs`, `TableView_SelectedIndexDPDispatchesIntoModel`, `TableView_SelectAllInMultipleSelectsEverything`, `TableView_SelectAllInSingleModeIsClampedToOne`, `TableView_SelectingSecondIndexInSingleClearsFirst`, `TableView_SelectRangeInMultiple`, `TableView_NoneClearsSelection`, `TableView_MultipleToSingleClampsToOne`, `TableView_NoneIgnoresExplicitSelect`, `TableView_ItemsSourceSwapClearsSelection`, `TableView_SelectOutOfRangeIsTolerated`, `TableView_InvalidSelectionDPCoercesToModelConsistency`, `TableView_SelectionChangedFiresOnSelect`, `TableView_SelectionChangedDoesNotFireForSameSelection`, `TableViewRow_IsSelectedDefaultsToFalse`

Sorting: `TableViewColumn_SortDefaults`, `TableViewColumn_CanSortRoundtrip`, `TableViewColumn_SortMemberPathRoundtrip`, `TableViewTextColumn_SortMemberPathFallsBackToBindingPath`, `TableView_SortDefaults`, `TableView_SortByColumnSetsDirectionAndIndex`, `TableView_SortByColumnReplacesExistingSort`, `TableView_ToggleSortDirectionCyclesNoneAscDescNone`, `TableView_ToggleSortDirectionReplaceClearsOtherColumns`, `TableView_ToggleSortDirectionRespectsCanSort`, `TableView_ClearSortRemovesEverything`, `TableView_SortingEventFiresWithTriggerColumnAndSnapshot`, `TableView_ClearSortFiresSortingEventWithNullTrigger`

Sticky header: `TableView_StickyHeader_TemplatePartsAreNamed`, `TableView_StickyHeader_HeaderTracksBodyHorizontalOffset`, `TableView_StickyHeader_HeaderDoesNotMoveOnVerticalScroll`

Column reorder and autosize: `P212_CanUserReorderColumnsDefaultsToTrue`, `P212_CanUserReorderColumnsRoundtripsThroughDP`, `P212_MoveColumn_ReordersColumnsVector`, `P212_MoveColumn_SameIndexReturnsTrueIdempotentNoOp`, `P212_MoveColumn_OutOfRangeFromIndexReturnsFalse`, `P212_MoveColumn_OutOfRangeToIndexReturnsFalse`, `P212_MoveColumn_GlobalGateBlocks`, `P212_MoveColumn_PerColumnGateBlocks`, `P212_MoveColumn_PreservesColumnState`, `P212_MoveColumn_RebuildsHeaderHostInNewOrder`, `P212_MoveColumn_RaisesColumnReorderedOnSuccess`, `P212_MoveColumn_DoesNotRaiseColumnReorderedOnNoOp`, `P212_AutoSizeColumn_NoOpWhenNoRowsRealized`, `P212_AutoSizeColumn_FitsRealizedCellContent`, `P212_AutoSizeColumn_RespectsMaxWidthClamp`, `P212_AutoSizeAllColumns_HitsEveryColumn`

## `controls\dev\TableView\APITests\TableView_AutomationPeer_APITests.cs` (13)

`VerifyGridPatternOnTableViewPeer`, `VerifyTablePatternOnTableViewPeer`, `VerifySelectionPatternOnTableViewPeer`, `VerifyScrollPatternOnTableViewPeer`, `VerifyItemContainerPatternOnTableViewPeer`, `VerifyGridItemPatternOnRowPeer`, `VerifySelectionItemPatternOnRowPeer`, `VerifyScrollItemPatternOnRowPeer`, `VerifyValuePatternOnTextCellPeer`, `VerifyInvokePatternOnHeaderPeer`, `VerifyGetNameOnColumnHeaderPeer`, `VerifyGridProviderGetItemForceRealizesOffscreenRow`, `VerifyTableItemPatternOnCellPeer`

## `controls\dev\TableView\APITests\TableView_Gap_APITests.cs` (17)

`LiveIncrementalUpdates_SingleItemObservableChangesUpdateRows`, `LiveIncrementalUpdates_MultiItemObservableChangesRebuildRows`, `SelectionChangedReportsAddedRemovedAndSourceChangesRebaseSelection`, `SortingEventCancelLeavesOrderAndSortStateUnchanged`, `CellEditEndingCancelActionRestoresOriginalValue`, `RowEditEndingCancelActionRestoresOriginalValue`, `ValidationFailureBlocksCommitUntilCanceled`, `ForcedEditTerminationOnItemsSourceResetIgnoresEndingCancelAndDoesNotCrash`, `ForcedEditTerminationOnUnloadDiscardsInvalidEditAndDoesNotCrash`, `Automation_GroupRowsExposeExpandCollapsePattern`, `Automation_HiddenColumnsAreSkippedInGridCoordinates`, `Automation_CellRuntimeIdIsStableAcrossRealization`, `HighContrastVisualStateBrushKeysAreWired`, `Virtualization_LargeSourceKeepsRealizedRowsBoundedAfterScroll`, `FilteringPredicateReflectsRowsAndFilterChangeReprojects`, `MutableKeyReplaceUpdatesSortedProjection`, `ReentrantReplaceDuringFilterRebuildIsCoalesced`

## `controls\dev\TableView\APITests\TableView_ConsumerThrow_APITests.cs` (2)

`SelectionChanged_ConsumerThrows_ControlIntact`, `ColumnReordered_ConsumerThrows_ControlIntact`

## `controls\dev\TableView\APITests\TableView_NegativePath_APITests.cs` (2)

`VerifyConsumerEventThrow_K2_RegressionPin`, `VerifySourceThrowsDuringIteration_ControlStateIntact`

## `controls\dev\TableView\APITests\TableView_FaultInjection_APITests.cs` (5)

`FaultInjection_OutOfMemory_ItemsSourcePath`, `FaultInjection_ROE_CLOSED_DispatcherResume`, `FaultInjection_STOWED_EXCEPTION_Continuation`, `FaultInjection_OutOfMemory_ColumnOperation`, `FaultInjection_ROE_CLOSED_SelectionChanged`

## `controls\dev\TableView\InteractionTests\TableViewTests.cs` (19)

`TestPageLoadsAndRendersStaticTable`, `AddingColumnAtRuntimeRebuildsHeaderAndCells`, `SelectFirstButtonUpdatesSelectionReadout`, `SelectAllInMultipleSelectsEveryItem`, `DeselectAllClearsSelection`, `SwitchingToNoneClearsSelection`, `SetFirstColumnWidth_ActualWidthMatches`, `SetFirstColumnWidthBelowMin_ActualWidthClampsToMin`, `SetFirstColumnMaxWidthBelowWidth_ActualWidthClampsToMax`, `ResetColumnWidth_ReadoutReflectsDefaults`, `SortByNameAsc_ColumnReadoutShowsAscendingAndIndexOne`, `SortByAgeDesc_AfterNameAsc_NameClearsAgeBecomesIndexOne`, `ToggleSortName_CyclesNoneAscendingDescendingNone`, `ClearSort_AfterSortingByName_ResetsEverything`, `DownArrowMovesSelectionToNextRow`, `UpArrowMovesSelectionToPreviousRow`, `HomeKeySelectsFirstRow`, `EndKeySelectsLastRow`, `TemplateColumnRendersCustomContent`

## PR coverage not represented in our plan

These exist only because the PR has controls/APIs this repo lacks. Revisit if those APIs land:

- `ColumnResizeGripper` as a standalone public primitive with `BeginResize`/`TryResize`/`EndResize` and value clamping. Our repo has an internal `ResizeGripper` only.
- `GroupedSourceAdapter` and `HierarchicalSourceAdapter` as separate testable adapters. Our grouping goes through `TableViewSource`.
- `ShapedCollectionView` with `SortDescriptions`, live sorting/grouping toggles, and incremental `VectorChanged`.
- Multiple/Extended selection, `SelectAll`, `SelectRange`.
- Column reorder (`MoveColumn`, `CanUserReorderColumns`, `ColumnReordered`) and autosize (`AutoSizeColumn`, `AutoSizeAllColumns`).
- Row-scoped editing (`RowEditEnding`).
- Fault injection harness (`OutOfMemory`, `RO_E_CLOSED`, stowed exception paths).
