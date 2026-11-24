# TableView API review

| | |
|---|---|
| **Status** | Proposal for API review |
| **Surface** | `controls/dev/TableView/TableView.idl`, `controls/dev/TabularShaping/SortDirection.idl`, plus `TabularControlsResources` and `XamlControlsTabularXamlMetaDataProvider` from `controls/dev/dll-tabular/Microsoft.UI.Xaml.Controls.Tabular.idl` |
| **Namespace / binary** | `Microsoft.UI.Xaml.Controls.Tabular`, `Microsoft.UI.Xaml.Controls.Tabular.dll`. Every type is `[MUX_PREVIEW]`. |
| **Companion docs** | [API spec](../../api-specs/TableView/TableView-spec.md), [dev spec](TableView-dev-spec.md), [functional spec](TableView-functional-spec.md), [hierarchy API spec](Hierarchical-Data-Rows-API-Spec.md) |

The IDLs are the source of truth for shape. This document reviews the whole proposed surface, so its sorting, grouping, selection-lookup and data sections (§1.2, §2.1, §2.3, §2.4 `IndexOf`/`StartBringItemIntoView`, §2.6, and the matching rows of §3 to §5) describe the proposed data-shaping surface; members of it that this branch's IDL does not have are marked *(proposed; not in this IDL)*. Where a contract depends on implementation that has not landed or been validated at run time, it says so.

## 1. Purpose and scope

TableView is preview API. This review checks whether the proposed surface is ready to stabilize: whether it is complete for the core scenarios, consistent with WinUI conventions, and has precise contracts. Preview-only breaking changes are cheap now and impossible after stabilization, so this proposal makes them now.

**In scope:** every public type in `TableView.idl` and `SortDirection.idl`, and the two Tabular DLL types. `TableViewSource` is out of scope: the proposed data-shaping surface removes it in favour of `CollectionViewSource`, and data shaping is reviewed with that surface. **Out of scope:** the framework-internal primitives `ResizeGripper` and `SortIndicator` (in `Microsoft.UI.Private.Controls`), the `[MUX_INTERNAL]` `TableViewRowTemplateSelector`, and the implicit template/theme-key surface (see §2.9 and §6).

### 1.1 Non-data cleanups

| | Member | Reason |
|---|---|---|
| Added | `TableViewColumn.NotifyCellContentChanged()` (protected) | Derived columns can regenerate their cells. |
| Changed | `TableViewDensity`: `Standard = 0`, `Compact = 1`, `Comfortable = 2` | Zero value now equals the default. Persisted integers change meaning. |
| Changed | `TableViewColumnHeaderAutomationPeer` ctor `(TableView, TableViewColumn)` → `(TableViewColumnHeader owner)` | Owner is the element (WinUI convention); old ctor made the TableView the owner. A header the table has not attached to one of its columns, or one a header rebuild has replaced, throws `E_INVALIDARG`. Stays unsealed. |
| Changed | `TableViewCellAutomationPeer` ctor `(FrameworkElement, TableViewRow, TableViewColumn, Int32)` → `(FrameworkElement owner)` | Owner-only, like every WinUI peer; row, column and index are derived from the cell, and a non-cell owner throws `E_INVALIDARG`. Stays unsealed. |
| Changed | `TableViewColumn.CanResize` → `CanUserResize` | Matches `CanUserResizeColumns` and WPF. Gates user input only (pointer, keyboard, UIA Transform). A Star column with `CanUserResize=false` keeps its authored share whatever `CanUserResizeColumns` is, and toggling `CanUserResizeColumns` moves no column. |
| Removed | `TableViewFrozenEdge.Trailing` | Reserved no-op; appending it later is compatible, changing a shipped no-op is not. |
| Removed | public `TableViewColumn.GenerateElement(Object)` | Only `GenerateElementCore` is an extension point. |

### 1.2 Data operations

These members are part of the proposed data-shaping surface.

| | Member | Reason |
|---|---|---|
| Added | `TableView.IndexOf(Object)`, `StartBringItemIntoView(Int32, BringIntoViewOptions)` (proposed; not in this IDL) | Map an item to the selection index space and reveal it; previously no way to select or show "this record" after shaping. |
| Added | `TableView.ExpandGroup(Object)`, `CollapseGroup(Object)`, `GroupExpanding`, `GroupCollapsed` + args (`GroupKey`) (proposed; not in this IDL) | Per-group programmatic expansion and a notification for every group state change. |
| Changed | `TableViewColumn.SortDirection` `{ get; }` → `{ get; set; }` (proposed; not in this IDL) | App-owned (e.g. server) sorts can show the arrow. Display only. |
| Changed | `TableViewGroupHeader.IsExpanded`/`IsExpandable` → read-only (proposed; not in this IDL) | Setters changed visuals but not rows, so UI/UIA could disagree with data. |
| Changed | `TableViewColumn.CanSort` → `CanUserSort` (proposed; not in this IDL) | Matches `CanUserSortColumns` and WPF. Gates user input only: programmatic sorts ignore it, and turning it off does not clear the active sort. |
| Removed | `TableViewGroupHeader.ToggleRequested`, `TableViewGroupHeaderToggleRequestedEventArgs` (proposed; not in this IDL) | Internal header→owner wiring; headers are control-created so apps could not subscribe usefully. Replaced by TableView-level events. |
| Removed | `TableViewGroupInfo.Level` (proposed; not in this IDL) | Always 0 and 0-based while `TableViewRow.Level` is 1-based. Re-add with nested grouping. |

### 1.3 Editing removal

| | Member | Reason |
|---|---|---|
| Added | `TableViewColumn.DisplayMemberBinding` (`Binding`, CLR) | Restores the template-free "bind a collection, render data cleanly" baseline without a new column type, after `TableViewTextColumn` was cut: a read-only `TextBlock` cell bound against the row item, whose `Path` is the sort key when `SortMemberPath` is empty. WPF `GridViewColumn.DisplayMemberBinding` precedent, including its precedence: `DisplayMemberBinding` over `CellTemplate`, so a template column ignores its `CellTemplate` while `DisplayMemberBinding` is set. |
| Removed | The editing surface: `TableView.IsReadOnly`, `IsEditing`, `CommitEdit`, `CancelEdit`, `BeginningEdit`, `CellEditEnding`, their args, `TableViewEditAction`, `TableViewColumn.IsReadOnly`, `CellEditingTemplate` | Not part of TableView (§2.5). TableView is always read-only, as the old default `IsReadOnly = true`. |
| Removed | `TableViewTextColumn` (and `Binding`) | Not part of TableView (§5). A display-only text cell is `TableViewColumn.DisplayMemberBinding`; a styled one is a `TableViewTemplateColumn` whose `CellTemplate` is a bound `TextBlock`, with `SortMemberPath` set. An unrealized row's UIA name comes from the visible columns' `DisplayMemberBinding` values, as its realized cells show them. |
| Removed | `IValueProvider` on `TableViewCellAutomationPeer` (`Value`, `IsReadOnly`, `SetValue`) | On `main` the cell exposed `IValueProvider` only while editing was enabled (`SupportsValuePattern`); with editing removed it has no purpose. A cell's text is its UIA `Name`; screen readers navigate cells through the `Grid`/`Table` patterns. |

## 2. API surface by area

### 2.1 Data and shaping: `TableViewSource`

Data shaping (sort levels, group order, refresh and deferral, re-entrancy, rollback on failure, enum keys) is part of the proposed data-shaping surface, which moves it to `CollectionViewSource` and removes `TableViewSource`; it is reviewed with that surface.

### 2.2 Columns

| Member | Type | Semantics / default |
|---|---|---|
| `TableViewColumn` | unsealed `DependencyObject`, public ctor, content property `Header` | Base column. |
| `Header`, `HeaderTemplate`, `HeaderTemplateSelector`, `HeaderStyle`, `HeaderToolTip` | DPs | Selector wins over template. `HeaderStyle` wins over `TableView.ColumnHeaderStyle`. |
| `Width` | `GridLength`, 120 px | `Pixel` exact; `Auto` = widest realized cell (grows within a data set) or current header; `*` = share of body viewport after fixed columns. |
| `MinWidth` / `MaxWidth` | `Double`, 20 / ∞ | Clamp every mode. |
| `ActualWidth` | `Double` (get), 120 | Realized width. |
| `CanUserResize` | `Boolean`, `true` | User resize (pointer, keyboard, UIA Transform) for this column, ANDed with `TableView.CanUserResizeColumns`. A Star column with `CanUserResize=false` keeps its authored share (resizing other columns does not move it) whether or not `CanUserResizeColumns` is set; `TableView.CanUserResizeColumns` gates input only, so toggling it moves no column; toggling a column's `CanUserResize` locks or unlocks that column, which redistributes the Star space. |
| Sort members | — | See §2.3. |
| `Visibility` | `Visibility`, `Visible` | Collapsed columns are absent from layout and from the UIA grid. |
| `FrozenEdge` | `TableViewFrozenEdge`, `None` | `Leading` pins only within the contiguous prefix from column 0. Out-of-range values act as `None`. |
| `DisplayMemberBinding` | `Binding` (CLR) | Read-only `TextBlock` cell bound against the row item (centred, density padding, ellipsis); `null` → the base column's cell is empty. Takes precedence over `CellTemplate` on a template column, as `GridViewColumn.DisplayMemberBinding` does in WPF. Its `Path` is the sort key when `SortMemberPath` is empty and the binding has no `Source`/`RelativeSource`/`ElementName`. Changing it regenerates realized cells. |
| `CellToolTipBinding` | `Binding` (CLR) | Per-cell tooltip content, evaluated against the row item. |
| `CellContextFlyout`, `HeaderContextFlyout` | `FlyoutBase` | See §2.7. |
| `overridable GenerateElementCore(Object)` | `FrameworkElement` | Creates the cell element; the base returns the `DisplayMemberBinding` cell or `null`. `dataItem` is for initial setup only; content must bind to the inherited `DataContext` because rows recycle. |
| `protected NotifyCellContentChanged()` | `void` | Regenerates realized cells for this column and resets `Auto` width. Once the table is in a live tree (has a `XamlRoot`), regeneration is always asynchronous: it is posted once per dispatcher turn, however many times it is requested. Until then it is immediate. `DisplayMemberBinding`, `CellTemplate` and `CellToolTipBinding` changes regenerate the same way. |
| `overridable GetSortMemberPathCore()` | `String` | Effective sort path; the base returns `SortMemberPath`, else `DisplayMemberBinding`'s path, and an app-derived column can override it. |
| `TableViewTemplateColumn.CellTemplate` | `DataTemplate` DP | Ignored while `DisplayMemberBinding` is set (WPF `GridViewColumn` precedence: `DisplayMemberBinding`, `CellTemplate`, `CellTemplateSelector`). Both null → empty cell (never `ToString()`). |
| `TableViewColumnHeader` | sealed `ContentControl`, no ctor; `Column` (get) | `TargetType` for header styles. A header replaced by a header rebuild reads `Column == null`. |
| `TableView.Columns` | `IVector<TableViewColumn>` (get) | Collection order = visual and UIA order. |

**Nuances.** Auto widths reset on `ItemsSource` assignment, `Columns` replacement, or a column's header/template change, not on in-place `Reset`, refilter or `Density`; re-fit currently needs re-assigning `ItemsSource` (§6). A completed user resize writes local `Pixel` widths to the dragged column and to earlier `*` columns, which replaces bindings on those columns (§6).

### 2.3 Sorting

| Member | Type | Semantics / default |
|---|---|---|
| `TableView.CanUserSortColumns` | `Boolean`, `true` | Gates **user input only** for the whole table. Turning it off does not clear the active sort. |
| `TableViewColumn.CanUserSort` (proposed; not in this IDL) | `Boolean`, `true` | Gates **user input only** (pointer, keyboard, UIA Invoke on the header). Turning it off does not clear an active sort on the column. |
| `TableViewColumn.SortDirection` (settable (proposed; not in this IDL)) | `SortDirection`, `None` | **Display only.** Set by the control on a column sort; settable by the app, including in XAML or before the column is added (honoured when it joins `Columns`; the last added wins). |
| `TableViewColumn.SortMemberPath` | `String` | Path for a column sort; empty → `GetSortMemberPathCore()` (the base falls back to `DisplayMemberBinding`'s path). |
| `TableViewColumn.SortCycle` | `TableViewSortCycle`, `AscendingDescending` | Header/`ToggleSortDirection` cycle; `*None` variants add a step back to source order. |
| `TableViewColumn.CustomSortComparer` | `ITableViewSortComparer` | `Int32 Compare(Object left, Object right)`; wins over the path; must be pure. An exception from it aborts a header sort or `SortByColumn` before commit: the previous sort and arrow stay, `Sorting` was raised and `Sorted` is not; the fault is logged. |
| `SortByColumn(column, direction)` | `Boolean` | Declares the control's sort axis. `None` clears. |
| `ToggleSortDirection(column)` | `Boolean` | Advances the column's `SortCycle`. |
| `TableView.ClearSort()` | `Boolean` | Clears the column sort and, with it, every sort axis on the source. `false` when no column is sorted. |
| `Sorting` | `TableViewSortingEventArgs` (`Column`, `Direction`, `Cancel`) | Raised first; cancellable. |
| `Sorted` | `TableViewSortedEventArgs` (`Column`, `Direction`) | Raised after. `Column == null` means clear-all. |
| `SortDirection` enum | `None = 0`, `Ascending = 1`, `Descending = 2` | Shared with the data layer. |

**Gates.**

| Trigger | `CanUserSortColumns` | `CanUserSort` | Raises `Sorting`/`Sorted` | Reshapes data |
|---|---|---|---|---|
| Header click / Enter / Space / UIA Invoke | gates | gates | yes | yes |
| `SortByColumn` / `ToggleSortDirection` | ignored | ignored | yes | yes |
| Setting `TableViewColumn.SortDirection` | ignored | ignored | no | **no** |
| A sort the app declares on the data source | — | — | — | yes; the proposed data-shaping surface defines how it lights the arrow |
| Turning `CanUserSortColumns`/`CanUserSort` off | — | — | no | no; the active sort and arrow stay (WPF parity) |

**Contracts.** One control sort axis is in force, and the arrow never shows a losing axis. `SortByColumn`/`ToggleSortDirection` return `false` and change nothing when `Sorting` is cancelled or the column has no sort path and no comparer. How a column sort maps onto the `CollectionViewSource` and how app-declared source sorts light the arrow are part of the proposed data-shaping surface.

**Display-only `SortDirection`.** Setting it updates the arrow and UIA state and records the column as lit by the app; it never reshapes. A non-`None` value clears the arrow on every other column. The next header click cycles from the displayed value. A value preset in XAML or before attach is applied when the column joins `Columns`. Setting `CustomSortComparer` while the arrow was set by the app neither reshapes nor raises events. Typical server-sort pattern: handle `Sorting`, set `Cancel = true`, fetch, set `Column.SortDirection`.

**Nuances.** `Sorted(null, None)` means clear-all; how app-declared source sorts are reported is part of the proposed data-shaping surface (§6). The direction contract of `ITableViewSortComparer` (does the control invert for `Descending`?) is not stated in the IDL (§6).

### 2.4 Selection and navigation

| Member | Type | Semantics / default |
|---|---|---|
| `SelectionMode` | `TableViewSelectionMode` (`None = 0`, `Single = 1`), `Single` | `None` also clears the selection. |
| `SelectedItem` | `Object` (read-only DP) | |
| `SelectedIndex` | `Int32` (read-only DP), `-1` | Index in the **displayed projection**. |
| `Select(Int32)`, `Deselect(Int32)`, `IsSelected(Int32)`, `DeselectAll()` | | `ItemsView` names. `Select(<0)` clears. `Deselect` only clears if `index` is selected. `DeselectAll` works in any mode. |
| `IndexOf(Object)` (proposed; not in this IDL) | `Int32` | Item → projection index; `-1` for null, not present, filtered out, or under a collapsed group/parent. Value-typed items (boxed numbers, strings) match by value; reference items by identity. Duplicates, including equal boxed values, resolve to the first displayed match. Works before `Loaded` and after `Unloaded` (reads the source projection). A `TableViewGroupInfo` is not a data item. |
| `StartBringItemIntoView(Int32, BringIntoViewOptions)` (proposed; not in this IDL) | `void` | `ItemsView` shape. Out-of-range → `E_INVALIDARG`; `options` may be null. Asynchronous; realizes the row and lays it out before bringing its real bounds into view (ItemsView pattern); accounts for the sticky header and frozen columns; safe before `Loaded`. A pending request is cancelled when `ItemsSource` or the projection changes, or on `Unloaded`. |
| `SelectionChanged` | `TypedEventHandler<TableView, SelectionChangedEventArgs>` | Platform args; raised once after the selection settles; not raised for index shifts. |

**Projection index space.** `SelectedIndex`, `Select`, `IsSelected`, `Deselect`, `IndexOf` and `StartBringItemIntoView` all use **display-projection indexes**: the displayed rows, where **group headers take slots** and collapsed descendants are absent. It is not an `ItemsSource` index once any shaping is active. Indexes are a snapshot: expanding or collapsing a group or row, sorting, filtering, regrouping and source changes all shift them. Call `IndexOf` immediately before `Select` or `StartBringItemIntoView` rather than caching an index. Group-header slots: `Select(headerIndex)` is a silent no-op that keeps the current selection (it neither selects the header nor clears); `IsSelected(headerIndex)` is always `false`, so `Deselect(headerIndex)` does nothing; `StartBringItemIntoView(headerIndex)` is valid and brings the group header band into view; `IndexOf` never returns a header slot (a `TableViewGroupInfo` or other header object returns `-1`). An out-of-range `Select` is also a no-op, while an out-of-range `StartBringItemIntoView` throws `E_INVALIDARG`. Typical use: `var i = table.IndexOf(item); if (i >= 0) { table.Select(i); table.StartBringItemIntoView(i, null); }`.

**Nuances.** Selection follows the item across reshapes; removing it clears the selection. `Select` before `ItemsSource` is set or while `SelectionMode == None` is a no-op (not queued). Selection is stashed across unload/reload. Keyboard follows the treegrid model (dev spec); selection follows focus for Up/Down; Ctrl+click toggles.

### 2.5 Editing (not part of TableView)

TableView is a read-only table: it has no editing API, edit mode or edit gestures, and no `TableViewTextColumn`, and neither is planned (removed members: §1.3, §5). Apps that need editing build it in a `CellTemplate`; editing scenarios belong to a DataGrid-class control. The named scenarios (Task Manager, File Explorer details view, File properties) need at most a one-field rename, which an app owns by swapping the cell template (for example from F2 through a `handledEventsToo` key handler); see [Rename-in-place guidance](../../api-specs/TableView/TableView-spec.md#rename-in-place-guidance). Live controls in a `TableViewTemplateColumn.CellTemplate` are unaffected, and the cell peer has no `Value` pattern (§1.3): a cell's text is its UIA `Name`.

### 2.6 Grouping and hierarchy

| Member | Type | Semantics / default |
|---|---|---|
| `TableView.GroupHeaderTemplate` | `DataTemplate` | Content of the control-owned band; data context `TableViewGroupInfo`. |
| `TableViewGroupInfo` | INPC; `Key`, `ItemCount`, `IsExpandable`, `IsExpanded`, `KeyText`, `ItemCountText` | Updated in place on recycle; `KeyText`/`ItemCountText` lazily culture-formatted. |
| `TableViewGroupHeader` | unsealed `ContentControl`; `IsExpanded`, `IsExpandable` read-only DPs (`false`) (proposed; not in this IDL) | Realized band; written internally only. An external `SetValue` is reverted (observers may see a transient change notification); bind `OneWay` only. |
| `ExpandGroup(Object key)` / `CollapseGroup(Object key)` (proposed; not in this IDL) | `void` | The key is matched against the groups' keys by identity (key resolution with `CollectionViewSource` grouping is part of the proposed data-shaping surface); a null key and `""` never collide. Unknown key or already in state: no-op, no event. |
| `ExpandAllGroups()` / `CollapseAllGroups()` | `void` | Moves the default (applies to groups that appear later) and resets per-group exceptions. No-op when not grouped. |
| `GroupExpanding` (proposed; not in this IDL) | args `GroupKey` | Always synchronous, before the state changes. Not cancellable. |
| `GroupCollapsed` (proposed; not in this IDL) | args `GroupKey` | After the state changes. |
| `ExpandAllRows()` / `CollapseAllRows()` | `void` | Tree rows of hierarchical data (proposed data-shaping surface); no-op otherwise. |
| `TableViewRow.Level` / `IsExpandable` / `IsExpanded` | read-only DPs | `Level` 1-based for tree rows, `0` otherwise (matches `AutomationProperties.Level`). |

**Group events.** Raised for **every** group whose state changes, from any source: pointer, keyboard, UIA ExpandCollapse, `ExpandGroup`/`CollapseGroup`, and the bulk methods (once per changed group). Order: `GroupExpanding` (synchronous) → state change → UIA `ExpandCollapseState` change; `GroupCollapsed` after the change. A request that arrives during layout posts the **whole operation** to the next dispatcher turn, so the events are never raised after the state already changed. A handler that changes the source (lazy load, filtering, sorting) reshapes it before the state change; the control then finds the group again by identity in the new projection and applies the change. Exceptions thrown by group-event handlers are contained and logged. Once `GroupExpanding` has been raised the direction is pinned: the operation never toggles back after the handler, and a handler that calls `ExpandGroup` on the same group is a no-op (one `GroupExpanding`, no recursion). After the handler the control re-checks the state and the source, so a handler that swaps `ItemsSource` does not expand the new source's groups. A bulk request made while a `GroupExpanding` is being raised is queued until it finishes. `TableViewGroupHeader.IsExpanded`/`IsExpandable` revert an external `SetValue`; pooled headers re-prepare through internal writes that raise nothing.

**Nuances.** Expansion is intent keyed by group identity, so a collapse survives sort, filter and regroup. Grouping is single-level. Tree rows have no per-row expand API and no events (§6). Group order is the source's, unchanged from `main` (§2.1).

### 2.7 Layout and visuals

| Member | Type | Semantics / default |
|---|---|---|
| `HeadersVisibility` | `[flags] TableViewHeadersVisibility` (`None`, `Column`), `Column` | No row headers. |
| `GridLinesVisibility` | `TableViewGridLinesVisibility` (`All = 0`, `Horizontal`, `None`, `Vertical`), `All` | WPF names and values; body lines only. |
| `Density` | `TableViewDensity` (`Standard = 0`, `Compact = 1`, `Comfortable = 2`), `Standard` | Row min height 32/24/40 and padding. |
| `RowBackground`, `AlternatingRowBackground` | `Brush`, null | Opt-in banding. |
| `EmptyTemplate` | `DataTemplate` | Shown when no rows. |
| `ColumnHeaderStyle` | `Style` | Layered over the default; no `BasedOn` needed; wrong `TargetType` ignored. |
| `CanUserResizeColumns` | `Boolean`, `true` | Table-level resize gate; user input only. |
| `CellContextFlyout` / `HeaderContextFlyout` | `FlyoutBase` | First match wins. Cell: flyout in cell content → `CellContextFlyout` → row `ContextFlyout` → `TableView.ContextFlyout`. Header: content → `HeaderContextFlyout` → `TableView.ContextFlyout`. Opening one focuses its target and does not change selection. |
| `CellToolTipBinding` / `HeaderToolTip` | | Control owns the `ToolTip` (placement `Mouse`); value is content, not a `ToolTip`. String content is published as UIA HelpText; non-string content is mouse-only. |
| `TableViewCellsPanel` | unsealed `Panel`, public ctor | For re-templates (`PART_HeaderHost`, `PART_CellsHost`); children are control-owned. |
| `TableViewRow` | unsealed `Control`, public ctor | Realized row; TableView cannot use a derived row today. |

**Nuances.** `Density` renumbering changes persisted integers; switch on names. Frozen columns use `Translation`; only the leading prefix freezes. Template parts and visual states are documented in the API spec; MIDL does not surface `[TemplatePart]`.

### 2.8 Automation peers

| Peer | Sealed | Public ctor | Control type | Patterns (IDL) |
|---|---|---|---|---|
| `TableViewAutomationPeer` | no | `(TableView)` | `DataGrid` | `Selection`, `Grid`, `Table`, `ItemContainer` |
| `TableViewRowAutomationPeer` | no | `(TableViewRow)` | `DataItem` | `SelectionItem`, `ExpandCollapse` |
| `TableViewCellAutomationPeer` | no | `(FrameworkElement owner)`: typed `FrameworkElement` because the cell element type is internal; validated at run time (`E_INVALIDARG` unless a prepared TableView cell with a row, column and index) | `DataItem` (localized `cell`) | `GridItem`, `TableItem` |
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
| Implicit surface | `TabularSurface*` and `TableView*` theme keys and template parts; which are contract is open (§6). |

## 3. Parity

Classification: **P** = parity, **I** = intentional difference (§4), **G** = gap (§6). Precedent abbreviations: WinUI = this repo; WPF = `DataGrid` + `ICollectionView`/`CollectionViewSource`; WCT = Windows Community Toolkit `DataGrid` / `AdvancedCollectionView` (ACV); 3P = WinUI.TableView (w-ahmad), Syncfusion `SfDataGrid`, Telerik `RadDataGrid`, DevExpress; Other = AG Grid, TanStack, MUI X, Qt, Swing, JavaFX, Apple. "(unv.)" = not verified in the precedent research. Sources: §3.8.

### 3.1 Data and shaping

Data-shaping parity is reviewed with the proposed data-shaping surface. The one TableView member here:

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Index mapping | `TableView.IndexOf` (projection) | WPF `CollectionView.IndexOf`/`GetItemAt`/`Count`; ACV `IndexOf`; `ItemsSourceView.IndexOf`; Swing `convertRowIndexToModel/View`; Qt `mapToSource/FromSource`; JavaFX `getSourceIndex`/`getViewIndex`. | P (item→view); view→item, `Count` G |

### 3.2 Columns

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Column types | Base column with `DisplayMemberBinding` (read-only text), `TableViewTemplateColumn`, derivable base | WPF `GridViewColumn.DisplayMemberBinding`/`CellTemplate`; WPF/WCT `DataGridTextColumn`, `DataGridTemplateColumn`, plus check-box and combo-box columns (unv.). | P; editable text and more built-ins G |
| Cell factory | `GenerateElementCore` | ItemsRepeater `ElementFactory.GetElement`/`GetElementCore`; WPF `GenerateElement`/`GenerateEditingElement` (unv.). | P |
| Cell refresh hook | `NotifyCellContentChanged()` | WPF `RefreshCellContent` (unv.). | P |
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
| Clear | `TableView.ClearSort()` (the column sort and its source axes) | WPF `SortDescriptions.Clear()`. | P |

### 3.4 Selection and navigation

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Modes | `None`, `Single` | `ItemsViewSelectionMode` (+ `Multiple`, `Extended`); WPF/WCT `Single`/`Extended`; WPF `SelectionUnit`. | I; multi-select G |
| Selected state | read-only `SelectedItem`/`SelectedIndex`; `Select(Int32)` | ItemsView read-only + `Select`; `Selector`/ListView/WPF/WCT settable. | P (ItemsView) |
| Item → index | `IndexOf(Object)` | `ItemsView.TryGetItemIndex`; `ItemsSourceView.IndexOf`; ListViewBase `IndexFromContainer`. | P |
| Bring into view | `StartBringItemIntoView(Int32, BringIntoViewOptions)` | `ItemsView.StartBringItemIntoView`; `ListViewBase.ScrollIntoView`; WCT `ScrollIntoView(Object, DataGridColumn)`; WPF `ScrollIntoView`. | P (ItemsView); by-item/column G |
| Current cell | internal only | WCT `CurrentItem`/`CurrentColumn`/`CurrentCellChanged`; ItemsView `CurrentItemIndex`; WPF `CurrentCell` (unv.). | G |
| Selection args | platform `SelectionChangedEventArgs` | ListView/WPF/WCT same type; ItemsView own args. | P |

### 3.5 Editing

| Capability | TableView | Precedent | Class |
|---|---|---|---|
| Cell editing | none; live controls in `CellTemplate` only | WPF/WCT `DataGrid` editing (`IsReadOnly`, `BeginEdit`, `CellEditEnding`, row editing). | G (not part of TableView; DataGrid-class control, §2.5) |

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
| New `SortDirection` with `None = 0` | `ListSortDirection` has no `None` and is not a WinRT type; `None = 0` follows the WinUI zero-value convention. Shared by the headless `TabularShaping` layer, not TableView-specific. |
| Single selection, `TableViewSelectionMode { None, Single }` | Multi-select needs an anchor/range model and gesture plumbing. A TableView-specific enum keeps `Multiple` from compiling and silently acting as `Single`; values match `ItemsViewSelectionMode`. |
| Read-only `SelectedItem`/`SelectedIndex`, index-based `Select` | `ItemsView` parity; avoids `Selector`'s deferral/coercion layer. `IndexOf` closes the item→index gap. |
| Projection index space includes group headers | Headers are rows in the virtualized list; one index space serves selection, reveal and UIA. |
| `Width : GridLength` | XAML-native `Pixel`/`Auto`/`*`. Auto semantics differ from `Grid` (grow-only, realized rows); see §6. |
| Per-column `FrozenEdge` instead of `FrozenColumnCount` | Leaves room for trailing pinning per column. |
| Display-only settable `SortDirection` | Separates sort state (arrow) from sort execution, as WCT/AG Grid/TanStack/MUI do. |
| `CanUser*` gate user input only | WPF naming and semantics; programmatic APIs are the app's own decision. |
| No editing | Read-only table; the named scenarios need at most an app-owned rename (§2.5). Editing scenarios belong to a DataGrid-class control. |
| Containers and columns unsealed | WinUI container convention (`ItemContainer`, `TreeViewItem`); sealing later is breaking. |
| No row headers, no column virtualization | Target is ~5–50 columns; row headers out of scope (`HeadersVisibility` has no `Row`). |

## 5. Removed and rejected alternatives

| Proposal | Origin | Decision | Reason |
|---|---|---|---|
| Seal `TableViewColumn`, or add a cell-refresh hook | IDL feedback #6 | Hook added (`NotifyCellContentChanged`); stays unsealed | Derivation is a supported scenario. |
| Seal `TableViewTemplateColumn`, `TableViewRow`, `TableViewCellsPanel` | IDL feedback #7, #8, #29; stability review | Not taken | WinUI keeps containers and panels unsealed (`ItemContainer`, `TreeViewItem`, `NavigationViewItem`, `WrapPanel`). The gap is a row factory, not sealing (§6). |
| `TableViewTextColumn` (`Binding`, built-in `TextBox` editor) | API review 2026-11-11 | Not part of TableView | Overkill: a column type for one property. Display-only text is `TableViewColumn.DisplayMemberBinding` on the base column (WPF `GridViewColumn` precedent), so no new type; a styled text cell is a `TableViewTemplateColumn` with a bound `TextBlock` (TableView has no cell editing, §2.5). Unrealized rows are named from the visible columns' `DisplayMemberBinding` values (with their converters); columns without one contribute nothing, and a row with no such text falls back to the item's `ToString`. An editable text cell is an app-owned editor in a `CellTemplate`. |
| Settable `TableViewGroupHeader.IsExpanded` routed to the owner | TVS feedback (TreeView/Expander precedent); an earlier proposal | Rejected: read-only + `ExpandGroup`/`CollapseGroup` | Headers are pooled and re-created on every reshape; expansion is intent keyed by group identity, so the API takes a key on the owner. |
| Remove `TableViewGroupInfo.ItemCountText` | IDL feedback #23; an earlier proposal | Kept | A localized, culture-formatted string the group header template binds; `{Binding}` cannot reproduce the localized format. |
| Remove `TableViewGroupInfo.IsExpandable` | IDL feedback #24 (optional) | Kept | Not constant: empty groups and hierarchical groups make it vary; drives the `NotExpandable` state. |
| Cell editing API (`IsReadOnly`, `IsEditing`, `CommitEdit`/`CancelEdit`, edit events and args, `CellEditingTemplate`) | API review 2026-11-11 | Not part of TableView | See §2.5. |
| Rename `SortDirection` → `TableViewSortDirection`; reuse `ListSortDirection` | TVS feedback A12; precedent P14 | Not taken | Shared shaping enum; `None` is required. |
| `Select(Object)` | IDL feedback #4 | Not taken | `IndexOf` + `Select(Int32)` keeps one selection API (ItemsView). |
| Move peer ctors to a TestHooks IDL | Stability review | Not taken; owner-element ctors instead | `(TableViewColumnHeader)` and `(FrameworkElement)` owner ctors follow the WinUI peer convention and validate the owner. |
| Seal `TableViewCellAutomationPeer`/`TableViewColumnHeaderAutomationPeer` | Stability review; an earlier proposal | Rejected | Every WinUI peer is unsealed and C++/WinRT composition requires it; apps deriving a TableView may supply derived peers. |
| Keep `TableViewFrozenEdge.Trailing` as reserved | — | Removed | A shipped no-op cannot later change behaviour. |

## 6. Open questions and future work

| Topic | Problem | Direction |
|---|---|---|
| **Width / auto-fit** | `GridLength` cannot express size-to-header/cells or shrink; no programmatic re-fit; user resize writes `Pixel` to the dragged and earlier `*` columns, replacing bindings; no `ColumnResized`. | Decide before stabilizing: keep `GridLength` + additive `AutoFit()` and stop rewriting other columns, or introduce `TableViewLength`. |
| **`Sorted` origin** | `Sorted(null, None)` means clear-all; an app-declared source sort may need a distinct origin. | Add an origin/`IsCleared` field to `TableViewSortedEventArgs`. |
| `ITableViewSortComparer` direction | Who inverts for `Descending` is unstated; interface cannot be versioned. | State it in the IDL/spec before stabilizing. |
| **Per-row expand APIs** | Tree rows: only `ExpandAllRows`/`CollapseAllRows`; no row events. | Additive `ExpandRow(item)`/`CollapseRow(item)` + row `Expanding`/`Collapsed` mirroring the group events. Consider `ExpandAllNodes` naming. |
| **Row lifecycle events** | No prepared/cleared events and no way to supply a derived `TableViewRow`. | `RowPrepared`/`RowClearing` (ItemsRepeater/`LoadingRow` precedent) and/or a container factory. |
| **Current cell** | No `CurrentItem`/`CurrentColumn`/`CurrentCellChanged`. | Add together when a scenario needs them. |
| **Data-index APIs** | Every index API uses display-projection indexes (group-header slots, collapsed rows absent), which shift on any expand, collapse or reshape. | Deferred: `GetItemFromDisplayIndex`, data-index or item-based overloads of `Select`/`StartBringItemIntoView`. Today: call `IndexOf` immediately before use. |
| **Multi-select** | `Multiple`/`Extended`, `SelectedItems`, anchor model. | Append enum values; add `SelectedItems` with them. |
| **Column reorder** | No `DisplayIndex`; collection order is visual and UIA order. | Lock "collection order = visual order + `MoveColumn`" vs `DisplayIndex` before reorder ships. |
| **Incremental loading** | No `ISupportIncrementalLoading` story. | Define how the shaped projection interacts with partial data. |
| **Data shaping** | `TableViewSource` open items (declared-state read-back, source comparers, callback errors, key types, sharing, live-shaping cost, callback lifetime, `CollectionViewSource` interop, row identity selector). | Handled by the proposed data-shaping surface, which moves data shaping to `CollectionViewSource`. |
| Peer `ClassName` | Mixed fully qualified vs short names. | Pick one before stabilizing. |
| Theme keys / template parts | Which `TabularSurface*`/`TableView*` keys and parts are contract. | Publish the supported list. |
| `TableViewRowTemplateSelector` | `[MUX_INTERNAL]` type in the public namespace. | Move to `Microsoft.UI.Private.Controls`. |
| Tabular resource/provider names | `TabularControlsResources` vs `XamlControlsTabularXamlMetaDataProvider` vs MUXC names. | Settle before stabilizing. |
