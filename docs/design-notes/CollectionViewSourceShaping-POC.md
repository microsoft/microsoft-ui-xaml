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
- Make it strictly opt-in. With no descriptor and no `Filter`, `View` is created by the same code as before (it
  only additionally answers `ICollectionViewShaping`; see "Behavior").
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
enum CollectionViewStringComparison   // System.StringComparison's names and values
{ CurrentCulture, CurrentCultureIgnoreCase, InvariantCulture, InvariantCultureIgnoreCase, Ordinal, OrdinalIgnoreCase };

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
runtimeclass PropertyGroupDescription
{
    PropertyGroupDescription(String propertyName);
    PropertyGroupDescription(String propertyName, IValueConverter converter);
    PropertyGroupDescription(String propertyName, IValueConverter converter, CollectionViewStringComparison stringComparison);
    String PropertyName { get; };
    IValueConverter Converter { get; };
    CollectionViewStringComparison StringComparison { get; };            // Ordinal unless set
    IObservableVector<SortDescription> SortDescriptions { get; };        // group order (WPF GroupDescription)
    CollectionViewComparer CustomSort;                                   // compares two group names
};

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
delegate void FilterEventHandler(Object sender, FilterEventArgs e);

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
runtimeclass FilterEventArgs
{
    Object Item { get; };
    Boolean Accepted;        // true when raised
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
        String Culture;                                    // BCP-47 tag; empty = user locale
        event FilterEventHandler FilterRequested;          // WPF's CollectionViewSource.Filter event
        void Refresh();
        Windows.Foundation.IClosable DeferRefresh();
        static ICollectionView GetDefaultView(Object source);
    }
};

// Optional capability of an ICollectionView, like ICollectionViewFactory or ISupportIncrementalLoading.
// A View that a CollectionViewSource creates over a vector implements it; it owns the rules (see "Architecture").
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
interface ICollectionViewShaping
{
    IObservableVector<SortDescription> SortDescriptions { get; };
    IObservableVector<PropertyGroupDescription> GroupDescriptions { get; };
    CollectionViewFilter Filter;
    void Refresh();
    Windows.Foundation.IClosable DeferRefresh();
    CollectionViewComparer CustomSort;
    Object SourceCollection { get; };                      // WPF ICollectionView.SourceCollection
    String Culture;                                        // WPF CollectionView.Culture, as a language tag
};

// WPF's ICollectionViewLiveShaping. A view a CollectionViewSource creates over a vector implements it.
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
interface ICollectionViewLiveShaping
{
    Boolean CanChangeLiveSorting { get; };                 // true
    Boolean CanChangeLiveFiltering { get; };               // true
    Boolean CanChangeLiveGrouping { get; };                // true
    Windows.Foundation.IReference<Boolean> IsLiveSorting;  // WPF bool?; never null here, setting null throws
    Windows.Foundation.IReference<Boolean> IsLiveFiltering;
    Windows.Foundation.IReference<Boolean> IsLiveGrouping;
    Windows.Foundation.Collections.IObservableVector<String> LiveSortingProperties { get; };   // empty: SortDescriptions'
    Windows.Foundation.Collections.IObservableVector<String> LiveFilteringProperties { get; }; // empty: none
    Windows.Foundation.Collections.IObservableVector<String> LiveGroupingProperties { get; };  // empty: GroupDescriptions'
};

runtimeclass CollectionViewSource
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
    {
    // ...the members above, and, applied to View as WPF's ApplyPropertiesToView does:
    Boolean IsLiveSortingRequested;
    Boolean IsLiveFilteringRequested;
    Boolean IsLiveGroupingRequested;
    Windows.Foundation.Collections.IObservableVector<String> LiveSortingProperties { get; };
    Windows.Foundation.Collections.IObservableVector<String> LiveFilteringProperties { get; };
    Windows.Foundation.Collections.IObservableVector<String> LiveGroupingProperties { get; };
    }
};
```

| WPF | This POC | Note |
|---|---|---|
| `CollectionViewSource.SortDescriptions` (`SortDescriptionCollection` of `SortDescription` structs) | `IObservableVector<SortDescription>`, immutable runtime class | WinRT has no struct-with-string collections that XAML can fill; an immutable class keeps WPF's value semantics. |
| `ListSortDirection` | `ListSortDirection` | Same names and values. |
| `CollectionViewSource.GroupDescriptions` (`GroupDescription` base) | `IObservableVector<PropertyGroupDescription>` | Only property grouping. See risks for the base-class decision. Several descriptions nest groups (below). |
| `ICollectionView.Filter` (`Predicate<object>`) | `Filter` property of type `CollectionViewFilter` on CVS and `ICollectionViewShaping` | See decision 3. |
| `CollectionViewSource.Filter` **event** (`FilterEventHandler`, `FilterEventArgs.Item` / `Accepted`) | `CollectionViewSource.FilterRequested` event (`FilterEventHandler`, `FilterEventArgs`) | Implemented. Named `FilterRequested` because `Filter` is already the delegate property and WinRT (C# projection) cannot have a property and an event of one name. As in WPF, `Accepted` starts `true`, each handler may change it, and adding or removing a handler refilters. **While it has handlers the event wins** and the `Filter` delegate is ignored (WPF replaces `view.Filter` with the event's wrapper); removing the last handler hands filtering back to the delegate. A CVS whose only rule is a handler shapes its view. |
| `ICollectionView.Refresh()` | `CollectionViewSource.Refresh()` and `ICollectionViewShaping.Refresh()` | `ICollectionView` is frozen, so the verb lives on CVS and on the opt-in view interface. |
| `ICollectionView.SortDescriptions` / `GroupDescriptions` / `Filter` | `ICollectionViewShaping` on the shaped `View` | Same objects as the CVS's: one set of descriptors, reachable from `ItemsSource` alone. |
| `CollectionViewGroup.Name` | `Key` and `Name` on the group object | `Key` matches the `IGrouping<K,T>` idiom WinUI samples already use. |
| `ICollectionView.DeferRefresh()` → `IDisposable` | `DeferRefresh()` → `IClosable` on CVS and `ICollectionViewShaping` | Implemented. Nested deferrals count; changes made while one is open (descriptors, `Filter`, `CustomSort`, `Refresh`, source changes) recompute once, when the last closes. C# `using` and C++/CX `delete` close it. An unclosed deferral defers forever, as in WPF. |
| `ListCollectionView.CustomSort` (`IComparer`) | `CustomSort` property of type `CollectionViewComparer` on CVS and `ICollectionViewShaping` | Implemented. A delegate, not an interface, so a C++/WinRT lambda or a C# method group works and nothing needs reflection (AOT-safe). As in WPF, setting it clears `SortDescriptions`, and adding a `SortDescription` clears it. Ties keep source order. A comparer that fails fails the change and leaves the view's rows. |
| `PropertyGroupDescription.Converter` (`IValueConverter`) | `PropertyGroupDescription(propertyName, converter)` and read-only `Converter` | Implemented. `Convert(value, Object, null, culture)` maps the property value to the group key (the language argument is the view's `Culture`), so a computed bucket (for example a date range) needs no model property. Read-only, like the rest of the descriptor. Custom `GroupDescription.GroupNameFromItem` is not added. |
| `PropertyGroupDescription.StringComparison` (`System.StringComparison`, default `Ordinal`) | `PropertyGroupDescription(propertyName, converter, stringComparison)` and read-only `StringComparison` of type `CollectionViewStringComparison` | Implemented. How string group names match (`String.Equals(name, name, comparison)`); other names use value equality. The group keeps the name of its first item. The enum mirrors `System.StringComparison` (names and values) under another name, because a `Microsoft.UI.Xaml.Data.StringComparison` would make `StringComparison` ambiguous in every C# file that also uses `System`. Set through the constructor, like `Converter` (WPF also has that constructor). |
| `GroupDescription.SortDescriptions` / `CustomSort` (WPF 4.5 group sorting) | `PropertyGroupDescription.SortDescriptions` and `CustomSort` | Implemented. Orders the groups at that level: `SortDescriptions` by a group property (`Name`/`Key`, `ItemCount`), `CustomSort` by comparing **two group names** (WPF passes `CollectionViewGroup`s; WinUI has no public group class, so the names are what an app can use). As in WPF they replace each other, a change regroups the views that use the description, groups are placed when created (a group whose `ItemCount` later changes does not move until `Refresh()`), and without a group order groups follow their first item. On `PropertyGroupDescription`, not a `GroupDescription` base (see risks). |
| `CollectionView.Culture` / `CollectionViewSource.Culture` (`CultureInfo`) | `Culture` (`String`, a BCP-47 language tag) on CVS and `ICollectionViewShaping` | Implemented. Strings sort with `CompareStringEx` in that locale; empty means the user's locale (WPF falls back to the element's `xml:lang`). A `String` because `CultureInfo` is not a WinRT type; an invalid tag throws `ArgumentException`. Changing it re-sorts. Group-name matching with `CurrentCulture*` uses the user's locale, as WPF uses `CultureInfo.CurrentCulture`. |
| `ICollectionView.SourceCollection` (`IEnumerable`) | `ICollectionViewShaping.SourceCollection` (`Object`) | Implemented: the collection the view was created over (the CVS's `Source`, not a wrapper). `ICollectionView` is frozen, so it is on the shaping interface. |
| `CollectionViewSource.GetDefaultView(object)` (`ViewManager` default view) | static `CollectionViewSource.GetDefaultView(Object)` | Implemented. One shared, shapeable view per list source (`IVector` / `IBindableVector`), created by `CollectionViewManager` and owned by no CVS, so it starts and stops grouping in place. The table holds it **weakly**: while anything holds the view, every call returns it; once nothing does, the next call creates a new one (its rules are gone). An `ICollectionView` source returns itself (WPF wraps it in a proxy), `null` returns `null`, and a non-list source (`IIterable`, `IVectorView`, `ICollectionViewFactory`) gets a new, unshared view on each call. Views are per UI thread. |
| `ListCollectionView` fine-grained `Add` / `Remove` / `Replace` | Incremental updates on **flat and grouped** shaped views | Implemented for sort + filter + `CustomSort`, with or without groups. See "Source changes". |
| `ICollectionView.Groups` (null while not grouping), the view kept when grouping starts or stops, `ItemsControl.IsGrouping` following it | `ICollectionView.CollectionGroups` of a shaping view: null while flat, else **one** `IObservableVector` for the view's lifetime; the view raises `PropertyChanged("CollectionGroups")` and then a `Reset` when grouping starts or stops; ListView, GridView, `ItemsControl` and TableView regroup the same `ItemsSource` | Implemented. A regroup is one `Reset` of `CollectionGroups`, raised once the groups are complete, then one `Reset` of the view (WPF raises `Reset` on `Groups`). WinRT `ICollectionView` has no `GroupDescriptions`, so the null/non-null switch plus `PropertyChanged` is the signal (see "Grouping keeps the view"). |
| `ListCollectionView.AddItemToGroups` / `RemoveItemFromGroups` | Grouped incremental updates | Implemented. An item joins its bottom-level group at its sorted position; a group it starts is inserted among its siblings (by the group order, else before the first sibling whose first item sorts after it, as WPF's `FindIndex` with a seed); a group it empties is removed. |
| Several `GroupDescriptions` (nested `CollectionViewGroup`s, `IsBottomLevel`, `Items`, `ItemCount`) | Multi-level groups | Implemented. Each group is an `ICollectionViewGroup` whose `GroupItems` are its subgroups (WPF `Items`) above the bottom level, else its items. `CollectionGroups` and the view's items stay single level (the view enumerates leaves, as WPF's does), so ListView and GridView show the first level with all its leaves; TableView renders every level. Bindings see `Key`, `Name`, `Items`, `ItemCount` and `IsBottomLevel` on a group. |
| `ICollectionViewLiveShaping` (implemented by `ListCollectionView`) | `ICollectionViewLiveShaping`, implemented by the view a CVS creates over a vector (and by a default view) | Implemented; see "Live shaping". A separate interface, as in WPF, rather than members on `ICollectionViewShaping`. |
| `CanChangeLiveSorting` / `CanChangeLiveFiltering` / `CanChangeLiveGrouping` | Same names | Always `true`, as on `ListCollectionView`. |
| `IsLiveSorting` / `IsLiveFiltering` / `IsLiveGrouping` (`bool?`) | Same names, `IReference<Boolean>` | WPF's `bool?` is WinRT's `IReference<Boolean>` (C# sees `bool?`). As on `ListCollectionView` the value is never null and setting null throws (`ArgumentNullException` in WPF, `E_INVALIDARG` here); the nullable type is kept so a view that cannot tell (WPF's `null`) can implement the interface. Default `false`. |
| `LiveSortingProperties` / `LiveFilteringProperties` / `LiveGroupingProperties` (`ObservableCollection<string>`) | Same names, `IObservableVector<String>` | Property paths. Empty sorting or grouping lists mean the `SortDescriptions`' or the `PropertyGroupDescription`s' properties (none with a `CustomSort`); filtering observes only the listed paths, as in WPF. A change rebuilds what is observed. |
| `CollectionViewSource.IsLiveSortingRequested` / `IsLiveFilteringRequested` / `IsLiveGroupingRequested` | Same names | Plain properties (WPF: DPs). Applied to `View` when it is created and whenever they change, as WPF's `ApplyPropertiesToView` copies them; the CVS's own `Live*Properties` are copied into the view only while their feature is requested. |
| `CollectionViewSource.LiveSortingProperties` / `LiveFilteringProperties` / `LiveGroupingProperties` | Same names | A change is applied to `View` (WPF `OnForwardedCollectionChanged`). |
| `CollectionViewSource.CanChangeLive*` / `IsLive*` (read-only DPs mirroring the view) | Not added | Read them from `View` through `ICollectionViewLiveShaping`. |
| `LiveShapingList` / `LiveShapingItem` (a binding per item and live property) | `VectorCollectionView::LiveShapingState` / `LiveShapingItem` / `LivePropertyObserver` | Each observer is a property path listener of the binding engine (a `{Binding}`'s), so `INotifyPropertyChanged` items (C# and C++), `ICustomPropertyProvider` items, `DependencyObject` properties and nested paths all work. |

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
person.LastName = "Smith";              // without live shaping: call PeopleSource.Refresh()

PeopleSource.IsLiveSortingRequested = true;   // now the edit moves the row (LastName is a sort property)
person.LastName = "Adams";
```

Property names use the same path syntax and resolution as `{Binding Path=...}`, including nested paths
(`Address.City`). An empty name means the item itself, as in WPF.

## Behavior

- **Opt-in.** Shaping starts when `SortDescriptions` or `GroupDescriptions` is non-empty or `Filter` or
  `CustomSort` is non-null.
  Touching the collections without adding anything does not rebuild `View`. Before that, `View` is the usual
  unshaped view, but a flat `View` over a vector source (`IVector` or `IBindableVector`, not `IsSourceGrouped`, not
  `ICollectionViewFactory`) already answers `ICollectionViewShaping`, so a consumer given only
  `new CollectionViewSource { Source = items }.View` (for example a control's `ItemsSource`) can start shaping it.
- **Precedence.** `IsSourceGrouped = true` and sources that implement `ICollectionViewFactory` keep their existing
  behavior; their view cannot shape. As in WPF (`CannotSortView` / `CannotGroupView` / `CannotFilterView`), a rule
  set on such a CVS (a description, `Filter`, `CustomSort` or a `FilterRequested` handler) throws
  `InvalidOperationException` (`E_ILLEGAL_METHOD_CALL`) instead of being ignored, when the rule is added or when the
  view is created with it. CVSs without rules are unaffected.
- **Pipeline.** Filter, then stable multi-key sort, then group, level by level. Without a group order
  (`PropertyGroupDescription.SortDescriptions` / `CustomSort`), groups appear in first-appearance order of the sorted
  items (WPF behavior).
- **Comparison.** Strings use `CompareStringEx` with the view's `Culture`, else the user locale. Numeric types compare as
  numbers: two integers (including `Int64` and `UInt64`) exactly, otherwise as `double`; and
  `DateTime`, `TimeSpan` and `Boolean` are also ordered. `null` sorts first. Anything else compares equal, so source
  order is kept.
- **Output.** The view is read-only and never writes to the source. Both flat and grouped output are presented by
  the same view class (`VectorCollectionView`, WinUI's `ListCollectionView`); grouped output's `CollectionGroups`
  contain `ICollectionViewGroup` entries. `Group` is the computed group object (`Key`, `Name`), and `GroupItems` are
  the group's items.
- **Source changes.** `IObservableVector` sources and `IBindableVector` + `INotifyCollectionChanged` sources (for
  example `ObservableCollection<T>`) update the view automatically. On a **flat** shaped view (sort, filter and/or
  `CustomSort`, no groups) an `Add`, `Remove` or `Replace` is applied **incrementally**: the item is filtered, then
  placed by binary search on its sort keys (or `CustomSort`), ties broken by source index, which is exactly the
  position a full recompute gives it. The view raises the matching `ItemInserted`, `ItemRemoved` or `ItemChanged`
  (a replacement that moves is `ItemRemoved` + `ItemInserted`; one rejected by the filter raises nothing), so
  currency, realized containers, selection and the UIA tree survive. The view keeps one source index per output
  item, so `Remove` is O(n) and `Add` is O(n) plus O(log n) key reads. On a **grouped** view the same changes are
  applied to the group tree (see "`ListCollectionView.AddItemToGroups`" above): the item's bottom-level group raises
  the change at the item's position, every group above it raises it at the item's leaf index, a new or emptied group
  is inserted into or removed from its parent's `GroupItems` (`CollectionGroups` at the top level), and the view's
  items raise one change. A `Replace` that keeps the item's place is one `ItemChanged`, otherwise a remove and an
  insert. As in WPF, groups that exist do not move when their first item changes; `Refresh()` reorders them. A source
  `Reset`, a change under `DeferRefresh`, a change made while the view runs app code or raises its changes (see
  "Reentrancy"), or a size/index mismatch still triggers a **full recompute** and one `Reset`. `Refresh()` always recomputes fully. Plain `IVector` sources need
  `Refresh()`. `IIterable`-only sources are snapshotted, so re-set `Source`.
- **Descriptor changes.** A change to `SortDescriptions`, `GroupDescriptions` or `Filter` reshapes an already
  shaped `View` **in place** (one `Reset`), so a consumer holding `View` keeps a live view. When shaping first
  starts on a `View` that already answers `ICollectionViewShaping`, that same `View` is retargeted to the shaped
  output (one `Reset`; currency follows the item). Adding the first or removing the last `GroupDescription` also keeps
  the `View` (see "Grouping keeps the view"). A new `View` is created only when shaping first starts on a view that
  cannot shape (an `IIterable` or `IVectorView` source), or when `IsSourceGrouped` changes. Clearing every descriptor
  keeps the view, in source order. Wrap several changes in `DeferRefresh()` to recompute (and raise `Reset`) once.
- **Grouping keeps the view** (WPF keeps its `ListCollectionView` and switches `Groups` between null and a group
  collection). On a shaping view, `CollectionGroups` is null while the view is flat and, while it groups, one
  `IObservableVector` object for the whole life of the view. Grouping starts or stops in place: the view switches
  its items and `CollectionGroups`, raises `PropertyChanged("CollectionGroups")`, then one `Reset` (a consumer that
  reacts to the property change reads the new items with the new groups). A regroup (a rule change, `Refresh()`, a
  source `Reset`) raises one `Reset` on `CollectionGroups` once every group is built, then one `Reset` on the view.
  When grouping stops, the group collection is emptied (one `Reset`) after the view's `Reset`, so it holds no items.
  Views that do not shape (`IsSourceGrouped`, factory and app views) keep their behavior: an `IsSourceGrouped`
  change still gives the CVS a new `View`, and their `CollectionGroups` changes are raised as before.
- **Currency** (WPF `ListCollectionView.RefreshOverride`). After a refresh, currency that was before the first item
  or after the last stays there; a current item is followed if it is still in the view, otherwise currency moves to
  the first item. So a sort with nothing current (a synchronized ListView with nothing selected) selects nothing. A
  rebuilt `View` starts at the first item, as today.
- **Reentrancy.** App code runs inside the engine (filters, `FilterRequested` handlers, property getters, converters,
  comparers) and while it raises its changes (every `VectorChanged` / `PropertyChanged` handler). A rule change,
  `Refresh()` or source change made from there is not dropped: it is recorded and the view recomputes once that work
  unwinds, then raises its own `Reset` (WPF refreshes re-entrantly; neither throws). A handler that changes the view
  on every change of it would never finish; after 64 rounds the view fails with `E_ILLEGAL_METHOD_CALL` (WPF
  overflows the stack).
- **Incremental loading.** A shaped view forwards `HasMoreItems` and `LoadMoreItemsAsync` to the collection it shapes
  when that implements `ISupportIncrementalLoading`; the loaded items arrive as source changes and are placed by the
  rules like any other.
- **Group notifications.** A computed group raises `PropertyChanged("ItemCount")` when an item joins or leaves it, as
  WPF's `CollectionViewGroup` does, so `{Binding ItemCount}` in a group header stays current.
- **Live shaping** (WPF `ICollectionViewLiveShaping`, `ListCollectionView.RestoreLiveShaping`). With `IsLiveSorting`,
  `IsLiveFiltering` or `IsLiveGrouping` on, the view observes each source item's live properties (see the table
  above for which). A change marks the item, and at the next dispatcher turn the view restores every marked item, as
  WPF does with `Dispatcher.BeginInvoke(DataBind)`:
  - **Sorting:** the item is placed by binary search among the items that are not marked (WPF's `FindPosition` skips
    `IsSortDirty` items). An item still in order raises nothing; one that moved raises `ItemRemoved` then
    `ItemInserted` (WinRT vectors have no `Move`; WPF raises `Move`). On a grouped view it moves within its group.
  - **Filtering:** an item the filter now rejects is removed; one it now accepts is inserted at its sorted position.
    Without live filtering an item stays even if it no longer passes, as in WPF.
  - **Grouping:** an item whose group names changed leaves its group (an emptied group is removed) and joins the new
    one (a new group is inserted as a source add inserts it). Without live grouping it stays in its group but still
    moves within it when sorting is live.
  - Under `DeferRefresh` the marks wait; closing the last deferral restores them at once, incrementally, unless a
    rule change is pending, whose recompute observes the items afresh. A source change the view cannot follow
    incrementally makes the next restore a full recompute.
  - The observation follows the source: an added item is observed, a removed or replaced one is no longer, a recompute
    rebuilds it, and turning the features off or the view's destruction ends it. A property change raised from inside
    a restore is restored at the next turn.
  - A source change made while a live sort edit waits for its restore is applied by a recompute (the output is out
    of order, so a binary search cannot place the new item).
  - Turning a live feature on, or changing what it observes, catches up on the changes made while they were not
    observed (WPF `RebuildLocalArray`): the view recomputes and raises a `Reset` only if its order changed. Changing
    `IsLiveSorting`, `IsLiveFiltering` or `IsLiveGrouping` raises `PropertyChanged` for it.
  - As for `{Binding}`, a live property must change on the view's thread: a `PropertyChanged` raised on another
    thread fails (`RPC_E_WRONG_THREAD`, an exception at the app's raise) rather than being silently missed.
  - The listeners are walked by the view's reference tracker walk, so items whose objects point back at the owner
    of the CVS do not form an uncollectable cycle.

## Design decisions and precedents

1. **Extend CVS rather than add a type.** This is what the reviewer and #4307 ask for. `ICollectionView` is frozen at
   `WinUIContract 1` (`dxaml/xcp/dxaml/idl/winrt/core/microsoft.ui.xaml.coretypes2.idl`, `interface ICollectionView`),
   so the shaping API is on CVS (and on its view through the optional `ICollectionViewShaping`), the way WPF's CVS
   proxies its view. The shaping itself is done by the view (see "Architecture").
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
   - `VectorCollectionView`, the view a CVS already creates over a vector, carries the engine and plays WPF's
     `ListCollectionView` (see "Architecture" below).
   - A new `CollectionView` subclass would need a `KnownTypeIndex` and metadata-table changes: framework views are
     `DependencyObject`s activated by type index, so a C++-only subclass cannot be created.
7. **Property paths use the framework's `PropertyPathListener`**, the same way `GroupedDataCollectionView` resolves
   `ItemsPath` (`GroupedDataCollectionView_Partial.cpp:302-323`). Paths therefore resolve exactly like `{Binding}`.
8. **Rebuild on descriptor change** copies `OnIsSourceGroupedChanged` → `EnsureView`
   (`CollectionViewSource_Partial.cpp:73`, `:122`), but only to start shaping a view that cannot shape. Otherwise
   the shaped view is reshaped in place, including when grouping starts or stops.
9. **`ICollectionViewShaping` is an optional interface on the view**, not a change to the frozen `ICollectionView`.
   It follows the `ICollectionViewFactory` / `ISupportIncrementalLoading` pattern of QI-able capabilities.
   `VectorCollectionView` implements it directly, and `QueryInterfaceImpl` answers it only when a CVS created the
   view to shape (`EnableShaping`): every view a CVS creates over a vector source (shaped or not), and the view it
   creates over a snapshot of any other source once a rule is set. An `IsSourceGrouped` view, a factory view, or a
   view created any other way does not offer it. The view and the CVS share one set of descriptor collections.

### Architecture: the view owns the engine

As in WPF, the view does the shaping and `CollectionViewSource` is a proxy over it.

| WPF | WinUI POC |
|---|---|
| `CollectionViewSource.EnsureView` → `ViewManager.GetViewRecord` creates a `ListCollectionView` for an `IList` | `CollectionViewSource::EnsureView` → `CollectionViewManager::GetViewRecord` creates a `VectorCollectionView` for an `IVector` / `IBindableVector` (and, once a rule is set, for a snapshot of any other enumerable) and reports it shapeable |
| `CollectionViewSource.ApplyPropertiesToView` copies `SortDescriptions`, `GroupDescriptions` and `Filter` onto the view | `CollectionViewSource::ApplyPropertiesToView` hands the view the CVS's descriptor collections (shared, not copied), `Filter` and `CustomSort` |
| `ListCollectionView` owns `InternalList`, sorting, filtering, grouping, `DeferRefresh` | `VectorCollectionView` owns the source subscription, the rules, `InternalList`, `RefreshOverride`, `ProcessCollectionChanged`, the `DeferRefresh` level and the groups |

- **The view owns the rules while it is `View`.** The descriptor collections are shared objects: `cvs.SortDescriptions`
  and `((ICollectionViewShaping)cvs.View).SortDescriptions` are the same collection, and a change to it is reported
  to the view (or, while there is no shaping view, to the CVS). `cvs.Filter` / `cvs.CustomSort` / `cvs.Refresh()`
  forward to the view; when `Source` changes, the CVS takes `Filter` and `CustomSort` back and applies them to the
  next view. A view the CVS has replaced keeps a copy of the rules it had and keeps following its own source, as
  each WPF view keeps the rules copied into it; nothing set on it reaches the CVS or its new `View`.
- **Lifetime.** The CVS holds its view (`View`); the view holds the CVS weakly, except while the CVS has
  `FilterRequested` handlers, when it holds it with a tracked reference (WPF's view holds the CVS through the
  wrapped filter delegate). A consumer that holds only `cvs.View` (for example TableView's `ItemsSource`) keeps a
  working, shapeable and still filtered view after the app drops the CVS.
- **Release builds.** Shaping is an experimental API: in a build without `MUX_PRERELEASE`, `EnsureView` takes the
  pre-shaping path, so the view never answers `ICollectionViewShaping` and `ItemsControl` attaches nothing.
- **`DeferRefresh`.** The level lives on the view. A deferral taken on the CVS takes one on its view while any CVS
  deferral is open (also on a view created while one is open), so deferrals from either side batch together.
- **Flat and grouped in one class.** A shaping view presents flat output directly and grouped output through an
  internal `GroupedDataCollectionView` over its group list, whose `CollectionGroups` it exposes. Each of the two
  output lists and the grouped presentation is created once, so switching between flat and grouped output keeps the
  view and its `CollectionGroups` object; the switch is announced with `PropertyChanged("CollectionGroups")` and a
  `Reset` (see "Grouping keeps the view"). The grouped presentation rebuilds `CollectionGroups` as one `Reset`
  (`GroupedDataCollectionView::SetRaisesSingleGroupsReset`); an `IsSourceGrouped` view's presentation still clears
  and refills its groups one notification at a time, as before.

### Internal names follow WPF

The engine (`VectorCollectionView_Shaping.cpp`) and the CVS proxy (`CollectionViewSource_Shaping.cpp`) use the names
of their WPF counterparts so the logic can be read side by side with dotnet/wpf. Public API names are unaffected.

| WinUI internal | WPF counterpart | Role |
|---|---|---|
| `VectorCollectionView` (with shaping enabled) | `ListCollectionView` | The view over a vector; owns the engine and answers `ICollectionViewShaping`. Keeps its WinUI name (see decision 6). |
| `InternalList` (`VectorCollectionView::m_tpInternalList`) | `ListCollectionView.InternalList` | The shaped list the view presents. |
| `VectorCollectionView::RefreshOverride` | `ListCollectionView.RefreshOverride` | Full recompute: filter, stable sort, group. |
| `VectorCollectionView::RefreshOrDefer` | `CollectionView.RefreshOrDefer` | Recompute now, or once the last `DeferRefresh` closes; keeps currency. |
| `VectorCollectionView::ProcessCollectionChanged` / `ProcessCollectionChangedWithAdjustedIndex` | `ListCollectionView.ProcessCollectionChanged` / `ProcessCollectionChangedWithAdjustedIndex` | A source change: applied incrementally on a flat view, else a full recompute. |
| `CollectionViewSource::ApplyPropertiesToView` | `CollectionViewSource.ApplyPropertiesToView` | Gives a new view the CVS's rules. |
| `CollectionViewManager::GetViewRecord` | `ViewManager.GetViewRecord` | Picks the view class for a source. |
| `CollectionViewGroupInternal` | `MS.Internal.Data.CollectionViewGroupInternal` | A computed group: its subgroups (`Items`) or items, its leaves, `Key` / `Name` / `ItemCount` / `IsBottomLevel`; `LeafCountBefore` and `FindSubgroupPosition` play `LeafIndexFromItem` and `FindIndex`. |
| `CollectionViewGroupRoot` | `MS.Internal.Data.CollectionViewGroupRoot` | The top of the group tree: the group descriptions per level (`GetGroupNames` = `GroupNameFromItem`, `NamesMatch`, the group order), `AddLoading` (`AddToSubgroups` while loading), `FindLeaf`. |
| `PlanGroupedInsertion` / `ApplyGroupedInsertion` / `ApplyGroupedRemoval` | `CollectionViewGroupRoot.AddToSubgroups` / `RemoveFromSubgroups` | An item joins or leaves the group tree incrementally. |
| `VectorCollectionView::ProcessGroupedCollectionChanged` | `ListCollectionView.AddItemToGroups` / `RemoveItemFromGroups` | A source change on a grouped view. |
| `VectorCollectionView::PassesFilter` | `CollectionView.PassesFilter` | The `FilterRequested` handlers, else the `Filter` delegate. |
| `SortFieldComparer` | `MS.Internal.Data.SortFieldComparer` | Multi-key (or `CustomSort`) comparison in the view's `Culture`, used by the full sort, the incremental placement and the group order. |
| `GetDefaultViewImpl` (default-view table) | `CollectionViewSource.GetDefaultView` / `ViewManager` | The shared per-source view. |
| `VectorCollectionView::LiveShapingState` | `MS.Internal.Data.LiveShapingList` | The live paths (`SetLiveShapingProperties`) and one `LiveShapingItem` per source item, in source order. The view keeps its `InternalList`; WPF swaps in a `LiveShapingList`. |
| `VectorCollectionView::LiveShapingItem` / `LivePropertyObserver` | `MS.Internal.Data.LiveShapingItem` | The dirty flags of one item and a property path listener per live property (WPF binds a DP per property). |
| `VectorCollectionView::OnLivePropertyChanged` / `RestoreLiveShaping` | `LiveShapingList.OnItemPropertyChanged` / `ListCollectionView.RestoreLiveShaping` | Marks the item, then restores the marked items at the next dispatcher turn. |
| `FindSortedRange` | `LiveShapingBlock.FindPosition` | Binary search that skips marked items and reports the range an item is in order in. |

`VectorCollectionView`, `GroupedDataCollectionView`, `CollectionView` and `CollectionViewManager` are pre-existing
WinUI classes and keep their names.

## How controls consume it

- **ListView / GridView / ItemsControl.** They read `ICollectionView.CollectionGroups` when a `GroupStyle`
  is set (`ItemsControl_Partial.cpp:2658`, `ItemContainerGenerator_Partial.cpp:1978`,
  `ModernCollectionBasePanel_CacheManager_Partial.cpp:146`). The group header's DataContext is the computed group, so
  `{Binding Key}` works. When `ItemsSource` is a shaping view (`ICollectionViewShaping`), `ItemsControl` also listens
  to its `PropertyChanged("CollectionGroups")` and clears its containers as for a new `ItemsSource`
  (`ItemsControl::OnCollectionViewPropertyChanged`); the next measure re-registers the items host, which subscribes
  to the new `CollectionGroups` and re-evaluates `IsGrouping` and the group header layout, and the view's `Reset`
  that follows regenerates the items with or without groups (WPF: `ItemsControl.IsGrouping` follows the view's
  groups). Other `ICollectionView`s are not listened to, so their behavior is unchanged.
- **SemanticZoom.** The zoomed-out view reads the same `CollectionGroups`
  (`ListViewBase_Partial_SemanticZoomInformation.cpp:386`); a binding to `View.CollectionGroups` follows the
  property change.
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

- **Rows and groups.** TableView projects the view read-only. `CollectionGroups` become group rows, read directly
  from each `ICollectionViewGroup` (`Group`, `GroupItems`) by `TableViewRowFlattener`, in the view's order. TableView
  never reorders, filters or groups the rows itself.
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
- **Live shaping.** TableView never turns live shaping on; the app does, on its CVS (`IsLiveSortingRequested`, ...)
  or on the view (`ICollectionViewLiveShaping`). With empty `LiveSortingProperties` the view observes whatever the
  header sort wrote, so a header-sorted column stays sorted as its values change. A `CustomSortComparer` column sorts
  through `CustomSort`, which names no property: list the column's properties in `LiveSortingProperties` to keep it
  live. A live move arrives as a remove and an insert of the row; when the selected row moves, TableView re-selects it
  at its new index without raising `SelectionChanged` (the item is still in the view's `SourceCollection`, so it was
  moved, not deleted). Keyboard focus on a moved row is not carried along.
- **Plain collection.** TableView wraps it in an internal `CollectionViewSource` and shows that `View` (see "One
  engine" below), so a header click on a plain collection takes exactly the path above.
- **`ICollectionView` without `ICollectionViewShaping`.** An app's own view (for example the Toolkit's
  `AdvancedCollectionView`) or an `IsSourceGrouped` CVS view is not wrapped. The header click raises `Sorting` only,
  and the app sorts the view; TableView neither reorders the rows nor lights the arrow, as WPF's `DataGrid` sorts only
  a view that `CanSort`. A descriptor-less CVS `View` over a vector is not such a view: it is shapeable, so
  `table.ItemsSource = new CollectionViewSource { Source = items }.View` header-sorts with no opt-in, and the table
  keeps that `View` when the sort starts.
- **Group identity.** A CVS recomputes its group objects on every `Refresh` or source change, so TableView keys a
  group by its key **value** (the group's `Key` property, else `ICollectionViewGroup.Group`; scalars compare by type
  and value), not by the group object's address. Expansion state therefore survives a recompute: a collapsed group
  stays collapsed after a source change or `Refresh()`.
- **Sample.** `Samples\TableViewSampleApp`, page "CollectionViewSource shaping" (`CollectionViewShapingPage`):
  Task Manager-style flat data, grouped by `GroupDescriptions`, a filter box using `Filter`, `Refresh()` after a
  value update, and header sort through the view. The "Task Manager" page (`TaskManagerPage`) sets
  `IsLiveSortingRequested`, `IsLiveGroupingRequested` and `IsLiveFilteringRequested` (with `LiveFilteringProperties`
  for the searched properties) instead, so its timer only writes values and the sorted rows move by themselves.

### TableViewSource removed

The public `TableViewSource` (and its `TableViewPredicate`, `TableViewKeySelector` and
`TableViewGroupIdentitySelector` delegates) is removed. Apps shape data with `CollectionViewSource` and hand
TableView its `View`. A plain collection is wrapped in a `CollectionViewSource` of TableView's own, so it is sorted by
the same view code on a header click (see "One engine").

### Hierarchy: TableView.ItemKeyPath / ParentKeyPath (V1)

Tree rows have **no CVS or WPF equivalent**. It is not just one more descriptor: in tree mode sort applies per
sibling set and grouping buckets roots only, and `ICollectionViewGroup` has no notion of depth. Task Manager's
expandable Apps rows need it.

For V1 the relation is declared on TableView with two string property paths, `ItemKeyPath` and `ParentKeyPath`
(preview, like the rest of TableView). TableView nests the items of the view it shows: the internal view over a plain
collection, or the app's CVS `View`. The view's filter, sort and grouping run first, then the rows are nested:

- Siblings keep the view's order, so `SortDescriptions` sorts each sibling set.
- The CVS filter runs before nesting: a match whose parent is filtered out is shown as an orphan root (the former
  `TableViewSource` kept the ancestors of a match).
- With a grouped view, each root is placed in its group and keeps its subtree.
- Live shaping moves an item in the view; the tree is rebuilt from the view's new order. A rebuild reads every item,
  so it is coalesced: the first `ItemInserted`, `ItemChanged` or `Reset` of a dispatcher turn rebuilds at once, and an
  `ItemRemoved` (the first half of a move) or any later change in the turn rebuilds once at the next turn. A live
  tick that moves k rows costs at most two rebuilds, not 2k. Applying single changes to the tree without a rebuild is
  a follow-up.

Hierarchy on CVS itself (a `HierarchyDescription`, or option (b)) is future work; if it is approved, the relation
moves from TableView into the view and `ItemKeyPath` / `ParentKeyPath` become a shorthand for it.

### One engine: every ItemsSource goes through a view

TableView has one data-shaping engine: the framework `CollectionViewSource` view. Like WPF, where every
`ItemsControl` shows its `ItemsSource` through a view, TableView routes every `ItemsSource` through an
`ICollectionView`, and only the view sorts, filters and groups.

| `ItemsSource` | View TableView shows | Header sort |
|---|---|---|
| Plain collection (`IVector`, `IBindableVector`, `ObservableCollection<T>`, `List<T>`) | An internal `CollectionViewSource { Source = list }.View`. An enumerable-only source is read once into a vector first. | Writes the view's `SortDescriptions`, or its `CustomSort` for a `CustomSortComparer` column, under one `DeferRefresh`. |
| `cvs.View` (answers `ICollectionViewShaping`) | That view, unwrapped. | The same, on the app's view: the sort is the view's, and the app sees it in `cvs.SortDescriptions`. |
| `ICollectionView` without `ICollectionViewShaping` (an app's own view, an `IsSourceGrouped` view) | That view, unwrapped. | Raises `Sorting` only. The app sorts the view; TableView does not reorder the rows or light the arrow. |

The rows come from the view as follows (`TableViewRowFlattener`, TableView's one row flattener, in the role of WPF's
`ItemContainerGenerator`: it turns the view into the flat row list of the single `ItemsRepeater` and never sorts,
filters or groups; the view never flattens, because expansion state is per control and header rows are not items):

- **Flat view:** the repeater is bound to the view itself, so a flat view's fine-grained `Add` / `Remove` / `Replace`
  reach the rows unchanged. After a header sort on a plain `ObservableCollection`, an `Add` arrives as one `Add` at
  its sorted position, not a `Reset`.
- **Grouped view** (`CollectionGroups` is not null): the flattener reads each view group's `GroupItems` as they are
  and emits a header row (`GroupedEntry`) plus the group's data rows. A shaping view's regroup (one `Reset` of
  complete groups) is read at once; a group added or removed one at a time, and any other view's group changes
  (an `IsSourceGrouped` view clears and refills its groups), are read one dispatcher turn later, once the view has
  finished. When the view starts or stops grouping (`PropertyChanged("CollectionGroups")`), the same flattener
  switches between flat and grouped rows; `ItemsSource` is not reassigned. Expansion is keyed by the group's key
  value, so a collapsed group stays collapsed across a regroup and across grouping off and on.
- **Tree** (`ItemKeyPath` / `ParentKeyPath`): the view's items, in the view's order, are indexed by key and parent key
  (`ParentKeyIndex`) and nested by the flattener; under a grouped view each root is placed in the group that holds
  it. The tree is rebuilt when the view changes.

Row identity is the item's COM identity. A header sort keeps the selection on its item (the selected item is found
again after the view reorders) and restores body focus; `Sorting` / `Sorted`, the arrow reconcile and
`TableViewGroupInfo` / `ExpandGroup` / `CollapseGroup` work as before. A `GroupExpanding` handler that changes the
source runs inside the view's `DeferRefresh`, so the view regroups once, after the expansion.

#### Internal components

None of these are in public IDL or the XAML metadata provider.

| Component | Role |
|---|---|
| `CollectionViewSource` view / `ICollectionViewShaping` (framework) | The one engine, owned by the view (`VectorCollectionView`; the CVS is a proxy over it): sort, filter, group, `CustomSort`, `DeferRefresh`, incremental flat updates. |
| `TableViewRowFlattener` | The one row flattener (WPF `ItemContainerGenerator`'s role): turns the view's items, `CollectionGroups` and tree into flat, grouped or tree rows, and owns group and row expansion state. No shaping. |
| `ParentKeyIndex` | The key / parent-key index of the view's items that the flattener nests. Stays on TableView until hierarchy moves into the view. |
| `GroupedEntry` | A group header row: the group's identity, key, counts and expansion. |
| `RowMetadataProvider` | Per-row kind, level, identity and expansion for grouped and tree rows. |

Retired: `ShapedItemsSource` (TableView's own sort, filter, group and live-shaping engine, with `RowIdentity`,
`LiveShapingTracker` and `ShapingPipeline`), the `TableViewShapingSource` facade over it, and the comparer rank
adapter TableView used to sort a `CustomSortComparer` column. Later, `GroupedSourceAdapter`,
`HierarchicalSourceAdapter`, the `TableViewCollectionViewGroups` bridge and `ShapedGroup` were merged into
`TableViewRowFlattener`, which reads the view's groups directly.

#### Behavior changes from the retired engine

- A list that holds one object twice, or a `null` item, now sorts (a `null` item has a `null` key, which sorts first).
  The engine refused such a sort.
- Values compare as the view compares them (`CompareStringEx` with the user locale; numbers, `DateTime`, `TimeSpan`
  and `Boolean` by value; anything else equal). The engine's comparer also ordered enums and fell back to `ToString`.
- An edit to a sorted row's sort key moves the row only when the app turns live sorting on
  (`IsLiveSortingRequested`); otherwise the row moves on `Refresh()` (or a source `Reset`), as with any CVS view. The
  engine always marked such a row and re-placed it on the next collection change.
- An item of an app's grouped view that no group holds is not shown (the engine showed it in an unnamed group).

#### Dependency on the experimental CVS shaping API

TableView V1 requires the CVS shaping API. Its own header sort on a plain collection works only because the internal
view answers `ICollectionViewShaping`. That API is `Feature_ExperimentalApi`, which is compiled in for prerelease
builds (`MUX_PRERELEASE`, the default for dev and test builds, so every test configuration has it) and compiled out
of final-release builds. Today only the CVS members are gated (`ICollectionViewSourceFeature_ExperimentalApi`); a
CVS's `View` answers `ICollectionViewShaping` in every configuration, so TableView sorts either way. If the view
interface is gated too, or the API does not ship, a header click on a plain collection degrades to raising `Sorting`
only. TableView and the CVS shaping API therefore have to leave experimental together.

#### Follow-ups (not in this change)

- Hierarchy plus filter: keep the ancestors of a match (the view filters before the tree is built, so a match whose
  parent is filtered out is shown as a root; `ParentKeyIndex` and `TableViewRowFlattener` keep their filter overlay
  for this).
- Keep keyboard focus on a row a live view moves.

## Out of scope for the POC

- Key-selector delegates. The `CustomSort` comparer covers C++ and AOT callers.
- `GroupDescription` subclasses and `GroupNames` (explicit groups), an item in several groups (a name that is a
  collection), and nested groups in ListView / GridView (they show the first level).
- XAML-declared descriptors (needs DP-backed collection properties and type-table entries).
- `CanSort` / `CanFilter` / `CanGroup` on `ICollectionViewShaping`.
- Hierarchy on CVS (see "Hierarchy: TableView.ItemKeyPath / ParentKeyPath (V1)").
- `Move` as a single change (a live move is a remove and an insert), and coalescing a burst of source changes (live
  property changes are coalesced per dispatcher turn, as in WPF).
- WPF's insertion-sort path for a restore where most items are marked (`LiveSortingDensityThreshold`): every marked
  item is placed one by one, which is O(marked × log n) comparisons.
- Performance work: virtualization, hashed group lookup (WPF's name-to-group map). A full recompute is O(n log n),
  plus O(n × groups × levels) for grouping; an incremental grouped change is O(n) to find the item plus O(groups) per
  level.

## Known risks

- **Codegen.** Generated outputs were first hand-written, then regenerated with `dxaml\xcp\runcodegen.cmd`
  (`a78612b94`, and again for `ICollectionViewShaping`). See "Regeneration" below.
- **Flat/grouped switch.** Adding the first or removing the last `GroupDescription` keeps `View`; ListView, GridView
  and `ItemsControl` regenerate their containers as for a new `ItemsSource` (scroll position and realized containers
  are not kept, as after a `Reset`). A custom control that read `CollectionGroups` once per `ItemsSource` must listen
  to the view's `PropertyChanged("CollectionGroups")`. Changing `IsSourceGrouped` still gives the CVS a new `View`.
- **Reorder.** `ListViewBase` drag-reorder calls `RemoveAt` / `InsertAt` on the items source
  (`ListViewBase_Partial_Reorder.cpp:1498-1499`). Shaped views are read-only, the same as `IsSourceGrouped` views
  today, so set `CanReorderItems="False"`. WPF also blocks reorder on sorted views.
- **Full refresh cost.** `Refresh()`, a descriptor change and a source `Reset` recompute fully and raise `Reset`;
  selection and scroll position follow `Reset` semantics. Source `Add` / `Remove` / `Replace` are incremental on flat
  and grouped views.
- **Group objects.** A group is an internal `CollectionViewGroupInternal` that is also an `ICollectionViewGroup`;
  apps reach its `Key`, `ItemCount` and subgroups through bindings or `ICustomPropertyProvider`, which is why a group
  `CustomSort` gets the names. A public `CollectionViewGroup` class would be an API-review decision.
- **Default-view table.** `GetDefaultView` keeps a per-thread table of weak references; a view removes its own entry
  when it is destroyed.
- **Default-view lifetime.** WPF keeps a default view (and its rules) alive as long as its collection, because the
  collection's change event roots the view in one managed heap. Here the source can be native or managed and the
  rules can hold app delegates, so a native table holding the view or its rules while the source lives would form a
  cycle no GC can see. The table stays weak until the `GetDefaultView` API decision (drop it for V1, or redefine it).
- **Deferral lifetime.** A `DeferRefresh` deferral that is never closed keeps the view stale (WPF behaves the same).
  It holds the view (or, for a deferral taken on the CVS, the CVS) strongly until closed.
- **API shape.** Decide `GroupDescription` (abstract base) vs. `PropertyGroupDescription` before leaving experimental.
  Adding a base class later is breaking in WinRT; group `SortDescriptions` / `CustomSort` would move to it.
  `FilterRequested` vs. renaming the `Filter` delegate property so the event can take WPF's name is the other naming
  decision. `SortDescription` and `ListSortDirection` share simple names with
  WPF and the Toolkit, so apps that use both namespaces need aliases.
- **Feature gate in tests.** The new test assumes `Feature_ExperimentalApi` is enabled in the test configuration,
  like other experimental-API tests (for example `GridViewIntegrationTests.cpp:573`).
- **Live shaping cost and lifetime.** One property path listener per source item and live property, rebuilt on each
  full recompute (a header sort, `Refresh()`, a source `Reset`). The view walks them (and a group walks its key and
  its `PropertyChanged` handlers) in its reference tracker walk; they unsubscribe when an item leaves the source,
  when the features are turned off and when the view is destroyed. A restore after a turn in which most items
  changed costs one remove and one insert per moved item; TableView's tree mode rebuilds its tree at most twice for
  it.
- **Live moves and consumers.** A move is `ItemRemoved` + `ItemInserted`. ListView and GridView recycle the moved
  container, so selection and focus on it follow their remove semantics; TableView keeps the selection (see "Live
  shaping" in the TableView section) but not keyboard focus.

## Regeneration

Generated outputs were hand-edited to match the generator. Run `dxaml\xcp\runcodegen.cmd` and keep its output.

| File | Change | Must regenerate / verify |
|---|---|---|
| `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Data.cs` | Source of truth (hand-authored) | — |
| `dxaml/xcp/dxaml/idl/winrt/core/microsoft.ui.xaml.coretypes2.idl` | Enum, delegate, two classes, CVS experimental block | Yes. Also expect `IReference<ListSortDirection>` in the enum `declare` block if the generator emits it. |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/CollectionViewSource.g.h/.g.cpp` | Forwarder, QI, accessors, `Refresh` | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/SortDescription.g.h/.g.cpp` (new) | Full class + factory | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/PropertyGroupDescription.g.h/.g.cpp` (new) | Full class + factory | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/FilterEventArgs.g.h/.g.cpp` (new) | Event args class | Yes |
| `Indexes.g.h`, `StableXbfIndexes.g.h`, `StableXbfIndexMapping.g.cpp`, `StableEventIndexes.g.csv`, `EnumDefs.g.h` | `CollectionViewSource_FilterRequested` event index, `GetDefaultView` method index, the enum | Yes (appended entries only) |
| `dxaml/xcp/dxaml/lib/Factories.g.cpp` | 2 forward declarations + 2 activation entries | Yes (ordering) |
| `dxaml/xcp/dxaml/lib/synonyms.g.h` | 5 typedefs (with `ICollectionViewShaping` and `ICollectionViewLiveShaping`) | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/Aggregate.g.cpp` | 2 includes | Yes |
| `dxaml/xcp/components/metadata/*`, `dxaml/xcp/core/metadata/*`, XBF indexes | None expected (PropertyOnly members, `IsCreateableFromXAML = false` classes, enum excluded from type tables) | Verify no diff |
| `dxaml/xcp/dxaml/idl/winrt/main/microsoft.ui.xaml.idl` | None | Add `declare` entries only if MIDL does not emit `IVector`/`IIterable` of the descriptor types |
| `packaging/Intellisense/*` | None | Regenerate docs XML when promoting |

Hand-written files: `CollectionViewSource_Partial.h/.cpp` (`EnsureView`), `CollectionViewSource_Shaping.cpp`
(the CVS proxy: rule properties, `ApplyPropertiesToView`, `DeferRefresh`, descriptor factories),
`VectorCollectionView_Partial.h` and `VectorCollectionView_Shaping.cpp` (the engine and `ICollectionViewShaping`),
`PropertyGroupDescription_Partial.h` (group order), `CollectionViewLiveShapingProperties.h` (the `Live*Properties`
collections),
`CollectionViewManager.h/.cpp` (`GetViewRecord`), `CollectionViewRefreshDeferral.h`, and `wrtdxamlfoundation.vcxproj`.

## Test

`BindingIntegrationTests::CollectionViewSourceShaping`
(`dxaml/test/native/external/framework/dataBinding/tests`) covers:

- the unshaped view;
- `Filter` (the source is unchanged);
- descending sort by path;
- live refresh on source `Append` with current-item preservation;
- grouping (`CollectionGroups`, `Key`, `GroupItems`);
- returning to the unshaped view.

`BindingIntegrationTests::CollectionViewShapingInterface` covers `ICollectionViewShaping`: offered by a descriptor-less
flat view and not by an `IsSourceGrouped` view; starting shaping through it keeps the same view (one `Reset`, currency
kept); offered by flat and grouped shaped views; shared descriptor collections; sort, `Filter` and `Refresh()` through
the view reshaping the same view in place; and grouping through the view keeping that view.

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

WPF-parity additions, one test each in the same class:

- `CollectionViewSourceGroupedIncrementalUpdates`: on a sorted, grouped view an add joins its group, a new name
  inserts a group by its first item, removing a group's last item removes the group, a replace that changes group
  moves it, an in-place replace is one `ItemChanged`, each on the view, `CollectionGroups` and the group, with no
  `Reset`; the result matches `Refresh()`.
- `CollectionViewSourceMultiLevelGroups`: two descriptions nest groups (leaves in group order, `Items`, `ItemCount`,
  `IsBottomLevel`); adds and removes update the right level; the result matches `Refresh()`.
- `CollectionViewSourceGroupSort`: group `SortDescriptions` (`Name`, `Key`) and `CustomSort` order the groups, a new
  group is inserted in order, and they replace each other.
- `CollectionViewSourceCultureAndStringComparison`: `Culture` re-sorts (`sv-SE` vs `en-US`), an invalid tag throws;
  `Ordinal` vs `OrdinalIgnoreCase` group names.
- `CollectionViewShapingSourceCollection`, `CollectionViewSourceFilterEvent` (event alone, event over delegate,
  back to the delegate), `CollectionViewSourceGetDefaultView` (shared per source, shapes in place, not the CVS's view).

Live shaping (phase B), one test each in the same class. Each change is made, the dispatcher runs, and the view is
checked:

- `CollectionViewLiveSorting`: the `ICollectionViewLiveShaping` defaults, null rejected; no move without live sorting;
  `IsLiveSortingRequested` with empty `LiveSortingProperties` observes the sort property and an item that moved is
  one remove + insert while one still in order raises nothing; explicit `LiveSortingProperties` replace the default;
  three changes in one turn end in sorted order with no `Reset`; turning it off on the view stops it.
- `CollectionViewLiveFiltering`: no live filtering without `LiveFilteringProperties`; an item the filter rejects is
  removed and one it accepts is inserted at its sorted position; a change that keeps the result raises nothing.
- `CollectionViewLiveGrouping`: with the group descriptions' property observed an item moves to another group, an
  emptied group is removed, a new group is inserted; explicit `LiveGroupingProperties` replace the default.
- `CollectionViewLiveShapingObservedItems`: `PropertyChanged` handler counts per item: one per live property, none
  after remove, replace, source `Reset`, turning live shaping off or the CVS letting go of the view.
- `CollectionViewLiveShapingDeferRefresh`: live changes wait for the deferral, then are restored incrementally; with a
  rule change pending, closing recomputes once.

Grouping keeps the view (phase C):

- `BindingIntegrationTests::CollectionViewSourceGroupingKeepsView`: grouping on, a regroup by sort and by
  description (under `DeferRefresh`), grouping off and on again keep `View` and its `CollectionGroups` object; a
  switch raises `PropertyChanged("CollectionGroups")` before the view's `Reset`; a regroup raises one `Reset` on
  `CollectionGroups` (groups complete) and then one on the view; a source add stays incremental; ungrouping empties
  the group collection.
- `MoCoGroupingIntegrationTests::ListViewFollowsCollectionViewSourceGrouping` and
  `GridViewFollowsCollectionViewSourceGrouping` (`dxaml/test/native/external/controls/moco`): with the CVS `View` as
  `ItemsSource`, the control shows no headers, then three headers and `IsGrouping` after grouping on, follows a
  regroup, shows flat items after grouping off and groups again, all with the same `ItemsSource`.
- `TableView_DataBinding_APITests.VerifyCollectionViewGroupingToggleKeepsView`: TableView keeps the view as
  `ItemsSource`, shows group rows after grouping on, shows a regroup before the dispatcher runs with a collapsed group
  kept collapsed, shows flat rows after grouping off and groups again with the expansion kept.

`TableView_DataBinding_APITests` (`controls\dev\TableView\APITests`) covers the end-to-end story:

- `VerifyHeaderSortOnShapingCollectionViewSortsTheView`: header click, `ToggleSortDirection`, `SortByColumn` and
  `ClearSort` write `SortDescriptions`; the arrow follows a sort applied to the view; `Sorting` / `Sorted` are raised.
- `VerifyFilterAndRefreshOnShapingCollectionViewUpdateRows`: `Filter`, `Refresh()` after an edit, source add.
- `VerifyGroupDescriptionsOnShapingCollectionViewRenderGroups`: group rows keyed by value, sort within groups.
- `VerifyCollapsedCollectionViewGroupSurvivesRecompute`: a collapsed group stays collapsed after a source change and
  after `Refresh()`.
- `VerifyNestedCollectionViewGroupsRenderSubgroupHeaders`: two descriptions render a header per level (`Level` 0 and
  1); collapsing a group hides its subgroups; an add starts a subgroup.
- `VerifyCustomSortComparerColumnSortsShapingCollectionView`: a `CustomSortComparer` column sorts the view through
  `CustomSort` in both directions; a path column's sort replaces it; a throwing comparer leaves the previous sort,
  rows and arrow; `ClearSort` clears both.
- `VerifyHeaderSortOnDescriptorlessCollectionViewSourceView`: a header click on a plain CVS `View` writes
  `SortDescriptions` and the `View` stays the same object.
- `VerifyIncrementalShapingCollectionViewChangesUpdateRows`: add, remove and replace on a sorted, filtered view reach
  the rows with no `Reset`, and the selection stays on its item.
- `VerifyLiveSortingCollectionViewMovesRow`: with `IsLiveSortingRequested`, a header-sorted row whose sort value
  changes moves with one remove and one insert, no `Reset`, keeps its selection without `SelectionChanged`, and the
  arrow stays.
- `VerifyHeaderSortOnPlainCollectionSortsThroughInternalView`: a header click on a plain `ObservableCollection`
  sorts the rows, leaves the collection's order alone, and a later add and remove arrive as one `Add` and one
  `Remove` at their sorted positions.
- `VerifyHeaderSortOnAppCollectionViewRaisesSortingWithoutReordering`: an app's own `ICollectionView` gets `Sorting`
  and keeps its order and an unlit arrow.
- `VerifySelectionFollowsItemAcrossHeaderSort`: the selection follows its item through a sort and a clear.
- `TableView_Hierarchy_APITests`: `VerifyHeaderSortOnPlainListTreeKeepsListOrder` and
  `VerifyHeaderSortOnCollectionViewTreeSortsTheView` sort a tree over a plain list and over `cvs.View`.
- `TableView_ViewEngine_APITests` (`TableViewViewEngineTests`): random source operations, rule changes and refreshes
  (and, live, property edits) on flat, grouped and two-level views compared with a fresh view after each step;
  re-entrant changes from `VectorChanged` handlers and a `Filter`; incremental loading through a shaped view;
  `FilterRequested` after a GC with only `View` held; no leak through live listeners; group `ItemCount`
  notifications; currency before first / after last; exact `Int64` / `UInt64` keys; turning live sorting on;
  replaced views; rules on an `IsSourceGrouped` CVS; off-thread live changes; and TableView live sort, filter,
  group and tree, including at most a couple of tree rebuilds per live tick.

## Effort to productize

These are rough estimates for one engineer who knows dxaml.

| Item | Estimate |
|---|---|
| Build, fix, run codegen, test matrix (ListView, GridView, SemanticZoom, `IsSourceGrouped`, CLR sources) | 1 week |
| Live shaping: perf on large lists, insertion-sort path, focus on moved rows (the POC has live sorting, filtering and grouping) | 0.5–1 week |
| `GroupDescription` base decision (grouped incremental updates, multi-level groups, group order, `Culture`, `StringComparison`, `CustomSort` and `Converter` are in the POC) | 0.5 week |
| XAML-declarable descriptors (DP collections, type tables) | 1 week |
| View-level shaping interface: `CanSort` / `CanFilter` / `CanGroup`, API review (the POC interface exists) | 0.5–1 week |
| API review, docs, perf and back-compat tests | 1–2 weeks |
| **Total** | **≈ 3.5–6 engineer-weeks** |

TableView's own shaping engine is retired (see "One engine"); live shaping and grouped incremental updates now live
in the CVS view itself.
