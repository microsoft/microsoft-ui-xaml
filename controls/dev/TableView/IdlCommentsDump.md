# TableView IDL comments dump

Comments moved out of the TableView-effort IDL files so the IDLs match the comment density of the
other controls' IDLs. Entries are grouped by source file and keyed by the type/member they documented.
Comment text is carried over from the IDLs; section-divider comments were dropped, and multi-line
blocks are shown de-indented. "Original" line numbers refer to the files at origin/main ebf444c10.

## controls/dev/TableView/TableView.idl

### TableViewFrozenEdge

Original: `controls/dev/TableView/TableView.idl` line 4

```
// Column pinning edge; None scrolls naturally, Leading pins left, Trailing is reserved.
```

### TableViewSelectionMode

Original: `controls/dev/TableView/TableView.idl` line 14

```
// Only None and Single this release. Multiple/Extended need an anchor and a range model; members
// are appended, so these values stay stable when they arrive.
```

### TableViewHeadersVisibility

Original: `controls/dev/TableView/TableView.idl` line 24

```
// Column-header visibility (None/Column). Mirrors the Column slot of WPF
// DataGridHeadersVisibility; row headers are out of scope, so Row/All are not defined.
```

### TableViewGridLinesVisibility

Original: `controls/dev/TableView/TableView.idl` line 35

```
// Mirrors WPF DataGridGridLinesVisibility names and values for persisted casts.
```

### TableViewDensity

Original: `controls/dev/TableView/TableView.idl` line 46

```
// Density preset for row height and built-in cell/header padding resources.
```

### TableViewEditAction

Original: `controls/dev/TableView/TableView.idl` line 56

```
// How an edit closed. A forced close reports Cancel, writing nothing, when the item was removed,
// filtered out, or ItemsSource was replaced; an item still present commits a valid pending value.
```

### TableViewSortCycle

Original: `controls/dev/TableView/TableView.idl` line 66

```
// Direction sequence a column's header walks on repeated clicks. The first pair member is the
// direction an unsorted column opens in: Ascending* suits names and categories, Descending* suits
// metrics where the interesting rows are the largest (CPU, memory, size). The *None variants add a
// trailing unsorted step, so a third click returns the column to source order instead of going
// straight back to the opening direction.
```

### ITableViewSortComparer

Original: `controls/dev/TableView/TableView.idl` line 81

```
// Orders two row items. Return <0, 0 or >0 like a classic three-way comparison. The comparer must
// be a pure function of its inputs: re-entering the control from it is an app bug.
// An interface rather than a delegate so a comparer can be declared as a XAML resource and assigned
// in markup.
```

### Namespace scope (between ITableViewSortComparer and TableViewColumn): item-owned edit transaction

Original: `controls/dev/TableView/TableView.idl` line 92

```
// Item-owned edit transaction (the WinRT counterpart of IEditableObject) is deliberately NOT in
// this release. It is the rollback mechanism for ROW-scoped editing, and editing is cell-scoped
// here, so shipping it now would be public surface with nothing behind it. It lands with row
// editing, when it has a job to do.
```

### TableViewColumn.TableViewColumn

Original: `controls/dev/TableView/TableView.idl` line 103

```
// Required for derivation: the overridable cell factories below are only reachable if a
// consumer can actually construct a derived column.
```

### TableViewColumn.HeaderTemplate

Original: `controls/dev/TableView/TableView.idl` line 110

```
// Optional header template; HeaderTemplateSelector takes precedence.
```

### TableViewColumn.HeaderToolTip

Original: `controls/dev/TableView/TableView.idl` line 117

```
// Opt-in header tooltip content; null or empty means no tooltip. Not a Binding property: a
// header is not bound against a row, so there is nothing to defer.
```

### TableViewColumn.HeaderStyle

Original: `controls/dev/TableView/TableView.idl` line 122

```
// Style for this column's TableViewColumnHeader; wins over TableView.ColumnHeaderStyle.
```

### TableViewColumn.Width

Original: `controls/dev/TableView/TableView.idl` line 126

```
// Width carries GridLength intent; ActualWidth is the resolved, MinWidth/MaxWidth-clamped
// pixels (see TableView_Layout.cpp). Pixel = exact; Auto = widest realized cell (grows within a
// data set); Star = a proportional share of the body viewport after fixed columns.
```

### TableViewColumn.CanResize

Original: `controls/dev/TableView/TableView.idl` line 141

```
// Gates the resize affordance for this column only; the owner's CanUserResizeColumns gates all of them.
```

### TableViewColumn.ActualWidth

Original: `controls/dev/TableView/TableView.idl` line 147

```
// Read-only DP written by the column's width resolve; the header host and realized cell panels
// arrange each cell to this width (no per-cell binding).
```

### TableViewColumn.FrozenEdge

Original: `controls/dev/TableView/TableView.idl` line 152

```
// Pins the column during horizontal scroll; Leading is implemented, Trailing reserved.
```

### TableViewColumn.CanSort

Original: `controls/dev/TableView/TableView.idl` line 161

```
// Per-column sort opt-out (default true). False blocks header clicks, SortByColumn (except None) and
// ToggleSortDirection; setting it false on the sorted column clears that sort.
```

### TableViewColumn.SortCycle

Original: `controls/dev/TableView/TableView.idl` line 167

```
// Direction sequence repeated header clicks walk for THIS column. Per-column because the cycle
// describes how one column responds to being clicked again, and a table can reasonably mix
// policies - a metric column that should open on its largest values alongside a name column
// that should open A-Z. Read at click time, so a change needs no rebuild.
```

### TableViewColumn.SortMemberPath

Original: `controls/dev/TableView/TableView.idl` line 174

```
// Property name on the row item used to extract the sort key. When empty, TableViewTextColumn
// falls back to its Binding.Path.Path; the base column returns an empty string and the column
// is only sortable if it supplies a CustomSortComparer.
//
// The path need not be unique. Several columns may name the same property - a common shape when
// one column formats a value and another ranks it. Each column sorts as its own independent
// axis, keyed on column identity rather than on this path, so duplicates neither collide nor
// prevent the table from being built. Sorting one of them leaves the others' SortDirection at
// None, since sorting is single-column: applying a sort clears any previous one.
```

### TableViewColumn.CustomSortComparer

Original: `controls/dev/TableView/TableView.idl` line 185

```
// Optional comparer supplied by the column for custom sort ordering. Takes precedence over
// SortMemberPath. With TableViewSource.IsLiveShaping, changed items are re-compared and moved.
```

### TableViewColumn.SortDirection

Original: `controls/dev/TableView/TableView.idl` line 189

```
// Read-only DP driven by the owning TableView. None means the column is not part of the
// active sort. Apps change it through the TableView sort methods or TableViewSource.Sort.
```

### TableViewColumn.GetSortMemberPathCore

Original: `controls/dev/TableView/TableView.idl` line 194

```
// Returns the effective sort key path. The base column returns SortMemberPath verbatim;
// TableViewTextColumn overrides to fall back to Binding.Path.Path when SortMemberPath is empty.
```

### TableViewColumn.GenerateElement

Original: `controls/dev/TableView/TableView.idl` line 198

```
// Generates the display-only visual root for a cell.
```

### TableViewColumn.GenerateElementCore

Original: `controls/dev/TableView/TableView.idl` line 201

```
// Overridable cell factory. Custom columns return the display visual for a cell.
// The returned element must bind reactively to its inherited DataContext (the row data item) and
// must not set a local DataContext: rows are recycled and their DataContext is updated in place,
// so cells refresh through inheritance/bindings. dataItem is provided for initial setup only;
// baking it in as static content (or a local DataContext) will show stale data after recycle.
```

### TableViewColumn.IsReadOnly

Original: `controls/dev/TableView/TableView.idl` line 208

```
// Per-column opt-out. A read-only column can still be the current cell, but cannot be edited.
```

### TableViewColumn.CellEditingTemplate

Original: `controls/dev/TableView/TableView.idl` line 213

```
// Editing visual. A column with no CellEditingTemplate and no built-in editor is not editable.
```

### TableViewColumn.CellToolTipBinding

Original: `controls/dev/TableView/TableView.idl` line 217

```
// Opt-in per-cell tooltip content, bound against the row's data item; null or empty means no
// tooltip. A CLR property, not a DP, so XAML passes the Binding through unevaluated.
```

### TableViewColumn.CellContextFlyout

Original: `controls/dev/TableView/TableView.idl` line 221

```
// Shown at the cell, with the row's item as its items' DataContext; shared by the column's cells.
// First match wins: a ContextFlyout inside the cell (not on a nested items host's items or
// anywhere in a nested TableView; on text controls only one set on the element itself),
// CellContextFlyout, the row's ContextFlyout, then TableView.ContextFlyout. The cell takes
// focus; selection does not change. No TableView menu opens while a cell is editing. Group
// headers never use it.
```

### TableViewColumn.HeaderContextFlyout

Original: `controls/dev/TableView/TableView.idl` line 229

```
// Shown at the header cell, after a Style/HeaderTemplate ContextFlyout and before
// TableView.ContextFlyout. Not opened for collapsed columns or while editing.
```

### TableViewColumn (editing hooks note, before the DependencyProperty statics)

Original: `controls/dev/TableView/TableView.idl` line 233

```
// The editing hooks a derived column would override - GenerateEditingElement, PrepareCellForEdit,
// CommitCellEdit, CancelCellEdit - are internal in this release. The built-in columns use them,
// but a CellEditingTemplate is the supported way to customise an editor here, so publishing the
// hooks now would fix their exact shape forever. They can be added later without breaking anyone.
```

### TableViewSortingEventArgs.Column

Original: `controls/dev/TableView/TableView.idl` line 265

```
// The trigger column. Null is the clear-all sentinel used by ClearSort.
```

### TableViewSortingEventArgs.Direction

Original: `controls/dev/TableView/TableView.idl` line 268

```
// The direction the control is about to apply.
```

### TableViewSortingEventArgs.Cancel

Original: `controls/dev/TableView/TableView.idl` line 271

```
// Set true to stop the control applying the sort, leaving the app to own the ordering. The
// control's sort state, glyph and Sorted are suppressed; TableViewSource.Sort can light the glyph.
```

### TableViewSortedEventArgs.Column

Original: `controls/dev/TableView/TableView.idl` line 280

```
// The trigger column for this sort-state change. Null is the clear-all sentinel.
```

### TableViewSortedEventArgs.Direction

Original: `controls/dev/TableView/TableView.idl` line 283

```
// The direction now applied to the column.
```

### TableViewBeginningEditEventArgs / TableViewCellEditEndingEventArgs (Cancel semantics)

Original: `controls/dev/TableView/TableView.idl` line 287

```
// Cancel is read synchronously, right after the handler returns, so a handler must decide before
// returning. Asynchronous validation would need a deferral, which is not in this release.
```

### TableViewTextColumn.Binding

Original: `controls/dev/TableView/TableView.idl` line 317

```
// CLR property so Binding-typed XAML values route through the setter.
```

### TableViewTemplateColumn

Original: `controls/dev/TableView/TableView.idl` line 321

```
/// A column whose display visuals are consumer-defined via DataTemplates.
```

### TableViewTemplateColumn.CellTemplate

Original: `controls/dev/TableView/TableView.idl` line 329

```
/// Template used to generate display cell content.
```

### TableViewCellsPanel

Original: `controls/dev/TableView/TableView.idl` line 336

```
// Cell layout host used as PART_HeaderHost and each row's PART_CellsHost. Measures cells for Auto
// size-to-content and arranges them at each column's resolved ActualWidth (see TableViewCellsPanel.cpp
// and TableView_Layout.cpp).
```

### TableViewGroupInfo

Original: `controls/dev/TableView/TableView.idl` line 346

```
// Read-only group-header projection: the binding source of a GroupHeaderTemplate.
// Mirrors ICollectionViewGroup (Key <-> Group, ItemCount <-> GroupItems.Count).
// Updated in place and raises PropertyChanged, so recycling does not re-evaluate
// every binding in the header template.
```

### TableViewGroupInfo.Key (trailing comment)

Original: `controls/dev/TableView/TableView.idl` line 354

```
// the GroupBy key
```

### TableViewGroupInfo.ItemCount (trailing comment)

Original: `controls/dev/TableView/TableView.idl` line 355

```
// members in the group
```

### TableViewGroupInfo.Level (trailing comment)

Original: `controls/dev/TableView/TableView.idl` line 356

```
// 0 for single-level v1
```

### TableViewGroupInfo.IsExpandable (trailing comment)

Original: `controls/dev/TableView/TableView.idl` line 357

```
// false when the group can't expand (e.g. empty)
```

### TableViewGroupInfo.KeyText / ItemCountText

Original: `controls/dev/TableView/TableView.idl` line 360

```
// Culture-formatted display strings for the built-in content template.
// Computed lazily: a template binding only Key / ItemCount pays nothing.
```

### TableViewGroupHeaderToggleRequestedEventArgs.GroupKey

Original: `controls/dev/TableView/TableView.idl` line 371

```
// Carried on the args so a handler that re-enters and mutates the header
// still sees the key that was actually activated.
```

### TableViewGroupHeader

Original: `controls/dev/TableView/TableView.idl` line 376

```
// A templated ContentControl rather than a code-built visual tree: the chrome and
// expander live in one control-owned ControlTemplate, and GroupHeaderTemplate fills
// only the content region, so it cannot drop the expander or the themed band.
// Replacing Style/Template outright still hands the app the whole visual -- the usual
// re-template contract, which is why the parts are documented here.
//
// The whole band is the toggle target (matching ListView / TreeView) rather than a
// nested Button, so there is one interactive element and one automation story.
//
// Parts: PART_Root, PART_ContentPresenter, PART_ExpanderIcon (targeted by the default states), PART_ExpanderGutter;
// optional PART_GridLineBorder.
//
// States: CommonStates Normal|PointerOver|Pressed|Disabled
//         ExpansionStates Expanded|Collapsed
//         ExpandabilityStates Expandable|NotExpandable
```

### TableViewGroupHeader.Content / ContentTemplate (inherited)

Original: `controls/dev/TableView/TableView.idl` line 398

```
// Content is the TableViewGroupInfo projection, and it is also the header's DataContext, so
// bindings in a group header Style or its ContextFlyout see it. ContentTemplate is the header content
// template. The default Style seeds it, and TableView.GroupHeaderTemplate overrides it as a
// local value that ClearValue reverts back to the Style setter.
```

### TableViewGroupHeader.ToggleRequested

Original: `controls/dev/TableView/TableView.idl` line 409

```
// The owning row forwards this to TableView::ToggleGroupExpansion, keeping the header
// free of TableView plumbing and independently testable.
```

### TableViewColumnHeader

Original: `controls/dev/TableView/TableView.idl` line 417

```
// Templated header, one per column, created only by TableView; restyled via TableView.ColumnHeaderStyle
// or TableViewColumn.HeaderStyle.
// Optional parts: PART_ContentPresenter, PART_GridLine, PART_SortIndicatorHost, PART_ResizeGripperHost.
```

### TableViewRow

Original: `controls/dev/TableView/TableView.idl` line 429

```
//
// Template contract (MIDL3 does not surface [TemplatePart], so this block is it).
// The first four parts are REQUIRED: the Selected* states target them by name.
//
//   PART_RootBorder              — Border. Row chrome; Background driven by CommonStates.
//   PART_GridLineBorder          — Border. Renders border chrome and terminal-line suppression.
//   PART_CellsHost               — Panel. One cell wrapper per column, in Columns order.
//   PART_CellForegroundPresenter — ContentPresenter hosting PART_CellsHost. Exists because a
//                                  Border has no Foreground for the Selected* states to animate.
//   PART_SelectionIndicator      — Shape (default: Rectangle). Selected* states animate Opacity
//                                  0 -> 1; SelectedDisabled also animates Fill. Custom templates
//                                  using other indicator types must adapt the state targets.
//
// The hierarchy parts are OPTIONAL, but declared together with the three hierarchy state groups.
//
//   PART_RowExpanderGutter       — FrameworkElement. Chevron column and toggle target; Margin set
//                                  in code to the row's indent.
//   PART_RowExpanderIcon         — FontIcon. Glyph and Visibility driven by the state groups.
//
//   CommonStates:        Normal, PointerOver, Pressed, Disabled,
//                        Selected, SelectedPointerOver, SelectedPressed, SelectedDisabled
//   RowHierarchyStates:     RowHierarchical, RowNotHierarchical
//   RowExpandabilityStates: RowExpandable, RowNotExpandable
//   RowExpansionStates:     RowExpanded, RowCollapsed
//
// The hierarchy groups are entered on every row, including in flat and grouped tables. Names are
// Row-prefixed so they cannot collide with an app's own Expanded/Collapsed states in a retemplate.
//
// Selected states share this one group so nothing depends on GoToState call order.
// Multiple/Extended selection will add a separate MultiSelectStates group (and a checkbox part)
// rather than extending this one, so it stays additive for existing templates.
//
```

### TableViewRow.IsSelected

Original: `controls/dev/TableView/TableView.idl` line 465

```
// Read-only DP written only by the owning TableView (Register + a setter that is not
// projected, NOT RegisterReadOnly). Drives the Selected* states.
```

### TableViewRow hierarchy properties / TableViewRow.Level

Original: `controls/dev/TableView/TableView.idl` line 470

```
// Read-only DPs written by the owning TableView. All 0/false for flat and grouped sources.

// 1-based depth (roots are 1), matching AutomationProperties.Level. 0 = not hierarchical.
```

### TableViewRow.IsExpandable

Original: `controls/dev/TableView/TableView.idl` line 477

```
// True when this row has children. Drives RowExpandabilityStates.
```

### TableViewRow.IsExpanded

Original: `controls/dev/TableView/TableView.idl` line 481

```
// True when this row's children are currently projected as rows beneath it.
```

### TableView

Original: `controls/dev/TableView/TableView.idl` line 494

```
//
// Template contract: re-templates must provide these named parts.
// Missing required parts disables the related feature.
//
//   PART_HeaderRow      — FrameworkElement. Collapsed when headers are hidden.
//   PART_HeaderHost     — Panel. Receives one header cell per column.
//   PART_RowsRepeater   — ItemsRepeater. Hosts row containers; do not TemplateBind ItemsSource.
//   PART_EmptyStatePresenter — ContentControl/ContentPresenter (optional). Hosts EmptyTemplate.
//
// Structure, not named parts: the header and body scrollers are the nearest ScrollViewer ancestors
// of PART_HeaderHost / PART_RowsRepeater inside the template; their names are not looked up.
//
// Unsealed: ReferenceTracker requires the cppwinrt-projected derivable type surface.
//
```

### TableView.Columns

Original: `controls/dev/TableView/TableView.idl` line 515

```
// ABI exposes IVector, but the backing vector is observable for live column updates.
```

### TableView.HeadersVisibility

Original: `controls/dev/TableView/TableView.idl` line 519

```
// Controls column-header visibility without mutating Columns.
```

### TableView.GridLinesVisibility

Original: `controls/dev/TableView/TableView.idl` line 524

```
// Body row dividers and column separators. Header lines are always drawn.
```

### TableView.CanUserResizeColumns

Original: `controls/dev/TableView/TableView.idl` line 529

```
// Master switch for the column-resize affordance; a column opts out individually via CanResize.
```

### TableView.RowBackground

Original: `controls/dev/TableView/TableView.idl` line 534

```
// Opt-in row banding; null brushes preserve the theme row background.
```

### TableView.EmptyTemplate

Original: `controls/dev/TableView/TableView.idl` line 541

```
// Optional empty-state template shown when ItemsSource is null or empty.
```

### TableView.ColumnHeaderStyle

Original: `controls/dev/TableView/TableView.idl` line 548

```
// Style for every TableViewColumnHeader; TableViewColumn.HeaderStyle overrides it per column.
```

### TableView.Density

Original: `controls/dev/TableView/TableView.idl` line 552

```
// Vertical density preset for row height and built-in cell/header padding.
```

### TableView.IsReadOnly

Original: `controls/dev/TableView/TableView.idl` line 557

```
// Editing is opt-in: while true (the default) no cell can be edited, whatever the column says.
```

### TableView.IsEditing

Original: `controls/dev/TableView/TableView.idl` line 562

```
// True while an edit is in flight on a cell: from the edit being accepted, through the open
// editor, until the matching close completes.
//
// It also covers the brief windows where an edit is still opening and where a close is already
// underway, because the control uses this to reject re-entrant edit operations from inside
// consumer callbacks. An app querying it from a BeginningEdit handler therefore sees true even
// if that handler goes on to cancel.
//
// Cell scope: this is "a cell editor is open". Row-scoped editing is not part of this release.
```

### TableView (editing entry points) / TableView.CommitEdit / TableView.CancelEdit

Original: `controls/dev/TableView/TableView.idl` line 575

```
// Editing starts from the user: double-click, or F2 on the current cell. There is no
// programmatic BeginEdit in this release, and therefore no public current-cell surface for one
// to target. The context-menu feature keeps current-cell access internal; a public command
// surface still arrives later, when there is a programmatic editing scenario to justify it.

// Closes the open CELL edit. False when it did not close: a veto or failed validation leaves the
// edit open and IsEditing true.
//
// There is no row-scoped commit/cancel API and no RowEditEnding in this release; both arrive
// with row editing.
```

### TableView (no post-close edit event) / TableView selection / TableView.SelectionMode

Original: `controls/dev/TableView/TableView.idl` line 591

```
// There is no post-close event in this release. CellEditEnding reports the outcome through
// EditAction and is raised for every close, so a second "it finished" event is surface without
// a scenario of its own yet. It can be added later without breaking anyone.

// Row-scoped, single item. SelectedItem and SelectedIndex are read-only projections of the
// selection and stay coherent with each other. Drive selection through Select / Deselect /
// DeselectAll, matching ItemsView. Independent of editing.

// On by default, matching ItemsView, ListView and WPF's DataGrid. None = display-only.
```

### TableView.SelectedItem

Original: `controls/dev/TableView/TableView.idl` line 605

```
// The selected data item, or null.
```

### TableView.SelectedIndex

Original: `controls/dev/TableView/TableView.idl` line 608

```
// The selected item's index in the displayed row projection, or -1 when nothing is selected.
// Under grouping, group headers occupy projection indexes but cannot be selected.
```

### TableView.Select / Deselect / IsSelected / DeselectAll

Original: `controls/dev/TableView/TableView.idl` line 613

```
// Named to match ItemsView. No identity-based overloads: an index is the only addressing mode.
```

### TableView.ExpandAllGroups / CollapseAllGroups

Original: `controls/dev/TableView/TableView.idl` line 621

```
// Expands or collapses every group in one batch. Individual groups are expanded and collapsed
// through the group header's ExpandCollapse pattern or by clicking the header itself; these
// are the programmatic bulk counterparts. Both are no-ops when the source is not grouped.
// Under GroupBy + ParentBy these affect only group headers, not tree nodes.
```

### TableView.ExpandAllRows / CollapseAllRows

Original: `controls/dev/TableView/TableView.idl` line 629

```
// Expands or collapses every tree node of a TableViewSource.ParentBy projection; no-ops
// otherwise. Independent of the group verbs. Sets the remembered state of every node, so the
// result persists after the filter changes or clears.
```

### TableView sorting / TableView.CanUserSortColumns

Original: `controls/dev/TableView/TableView.idl` line 636

```
// Single-column sort. The control reshapes the rows itself: an ItemsSource that is not already
// a TableViewSource is projected through one internally, so sorting works over any collection
// the control can display. Sorting is raised first; cancel it to own the ordering instead.
// The active sort state lives on the column, as TableViewColumn.SortDirection; Sorted hands
// the app the column that changed, and a null column there means the sort was cleared.
//
// This and TableViewSource.Sort reconcile rather than stack: exactly one sort axis is ever in
// force, and the last writer wins. See the dev spec for the full ownership rules.

// Control-wide gate for the click-to-sort UX. When false no sort affordance is built and
// header clicks do not sort; programmatic sorting still works. The per-column cycle policy is
// TableViewColumn.SortCycle.
```

### TableView.SortByColumn

Original: `controls/dev/TableView/TableView.idl` line 653

```
// Clears any other sort state, then sets this column's SortDirection. Pass SortDirection.None
// to clear. Returns true when the sort state changed; returns false when the sort could not be
// applied (for example, the column cannot sort). Clearing with None works even when CanSort is false.
```

### TableView.ToggleSortDirection

Original: `controls/dev/TableView/TableView.idl` line 658

```
// Cycles the trigger column's direction per SortCycle, clearing any other sort state first.
// Returns true when the sort state changed; returns false when the sort could not be applied
// (for example, when the rows cannot be ordered, or the column cannot sort, as for SortByColumn).
```

### TableView.ClearSort

Original: `controls/dev/TableView/TableView.idl` line 663

```
// Clears every column's sort state in one batch. Returns true when anything was cleared.
```

### TableView.Sorting

Original: `controls/dev/TableView/TableView.idl` line 666

```
// Raised BEFORE a sort-state change is applied. Set Cancel=true to keep the control's sort
// state unchanged and let the app own the ordering.
```

### TableView.Sorted

Original: `controls/dev/TableView/TableView.idl` line 670

```
// Raised AFTER a sort-state change has been applied. Not cancellable.
```

### TableViewRowAutomationPeer

Original: `controls/dev/TableView/TableView.idl` line 703

```
// ExpandCollapse is advertised only by rows of a hierarchical source.
```

### TableViewRowTemplateSelector

Original: `controls/dev/TableView/TableView.idl` line 714

```
// Picks the row container template from TableViewRowKind. Set as PART_RowsRepeater.ItemTemplate,
// so ItemsRepeater supplies the per-template recycle pools. Projected only because
// ItemTemplateWrapper lives in another DLL; not app-facing, since selection needs an owning
// TableView that only TableView can set. unsealed + ctor is what MIDL requires for a member-less
// runtimeclass.
```

### TableViewGroupHeaderAutomationPeer

Original: `controls/dev/TableView/TableView.idl` line 727

```
// Automation peer for the group-header band. Separate from TableViewRowAutomationPeer because
// the header is now its own container rather than a row in "group mode": there is no adaptive
// branch, no GridItem coordinates for a band that spans every column, and ExpandCollapse is
// unconditional (a non-expandable group reports LeafNode rather than dropping the pattern).
```

### TableViewColumnHeaderAutomationPeer

Original: `controls/dev/TableView/TableView.idl` line 741

```
// Peer of a TableViewColumnHeader and of the TableView-owned header entries.
```

### TableViewCellAutomationPeer

Original: `controls/dev/TableView/TableView.idl` line 751

```
// Cell peer supplies per-cell grid/table structure. Its name is the cell value ("blank" when empty);
// the column header is reported through ITableItemProvider.GetColumnHeaderItems while column headers
// are shown and the column is visible. Only when column headers are hidden does the name become
// "{column header}, {cell value}". A cell holding an interactive control is named after its column
// header, since the control names itself.
// The constructor throws E_INVALIDARG for null arguments, a negative columnIndex, or a column/index outside the owning table.
```

## controls/dev/TableView/TableViewSource.idl

### TableViewIdentitySelector

Original: `controls/dev/TableView/TableViewSource.idl` line 12

```
// Stable string identity for a group key (GroupBy overload). Row identity is not app-supplied:
// the projection derives it from each item's object identity.
```

### TableViewSource

Original: `controls/dev/TableView/TableViewSource.idl` line 20

```
// Sealed deliberately: TableView drives the concrete implementation, so an override of a shaping
// verb would be bypassed. Unsealing later is compatible; sealing later is not.
```

### TableViewSource (whole-class contract) / TableViewSource.From

Original: `controls/dev/TableView/TableViewSource.idl` line 24

```
// Whole class:
//  * Null items, predicate, or keySelector throws E_INVALIDARG. (Note this differs from
//    TableView's command surface, which no-ops on bad input.) The optional
//    groupIdentitySelector may be null and selects the built-in value-type group identity.
//  * UI-thread affinity, like any XAML items source. The underlying collection must also
//    raise its change notifications on that thread; raising them from a background thread
//    is not supported. The verbs hold no lock.
//  * Rows are identified by their item's OBJECT identity (under ParentBy, by its node key,
//    which also survives re-creation). So selection and focus re-anchor across a reshape, but
//    NOT across an item being re-created, and the same object in two rows fails fast at
//    materialization.
//  * A TableViewSource can be the ItemsSource of one TableView at a time. A second table it is
//    bound to is refused and renders empty until the owner releases it (assigns a different
//    ItemsSource or is unloaded), then binds it. The refusal is not an error: nothing throws.
//    An unloaded owner that is loaded again reacquires the source automatically, or waits for
//    it if another table took it meanwhile.

// Accepts the same collection interfaces as ItemsSourceView: IVector<Object> (so also
// IObservableVector<Object>), IBindableVector, IIterable<Object>, or IBindableIterable.
// Anything else throws E_INVALIDARG. The projection is populated by the time From returns.
```

### TableViewSource.Filter

Original: `controls/dev/TableView/TableViewSource.idl` line 46

```
// Use ClearFilter() to remove an existing filter rather than passing a pass-everything one.
```

### TableViewSource.GroupBy(keySelector) [method_name("GroupBy")]

Original: `controls/dev/TableView/TableViewSource.idl` line 49

```
// keySelector: required, non-null (throws E_INVALIDARG when null; use ClearGroupBy() to
// remove grouping). Overload metadata is frozen at v1 — without it MIDL bakes GroupBy2 into
// the ABI.
//
// Grouping FAILS FAST. When a group's identity cannot be resolved, the call that builds the
// projection throws E_INVALIDARG; the projection is never silently flattened, because a
// grouping request that quietly renders ungrouped is a bug an app ships without noticing.
// An identity is unresolvable when:
//   * the key is null, or is a reference type and no groupIdentitySelector was supplied;
//   * the key is the EMPTY STRING, or a supplied groupIdentitySelector returns the empty
//     string. String is a supported key type, but "" is not a usable identity, so an app
//     grouping on a property that can be null/blank must coalesce it to a real bucket label
//     (e.g. "(none)") itself;
//   * a supplied groupIdentitySelector throws;
//   * two logically-different keys resolve to the same identity string and no
//     groupIdentitySelector was supplied to declare that collapse intentional.
```

### TableViewSource.GroupBy(keySelector, groupIdentitySelector) [method_name("GroupByWithIdentity")]

Original: `controls/dev/TableView/TableViewSource.idl` line 68

```
// groupIdentitySelector is OPTIONAL: null selects the built-in value-type group identity
// (String/Int32/Int64/Guid/Boolean/enum); a reference-type group key with no selector fails
// fast at projection time, as does an empty identity from either source (see above).
```

### TableViewSource.Sort(sortMemberPath, direction) [method_name("SortByPath")]

Original: `controls/dev/TableView/TableViewSource.idl` line 74

```
// SortDirection.None removes that key's sort axis rather than seeding a stage, so sorting
// every axis to None leaves the source unsorted. When several axes are active the FIRST one
// declared is the primary sort and each later axis breaks ties within the previous, matching
// WPF DataGrid's SortDescriptions order; re-sorting an existing axis keeps its position.
//
// Both overloads are LAST WRITER WINS against TableView's own sort: declaring a sort here
// replaces the control's axis rather than stacking behind it, so the rows are ordered by this
// axis alone. What differs is what the headers can say about it.
//
// sortMemberPath names the property this axis sorts on and is the preferred form. The path is
// evaluated by the same binding-based evaluator a column uses for its SortMemberPath, so a
// path that displays also sorts (dotted paths and indexers included), and a bound TableView
// can match the path to a column and light that column's sort indicator. Re-sorting the same
// path replaces that axis in place rather than adding a second one.
// Throws E_INVALIDARG when sortMemberPath is empty.
// Overload metadata is frozen at v1 — without it MIDL bakes Sort2 into the ABI.
```

### TableViewSource.Sort(keySelector, direction) [method_name("Sort")]

Original: `controls/dev/TableView/TableViewSource.idl` line 93

```
// keySelector form: for keys no property path expresses — computed, multi-field, normalized,
// or the identity of a primitive collection. The axis is ANONYMOUS: nothing names a property,
// so a bound TableView cannot attribute it to a column and clears every sort indicator
// instead of leaving one describing a sort that is no longer primary. Prefer the
// sortMemberPath overload whenever the key is a property.
//
// The axis is keyed by delegate instance: the same instance re-sorts it in place and None removes
// it; any other instance (e.g. a re-evaluated lambda) adds a new least-significant axis, and
// None with an instance that keys no axis leaves the sort unchanged. To remove the axis, pass
// the same delegate instance with None; ClearSort removes every axis.
```

### TableViewSource.ParentBy

Original: `controls/dev/TableView/TableViewSource.idl` line 110

```
// Declares a self-referencing parent/child relation over the flat source: keySelector returns
// each item's unique key, parentKeySelector its parent's key (null or empty for a root; an
// unknown parent key makes an orphan root). Null selectors, duplicate/empty keys, self-parent
// and cycles throw E_INVALIDARG and keep the previous projection. Value-typed keys compare by
// value and type; other objects (including app-defined enums and structs) by reference, never
// by Equals. Rows start collapsed; sort applies per sibling set, filter keeps matching rows plus
// their ancestors, and GroupBy buckets the roots.
// See docs/design-notes/TabularControls/Hierarchical-Data-Rows-Parent-Key-Design.md.
```

### TableViewSource.ClearParentBy

Original: `controls/dev/TableView/TableViewSource.idl` line 120

```
// Removes the relation and shows the source flat again. Resets expansion state.
```

### TableViewSource.IsLiveShaping

Original: `controls/dev/TableView/TableViewSource.idl` line 123

```
// Opt-in: reshapes when an item's sort key, group key, filter result or parent relation changes
// (PropertyChanged), coalesced to one reshape per dispatcher turn and deferred while a cell is
// being edited. Each reshape re-reads every item (O(n) in the source size), however few
// changed. A PropertyChanged raised off the UI thread is marshalled to it; one that cannot be
// is picked up by the next live change on the UI thread.
```

## controls/dev/dll-tabular/Microsoft.UI.Xaml.Controls.Tabular.idl

### #define MU_PRIVATE_CONTROLS_NAMESPACE

Original: `controls/dev/dll-tabular/Microsoft.UI.Xaml.Controls.Tabular.idl` line 6

```
// Deliberately the SAME value MUXC's metadata IDL uses, not a Tabular-specific child such as
// Microsoft.UI.Xaml.Controls.Tabular.Primitives.Private. A child namespace breaks the projection
// outright: cppwinrt emits an include for the parent namespace and no parent is ever generated.
// Reusing the value is safe because MIDL never references MUXC's private winmd from this build, so
// there is no duplicate-runtimeclass hazard, and framework-internal primitives stay out of every
// shipped winmd either way because metadata is partitioned by namespace.
```

### File header

Original: `controls/dev/dll-tabular/Microsoft.UI.Xaml.Controls.Tabular.idl` line 14

```
// Tabular controls DLL (Microsoft.UI.Xaml.Controls.Tabular) metadata IDL.
//
// This is a renamed copy of controls\idl\Microsoft.UI.Xaml.Controls.idl. MUXC's
// winmd already owns XamlControlsXamlMetaDataProvider and XamlControlsResources,
// so the Tabular DLL PRODUCEs differently-named types to avoid duplicate-runtimeclass
// collisions: XamlControlsTabularXamlMetaDataProvider and TabularControlsResources.
//
// The Microsoft.UI.Xaml.CustomAttributes types (MUXHasCustomActivationFactoryAttribute
// etc.) are NOT redeclared here — MIDL references MUXC's winmd (see
// controls\Tabular.ProjectImports.targets), which already defines them. Only the
// MUX_* macros are kept verbatim, since tabular control IDLs depend on them. This
// file is the FIRST entry in the Tabular project's @(Midl) so those macros are
// visible to all control IDLs merged after it.
```

## controls/dev/ResizeGripper/ResizeGripper.idl

### namespace MU_PRIVATE_CONTROLS_NAMESPACE (file header)

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 1

```
// Framework-internal: the private namespace (not the MUX_INTERNAL attribute) is what keeps this
// out of every shipped WinMD. Move to MU_XCP_NAMESPACE + MUX_PREVIEW only if it is ever exposed.
```

### ResizeGripperDragDeltaEventArgs

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 6

```
// TotalDelta spans the whole gesture from DragStarted; Delta is the change since the last raise.
```

### ResizeGripperDragCompletedEventArgs

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 16

```
// Canceled means the gesture was torn down (pointer canceled, capture lost, unloaded) rather than
// released: the host should revert to the width it captured at DragStarted, not keep the last one.
```

### ResizeGripper

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 27

```
// Drag handle. Owns the gesture, its own presentation and the keyboard step; owns no value and no
// bounds - it reports how far the user has dragged, and the host clamps that and applies it.
//
// Template contract (MIDL3 does not surface [TemplatePart], so this block is it):
//   Required parts:      none. No GetTemplateChild call; a template may omit everything.
//   State targets:       "Separator" - the visual-state setters target it by name.
//   Visual state groups: CommonStates      - Normal, PointerOver, Pressed, Disabled.
//                        OrientationStates - Horizontal, Vertical; follows DragOrientation.
//   Styling keys:        ResizeGripperSeparatorBrush, ResizeGripperSeparatorThickness,
//                        ResizeGripperSeparatorThicknessVertical (ResizeGripper_themeresources.xaml).
//   RTL:                 no mirrored state; only the drag delta is mirrored in code.
```

### ResizeGripper.ManipulationContainer

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 49

```
// The frame drags are measured in. A host sets this to an ancestor that does NOT move while the
// drag resizes something: the gripper travels with the edge it drags, so the default frame (the
// gripper itself) feeds the resize back into the gesture and stalls it, while the XamlRoot
// content picks up any scale or zoom applied between it and the gripper. Null falls back to the
// XamlRoot content, which is correct whenever nothing between it and the gripper is scaled.
```

### ResizeGripper.OwnerName

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 56

```
// Qualifier the automation peer prepends to its localized name, so N grippers in one band are
// distinguishable ("Price, Resize gripper"). Declared rather than read off Tag: Tag is a
// general-purpose slot a host may already be using, and a silent wrong name is an a11y bug.
```

### ResizeGripper.DragOrientation

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 61

```
// The axis the drag is measured along, which for a divider is perpendicular to how it looks:
// Horizontal drags left/right. Named DragOrientation rather than Orientation because the
// primitive owns no shape - the host sizes it - so the control's own layout axis is not
// something it can describe. Only horizontal mirrors under RTL.
```

### ResizeGripper.KeyboardIncrement

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 68

```
// How far one arrow key travels; Shift multiplies it. Step size is host policy, so it is
// settable rather than baked in.
```

### ResizeGripper.BeginDrag / TryDrag / EndDrag

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 73

```
// Raise the same events a pointer drag would, so a keyboard step is indistinguishable from the
// gesture. TryDrag takes the offset from where the drag began and returns whether it moved far
// enough to raise DragDelta. Pass canceled to EndDrag when the gesture was torn down.
```

### ResizeGripper.TryKeyboardStep

Original: `controls/dev/ResizeGripper/ResizeGripper.idl` line 80

```
// One arrow key as a complete one-step drag. The gripper acts on its own keys when a host makes
// it focusable; a host that keeps focus on its own element calls this instead, so neither path
// re-derives the direction, the RTL mirror or the step. False when the key is not on this axis.
```

## controls/dev/ResizeGripper/ResizeGripperAutomationPeer.idl

### namespace MU_PRIVATE_CONTROLS_NAMESPACE (file header)

Original: `controls/dev/ResizeGripper/ResizeGripperAutomationPeer.idl` line 1

```
// Peer for the framework-internal ResizeGripper primitive, so it lives in the
// same private namespace as its owner and never reaches a shipped WinMD.
```

## controls/dev/TabularShaping/SortDirection.idl

### SortDirection

Original: `controls/dev/TabularShaping/SortDirection.idl` line 4

```
// Control-agnostic sort direction for the shaping primitives. Declared in layer 1 rather than
// beside any one consumer, because TableViewSource, TableViewColumn and TableViewSortingEventArgs
// all name it.
//
// The namespace is load-bearing, not cosmetic: this type must NOT be declared into
// Microsoft.UI.Xaml.Data, a namespace the Windows App SDK also owns. A managed consumer
// referencing both winmds binds its C# projection to the SDK's copy of that namespace, which
// makes a type declared there by this binary invisible:
//
//   error CS0234: The type or namespace name 'SortDirection' does not exist in the
//                 namespace 'Microsoft.UI.Xaml.Data'
//
// Declaring shaping types only in namespaces this binary exclusively owns removes the
// split-namespace collision at the root.
```
