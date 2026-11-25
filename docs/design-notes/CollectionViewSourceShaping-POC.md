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
// WPF's ListSortDirection and System.StringComparison, renamed (see "Naming").
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
enum CollectionViewSortDirection { Ascending, Descending };

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
enum CollectionViewStringComparison
{ CurrentCulture, CurrentCultureIgnoreCase, InvariantCulture, InvariantCultureIgnoreCase, Ordinal, OrdinalIgnoreCase };

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
delegate Boolean CollectionViewFilter(Object item);

// Negative, zero or positive as x sorts before, with or after y (WPF ListCollectionView.CustomSort).
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
delegate Int32 CollectionViewComparer(Object x, Object y);

// WPF's SortDescription (a struct there). Settable until a SortDescriptions collection holds it; then a change throws,
// through the setters and through the property system (markup, a binding).
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
runtimeclass SortDescription : Microsoft.UI.Xaml.DependencyObject
{
    SortDescription();
    SortDescription(String propertyName, CollectionViewSortDirection direction);
    String PropertyName;
    CollectionViewSortDirection Direction;
};

// WPF's System.ComponentModel.GroupDescription. Derive from it for custom grouping. As in WPF, GroupNameFromItem and
// NamesMatch are public, so any view can group by a description; they call the protected *Core overridables (the
// AutomationPeer pattern: GetName / GetNameCore).
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
unsealed runtimeclass GroupDescription : Microsoft.UI.Xaml.DependencyObject
{
    protected GroupDescription();
    IObservableVector<Object> GroupNames { get; };                     // read-only DependencyProperty: explicit groups,
                                                                       // first, in order, kept when empty
    IObservableVector<SortDescription> SortDescriptions { get; };      // group order (Name/Key, ItemCount)
    CollectionViewComparer CustomSort;                                 // compares two group names
    Object GroupNameFromItem(Object item, Int32 level, String culture);   // culture = view's Culture
    Boolean NamesMatch(Object groupName, Object itemName);
    overridable Object GroupNameFromItemCore(Object item, Int32 level, String culture);   // protected
    overridable Boolean NamesMatchCore(Object groupName, Object itemName);              // protected; default Object.Equals
    protected void OnPropertyChanged(PropertyChangedEventArgs e);                       // WPF: regroups the views using it
};

[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
runtimeclass PropertyGroupDescription : GroupDescription
{
    PropertyGroupDescription();
    PropertyGroupDescription(String propertyName);
    PropertyGroupDescription(String propertyName, IValueConverter converter);
    PropertyGroupDescription(String propertyName, IValueConverter converter, CollectionViewStringComparison stringComparison);
    String PropertyName;
    IValueConverter Converter;                                         // Convert(value, Object, level, culture), as in WPF
    CollectionViewStringComparison StringComparison;                   // Ordinal unless set
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
        IObservableVector<SortDescription> SortDescriptions { get; };     // read-only DependencyProperty: markup fills it
        IObservableVector<GroupDescription> GroupDescriptions { get; };   // read-only DependencyProperty
        CollectionViewComparer CustomSort;
        String Culture;                                    // DependencyProperty; BCP-47 tag; empty = user locale
        Boolean IsLiveSortingRequested;                    // DependencyProperties, as in WPF
        Boolean IsLiveFilteringRequested;
        Boolean IsLiveGroupingRequested;
        IObservableVector<String> LiveSortingProperties { get; };          // read-only DependencyProperties
        IObservableVector<String> LiveFilteringProperties { get; };
        IObservableVector<String> LiveGroupingProperties { get; };
        event FilterEventHandler Filter;                   // WPF's CollectionViewSource.Filter event
        void Refresh();
        Windows.Foundation.IClosable DeferRefresh();
        static DependencyProperty CultureProperty { get; };
        static DependencyProperty IsLiveSortingRequestedProperty { get; };
        static DependencyProperty IsLiveFilteringRequestedProperty { get; };
        static DependencyProperty IsLiveGroupingRequestedProperty { get; };
    }
};

// Optional capability of an ICollectionView, like ICollectionViewFactory or ISupportIncrementalLoading. The view a
// CollectionViewSource creates over a vector implements it, and so can an app's own view (see "Extensibility").
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
interface ICollectionViewShaping
{
    IObservableVector<SortDescription> SortDescriptions { get; };
    IObservableVector<GroupDescription> GroupDescriptions { get; };
    CollectionViewFilter Filter;                           // WPF ICollectionView.Filter
    void Refresh();
    Windows.Foundation.IClosable DeferRefresh();
    CollectionViewComparer CustomSort;
    Object SourceCollection { get; };                      // WPF ICollectionView.SourceCollection
    String Culture;                                        // WPF CollectionView.Culture, as a language tag
    Boolean CanSort { get; };                              // WPF ICollectionView.CanSort / CanFilter / CanGroup;
    Boolean CanFilter { get; };                            // true on the framework's view
    Boolean CanGroup { get; };
};

// WPF's CollectionViewGroup, as an interface the groups of a view implement: the computed groups of a shaping view
// and the groups of an IsSourceGrouped view (Name = the app's group object, always bottom level).
[contract(Microsoft.UI.Xaml.WinUIContract, 12)] [feature(Feature_ExperimentalApi)]
interface ICollectionViewGroup2
{
    Object Name { get; };
    IObservableVector<Object> Items { get; };              // subgroups, or the items at the bottom level
    Int32 ItemCount { get; };                              // leaves; the computed group raises PropertyChanged for it
    Boolean IsBottomLevel { get; };
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
    Windows.Foundation.Collections.IObservableVector<String> LiveGroupingProperties { get; };  // empty: PropertyGroupDescriptions'
};
```

### Changes in batch 3 (API review follow-up)

All of these change the experimental API only; the shipped `CollectionViewSource`, `ICollectionView`,
`ICollectionViewFactory` and `ICollectionViewGroup` are unchanged.

| Before | After | Why |
|---|---|---|
| `enum ListSortDirection` | `enum CollectionViewSortDirection` (same values) | Naming rule shared with `CollectionViewStringComparison` (see "Naming"). |
| `runtimeclass SortDescription` (immutable, `(String, ListSortDirection)` constructor only, not creatable from XAML) | `SortDescription : DependencyObject` with a default constructor, settable `PropertyName` / `Direction`, sealed once a `SortDescriptions` collection holds it | WPF's markup idiom. A change after sealing throws `InvalidOperationException`, as WPF's `SortDescription` does. |
| `PropertyGroupDescription` sealed, read-only, no base | `GroupDescription` (unsealed, protected constructor, `GroupNames`, `SortDescriptions`, `CustomSort`, overridable `GroupNameFromItem` / `NamesMatch`); `PropertyGroupDescription : GroupDescription` with a default constructor and settable `PropertyName`, `Converter`, `StringComparison` | WPF's base class, before leaving experimental (adding a base later is breaking). A property change regroups the views that use it, as WPF's `PropertyChanged` does. |
| `GroupDescriptions : IObservableVector<PropertyGroupDescription>` (CVS and `ICollectionViewShaping`) | `IObservableVector<GroupDescription>` | Follows the base class. |
| `CollectionViewSource.Filter` (delegate) + `event FilterRequested` | `event Filter` only; the delegate is the view's `ICollectionViewShaping.Filter` | WPF's names: the CVS has the event, the view has the predicate. The event name was taken only because the CVS also had the delegate. |
| `SortDescriptions`, `GroupDescriptions`, `Live*Properties` plain properties | read-only `DependencyProperty`s whose values are created on first use | `<CollectionViewSource.SortDescriptions>` etc. in markup. |
| `IsLive*Requested` plain properties | `DependencyProperty`s with `Is*RequestedProperty` statics | Settable in markup and bindable, as in WPF. |
| Group `Converter.Convert(value, Object, null, culture)` | `Convert(value, Object, level, culture)` | WPF passes the level as the converter parameter. |

| WPF | This POC | Note |
|---|---|---|
| `CollectionViewSource.SortDescriptions` (`SortDescriptionCollection` of `SortDescription` structs) | `IObservableVector<SortDescription>` of `SortDescription : DependencyObject`, a read-only `DependencyProperty` | Filled in markup (`<CollectionViewSource.SortDescriptions><SortDescription PropertyName="Name"/>`) or code. WinRT structs cannot be created and filled in markup, so the description is a `DependencyObject` (as WinUI's `Setter` and `Binding` are); like WPF's struct it is sealed once a `SortDescriptions` collection holds it, after which a change throws. It has no value equality: find one by its `PropertyName`. |
| `ListSortDirection` | `CollectionViewSortDirection` | Same values. Renamed with `CollectionViewStringComparison` (see "Naming"). |
| `CollectionViewSource.GroupDescriptions` (`ObservableCollection<GroupDescription>`) | `IObservableVector<GroupDescription>`, a read-only `DependencyProperty` | Markup or code. Several descriptions nest groups (below). |
| `ICollectionView.Filter` (`Predicate<object>`) | `ICollectionViewShaping.Filter` (`CollectionViewFilter`) | On the view, as in WPF; the CVS has no `Filter` property (WPF's does not either). Set on `cvs.View`, it is kept when the CVS replaces its view (a new `Source`). |
| `CollectionViewSource.Filter` **event** (`FilterEventHandler`, `FilterEventArgs.Item` / `Accepted`) | `CollectionViewSource.Filter` event (`FilterEventHandler`, `FilterEventArgs`) | Same name (it was `FilterRequested` while the CVS also had a `Filter` delegate property). As in WPF, `Accepted` starts `true`, each handler may change it, and adding or removing a handler refilters. **While it has handlers the event wins** over the view's `Filter` delegate (WPF replaces `view.Filter` with the event's wrapper); removing the last handler hands filtering back to the delegate. A CVS whose only rule is a handler shapes its view. It can be hooked in markup (`<CollectionViewSource Filter="OnFilter">`). |
| `ICollectionView.Refresh()` | `CollectionViewSource.Refresh()` and `ICollectionViewShaping.Refresh()` | `ICollectionView` is frozen, so the verb lives on CVS and on the opt-in view interface. |
| `ICollectionView.SortDescriptions` / `GroupDescriptions` / `Filter` | `ICollectionViewShaping` on the shaped `View` | `SortDescriptions` and `GroupDescriptions` are the CVS's objects: one set of descriptors, reachable from `ItemsSource` alone. |
| `CollectionViewGroup.Name` | `Key` and `Name` on the group object; `ICollectionViewGroup2.Name` | `Key` matches the `IGrouping<K,T>` idiom WinUI samples already use. |
| `ICollectionView.DeferRefresh()` → `IDisposable` | `DeferRefresh()` → `IClosable` on CVS and `ICollectionViewShaping` | Implemented. Nested deferrals count; changes made while one is open (descriptors, `Filter`, `CustomSort`, `Refresh`, source changes) recompute once, when the last closes. C# `using` and C++/CX `delete` close it. An unclosed deferral defers forever, as in WPF. |
| `ListCollectionView.CustomSort` (`IComparer`) | `CustomSort` property of type `CollectionViewComparer` on CVS and `ICollectionViewShaping` | Implemented. A delegate, not an interface, so a C++/WinRT lambda or a C# method group works and nothing needs reflection (AOT-safe). As in WPF, setting it clears `SortDescriptions`, and adding a `SortDescription` clears it. Ties keep source order. A comparer that fails fails the change and leaves the view's rows. |
| `PropertyGroupDescription.Converter` (`IValueConverter`) | `Converter` (settable; also a constructor parameter) | As in WPF, `Convert(value, typeof(object), level, culture)`: the parameter is the boxed level (0 for the first description) and the language is the view's `Culture` (empty when unset, where WPF passes a null `CultureInfo`). A computed bucket (for example a date range) needs no model property. |
| `PropertyGroupDescription.StringComparison` (`System.StringComparison`, default `Ordinal`) | `StringComparison` of type `CollectionViewStringComparison` (settable; also a constructor parameter) | How string group names match (`String.Equals(name, name, comparison)`); other names use value equality. The group keeps the name of its first item. Default `Ordinal`. |
| `GroupDescription.SortDescriptions` / `CustomSort` (WPF 4.5 group sorting) | `GroupDescription.SortDescriptions` and `CustomSort` | Orders the groups at that level: `SortDescriptions` by a group property (`Name`/`Key`, `ItemCount`), `CustomSort` by comparing **two group names** (WPF passes `CollectionViewGroup`s; WinUI has no public group class, so the names are what an app can use). As in WPF they replace each other, a change regroups the views that use the description, a group ordered by its name or `CustomSort` is placed when created, and without a group order groups follow their first item. A group order on a property that changes with the items (`ItemCount`) is kept: groups are ordered once their items are known, and a change that leaves them out of order recomputes the view (one `Reset`; WPF places a group when it is created and does not move it). |
| `GroupDescription` (abstract), `GroupNameFromItem(item, level, culture)`, `NamesMatch(groupName, itemName)` | `GroupDescription` (unsealed, protected constructor), public `GroupNameFromItem(Object, Int32, String culture)` and `NamesMatch`, which call the protected overridables `GroupNameFromItemCore` and `NamesMatchCore` | As in WPF the methods are public, so any view (an app's `ICollectionViewShaping` view too) can ask a description for an item's group. WinRT overridables are protected, so an app overrides the `...Core` methods (`protected override object GroupNameFromItemCore(...)`), the `AutomationPeer.GetName` / `GetNameCore` pattern. The view calls them through the overrides interface, so a derived class decides each item's group at each level; its group names have no hash key, so they are matched with its `NamesMatch` (WPF looks them up in its name map first). A base `GroupDescription` that does not override `GroupNameFromItem` puts every item in a group named null (WPF's is abstract). `PropertyGroupDescription` is read natively, without a call through the ABI. A derived class whose own logic changes calls the protected `OnPropertyChanged`, which regroups the views that use it, as WPF's does. |
| `GroupDescription.GroupNames` (`ObservableCollection<object>`) | `GroupDescription.GroupNames` (`IObservableVector<Object>`) | Explicit groups: at each level they come first, in `GroupNames` order, exist when empty and stay when they empty; other groups follow. A change regroups. A read-only `DependencyProperty`, so markup fills it (`<PropertyGroupDescription.GroupNames><x:Int32>3</x:Int32>...`), as in WPF. |
| `CollectionView.Culture` / `CollectionViewSource.Culture` (`CultureInfo`) | `Culture` (`String`, a BCP-47 language tag): a `DependencyProperty` on CVS (`CultureProperty`), a property on `ICollectionViewShaping` | Implemented. Strings sort with `CompareStringEx` in that locale; empty means the user's locale (WPF falls back to the element's `xml:lang`). A `String` because `CultureInfo` is not a WinRT type; an invalid tag throws `ArgumentException` from the setter, and is undone (logged as a binding failure) when it comes from markup or a binding. As in WPF the CVS's value is its own: it is applied to each view the CVS creates and to `View` when it changes, and a `Culture` set on the view directly stays the view's. Changing it re-sorts. Group-name matching with `CurrentCulture*` uses the user's locale, as WPF uses `CultureInfo.CurrentCulture`. |
| `ICollectionView.SourceCollection` (`IEnumerable`) | `ICollectionViewShaping.SourceCollection` (`Object`) | Implemented: the collection the view was created over (the CVS's `Source`, not a wrapper). `ICollectionView` is frozen, so it is on the shaping interface. |
| `CollectionViewSource.GetDefaultView(object)` (`ViewManager` default view) | Not added | Skipped by choice: WinUI controls don't use default views; WPF's lifetime isn't GC-safe with native sources (see "Default view lifetime"); use `CollectionViewSource`. Can be added later if `ItemsControl` gains default-view support. |
| `ListCollectionView` fine-grained `Add` / `Remove` / `Replace` | Incremental updates on **flat and grouped** shaped views | Implemented for sort + filter + `CustomSort`, with or without groups. See "Source changes". |
| `ICollectionView.Groups` (null while not grouping), the view kept when grouping starts or stops, `ItemsControl.IsGrouping` following it | `ICollectionView.CollectionGroups` of a shaping view: null while flat, else **one** `IObservableVector` for the view's lifetime; the view raises `PropertyChanged("CollectionGroups")` and then a `Reset` when grouping starts or stops; ListView, GridView, `ItemsControl` and TableView regroup the same `ItemsSource` | Implemented. A regroup is one `Reset` of `CollectionGroups`, raised once the groups are complete, then one `Reset` of the view (WPF raises `Reset` on `Groups`). WinRT `ICollectionView` has no `GroupDescriptions`, so the null/non-null switch plus `PropertyChanged` is the signal (see "Grouping keeps the view"). |
| `ListCollectionView.AddItemToGroups` / `RemoveItemFromGroups` | Grouped incremental updates | Implemented. An item joins its bottom-level group at its sorted position; a group it starts is inserted among its siblings (by the group order, else before the first sibling whose first item sorts after it, as WPF's `FindIndex` with a seed); a group it empties is removed. |
| Several `GroupDescriptions` (nested `CollectionViewGroup`s, `IsBottomLevel`, `Items`, `ItemCount`) | Multi-level groups | Implemented. Each group is an `ICollectionViewGroup` whose `GroupItems` are its subgroups (WPF `Items`) above the bottom level, else its items. `CollectionGroups` and the view's items stay single level (the view enumerates leaves, as WPF's does), so ListView and GridView show the first level with all its leaves; TableView renders every level. Bindings see `Key`, `Name`, `Items`, `ItemCount` and `IsBottomLevel` on a group, and code reads them through `ICollectionViewGroup2` (WPF's `CollectionViewGroup` members). |
| `ICollectionViewLiveShaping` (implemented by `ListCollectionView`) | `ICollectionViewLiveShaping`, implemented by the view a CVS creates over a vector | Implemented; see "Live shaping". A separate interface, as in WPF, rather than members on `ICollectionViewShaping`. |
| `CanChangeLiveSorting` / `CanChangeLiveFiltering` / `CanChangeLiveGrouping` | Same names | Always `true`, as on `ListCollectionView`. |
| `IsLiveSorting` / `IsLiveFiltering` / `IsLiveGrouping` (`bool?`) | Same names, `IReference<Boolean>` | WPF's `bool?` is WinRT's `IReference<Boolean>` (C# sees `bool?`). As on `ListCollectionView` the value is never null and setting null throws (`ArgumentNullException` in WPF, `E_INVALIDARG` here); the nullable type is kept so a view that cannot tell (WPF's `null`) can implement the interface. Default `false`. |
| `LiveSortingProperties` / `LiveFilteringProperties` / `LiveGroupingProperties` (`ObservableCollection<string>`) | Same names, `IObservableVector<String>` | Property paths. Empty sorting or grouping lists mean the `SortDescriptions`' or the `PropertyGroupDescription`s' properties (none with a `CustomSort`); filtering observes only the listed paths, as in WPF. A change rebuilds what is observed. |
| `CollectionViewSource.IsLiveSortingRequested` / `IsLiveFilteringRequested` / `IsLiveGroupingRequested` | Same names, `DependencyProperty`s | Settable in markup and bindable, as in WPF. Applied to `View` when it is created and whenever they change, as WPF's `ApplyPropertiesToView` copies them; the CVS's own `Live*Properties` are copied into the view only while their feature is requested. |
| `CollectionViewSource.LiveSortingProperties` / `LiveFilteringProperties` / `LiveGroupingProperties` | Same names, read-only `DependencyProperty`s | Filled in markup (`<x:String>`) or code. A change is applied to `View` (WPF `OnForwardedCollectionChanged`). |
| `CollectionViewSource.CanChangeLive*` / `IsLive*` (read-only DPs mirroring the view) | Not added | Kept out after review: they only mirror `View`'s values (`ICollectionViewLiveShaping` on `View`, which is always the CVS's own view here and always `true` for `CanChangeLive*`), and WPF uses them mostly for triggers, which WinUI does not have. Six more read-only properties (and DPs) for no new state; read them from `View`. |
| `LiveShapingList` / `LiveShapingItem` (a binding per item and live property) | `VectorCollectionView::LiveShapingState` / `LiveShapingItem` / `LivePropertyObserver` | Each observer is a property path listener of the binding engine (a `{Binding}`'s), so `INotifyPropertyChanged` items (C# and C++), `ICustomPropertyProvider` items, `DependencyObject` properties and nested paths all work. |

### WPF parity pass

WPF parity is the default, not a goal at any cost. Each remaining WPF member was decided as **Match** (same name and
behavior), **Deviate** (a better design for WinUI/WinRT, with the reason) or **Skip** (no sound use in WinUI).

| WPF | Decision | WinUI | Reason |
|---|---|---|---|
| `ICollectionView.CanSort` / `CanFilter` / `CanGroup` | Match | `ICollectionViewShaping.CanSort` / `CanFilter` / `CanGroup`; `true` on the framework's view | Consumers ported from WPF (DataGrid) check them; TableView sorts through the view only when `CanSort` (else it raises `Sorting` only, as for a view without the interface). |
| `CollectionViewSource.GetDefaultView(object)` / `IsDefaultView(ICollectionView)` | Skip | `CollectionViewSource` | Skipped by choice: WinUI controls don't use default views; WPF's lifetime isn't GC-safe with native sources (see "Default view lifetime"); use `CollectionViewSource`. Can be added later if `ItemsControl` gains default-view support. |
| `CollectionViewSource.Culture` (DP) | Match | `Culture` `DependencyProperty` (`CultureProperty`), a `String` | Bindable and settable in markup as in WPF. The CVS keeps its own value and applies it to its views (it used to forward to `View`). |
| `CollectionViewGroup` (abstract class: `Name`, `Items`, `ItemCount`, `IsBottomLevel`, `PropertyChanged`) | Deviate | `ICollectionViewGroup2` on every group, plus `INotifyPropertyChanged` on computed groups | An interface: groups are created by views, a public composable class would add a framework type and an ABI hop per member for no app use, and the name `CollectionViewGroup` is the framework's internal `IsSourceGrouped` group class. |
| `GroupDescription.GroupNameFromItem` returning an `ICollection` (item in several groups) | Match (behavior), Deviate (cost) | A group name that is iterable (`IBindableIterable`, `IIterable<Object>`, `IIterable<String>`; a string is not) puts the item in a group for each element; an empty collection in none | WPF tracks each item's groups for incremental changes; here, while any item is in several groups, a source change or live restore on the grouped view recomputes it (one `Reset`). Tags-style grouping is rare and the incremental bookkeeping would cost every grouped view. |
| `GroupDescription.OnPropertyChanged` / `PropertyChanged` | Match (method), Skip (event) | `protected void OnPropertyChanged(PropertyChangedEventArgs)` regroups the views that use the description | A subclass whose own state changes its grouping announces it, as in WPF. Views are told internally, so no public event is added (WPF's is protected too). |
| `ICollectionView.Contains`, `IsEmpty`; `CollectionView.Count`, `GetItemAt`, `IndexOf` | Skip | `IVector.IndexOf` / `GetAt` / `Size` | WinRT's `ICollectionView` is an `IObservableVector`: `Contains` is `IndexOf`'s `found`, `IsEmpty` is `Size == 0`. |
| `PropertyGroupDescription.CompareNameAscending` / `CompareNameDescending` | Skip | `GroupDescription.SortDescriptions.Add(new SortDescription("Name", ...))` | The sort on `Name` reads each group's key once and orders groups by binary search; a static comparer delegate would be slower and adds statics the codegen has no precedent for on an instance class. |
| `ListCollectionView.GroupBySelector` | Skip | A `GroupDescription` subclass | Chooses a description per group; a subclass covers custom grouping without a delegate call per group, and no partner needs it. |
| `CollectionViewSource.CanChangeLive*` / `IsLive*` mirrors | Skip | Read `View`'s `ICollectionViewLiveShaping` | They only mirror `View` (see above). |
| `CollectionView.Comparer`, `NeedsRefresh`, `IsInUse`, `DetachFromSourceCollection`, `IItemProperties` | Skip | — | Members of WPF's public view class; WinUI has no public view class, and none has a consumer. |
| `IEditableCollectionView`, `IEditableCollectionViewAddNewItem`, `NewItemPlaceholder`, `IEditableObject` | Missing by choice | — | TableView doesn't edit; can be added later as a separate optional interface on the view (as WPF's is separate from `ICollectionView`) without breaking changes. |

## Usage

```xml
<Page.Resources>
    <!-- WPF's markup: the rules on the CollectionViewSource, Source set by code (or a binding). -->
    <CollectionViewSource x:Name="PeopleSource" Filter="OnPeopleFilter" IsLiveSortingRequested="True">
        <CollectionViewSource.SortDescriptions>
            <SortDescription PropertyName="LastName" />
            <SortDescription PropertyName="FirstName" />
        </CollectionViewSource.SortDescriptions>
        <CollectionViewSource.GroupDescriptions>
            <PropertyGroupDescription PropertyName="Department" />
        </CollectionViewSource.GroupDescriptions>
    </CollectionViewSource>
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

void OnPeopleFilter(object sender, FilterEventArgs e) => e.Accepted = ((Person)e.Item).IsActive;

// The same rules in code:
PeopleSource.SortDescriptions.Add(new SortDescription("LastName", CollectionViewSortDirection.Ascending));
PeopleSource.GroupDescriptions.Add(new PropertyGroupDescription("Department"));
((ICollectionViewShaping)PeopleSource.View).Filter = item => ((Person)item).IsActive;   // the view's predicate

people.Add(new Person { /* ... */ });   // View refreshes automatically
person.LastName = "Adams";              // moves the row: live sorting is on (LastName is a sort property)
```

A custom grouping derives from `GroupDescription`, as in WPF:

```csharp
sealed class InitialGroupDescription : GroupDescription
{
    protected override object GroupNameFromItemCore(object item, int level, string culture) => ((Person)item).LastName[..1];
    protected override bool NamesMatchCore(object groupName, object itemName) =>
        string.Equals((string)groupName, (string)itemName, StringComparison.CurrentCultureIgnoreCase);
}
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
  set on such a CVS (a description, `CustomSort` or a `Filter` event handler) throws
  `InvalidOperationException` (`E_ILLEGAL_METHOD_CALL`) instead of being ignored, when the rule is added or when the
  view is created with it. CVSs without rules are unaffected.
- **Pipeline.** Filter, then stable multi-key sort, then group, level by level. Without a group order
  (`GroupDescription.SortDescriptions` / `CustomSort`), groups appear in first-appearance order of the sorted
  items (WPF behavior).
- **Comparison.** Strings use `CompareStringEx` with the view's `Culture`, else the user locale. Numeric types compare as
  numbers exactly: two integers (including `Int64` and `UInt64`), and an integer against a `double` (no rounding above 2^53), with NaN before every other
  number (as `Double.CompareTo`); `DateTime`, `TimeSpan` and `Boolean` are also ordered. Keys of different kinds
  keep one total order, so a full sort and an incremental insert agree: `null`, then `Boolean`, numbers, `DateTime`,
  `TimeSpan`, strings, and last the values that cannot be ordered (custom objects, `Guid`, ...), which compare equal
  and so keep their source order. WPF's `Comparer.Default` throws `ArgumentException` on keys of different types
  instead (deviation: WinRT values have no `IComparable`, and a view that throws on a mixed column is not useful).
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
  currency, realized containers, selection and the UIA tree survive. The view keeps one source index and the
  decoded sort keys per output item, so `Remove` is O(n) and `Add` is O(n) plus O(log n) key compares (no reads). On a **grouped** view the same changes are
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
- **Currency** (WPF `ListCollectionView`; `VectorCollectionView_Currency.cpp`). An unshaped view keeps
  `CollectionView`'s contract. On a shaped view:
  - A recompute (a rule change, `Refresh()`, a source `Reset`, grouping on or off, closing `DeferRefresh`) raises one
    non-cancelable `CurrentChanging` before it recomputes, places the currency before its `Reset` is raised (so a
    `VectorChanged` handler reads the new `CurrentItem` / `CurrentPosition`), then raises the changed currency
    properties and `CurrentChanged` once. Currency that was before the first item or after the last stays there; a
    current item is followed if it is still in the view, otherwise currency moves to the first item; an empty result is
    before the first. So a sort with nothing current (a synchronized ListView with nothing selected) selects nothing.
    A handler of the `Reset` that moves the currency (a synchronized `Selector` clears a selection the `Reset` moved) is
    overruled once, with a non-cancelable `CurrentChanging`, so the selection follows the item, as before.
  - A live move (`ItemRemoved` + `ItemInserted`) keeps currency on the moved item and raises only
    `PropertyChanged("CurrentPosition")`, as WPF's `Move`; an item live filtering drops moves currency to the item now
    at its place, or the last, with one non-cancelable `CurrentChanging` / `CurrentChanged`, as a removal does.
  - A source `Replace` that keeps the current item's place makes the replacement current (WPF), instead of
    `CollectionView`'s move to the first item. A `Remove` of the current item moves currency to the item now at its
    place, or the last, in both (unchanged).
  - A rebuilt `View` starts at the first item, as today (WPF `ViewRecord.InitializeView`).
- **Reentrancy.** App code runs inside the engine (filters, `Filter` event handlers, property getters, converters, `GroupDescription` overrides,
  comparers) and while it raises its changes (every `VectorChanged` / `PropertyChanged` handler). A rule change,
  `Refresh()` or source change made from there is not dropped: it is recorded and the view recomputes once that work
  unwinds, then raises its own `Reset` (WPF refreshes re-entrantly; neither throws). A handler that changes the view
  on every change of it would never finish; after 8 rounds the view stops with `E_ILLEGAL_METHOD_CALL` and a message
  (WPF overflows the stack), and recomputes on the next change or `Refresh()`.
- **Exceptions in app code.** All the app code a change needs (filter, `Filter` event, getters, converters,
  comparers, `GroupDescription` overrides) runs before the view raises anything for it, so a failure never leaves a
  removal without its insertion; a grouped change plans the insertion for the tree as the removal will leave it. A
  handler of a change the view (or a group's `ItemCount`) raises that throws does not stop the view either: the change
  is finished and the failure is returned afterwards.
  - Called by the app (`Refresh()`, a rule change, closing `DeferRefresh`, `IsLive*`): the exception propagates to
    the caller, as in WPF. A recompute that fails keeps the previous output, which recomputes on the next change.
  - A source change: the view is one of the source's listeners, so the failure is not returned to the source (that
    would stop its other listeners and throw from the app's `Add`): the change the app code failed on is applied by a
    recompute (one `Reset`); if that fails too, the view keeps its output and recomputes on the next change. The
    failure is reported to `Application.UnhandledException` with the app's error info (its message and stack), as
    for app code a dispatcher callback runs; unhandled, it ends the app. WPF lets it propagate from
    `CollectionChanged`. Only a failure that carries error info (an exception app code threw) is reported; a bare
    `HRESULT` from a framework listener of the view is contained without a report (a `CollectionViewContainedFailure`
    TraceLogging event records it). "Carries error info" is a heuristic: a framework listener that originates error
    info is reported too. The report runs inside the
    source's change notification, so an `UnhandledException` handler that changes the same source re-enters it (an
    `ObservableCollection` throws). `E_OUTOFMEMORY` and `RPC_E_WRONG_THREAD` are not contained: they stop the change.
  - A live restore (a dispatcher callback): the same; the items placed before the failure stay placed.
- **Incremental loading.** A shaped view forwards `HasMoreItems` and `LoadMoreItemsAsync` to the collection it shapes
  when that implements `ISupportIncrementalLoading`; the loaded items arrive as source changes and are placed by the
  rules like any other. A filter that rejects the loaded items would make a consumer load until the source is
  drained, so after 10 loads in a row that grew the source but not the view, `HasMoreItems` is false until the view
  grows or the app starts over (a rule change, `Refresh()`, a new source, or the source shrinking, as when it is cleared
  for a new query; a source `Reset` that only adds, as an AddRange-style collection raises for each load, does not).
  A filter that matches less than once in 10
  pages stops early; the source's own `HasMoreItems` is unaffected.
  `LoadMoreItemsResult.Count` is the source's count, not the view's.
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
  - A source change made while a live sort edit waits for its restore is applied incrementally: the binary search
    skips the edited items, whose places are stale (WPF's `FindPosition`), and the restore then places them.
  - Moves keep the realized rows, keyboard focus, scroll position and the UIA tree, so the restore prefers them to a
    `Reset` by cost: a move costs two vector shifts (about 2 ns a row each at 100k items since P-2, estimated; about 14 ns
    measured before it, fre) and two raised changes
    (about 30 µs each); a flat re-sort with a `Reset` about 1.4 µs a row, a grouped recompute about 13 µs a row. While the k
    changed items' moves cost less than twice a `Reset`, or fit a budget of 50 ms, each is moved. Otherwise a flat
    view re-sorts once on its cached keys (reading only the changed items) and raises the fewest moves that reach the
    new order (the items outside the longest run still in order; none when the order did not change), or one `Reset`
    if those still cost more; a grouped view recomputes. At 100k rows that is about 100 moves before a `Reset`, at 10k
    about 150, at 2k about 430, so a Task Manager-size view moves rows and resets only for a mass change. WPF
    re-sorts with a `Reset` once 80% of the items changed (`LiveSortingDensityThreshold`) and insertion-sorts below.
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
3. **WPF's split: the view has the `Filter` predicate, the CVS has the `Filter` event.**
   - `ICollectionViewShaping.Filter` is a `CollectionViewFilter` delegate (WPF's `ICollectionView.Filter`, the
     Toolkit's `AdvancedCollectionView.Filter`): one well-defined result, no reflection, AOT-safe.
   - `CollectionViewSource.Filter` is the event (`FilterEventHandler`, `FilterEventArgs`), as in WPF; markup can hook
     it. An earlier version also had a `Filter` delegate on the CVS and so named the event `FilterRequested`; the
     delegate was dropped so the event could take WPF's name (WinRT, and the C# projection, cannot have a property
     and an event of one name on a class).
   - Precedent for an experimental framework delegate with a return value: `DataTemplateElementFactory`
     (`Microsoft.UI.Xaml.cs:3788-3792`, `microsoft.ui.xaml.controls.controls.idl:452-455`), stored in a `TrackerPtr`
     (`DataTemplate_Partial.h:30`).
4. **Markup-settable members are `DependencyProperty`s; the rest stay `PropertyKind.PropertyOnly`.** The XAML parser
   sets only properties in the type tables. `SortDescriptions`, `GroupDescriptions` and `Live*Properties` are
   read-only, DXaml-only `DependencyProperty`s whose collection is created on first read (the default-value path
   `Hub.SectionHeaders` uses, `DependencyProperty.cpp`), so `<CollectionViewSource.SortDescriptions>` fills them as
   `<ItemsControl.GroupStyle>` fills `GroupStyle`; the string collections take `<x:String>` items through
   `IUntypedVector`. `IsLive*Requested` are ordinary `DependencyProperty`s with public identifiers, as in WPF.
   `CustomSort` and `Culture` stay `PropertyOnly` (a delegate is not set in markup; see "Naming" for `Culture`). The
   XBF type, property and event tables were appended to by `runcodegen` (no reordering). `CollectionViewSource_Filter`
   is event 340, the index `FilterRequested` had in batches 1-2 (it never shipped, so it was reused, not tombstoned);
   `GroupDescription_GroupNames` is property 2490 (a compiled-XAML test page loads it from XBF). A `SortDescription`
   held by a collection is sealed in the property system too: a markup or binding change to it, or an invalid
   `Direction` or `StringComparison`, is undone. Only the effective value is restored, so the binding, style or
   ThemeResource stays attached (as when a binding's conversion fails); the rejection is logged as a binding failure
   (`DebugSettings.BindingFailed`). A TwoWay binding does not write the restored value back: its source keeps the
   rejected value, and the next valid value flows as usual.
5. **Descriptors are `DependencyObject`s, like `AutomationAnnotation` and `Setter`.** A default constructor plus a
   convenience constructor, settable properties, creatable from markup. They are UI-thread objects (the free-threaded
   immutable classes they replace could not be created in markup). `GroupDescription` copies `NavigationTransitionInfo`
   (an abstract `DependencyObject` with a protected constructor and protected overridables, called through the
   generated `...Protected` helpers). Whole-class feature gating copies the generated
   `InputValidationContext.g.h:14-41` / `.g.cpp:24-37` pattern. `[Optional]` string constructor parameters follow
   `PropertyChangedEventArgs` (`Microsoft.UI.Xaml.Data.cs:57`). The enums are in the type tables (markup parses
   `Direction="Descending"`), experimental.
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

### Naming

- **Enums.** WPF's names would be `ListSortDirection` and `StringComparison`. Both collide with .NET types an app's
  C# files import anyway: `System.StringComparison` (every file has `using System;`) and
  `System.ComponentModel.ListSortDirection` (in the base library, not WPF-only; every view model with
  `INotifyPropertyChanged` has `using System.ComponentModel;`). A same-named WinUI enum makes the simple name
  ambiguous (CS0104) in exactly the files that shape data. So both take the `CollectionView` prefix the other new
  Data types already use (`CollectionViewFilter`, `CollectionViewComparer`): `CollectionViewSortDirection`,
  `CollectionViewStringComparison`. Values match WPF's. The type names WPF has only in WPF assemblies
  (`SortDescription`, `GroupDescription`, `PropertyGroupDescription`, `FilterEventArgs`) keep WPF's names: a WinUI
  app does not reference them.
- **TableView's `SortDirection`** (`None`, `Ascending`, `Descending`; `MUX_PREVIEW`) stays separate. It is a column's
  sort *state*, which needs a "not sorted" value; a `SortDescription` always has a direction. WPF models the state as
  `ListSortDirection?`; the WinRT equivalent, an `IReference<CollectionViewSortDirection>` dependency property, has
  no precedent in the controls, whose metadata provider would have to parse a nullable enum from markup, and
  `TableViewColumn.SortDirection` is set in markup today. TableView maps between the two in one place
  (`TableView_Sort.cpp`).
- **`ICollectionViewShaping`, not `ICollectionView2`.** The `2` suffix in WinUI names the next version of the same
  contract, which every implementer is expected to take on (`IUIElement2`-style; for an app-implemented interface,
  the next version a caller may require). `ICollectionView` is implemented by apps and toolkits; shaping is an
  optional capability that a paged or virtualized view may not have, like `ISupportIncrementalLoading` and
  `ICollectionViewFactory`, which are named for what they add. A consumer asks for it with a QI and falls back
  (TableView raises `Sorting` only). So the capability name stays; it needs an API-council naming sign-off.
  `ICollectionViewLiveShaping` keeps WPF's name.
- **`Culture`** stays a `String` (`CultureInfo` is not a WinRT type). On the CVS it is a `DependencyProperty`, as in
  WPF: the setter throws on an invalid tag, and markup or a binding that sets one has it undone and logged (a
  `DependencyProperty` set from markup or a binding cannot report the error at a setter).

### Extensibility: views other than the framework's

- **Implementing the interfaces.** `ICollectionViewShaping` and `ICollectionViewLiveShaping` are not `exclusiveto`;
  an app or toolkit view can implement them next to `ICollectionView`. Consumers use only the interfaces: TableView
  sorts any `ItemsSource` that answers `ICollectionViewShaping` by writing its `SortDescriptions` (or `CustomSort`)
  under one `DeferRefresh`, and lights the arrow for a sort the view's owner writes there; `ItemsControl` regroups when
  such a view raises `PropertyChanged("CollectionGroups")`. `TableView_DataBinding_APITests`
  `VerifyHeaderSortOnAppShapingViewWritesItsSortDescriptions` runs TableView over a C# view that sorts itself.
- **The contract a third-party view follows**: changes as `IObservableVector` changes (a move is a remove and an
  insert, a recompute a `Reset`); `CollectionGroups` null while flat, and `PropertyChanged("CollectionGroups")` then a
  `Reset` when grouping starts or stops; nested groups as `ICollectionViewGroup`s whose `GroupItems` are subgroups
  above the bottom level; `DeferRefresh` deferrals that tolerate `Close` from an error path.
- **Custom grouping** derives from `GroupDescription`; **custom filtering and sorting** use the `Filter` delegate or
  event and `CustomSort`. These cover WPF's `ListCollectionView` overrides of `PassesFilter` and `Compare`.
- **Future:** a public, composable `ListCollectionView`-like class (overriding `RefreshOverride`,
  `ProcessCollectionChanged`) needs a new framework view type, per-item ABI calls for every override, and a review of
  composability; it is not in this change. A view that implements the interface says which rules it applies with
  `CanSort` / `CanFilter` / `CanGroup`, and its groups can implement `ICollectionViewGroup2`.

### Default view lifetime

WPF keeps a default view, and the rules set on it, alive as long as its collection: the view is subscribed to the
collection's `CollectionChanged`, so the collection roots the view, and the view roots the collection; both are in
one managed heap, so the GC collects them together. The WinUI equivalent was considered and is not GC-safe:

- A native table that holds the view strongly roots it, and with it the source it holds and any app delegate in its
  rules (`Filter`, `CustomSort`, a `GroupDescription` subclass). A filter lambda that captures the page that owns
  the list then keeps the page alive forever.
- Making the source hold the view (a strong `VectorChanged` handler, as WPF's event does) relies on the C#
  projection reporting that reference to the GC through the reference tracker. Delegates the framework hands to a
  managed event are not tracker-walked, so the cycle source → handler → view → source is invisible to the GC and
  leaks; for a native source it is a plain reference-count cycle. The framework subscribes weakly for this reason.
- An ephemeron (a `ConditionalWeakTable`: the view lives as long as the source) needs the garbage collector, which
  native code does not have.

A weakly held default view, the only safe option, would lose its rules after a GC unless the app kept the view, and
`ItemsControl` never shows a list through a default view, so its rules would reach only the controls it is given to.
So there is no public default view (A-3, resolved): `CollectionViewSource.GetDefaultView` and `IsDefaultView` are not
added, and apps shape a list through a `CollectionViewSource`. They can be added later if `ItemsControl` gains
default-view support.

### Architecture: the view owns the engine

As in WPF, the view does the shaping and `CollectionViewSource` is a proxy over it.

| WPF | WinUI POC |
|---|---|
| `CollectionViewSource.EnsureView` → `ViewManager.GetViewRecord` creates a `ListCollectionView` for an `IList` | `CollectionViewSource::EnsureView` → `CollectionViewManager::GetViewRecord` creates a `VectorCollectionView` for an `IVector` / `IBindableVector` (and, once a rule is set, for a snapshot of any other enumerable) and reports it shapeable |
| `CollectionViewSource.ApplyPropertiesToView` copies `SortDescriptions`, `GroupDescriptions` and `Filter` onto the view | `CollectionViewSource::ApplyPropertiesToView` hands the view the CVS's descriptor collections (shared, not copied), `Filter` and `CustomSort` |
| `ListCollectionView` owns `InternalList`, sorting, filtering, grouping, `DeferRefresh` | `VectorCollectionView` owns the source subscription, the rules, `InternalList`, `RefreshOverride`, `ProcessCollectionChanged`, the `DeferRefresh` level and the groups |

- **The view owns the rules while it is `View`.** The descriptor collections are shared objects: `cvs.SortDescriptions`
  and `((ICollectionViewShaping)cvs.View).SortDescriptions` are the same collection, and a change to it is reported
  to the view (or, while there is no shaping view, to the CVS). `cvs.CustomSort` and `cvs.Refresh()`
  forward to the view; when `Source` changes, the CVS takes `CustomSort` back and applies it to the next view.
  `cvs.Culture` is the CVS's own `DependencyProperty`, applied to each view and to `View` when it changes (WPF). The view's `Filter` delegate is the view's own, as WPF's `ICollectionView.Filter`: it stays with the
  view and does not move to the next one (the CVS filters through its `Filter` event). A view the CVS has replaced
  keeps a copy of the rules it had and keeps following its own source, as each WPF view keeps the rules copied into
  it; nothing set on it reaches the CVS or its new `View`.
- **Lifetime.** The CVS holds its view (`View`); the view holds the CVS weakly. While the CVS has `Filter` event
  handlers the view holds the handler list itself (WPF's view holds the wrapped filter delegate) and uses the CVS as
  the sender while it lives, so the two never form a native reference cycle the GC cannot see. A consumer that holds only `cvs.View` (for example TableView's `ItemsSource`) keeps a
  working, shapeable and still filtered view after the app drops the CVS. The app objects the view keeps between
  changes (the `CustomSort` comparer, a group description's `Converter` and `CustomSort`, a `GroupDescription`
  subclass, `GroupNames`) are held through tracked references its reference tracker walk reports, so a delegate that
  captures its page does not root the page.
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

The engine (`VectorCollectionView_Shaping.cpp` with `_SourceChanges.cpp`, `_LiveShaping.cpp` and the private header
`VectorCollectionView_ShapingInternal.h`) and the CVS proxy (`CollectionViewSource_Shaping.cpp`) use the names
of their WPF counterparts so the logic can be read side by side with dotnet/wpf. Public API names are unaffected.

| WinUI internal | WPF counterpart | Role |
|---|---|---|
| `VectorCollectionView` (with shaping enabled) | `ListCollectionView` | The view over a vector; owns the engine and answers `ICollectionViewShaping`. Keeps its WinUI name (see decision 6). |
| `InternalList` (`VectorCollectionView::m_tpInternalList`) | `ListCollectionView.InternalList` | The shaped list the view presents. |
| `VectorCollectionView::RefreshOverride` | `ListCollectionView.RefreshOverride` | Full recompute: filter, stable sort, group. |
| `VectorCollectionView::RefreshOrDefer` | `CollectionView.RefreshOrDefer` | Recompute now, or once the last `DeferRefresh` closes; keeps currency. |
| `VectorCollectionView::ProcessCollectionChanged` / `ProcessCollectionChangedWithAdjustedIndex` | `ListCollectionView.ProcessCollectionChanged` / `ProcessCollectionChangedWithAdjustedIndex` | A source change: applied incrementally on a flat view, else a full recompute. |
| `CollectionViewSource::ApplyPropertiesToView` | `CollectionViewSource.ApplyPropertiesToView` | Gives a new view the CVS's rules. |
| `CollectionViewManager::GetViewRecord` | `ViewManager.GetViewRecord` | Picks the view class for a source. Always creates a view: there is no view cache (see "WPF semantics" below). |
| `CollectionViewGroupInternal` | `MS.Internal.Data.CollectionViewGroupInternal` | A computed group: its subgroups (`Items`) or items, its leaves, `Key` / `Name` / `ItemCount` / `IsBottomLevel`; `LeafCountBefore` and `FindSubgroupPosition` play `LeafIndexFromItem` and `FindIndex`. |
| `CollectionViewGroupRoot` | `MS.Internal.Data.CollectionViewGroupRoot` | The top of the group tree: the group descriptions per level (`GetGroupNames` = `GroupNameFromItem`, `NamesMatch`, the group order), `AddLoading` (`AddToSubgroups` while loading), `FindLeaf`. |
| `PlanGroupedInsertion` / `ApplyGroupedInsertion` / `ApplyGroupedRemoval` | `CollectionViewGroupRoot.AddToSubgroups` / `RemoveFromSubgroups` | An item joins or leaves the group tree incrementally. |
| `VectorCollectionView::ProcessGroupedCollectionChanged` | `ListCollectionView.AddItemToGroups` / `RemoveItemFromGroups` | A source change on a grouped view. |
| `VectorCollectionView::PassesFilter` | `CollectionView.PassesFilter` | The CVS's `Filter` event handlers, else the view's `Filter` delegate. |
| `SortFieldComparer` | `MS.Internal.Data.SortFieldComparer` | Multi-key (or `CustomSort`) comparison in the view's `Culture`, used by the full sort, the incremental placement and the group order. |
| `VectorCollectionView::LiveShapingState` | `MS.Internal.Data.LiveShapingList` | The live paths (`SetLiveShapingProperties`) and one `LiveShapingItem` per source item, in source order. The view keeps its `InternalList`; WPF swaps in a `LiveShapingList`. |
| `VectorCollectionView::LiveShapingItem` / `LivePropertyObserver` | `MS.Internal.Data.LiveShapingItem` | The dirty flags of one item and a property path listener per live property (WPF binds a DP per property). |
| `VectorCollectionView::OnLivePropertyChanged` / `RestoreLiveShaping` | `LiveShapingList.OnItemPropertyChanged` / `ListCollectionView.RestoreLiveShaping` | Marks the item, then restores the marked items at the next dispatcher turn. |
| `FindSortedRange` | `LiveShapingBlock.FindPosition` | Binary search that skips marked items and reports the range an item is in order in. |

`VectorCollectionView`, `GroupedDataCollectionView`, `CollectionView` and `CollectionViewManager` are pre-existing
WinUI classes and keep their names.

### WPF semantics: what matches and what deliberately does not (batch 6)

The rule: match WPF where its design is sound for a WinRT, reference-counted, single-threaded view and performs well;
deviate where it would be unsafe for the GC, contradict a shipped WinUI contract, or put per-control state in the
shared view. Editing (`IEditableCollectionView`) is out of scope: TableView does not edit.

**Currency** (WPF `ListCollectionView`, `CollectionView`; ours `VectorCollectionView_Currency.cpp`, see "Behavior"):

| Behavior | WPF | Ours | Decision | Why |
|---|---|---|---|---|
| After a recompute | Before first / after last kept, else item followed, else first | Same | Match | Batch 1 |
| `CurrentChanging` on a recompute | One, non-cancelable, before recomputing | Same (was a cancelable one after the `Reset`, up to three pairs with a transient "before first") | Match | Handlers save state with the old currency; selection no longer flickers through "nothing" |
| Currency when the `Reset` is raised | Already placed | Same (was stale) | Match | A `VectorChanged` handler reads consistent currency |
| A `Reset` handler moves the currency | Kept (WPF's `Selector` reselects the current item) | Overruled once, non-cancelable | Deviate | WinUI's `Selector` unselects a moved item on `Reset` and moves currency to "nothing"; without this a sort would drop a synchronized ListView's selection |
| Order of the currency events | `CurrentChanged`, then `PropertyChanged` | `PropertyChanged`, then `CurrentChanged` | Deviate | `CollectionView`'s order, one order for every view |
| Live move of the current item | `Move`: currency moves, only `CurrentPosition` raised | Same, over `ItemRemoved` + `ItemInserted` (was two `CurrentChanging`/`CurrentChanged` pairs) | Match | WinRT has no `Move`; the pair is folded back into one |
| Current item dropped by live filtering | To the item now at its place, or the last | Same (was the first item) | Match | Same rule as a removal |
| Source `Replace` of the current item, same place | The replacement is current | Same on a shaped view (was the first item) | Match | Unshaped views keep the old behavior (below) |
| `Remove` of the current item; insert before it; a `Replace` that moves | Item at its place or last; position shifts; item at its place | Same (unchanged `CollectionView` code) | Match | — |
| Unshaped views (`VectorCollectionView` before a rule, `IsSourceGrouped`, factory, app views) | — | `CollectionView`'s shipped contract (a `Replace` of the current item moves to the first) | Deviate (keep) | Shipped behavior; only views that shape get the WPF rules |
| `MoveCurrentTo*` to the current item | No events | `CurrentChanging` + `CurrentChanged` | Deviate (keep) | Shipped `ICollectionView` contract, same class shaped or not; `Selector` sync relies on the events |
| `MoveCurrentToPosition` out of range | Throws `ArgumentOutOfRangeException` | Clamps to before first / after last | Deviate (keep) | Shipped contract |
| A cancelled `MoveCurrentTo*` | Returns `IsCurrentInView` | Returns false | Deviate (keep) | Shipped contract |
| Empty view | Before first and after last | Before first | Deviate (keep) | WinUI derives both flags from the position |
| Currency members during `DeferRefresh` | Throw | Work on the output not yet recomputed | Deviate | WinUI `ICollectionView` never throws; a selection handler may move currency while the app defers |
| First item added to an empty view that is after last | Before first | After last | Skip | Edge case of the shipped contract |

**The questionable rows of the WPF parity audit:**

| Row | Decision | Why |
|---|---|---|
| `GetDefaultView` lifetime (#17, A-3) | Skip (resolved) | No public default view: WinUI controls don't use default views and WPF's lifetime is not GC-safe here (see "Default view lifetime"); use `CollectionViewSource` |
| Rules shared with the view, not copied (#7, #8, #34) | Deviate (keep) | One set of rules per CVS: a TableView header sort written on the view is the CVS's rule too, so it survives a new `View` (an `IsSourceGrouped` change); a replaced view keeps a copy, as each WPF view does |
| No view cache in `CollectionViewManager::GetViewRecord` (#186) | Deviate (keep, documented) | Each CVS holds its `View` and `ItemsControl` never shows a default view, so a cache would have nothing to share; a CVS whose `Source` goes back to an earlier collection gets a new view (currency on the first item), where WPF reuses the old one. A cache would also need a GC-safe lifetime (see A-3) |
| Internal names that differ (#162 `ProcessGroupedCollectionChanged`, #169 `Plan`/`ApplyGroupedInsertion`, #178 `LiveShapingState`) | Deviate (keep) | The code is shaped differently (plan before raising, for exception safety; no `LiveShapingList` swapped in); WPF's names would claim a parity the structure lacks. The table above maps them |

**Other missing WPF semantics:**

| Feature | Decision | Why |
|---|---|---|
| Live restore at `DispatcherPriority.DataBind` | Skip | The XAML dispatcher has no data-binding priority; whether the restore lands a frame late is measured with the live-shaping performance work |
| `NeedsRefresh`, `IsInUse`, `DetachFromSourceCollection`, `Comparer`, `IItemProperties` (#72–#75, #92) | Skip | New members without a partner need |
| `GroupBySelector`, `IsDataInGroupOrder`, `CompositeCollection` (#64, #65, #153) | Skip | No partner need |
| `ItemCollection` shaping, nested groups in ListView (#202, #204) | Skip | Separate proposals |
| `IEditableCollectionView`, `NewItemPlaceholder` (#76, #91, #93–#101) | Skip | TableView does not edit |

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

cvs.Filter += (s, e) => e.Accepted = ((ProcessItem)e.Item).Name.Contains(query);   // rows follow the view
cvs.Refresh();                                                                    // after edits or a new query
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
  moved, not deleted). Keyboard focus follows the item too: when the focused row is removed and its item is still in
  the source, or the view raises a `Reset` on its own (a mass live restore, an app's rule change), focus is put back
  on the item's new row at the end of the turn. The focused row is found where it is when the change arrives (rows
  inserted, removed or moved above it shift it), and a removal of several rows counts if it contains it. Focus does
  not follow when the row was scrolled out of view before the change (the viewport is not moved for it), or when the
  app or user moved focus elsewhere in the same turn (ItemsRepeater's own move of focus off the recycled row does not
  count). When the focused row was in view and the change moves it out of view, the viewport follows it (the row is
  realized and focused, which brings it into view); restoring focus without moving the viewport is pending a design
  decision. TableView's own header sort keeps focus at the same row index, as before.
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
  Task Manager-style flat data; the `CollectionViewSource`, its `PropertyGroupDescription` and its `Filter` handler
  are declared in the page's markup, the filter box calls `Refresh()`, and a header click sorts through the view. The "Task Manager" page (`TaskManagerPage`) sets
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
- Live shaping moves an item in the view; the tree is rebuilt from the view's new order. A rebuild is coalesced: the
  first `ItemInserted`, `ItemChanged` or `Reset` of a dispatcher turn rebuilds at once, and an `ItemRemoved` (the first
  half of a move) or any later change in the turn rebuilds once at the next turn. A live tick that moves k rows costs
  at most two rebuilds, not 2k. A rebuild reads the key and parent key only of the rows a view `ItemInserted` or
  `ItemChanged` brought in since the last one (a `Reset`, a new `ItemKeyPath`/`ParentKeyPath` and object keys read
  every row), and the rebuilt rows reach the repeater as the inserts and removals that turn the old rows into them
  (up to 64 rows, or a moved run of up to 64 rows) instead of a `Reset`, so realized rows, scroll position and focus
  stay. The rebuild itself is still O(n) native work (see "Performance").

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
  it. A view `Insert`, `Remove` or `Replace` of a plain tree is applied to the index in place: only the changed row's
  keys are read, it joins or leaves its sibling list at its place in the view's order, and its row is spliced in or out
  of the visible rows. What that cannot localize (a row with children leaving, a row whose key orphaned rows name,
  object keys, a duplicate, a tree under a grouped view, a `Reset`) rebuilds the tree, at most once per dispatcher turn.
  The flattener owns the index and is its only writer; everything is located before anything changes, and a view
  change made by app code (a key selector, a row handler) while one is applied makes it fall back to a rebuild. The
  expansion intent of a removed row is dropped at the end of the turn unless the row came back (a move).

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

## Performance

n = source items, m = items shown, f = sort fields, G = groups at a level, L = levels, g = items in one bottom
group, k = items a live tick changed.

| Path | Before (batch 1) | Now |
|---|---|---|
| Full recompute (sort) | O(n·f) reads + O(m log m) compares, each re-decoding both keys (2 QIs, 2 `HSTRING` copies, `CompareStringEx`) | O(n·f) reads, each decoded once into a typed key (`LCMAP_SORTKEY` bytes for a string, exact integer, double, ticks) + O(m log m) `memcmp` or integer compares |
| Full recompute (grouping) | O(m·L·G) `NamesMatch` + every ancestor inserts each leaf mid-vector | O(m·L) hashed group lookups (a key per `StringComparison`: ordinal, invariant uppercase or the culture's sort key; a scalar's value; an object's identity), leaves appended to bottom groups, then each ancestor collects its leaves once |
| New group with group `SortDescriptions` on `Key`/`Name` | O(G) compares, 2 property reads each | O(log G) compares on keys read when the group was created (other paths, such as `ItemCount`, are still read at each compare, as in WPF) |
| Flat `Add` / `Remove` / `Replace` | O(n) scans + O(log m) compares reading the other item's keys + the vector shifts | the same scans, O(log m) compares on cached keys, no comparer rebuilt per change, no scan for an `Add`; batch 6: the presented list shifts 4-byte slot numbers, not tracker pointers (see below) |
| Grouped `Add` / `Remove` / `Replace` | O(n) leaf search + O(L·G) `NamesMatch` | O(1) bottom group by source index + O(g) in it, O(L) hashed lookups; O(n) source-index shift |
| Group lookup, custom `GroupDescription` (batch 4) | O(G) `NamesMatch` calls per item and level (the ABI hop each time) | the name hashed as the default `NamesMatch` compares it (ordinal) and one `NamesMatch` call to confirm; a miss (a new group, or an override that matches more names than the default) scans the subgroups with `NamesMatch`, as WPF always does: O(m + G²) calls for a load instead of O(m·G) |
| Live tick, k changed items (batch 4) | O(n + k·(m + log m compares)), two changes per move; one `Reset` past fixed thresholds (k ≥ 80% of n, or n ≥ 1024 and k ≥ n/8, flat k ≥ 256) | k moves while they cost less than twice a `Reset` or 50 ms; else a flat view re-sorts on cached keys (only the k items are read), finds the longest run still in order (O(m log m)) and raises a move for each other item, or one `Reset` if those cost more; a grouped view recomputes; a source change while an edit waits is applied incrementally |
| `Refresh` with live shaping on | n·p listeners torn down and recreated | kept when the live properties and the items are unchanged |
| Live shaping setup, p live properties (batch 6) | n·p property path listeners (a listener, its step, a property access and a `PropertyChanged` handler each) | n `PropertyChanged` handlers and n tracked references for the plain property names on `INotifyPropertyChanged` items; a listener per item and path only for a dotted or indexed path, a `DependencyObject`, a map or a collection view |
| TableView tree, one view change | every row's key and parent key read (app code) + O(n) rebuild + `Reset` | batch 3: only the rows the change brought in are read + O(n) native rebuild, published as a splice; batch 5: an `Insert`, `Remove` or `Replace` reads the one row's keys and updates the index and rows in place: O(siblings) to place it plus O(rows) scans of native pointers to find its row, no rebuild |

Measured with `CollectionViewPerfBenchmarks` (TableView API tests, `Ignore=True`; run them with `te.exe ...
/select:"@Name='*CollectionViewPerfBenchmarks*'" /runIgnoredTests`): amd64**fre**, Hyper-V test VM, C# rows with
`INotifyPropertyChanged`, string keys of 8 random mixed-case letters, milliseconds.

| Scenario | 10k before | 10k now | 100k before | 100k now |
|---|---:|---:|---:|---:|
| `Refresh`, sort by a string | 160 | 42.7 | 1,714 | 493 |
| `Refresh`, filter + string sort | 67.9 | 22.9 | 860 | 253 |
| Group into 1,000 groups (`Ordinal`) | 2,232 | 81.2 | 21,749 | 828 |
| Group into 1,000 groups (`CurrentCultureIgnoreCase`) | 2,637 | 84.9 | 35,423 | 757 |
| Group into 3 levels | 346 | 134 | 10,067 | 1,590 |
| Flat `Add`, each | 0.21 | 0.17 | 1.7 | 1.4 |
| Grouped `Replace` (100 groups), each | 0.13 | 0.06 | 0.18 | 0.07 |
| `Refresh` with live sorting on | 1,781 | 52.0 | 3,131 | 533 |
| Live tick, 1,000 edits, flat | 284 | 16.5 | 2,450 | 143 |
| Live tick, 0.9·n edits (≈59% of rows), flat | 1,694 | 53.7 | 230,143 | 726 |
| Live tick, 1,000 edits, 100 groups | 49.7 | 33.8 | 104 | 63 (one run: 1,424) |
| Live tick, 0.9·n edits, 100 groups | 322 | 105 | 6,005 | 1,275 |
| TableView tree, insert one row | 411 | 43.2 | 4,298 | 540 |
| TableView tree, expand all | 19.8 | 16.3 | 156 | 127 |

Each number is one run (the sorts are the median of three); the 100k live ticks vary with garbage collection of the
earlier scenarios' rows (the grouped 1,000-edit tick took 63 ms in two runs and 1,424 ms in a third, on the same
code path). The chk build is not a reference: its tracker-pointer registry (`ReferenceTrackerManager::m_allTrackerPtrs`, DBG
only) makes every tracker pointer release O(all tracker pointers), so 100k live listeners took 20 minutes there. Batch 6 makes that registry a hash set (DBG only), which was
also why creating small live views in the chk teardown benchmark took 1.3–1.9 s each (see "Batch 6" below).

Batch 4 (same harness and VM, `7c7446b` vs batch 4, no other build on the host; milliseconds; "moves" or "Reset" is
what the tick raised):

| Scenario | 10k before | 10k now | 100k before | 100k now |
|---|---:|---:|---:|---:|
| Group into 1,000 groups by a C# `GroupDescription` subclass (no `NamesMatch` override) | 5,527 | 746 | 62,945 | 1,869 |
| The same with a C# `NamesMatch` override | 1,018 | 190 | 11,526 | 936 |
| `Replace` under that subclass, each | 1.50 | 0.26 | 1.45 | 0.28 |
| Live tick, 50 rows changed, flat | 18 (moves) | 17 (moves) | 129 (moves) | 140 (moves) |
| Live tick, 255 rows changed, flat | 328 (moves) | 11 (`Reset`) | 668 (moves) | 153 (`Reset`) |
| Live tick, 1,000 rows changed, flat | 18 (`Reset`) | 13 (`Reset`) | 156 (`Reset`) | 162 (`Reset`) |
| Live tick, 0.9·n edits, 100 groups | 111 (`Reset`) | 121 (`Reset`) | 1,351 (`Reset`) | 1,474 (`Reset`) |

The subclass without an override is slower than the one with: its `NamesMatch` crosses the ABI twice (into the app's
class and back to the base), and each new group still asks it of every existing group (O(G²) calls, about 0.5 million
for 1,000 groups). Other rows are unchanged within run-to-run noise (±15%); the full table is in the batch 4 logs.

What is still O(n) per change, and why it stays for now:

- **The presented list's integer arrays shift.** Since batch 6 the view's output (`InternalList`, and each group's
  leaves) keeps each item's `TrackerPtr` in a slot that does not move while the item stays, with the order as a
  vector of slot numbers (`StableTrackerPtrVector`, see "Batch 6"). An insert or a remove still moves the later
  entries of three integer arrays (the order, the source indices and the sort key slots) and scans the source
  indices: O(n) memory moves of 4-byte values, about 2 ns a row, not O(n) lock-taking `TrackerPtr` moves. A live move
  is therefore O(n) with a small constant, plus O(log m) compares. An order-statistic tree (WPF's
  `LiveShapingTree`) would make it O(log n), but at 100k rows the integer moves (~0.1–0.2 ms) are already below the
  cost of raising the two changes and of the consumers' handling of them, so it is not done.
- **Source index shifts.** A source insert or remove renumbers the later source indices (a scan of n integers).
- **TableView's tree** applies a single row's change in place (batch 5) but still finds the row with a linear scan of
  its visible rows and renumbers the sibling positions it shows (a few ms at 100k rows). Per-sibling visible-subtree
  counts (a Fenwick tree) would make that logarithmic; not needed at these sizes. A row with children leaving, a
  `Reset` and the other cases in "Tree" above still rebuild (about 0.5 s at 100k rows). A node expand or collapse
  splices up to 64 rows per toggle (WinRT vector changes have no ranges) and otherwise raises one `Reset`.
- Each item still costs one property read per sort field, group level and live property on a full recompute, and
  one observer per item (a listener per item and path for paths that are not a plain name; WPF binds each).

Batch 5 (TableView tree, `38c2a7b` vs the incremental index, same harness; n/10 roots with nine children each,
sorted view, every row expanded; "through the turn" includes work deferred to the end of the dispatcher turn):

| Scenario | 10k before | 10k now | 100k before | 100k now |
|---|---:|---:|---:|---:|
| Insert one row, through the turn | 50.6 | 3.9 | 614 | 7.3 |
| Remove one leaf, through the turn | 51.8 | 3.8 | 624 | 5.2 |
| Reparent one row (`Replace`), through the turn | 57.0 | 3.7 | 700 | 5.8 |
| Insert one row, synchronous part | 53.8 | 1.1 | 624 | 4.8 |
| Load (create, bind, first layout) | 924 | 927 | 8,375 | 8,633 |
| Expand all | 19.4 | 18.5 | 152 | 147 |
| Collapse all | 7.7 | 7.9 | 51 | 50 |

Load and expand-all build the index once and were unchanged by batch 5.

Batch 6 (code complete, not yet measured; the expected effect is stated, and the benchmarks above are the ones to rerun:
`BenchIncrementalChanges`, `BenchLiveSortingTick` including its new `LiveFlat.Create` / `LiveGrouped100.Create`,
`BenchTree`, and `BenchTeardown` on fre and chk):

- **Slow live view creation (chk).** `BenchTeardown` created a grouped, live-sorted, live-grouped view over 60 rows
  in 1.3–1.9 s, and non-live views got slower the more were alive (19 ms each for 100 views, 87 ms for 1,600). The
  runs were amd64**chk**. In DBG builds every `TrackerPtr` a `WeakReferenceSource` owns registers in
  `ReferenceTrackerManager::m_allTrackerPtrs` (for the "every tracker pointer is walked" validation), which was a
  sorted `vector_set` shrunk to fit on every erase: each registration and unregistration moved or reallocated the
  whole registry, O(all tracker pointers alive). A live item cost p property path listeners, each with several
  registered tracker pointers, so a live view's creation paid that O(N) dozens of times per row. Fix: the registry
  is a hash set (O(1); DBG only, no fre change), and a live item creates far fewer tracker pointers (next item).
  Expected: chk creation per view in the low milliseconds and flat in the number of views alive; fre unchanged
  by the registry.
- **P-4, one `PropertyChanged` handler per item.** A live property that is a plain name (no `.`, indexer or
  attached property) on an `INotifyPropertyChanged` item that is not a `DependencyObject`, a map or a collection view
  is followed by one handler per item for all such properties, matching the name ordinally and treating a null or
  empty name as every property, exactly as the listener's `INPCListenerBase` does. The item is held by one tracked
  reference the view walks, as the listener held it. Other paths and items keep a listener per path. Expected:
  setup allocates one handler instead of p listeners (each a listener, a step, a property access and a handler) per
  item; for the teardown benchmark's three live properties, about 3× fewer `PropertyChanged` subscriptions and
  ~10× fewer tracker pointers. WPF binds each `LiveShapingItem` to each live property; here the binding engine's
  per-path machinery is not needed for a plain name, so it is not used.
- **P-2, presented-list storage.** `TrackerCollection` takes its storage as a template parameter (default
  `TrackerPtrVector`, so every other collection is unchanged). The view's lists use `StableTrackerPtrVector`: the
  `TrackerPtr`s live in a `TrackerPtrVector` that only grows at its end (an item keeps its slot until it leaves; a
  freed slot is reused), and the list order is a `std::vector<UINT>` of slots. `GetAt` is one more index; the
  reference tracker walk walks the slots (a free slot is empty), under the same lock as before; the order is not
  read by the walk, so it changes without the lock. Chosen over a chunked or gap buffer of `TrackerPtr`s because
  those still move tracker pointers (a gap buffer moves O(distance) per edit, a chunked list O(chunk)), and over
  making `TrackerPtrVector` itself relocate cheaply because that changes a lifetime primitive every collection in
  the framework uses. Expected: a flat `Add`/`Remove`/`Replace` at 100k from ~1.4 ms to ~0.1–0.2 ms (what is left
  is the integer scans and moves above and raising the change); a live move likewise; a grouped change no longer
  moves its group's tracker pointers. The live restore's cost model (`c_nsPerShiftedRow`, 14 → 2 ns) prefers moves
  over a `Reset` for about 7× more rows; it is an estimate to recalibrate from the new numbers.
  **TODO: recalibrate from bench.** Measure the post-P-2 per-row shift cost with `CollectionViewPerfBenchmarks`
  (amd64fre) and update `c_nsPerShiftedRow` and the move-versus-`Reset` row counts in Behavior (computed at 14 ns).
- **P-5.** The tracker shift it listed is gone (P-2). The flat `std::find`, the source-index renumbering and burst
  coalescing are not changed: they are integer scans, and coalescing changes the notifications (out of scope).
- **TableView tree load.** Every row's `ItemKeyPath` and `ParentKeyPath` are read through a one-time `{Binding}`
  (`SortMemberPathResolver`), which set the probe's `DataContext` to the row and then back to null, and a load read
  every key twice: once to validate the relation (`SetHierarchy`) and again, with the key cache just cleared, to
  build it. Now the validation's reads fill the key cache the build uses, and the probe's `DataContext` is cleared
  once on the next dispatcher turn instead of after each read (the same row twice in a row is still re-read through
  a null `DataContext`, so an edited key is seen). Expected: a quarter of the binding evaluations at load (4n
  instead of 8n `DataContext` writes for n rows); if those dominate, as their count suggests, the 8.4 s load at
  100k should drop to roughly 2–3 s, the rest being the view's sort, the index build and the first layout.
- **Tree rebuild and expand-all.** A rebuild no longer builds the node-key index eagerly (it is built on first use,
  as after a splice), no longer collects every key into a set to prune expansion intent when no key differs from
  the default (a load, after expand-all or collapse-all), and skips the context-key and intent lookups when those
  sets are empty. Expected: expand-all at 100k somewhat below its 147 ms (one hash map of n `hstring`s and n
  lookups fewer); the per-row `hstring` and `NodeRow` are unchanged.

## Out of scope for the POC

- Key-selector delegates. The `CustomSort` comparer covers C++ and AOT callers.
- `GroupNames` in markup is supported; nested groups in ListView / GridView (they show the first level) are not.
- A public composable `ListCollectionView` (see "Extensibility") and editing (`IEditableCollectionView`, see "WPF
  parity pass").
- Hierarchy on CVS (see "Hierarchy: TableView.ItemKeyPath / ParentKeyPath (V1)").
- `Move` as a single change (a live move is a remove and an insert), and coalescing a burst of source changes (live
  property changes are coalesced per dispatcher turn, as in WPF).
- WPF's insertion-sort path for a restore where most items are marked: the cost-based restore replaces it (see
  "Live shaping").
- The remaining O(n) costs listed in "Performance".

## Known risks

- **Codegen.** Generated outputs were first hand-written, then regenerated with `dxaml\xcp\runcodegen.cmd`
  (`a78612b94`, and again for `ICollectionViewShaping`). See "Regeneration" below.
- **Flat/grouped switch.** Adding the first or removing the last `GroupDescription` keeps `View`; ListView, GridView
  and `ItemsControl` regenerate their containers as for a new `ItemsSource` (scroll position and realized containers
  are not kept, as after a `Reset`). A custom control that read `CollectionGroups` once per `ItemsSource` must listen
  to the view's `PropertyChanged("CollectionGroups")`. Changing `IsSourceGrouped` still gives the CVS a new `View`.
- **Reorder.** `ListViewBase` drag-reorder calls `RemoveAt` / `InsertAt` on the items source
  (`ListViewBase_Partial_Reorder.cpp:1498-1499`). Shaped views are read-only, so, as for grouped views, reorder is
  off while `ItemsSource` is a shaped view or an app `ICollectionViewShaping` view with a sort or a filter: keyboard
  reorder, drop, drag start, the live reorder animation and the item's reorder hints all ask
  `GetCanReorderItemsOverItemsSource`. WPF also blocks reorder on sorted views. The containers' visual states read a
  value cached per `ItemsSource` and items change (a rule change that moves no item leaves the hints stale until the
  next change; it is not cached while the view is mid-change or deferred) and ask only the internal view, so no app
  code runs from a visual state: over a sorted app view the hints show "reorderable" while the drag refuses. A drag
  over the list asks once, on `DragEnter`, app views included; keyboard reorder and drag start ask afresh. All of it
  is compiled only with `MUX_PRERELEASE`. A shaped view whose rules were all removed
  presents its source in order again; while it does (it listens to an observable source, its source indices are valid
  and no recompute is pending) its index writes go to the source, as WPF's view does once it no longer needs a local
  array, so reorder works again. A reorder picks the collection it writes to once (`GetReorderTarget`), so its
  `RemoveAt` and `InsertAt` reach the same collection even if a source handler adds a rule in between; the view then
  no longer presents source order, so the rest of a multi-item move stops and selection and focus follow the moved
  item to where the view shows it. If an `InsertAt` fails without inserting (the collection is one item short), the
  item is put back where it was; if it inserted and then failed (a change handler failed), it is left at its new
  place. Either way the error reaches the caller. Over a source that raises no changes, a shaped view
  never writes through: writes fail and leave the source unchanged.
- **Full refresh cost.** `Refresh()`, a descriptor change and a source `Reset` recompute fully and raise `Reset`;
  selection and scroll position follow `Reset` semantics. Source `Add` / `Remove` / `Replace` are incremental on flat
  and grouped views.
- **Group objects.** A group is an internal `CollectionViewGroupInternal` that implements `ICollectionViewGroup`,
  `ICollectionViewGroup2` (`Name`, `Items`, `ItemCount`, `IsBottomLevel`) and `ICustomPropertyProvider` (so bindings
  see `Key` too). A group `CustomSort` gets the names, not the groups, as before.
- **Deferral lifetime.** A `DeferRefresh` deferral that is never closed keeps the view stale (WPF behaves the same).
  It holds the view (or, for a deferral taken on the CVS, the CVS) strongly until closed.
- **API shape.** The `GroupDescription` base, the CVS `Filter` event and the enum names were settled in the
  batch 3 follow-up (see "Changes in batch 3" and "Naming"). `SortDescription` shares its simple name with WPF and the
  Toolkit's `AdvancedCollectionView` (`Microsoft.Toolkit.Uwp.UI.SortDescription`), so an app that uses both
  namespaces needs an alias. The council still has to sign off on `ICollectionViewShaping` as a capability name.
- **Descriptors are UI-thread objects.** `SortDescription` and `GroupDescription` are `DependencyObject`s now (they were
  free-threaded immutable classes, as WPF's `SortDescription` struct is), so they are created and changed on the UI
  thread (a view model built on a worker thread cannot create them), each has a core peer, and two
  `SortDescription`s with the same values are not equal. The price of setting them in markup; a free-threaded
  immutable `SortDescription` would need another way to be created from XAML.
- **Errors in app code are reported, not thrown, for source changes and live restores** (see "Exceptions in app
  code"). An app that relies on `Add` throwing when its filter throws (WPF) sees no exception there; the error goes
  to `Application.UnhandledException` (and ends an app that does not handle it) and the view recomputes.
- **TableView tree and `Id` / `ParentId` edits.** The tree reads the keys of the rows a change brings in and caches
  the others, so re-parenting a row by editing its `ParentId` (no Insert, Replace or Reset) is not seen until the
  next `Refresh()` or source `Reset`. A tree `Remove` that cannot be applied in place (the row has children) is
  published at the end of the dispatcher turn, so for that turn the rows, selection and UIA may still show it.
- **Feature gate in tests.** The new test assumes `Feature_ExperimentalApi` is enabled in the test configuration,
  like other experimental-API tests (for example `GridViewIntegrationTests.cpp:573`).
- **Live shaping cost and lifetime.** One `PropertyChanged` handler per `INotifyPropertyChanged` source item for its
  plain live property names, and one property path listener per item and other live path (batch 6). A full
  recompute (a header sort, `Refresh()`) keeps them when the live properties and the source items are unchanged; a
  source `Reset` or a new live property list rebuilds them. The view walks them (and a group walks its key and
  its `PropertyChanged` handlers) in its reference tracker walk; they unsubscribe when an item leaves the source,
  when the features are turned off and when the view is destroyed. A restore after a turn in which most items
  changed costs one remove and one insert per moved item (one `Reset` when the moves would cost more, see "Live
  shaping"); TableView's tree mode rebuilds its tree at most twice for it.
- **Live moves and consumers.** A move is `ItemRemoved` + `ItemInserted`. ListView and GridView recycle the moved
  container, so selection and focus on it follow their remove semantics (focus stays in the list, on the row that
  took the removed one's place); TableView keeps the selection and keyboard focus on the item (see "Live shaping"
  in the TableView section).

## Regeneration

Generated outputs were hand-edited to match the generator. Run `dxaml\xcp\runcodegen.cmd` and keep its output.

| File | Change | Must regenerate / verify |
|---|---|---|
| `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Data.cs` | Source of truth (hand-authored) | — |
| `dxaml/xcp/dxaml/idl/winrt/core/microsoft.ui.xaml.coretypes2.idl` | Enum, delegate, two classes, CVS experimental block | Yes. Also expect `IReference<CollectionViewSortDirection>` in the enum `declare` block if the generator emits it. |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/CollectionViewSource.g.h/.g.cpp` | Forwarder, QI, accessors, `Refresh` | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/SortDescription.g.h/.g.cpp` (new) | Full class + factory | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/PropertyGroupDescription.g.h/.g.cpp` (new) | Full class + factory | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/FilterEventArgs.g.h/.g.cpp` (new) | Event args class | Yes |
| `Indexes.g.h`, `StableXbfIndexes.g.h`, `StableXbfIndexMapping.g.cpp`, `StableEventIndexes.g.csv`, `EnumDefs.g.h` | `CollectionViewSource_Filter` event index (340), `GroupDescription_GroupNames` property index (2490), `CollectionViewSource_Culture` property index (2491), the enum | Yes (appended entries only) |
| `dxaml/xcp/dxaml/lib/Factories.g.cpp` | 2 forward declarations + 2 activation entries | Yes (ordering) |
| `dxaml/xcp/dxaml/lib/synonyms.g.h` | 5 typedefs (with `ICollectionViewShaping` and `ICollectionViewLiveShaping`) | Yes |
| `dxaml/xcp/dxaml/lib/winrtgeneratedclasses/Aggregate.g.cpp` | 2 includes | Yes |
| `dxaml/xcp/components/metadata/*`, `dxaml/xcp/core/metadata/*`, XBF indexes | None expected (PropertyOnly members, `IsCreateableFromXAML = false` classes, enum excluded from type tables) | Verify no diff |
| `dxaml/xcp/dxaml/idl/winrt/main/microsoft.ui.xaml.idl` | None | Add `declare` entries only if MIDL does not emit `IVector`/`IIterable` of the descriptor types |
| `packaging/Intellisense/*` | None | Regenerate docs XML when promoting |

Hand-written files: `CollectionViewSource_Partial.h/.cpp` (`EnsureView`), `CollectionViewSource_Shaping.cpp`
(the CVS proxy: rule properties, `ApplyPropertiesToView`, `DeferRefresh`, descriptor factories),
`VectorCollectionView_Partial.h` and the engine: `VectorCollectionView_ShapingInternal.h` (sort keys, the group tree,
failure containment, in the named namespace `DirectUI::ShapingEngine`), `VectorCollectionView_Shaping.cpp` (rules,
`ICollectionViewShaping`, recompute), `VectorCollectionView_SourceChanges.cpp` (incremental source changes) and
`VectorCollectionView_LiveShaping.cpp` (live shaping), `VectorCollectionView_Currency.cpp` (currency of a shaped view),
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
- `CollectionViewSourceGroupConverter`: `Converter` maps values to group keys and gets the level as its parameter and
  the view's `Culture` as its language; a default-constructed `PropertyGroupDescription` (`Ordinal`) regroups when
  its `PropertyName` or `Converter` is set.
- `CollectionViewSourceCustomGroupDescription`: a C++/CX `GroupDescription` subclass groups through its
  `GroupNameFromItem` and `NamesMatch` overrides (case-insensitive initials), incrementally too, and its
  `SortDescriptions` order its groups.
- `CollectionViewSourceGroupNames`: `GroupNames` groups come first, in order, even when empty; an item joins its
  explicit group; an explicit group stays when it empties while another group goes; a new group goes after the
  explicit ones; a `GroupNames` change regroups.
- `CollectionViewSourceMarkup`: `XamlReader.Load` of a `CollectionViewSource` with `IsLiveSortingRequested`,
  `<SortDescription PropertyName Direction>`, `<PropertyGroupDescription PropertyName StringComparison>` and
  `<x:String>` live properties shapes the view once `Source` is set; a held `SortDescription` throws on change, a new
  one is settable; `ClearValue(IsLiveSortingRequestedProperty)` turns live sorting off.
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
- `CollectionViewShapingSourceCollection`, `CollectionViewSourceFilterEvent` (the `Filter` event alone, over the
  view's `Filter` delegate, back to the delegate).

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
  the `Filter` event after a GC with only `View` held; no leak through live listeners; group `ItemCount`
  notifications; currency before first / after last; exact `Int64` / `UInt64` keys; turning live sorting on;
  replaced views; rules on an `IsSourceGrouped` CVS; off-thread live changes; and TableView live sort, filter,
  group and tree, including at most a couple of tree rebuilds per live tick. Batch 4 adds: a total order over mixed
  key types and NaN; a C# `GroupDescription` subclass (with and without `NamesMatch`) against a fresh view; no leak
  through a `CustomSort`, a group `CustomSort`, a `Converter` or a `GroupDescription` subclass that captures its
  owner; app code that throws (filter, `CustomSort`, converter, `GroupDescription` overrides, the `Filter` event, a
  view or group change handler) on insert, replace, remove and move leaves the source's other listeners informed and
  the view equal to a fresh view; a throwing comparer in a live restore and a throwing filter in `Refresh()`; a
  handler that refreshes on every change stops; a view's `Filter` stays with it; live ticks that move, raise
  nothing, raise only the few moves, or one `Reset`; and a large TableView live tick that keeps its rows and
  selection. Batch 6 adds currency on a shaped view: one non-cancelable `CurrentChanging` before a recompute with the
  currency placed when its `Reset` is raised, then `CurrentChanged` once; a `Replace` of the current item in place on
  a shaped and an unshaped view; a live move that keeps currency on its item and a live filter that drops it.

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
