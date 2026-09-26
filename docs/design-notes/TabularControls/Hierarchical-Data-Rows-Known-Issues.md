# TableView Hierarchical Rows — Known Issues

Issues identified by a source audit of the hierarchical ("tree grid") rows feature and the
surrounding shaping/row infrastructure. Each entry cites the code it refers to and the change that
resolves it.

The list is split into two categories:

- **Part A — Hierarchy-specific.** Defects in the hierarchical rows feature itself. These exist only
  because the feature exists, and resolving them is part of finishing it.
- **Part B — General.** Issues in shared TableView infrastructure — shaping, grouping, selection,
  row rendering, docs. The hierarchy feature exposed or amplified several of these, but they are not
  hierarchy defects and are independently useful to fix.

Companion document: [`Hierarchical-Data-Rows-Design.md`](./Hierarchical-Data-Rows-Design.md), which
is the behavioural contract this audit was measured against.

**Legend.** Impact is `blocking` (must be resolved before the feature is complete), `important`
(resolve before the feature is considered stable), or `minor`.

---

# Part A — Hierarchy-specific findings

## A1 · Hierarchical rows expose no UIA expand/collapse pattern — FIXED

**Impact:** blocking · **Area:** accessibility · `TableViewRowAutomationPeer.cpp:28`

`TableViewRowAutomationPeer` is declared as `ISelectionItemProvider` only, and `GetPatternCore`
returns `SelectionItem`. Row hierarchy state is pushed only to visual dependency properties
(`TableView.cpp:1514`), and `Level` is a normal `TableViewRow` property
(`Generated/TableViewRow.properties.cpp:49`) rather than the UIA `Level` property.

Assistive technology therefore sees tree rows as flat data items: it cannot invoke expand or
collapse, cannot announce depth or sibling position, and receives no notification when a row's
expansion state changes.

The group-header peer already implements this pattern and is the model to follow — it advertises
`ExpandCollapse` (`TableViewGroupHeaderAutomationPeer.cpp:27`), reports
`Expanded`/`Collapsed`/`LeafNode` from live state (`:141`), and raises state-change events from
`TableView_Grouping.cpp:723`.

**Resolution.** Implement `IExpandCollapseProvider` on the row peer: report the state (including
`LeafNode` for non-expandable rows) and implement `Expand()`/`Collapse()` against the owning
`TableView` and the row's identity. Override `GetLevelCore`, `GetSizeOfSetCore` and
`GetPositionInSetCore` from row metadata. Raise `ExpandCollapseStateProperty` when a realized row's
expansion state changes, and keep the existing structure-changed notification for row
insertion/removal.

**Fixed.** `TableViewRowAutomationPeer` now also derives `IExpandCollapseProvider`. The pattern is
advertised only when `Level > 0`, so an ordinary flat or merely grouped table is not described to a
screen reader as a fully-collapsed tree; a tree *leaf* keeps the pattern and reports `LeafNode`,
which distinguishes "nothing to expand" from "not a tree". `Expand()`/`Collapse()` are directional
rather than toggles — the mutation applies on a later turn, so a toggle could not be made idempotent
as the pattern requires. `GetLevelCore` returns the already 1-based `Level`, or `-1` on a non-tree
row; `GetPositionInSetCore`/`GetSizeOfSetCore` count siblings by walking the flat row axis outward,
stopping at the parent or at a group-header boundary. State transitions are announced from
`TableViewRow::SetHierarchyStateInternal`, guarded on `ListenerExists` so no peer is forced into
existence for a table nothing is attached to.

Fixing this exposed a second defect, tracked here as A4: the toggled row keeps its index, so the
repeater never re-prepared it and it kept `IsExpanded=false` after expanding. Every realized row is
now re-derived after a reshape (`TableView::RefreshRealizedRowHierarchyState`), which the chevron
depends on as much as the peer does.

## A2 · Expand and collapse are pointer-only — FIXED (except `*`)

**Impact:** blocking · **Area:** accessibility · `TableView_Keyboard.cpp:149`

The design contract specifies `Right` to expand, `Left` to collapse and `*` to expand the current
subtree. None are implemented: the only `Left`/`Right` handling is header column resize, row
navigation covers `Up`/`Down`/`Home`/`End`/`PageUp`/`PageDown` only (`:380`), and `Space` on a
focused row selects it (`:295`) rather than toggling expansion.

A keyboard-only user cannot expand or collapse any row. Group headers already implement the
equivalent gestures, including right-to-left mirroring (`TableViewGroupHeader.cpp:203`).

**Resolution.** Handle row hierarchy keys ahead of selection and navigation: `Enter`/`Space` toggle,
`Right`/`Left` for directional expand/collapse with flow-direction mirroring, and `Multiply` for
subtree expansion. Apply only outside an active cell editor so caret keys are unaffected.

**Fixed, with two deliberate deviations from the resolution above.**

`TableViewRow::OnKeyDown` now handles `Right` to expand and `Left` to collapse, mirrored under
right-to-left, modelled on `TableViewGroupHeader::OnKeyDown`. It is placed on the row rather than in
`TableView`'s navigation handler because that handler owns the row-to-row cursor while these keys
move within a row's own state. A guard confirms focus is on the row container itself, so arrow keys
belonging to an editor or a `ComboBox` inside a template cell still reach it instead of bubbling out
and collapsing the row.

`Space` is deliberately **not** taken. It already selects the focused row
(`TableView_Keyboard.cpp:295`), and claiming it would silently change selection behaviour for the
rows of a hierarchical source only. Group headers are free to toggle on `Space` because they are not
selectable; rows are not in that position. The design document's contract is `Right`/`Left`, not
`Space`, so nothing is lost by following it.

`*` (expand the current subtree) remains **open**. It has no adapter support — the hierarchical
adapter exposes only per-node `SetNodeExpanded`, plus `ExpandAll`/`CollapseAll` — so scoped
recursive expansion of a lazily materialized subtree is new adapter work, and it is entangled with
A3 below, which has to decide what bulk expansion over a lazy tree should mean in the first place.

## A3 · Bulk group commands drive the row axis and defeat lazy loading

**Impact:** blocking · **Area:** public API, performance · `TableView.idl:584`

The design contract specifies separate `ExpandAllRows()` / `CollapseAllRows()` verbs, with the group
verbs remaining group-only, because "a unified 'expand everything' would mean two different things
under the two axes."

The implementation instead exposes only `ExpandAllGroups()` / `CollapseAllGroups()` and routes them
into both axes: `RowMetadataProvider.cpp:513-540` calls `m_hierarchicalAdapter->ExpandAll()` and
then `m_groupedAdapter->ExpandAll()`.

The consequence is not cosmetic. `ExpandAll()` moves the hierarchy baseline
(`HierarchicalSourceAdapter.cpp:475-482`), which triggers a full rebuild (`:492-510`), which invokes
the children selector recursively on every expanded node (`:407-432`). An application calling
`ExpandAllGroups()` to open its group headers therefore materializes the **entire** lazy tree — the
exact cost lazy loading exists to avoid.

The accompanying IDL comment is also inaccurate. It states that a source is "never both grouped and
hierarchical" (`TableView.idl:584-586`), while the grouped-hierarchical projection is selected at
`ShapedItemsSource.cpp:1142-1144` and published at `:1590-1593`.

**Resolution.** Add `ExpandAllRows()` / `CollapseAllRows()` routed only to the hierarchical adapter,
and restrict the group verbs to the grouped adapter. Under a grouped hierarchical source, group
verbs affect headers and row verbs affect nodes. Correct the IDL comment. Resolve this while the
surface is still `[MUX_PREVIEW]`.

## A4 · The toggled row is not restamped after an expand or collapse — FIXED

**Impact:** blocking · **Area:** correctness · `HierarchicalSourceAdapter.cpp:678`

A single-node toggle updates the descriptor and then inserts or removes the descendant rows after
the parent, but never changes `m_entries[index]` for the parent itself. The parent row therefore
receives neither a prepare nor an index-change notification, so `RefreshRowHierarchyState` is never
called on it.

Expanding a collapsed root makes its children appear while the root keeps its collapsed chevron,
visual state and automation state until it is recycled for some unrelated reason. Collapse leaves
the symmetric stale expanded chrome.

**Resolution.** Publish a coherent change for the toggled parent as part of the splice — for example
`SetAt(index, node.Item)` — before raising `ProjectionChanged`, so descriptors and rows agree at
every observable edge.

**Fixed at the control layer instead.** `TableView::ApplyGroupExpansionByIdentity` and
`SetAllGroupsExpansion` now call `RefreshRealizedRowHierarchyState()` after a successful mutation,
which re-derives `Level`/`IsExpandable`/`IsExpanded` for every row the repeater still holds. This was
preferred to a `SetAt` splice in the adapter because the same staleness applies to any reshape that
leaves a row's index untouched, not only to a single-node toggle, and because a synthetic replace
notification would make the repeater discard and re-realize a container that is otherwise perfectly
valid. Verified in the sample: an expanded root now reports `IsExpanded=true` immediately, and a
second `Expand()` in the same client turn is a no-op rather than a second toggle.

## A5 · Child collections realized for a collapsed row are not observed

**Impact:** blocking · **Area:** correctness · `HierarchicalSourceAdapter.cpp:397`

When no has-children predicate is supplied, a collapsed but visible node still realizes its child
collection in order to determine expandability. The subscription is skipped, because the code returns
early for collapsed nodes; `SubscribeToNode` runs only after expansion.

If the application then removes the last child while the parent stays collapsed, no notification
reaches the adapter and the parent remains marked as expandable. Expanding it enumerates an empty
collection and yields an expanded, empty, still-expandable row that persists until an unrelated full
rebuild.

**Resolution.** Subscribe to any child collection realized for the purpose of computing
expandability, refreshing metadata only on change; or recompute expandability on expansion when the
realized child count turns out to be zero.

## A6 · Duplicate descendant objects bypass row-identity validation

**Impact:** important · **Area:** correctness · `ShapedItemsSource.cpp:1115`

Row-identity validation runs against the root rows before the hierarchy is built. The hierarchical
rebuild then publishes the full visible set without revalidating it. The adapter rejects duplicate
**path** keys, and deliberately permits the same object under two different parents because the paths
differ.

If the children selector returns the same object for two different parents, that object appears twice
in the row axis under two identities. Item-based paths such as `SelectedItem` cannot distinguish
them, while the contract requires duplicate row objects to fail fast.

**Resolution.** Validate the visible hierarchical rows after each rebuild and reslice, for both the
hierarchical and grouped-hierarchical projections, raising the same diagnostic the flat path raises.

## A7 · Per-row indent resource lookup is uncached and performed twice — FIXED

**Impact:** important · **Area:** performance · `TableView.cpp:1271`

`GetRowIndentSize()` performs an uncached `LookupElementResource` walk — control resources, then
ancestors, then application resources, then the generic dictionary — and `HierarchyIndent()` reaches
it twice per row preparation, once from `ApplyHierarchyAffordance()` and once from
`ApplyHierarchyIndentToCells()`.

Every realized row therefore pays two boxed-key tree walks on every scroll, recycle and reindex pass.
The neighbouring metrics in the same file — density, fonts, grid-line brushes — are all cached.

The lookup is uncached by design, because a theme resource carries no change notification.

**Resolution.** Cache the resolved value in `TableViewResourceCache`, invalidated through the
existing `OnApplyTemplate`, `ActualThemeChanged` and theme-settings paths, and document that changing
the resource after rows are prepared requires them to be re-prepared. Resolve once per preparation
rather than twice. Do not read a custom dependency property on this path: doing so forces framework
metadata resolution during the first layout pass, which fails.

**Fixed.** `TableViewResourceCache` gains a `HierarchyInfo` group holding the resolved indent, kept
separate from `DensityInfo` for the same reason `FontInfo` is: the key is not density-suffixed and
grouping it under density would imply otherwise. It is cleared by the existing
`InvalidateTableViewResourceCache`, so the density, theme and high-contrast paths already invalidate
it and no new invalidation point was needed. Validation (finite, non-negative) still happens once,
before the value is cached, so an invalid resource falls back exactly as before.

The double resolution per preparation is gone: `ApplyHierarchyIndentToCells` gained an overload
taking the already-resolved indent, which `ApplyHierarchyAffordance` now uses. The no-argument form
is kept for the column-visibility path, which has no indent in hand.

Behaviour is unchanged as documented above: because a theme resource carries no change
notification, swapping it outside a density/theme/high-contrast change still requires the affected
rows to be re-prepared, which was already true of every neighbouring metric in this cache.

Verified in the sample, which overrides the resource to 24: leading text sits at x = 25 for level 1
and x = 49 for level 2, and stays correct through expand-all to five levels.

## A8 · Grouped hierarchical row metadata is resolved by linear search

**Impact:** important · **Area:** performance · `RowMetadataProvider.cpp:341`

On the grouped hierarchical path, `GetRowInfo()` resolves each row through
`TryGetNodeRowForItem`, which scans `m_descriptors` linearly
(`HierarchicalSourceAdapter.cpp:794`). Row preparation is therefore O(V) in the number of visible
rows. `EnsureIdentityIndex()` (`:209`) then loops over every row calling `GetIdentity`, which reaches
the same linear lookup at `:412`, making the identity-index rebuild O(V²).

This affects preparation, focus restoration, selection anchoring and expansion reindexing on grouped
trees.

**Resolution.** Maintain an item-identity to descriptor map in the adapter for the grouped path,
keyed by canonical `IUnknown` and updated alongside `m_indexByPathKey`; or carry descriptor metadata
into the grouped slices so the metadata provider never searches by item.

## A9 · Unchanged hierarchy properties are written on every row preparation

**Impact:** important · **Area:** performance · `TableViewRow.cpp:528`

`SetHierarchyStateInternal()` unconditionally assigns `Level`, `IsExpandable` and `IsExpanded`. The
generated setters box the value and call `SetValue`, so a row whose metadata has not changed still
pays three boxed property writes per preparation. `SetIsSelectedInternal` (`:516`) already guards
against equal values.

**Resolution.** Guard each property write on inequality. `ApplyHierarchyAffordance()` must stay
unconditional, because the indent depends on state outside the row.

## A10 · Path identity is computed twice for every emitted node

**Impact:** minor · **Area:** performance · `HierarchicalSourceAdapter.cpp:151`

`MakePathKey()` computes `ObjectIdentity(item)`, and `Emit()` immediately recomputes it at `:301`.
Each call queries for the canonical `IUnknown` and allocates a string, so every visible node pays the
cost twice during a rebuild or a large expansion, in addition to the O(depth) path concatenation.

**Resolution.** Compute the identity once in `Emit()` and pass it to a
`MakePathKey(parentPath, selfId)` overload, reusing it for cycle detection.

## A11 · Clearing the hierarchy leaves the application's delegates rooted

**Impact:** important · **Area:** lifetime · `ShapedItemsSource.cpp:1597`

`ClearChildren()` clears only the shaping-layer fields. `ReleaseHierarchyProjection()` clears the
projection callback and detaches the source, but the adapter itself survives in
`m_hierarchicalAdapter` and continues to own the children selector, the has-children predicate and
the sibling-shaping callback (`HierarchicalSourceAdapter.h:284-286`).

A selector that captures a view model therefore stays rooted until the next hierarchy declaration or
until the source is destroyed, so clearing the hierarchy does not release the application's state.

**Resolution.** Clear the adapter's callbacks while it is detached, or reset the adapter once the
callbacks held by row metadata have been safely released.

## A12 · A rebuild can publish inconsistent metadata if an allocation fails

**Impact:** important · **Area:** exception safety · `HierarchicalSourceAdapter.cpp:260`

The rebuild mutates published state in stages: descriptors are moved into place, the path index is
reserved and filled, and only then are the entries replaced. If an allocation throws between those
steps, the previously published entries remain visible while the descriptors already describe the new
rows, so `RefreshRowHierarchyState` reads the wrong depth and expandability for a displayed item.

**Resolution.** Build the new index map locally and swap the descriptors and index immediately before
replacing the entries, with a scope guard that restores the previous state if the replacement throws.

## A13 · Indent and chevron do not mirror in right-to-left layouts — NOT A DEFECT

**Impact:** important · **Area:** globalization · `TableViewRow.cpp:572`

The chevron glyph mirrors correctly — `MirroredWhenRightToLeft` at `TableView.xaml:302`, matching the
group header at `:442` — but its placement does not. The gutter is `HorizontalAlignment="Left"`
(`:295`) with a left margin, and the cell reservation writes `padding.Left` (`TableViewRow.cpp:651`).

Under `FlowDirection="RightToLeft"` the tree affordance and the content indent appear on the trailing
edge rather than the leading edge.

**Resolution — none needed; the premise is wrong.** XAML does not ask layout to swap sides under
`FlowDirection="RightToLeft"`. Layout runs unchanged in logical space and the whole subtree is
mirrored at the flow-direction boundary, so `HorizontalAlignment="Left"`, `Margin.Left` and
`Padding.Left` all resolve to the *leading* edge — visually the right — automatically. The finding
assumed the absence of mirroring that the framework in fact performs.

Measured on the sample, dumping every realized row's text, gutter and icon bounds relative to the
`TableView` while toggling `Table.FlowDirection` LTR → RTL → LTR:

```
[LTR expanded]  L1+ text=25..58 gutter=1..25 | L2- text=49..115 gutter=25..49
[RTL expanded]  L1+ text=25..58 gutter=1..25 | L2- text=49..115 gutter=25..49
[back to LTR]   L1+ text=25..58 gutter=1..25 | L2- text=49..115 gutter=25..49
```

Identical in logical coordinates across the flip, which is exactly what a correctly mirrored tree
looks like: the framework flips the whole row once, so the indent and the gutter move to the leading
edge together and stay in step. Making the code flow-aware on top of this would mirror twice and put
the chevron back on the trailing edge.

The glyph's own `MirroredWhenRightToLeft="True"` continues to flip the arrow direction, which *is*
needed — it is a directional glyph, not a layout offset.

## A14 · The selected row's chevron keeps the unselected foreground in high contrast — FIXED

**Impact:** important · **Area:** theming · `TableView.xaml:301`

`PART_RowExpanderIcon` uses `{TemplateBinding Foreground}`, while the selected visual states retarget
only `PART_CellForegroundPresenter.Foreground` (`:178`, `:191`, `:204`). In high contrast a selected
row paints the highlight background, so the chevron can render in window-text colour over that
highlight while the cell text correctly switches to the highlight-text colour.

**Resolution — fixed by inheritance, not by state setters.** The suggested fix (add
`PART_RowExpanderIcon.Foreground` setters to the four `Selected*` states) was rejected: those states
live in `CommonStates`, a **required** group, so a by-name reference to the icon from there would
silently promote an optional template part to a required one and regress the contract written for
A15 immediately below.

Instead `PART_RowExpanderGutter` now sits **inside** `PART_CellForegroundPresenter`, wrapped with
`PART_CellsHost` in a `Grid`, and `PART_RowExpanderIcon` sets no `Foreground` of its own. `Foreground`
is an inherited property, so the chevron now picks up whatever the presenter carries — exactly the
mechanism the cell text already used. This covers all four selected states, plus any state added
later, without touching a single state group and without making either part required.

The gutter is the **later** `Grid` child so it still wins hit-testing and remains the toggle target.
`m_cellsHost` is resolved by `GetTemplateChild` name lookup, so the extra nesting is invisible to code.

Verified at runtime by pushing a foreground onto one row's presenter the way the `Selected*` states
do and reading back the realized elements:

```
[LTR row 0 selected]  L1+ iconFg=#FFFFFFFF textFg=#FFFFFFFF sel=True
[presenter fg = Red]  L1+ iconFg=#FFFF0000 textFg=#FFFF0000 sel=True   <- chevron follows the text
                      (other rows unchanged at #FFFFFFFF)
hit-test over gutter: PART_RowExpanderIcon, PART_RowExpanderGutter, PART_CellsHost, ...
```

The chevron tracks the presenter, only the targeted row changes, and the gutter is still topmost
under the pointer. Light and dark themes cannot show the bug at all — selected and unselected
foregrounds are the same brush there — which is why the check drives the presenter directly rather
than relying on a theme switch.

## A15 · New row template parts are missing from the template contract — FIXED

**Impact:** important · **Area:** public API · `TableView.idl:406`

The documented `TableViewRow` template contract lists `PART_RootBorder`, `PART_CellsHost`,
`PART_CellForegroundPresenter` and `PART_SelectionIndicator`. The default template and the code also
rely on `PART_RowExpanderGutter` and `PART_RowExpanderIcon` (`TableView.xaml:292`, `:298`;
`TableViewRow.cpp:150`), which are absent from the contract, so a retemplating application cannot
tell whether they are required.

**Resolution.** Document both parts in the template contract and state their required-or-optional
semantics explicitly. If optional, confirm every visual-state and code path is safe when they are
absent.

**Fixed.** The contract block now lists `PART_RowExpanderGutter` and `PART_RowExpanderIcon` and
states the three hierarchy state groups (`HierarchyStates`, `ExpandabilityStates`,
`ExpansionStates`) that were also undocumented. It no longer says "all parts are REQUIRED", which
had become untrue.

The two hierarchy parts are **optional, but as a pair with the state groups** — a template declares
both or neither. That is not a style preference: the groups are entered on every row, including in a
flat table, and the default template's setters resolve `PART_RowExpanderIcon` by name, so declaring
the groups without the part fails when the state is applied rather than when the template is parsed.
Declaring neither is safe, because `GoToState` simply reports that the state was not found.

Verified by retemplating a live, expanded row to a template carrying only the four required parts
and no hierarchy groups: nothing faulted, the row kept rendering, its indent stayed correct, and
expand/collapse still worked through the automation pattern — which is the point of calling them
optional. Restoring the default template brought the gutter back, visible and correctly stated.

One consequence is now documented rather than left to be discovered: the leading cell's chevron
reservation is driven by `Level`, not by the presence of the gutter, so a template that drops the
gutter leaves that space blank instead of reclaiming it. Confirmed in the same run — the stripped
row's text stayed at the same x as its fully-templated siblings.

## A16 · Retemplating away the gutter leaves its handler attached — FIXED

**Impact:** minor · **Area:** lifetime · `TableViewRow.cpp:150`

`OnApplyTemplate()` replaces `m_rowExpanderGutter` but only overwrites the pointer-pressed revoker
inside the branch that runs when the new part exists. Retemplating from a template that has
`PART_RowExpanderGutter` to one that does not leaves the previous element subscribed.

**Resolution.** Reset the revoker before assigning the new part and attach only when the new part
exists.

**Fixed.** `OnApplyTemplate` now revokes unconditionally, before the new part is inspected, so the
gutter-absent case takes no branch and still ends with nothing subscribed. Retemplating in both
directions was exercised on a single live row instance: away from the default template and back,
with the gesture working again afterwards.

## A17 · `RowExpansionModel::Toggle` has no callers — FIXED

**Impact:** minor · **Area:** dead code · `RowExpansionModel.h:65`

A repository-wide search finds only the declaration. Callers open-code
`SetExpanded(key, !IsExpanded(key))`.

**Resolution — removed, because routing callers through it is not possible.** The suggested
alternative (route the existing callers through `Toggle`) was checked and rejected: there are no
callers at this layer to route. The two `SetExpanded` call sites, `GroupedSourceAdapter.cpp:224` and
`HierarchicalSourceAdapter.cpp:472`, are already handed a resolved `bool`. The toggle is resolved one
layer up, in `RowMetadataProvider::SetGroupExpandedCore` / `SetNodeExpandedCore`, where `std::nullopt`
means "flip it" — and it has to be resolved there, because those methods **return** the new state to
the caller and the adapter needs the concrete value to splice rows. A model-level `Toggle` that
swallowed the outcome could not serve either.

So the method was dead *and* unusable by construction. Deleted.

## A18 · Design document is out of date — FIXED

**Impact:** important · **Area:** documentation · `Hierarchical-Data-Rows-Design.md`

The design document is the behavioural contract, so its defects propagate into wrong fixes and wrong
tests. Four corrections are needed:

1. **Line 3** still reads "Status: design proposal, not implemented. No code for this feature exists
   yet," while the feature is implemented.
2. **Line 687** states that `Filter` combined with a lazy has-children selector throws in v1, while
   **line 506** describes it as supported with match-node-only semantics. The code does not throw —
   `ShapedItemsSource.cpp:1654-1664` applies the filter per sibling set. Section 5 is correct and
   section 9's bullet should be removed.
3. **Lines 39 and 129** name the removal verb `ClearHierarchy()`; the implemented verb is
   `ClearChildren()`, which is the correct name (see B4).
4. **Line 127** uses `TableViewPredicate` for the has-children parameter; the implemented type is
   `TableViewHasChildrenPredicate`, which is also correct (see B5).

**Resolution — all four applied, plus a fifth the review did not name.**

1. The status line now reads *implemented*, and states that a disagreement between the document and
   the code is a bug in one of them — the document is a contract, not a historical record.
2. **and a fifth:** the `Filter` contradiction was wider than reported. Section 9's bullet claimed a
   throw, but **section 5 contradicted itself too**: its "Filter semantics (v1)" paragraph declared
   *ancestor retention* as the shipping behaviour and explicitly recorded match-node-only as
   "rejected", while the very next paragraph resolved the same question the opposite way. Fixing only
   section 9 would have left the document asserting both semantics two paragraphs apart.

   The code settles it: `ShapingHelpers::ShapeSiblingsFn` (`HierarchyContract.h:59`) shapes **one
   sibling set at a time, as the walk reaches it**, so a filtered-out parent's children are never
   visited. That is match-node-only by construction, and nothing throws. The section 5 paragraph was
   rewritten to state match-node-only and defer retention; the section 9 bullet was replaced by an
   **Ancestor retention under `Filter`** bullet, so the forward reference in section 5 still lands.
3. `ClearHierarchy()` → `ClearChildren()` at both lines 39 and 129.
4. `TableViewPredicate` → `TableViewHasChildrenPredicate` at line 127.

## A19 · The feature has no automated test coverage

**Impact:** blocking · **Area:** tests

Approximately 4,200 lines of index arithmetic, event subscription and projection composition ship
with no tests. The minimum suite, cheapest and highest-value first:

| # | Suite | Layer | Key assertions |
|---|---|---|---|
| 1 | Flattening and index arithmetic | unit | Visible order, descriptor depth, `Level == Depth + 1`, path-to-index lookup and descriptor/vector alignment after each operation. Cover a single root, empty children, deep nesting, expansion of a middle subtree, collapse at a subtree boundary, and depth limits. |
| 2 | Lazy materialization | unit | With a call-counting selector: a collapsed node never enumerates children; expanding calls the selector exactly once; expansion intent recorded under a collapsed ancestor is honoured when the ancestor expands. |
| 3 | Source mutation | unit | Add, Remove, Replace, Move and Reset against both the roots and a materialized child collection, with the parent collapsed and expanded. Assert visible order, path-map coherence, subscription revocation on collapse, and that mutations under a collapsed parent surface no rows until expansion. |
| 4 | Composition | API | Sorting applies per sibling set; filtering is match-node-only; grouping buckets roots only and keeps each subtree contiguous under its root; expansion survives refilter, resort and regroup. |
| 5 | Grouped child counts | API + UI | Header count equals the immediate root count when collapsed and does not change when descendants are expanded or collapsed; root moves between buckets update only the affected headers. |
| 6 | Selection | API / UI | Selecting a descendant then collapsing its ancestor; re-expansion; removal of a selected descendant or ancestor; re-anchoring across reshapes. |
| 7 | Rendering and interaction | UI | Chevron on expandable rows, reserved gutter with hidden glyph on leaves, indent per level, the grouped base offset, right-to-left mirroring, and that clicking the chevron toggles without selecting while clicking elsewhere selects normally. |
| 8 | Accessibility | UIA / UI | `LeafNode` for leaves, expand/collapse through the pattern, 1-based `Level`, sibling-relative position and set size, and exactly one state-change event carrying the correct old and new values. |

Suites 1 to 3 are headless and should land first — they are the regression net for everything else.

---

# Part B — General findings

These concern shared infrastructure rather than the hierarchy feature. Several were surfaced by
adding hierarchy, but each stands on its own.

## B1 · Bulk expansion suppresses all failures

**Impact:** blocking · **Area:** error handling · `TableView_Grouping.cpp:486`

`SetAllGroupsExpansion` wraps its work in `catch (...)`, commented as best-effort because the
grouping source can change during cleanup. The suppression is far wider than that justification: any
diagnostic raised from the expansion path — including the depth, cycle and duplicate-identity guards
that are supposed to surface `E_INVALIDARG` — is silently discarded, so a malformed source produces
neither expansion nor an error.

**Resolution.** Let expansion diagnostics propagate. If best-effort behaviour is still required for
cleanup, narrow the catch to that specific window and report before suppressing.

## B2 · Selection intent is anchored to visible position, not identity

**Impact:** important · **Area:** correctness · `TableView_Selection.cpp:506`

Selection intent is captured as a visible index and item, and the restore path is armed only for
`Reset` — `OnSelectionItemsSourceCollectionChanged` returns early for every other action. Any
projection change that removes rows individually therefore drops the selection rather than restoring
it.

The hierarchy feature makes this visible, because collapsing a node removes its descendants with
per-row removals: selecting a child, collapsing its parent and re-expanding returns the child
unselected, contrary to the contract that hidden descendants retain selection and return selected on
expansion. The same weakness applies to any incremental reshape.

**Resolution.** Anchor selection intent to row identity rather than index. Retain intent for rows
that leave the axis while reporting only visible rows through `SelectedItems`, and reselect when a
retained identity becomes visible again. This requires a decision on how long retained intent lives
and when it is pruned.

## B3 · A single toggle under grouping rebuilds the grouped axis once per group

**Impact:** important · **Area:** performance · `ShapedItemsSource.cpp:1777`

`ResliceGroupsFromHierarchy()` recomputes every slice and then calls `SetItems` on each group.
`ShapedGroup::SetItems` performs `ReplaceAll` (`ShapedGroup.cpp:96`), and the grouped adapter
subscribes to every group's item vector with handlers that call `Rebuild()`
(`GroupedSourceAdapter.cpp:478`, `:486`, `:494`).

One change in a single bucket therefore causes a synchronous full rebuild of the grouped axis once
per group. Each rebuild republishes the presented row axis, so virtualization gains nothing.

**Resolution.** Batch the reslice: suppress or detach the grouped adapter, update only the slices
that actually changed, then trigger a single rebuild.

## B4 · Removal verbs are named after the configured operand

**Impact:** informational — no change required · **Area:** public API · `TableViewSource.idl:133`

`TableViewSource` names each removal verb after the thing being configured: `Filter`/`ClearFilter`,
`GroupBy`/`ClearGroupBy`, `Sort`/`ClearSort` (`:53-96`) and now `WithChildren`/`ClearChildren`
(`:120-133`). `TreeView` likewise names its hierarchy surface `Children` rather than `Hierarchy`
(`TreeView.idl:35-39`).

`ClearChildren()` is therefore the consistent name, and the design document's `ClearHierarchy()` is
the entry that should change (see A18).

## B5 · Delegates are named per role, not per signature

**Impact:** informational — no change required · **Area:** public API · `TableViewSource.idl:28`

`TableViewSource` already declares distinct delegate types for distinct roles even when the signatures
match — `TableViewKeySelector` and `TableViewChildrenSelector` are both `Object` to `Object`
(`:10`, `:22`).

`TableViewHasChildrenPredicate` alongside `TableViewPredicate` follows that precedent. It is redundant
by signature but not by meaning, and the design document is the entry that should change (see A18).

## B6 · Projection kind is a closed enumeration threaded through both layers

**Impact:** important · **Area:** extensibility · `ShapedItemsSource.h:52`

`ProjectionKind` is closed, and every new kind must be added to switches in the control layer
(`TableViewSource.cpp:240`, `:248`, `:256`) and the metadata layer (`RowMetadataProvider.cpp:195`,
`:198`, `:284`, `:308`, `:381`, `:397`). Adding the two hierarchical kinds demonstrated the cost: a
future virtualized or asynchronous source cannot be introduced without editing the shaping engine,
`TableViewSource` and `RowMetadataProvider` together.

**Resolution.** Publish a row-axis descriptor that supplies the source view, the row metadata
provider, the grouping status and the expansion endpoints, and let `ShapedItemsSource` provide one
instead of an enumeration value. Retain the enumeration only as diagnostic or cache state if useful.

## B7 · Row metadata is bound to one concrete adapter type

**Impact:** important · **Area:** extensibility · `RowMetadataProvider.h:14`

`RowMetadataProvider` includes `HierarchicalSourceAdapter.h`, its factories accept
`HierarchicalSourceAdapterPtr` (`:46-56`) and it stores the concrete type (`:129`). The adapter
itself is properly control-agnostic — it depends only on `HierarchyContract.h` — but the control's
metadata layer is welded to that one implementation.

**Resolution.** Extract the metadata and expansion subset into an interface — `TryGetNodeRow`,
`TryGetNodeRowForItem`, `TryGetIndexForPathKey`, `SetNodeExpanded`, `ExpandAll`, `CollapseAll` — have
the adapter implement it, and have the provider depend on the interface.

Both B6 and B7 are cheapest to resolve now, while the surrounding code is still being shaped.

## B8 · Comments carry development-process wording

**Impact:** minor · **Area:** documentation · `TableView.idl:506`

This repository mirrors publicly and comments ship verbatim, so wording that refers to the
development process rather than the code does not belong in shipped text. `TableView.idl:506` refers
to "this PR", and the design document's final section is laid out as a delivery plan with columns for
change sequencing and review gates.

**Resolution.** Rewrite the IDL comment to describe where the handler lives and why the attribute
must stay with it. Recast the design document's final section as implementation notes covering
components and test coverage.

---

## Suggested order

1. **A3** with **B1** — splitting the row and group verbs fixes an API contract violation, the lazy
   loading defeat and the suppressed diagnostics together, while the surface is still preview.
2. **A4**, **A5** — the two stale-state defects. Small, self-contained and user-visible.
3. **A1**, **A2** — accessibility. The largest body of work, with a working in-repo precedent to
   follow.
4. **A19**, suites 1 to 3 — headless adapter tests, as the regression net for everything after.
5. **A7** to **A10**, **B3** — performance.
6. **A11** to **A16** — lifetime, exception safety, right-to-left and theming.
7. **B6**, **B7** — extensibility seams, while the scaffolding is still fresh.
8. **A17**, **A18**, **B8** — dead code and documentation.
9. **A6**, **B2** — identity validation and selection retention. Both need a behavioural decision
   before implementation: the policy for shared child objects, and the lifetime of retained selection
   intent.

---

## Verified behaviour

The following were confirmed working against the sample application and are not affected by the
findings above:

- Lazy materialization — no child collection is enumerated while its parent is collapsed.
- Chevron visibility on expandable rows, with the gutter still reserved on leaf rows so sibling text
  stays aligned.
- Per-level indent, including overriding `TableViewRowIndentSize` per table.
- Grouping over a hierarchical source — roots are bucketed, subtrees stay intact, and headers report
  immediate-child counts in both collapsed and expanded states.
- Indent under grouping — roots nest inside their group header.

Accessibility (A1, A2), the parent-row restamp (A4), collapsed-node observation (A5), right-to-left
layout (A13) and high contrast (A14) were identified by source inspection and are not covered by the
verification above.
