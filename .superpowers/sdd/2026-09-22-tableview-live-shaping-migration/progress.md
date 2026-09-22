# SDD ledger — plan: docs/superpowers/plans/2026-09-22-tableview-live-shaping-migration.md

Baseline: cde6ce11cc0dc334e30cbeb9e3b6c263c683d885

Pre-flight: Task 1 produces LiveShapingTracker interfaces consumed by Task 2; Task 2 produces engine live-shaping interfaces consumed by Task 3; Task 3 produces the app-facing API consumed by Task 4. Current source confirms these dependency directions.

Ruling: No automated tests will be added or run — the user explicitly requires app-only validation, overriding the generic TDD validation path; risk is reduced automated regression coverage.

Task 1: LiveShapingTracker added and registered in ShapedItemsSource.vcxitems.
Task 2: ShapedItemsSource now tracks source item property changes, captures live sort/group/filter snapshots, and refreshes through the existing shaping pipeline.
Task 3: TableViewSource exposes opt-in IsLiveSorting, IsLiveFiltering, and IsLiveGrouping properties; generated XAML metadata maps all three properties to TableViewSource.
Validation: product build succeeded; packaged C# TableView app build succeeded with 0 errors; aggregate Build.cmd samples target could not run because the repository's `buildsamples.cmd` command was unavailable. No automated tests run per ruling.

Follow-up (user-reported): incremental collection changes re-enumerated and re-subscribed the entire source on every notification (ResubscribeLiveShapingFromSource at the top of ApplyIncrementalChange / ApplyIncrementalVectorChange), making each single-item change O(N x selectors) and bulk load O(N^2), defeating the incremental fast paths and double-charging every Refresh fallback. Fixed by:
- Deriving the subscription delta from NotifyCollectionChangedEventArgs for the INCC path (correct for every branch, including the sorted fast path).
- Applying subscribe/unsubscribe at the VectorChanged splice sites, where the affected object is in hand (VectorChanged carries no items).
- Converting RefreshLiveShapingSubscriptions to a mark-and-sweep reconcile, so a refresh revokes only departures and subscribes only arrivals.
- Re-capturing snapshots on the TryApplyShapingDeltaInPlace path, which skips Refresh and so previously left cached keys describing the superseded spec.
- Holding items weakly in LiveShapingTracker (the auto-revoker was already weak, so the subscription entry was the only strong reference), with recycled-ABI-address detection on Subscribe.
Re-validated: product build succeeded; packaged C# TableView app succeeded with 0 errors.

Task 4 (validation host): Samples\TableViewSampleApp\ShapingPage now exposes live shaping so the feature can be exercised at runtime.
- Data.Item.Score converted from init-only to a settable, PropertyChanged-raising property (int overload of Set added), so the numeric filter threshold and Score-band grouping are reachable by mutation. Data.Roles / Data.Cities widened to internal so the page cycles within the authored value sets.
- ShapingPage.xaml gained a live-shaping row (row 5; table and status shifted to rows 6 and 7): Sorting / Filtering / Grouping checkboxes plus an "Editable cells" toggle on the left, and Role / City / Name / Score mutators for the selected row on the right.
- ShapingPage.xaml.cs wires the toggles to TableViewSource.IsLiveSorting / IsLiveFiltering / IsLiveGrouping behind the existing _ready guard, seeds them from the checkboxes in the constructor (so control and UI cannot start out disagreeing), flips Table.IsReadOnly for in-cell editing, and adds mutation handlers that raise a property change on exactly one existing row - as distinct from "Add item", which is a collection change and reshapes regardless of the flags. Status line now reports the three live flags and the edit mode.
Validation: TableViewSampleApp build succeeded, 0 errors (14 pre-existing warnings). Runtime behavioural validation still outstanding - nothing has been observed running yet. No commits made.
