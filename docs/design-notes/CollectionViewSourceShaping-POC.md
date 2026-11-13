# CollectionViewSource shaping (sort, filter, group): proof of concept

Status: **POC, experimental (`Feature_ExperimentalApi`).** Built (amd64chk) and run on a test VM. Branches
`user/hik/tableview/cvs-shaping-poc` (framework) and `user/hik/tableview/cvs-shaping-poc-e2e` (framework plus
TableView consuming it end to end).

This answers the 2026-11-11 TableView API review feedback: *"CollectionViewSource should be beefed up (to do
filter, sort, group) bringing it closer to WPF"*. It is also what GitHub issue
[#4307](https://github.com/microsoft/microsoft-ui-xaml/issues/4307) ("WPF Gap: CollectionViewSource doesn't support
all WPF scenarios") asks for.

## Goals

- Add WPF-style `SortDescriptions`, `GroupDescriptions` and `Filter` to the framework `CollectionViewSource` (CVS).
- Make it strictly opt-in. With no descriptor and no `Filter`, `View` is created by the same code as before.
- Produce standard `ICollectionView` / `ICollectionViewGroup` output, so ListView, GridView and SemanticZoom work
  unchanged, and TableView can consume the same contract.

## API sketch

All new API is in `Microsoft.UI.Xaml.Data`, in `Microsoft.UI.Xaml.dll`, and is `[feature(Feature_ExperimentalApi)]`.

```idl
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
enum ListSortDirection { Ascending, Descending };

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
delegate Boolean CollectionViewFilter(Object item);

// Negative, zero or positive as x sorts before, with or after y (WPF ListCollectionView.CustomSort).
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
delegate Int32 CollectionViewComparer(Object x, Object y);

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
runtimeclass SortDescription
{
    SortDescription(String propertyName, ListSortDirection direction);
    String PropertyName { get; };
    ListSortDirection Direction { get; };
};

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
runtimeclass PropertyGroupDescription
{
    PropertyGroupDescription(String propertyName);
    PropertyGroupDescription(String propertyName, IValueConverter converter);
    String PropertyName { get; };
    IValueConverter Converter { get; };
};

runtimeclass CollectionViewSource : DependencyObject
{
    // ...existing members unchanged...

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
    [interface_name("Microsoft.UI.Xaml.Data.ICollectionViewSourceFeature_ExperimentalApi")]
    {
        IObservableVector<SortDescription> SortDescriptions { get; };
        IObservableVector<PropertyGroupDescription> GroupDescriptions { get; };
        CollectionViewFilter Filter;
        CollectionViewComparer CustomSort;
        void Refresh();
        Windows.Foundation.IClosable DeferRefresh();
    }
};

// Optional capability of an ICollectionView, like ICollectionViewFactory or ISupportIncrementalLoading.
// A View that a CollectionViewSource shapes implements it and forwards every member to that CVS.
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
interface ICollectionViewShaping
{
    IObservableVector<SortDescription> SortDescriptions { get; };
    IObservableVector<PropertyGroupDescription> GroupDescriptions { get; };
    CollectionViewFilter Filter;
    void Refresh();
    Windows.Foundation.IClosable DeferRefresh();
    CollectionViewComparer CustomSort;
};
```

| WPF | This POC | Note |
|---|---|---|
| `CollectionViewSource.SortDescriptions` (`SortDescriptionCollection` of `SortDescription` structs) | `IObservableVector<SortDescription>`, immutable runtime class | WinRT has no struct-with-string collections that XAML can fill; an immutable class keeps WPF's value semantics. |
| `ListSortDirection` | `ListSortDirection` | Same names and values. |
| `CollectionViewSource.GroupDescriptions` (`GroupDescription` base) | `IObservableVector<PropertyGroupDescription>` | Only property grouping. See risks for the base-class decision. |
| `CollectionViewSource.Filter` **event** + `ICollectionView.Filter` (`Predicate<object>`) | `Filter` property of type `CollectionViewFilter` | See decision 3. |
| `ICollectionView.Refresh()` | `CollectionViewSource.Refresh()` and `ICollectionViewShaping.Refresh()` | `ICollectionView` is frozen, so the verb lives on CVS and on the opt-in view interface. |
| `ICollectionView.SortDescriptions` / `GroupDescriptions` / `Filter` | `ICollectionViewShaping` on the shaped `View` | Same objects as the CVS's: one set of descriptors, reachable from `ItemsSource` alone. |
| `CollectionViewGroup.Name` | `Key` and `Name` on the group object | `Key` matches the `IGrouping<K,T>` idiom WinUI samples already use. |
| `ICollectionView.DeferRefresh()` → `IDisposable` | `DeferRefresh()` → `IClosable` on CVS and `ICollectionViewShaping` | Implemented. Nested deferrals count; changes made while one is open (descriptors, `Filter`, `CustomSort`, `Refresh`, source changes) recompute once, when the last closes. C# `using` and C++/CX `delete` close it. An unclosed deferral defers forever, as in WPF. |
| `ListCollectionView.CustomSort` (`IComparer`) | `CustomSort` property of type `CollectionViewComparer` on CVS and `ICollectionViewShaping` | Implemented. A delegate, not an interface, so a C++/WinRT lambda or a C# method group works and nothing needs reflection (AOT-safe). As in WPF, setting it clears `SortDescriptions`, and adding a `SortDescription` clears it. Ties keep source order. A comparer that fails fails the change and leaves the view's rows. |
| `PropertyGroupDescription.Converter` (`IValueConverter`) | `PropertyGroupDescription(propertyName, converter)` and read-only `Converter` | Implemented. `Convert(value, Object, null, null)` maps the property value to the group key, so a computed bucket (for example a date range) needs no model property. Read-only, like the rest of the descriptor, so no change notification is needed. WPF's `StringComparison` and custom `GroupDescription.GroupNameFromItem` are not added. |
| `ListCollectionView` fine-grained `Add` / `Remove` / `Replace` | Incremental updates on a **flat** shaped view | Implemented for sort + filter + `CustomSort`. Grouped views still `Reset`. See "Source changes". |

## Usage

```xml
<Page.Resources>
    <CollectionViewSource x:Name="PeopleSource" />
</Page.Resources>

<ListView ItemsSource="{Binding Source={StaticResource PeopleSource}}">
    <ListView.GroupStyle>
        <GroupStyle>
            <GroupStyle.HeaderTemplate>
                <DataTemplate>
                    <!-- DataContext is the computed group; Key is the GroupDescriptions value. -->
                    <TextBlock Text="{Binding Key}" />
                </DataTemplate>
            </GroupStyle.HeaderTemplate>
        </GroupStyle>
    </ListView.GroupStyle>
</ListView>
```

```csharp
PeopleSource.Source = people;   // e.g. ObservableCollection<Person>
PeopleSource.Filter = item => ((Person)item).IsActive;
PeopleSource.SortDescriptions.Add(new SortDescription("LastName", ListSortDirection.Ascending));
PeopleSource.SortDescriptions.Add(new SortDescription("FirstName", ListSortDirection.Ascending));
PeopleSource.GroupDescriptions.Add(new PropertyGroupDescription("Department"));

people.Add(new Person { /* ... */ });   // View refreshes automatically
person.LastName = "Smith";              // no live shaping: call PeopleSource.Refresh()
```

Property names use the same path syntax and resolution as `{Binding Path=...}`, including nested paths
(`Address.City`). An empty name means the item itself, as in WPF.

## Behavior

- **Opt-in.** Shaping starts when `SortDescriptions` or `GroupDescriptions` is non-empty or `Filter` or
  `CustomSort` is non-null.
  Touching the collections without adding anything does not rebuild `View`.
- **Precedence.** `IsSourceGrouped = true` and sources that implement `ICollectionViewFactory` keep their existing
  behavior. Descriptors are ignored for them in the POC.
- **Pipeline.** Filter, then stable multi-key sort, then group. Groups appear in first-appearance order of the sorted
  items (WPF behavior). Sort by the group property first if you want ordered groups.
- **Comparison.** Strings use `CompareStringEx` with the user locale. Numeric types compare as numbers, and
  `DateTime`, `TimeSpan` and `Boolean` are also ordered. `null` sorts first. Anything else compares equal, so source
  order is kept.
- **Output.** The view is read-only and never writes to the source. Ungrouped output is a `VectorCollectionView`;
  grouped output is a `GroupedDataCollectionView` whose `CollectionGroups` contain `ICollectionViewGroup` entries.
  `Group` is the computed group object (`Key`, `Name`), and `GroupItems` are the group's items.
- **Source changes.** `IObservableVector` sources and `IBindableVector` + `INotifyCollectionChanged` sources (for
  example `ObservableCollection<T>`) update the view automatically. On a **flat** shaped view (sort, filter and/or
  `CustomSort`, no groups) an `Add`, `Remove` or `Replace` is applied **incrementally**: the item is filtered, then
  placed by binary search on its sort keys (or `CustomSort`), ties broken by source index, which is exactly the
  position a full recompute gives it. The view raises the matching `ItemInserted`, `ItemRemoved` or `ItemChanged`
  (a replacement that moves is `ItemRemoved` + `ItemInserted`; one rejected by the filter raises nothing), so
  currency, realized containers, selection and the UIA tree survive. The view keeps one source index per output
  item, so `Remove` is O(n) and `Add` is O(n) plus O(log n) key reads. A source `Reset`, any change to a **grouped**
  view, a change under `DeferRefresh`, a change raised while the view is computing, or a size/index mismatch still
  triggers a **full recompute** and one `Reset`. `Refresh()` always recomputes fully. Plain `IVector` sources need
  `Refresh()`. `IIterable`-only sources are snapshotted, so re-set `Source`.
- **Descriptor changes.** A change to `SortDescriptions`, `GroupDescriptions` or `Filter` reshapes an already
  shaped `View` **in place** (one `Reset`), so a consumer holding `View` keeps a live view. A new `View` is created
  only when shaping first starts, or when output switches between flat and grouped (a different view class), the
  same way changing `IsSourceGrouped` does today. Clearing every descriptor of a flat shaped view keeps that view, in
  source order; clearing the last `GroupDescription` returns a new `View` (unshaped if no sort or filter remains).
  Wrap several changes in `DeferRefresh()` to recompute (and raise `Reset`) once.
- **Currency.** After a source-driven refresh, the previous `CurrentItem` is restored if it is still in the view.
  Otherwise the view moves to the first item. A rebuilt `View` starts at the first item, as today.

## Design decisions and precedents

1. **Extend CVS rather than add a type.** This is what the reviewer and #4307 ask for. `ICollectionView` is frozen at
   `WinUIContract 1` (`dxaml/xcp/dxaml/idl/winrt/core/microsoft.ui.xaml.coretypes2.idl`, `interface ICollectionView`),
   so shaping state lives on CVS, the way WPF's CVS proxies its view.
2. **Experimental members on an existing framework class.** This follows the `Window.Width`/`MinWidth` and
   `XamlIsland.ShouldConstrainPopupsToWorkArea` precedent:
   - The model uses `[Platform("Feature_ExperimentalApi", ...)]` on the class and `[VelocityFeature("Feature_ExperimentalApi")]`
     on members. See `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.cs:3478-3481` (Window.Width) and
     `:6084-6088` (XamlIsland).
   - The IDL gets a `[feature] [interface_name("...Feature_ExperimentalApi")]` member block
     (`coretypes2.idl:883-894`, Window).
   - The generated class adds a `forwarder_holder` and a QI that checks `Feature_ExperimentalApi::IsEnabled()`
     (`dxaml/xcp/dxaml/lib/winrtgeneratedclasses/Window.g.h:22-84`, `Window.g.cpp:48-53`).
3. **`Filter` is a delegate property, not WPF's `Filter` event.**
   - It matches WPF's `ICollectionView.Filter` and the Toolkit's `AdvancedCollectionView.Filter`.
   - It has one well-defined result instead of multicast "any handler can reject" semantics.
   - Framework events need a `KnownEventIndex` and an args runtime class (`Popup.g.cpp:290-300`), so an event means
     more type-table churn.
   - Precedent for an experimental framework delegate with a return value: `DataTemplateElementFactory`
     (`Microsoft.UI.Xaml.cs:3788-3792`, `microsoft.ui.xaml.controls.controls.idl:452-455`), stored in a `TrackerPtr`
     (`DataTemplate_Partial.h:30`).
   - A WPF-style event can be added later as XAML-friendly sugar.
4. **`PropertyKind.PropertyOnly`, no new dependency properties.** PropertyOnly members are not in the type tables:
   `Indexes.g.h:1549` has `Window_Width` (a DP) but there is no `Window_ExtendsContentIntoTitleBar` (PropertyOnly).
   That keeps the hand-edited generated surface small. The cost is that descriptors cannot be set in markup yet.
5. **Descriptor classes copy `ItemIndexRange`** (`Microsoft.UI.Xaml.Data.cs:707-715`): plain, free-threaded,
   `IsCreateableFromXAML = false`, factory constructor. Whole-class feature gating copies the generated
   `InputValidationContext.g.h:14-41` / `.g.cpp:24-37` pattern. `[Optional]` string constructor parameters follow
   `PropertyChangedEventArgs` (`Microsoft.UI.Xaml.Data.cs:57`). The enum follows `XamlChangeId`
   (`Microsoft.UI.Xaml.Settings.cs:11-16`): experimental, excluded from the type tables.
6. **Reuse the existing view classes; no new internal view type.**
   - The engine writes into a read-only observable vector. The flat case wraps it with `VectorCollectionView`. The
     grouped case passes a vector of computed group objects to `GroupedDataCollectionView` with no `ItemsPath`, which
     is exactly what `IsSourceGrouped = true` does (`CollectionViewManager.cpp:43`).
   - A new `CollectionView` subclass would need a `KnownTypeIndex` and metadata-table changes.
7. **Property paths use the framework's `PropertyPathListener`**, the same way `GroupedDataCollectionView` resolves
   `ItemsPath` (`GroupedDataCollectionView_Partial.cpp:302-323`). Paths therefore resolve exactly like `{Binding}`.
8. **Rebuild on descriptor change** copies `OnIsSourceGroupedChanged` → `EnsureView`
   (`CollectionViewSource_Partial.cpp:73`, `:122`), but only to start shaping or to switch between flat and grouped
   output. Otherwise the shaped view is reshaped in place.
9. **`ICollectionViewShaping` is an optional interface on the view**, not a change to the frozen `ICollectionView`.
   It follows the `ICollectionViewFactory` / `ISupportIncrementalLoading` pattern of QI-able capabilities. The
   existing `CollectionView` base implements it, and `QueryInterfaceImpl` answers it only when a CVS has set itself
   as the view's shaping owner (`CollectionView::SetShapingOwner`, a weak reference). An unshaped view, or a view
   created any other way, does not offer it. Every member forwards to the owning CVS, so the view and the CVS share
   one set of descriptor collections.

## How controls consume it

- **ListView / GridView / ItemsControl.** Unchanged. They read `ICollectionView.CollectionGroups` when a `GroupStyle`
  is set (`ItemsControl_Partial.cpp:2658`, `ItemContainerGenerator_Partial.cpp:1978`,
  `ModernCollectionBasePanel_CacheManager_Partial.cpp:146`). The group header's DataContext is the computed group, so
  `{Binding Key}` works.
- **SemanticZoom.** The zoomed-out view reads the same `CollectionGroups`
  (`ListViewBase_Partial_SemanticZoomInformation.cpp:386`).
- **TableView.** See the end-to-end story below.

## End to end: TableView on a shaped CollectionViewSource

The app gives TableView `cvs.View` as `ItemsSource`:

```csharp
var cvs = new CollectionViewSource { Source = processes };
cvs.GroupDescriptions.Add(new PropertyGroupDescription("Category"));
table.ItemsSource = cvs.View;                       // grouped, shapeable view

cvs.Filter = item => ((ProcessItem)item).Name.Contains(query);   // rows follow the view
((ICollectionViewShaping)cvs.View).Refresh();                    // after property edits
```

- **Rows and groups.** TableView projects the view read-only. `CollectionGroups` become group rows
  (`TableViewCollectionViewGroups`), in the view's order. TableView never reorders, filters or groups the rows
  itself.
- **Header sort.** When `ItemsSource` implements `ICollectionViewShaping`, a header click, `SortByColumn`,
  `ToggleSortDirection` and `ClearSort` write the column's `SortMemberPath` to the view's `SortDescriptions` (the
  column's sort replaces the previous one), and `Sorting` / `Sorted` are raised. A `CustomSortComparer` column
  (with or without a `SortMemberPath`; the comparer wins, as in the control's own sort) sorts the view through its
  `CustomSort` instead: TableView wraps the column's comparer, negated for descending, and installs it under one
  `DeferRefresh`, which also clears `SortDescriptions`. If the comparer throws, TableView puts the view's previous
  `SortDescriptions` / `CustomSort` back, so the previous order and arrow stay. Setting a new `CustomSortComparer` on
  the sorted column re-sorts the view.
- **Sort arrow.** The arrow follows the view's primary `SortDescription`, including a sort the app or another
  consumer applies to the view directly (`TableView` listens to `SortDescriptions.VectorChanged`). While the view
  still sorts with the `CustomSort` TableView installed, the comparer column stays lit. A `CustomSort` the app
  sets directly is not reflected by an arrow (there is no change event for it).
- **Row updates.** A source add, remove or replace on a flat shaped view reaches TableView as a fine-grained vector
  change, so selection and realized rows follow the item (Task Manager's periodic edits no longer tear the table
  down). Grouped views still `Reset`; TableView keeps expansion and selection across it by key and identity.
- **Plain `ICollectionView`.** A view without `ICollectionViewShaping` keeps today's contract: the header click raises
  `Sorting` only, and the app sorts the view.
- **Group identity.** A CVS recomputes its group objects on every `Refresh` or source change, so TableView keys a
  group by its key **value** (the group's `Key` property, else `ICollectionViewGroup.Group`; scalars compare by type
  and value), not by the group object's address. Expansion state therefore survives a recompute: a collapsed group
  stays collapsed after a source change or `Refresh()`.
- **Sample.** `Samples\TableViewSampleApp`, page "CollectionViewSource shaping" (`CollectionViewShapingPage`):
  Task Manager-style flat data, grouped by `GroupDescriptions`, a filter box using `Filter`, `Refresh()` after a
  value update, and header sort through the view.

### TableViewSource removed

The public `TableViewSource` (and its `TableViewPredicate`, `TableViewKeySelector` and
`TableViewGroupIdentitySelector` delegates) is removed. Apps shape data with `CollectionViewSource` and hand
TableView its `View`. A plain collection is still sorted by TableView itself on a header click, through the internal
engine below.

### Hierarchy: TableView.ItemKeyPath / ParentKeyPath (V1)

Tree rows have **no CVS or WPF equivalent**. It is not just one more descriptor: in tree mode sort applies per
sibling set and grouping buckets roots only, and `ICollectionViewGroup` has no notion of depth. Task Manager's
expandable Apps rows need it.

For V1 the relation is declared on TableView with two string property paths, `ItemKeyPath` and `ParentKeyPath`
(preview, like the rest of TableView). They drive the internal engine's parent-key relation over whatever
`ItemsSource` is: a plain collection, or a CVS `View`. Over a view the CVS filter, sort and grouping run first, then
the rows are nested:

- Siblings keep the view's order, so `SortDescriptions` sorts each sibling set.
- The CVS filter runs before nesting: a match whose parent is filtered out is shown as an orphan root (the former
  `TableViewSource` kept the ancestors of a match).
- With a grouped view, each root is placed in its group and keeps its subtree.
- No live shaping: a key edit takes effect on `Refresh()` or a collection change.

Hierarchy on CVS itself (a `HierarchyDescription`, or option (b)) is future work; if it is approved, the relation
moves from TableView into the view and `ItemKeyPath` / `ParentKeyPath` become a shorthand for it.

### Internal components after TableViewSource removal

None of these are in public IDL or the XAML metadata provider.

| Component | Role | V1 | End state |
|---|---|---|---|
| `ShapedItemsSource` | Engine: sort, filter, group and live shaping for plain collections, plus the parent-key hierarchy. | Internal, unchanged. | Sort, filter and group retire once CVS has live shaping, comparer sort and incremental updates; TableView then wraps a plain list in an internal CVS. |
| `TableViewShapingSource` (was `TableViewSource`) | Non-projected facade TableView creates over `ShapedItemsSource` for every `ItemsSource`; header sort on plain collections and `ItemKeyPath` / `ParentKeyPath` go through it. | Internal. | Shrinks with the engine to the hierarchy facade. |
| `GroupedSourceAdapter` | Flattens groups into header and data rows for the repeater. | Internal, unchanged. | Stays: TableView's presentation layer over `ICollectionView.CollectionGroups`. |
| `HierarchicalSourceAdapter` | Flattens the tree and tracks row expansion. | Internal, unchanged. | Stays in TableView (`ItemKeyPath` / `ParentKeyPath`), or moves into CVS if hierarchy on CVS is approved. |
| `TableViewCollectionViewGroups` | Maps CVS `CollectionGroups` to the group contract the adapters read. | Internal, unchanged. | Stays while CVS groups feed the adapters. |

## Out of scope for the POC

- Live shaping: re-sorting, re-filtering or re-grouping when an item's *property* changes (WPF `IsLiveSorting` etc.).
  Use `Refresh()`.
- Key-selector delegates, `StringComparison` / culture selection (WPF `CollectionViewSource.Culture`). The
  `CustomSort` comparer covers C++ and AOT callers.
- Multi-level grouping (only the first `GroupDescription` is used; `ICollectionViewGroup` is single level),
  `GroupDescription` subclasses, and group sorting.
- XAML-declared descriptors (needs DP-backed collection properties and type-table entries).
- `CanSort` / `CanFilter` / `CanGroup` on `ICollectionViewShaping`.
- Hierarchy on CVS (see "Hierarchy: TableView.ItemKeyPath / ParentKeyPath (V1)").
- Incremental updates for **grouped** views (a change re-buckets and raises `Reset`), `Move` as a single change,
  and coalescing a burst of changes. Grouped incremental updates need per-group bookkeeping and group insert/remove
  notifications through `GroupedDataCollectionView`; they are deferred to keep this change reviewable.
- Performance work: virtualization, hashed group lookup. A full recompute is O(n log n), plus O(n × groups) for
  grouping.

## Known risks

- **Codegen.** Generated outputs were first hand-written, then regenerated with `dxaml\xcp\runcodegen.cmd`
  (`a78612b94`, and again for `ICollectionViewShaping`). See "Regeneration" below.
- **Flat/grouped switch.** Adding the first or removing the last `GroupDescription` gives the CVS a new `View`, so a
  control bound to the old `View` must be given the new one (the sample reassigns `ItemsSource`).
- **Reorder.** `ListViewBase` drag-reorder calls `RemoveAt` / `InsertAt` on the items source
  (`ListViewBase_Partial_Reorder.cpp:1498-1499`). Shaped views are read-only, the same as `IsSourceGrouped` views
  today, so set `CanReorderItems="False"`. WPF also blocks reorder on sorted views.
- **Incremental loading.** `ISupportIncrementalLoading` sources lose `HasMoreItems` because the full source is
  materialized. WPF and ACV have the same limitation.
- **Full refresh cost.** On a grouped view, every source change is a full recompute plus a `Reset`; selection and
  scroll position follow `Reset` semantics. Flat views apply `Add` / `Remove` / `Replace` incrementally.
- **Reentrancy.** Changes raised while computing (for example from inside `Filter`, `CustomSort` or a property
  getter) are not applied; the view's incremental bookkeeping is invalidated, so the next source change or
  `Refresh()` recomputes fully.
- **Deferral lifetime.** A `DeferRefresh` deferral that is never closed keeps the view stale (WPF behaves the same).
  It holds the CVS strongly until closed.
- **Lifetime.** The group key is held with a `ComPtr`, not a `TrackerPtr`: `ObservableTrackerCollection`'s reference
  tracker walk is `final`. A managed key object is kept alive by its group until the next refresh.
- **API shape.** Decide `GroupDescription` (abstract base) vs. `PropertyGroupDescription` before leaving experimental.
  Adding a base class later is breaking in WinRT. `SortDescription` and `ListSortDirection` share simple names with
  WPF and the Toolkit, so apps that use both namespaces need aliases.
- **Feature gate in tests.** The new test assumes `Feature_ExperimentalApi` is enabled in the test configuration,
  like other experimental-API tests (for example `GridViewIntegrationTests.cpp:573`).

## Regeneration

Generated outputs were hand-edited to match the generator. Run `dxaml\xcp\runcodegen.cmd` and keep its output.

| File | Change | Must regenerate / verify |
|---|---|---|
| `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Data.cs` | Source of truth (hand-authored) | — |
| `dxaml/xcp/dxaml/idl/winrt/core/microsoft.ui.xaml.coretypes2.idl` | Enum, delegate, two classes, CVS experimental block | Yes. Also expect `IReference<ListSortDirection>` in the enum `declare` block if the generator emits it. |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/CollectionViewSource.g.h/.g.cpp` | Forwarder, QI, accessors, `Refresh` | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/SortDescription.g.h/.g.cpp` (new) | Full class + factory | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/PropertyGroupDescription.g.h/.g.cpp` (new) | Full class + factory | Yes |
| `dxaml/xcp/dxaml/lib/Factories.g.cpp` | 2 forward declarations + 2 activation entries | Yes (ordering) |
| `dxaml/xcp/dxaml/lib/synonyms.g.h` | 4 typedefs (with `ICollectionViewShaping`) | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/Aggregate.g.cpp` | 2 includes | Yes |
| `dxaml/xcp/components/metadata/*`, `dxaml/xcp/core/metadata/*`, XBF indexes | None expected (PropertyOnly members, `IsCreateableFromXAML = false` classes, enum excluded from type tables) | Verify no diff |
| `dxaml/xcp/dxaml/idl/winrt/main/microsoft.ui.xaml.idl` | None | Add `declare` entries only if MIDL does not emit `IVector`/`IIterable` of the descriptor types |
| `packaging/Intellisense/*` | None | Regenerate docs XML when promoting |

Hand-written files: `CollectionViewSource_Partial.h/.cpp` (hook in `EnsureView`), `CollectionViewSource_Shaping.cpp`
(engine, descriptor factories and the `ICollectionViewShaping` forwarders), `CollectionView_Partial.h`
(`ICollectionViewShaping` on the view base), and `wrtdxamlfoundation.vcxproj`.

## Test

`BindingIntegrationTests::CollectionViewSourceShaping`
(`dxaml/test/native/external/framework/dataBinding/tests`) covers:

- the unshaped view;
- `Filter` (the source is unchanged);
- descending sort by path;
- live refresh on source `Append` with current-item preservation;
- grouping (`CollectionGroups`, `Key`, `GroupItems`);
- returning to the unshaped view.

`BindingIntegrationTests::CollectionViewShapingInterface` covers `ICollectionViewShaping`: not offered by an unshaped
view; offered by flat and grouped shaped views; shared descriptor collections; sort, `Filter` and `Refresh()` through
the view reshaping the same view in place; and the grouped-view switch.

The capabilities added after the first POC each have a test in the same class:

- `CollectionViewSourceDeferRefresh`: descriptors set under a deferral take effect when it closes; nested
  deferrals (from the CVS and from the view) batch descriptor, `Filter`, `Refresh()` and source changes into one
  `Reset`; an idle deferral raises nothing.
- `CollectionViewSourceCustomSort`: `CustomSort` alone shapes the view with ties in source order; adding a
  `SortDescription` clears it; setting it clears `SortDescriptions` with one `Reset`; a throwing comparer fails and
  leaves the rows.
- `CollectionViewSourceGroupConverter`: `Converter` maps values to group keys.
- `CollectionViewSourceIncrementalUpdates`: `Add`, filtered `Insert`, `Remove` (currency kept), a moving `Replace`
  (`Remove` + `Insert`), an in-place `Replace` (`ItemChanged`) and a filter-accepted `Replace` (`Insert`) raise exactly
  those changes, the result matches `Refresh()`, a source `Reset` and a grouped view recompute fully.

`TableView_DataBinding_APITests` (`controls\dev\TableView\APITests`) covers the end-to-end story:

- `VerifyHeaderSortOnShapingCollectionViewSortsTheView`: header click, `ToggleSortDirection`, `SortByColumn` and
  `ClearSort` write `SortDescriptions`; the arrow follows a sort applied to the view; `Sorting` / `Sorted` are raised.
- `VerifyFilterAndRefreshOnShapingCollectionViewUpdateRows`: `Filter`, `Refresh()` after an edit, source add.
- `VerifyGroupDescriptionsOnShapingCollectionViewRenderGroups`: group rows keyed by value, sort within groups.
- `VerifyCollapsedCollectionViewGroupSurvivesRecompute`: a collapsed group stays collapsed after a source change and
  after `Refresh()`.
- `VerifyCustomSortComparerColumnSortsShapingCollectionView`: a `CustomSortComparer` column sorts the view through
  `CustomSort` in both directions; a path column's sort replaces it; a throwing comparer leaves the previous sort,
  rows and arrow; `ClearSort` clears both.
- `VerifyIncrementalShapingCollectionViewChangesUpdateRows`: add, remove and replace on a sorted, filtered view reach
  the rows with no `Reset`, and the selection stays on its item.

## Effort to productize

These are rough estimates for one engineer who knows dxaml.

| Item | Estimate |
|---|---|
| Build, fix, run codegen, test matrix (ListView, GridView, SemanticZoom, `IsSourceGrouped`, CLR sources) | 1 week |
| Incremental updates for grouped views (flat views and `DeferRefresh` are in the POC) | 1 week |
| Live shaping (property-change tracking) | 1–2 weeks |
| Culture / `StringComparison`, `GroupDescription` base decision (`CustomSort` and `Converter` are in the POC) | 0.5 week |
| XAML-declarable descriptors (DP collections, type tables) | 1 week |
| View-level shaping interface: `CanSort` / `CanFilter` / `CanGroup`, API review (the POC interface exists) | 0.5–1 week |
| API review, docs, perf and back-compat tests | 1–2 weeks |
| **Total** | **≈ 5–8.5 engineer-weeks** |

Porting the existing TableView shaping engine (identity-preserving reshapes, coalesced live shaping) into dxaml
instead of extending this POC is the main lever on the upper half of that range.
