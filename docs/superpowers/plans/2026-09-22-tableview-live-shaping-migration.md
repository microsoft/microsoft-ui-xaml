# TableView Live Shaping Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Port live sorting, filtering, and grouping updates from PR 16507391 onto the current TableView shaping infrastructure.

**Architecture:** Add a layer-2 item-notification tracker and keep live-shaping state in `ShapedItemsSource`. The engine re-evaluates current pipeline selectors and uses the existing full `Refresh()` path when an enabled shaping dimension changes, preserving current sort-axis and group-adapter contracts. `TableViewSource` exposes the three opt-in Boolean properties and remains the public source of the feature.

**Tech Stack:** C++/WinRT, WinUI TableView, MIDL IDL, MSBuild shared-item projects, TableView sample apps.

**Spec:** `docs/superpowers/specs/2026-09-22-tableview-live-shaping-migration-design.md`

## Global Constraints

- All live-shaping properties default to `false`.
- Track the complete source set, including items excluded by filtering.
- Use the current `ShapingPipeline` and `GroupedSourceAdapter`; do not restore retired sorting/grouping structures.
- Preserve fluent and column-owned sort-axis token, replacement, precedence, and path behavior.
- Respect the current UI-thread-affine source/projection contract; do not add background dispatch or broad exception swallowing.
- Validation is app-only through TableView sample apps.
- Do not commit implementation or documentation changes.

## Review Focus

- Filtered-out item becomes eligible after a property edit: subscription must remain active.
- Group-key edit in a grouped projection: refresh must flow through `GroupedSourceAdapter`, not mutate stale flat rows.
- Fluent and column-owned sort axes: live sort must observe the active pipeline axes without changing ownership or precedence.
- Unrelated property edit: no projection refresh when enabled selectors produce unchanged effective values.
- Disable/re-enable lifecycle: disabling all modes must revoke subscriptions and re-enable must capture current values.

---

### Task 1: Add the item notification tracker

**Files:**
- Create: `controls/dev/ShapedItemsSource/LiveShapingTracker.h`
- Create: `controls/dev/ShapedItemsSource/LiveShapingTracker.cpp`
- Modify: `controls/dev/ShapedItemsSource/ShapedItemsSource.vcxitems`
- Modify: `controls/Tabular.ProjectImports.targets` only if the current import comments or item ownership require updating

**Interfaces:**
- Consumes: `winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged`, `winrt::PropertyChangedEventHandler`, and an engine callback.
- Produces: `LiveShapingTracker::Configure`, `Subscribe`, `Unsubscribe`, `UnsubscribeAll`, and `IsSubscribed` methods used by `ShapedItemsSource`.

- [ ] **Step 1: Define the tracker contract**

  Define a tracker that owns only event revokers and weakly references the engine callback. Its change handler accepts the notifying item and property name. Keep the tracker UI-thread-affine; do not add a dispatcher or background queue.

- [ ] **Step 2: Implement subscription and revocation**

  `Subscribe(item)` should `try_as<INotifyPropertyChanged>()`, subscribe with `auto_revoke`, and retain one revoker per item identity. `Unsubscribe(item)` removes that item’s revoker. `UnsubscribeAll()` clears every revoker. Non-observable items are valid no-op inputs.

- [ ] **Step 3: Forward property changes**

  Forward the sender item and `PropertyChangedEventArgs.PropertyName()` to the configured callback. Treat an empty property name as a valid “all properties changed” notification. Do not catch and suppress callback failures.

- [ ] **Step 4: Register the new files**

  Add the tracker header/source to `ShapedItemsSource.vcxitems`, preserving layer-2 ownership and include-directory conventions.

- [ ] **Step 5: Inspect the diff**

  Run `git diff --check` and confirm only tracker files and the required shared-item registration changed.

### Task 2: Integrate subscriptions and live reevaluation into ShapedItemsSource

**Files:**
- Modify: `controls/dev/ShapedItemsSource/ShapedItemsSource.h`
- Modify: `controls/dev/ShapedItemsSource/ShapedItemsSource.cpp`

**Interfaces:**
- Consumes: `LiveShapingTracker`, `ShapingPipeline::ActiveSortAxes`, `ShapingPipeline::PassesFilter`, `m_groupSelector`, and `Refresh()`.
- Produces: `SetLiveShaping(bool liveSorting, bool liveGrouping, bool liveFiltering)`, `IsLiveSorting()`, `IsLiveGrouping()`, `IsLiveFiltering()`, and `IsLiveShapingEnabled()` for `TableViewSource`.

- [ ] **Step 1: Add engine state and snapshot types**

  Add the three Boolean flags, one tracker, and a COM-identity-keyed snapshot map. Snapshot only enabled dimensions: filter result, each active sort key, and the active group key. Use existing key stringification/equality helpers so snapshots follow current shaping semantics.

- [ ] **Step 2: Configure tracker callbacks**

  Configure the tracker in the constructor with an enabled predicate and an engine callback. The callback must call a new `OnLiveShapedItemChanged(item, propertyName)` method on the owning UI thread.

- [ ] **Step 3: Subscribe the complete source after refresh**

  In the authoritative materialization portion of `Refresh()`, subscribe to the pre-filter source rows before filtering. Rebuild snapshots at the same time. When no live mode is enabled, clear subscriptions and snapshots.

- [ ] **Step 4: Maintain subscriptions for collection changes**

  Update add/remove/replace paths using the event’s old/new item lists. For reset or vector notifications without item lists, materialize the source and rebuild the subscription set. Removed items must be unsubscribed before their snapshot entries are erased.

- [ ] **Step 5: Implement live-shaping mode changes**

  Implement `SetLiveShaping` as an idempotent flag update. Enabling any mode resubscribes from the complete source and captures current snapshots. Disabling all modes clears subscriptions and snapshots without calling `Refresh()` or changing projection contents.

- [ ] **Step 6: Re-evaluate changed items**

  On notification, recapture only enabled dimensions. If snapshots are unchanged, return without notification. If any enabled dimension differs, update the snapshot and call `Refresh()`. If the item is not in the current source snapshot, resubscribe/materialize before evaluating rather than silently accepting a stale item.

- [ ] **Step 7: Preserve current shaping behavior**

  Do not modify `SetSort`, `ClearSort*`, `SetGroup`, `ClearGroup`, `ApplyShapingChange`, or `TryApplyShapingDeltaInPlace` semantics. Live updates must enter through `Refresh()` so grouped projections continue through `GroupedSourceAdapter` and owner projection notifications remain intact.

- [ ] **Step 8: Review lifetime and error paths**

  Ensure destructor order revokes source collection handlers and item handlers. Keep selector/filter exception behavior aligned with existing pipeline methods; do not add broad catches around refresh or callback execution.

- [ ] **Step 9: Inspect the diff**

  Run `git diff --check` and review that no old `CoalescedUIWork`, old grouping state, background dispatch, or obsolete live-shaping API was introduced.

### Task 3: Expose live-shaping properties through TableViewSource

**Files:**
- Modify: `controls/dev/TableView/TableViewSource.idl`
- Modify: `controls/dev/TableView/TableViewSource.h`
- Modify: `controls/dev/TableView/TableViewSource.cpp`
- Modify: `controls/dev/dll-tabular/XamlMetadataProviderGenerated.h`

**Interfaces:**
- Consumes: `ShapedItemsSource::SetLiveShaping` and its three getters.
- Produces: WinRT `Boolean IsLiveSorting`, `IsLiveFiltering`, and `IsLiveGrouping` properties usable from code and XAML.

- [ ] **Step 1: Add the IDL properties**

  Add the three properties near the existing shaping verbs, with comments stating that they are opt-in, subscribe to source item notifications, and affect only their corresponding shaping dimension.

- [ ] **Step 2: Add native declarations**

  Add getter/setter declarations to `TableViewSource.h`, following generated WinRT property naming conventions already used by the project.

- [ ] **Step 3: Forward getters and setters**

  Implement getters from engine state. Each setter reads the other two engine flags and calls one `SetLiveShaping` operation, avoiding partial state updates.

- [ ] **Step 4: Register XAML metadata**

  Add Boolean `XamlType::AddMember` entries for all three properties in the generated TableViewSource metadata block, matching existing generated property callbacks.

- [ ] **Step 5: Check generated-surface consistency**

  Confirm property names, ABI types, getter/setter signatures, and metadata callbacks match the IDL-generated `TableViewSource` interfaces. Do not add dependency properties; these are ordinary source properties.

### Task 4: App-only validation

**Files:**
- No new automated test files.
- Use the existing TableView sample app and its current data/shaping UI.

**Interfaces:**
- Consumes: the completed `TableViewSource` live-shaping API and current TableView grouping/sorting/filtering UI.
- Produces: manual validation evidence only; no test code and no commits.

- [ ] **Step 1: Build the app configuration already used for TableView samples**

  Use the repository’s existing app build workflow. Do not add tools, tests, dependencies, or generated planning artifacts.

- [ ] **Step 2: Validate disabled behavior**

  With filtering, sorting, and grouping configured, edit an item property while all live flags are disabled. Confirm the projection remains stale until an existing collection or explicit refresh action changes it.

- [ ] **Step 3: Validate live filtering**

  Enable only live filtering. Edit an excluded item into the filter range and an included item out of range. Confirm admission and eviction without collection edits.

- [ ] **Step 4: Validate live sorting**

  Enable only live sorting. Edit the active sort property on visible rows and confirm row order changes. Exercise both a column-owned path sort and a fluent/path sort where the app exposes both.

- [ ] **Step 5: Validate live grouping**

  Enable only live grouping. Edit an item’s group key and confirm it moves to the correct group while grouped row headers and expansion state remain coherent.

- [ ] **Step 6: Validate lifecycle and source membership**

  Toggle flags off and on, add/remove/replace items, reset the source, and edit filtered-out items. Confirm removed items no longer affect the view and filtered-out items can re-enter.

- [ ] **Step 7: Record failures without committing**

  Capture app behavior and any build/runtime failures in the session response or existing work-item/PR discussion as requested. Leave all implementation and spec changes uncommitted.
