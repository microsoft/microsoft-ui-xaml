# TableView hierarchical rows and live shaping: test plan

This plan covers automated tests for:

- **Hierarchical rows:** `TableViewSource.ParentBy` / `ClearParentBy`, `TableView.ExpandAllRows` /
  `CollapseAllRows`, and `TableViewRow.Level` / `IsExpandable` / `IsExpanded`.
- **Live shaping:** `TableViewSource.IsLiveShaping`.

The contract is in [Hierarchical-Data-Rows-Parent-Key-Design.md](Hierarchical-Data-Rows-Parent-Key-Design.md).
Each test lists what it is for (**Intent**), what must happen (**Expected**), and what a failure tells
you (**Failure means**).

## 1. Where the tests go

| Suite | File | Notes |
| --- | --- | --- |
| API | `controls/dev/TableView/APITests/TableView_Hierarchy_APITests.cs` | Projection, keys, errors, composition, source changes, selection, robustness. |
| API | `controls/dev/TableView/APITests/TableView_LiveShaping_APITests.cs` | `IsLiveShaping` on flat, grouped and tree projections. |
| API | `controls/dev/TableView/APITests/TableView_AutomationPeer_APITests.cs` | New methods: ExpandCollapse, Level, PositionInSet/SizeOfSet. |
| Interaction | `controls/dev/TableView/InteractionTests/TableView_Hierarchy_InteractionTests.cs` | Keyboard, pointer and UIA end to end. |
| Test UI | `controls/dev/TableView/TestUI/TableViewPage.xaml(.cs)`, `TableViewTestPageFacts.cs` | New `Hierarchy` pivot (§5). |

### 1.1 Existing coverage (not re-added)

No existing TAEF test references `ParentBy`, `IsLiveShaping`, row `Level` / `IsExpanded`, or a tree
projection. These flat or grouped behaviours are already covered, so the plan does not repeat them:

| Behaviour | Existing test | Plan action |
| --- | --- | --- |
| Live-off: filter / group key edit stays stale until a collection change | `TableViewSource_APITests.VerifyFilterMembershipPropertyChangeFollowsTheSameInvariant`, `VerifyGroupKeyPropertyChangeFollowsTheSameInvariant` | Covered; no new test. |
| Live-off: sort key edit stays stale; a later change re-sorts | `VerifySortKeyPropertyChangeDoesNotMoveTheRowUntilACollectionChange` (**Ignored**, product bug) | Covered; the test is Ignored for a product bug and should be re-enabled once fixed. |
| `IsLiveShaping` defaults to off | `VerifyFreshSourceIsUnshaped` | Add a default-off assert there. |
| Selection follows the item object across sort / filter | `VerifySelectionReanchorsAcrossAReshape` | Covered; no new test. |
| Cell drill-out (Left on first cell returns to row) | `TableView_Keyboard_InteractionTests.RightDrillsIntoFirstCellAndLeftReturnsToRow` | Covered; no new test. |
| Group header ExpandCollapse / PositionInSet | `TableView_AutomationPeer_APITests.VerifyGroupHeader*`, `VerifyGroupedRowPeerPositionInSetIsRelativeToItsGroup` | Only tree-row variants added (U02–U05). |

**Open contract decisions (tests not written; each is noted at the top of its test file):**

- **Replace and selection.** Flat mode: `VerifySelectionIsNotReanchoredAcrossItemRecreation` says
  a re-created item must not inherit selection (object identity). Hierarchy mode re-anchors by
  node key. The design doc does not state this difference. Document it or align the two before
  landing L01.
- **Focus across a reshape.** Focus is position-based by spec
  (`FocusStaysAtSamePositionWhenSortReordersRows`, Ignored as a product gap), so a test that focus
  follows the item is not planned.

Register the new files in the matching `.projitems`. API tests follow the existing pattern:
`RunOnUIThread.Execute`, `TableViewTestHelpers.CreateTableView`, and `IdleSynchronizer.Wait()`
before asserting. Live reshapes are posted to the dispatcher, so every live test must wait for idle
before it reads the projection.

## 2. Fixtures and notation

**Tree fixture (T).** `Employee { Id, ManagerId, Name, Dept, Score }` with `ParentBy(Id, ManagerId)`:

```
Ada (Eng)          Eve (Ops)       Gus (Ops, leaf)
├─ Ben             └─ Fay
│  └─ Dan
└─ Cy
```

**Flat fixture (F).** `Row { Name, Score, Group }`: A(10, X), B(20, Y), C(30, X), D(40, Y). All rows
implement `INotifyPropertyChanged`.

**Projection label.** Projected rows in order, each written as `Name` + `Level` + state:

- state: `+` collapsed expandable, `-` expanded, nothing for a leaf
- a Level `0` row (not in a hierarchy) is written as the bare name, with no level or state
- group headers are `[key]`

Example: `Ada1- Ben2+ Cy2 Eve1+ Gus1`. The `TreeLabels(TableView)` helper in
`TableView_Hierarchy_APITests_Common.cs` builds this string from `GetProjectedElements` and each
realized row's `Level` / `IsExpandable` / `IsExpanded`.

**Priority.** P0 = contract or crash and must land with the feature; P1 = important behaviour; P2 =
edge case or perf guard.

## 3. API tests: hierarchy (`TableView_Hierarchy_APITests.cs`)

### 3.1 Projection basics

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| H01 P0 | `ParentByProjectsRootsCollapsed` | The first projection shows only roots, collapsed. | `Ada1+ Eve1+ Gus1`. | Roots/children are misclassified, or the default expansion state changed. |
| H02 P0 | `RootsAndOrphansAreRoots` | A parent key that matches no item, or a null parent key, makes the row a root (decision 2). | Orphans appear at Level 1 in source order among roots; no row is dropped. | Rows silently disappear, a data-loss bug. |
| H03 P1 | `EmptyStringParentIsRoot` | `""` as a parent key is "no parent", not a key lookup. | A row with `ManagerId == ""` is a Level 1 root. | Empty-string keys are treated as real keys and may link to an item keyed `""`. |
| H04 P0 | `LevelAndExpandableDPs` | Row DPs reflect node state after expand. | After `ExpandAllRows`: Ada `Level=1, IsExpandable, IsExpanded`; Dan `Level=3, !IsExpandable`. | The template binds to wrong values: wrong indent or chevron. |
| H05 P0 | `ExpandAllCollapseAll` | Bulk verbs touch every node. | Expand: `Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1`. Collapse: `Ada1+ Eve1+ Gus1`. | The bulk verbs miss nested nodes or leave stale rows. |
| H06 P1 | `NestedReexpandKeepsState` | Expansion intent is per node. | Collapse Ben, collapse Ada, expand Ada: Ben is still collapsed. | Collapsing a parent wipes its descendants' intent (regression from the design). |
| H07 P0 | `ClearParentByReturnsFlat` | `ClearParentBy` restores the flat projection. | `Ada Ben Cy Dan Eve Fay Gus` (source order, Level 0). | Hierarchy state leaks after clear. |
| H08 P1 | `ClearParentByReleasesRows` | Clear drops every reference the hierarchy held. | After clear plus GC, weak refs to the hierarchy's projected rows are dead (`alive=0`). | A leak through the adapter or the expansion-intent maps. |
| H09 P1 | `TwoRelationsOneSource` | Two `TableViewSource`s over one collection keep independent trees. | Manager tree and mentor tree each match their own relation; expanding one does not affect the other. | State is shared across sources (static or global caching). |
| H10 P1 | `RedeclareClearsIntent` | A second `ParentBy` replaces the relation and resets expansion (decision 5). | After expand plus redeclare: all roots collapsed. | Stale intent keyed to the old relation leaks into the new tree. |

### 3.2 Key semantics

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| K01 P0 | `Int32VsInt64KeysDoNotLink` | Keys compare by type and value; there is no numeric widening. | Int32 ids with Int64 parent keys: every row is a root (`... Level 1`). | Cross-type equality crept in; trees silently differ by platform or boxing. |
| K02 P2 | `EnumKeys` | WinRT enum keys link by value. | Tree forms correctly. | Enum boxing is compared by reference. |
| K04 P1 | `ObjectKeysLinkByIdentity` | Reference-type keys link by identity; a selector minting new objects per call never links and never reports false duplicates. | Shared instances: tree forms. Fresh instances: all roots, no exception. | Identity comparison is broken, or spurious duplicate-key throws. |
| K07 P1 | `KeySurvivesRecreate` | Expansion intent is keyed by node key, not object identity. | Replace Ada with a new object that has the same Id: Ada stays expanded. | Intent is lost on immutable-record updates. |

### 3.3 Error contract

All of these throw `ArgumentException` (E_INVALIDARG) from the reshape, leave the previous
projection unchanged, and recover once the data is fixed.

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| E01 P0 | `NullSelectorThrows` | Argument validation. | `ParentBy(null, x)` / `(x, null)` throws. | A null delegate crashes later in a rebuild. |
| E02 P0 | `DuplicateKeyThrows` | Duplicate node keys are a caller bug. | Throws; the message names the duplicate value; projection unchanged. | Silent wrong tree, or a corrupted projection after the throw. |
| E03 P0 | `NullKeyThrows` | A null node key is rejected. | Throws; unchanged. | Null keys collide with roots. |
| E04 P0 | `SelfParentThrows` | An item cannot be its own parent. | Throws; unchanged. | Infinite loop or stack overflow in depth computation. |
| E05 P0 | `CycleThrows` | A→B→A is rejected; the message names a member of the cycle, even when a descendant of the cycle is visited first. | Throws; message contains `A` or `B`; unchanged. | Hang or crash on cyclic data; unactionable error message. |
| E06 P1 | `RecoverAfterFix` | After a throw, correcting the data and reshaping works. | Fixed data projects correctly (`FixDuringFailingPassIsReplayed`). | The failed pass left a poisoned state. |

### 3.4 Composition with sort, filter and group

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| C01 P0 | `SortWithinSiblings` | Sort orders each sibling set; children stay under their parent. | Sort Name desc: `Gus1 Eve1+ Ada1- Cy2 Ben2+`. | Sort flattens the tree or reorders across parents. |
| C02 P1 | `SortKeepsExpansion` | Reshape by sort keeps intent. | Expanded nodes stay expanded after sort. | Sort resets the tree, a UX regression. |
| C03 P0 | `FilterKeepsAncestors` | A deep match brings its ancestor chain, auto-expanded (decisions 3–4). | Filter `Dan`: `Ada1- Ben2- Dan3`. | A match is hidden under collapsed or missing parents. |
| C04 P1 | `FilterParentMatchExcludesUnmatchedChildren` | Matches pull in ancestors, not descendants; an empty result is valid. | Filter `Ada`: `Ada1`. Filter matching nothing: zero rows, no exception. | Filter semantics drifted to "with descendants", or crash on an empty tree. |
| C06 P0 | `FilterClearRestoresIntent` | The auto-expand overlay is temporary. | Clear filter: the user's prior state is back (`Ada1+ Eve1- Fay2 Gus1`). | The overlay leaked into persistent intent (DevExtreme-style reset). |
| C07 P1 | `FilteredTogglesAreOverlayOnly` | Collapsing a context row while filtered does not change saved intent; `CollapseAllRows` does. | Collapse context Ada: `Ada1+`; clear the filter: original intent. `CollapseAllRows` while filtered: all collapsed after the filter clears. | A per-row toggle writes through to intent, or a bulk verb fails to move the baseline. |
| C08 P1 | `FilterReplaceAfterContextCollapse` | Changing the filter rebuilds the overlay. | `Ada1+`, then new filter gives `Ada1- Cy2`. | The previous overlay state sticks. |
| C10 P0 | `GroupByBucketsRootsOnly` | Grouping applies to roots; descendants follow their root. | `[Eng] Ada1- Ben2+ Cy2 [Ops] Eve1+ Gus1`. | Children land in other groups, splitting subtrees. |
| C11 P1 | `GroupedPostSortKeepsHeaderOrder` | A sort under grouping keeps header order. | Header order unchanged; siblings sorted. | Group order is mixed into the row sort. |
| C12 P1 | `RegroupExpandedTree` | Regroup and ungroup keep expansion. | Grouped then ungrouped: the same expanded rows. | Expansion is lost on a group toggle. |

### 3.5 Source collection changes

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| S01 P0 | `AddRoot` | Incremental add of a root. | New root at its sorted/source position, collapsed. | Add takes the flat fast path and loses tree structure. |
| S02 P0 | `AddChildUnderLeaf` | A leaf becomes expandable. | `Gus1` becomes `Gus1+`; expand shows `Ivy2`. | `HasChildren` is not recomputed. |
| S03 P0 | `RemoveParentOrphansChildren` | Removing a parent promotes its children to roots. | Remove Ada: `Ben1+ Cy1 Eve1+ Gus1`. | Children vanish (data loss) or keep a dangling parent. |
| S04 P0 | `ReplaceReparents` | Replace with a changed parent key moves the subtree. | `Ada1- Ben2+ Eve1- Cy2 Fay2 Gus1`. | Replace is treated as an in-place update. |
| S05 P1 | `MoveKeepsSiblingOrderBySource` | Unsorted siblings follow source order. | After move: `Gus1 Ada1+ Eve1+`. | Order is cached and stale. |
| S06 P1 | `ResetKeepsIntent` | Collection Reset keeps intent for surviving keys. | `Ada1- Ben2+ Cy2 Eve1+ Gus1`. | Reset wipes user state. |

### 3.6 Selection and focus across reshapes

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| L01 P0 | `SelectionSurvivesSameKeyReplace` | In a tree, selection is anchored by node key through Replace (see the §1.1 conflict). | Index and `SelectedItem` follow the new object; ungrouped and grouped. | Selection jumps or clears on data refresh. |
| L02 P0 | `CollapseDropsSelectionOfHiddenRow` | Collapsing the parent of the selected row clears selection; re-expanding does not restore it. | `sel=Dan`, then `null` after collapse and after re-expand. | Hidden selected rows: invisible selection, broken UIA. |

### 3.7 Re-entrancy and robustness

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| R01 P0 | `ReentrantClearParentFromSelector` | App code calls `ClearParentBy` or `ParentBy` from inside a key or sort selector mid-rebuild (`ExpandAllRows`, sort). | No crash or exception; final state matches the last call. | Use-after-free, an iterator invalidation crash, or the closure destroyed while executing. |
| R02 P0 | `ReentrantClearParentDuringRefreshAdd` / `...Filter` | Same, during an incremental add or filter refresh. | No crash; flat result including the added row. | Same as R01. |
| R03 P1 | `ReentrantAddNewGroupRootDuringExpand` | A source mutation during grouped expand. | New group `[H] Hal1` appears; no crash. | Group adapter state is torn. |
| R05 P0 | `ClearParentTeardownSurvivesFailing*` | A throwing reshape during `ClearParentBy` (flat, grouped, publication). | Exception surfaces once; the next call succeeds; data intact. | Teardown leaves a half-cleared state. |
| R06 P2 | `DeferredRequestSurvivesFailingIncrementalChange` | A queued expand request survives a failed incremental change. | Callback fires once; correct count. | A lost request or a double-run. |

### 3.8 Chevron and template (API level)

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| V02 P1 | `ChevronClippedToLeadCell` | Chevron and indent stay inside the first column. | Chevron bounds are within lead cell bounds at every level. | Deep indent overflows into the next column. |
| V03 P2 | `ChevronClipTracksColumns` | Lead column hidden, shown or resized. | Hidden: width 0, not hit-testable; shown and widened: ok. | Chevron is clickable while invisible. |

## 4. API tests: live shaping (`TableView_LiveShaping_APITests.cs`)

All use fixture F unless noted. "Mutate" means setting a property on an item (which raises
`PropertyChanged`), then `IdleSynchronizer.Wait()`.

Live-off staleness and the default flag value are covered by existing tests (§1.1).

### 4.2 Sort

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| LS11 P0 | `LiveOnRowMoves` | Sort key edit re-sorts. | `B C D A`. | Core feature broken. |
| LS13 P0 | `ReshapeIsPostedNotInline` | Reshape is coalesced to the next dispatcher turn. | Inline read `A B C D`; after idle `B C D A`. | Synchronous reshape inside `PropertyChanged`, a re-entrancy hazard. |
| LS14 P1 | `BulkMutationOneTurn` | Many edits in one turn give one consistent reshape. | `C D B A`. | Partial or intermediate orders, extra Resets. |
| LS15 P1 | `UnrelatedPropertyKeepsOrder` | Only properties the shaping reads trigger work. | Rename B: `A B2 C D`, no reorder. | Over-eager reshapes (perf) or wrong reorder. |
| LS16 P1 | `PathAxis` | Property-path sort keys are tracked. | `D A B C`. | Path columns are not tracked. |
| LS17 P0 | `SelectionFollowsItem` | Selection stays on the moved item. | `SelectedItem == A`, index 3. | Selection stays on the old index (wrong row). |
| LS18 P1 | `ThrowingSelectorNoCrash` | App selector throws during live reshape. | No crash; 4 rows. | Unhandled exception from a dispatcher callback. |
| LS19 P1 | `DisableStopsTracking` | Turning off unsubscribes. | Edits after off: `A B C D`. | Leaked subscriptions keep reshaping. |
| LS20 P1 | `TurnOnPicksUpUntrackedEdit` | Turning on runs one catch-up reshape. | `off=A B C D; on=B C D A`. | Edits made while off are never reflected. |
| LS21 P0 | `HeldWhileEditing` | Live reshape waits for an open cell edit. | During edit `A B C D`, still editing; after commit `B C D A`. | Live data ends the user's edit or moves the edited row. |
| LS22 P1 | `BulkOnFlatDoesNotEndEdit` | Bulk changes during an edit do not cancel it. | `editing=True kept=True endings=0`. | Edit is cancelled by background updates. |

### 4.3 Custom comparer (`CustomSortComparer`)

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| LS31 P0 | `ComparerLiveOffThenOn` | The comparer path (a separate rank adapter) is stale when off and re-places a changed row when on. | Off: `D C B A` unchanged. On: `D C A B`. | Compat break, or comparer sort ignores live changes. |
| LS32 P1 | `ComparerLeavesTie` | Leaving a tie re-ranks the former partner correctly. | `A B C D`, then `A C B D`. | Dense-rank bookkeeping bug. |
| LS33 P1 | `ComparerBulkReranks` | A burst over the threshold does one full re-rank (the deferred-skip path). | `D C B A`. | Deferred evictions are lost or double-placed. |
| LS34 P0 | `ComparerTurnOnPicksUpUntrackedEdit` | Turning live shaping on re-ranks a comparer sort for edits made while it was off. | Off: `D C B A`. On: `D C A B`. | Ranks from the header sort survive the catch-up reshape. |
| LS35 P1 | `ComparerReinsertedRowIsReplaced` | A row removed, edited and re-inserted is compared back in, not given its old rank. | `D C A B`. | Arrivals reuse a stale rank. |

### 4.4 Filter and group

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| LS41 P0 | `FilterLiveOnLeaveAndJoin` | Rows leave and join on predicate input change. | Leave: `C D`; join: `A C D`. | Filter not live. |
| LS42 P1 | `SortKeyEditAlsoRefilters` | One edit affecting both axes. | `C D`. | Only one axis is re-applied. |
| LS44 P0 | `GroupLiveOnRowChangesBucket` | A group key change moves the row; an empty group is removed and a new one created. | `[Y] A B D [X] C`. | Rows stuck in the wrong group; empty headers left. |
| LS45 P1 | `ObjectKeyRegroup` | Object group keys regroup by identity. | `[g] Ann Bob Cat Eli [g] Dee`. | Group identity is mis-resolved. |

### 4.5 Source tracking

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| LS50 P0 | `AddedItemIsTracked` | New items get subscribed. | `add=A B E C D; mutate=A B C D E`. | Added rows are never live. |
| LS51 P0 | `RemovedItemIsUntracked` | Removed items are unsubscribed. | Mutating the removed item does nothing: `B C D`. | Leak plus phantom reshapes. |
| LS52 P1 | `ReplacedItemOldOutNewIn` | Replace swaps the subscription. | Old item edits ignored; new item edits tracked. | Leak, or the new item is untracked. |
| LS53 P1 | `ResetRetracks` | Reset re-subscribes. | `B C D A`. | All tracking is lost after Reset. |
| LS54 P2 | `NoINPCNotTracked` | Items without INPC are ignored safely. | `A B C D`, no exception. | Crash on non-observable items. |

### 4.6 Live shaping on a tree (fixture: Ann→{Bob, Cat}, Dee→{Eli})

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| LT01 P0 | `LiveSortWithinSiblings` | Live re-sort stays inside the sibling set. | `Bob2` and `Cat2` swap under `Ann1-`. | Live sort flattens the tree. |
| LT02 P0 | `LiveOnReparentByProperty` | Parent-key edit moves the row (and subtree) and keeps other nodes' expansion. | `Ann1- Bob2 Cat2 Eli2 Dee1`; Ann still expanded. | Reparent not live, or reparent resets expansion. |
| LT03 P1 | `LiveOffReparentStaysStaleUntilResort` | Off means no reparent; a manual re-sort then picks up the parent change. Existing live-off tests cover sort/filter/group keys only. | Off: `Ann1- Bob2 Cat2 Dee1- Eli2`. After re-sort: `Dee1 Ann1- Eli2 Cat2 Bob2`. | Compat break, or stale parent structure. |
| LT04 P1 | `ReparentAfterSortEditSameTurn` | Mixed edits in one turn. | `Dee1 Ann1- Eli2 Bob2 Cat2`. | Ordering between axes is wrong. |
| LT06 P2 | `LiveTurnedOnAfterUntrackedReparent` | Catch-up on enable includes reparent. | Same as LT03 after re-sort. | Catch-up misses the parent axis. |
| LT07 P1 | `ObjectKeyReparent` | Object parent keys live-reparent. | Row moves under the new parent. | Identity keys are not re-snapshotted. |
| LT08 P0 | `LiveFilterMatchBringsAncestor` | A live filter match pulls in its ancestors. | `Ann1`, then `Ann1 Dee1- Eli2`. | Ancestors are missing for live matches. |
| LT09 P0 | `GroupedRootMovesWithSubtree` | Root group-key edit moves the whole subtree. | `[Y] Ann1- Bob2 Cat2 Dee1- Eli2`. | The subtree is split across groups. |
| LT10 P1 | `GroupedDescendantKeyIgnored` | A descendant's group key does not regroup it (roots only). | `[X] Ann1- Bob2 Cat2 [Y] Dee1- Eli2`. | Children are pulled out of their parent. |
| LT11 P1 | `CycleKeepsProjectionThenRecovers` | A live edit that creates a cycle keeps the last good projection and recovers when fixed. | Projection unchanged; then corrected. | Crash or empty table on bad live data. |

## 5. Interaction tests (`TableView_Hierarchy_InteractionTests.cs`)

**Test UI additions.** Add a `Hierarchy` pivot (`TableViewTestPageFacts.HierarchyPivotItem`) to
`TableViewPage`. It needs:

- a TableView `HierarchyTable` over fixture T (Name, Dept, Score columns)
- buttons: `GroupByDept`, `FilterDan`, `ClearShaping`
- a readout `HierarchyReadout` that shows the projection label (§2), the selected item and the
  focused row

Pointer and keyboard tests use the existing `TableViewInteractionTestHelpers`, plus a
`SelectHierarchyPivotAndGetTable()` helper.

### 5.1 UI Automation

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| U01 P0 | `FlatRowHasNoExpandCollapse` | Only hierarchical rows expose the pattern. | Basic pivot row: no ExpandCollapse pattern. | Narrator announces "collapsed" on flat tables, an a11y regression. |
| U02 P0 | `ExpandCollapseStates` | Pattern state matches the node. | Ada `Collapsed`, then `Expand()` gives `Expanded` and row count +2; Cy and Gus `LeafNode`. | Wrong state for AT users. |
| U03 P0 | `ExpandCollapseRaisesStateChanged` | AT is notified. | A `ExpandCollapseExpandCollapseStateProperty` changed event is observed on toggle. | Screen readers don't announce the change. |
| U04 P0 | `LevelPositionSizeUngrouped` | Tree position properties. | Ada `L1 1/3`, Ben `L2 1/2`, Cy `L2 2/2`, Eve `L1 2/3`, Gus `L1 3/3`. | Narrator says "level/item x of y" wrongly. |
| U05 P1 | `PositionInSetGrouped` | Sets are scoped by group. | Ada `1/1`; Eve `1/2`; Gus `2/2`. | Counts span groups. |
| U06 P1 | `TreeRowKeepsFocusAcrossCollapseViaUia` | Tree-row analogue of `GroupHeaderKeepsFocusAcrossCollapseWhenFocusedThroughUia`: a focused row toggled through UIA keeps focus. | Focus stays on Ada after `Collapse()` / `Expand()`. | Focus jumps or is lost for AT users. |

### 5.2 Keyboard (design §8; row container focused)

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| KB01 P0 | `RightExpandsCollapsed` | Treegrid Right. | Ada collapsed, Right: expanded; focus stays on Ada. | Tree navigation broken. |
| KB02 P0 | `RightOnExpandedDrillsToCell` | Right on an expanded row enters cells; Left from the first cell returns to the row without collapsing it. | Focus is the first cell of Ada; Left: focus on Ada, still expanded. | Right moves to the next row instead of entering cells, or Left on a cell collapses. |
| KB03 P0 | `LeftCollapsesExpanded` | Left on expanded. | Collapsed; focus stays. | Left falls through to the flat cell drill. |
| KB04 P0 | `LeftOnChildFocusesAndSelectsParent` | Left on a child or leaf. | Focus and selection go to the parent (`TreeMovesSelect`). | Cannot climb the tree by keyboard. |
| KB05 P1 | `LeftOnRootIsConsumed` | Focus does not leave the table. | Focus stays on the root row. | Focus escapes the table. |
| KB06 P1 | `DownReachesFirstChild` | Linear navigation includes children. | Down from expanded Ada: Ben. | Children are skipped in tab/arrow order. |
| KB07 P1 | `MultiplyExpandsSubtree` | `*` expands all descendants. | `Ada1- Ben2- Dan3 Cy2`. | Subtree expand is missing. |
| KB08 P1 | `ModifiedArrowsIgnoreTree` | Ctrl/Alt chords fall through; Shift uses the plain drill. | Tree state unchanged under modifiers. | Shortcuts collide with app or selection chords. |
| KB09 P1 | `RtlMirrorsLeftRight` | Under RTL, Left expands and Right collapses (extends `RightToLeftDrillInAndCellArrowsMirror` to tree rows). | Mirrored behaviour. | RTL users get inverted navigation. |

### 5.3 Pointer

| ID | Test | Intent | Expected | Failure means |
| --- | --- | --- | --- | --- |
| P01 P0 | `ChevronClickToggles` | Clicking the chevron toggles the node. | Ada expands, then collapses; peer state matches. | Chevron is not hit-testable or toggles twice. |
| P02 P0 | `ChevronClickDoesNotSelectOrEdit` | The chevron is not a cell. | Selection unchanged; double-press does not begin an edit (`beginning=0`). | Clicking the chevron enters edit mode or changes selection. |
Chevron clipping is checked at API level (V02), so there is no pointer duplicate.

### 5.4 Live shaping end to end

No interaction tests. Live behaviour is fully observable at API level (LS17 selection, LS21 edit
hold, LT02 reparent plus expansion). Focus across a reshape is governed by the existing dev-spec
rule (§1.1), not by this feature.

## 6. Out of scope for this plan

- Perf benchmarks (manual 100k probe on the sample's Hierarchy page).
- Theming and visual tests (high contrast and light/dark brushes for the chevron); covered by the
  existing TableView theming tests once the row template is in their baselines.
- Lazy children, a per-level sort and drag-reparent, which are not features of this version.

## 7. Suggested landing order

1. Common helpers (`TreeLabels`, fixtures), then H01–H07, E01–E05 and C01, C03, C06, C10 (P0 core).
2. Live P0s: LS11, LS13, LS17, LS21, LS31, LS41, LS44, LS50, LS51, LT01, LT02, LT08, LT09, plus
   the `IsLiveShaping` default assert in `VerifyFreshSourceIsUnshaped`. Re-enable the Ignored
      `VerifySortKeyPropertyChangeDoesNotMoveTheRowUntilACollectionChange` when the product bug is fixed.
3. Robustness P0s: R01, R02, R05; selection L02, then L01 once the §1.1 conflict is settled.
4. Test UI pivot, then U01–U04, KB01–KB04 and P01–P02.
5. P1/P2 remainder.
