# TableView Live Shaping Migration Design

## Goal

Port the live-shaping behavior from PR 16507391 onto the current TableView
shaping stack, whose sorting, grouping, projection, identity, and notification
contracts have evolved since that PR was authored.

The resulting implementation must preserve the opt-in public API:

- `TableViewSource.IsLiveSorting`
- `TableViewSource.IsLiveFiltering`
- `TableViewSource.IsLiveGrouping`

All properties remain disabled by default.

## Current constraints

The current stack has these authoritative behaviors:

- `ShapingPipeline` owns filter axes, sort axes, sort precedence, group-verb
  state, and committed `ShapingSpec` deltas.
- `ShapedItemsSource` owns the live projection and emits projection/shaping
  notifications.
- `GroupedSourceAdapter` owns the presented grouped row vector and group
  expansion behavior.
- Sort axes may be declared by the fluent `TableViewSource` API or by the
  TableView column front-end. Axis tokens, delegate identity, replacement,
  precedence, and path metadata must remain unchanged.
- Grouping is one effective group verb. Grouped rows are rebuilt through
  `GroupedSourceAdapter`.
- Projection mutation and source notifications are UI-thread-affine. The live
  shaping port must not reintroduce the earlier implementation's background
  dispatch or exception-swallowing behavior.

## Design

### LiveShapingTracker

Add a layer-2 `LiveShapingTracker` responsible only for item-side
`INotifyPropertyChanged` subscriptions:

- Subscribe to source items when any live-shaping mode is enabled.
- Track the complete source set, including items currently excluded by a
  filter, so an item can become eligible later.
- Update subscriptions for collection add, remove, replace, and reset.
- Remove all subscriptions when live shaping is disabled or the projection is
  destroyed.
- Invoke the engine's change callback on the owning UI thread under the
  existing source/projection thread contract.

The tracker must not know how filtering, sorting, or grouping is applied.

### ShapedItemsSource integration

Keep the three live-shaping flags in `ShapedItemsSource`, with one setter that
updates the complete flag set. Turning a mode on or off changes subscriptions
and snapshots only; it does not itself alter the projection.

On a watched item notification:

1. Re-evaluate the active shape dimensions that are enabled.
2. Ignore notifications whose effective filter result, active sort keys, and
   group key are unchanged.
3. Otherwise call the existing `Refresh()` path.

The refresh path remains the single correctness path. It must use the current
`ShapingPipeline` and `GroupedSourceAdapter`, preserving current filter
conjunction, sort-axis precedence/replacement, group identity, projection kind,
row metadata, and owner notifications.

Because selectors are opaque delegates, snapshots may use the effective values
currently available from the pipeline, but the implementation must fall back to
refresh rather than guess when equality or selector evaluation cannot be
established safely. No old grouping or sorting data structures should be
reintroduced.

Live subscriptions are refreshed after a full materialization and maintained
after incremental collection changes. A filtered-out item remains subscribed;
an item removed from the source does not.

### TableViewSource API

Add the three Boolean properties to `TableViewSource.idl`, implement forwarding
getters/setters in `TableViewSource`, and register them in
`XamlMetadataProviderGenerated.h`.

Setters preserve the other two flag values and forward the complete state to
`ShapedItemsSource`. Getters read from the engine so there is one source of
truth.

No changes are made to existing sort or group public methods.

## Error and lifetime behavior

- Subscription revoke and tracker teardown follow existing WinRT revoker and
  ownership patterns.
- Item notifications from objects that do not implement
  `INotifyPropertyChanged` are ignored because there is nothing to subscribe
  to.
- Selector/filter evaluation follows the existing shaping behavior. Do not add
  broad catches or silent success fallbacks; use existing predicate/key
  semantics and surface unexpected errors consistently.
- Weak callbacks prevent the tracker from keeping `ShapedItemsSource` alive.
- Disabling all live modes must release item subscriptions and snapshot state.

## Validation

Extend or port existing TableView shaping tests to cover:

1. Live filtering admits and evicts rows after item property changes.
2. Live sorting reorders rows after an active sort-key change.
3. Live grouping moves rows after a group-key change.
4. Each live mode is independently opt-in; disabled dimensions remain stale.
5. Filtered-out items are still observed and can re-enter.
6. Add/remove/replace/reset collection changes maintain the subscription set.
7. Fluent and column-owned sort axes continue to honor current token and
   precedence rules.
8. Disabling live shaping removes subscriptions without changing the current
   projection.

Build the Tabular project and run the targeted TableView shaping tests.
