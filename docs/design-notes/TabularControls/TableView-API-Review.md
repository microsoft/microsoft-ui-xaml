# TableView API review

| | |
|---|---|
| **Status** | Proposal for API review (draft PR #12420, branch `user/hik/tableview/tableviewsource-api-proposal`) |
| **Surface** | `controls/dev/TableView/TableView.idl`, `controls/dev/TableView/TableViewSource.idl`, `controls/dev/TabularShaping/SortDirection.idl`, plus `TabularControlsResources` and `XamlControlsTabularXamlMetaDataProvider` from `controls/dev/dll-tabular/Microsoft.UI.Xaml.Controls.Tabular.idl` |
| **Namespace / binary** | `Microsoft.UI.Xaml.Controls.Tabular`, `Microsoft.UI.Xaml.Controls.Tabular.dll`. Every type is `[MUX_PREVIEW]`. |
| **Companion docs** | [API spec](../../api-specs/TableView/TableView-spec.md), [dev spec](TableView-dev-spec.md), [functional spec](TableView-functional-spec.md), [hierarchy API spec](Hierarchical-Data-Rows-API-Spec.md) |

The IDLs are the source of truth for shape. Behaviour in this document is the implementation contract for this PR. Where a contract depends on implementation that has not landed or been validated at run time, it says so.

## 1. Purpose and scope

TableView and TableViewSource are preview API. This review checks whether the proposed surface is ready to stabilize: whether it is complete for the core scenarios, consistent with WinUI conventions, and has precise contracts. Preview-only breaking changes are cheap now and impossible after stabilization, so this PR makes them now.

**In scope:** every public type in the three IDLs and the two Tabular DLL types. **Out of scope:** the framework-internal primitives `ResizeGripper` and `SortIndicator` (in `Microsoft.UI.Private.Controls`), the `[MUX_INTERNAL]` `TableViewRowTemplateSelector`, and the implicit template/theme-key surface (see §2.9 and §6).

### 1.1 Changes in this PR vs `main`

| | Member | Reason |
|---|---|---|
| **Added** | `TableViewSource.ThenSort(String, SortDirection)` (`ThenSortByPath`), `ThenSort(TableViewKeySelector, SortDirection)` | Adding a sort level becomes explicit (LINQ `OrderBy`/`ThenBy`). Previously a key-selector `Sort` silently appended depending on delegate identity. |
| Added | `TableViewSource.SortGroups(SortDirection)` | Group order never depends on call order. `None` (default) follows each group's first item (WPF parity); `Ascending`/`Descending` pins key order against item and header sorts. |
| Added | `TableViewSource.Refresh()`, `IClosable DeferRefresh()` | Reshape on demand when live shaping is off; batch several calls into one Reset. |
| Added | `TableViewSource.Source { get; }` | Read back the wrapped collection without the app keeping its own reference. |
| Added | `TableView.IndexOf(Object)`, `StartBringItemIntoView(Int32, BringIntoViewOptions)` | Map an item to the selection index space and reveal it; previously no way to select or show "this record" after shaping. |
| Added | `TableView.ExpandGroup(Object)`, `CollapseGroup(Object)`, `GroupExpanding`, `GroupCollapsed` + args (`GroupKey`) | Per-group programmatic expansion and a notification for every group state change. |
| Added | `TableView.CellEditEnded` + `TableViewCellEditEndedEventArgs`; `TableViewCellEditEndingEventArgs.EditingElement` | `CellEditEnding` reports an attempt before the value is written; handlers need the editor to validate, and a signal for the outcome. |
| Added | `TableView.IsEditingProperty` | `IsEditing` becomes a read-only DP so it can be bound. |
| Added | `TableViewColumn.CommitCellEditCore(FrameworkElement)` (overridable), `NotifyCellContentChanged()` (protected) | Editors outside the built-in binding list (DatePicker, ColorPicker, `{x:Bind}`) can commit; derived columns can regenerate their cells. |
| **Changed** | `Sort(...)` (both overloads): **replaces** all item sort levels | Was "append unless same path/delegate". Identity of a C# lambda is not visible to callers. |
| Changed | `TableViewColumn.CanResize` → `CanUserResize`, `CanSort` → `CanUserSort` | Match `CanUserResizeColumns`/`CanUserSortColumns` and WPF. All four `CanUser*` flags gate user input only: programmatic sorts ignore them, and turning one off does not clear the active sort. |
| Changed | `TableViewColumn.SortDirection` `{ get; }` → `{ get; set; }` | App-owned (e.g. server) sorts can show the arrow. Display only. |
| Changed | `TableView.ClearSort()` clears column **and** source item sorts | Previously returned `false` when only a source key-selector sort was active. |
| Changed | `TableViewIdentitySelector` → `TableViewGroupIdentitySelector` | Used only by `GroupBy`. |
| Changed | `TableViewDensity`: `Standard = 0`, `Compact = 1` | Zero value now equals the default. Persisted integers change meaning. |
| Changed | `TableViewGroupHeader.IsExpanded`/`IsExpandable` → read-only | Setters changed visuals but not rows, so UI/UIA could disagree with data. |
| Changed | `TableViewColumnHeaderAutomationPeer` ctor `(TableView, TableViewColumn)` → `(TableViewColumnHeader owner)` | Owner is the element (WinUI convention); old ctor made the TableView the owner. Stays unsealed. |
| Changed | `TableViewCellAutomationPeer` ctor `(FrameworkElement, TableViewRow, TableViewColumn, Int32)` → `(FrameworkElement owner)` | Owner-only, like every WinUI peer; row, column and index are derived from the cell, and a non-cell owner throws `E_INVALIDARG`. Stays unsealed. |
| **Removed** | `TableViewFrozenEdge.Trailing` | Reserved no-op; appending it later is compatible, changing a shipped no-op is not. |
| Removed | `TableViewGroupHeader.ToggleRequested`, `TableViewGroupHeaderToggleRequestedEventArgs` | Internal header→owner wiring; headers are control-created so apps could not subscribe usefully. Replaced by TableView-level events. |
| Removed | `TableViewGroupInfo.Level` | Always 0 and 0-based while `TableViewRow.Level` is 1-based. Re-add with nested grouping. |
| Removed | public `TableViewColumn.GenerateElement(Object)` | Only `GenerateElementCore` is an extension point. |

Engine fixes that come with the contract (no shape change): `GroupBy`/`Sort` on WinRT enum keys compare by underlying value (previously `GroupBy` threw and sort ordered by address); re-entrant shaping calls from inside app callbacks are refused (`E_ILLEGAL_METHOD_CALL`) instead of destroying the running filter; a shaping verb whose rebuild throws rolls back its declaration (strong exception guarantee; `ClearParentBy` excepted).

## 2. API surface by area

### 2.1 Data and shaping: `TableViewSource`

| Member | Type | Semantics / default |
|---|---|---|
| `static From(Object items)` | `TableViewSource` | Wraps `IVector<Object>`, `IBindableVector`, `IIterable<Object>` or `IBindableIterable`. Other types (including a `TableViewSource` or a `CollectionViewSource`) throw `E_INVALIDARG`. An `ICollectionView` is consumed as a flat list; its groups are ignored. |
| `Source` | `Object` (get) | The `items` passed to `From`, same reference. |
| `IsLiveShaping` | `Boolean` | Default `false`. When `true`, an item's `PropertyChanged` that moves its sort key, group key, filter result or parent reshapes (coalesced per dispatcher turn; held while a cell is edited). When `false`, order is stale until `Refresh()` or a structural collection change. |
| `Filter(TableViewPredicate)` | fluent | One predicate; a second call **replaces** it. Compose inside one predicate. |
| `ClearFilter()` | fluent | Removes the filter. |
| `Sort(String path, SortDirection)` (`SortByPath`) / `Sort(TableViewKeySelector, SortDirection)` | fluent | **Replaces** every item sort level with one level. `None` clears all item levels (= `ClearSort()`). `SortGroups` is untouched. |
| `ThenSort(String path, SortDirection)` (`ThenSortByPath`) / `ThenSort(TableViewKeySelector, SortDirection)` | fluent | Appends a lower-precedence level; with no existing level behaves as `Sort`. `None` throws `E_INVALIDARG`. Same path as an existing level: replaces that level's direction in place, keeps its precedence. Key-selector form always appends. |
| `ClearSort()` | fluent | Clears all item sort levels; keeps `SortGroups`. |
| `GroupBy(TableViewKeySelector)` / `GroupBy(key, TableViewGroupIdentitySelector)` (`GroupByWithIdentity`) | fluent | Single level; a second call replaces. Group identity is a stable non-empty string: built in for boxed scalars and WinRT enums, otherwise from the identity selector (which receives the **key**, not the item). |
| `SortGroups(SortDirection)` | fluent | `Ascending`/`Descending`: orders groups by key (enums by underlying value, numerics numerically, strings culture-aware, `DateTime` chronologically); item sorts then order rows only within groups. `None` (default) orders groups by the position of each group's first item in the filtered, item-sorted sequence (source order when there is no item sort), matching WPF `ListCollectionView` without group sort descriptions. |
| `ClearGroupBy()` | fluent | Removes grouping; keeps the `SortGroups` declaration. |
| `ParentBy(keySelector, parentKeySelector)` / `ClearParentBy()` | fluent | Flat list → tree. Rules in the [hierarchy API spec](Hierarchical-Data-Rows-API-Spec.md). |
| `Refresh()` | `void` | Re-applies all shaping now as one Reset. Inside a deferral: recorded, runs at outermost close. UI thread only. |
| `DeferRefresh()` | `IClosable` | Ref-counted deferral; UI thread only; see below. |
| `TableViewPredicate`, `TableViewKeySelector`, `TableViewGroupIdentitySelector` | delegates | `Boolean(Object)`, `Object(Object)`, `String(Object)`. |

**Composition.** Declarations are independent; the pipeline order is fixed: filter → hierarchy (`ParentBy`) → group → sort. Item sort levels order rows **within** each group (and among siblings under `ParentBy`). Group order never depends on call order: with `SortGroups(None)` groups follow their first item in the item-sorted sequence, so an item sort can also reorder groups; with `SortGroups(Ascending/Descending)` groups are pinned to key order. `GroupBy` under `ParentBy` buckets the roots. A filter under `ParentBy` keeps matches plus ancestors. A group whose last row is filtered out disappears.

**Lifetime and ordering contracts.**

| Contract | Detail |
|---|---|
| Fluent calls mutate | Every call changes this instance and returns it. `var b = a.Sort(...)` changes `a`. This differs from LINQ (immutable chain) and WPF (mutable, non-fluent). |
| `SortGroups` is retained | Inactive without `GroupBy`; survives `GroupBy` replacement, `ClearGroupBy`, `ClearSort`, `Sort`, and column-header sorts. |
| `DeferRefresh` | While any deferral is open, every shaping call (`Filter`/`Sort`/`ThenSort`/`GroupBy`/`ParentBy`/`SortGroups`/`Clear*`/`Refresh`) records its declaration without reshaping. Closing the **outermost** deferral runs one reshape (one Reset) if anything changed. Source collection changes during a deferral apply at close. Deferrals are ref-counted, so nested `DeferRefresh` calls need matching closes. `Close()` is idempotent (a second close does not decrement the count). UI thread only: `Close()` from another thread throws `RPC_E_WRONG_THREAD`. A deferral released without `Close()` (for example, garbage-collected) ends on a later dispatcher turn rather than synchronously, so always close it, ideally with `using`. |
| Re-entrancy | Any shaping member called from inside an app delegate (filter, key, identity or parent-key selector) while the source is reshaping throws `E_ILLEGAL_METHOD_CALL`. This covers every verb and `Clear*`, the `IsLiveShaping` setter, `Refresh`, `DeferRefresh` and closing a deferral. |
| Failure | Strong exception guarantee: a shaping verb whose rebuild throws is rolled back, so the declared state matches what is displayed and later reshapes do not keep throwing. Exception: `ClearParentBy()` is not rolled back; the relation is removed even if the rebuild throws. Invalid data that arrives later (not through a verb) leaves the declaration in place and throws on each reshape until fixed. Data errors arriving through the app's own collection change (e.g. a null group key on `Add`) are rethrown into that call, as WPF `ListCollectionView` does. No error event. |
| Single owner | One owning `TableView` at a time. A second table is refused (renders empty, cannot sort) until the owner releases it (`ItemsSource` change or `Unloaded`); it then binds. Only signal: a log message. |
| Threading | UI-thread affine. Collection changes must be raised on the UI thread; off-thread `PropertyChanged` is marshalled for live shaping. Callbacks always run on the UI thread. |

**Nuances and edge cases.**

- **Path sort needs bindability.** `Sort(path)`/`ThenSort(path)` and column `SortMemberPath` evaluate a `{Binding}` on a probe element. A misspelled path, a non-bindable C++ item, or a trimmed/NativeAOT C# type silently yields null keys and no sort. Key-selector overloads avoid this and are the recommendation for AOT.
- **Key comparison.** WinRT scalars compare by value. Enums compare by underlying value first (signed or unsigned as appropriate), so keys of different enum types interleave by value rather than by type name; group identity still includes the type. Keys that are not `IPropertyValue` compare by `ToString()` text if `IStringable`, otherwise by address; `IComparable` is not used, so .NET-only types (`DateOnly`, `BigInteger`, …) likely sort as text. Use a key selector that returns a WinRT scalar.
- **Live shaping limits.** Re-evaluates every active callback on any `PropertyChanged`, whatever the property; nested paths (`A.B`) are not tracked. With Filter/GroupBy/ParentBy active, each single-item collection change rebuilds (O(N) per change); batch with `DeferRefresh`.
- **Callback lifetime.** Callbacks are held strongly by native code. A C# callback capturing its page forms a cycle across the native boundary that the GC may not collect (finding B5, not yet run). Avoid capturing the page.
- **Row identity** is object reference (node key under `ParentBy`). Selection and focus follow the item across a reshape but not across item re-creation; the same object on two rows fails fast while a verb is active.
- **Throwing callbacks** are not yet handled uniformly (a throwing filter hides the row, a throwing sort key sorts as null, a throwing group key reports "null key"). See §6.

### 2.2 Columns

| Member | Type | Semantics / default |
|---|---|---|
| `TableViewColumn` | unsealed `DependencyObject`, public ctor, content property `Header` | Base column. |
| `Header`, `HeaderTemplate`, `HeaderTemplateSelector`, `HeaderStyle`, `HeaderToolTip` | DPs | Selector wins over template. `HeaderStyle` wins over `TableView.ColumnHeaderStyle`. |
| `Width` | `GridLength`, 120 px | `Pixel` exact; `Auto` = widest realized cell (grows within a data set) or current header; `*` = share of body viewport after fixed columns. |
| `MinWidth` / `MaxWidth` | `Double`, 20 / ∞ | Clamp every mode. |
| `ActualWidth` | `Double` (get), 120 | Realized width. |
| `CanUserResize` | `Boolean`, `true` | User resize (pointer, keyboard, UIA Transform) for this column, ANDed with `TableView.CanUserResizeColumns`. |
| `FrozenEdge` | `TableViewFrozenEdge`, `None` | `Leading` pins only within the contiguous prefix from column 0. Out-of-range values act as `None`. |
| `Visibility` | `Visibility`, `Visible` | Collapsed columns are absent from layout and from the UIA grid. |
| `IsReadOnly` | `Boolean`, `false` | Per-column opt-out; still a valid current cell. |
| `CellEditingTemplate` | `DataTemplate` | Editor for any column type. |
| `CellToolTipBinding` | `Binding` (CLR) | Per-cell tooltip content, evaluated against the row item. |
| `CellContextFlyout`, `HeaderContextFlyout` | `FlyoutBase` | See §2.7. |
| Sort members | — | See §2.3. |
| `overridable GenerateElementCore(Object)` | `FrameworkElement` | Creates the cell element. `dataItem` is for initial setup only; content must bind to the inherited `DataContext` because rows recycle. |
| `overridable CommitCellEditCore(FrameworkElement)` | `Boolean` | Writes the editor's value. Default: built-in recognition of the editor's binding. Return `false` to keep the edit open. An override that throws counts as a failed commit (edit stays open, error logged). |
| `protected NotifyCellContentChanged()` | `void` | Regenerates realized cells for this column and resets `Auto` width. Queued if called during measure/arrange. |
| `overridable GetSortMemberPathCore()` | `String` | Effective sort path; `TableViewTextColumn` derives it from `Binding.Path` when the binding targets the row item. |
| `TableViewTextColumn.Binding` | `Binding` (CLR) | Display `TextBlock`; editor `TextBox` reuses it with `TwoWay` + `Explicit`. |
| `TableViewTemplateColumn.CellTemplate` | `DataTemplate` DP | Null → empty cell (never `ToString()`). |
| `TableViewColumnHeader` | sealed `ContentControl`, no ctor; `Column` (get) | `TargetType` for header styles. |
| `TableView.Columns` | `IVector<TableViewColumn>` (get) | Collection order = visual and UIA order. |

**Nuances.** Auto widths reset on `ItemsSource` assignment, `Columns` replacement, or a column's header/template change, not on in-place `Reset`, refilter or `Density`; re-fit currently needs re-assigning `ItemsSource` (§6). A completed user resize writes local `Pixel` widths to the dragged column and to earlier `*` columns, which replaces bindings on those columns (§6). `{x:Bind}` in a `CellEditingTemplate` is not discoverable by the default commit; override `CommitCellEditCore` or use `{Binding … UpdateSourceTrigger=Explicit}`.

### 2.3 Sorting

| Member | Type | Semantics / default |
|---|---|---|
| `TableView.CanUserSortColumns` | `Boolean`, `true` | Gates **user input only** for the whole table. Turning it off does not clear the active sort. |
| `TableViewColumn.CanUserSort` | `Boolean`, `true` | Gates **user input only** (pointer, keyboard, UIA Invoke on the header). Turning it off does not clear an active sort on the column. |
| `TableViewColumn.SortCycle` | `TableViewSortCycle`, `AscendingDescending` | Header/`ToggleSortDirection` cycle; `*None` variants add a step back to source order. |
| `TableViewColumn.SortMemberPath` | `String` | Path for a column sort; empty → `GetSortMemberPathCore()`. |
| `TableViewColumn.CustomSortComparer` | `ITableViewSortComparer` | `Int32 Compare(Object left, Object right)`; wins over the path; must be pure. |
| `TableViewColumn.SortDirection` | `SortDirection`, `None` | **Display only.** Set by the control on a column sort; settable by the app, including in XAML or before the column is added (honoured when it joins `Columns`; the last added wins). |
| `SortByColumn(column, direction)` | `Boolean` | Declares the control's sort axis. `None` clears. |
| `ToggleSortDirection(column)` | `Boolean` | Advances the column's `SortCycle`. |
| `TableView.ClearSort()` | `Boolean` | Clears column sorts and item sorts on the effective source; keeps `SortGroups`. `true` if anything was cleared. |
| `Sorting` | `TableViewSortingEventArgs` (`Column`, `Direction`, `Cancel`) | Raised first; cancellable. |
| `Sorted` | `TableViewSortedEventArgs` (`Column`, `Direction`) | Raised after. `Column == null` means clear-all. |
| `SortDirection` enum | `None = 0`, `Ascending = 1`, `Descending = 2` | Shared by `TableViewSource` and `TableView`. |

**Gates.**

| Trigger | `CanUserSortColumns` | `CanUserSort` | Raises `Sorting`/`Sorted` | Reshapes data |
|---|---|---|---|---|
| Header click / Enter / Space / UIA Invoke | gates | gates | yes | yes |
| `SortByColumn` / `ToggleSortDirection` | ignored | ignored | yes | yes |
| Setting `TableViewColumn.SortDirection` | ignored | ignored | no | **no** |
| `TableViewSource.Sort(path, …)` | — | — | — | yes; lights the column whose path matches |
| Turning `CanUserSortColumns`/`CanUserSort` off | — | — | no | no; the active sort and arrow stay (WPF parity) |

**Contracts.** One control sort axis is in force: a column sort replaces the source's item sort levels and a source `Sort` replaces the column's axis, so the arrow never shows a losing axis. A header sort on grouped data replaces item sort levels and keeps `SortGroups`; with `SortGroups(None)` it therefore **can reorder groups** (each group follows its first item), with `Ascending`/`Descending` it cannot. `SortByColumn`/`ToggleSortDirection` return `false` and change nothing when `Sorting` is cancelled, an open edit will not close, the column has no sort path and no comparer, rows lack identity, or the source is refused.

**Display-only `SortDirection`.** Setting it updates the arrow and UIA state and records the column as lit by the app; it never reshapes. A non-`None` value clears the arrow on every other column. The next header click cycles from the displayed value. A value preset in XAML or before attach is applied when the column joins `Columns`. An app `ThenSort` after a header sort keeps the header sort as the primary level (and its arrow); only a foreign level declared before it displaces the column's axis. Setting `CustomSortComparer` while the arrow was set by the app neither reshapes nor raises events. Typical server-sort pattern: handle `Sorting`, set `Cancel = true`, fetch, set `Column.SortDirection`.

**Nuances.** `Sorted(null, None)` is raised both for clear-all and for an app key-selector sort (§6). The direction contract of `ITableViewSortComparer` (does the control invert for `Descending`?) is not stated in the IDL (§6).

### 2.4 Selection and navigation

| Member | Type | Semantics / default |
|---|---|---|
| `SelectionMode` | `TableViewSelectionMode` (`None = 0`, `Single = 1`), `Single` | `None` also clears the selection. |
| `SelectedItem` | `Object` (read-only DP) | |
| `SelectedIndex` | `Int32` (read-only DP), `-1` | Index in the **displayed projection**. |
| `Select(Int32)`, `Deselect(Int32)`, `IsSelected(Int32)`, `DeselectAll()` | | `ItemsView` names. `Select(<0)` clears. `Deselect` only clears if `index` is selected. `DeselectAll` works in any mode. |
| `IndexOf(Object)` | `Int32` | Item → projection index; `-1` for null, not present, filtered out, or under a collapsed group/parent. Value-typed items (boxed numbers, strings) match by value; reference items by identity. Duplicates: first match. Works before `Loaded` and after `Unloaded` (reads the source projection). A `TableViewGroupInfo` is not a data item. |
| `StartBringItemIntoView(Int32, BringIntoViewOptions)` | `void` | `ItemsView` shape. Out-of-range → `E_INVALIDARG`; `options` may be null. Asynchronous; realizes the row and lays it out before bringing its real bounds into view (ItemsView pattern); accounts for the sticky header and frozen columns; safe before `Loaded`. A pending request is cancelled when `ItemsSource` or the projection changes, or on `Unloaded`. |
| `SelectionChanged` | `TypedEventHandler<TableView, SelectionChangedEventArgs>` | Platform args; raised once after the selection settles; not raised for index shifts. |

**Projection index space.** `SelectedIndex`, `Select`, `IsSelected`, `Deselect`, `IndexOf` and `StartBringItemIntoView` share one space: the displayed rows, where **group headers take slots** and collapsed descendants are absent. It is not an `ItemsSource` index once any shaping is active. Headers cannot be selected (`Select` on a header index is rejected). Typical use: `var i = table.IndexOf(item); if (i >= 0) { table.Select(i); table.StartBringItemIntoView(i, null); }`.

**Nuances.** Selection follows the item across reshapes; removing it clears the selection. `Select` before `ItemsSource` is set or while `SelectionMode == None` is a no-op (not queued). Selection is stashed across unload/reload. Keyboard follows the treegrid model (dev spec); selection follows focus for Up/Down; Ctrl+click toggles. Pointer selection is not blocked during an open edit (known limitation).

### 2.5 Editing

| Member | Type | Semantics / default |
|---|---|---|
| `TableView.IsReadOnly` | `Boolean`, **`true`** | Editing is opt-in. Setting `true` closes an open edit. |
| `TableView.IsEditing` (+ `IsEditingProperty`) | `Boolean` read-only DP, `false` | `true` when an editor opens; set to `false` synchronously when the edit closes, before `CellEditEnded`. An external `SetValue` is reverted. |
| `CommitEdit()` / `CancelEdit()` | `Boolean` | Close the open **cell** edit; `false` if none, vetoed, validation rejected, or the column could not write. |
| `BeginningEdit` | args `Item`, `Column`, `Cancel` | Before an editor opens. |
| `CellEditEnding` | args `Item`, `Column`, `EditAction`, `EditingElement`, `Cancel` | Before close; synchronous veto. On commit the source is **not yet written**: validate via `EditingElement` (valid only during the handler). |
| `CellEditEnded` | args `Item`, `Column`, `EditAction` | Exactly once per edit session after close. |
| `TableViewEditAction` | `Commit = 0`, `Cancel = 1` | |
| `TableViewColumn.CommitCellEditCore` | see §2.2 | |

**Sequence (commit).** `CellEditEnding` (veto → edit stays open, no `Ended`) → `CommitCellEditCore` writes (`false`/throw → edit stays open, no `Ended`) → validation (`INotifyDataErrorInfo`; failure → stays open) → editor closes, `IsEditing = false` → `CellEditEnded` → reshapes queued during the edit run.

**Contracts.** `IsEditing` is never posted; only `CellEditEnded` may be. It is posted when the close happens during layout or during a teardown forced by a DP change (`ItemsSource` swap, table or column `IsReadOnly`, projection swap); a posted `CellEditEnded` is always raised before the next session's `BeginningEdit`. `CellEditEnding` for a forced close still has a live `EditingElement`. Forced closes (item removed or filtered out, `ItemsSource` replaced, recycle, unload) raise `CellEditEnded` with `EditAction == Cancel`. If the item is still present after a sort/regroup, a valid pending value is committed. Editing starts from user gestures only (double-click/tap, F2). Writes happen per cell on commit (no row transaction). Live-shaping reshapes are held while editing.

### 2.6 Grouping and hierarchy

| Member | Type | Semantics / default |
|---|---|---|
| `TableView.GroupHeaderTemplate` | `DataTemplate` | Content of the control-owned band; data context `TableViewGroupInfo`. |
| `TableViewGroupInfo` | INPC; `Key`, `ItemCount`, `IsExpandable`, `IsExpanded`, `KeyText`, `ItemCountText` | Updated in place on recycle; `KeyText`/`ItemCountText` lazily culture-formatted. |
| `TableViewGroupHeader` | unsealed `ContentControl`; `IsExpanded`, `IsExpandable` read-only DPs (`false`) | Realized band; written internally only. |
| `ExpandGroup(Object key)` / `CollapseGroup(Object key)` | `void` | The key is resolved to a group identity exactly as `GroupBy` does (built-in identity, or the source's `TableViewGroupIdentitySelector`) and matched by identity; a null key and `""` never collide. Unknown key or already in state: no-op, no event. |
| `ExpandAllGroups()` / `CollapseAllGroups()` | `void` | Moves the default (applies to groups that appear later) and resets per-group exceptions. No-op when not grouped. |
| `GroupExpanding` | args `GroupKey` | Always synchronous, before the state changes. Not cancellable. |
| `GroupCollapsed` | args `GroupKey` | After the state changes. |
| `ExpandAllRows()` / `CollapseAllRows()` | `void` | Tree rows under `ParentBy`; no-op otherwise. |
| `TableViewRow.Level` / `IsExpandable` / `IsExpanded` | read-only DPs | `Level` 1-based for tree rows, `0` otherwise (matches `AutomationProperties.Level`). |

**Group events.** Raised for **every** group whose state changes, from any source: pointer, keyboard, UIA ExpandCollapse, `ExpandGroup`/`CollapseGroup`, and the bulk methods (once per changed group). Order: `GroupExpanding` (synchronous) → state change → UIA `ExpandCollapseState` change; `GroupCollapsed` after the change. A request that arrives during layout (including the measure/arrange of rows, headers and the cells panel) posts the **whole operation** to the next dispatcher turn, so the events are never raised after the state already changed. While `GroupExpanding` is raised and the state applied, the control holds a refresh deferral on the source: a handler that changes the source (lazy load, `Filter`, `Sort`) is coalesced into one reshape, and the group is then found again by identity. Exceptions thrown by group-event handlers are contained and logged, as for `CellEditEnded`. `TableViewGroupHeader.IsExpanded`/`IsExpandable` revert an external `SetValue`; pooled headers re-prepare through internal writes that raise nothing.

**Nuances.** Expansion is intent keyed by group identity, so a collapse survives sort, filter and regroup. Grouping is single-level. Tree rows have no per-row expand API and no events (§6). Group order: see §2.1 (`SortGroups(None)` follows first items; `Ascending`/`Descending` pins key order).

### 2.7 Layout and visuals

| Member | Type | Semantics / default |
|---|---|---|
| `HeadersVisibility` | `[flags] TableViewHeadersVisibility` (`None`, `Column`), `Column` | No row headers. |
| `GridLinesVisibility` | `TableViewGridLinesVisibility` (`All = 0`, `Horizontal`, `None`, `Vertical`), `All` | WPF names and values; body lines only. |
| `Density` | `TableViewDensity` (`Standard = 0`, `Compact = 1`, `Comfortable = 2`), `Standard` | Row min height 40/30/48 and padding. |
| `RowBackground`, `AlternatingRowBackground` | `Brush`, null | Opt-in banding. |
| `EmptyTemplate` | `DataTemplate` | Shown when no rows. |
| `ColumnHeaderStyle` | `Style` | Layered over the default; no `BasedOn` needed; wrong `TargetType` ignored. |
| `CanUserResizeColumns` | `Boolean`, `true` | Table-level resize gate; user input only. |
| `CellContextFlyout` / `HeaderContextFlyout` | `FlyoutBase` | First match wins. Cell: flyout in cell content → `CellContextFlyout` → row `ContextFlyout` → `TableView.ContextFlyout`. Header: content → `HeaderContextFlyout` → `TableView.ContextFlyout`. Opening one focuses its target and does not change selection; header flyouts stay closed while a cell is editing. |
| `CellToolTipBinding` / `HeaderToolTip` | | Control owns the `ToolTip` (placement `Mouse`); value is content, not a `ToolTip`. String content is published as UIA HelpText; non-string content is mouse-only. |
| `TableViewCellsPanel` | unsealed `Panel`, public ctor | For re-templates (`PART_HeaderHost`, `PART_CellsHost`); children are control-owned. |
| `TableViewRow` | unsealed `Control`, public ctor | Realized row; TableView cannot use a derived row today. |

**Nuances.** `Density` renumbering changes persisted integers; switch on names. Frozen columns use `Translation`; only the leading prefix freezes. Template parts and visual states are documented in the API spec; MIDL does not surface `[TemplatePart]`.

### 2.8 Automation peers

| Peer | Sealed | Public ctor | Control type | Patterns (IDL) |
|---|---|---|---|---|
| `TableViewAutomationPeer` | no | `(TableView)` | `DataGrid` | `Selection`, `Grid`, `Table`, `ItemContainer` |
| `TableViewRowAutomationPeer` | no | `(TableViewRow)` | `DataItem` | `SelectionItem`, `ExpandCollapse` |
| `TableViewCellAutomationPeer` | no | `(FrameworkElement owner)`; `E_INVALIDARG` unless a TableView cell | see §6 | `GridItem`, `TableItem`, `Value` |
| `TableViewColumnHeaderAutomationPeer` | no | `(TableViewColumnHeader)` | `HeaderItem` | `Invoke` (+ `Transform`, implementation-only) |
| `TableViewGroupHeaderAutomationPeer` | no | `(TableViewGroupHeader)` | `Group` | `ExpandCollapse`, `GridItem` |

**Nuances.** Selection patterns are advertised only while `SelectionMode != None`. UIA counts only visible columns. All peers are unsealed, as every WinUI peer is (C++/WinRT composition needs it), and every public ctor takes only the owner element: the header peer derives table and column from the header, the cell peer derives row, column and column index from the cell. `FrameworkElementAutomationPeer.CreatePeerForElement` on a realized element returns the control's own peer. `ScrollItem` comes from `FrameworkElementAutomationPeer`.

### 2.9 Packaging and resources

| Item | Contract |
|---|---|
| `TabularControlsResources : ResourceDictionary` | **Required.** Apps merge it next to `XamlControlsResources`; without it the factory throws ("You must put an instance of …TabularControlsResources in your Application.Resources.MergedDictionaries"). |
| `XamlControlsTabularXamlMetaDataProvider` (`IXamlMetadataProvider`, `static Initialize()`) | XAML type metadata for the Tabular DLL. |
| `SortDirection` namespace | Declared into `Microsoft.UI.Xaml.Controls.Tabular`, not `Microsoft.UI.Xaml.Data` (C# consumers would hit `CS0234`). |
| xmlns | `using:Microsoft.UI.Xaml.Controls.Tabular` |
| Overload ABI names | `GroupBy`/`GroupByWithIdentity`, `Sort`/`SortByPath`, `ThenSort`/`ThenSortByPath` via `[default_overload]`/`[method_name]`; without them MIDL emits `*2` names. |
| Implicit surface | `TabularSurface*` and `TableView*` theme keys and template parts; which are contract is open (§6). |

## 3. Parity

Classification: **P** = parity, **I** = intentional difference (§4), **G** = gap (§6). Precedent abbreviations: WinUI = this repo; WPF = `DataGrid` + `ICollectionView`/`CollectionViewSource`; WCT = Windows Community Toolkit `DataGrid` / `AdvancedCollectionView` (ACV); 3P = WinUI.TableView (w-ahmad), Syncfusion `SfDataGrid`, Telerik `RadDataGrid`, DevExpress; Other = AG Grid, TanStack, MUI X, Qt, Swing, JavaFX, Apple. "(unv.)" = not verified in the precedent research. Sources: §3.8.

### 3.1 Data and shaping

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Where shaping lives | `TableViewSource` wrapping the collection | WinUI `ICollectionView` has no sort/filter. WPF `ICollectionView`. WCT ACV. 3P: descriptor collections on the grid. Qt proxy model, JavaFX `SortedList`/`FilteredList`. | P (concept) |
| Declaration style | Fluent, mutating, code-only delegates | WPF/ACV: mutable properties/collections, not fluent. LINQ: fluent, immutable. No WinUI IDL returns `this`. | I |
| Filter | One predicate; replaces | WPF `ICollectionView.Filter`, ACV `Filter`, DevExpress `FilterCriteria` replace. WPF `CollectionViewSource.Filter` event, Swing `andFilter`, MUI `logicOperator`, Syncfusion `FilterPredicates` combine. | P (WPF) |
| Multi-level sort | `Sort` replaces, `ThenSort` appends | LINQ `OrderBy`/`ThenBy`. WPF/ACV `SortDescriptions`, AG Grid `sortIndex`, TanStack `SortingState[]`, Swing `SortKey` list, `NSSortDescriptor[]`, w-ahmad `SortDescriptions`. | P (LINQ); descriptor list I |
| Group order | `SortGroups(None)`: by first item in the item-sorted sequence; `SortGroups(Asc/Desc)`: by key | WPF `ListCollectionView` without group sort descriptions orders groups by first item; WPF `GroupDescription.SortDescriptions`/`CustomSort`; w-ahmad `GroupDescription.SortMode`/`Direction`; Telerik `GroupDescriptor.SortOrder`; AG Grid orders by the grouped column's sort. | P (separate declaration); by-count or custom group order G |
| Refresh / batching | `Refresh()`, `IClosable DeferRefresh()` | WPF `Refresh`/`DeferRefresh` (`IDisposable`), ACV `DeferRefresh`; Syncfusion `BeginInit`/`EndInit`; DevExpress `BeginDataUpdate`; w-ahmad `RefreshSorting`/`RefreshFilter`/`RefreshGrouping`; AG Grid transactions; Qt `invalidate()`. No WinUI IDL has a defer API. | P (WPF) |
| Live shaping | One `IsLiveShaping`, default off | WPF `IsLiveSorting/Filtering/Grouping` + property lists; Qt `dynamicSortFilter` (default on); Swing `sortsOnUpdates` (default off); JavaFX needs an extractor. | I (single switch); property scoping G |
| Read back source | `Source` (get-only) | WPF `CollectionViewSource.Source` (settable). | P; settable G |
| Read back declared state | none | WPF `SortDescriptions`/`Filter`/`GroupDescriptions`, ACV, 3P descriptor collections. | G |
| Index mapping | `TableView.IndexOf` (projection) | WPF `CollectionView.IndexOf`/`GetItemAt`/`Count`; ACV `IndexOf`; `ItemsSourceView.IndexOf`; Swing `convertRowIndexToModel/View`; Qt `mapToSource/FromSource`; JavaFX `getSourceIndex`/`getViewIndex`. | P (item→view); view→item, `Count` G |
| Tree from flat list | `ParentBy(key, parentKey)` | DevExpress `KeyFieldName`/`ParentFieldName`; AG Grid `treeDataParentIdField`; Syncfusion `SfTreeGrid` (unv.); MUI `getTreeDataPath`. Filter keeps ancestors = AG Grid default. | P |
| Custom comparer on source | none (column-level `ITableViewSortComparer` only) | WPF `CustomSort`; w-ahmad `SortDescription(path, dir, IComparer)`; Telerik `DelegateSortDescriptor`; AG Grid `comparator`; TanStack `sortingFn`. | G (deferred, §5) |
| Errors in callbacks/data | Synchronous throw; rollback | WPF `ListCollectionView` propagates (inferred from source); ItemsRepeater throws HRESULTs; no framework documents callback-throw behaviour. | P |
| Sharing a source | Single owner | WPF shares one view across controls; Qt, JavaFX share models; AG Grid row model per grid. | I |
| Row identity | Object reference | AG Grid/TanStack/MUI `getRowId`. | G |
| Thread model | UI-thread affine | WPF view is dispatcher-affine + `EnableCollectionSynchronization`. | P |

### 3.2 Columns

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Column types | `TableViewTextColumn`, `TableViewTemplateColumn`, derivable base | WPF/WCT `DataGridTextColumn`, `DataGridTemplateColumn`, plus check-box and combo-box columns (unv.). | P; more built-ins G |
| Cell factory | `GenerateElementCore` | ItemsRepeater `ElementFactory.GetElement`/`GetElementCore`; WPF `GenerateElement`/`GenerateEditingElement` (unv.). | P |
| Commit hook | `CommitCellEditCore(FrameworkElement)` | WPF `DataGridColumn.CommitCellEdit`. | P |
| Cell refresh hook | `NotifyCellContentChanged()` | WPF `RefreshCellContent` (unv.). | P |
| Editing template | `CellEditingTemplate` on base column | WPF: template column only. | I |
| Width type | `GridLength` (Pixel/Auto/Star) | WPF `DataGridLength` (adds `SizeToHeader`/`SizeToCells`); WCT `ColumnWidth`. | I; auto-fit G |
| Resize gates | `CanUserResize` + `CanUserResizeColumns` | WPF `DataGridColumn.CanUserResize`; WCT `CanUserResizeColumns`. | P |
| Frozen | per-column `FrozenEdge.Leading` (prefix) | WPF/WCT `FrozenColumnCount`. | I; trailing G |
| Reorder | none; collection order = visual order | WCT `DisplayIndex`, `CanUserReorderColumns`, `ColumnReordered`; WPF `DisplayIndex`. | G |
| Auto-generate | none | WCT `AutoGenerateColumns`; WPF same (unv.). | G |

### 3.3 Sorting

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Header sort, cancellable event | `Sorting` (`Cancel`) / `Sorted` | WPF/WCT `Sorting` (`Handled`); w-ahmad `Sorting` (`Handled`). | P (`Cancel` follows the WinUI -ing event idiom) |
| App-owned sort with arrow | settable display-only `SortDirection` | WCT `SortDirection` settable and cosmetic only; WPF settable; w-ahmad settable; AG Grid SSRM, TanStack `manualSorting`, MUI `sortingMode="server"`. | P |
| Gate naming / scope | `CanUserSort`, `CanUserSortColumns`: user input only | WPF `CanUserSort`/`CanUserSortColumns`. | P |
| Sort cycle | `SortCycle` incl. tri-state | WPF/AG Grid cycles (unv.). | P (tri-state is an addition) |
| Multi-column header sort | none (single axis) | WPF Shift+click; w-ahmad Ctrl+click; AG Grid Shift; TanStack/MUI modifier. | G |
| Sort direction enum | `SortDirection { None, Ascending, Descending }` | WPF/WCT/ACV `ListSortDirection { Ascending = 0, Descending = 1 }`, no `None`. | I |
| Clear | `TableView.ClearSort()` (all item sorts) + `TableViewSource.ClearSort()` | WPF `SortDescriptions.Clear()`. | P |

### 3.4 Selection and navigation

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Modes | `None`, `Single` | `ItemsViewSelectionMode` (+ `Multiple`, `Extended`); WPF/WCT `Single`/`Extended`; WPF `SelectionUnit`. | I (v1); multi G |
| Selected state | read-only `SelectedItem`/`SelectedIndex`; `Select(Int32)` | ItemsView read-only + `Select`; `Selector`/ListView/WPF/WCT settable. | P (ItemsView) |
| Item → index | `IndexOf(Object)` | `ItemsView.TryGetItemIndex`; `ItemsSourceView.IndexOf`; ListViewBase `IndexFromContainer`. | P |
| Bring into view | `StartBringItemIntoView(Int32, BringIntoViewOptions)` | `ItemsView.StartBringItemIntoView`; `ListViewBase.ScrollIntoView`; WCT `ScrollIntoView(Object, DataGridColumn)`; WPF `ScrollIntoView`. | P (ItemsView); by-item/column G |
| Current cell | internal only | WCT `CurrentItem`/`CurrentColumn`/`CurrentCellChanged`; ItemsView `CurrentItemIndex`; WPF `CurrentCell` (unv.). | G |
| Selection args | platform `SelectionChangedEventArgs` | ListView/WPF/WCT same type; ItemsView own args. | P |

### 3.5 Editing

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Opt-in | `IsReadOnly = true` default | WPF/WCT default `false`. | I |
| Begin | user gestures only; `BeginningEdit` | WCT `BeginEdit()`, `BeginningEdit`, `PreparingCellForEdit`; WPF same (unv.). | P (event); `BeginEdit` G |
| Ending | `CellEditEnding` + `EditingElement` + `Cancel` | WPF `DataGridCellEditEndingEventArgs.EditingElement`; WCT `CellEditEnding`. | P |
| Ended | `CellEditEnded` | WCT `CellEditEnded`; WPF has none. | P (WCT) |
| Observable state | `IsEditing` DP | No equivalent property in WCT. | I (addition) |
| Scope | cell only, write on commit | WPF row `BindingGroup`, `RowEditEnding`; WCT `RowEditEnding`/`RowEditEnded`, `DataGridEditingUnit`. | I; row editing G |

### 3.6 Grouping and hierarchy

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Per-group expand | `ExpandGroup(key)`/`CollapseGroup(key)` | WCT `ExpandRowGroup`/`CollapseRowGroup(ICollectionViewGroup, Boolean)`; TreeView `Expand(node)`. | P |
| Expand state on element | read-only header DPs | `TreeViewNode.IsExpanded`, `Expander.IsExpanded` settable and drive expansion. | I |
| Expansion events | `GroupExpanding`/`GroupCollapsed` (`GroupKey`) | TreeView `Expanding`/`Collapsed` (`Node`, `Item`); Expander `Expanding`/`Collapsed`. | P |
| Group header content | `GroupHeaderTemplate` over `TableViewGroupInfo` | WCT `RowGroupHeaderStyles`, `LoadingRowGroup`; WPF `CollectionViewGroup` (`Name`, `ItemCount`). | P |
| Multi-level grouping | none | WPF `GroupDescriptions`; Syncfusion `GroupColumnDescriptions`; TanStack grouping array. | G |
| Tree row expand | `ExpandAllRows`/`CollapseAllRows` only | TreeView `Expand(node)`, events; AG Grid node API (unv.). | G |

### 3.7 Layout, visuals, automation, packaging

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Headers / grid lines | `HeadersVisibility` (`None`, `Column`); `GridLinesVisibility` WPF values | WPF `DataGridHeadersVisibility`, `DataGridGridLinesVisibility`. | P (no row headers: I) |
| Density | `TableViewDensity` | WCT `RowHeight`; none in WPF. | I |
| Row/cell styles | `ColumnHeaderStyle`, `HeaderStyle` only | WPF/WCT `RowStyle`, `CellStyle`, `RowHeaderStyle`. | G |
| Row lifecycle | none | WCT `LoadingRow`/`UnloadingRow`; WPF `LoadingRow`; `ItemsRepeater.ElementPrepared`/`ElementClearing` (unv.). | G |
| Context menus | `CellContextFlyout`, `HeaderContextFlyout` | WinUI `UIElement.ContextFlyout`. | P |
| Peer ctors | owner element (`TreeViewItemAutomationPeer(TreeViewItem)`) | in-repo convention; test hooks `[MUX_INTERNAL]` (`ItemsViewTestHooks.idl`). | P |
| Containers unsealed | `TableViewRow`, `TableViewGroupHeader`, panel, columns | `ItemContainer`, `TreeViewItem`, `NavigationViewItem` unsealed with ctors. | P |
| Separate DLL + resources | `TabularControlsResources` | MUXC `XamlControlsResources`. | P (name asymmetry open) |
| Incremental loading | none | WCT `IncrementalLoadingTrigger`/`Threshold`, `DataFetchSize`; ListViewBase `ISupportIncrementalLoading` (unv.). | G |

### 3.8 Sources

- WinUI (this repo): `controls/dev/ItemsView/ItemsView.idl`, `controls/dev/Repeater/ItemsRepeater.idl`, `controls/dev/TreeView/TreeView.idl`, `controls/dev/TreeView/TreeViewAutomationPeers.idl`, `controls/dev/Expander/Expander.idl`, `controls/dev/ItemsView/ItemsViewTestHooks.idl`, `controls/dev/ItemContainer/ItemContainer.idl`, `controls/dev/NavigationView/NavigationView.idl`; `ListViewBase`/`Selector` in `dxaml/.../microsoft.ui.xaml.controls.controls.idl`; `ICollectionView`/`CollectionViewSource` in `coretypes2.idl`.
- WPF: https://learn.microsoft.com/dotnet/api/system.componentmodel.groupdescription.sortdescriptions, https://learn.microsoft.com/dotnet/api/system.componentmodel.icollectionview.sortdescriptions, https://learn.microsoft.com/dotnet/api/system.componentmodel.icollectionview.filter, https://learn.microsoft.com/dotnet/api/system.windows.data.collectionviewsource.filter, https://learn.microsoft.com/dotnet/api/system.componentmodel.icollectionview.deferrefresh, https://learn.microsoft.com/dotnet/api/system.windows.data.collectionviewsource.source, https://learn.microsoft.com/dotnet/api/system.componentmodel.listsortdirection, https://learn.microsoft.com/dotnet/api/system.windows.data.bindingoperations.enablecollectionsynchronization, https://learn.microsoft.com/dotnet/api/system.windows.controls.datagrid.sorting; source https://github.com/dotnet/wpf (`ListCollectionView.cs`, `DataGridColumnHeader.cs`).
- WCT: https://learn.microsoft.com/dotnet/api/microsoft.toolkit.uwp.ui.controls.datagrid, https://learn.microsoft.com/dotnet/api/microsoft.toolkit.uwp.ui.controls.datagridcolumn.sortdirection, https://learn.microsoft.com/windows/communitytoolkit/controls/datagrid_guidance/group_sort_filter; ACV: CommunityToolkit/Windows `components/Collections/src/AdvancedCollectionView/AdvancedCollectionView.cs`.
- 3P: https://w-ahmad.dev/WinUI.TableView/docs/grouping.html, https://w-ahmad.dev/WinUI.TableView/docs/sorting.html, https://w-ahmad.dev/WinUI.TableView/docs/filtering.html; https://help.syncfusion.com/winui/datagrid/grouping, https://help.syncfusion.com/winui/datagrid/filtering, https://help.syncfusion.com/winui/datagrid/sorting. Telerik and DevExpress: WPF documentation only; WinUI editions unverified.
- Other: https://www.ag-grid.com/javascript-data-grid/grouping-sorting/, https://www.ag-grid.com/javascript-data-grid/row-sorting/, https://www.ag-grid.com/javascript-data-grid/row-ids/, https://www.ag-grid.com/javascript-data-grid/tree-data/, https://www.ag-grid.com/javascript-data-grid/tree-data-filtering/, https://www.ag-grid.com/javascript-data-grid/data-update-transactions/, https://www.ag-grid.com/javascript-data-grid/server-side-model-sorting/; https://tanstack.com/table/v8/docs/guide/sorting, https://tanstack.com/table/v8/docs/guide/grouping; https://mui.com/x/react-data-grid/filtering/, https://mui.com/x/react-data-grid/tree-data/, https://mui.com/x/react-data-grid/server-side-data/; https://doc.qt.io/qt-6/qsortfilterproxymodel.html; https://docs.oracle.com/javase/8/docs/api/javax/swing/table/TableRowSorter.html, https://docs.oracle.com/javase/8/docs/api/javax/swing/RowFilter.html; https://openjfx.io/javadoc/21/javafx.base/javafx/collections/transformation/SortedList.html; https://developer.apple.com/documentation/foundation/nssortdescriptor.

## 4. Intentional differences and rationale

| Difference | Rationale |
|---|---|
| Shaping is code-first on `TableViewSource` (delegates), not XAML descriptors | WinRT has no generics, and delegates are not XAML-declarable; WinUI `ICollectionView` has no sort/filter to build on. Delegates for stateless callbacks match `ListViewItemToKeyHandler`. Key selectors also avoid reflection paths that fail silently or under AOT. |
| Fluent calls mutate and return `this` | Lets a declaration read as one chain while keeping one live object the control can own. Documented explicitly because it is novel to WinUI. |
| `Sort`/`ThenSort` instead of a descriptor collection | Closest precedent to "replace vs. append" (LINQ); avoids exposing a mutable collection type that would need its own change semantics. Read-back stays open (§6). |
| `SortGroups(direction)`: first-item order by default, key order when pinned | Group order must not depend on call order. The `None` default matches WPF `ListCollectionView`; an app that wants groups to stay put under header sorts pins key order. Ordering by count or by a custom comparer can be added as overloads. |
| New `SortDirection` with `None = 0` | `ListSortDirection` has no `None` and is not a WinRT type; `None = 0` follows the WinUI zero-value convention. Shared by the headless `TabularShaping` layer, not TableView-specific. |
| One `IsLiveShaping` switch | Simpler than WPF's three switches plus property lists; scoping can be added additively. |
| Single selection, `TableViewSelectionMode { None, Single }` | Multi-select needs an anchor/range model and gesture plumbing. A TableView-specific enum keeps `Multiple` from compiling and silently acting as `Single`; values match `ItemsViewSelectionMode`. |
| Read-only `SelectedItem`/`SelectedIndex`, index-based `Select` | `ItemsView` parity; avoids `Selector`'s deferral/coercion layer. `IndexOf` closes the item→index gap. |
| Projection index space includes group headers | Headers are rows in the virtualized list; one index space serves selection, reveal and UIA. |
| `Width : GridLength` | XAML-native `Pixel`/`Auto`/`*`. Auto semantics differ from `Grid` (grow-only, realized rows); see §6. |
| Per-column `FrozenEdge` instead of `FrozenColumnCount` | Leaves room for trailing pinning per column. |
| Display-only settable `SortDirection` | Separates sort state (arrow) from sort execution, as WCT/AG Grid/TanStack/MUI do. |
| `CanUser*` gate user input only | WPF naming and semantics; programmatic APIs are the app's own decision. |
| `IsReadOnly` default `true`, cell-scoped editing, write on commit | Display-first control; WinUI has no `BindingGroup`, so no row transaction. |
| `CellEditingTemplate` on the base column | Any column can take an editor without subclassing. |
| Single owner per `TableViewSource` | The control writes expansion, sort-axis and edit-hold state into the source. Sharing would make one table's gestures reshape another. |
| Synchronous throw, no error event | WinUI (ItemsRepeater) and WPF precedent; rollback keeps declared and actual state consistent. |
| Containers and columns unsealed | WinUI container convention (`ItemContainer`, `TreeViewItem`); sealing later is breaking. |
| No row headers, no column virtualization | Target is ~5–50 columns; row headers out of scope (`HeadersVisibility` has no `Row`). |

## 5. Removed and rejected alternatives

| Proposal | Origin | Decision | Reason |
|---|---|---|---|
| Seal `TableViewColumn`, or add a cell-refresh hook | IDL feedback #6 | Hook added (`NotifyCellContentChanged`); stays unsealed | Derivation is a supported scenario. |
| Seal `TableViewTextColumn`/`TemplateColumn`, `TableViewRow`, `TableViewCellsPanel` | IDL feedback #7, #8, #29; stability review | Not taken | WinUI keeps containers and panels unsealed (`ItemContainer`, `TreeViewItem`, `NavigationViewItem`, `WrapPanel`). The gap is a row factory, not sealing (§6). |
| `ShapingFailed` event | Stability review; TVS feedback A9 | Rejected | No WinUI collection control has a failure event; WPF and ItemsRepeater throw synchronously. Replaced by consistent throws + rollback. |
| `Sort(ITableViewSortComparer, …)` / `ThenSort(ITableViewSortComparer, …)` | IDL feedback #26; first draft of this PR | Deferred | Not in the final IDL; additive later. |
| `HasFilter`/`HasSort`/`HasGroupBy`/`HasParentBy` | TVS feedback A6; first draft of this PR | Removed | Unobservable (no change notification, so not bindable) and insufficient as read-back: they expose presence, not the declared state. Full read-back stays open (§6). |
| Settable `TableViewGroupHeader.IsExpanded` routed to the owner | TVS feedback (TreeView/Expander precedent); first draft of this PR | Rejected: read-only + `ExpandGroup`/`CollapseGroup` | Headers are pooled and re-created on every reshape; expansion is intent keyed by group identity, so the API takes a key on the owner. |
| Remove `TableViewGroupInfo.ItemCountText` | IDL feedback #23; first draft of this PR | Kept | A localized, culture-formatted string the group header template binds; `{Binding}` cannot reproduce the localized format. |
| Remove `TableViewGroupInfo.IsExpandable` | IDL feedback #24 (optional) | Kept | Not constant: empty groups and hierarchical groups make it vary; drives the `NotExpandable` state. |
| `EditingElement` on `TableViewCellEditEndedEventArgs` | First draft of this PR | Removed | By the time `CellEditEnded` is raised the editor has been torn down or recycled; exposing it would be a use-after-recycle hazard. |
| `CellEditEnding` after the write with rollback on `Cancel`; `ProposedValue` | IDL feedback #2 | Not taken | `EditingElement` matches WPF and works for any editor type. |
| `EditingChanged` event for `IsEditing` | IDL feedback #3 | Not taken | Read-only DP matches adjacent state (`SelectedIndex`, `Row.IsSelected`). |
| `GroupBy(key, SortDirection)` | TVS feedback A1 | `SortGroups` instead | A separate declaration survives `GroupBy` replacement and `ClearGroupBy`, and its `None` default can keep WPF's first-item order. |
| AND-combining `Filter` | Precedent (MUI, Swing) | Not taken | WPF/ACV/DevExpress replace; compose in one predicate. |
| Return `void` / immutable chain | TVS feedback A3 | Not taken | Keep fluent; document mutation. |
| Rename `SortDirection` → `TableViewSortDirection`; reuse `ListSortDirection` | TVS feedback A12; precedent P14 | Not taken | Shared shaping enum; `None` is required. |
| `Select(Object)` | IDL feedback #4 | Not taken | `IndexOf` + `Select(Int32)` keeps one selection API (ItemsView). |
| Move peer ctors to a TestHooks IDL | Stability review | Not taken; owner-element ctors instead | `(TableViewColumnHeader)` and `(FrameworkElement)` owner ctors follow the WinUI peer convention and validate the owner. |
| Seal `TableViewCellAutomationPeer`/`TableViewColumnHeaderAutomationPeer` | Stability review; first draft of this PR | Rejected | Every WinUI peer is unsealed and C++/WinRT composition requires it; apps deriving a TableView may supply derived peers. |
| Keep `TableViewFrozenEdge.Trailing` as reserved | — | Removed | A shipped no-op cannot later change behaviour. |

## 6. Open questions and future work

| Topic | Problem | Direction |
|---|---|---|
| **Width / auto-fit** | `GridLength` cannot express size-to-header/cells or shrink; no programmatic re-fit; user resize writes `Pixel` to the dragged and earlier `*` columns, replacing bindings; no `ColumnResized`. | Decide before stabilizing: keep `GridLength` + additive `AutoFit()` and stop rewriting other columns, or introduce `TableViewLength`. |
| **`Sorted` origin** | `Sorted(null, None)` means both clear-all and "app key-selector sort". | Add an origin/`IsCleared` field to `TableViewSortedEventArgs`. |
| `ITableViewSortComparer` direction | Who inverts for `Descending` is unstated; interface cannot be versioned. | State it in the IDL/spec before stabilizing. |
| **Per-row expand APIs** | Tree rows: only `ExpandAllRows`/`CollapseAllRows`; no row events. | Additive `ExpandRow(item)`/`CollapseRow(item)` + row `Expanding`/`Collapsed` mirroring the group events. Consider `ExpandAllNodes` naming. |
| **Row lifecycle events** | No prepared/cleared events and no way to supply a derived `TableViewRow`. | `RowPrepared`/`RowClearing` (ItemsRepeater/`LoadingRow` precedent) and/or a container factory. |
| **Current cell** | No `CurrentItem`/`CurrentColumn`/`CurrentCellChanged`; no `BeginEdit`. | Add together when a scenario needs them. |
| **Multi-select** | `Multiple`/`Extended`, `SelectedItems`, anchor model. | Append enum values; add `SelectedItems` with them. |
| **Column reorder** | No `DisplayIndex`; collection order is visual and UIA order. | Lock "collection order = visual order + `MoveColumn`" vs `DisplayIndex` before reorder ships. |
| **Incremental loading** | No `ISupportIncrementalLoading` story. | Define how the shaped projection interacts with partial data. |
| **`CollectionViewSource` interop** | `ICollectionView` consumed flat; `CollectionViewSource` throws. | Document migration; decide whether to honour `ICollectionView` groups. |
| **Row identity selector** | Object-reference identity breaks for re-created items and possibly re-boxed primitives (untested). | Optional `getRowId`-style selector. |
| Declared-state read-back | No way to read the current filter/sort/group declarations. | Descriptor read-back or getters. |
| Source comparer overloads | §5. | Additive. |
| Throwing callbacks | Five different behaviours today (filter hides row, sort key → null, …). | One documented policy; keep the original error. |
| Key types | Non-WinRT keys fall back to text/address comparison. | Document; diagnostic on fallback. |
| Shared source | Single owner is silent. | Debug diagnostic, or allow sharing. |
| Live shaping cost | Any `PropertyChanged` re-evaluates every callback; single-item changes rebuild under Filter/GroupBy/ParentBy. | Property scoping; incremental paths. |
| Callback lifetime | Possible page leak through native-held C# callbacks. | GC-tracked references or documentation; lifetime test. |
| Cell UIA control type | API spec says `Custom` ("cell"); implementation and dev spec use `DataItem`. | Decide with Narrator/Axe evidence. |
| Peer `ClassName` | Mixed fully qualified vs short names. | Pick one before stabilizing. |
| Theme keys / template parts | Which `TabularSurface*`/`TableView*` keys and parts are contract. | Publish the supported list. |
| `TableViewRowTemplateSelector` | `[MUX_INTERNAL]` type in the public namespace. | Move to `Microsoft.UI.Private.Controls`. |
| Tabular resource/provider names | `TabularControlsResources` vs `XamlControlsTabularXamlMetaDataProvider` vs MUXC names. | Settle before stabilizing. |
