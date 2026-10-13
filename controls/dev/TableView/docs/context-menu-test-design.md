# TableView context menus: test design

Status: design. Nothing here is implemented yet.

Scope:
- `TableView.RowContextFlyout`, `TableViewColumn.CellContextFlyout`,
  `TableViewColumn.HeaderContextFlyout`.
- The `TableView.ContextFlyout` fallback.
- `ContextFlyout`s set inside cell and header content and on rows (style or template).

Related: [context-menu-design-considerations.md](context-menu-design-considerations.md).

## How tests observe behavior

All tests use real `MenuFlyout`s; no mocks. Each candidate flyout gets a distinct name:
`CellMenu`, `RowMenu`, `HeaderMenu`, `TableMenu`, `ContentMenu`, `RowStyleMenu`,
`OuterMenu` (on an ancestor of the TableView).

A test helper attaches `Opening` and `Closed` handlers and records:

- which flyout opened (by name), and how many opened (must be exactly 1);
- `Target`: the element the flyout is anchored to;
- the `DataContext` the menu items see (the row item, or the column for header menus);
- the TableView state after the request:
  - current cell (row index, column);
  - focused element;
  - selected items.

"Cell element" means the cell wrapper in `PART_CellsHost` for that row and column.
"Row" means the realized `TableViewRow` for that item.

Request sources:

| Source | API tests (in-proc) | Interaction tests (real input) |
|---|---|---|
| UIA `ShowContextMenu` on a cell, row or header peer | yes | yes |
| Right-click | no | yes |
| Shift+F10 / Menu key | no | yes |
| Touch-and-hold, pen barrel | no | manual only |

Test homes:
- `APITests\TableView_ContextMenu_APITests.cs` (new): precedence, candidate resolution,
  side effects, recycling. Requests are raised through automation peers.
- `InteractionTests\TableView_ContextMenu_InteractionTests.cs` (new): real mouse and keyboard
  input, keyboard focus levels, chevron, editing.

Status column:
- **Green**: expected to pass on the current branch.
- **Red**: expected to fail today; it encodes a fix or a proposal. Write these first and
  watch them fail.
- **Unverified**: expected to pass, but not yet confirmed against the current branch.
- **Green (provisional)**: matches current behavior; to be re-checked against WPF.

Test data: 10 people, columns `Name` (text), `Department` (template), `Office` (text). Unless a
test says otherwise:
- `Name` has `CellMenu`; `Department` and `Office` have no cell menu.
- The table has `RowMenu`; `TableView.ContextFlyout` is `TableMenu`.
- The `Name` header has `HeaderMenu`.

---

## A. Body precedence (cell targets)

| ID | Test | Status | Intent | Expected | Failure means |
|---|---|---|---|---|---|
| A1 | `CellMenuWinsOverRowMenu` | Green | A column's cell menu is more specific than the row menu. | UIA on row 3 / `Name` opens `CellMenu`. Target is the row 3 `Name` cell. Items see person 3. | Precedence order broken, or the wrong column was resolved. |
| A2 | `RowMenuUsedWhenColumnHasNoCellMenu` | Green | Fallback from cell to row. | UIA on row 3 / `Office` opens `RowMenu`. Target is row 3. Items see person 3. | A missing cell menu is treated as "suppress", or the row candidate was skipped. |
| A3 | `TableMenuUsedWhenNoCellOrRowMenu` | Green | Final fallback to the native `TableView.ContextFlyout`. | `RowContextFlyout` = null; UIA on row 3 / `Office` opens `TableMenu`. | The fallback is not reached. |
| A4 | `UnhandledWhenNoCandidateBubblesToAncestor` | Green | With no candidate, TableView must not swallow the request. | No menus set; the TableView sits in a `Border` with `OuterMenu`. UIA on a cell opens `OuterMenu`. | Request marked handled with nothing shown, so app-level menus break. |
| A5 | `CellMenuChangesApplyOnNextRequest` | Green | The property is read per request; null removes the candidate. | On row 3 / `Name`: open/close `CellMenu`. Set `CellContextFlyout` = `CellMenu2`, request: `CellMenu2`. Set it to null, request: `RowMenu`. | A stale candidate is cached on the column or row. |
| A7 | `CellTemplateFlyoutWinsOverCellMenu` | Green | An app-set `ContextFlyout` inside the cell content wins for every input method. | `Department` template root has `ContentMenu`, and `Department` has `CellMenu`. UIA opens `ContentMenu`. | Content DFS not run, or run after the property candidates. |
| A8 | `CellTemplateFlyoutCoversCellPadding` | Green (interaction) | The content menu applies to the whole cell, padding included. | Right-click in the cell padding, outside the template root: `ContentMenu` opens. | Content flyout honored only by native hit-test routing. |
| A9 | `CellTemplateInnerFlyoutFound` | Green | A flyout on a non-root element inside the template counts. | `ContentMenu` on a nested `Border`; UIA opens `ContentMenu`. | DFS checks only the root. |
| A10 | `OutermostContentFlyoutWins` | Green | Keyboard/UIA pick the outermost content flyout (documented rule). | Root has `ContentMenu`, child has `ContentMenu2`. UIA opens `ContentMenu`. | DFS order changed. |
| A11 | `CollapsedContentFlyoutIgnored` | Green | Hidden elements are not candidates. | Template element with `ContentMenu` is `Collapsed`; UIA opens `CellMenu`. | DFS descends into collapsed subtrees. |
| A12 | `TextControlDefaultFlyoutIgnored` | Green | Built-in copy/select flyouts on text controls are not app menus. | `Name` cell template is a `TextBox` / selectable `TextBlock` with no local `ContextFlyout`. UIA opens `CellMenu`. | The DFS treats the text control's default flyout as an app menu, so `CellMenu` is unreachable. |
| A13 | `TextControlLocalFlyoutCounts` | Green | A local `ContextFlyout` on a text control is an app menu. | Same, with local `ContentMenu` on the `TextBox`. UIA opens `ContentMenu`. | The text-control exclusion is too broad. |
| A14 | `NestedTableViewFlyoutsNotPickedByOuter` | Green | The DFS stops at a nested TableView. | Cell template hosts an inner TableView whose cells have menus. UIA on the outer cell opens the outer `CellMenu`. | DFS crosses control boundaries. |
| A15 | `NestedItemsControlFlyoutNotPicked` | Red | Same boundary rule for ListView/ItemsRepeater content. | Cell template hosts a `ListView` whose items have `ContentMenu`. UIA on the cell opens `CellMenu`. | DFS descends into nested items controls and picks an item's menu. |
| A16 | `CellMenuWinsOverRowStyleMenu` | Red | A cell menu is more specific than a row menu, however the row menu is set. Today a row menu set by an implicit style or template wins; this is a bug (proposal 3). The sample README precedence text changes with the fix. | Implicit row style sets `RowStyleMenu`. UIA on row 3 / `Name` opens `CellMenu`; on `Office` it opens `RowStyleMenu`. | Cell menus not resolved in `PART_CellsHost` before the row's native flyout. |
| A17 | `CellTemplateFlyoutWinsOverRowStyleMenu` | Green | A content flyout beats a row style/template flyout. | Row style `RowStyleMenu`, `Department` template `ContentMenu`. UIA on `Department` opens `ContentMenu`. | Cells-host content pass broken. |
| A18 | `RowStyleMenuWinsOverRowContextFlyout` | Green | An app-set row `ContextFlyout` (style or template) beats `RowContextFlyout`. | Implicit row style `RowStyleMenu` and `RowContextFlyout` = `RowMenu`. UIA on row 3 / `Office` opens `RowStyleMenu`. | The row handler preempts the app's own row flyout. If `RowContextFlyout` is removed later, rewrite this test. |

## B. Row targets and keyboard focus levels

| ID | Test | Status | Intent | Expected | Failure means |
|---|---|---|---|---|---|
| B1 | `RowFocusShiftF10OpensRowMenu` | Green (interaction) | Row-level keyboard focus targets the row, not the remembered column. | Click row 3 `Name`, Left, Left (row level), Shift+F10: `RowMenu` opens. Target is row 3. | Falls back to the stale `CurrentColumn`, so the row menu can't be reached by keyboard. |
| B2 | `RowFocusMenuKeyOpensRowMenu` | Green (interaction) | Same for the Menu key. | As B1 with the Menu key: `RowMenu`. | Key-specific routing differs. |
| B3 | `CellFocusShiftF10OpensCellMenu` | Green (interaction) | Cell-level focus targets the focused cell. | Focus row 3 `Name` cell, Shift+F10: `CellMenu`. Target is the row 3 `Name` cell. | Keyboard requests ignore the focused cell. |
| B4 | `DrillBackIntoCellOpensCellMenu` | Green (interaction) | Leaving and re-entering cell level restores cell targeting. | B1 state, then Right, Right (to `Name`), Shift+F10: `CellMenu`. | Row-level state leaks into cell-level requests. |
| B5 | `CellFocusOnColumnWithoutCellMenuOpensRowMenu` | Green (interaction) | Keyboard fallback from cell to row. | Focus row 2 `Office`, Shift+F10: `RowMenu`, Target row 2. | Fallback differs between pointer and keyboard. |
| B6 | `RowPeerShowContextMenuOpensRowMenu` | Green | UIA on the row peer targets the row. | UIA on row 3's peer while `CurrentColumn` is `Name`: `RowMenu`. | Stale current column used for row peers. |
| B7 | `ChevronRightClickOpensRowMenu` | Red | The hierarchy chevron belongs to the row, not the first cell. | Hierarchical data, first column has `CellMenu`. Right-click the expander chevron of a parent row: `RowMenu` opens, current cell unchanged. | The chevron hit-tests to the first cell. |
| B8 | `RowFocusWithRowStyleMenuOpensRowStyleMenu` | Green (interaction) | Native row flyout works at row-level focus. | Implicit row style `RowStyleMenu`; row-level focus + Shift+F10: `RowStyleMenu`. | Row handler preempts the native row flyout. |

## C. Column headers

| ID | Test | Status | Intent | Expected | Failure means |
|---|---|---|---|---|---|
| C1 | `HeaderMenuOpensOnHeader` | Green | Header requests use the column's header menu. | UIA on the `Name` header peer: `HeaderMenu`. Target is the `Name` header. | Header target not resolved. |
| C2 | `HeaderWithoutMenuFallsBackToTableMenu` | Green | Header fallback skips cell and row menus. | UIA on the `Office` header: `TableMenu` (not `RowMenu`, not `CellMenu`). | Header falls into body precedence. |
| C3 | `HeaderTemplateFlyoutWinsOverHeaderMenu` | Green | Content rule applies to headers. | `Name` `HeaderTemplate` with `ContentMenu`: UIA opens `ContentMenu`. | Header content DFS missing. |
| C4 | `HeaderRequestDoesNotMoveBodyState` | Green | Header menus don't change body state. | Current cell row 2 `Office`, selection {row 2}. After a `Name` header request: current cell, selection and body focus unchanged. | Header path runs body side effects. |
| C5 | `HeaderKeyboardRequestOpensHeaderMenu` | Green (interaction) | Headers are tab stops, so the keyboard works. | Focus the `Name` header, Shift+F10: `HeaderMenu`. | Header focus is not a valid keyboard source. |
| C6 | `CollapsedColumnHeaderPeerOpensNoMenu` | Red | UIA on a header peer with no realized header must not open the table menu. | `Office` has `HeaderMenu` and is collapsed (`Visibility` = Collapsed). Get its header peer, call `ShowContextMenu`: zero `Opening` events on any menu; current cell, focus and selection unchanged. | The peer forwards the request to the TableView, so `TableMenu` opens for a column the user can't see. |

## D. Group headers

| ID | Test | Status | Intent | Expected | Failure means |
|---|---|---|---|---|---|
| D1 | `GroupHeaderUsesTableMenu` | Green | Group headers never use cell or row menus. | Grouped source; UIA/keyboard on a group header: `TableMenu`. | Group header treated as a row. |
| D2 | `GroupHeaderStyleMenuWins` | Unverified | A group-header `ContextFlyout` from a style or `GroupHeaderTemplate` wins over `TableView.ContextFlyout`. No dedicated property. | Both `GroupHeaderMenu` (via group-header style) and `TableMenu` set. UIA/keyboard on the group header: exactly `GroupHeaderMenu` opens. | A TableView handler preempts the native group-header flyout. |

## E. Side effects

| ID | Test | Status | Intent | Expected | Failure means |
|---|---|---|---|---|---|
| E1 | `BodyRequestSetsCurrentCell` | Green | A context request selects the cell the menu acts on, like a click. | Current cell row 1 `Office`; UIA on row 5 `Name` → current cell row 5 `Name`. | Commands that read the current cell act on the wrong cell. |
| E2 | `BodyRequestDoesNotChangeSelection` | Green | Opening a menu must not change selection. | Selection {row 1}; request on row 5 → selection still {row 1}. | Context request treated as a click-select. |
| E3 | `BodyRequestMovesFocusIntoTargetRow` | Green | Focus follows the menu target so focus restore lands correctly. | Focus on row 1; request on row 5 → focus within row 5. After the menu closes, focus is in row 5. | Focus restore returns to an unrelated row. |
| E4 | `BodyRequestFocusesTargetCell` | Red | Focus lands on the exact target cell (row targets: the row), so Narrator and focus restore match the current cell. | Focus row 5 `Office` cell; request on row 5 `Name` → focus on the row 5 `Name` cell. Row-peer request → focus on row 5 itself. | Focus is only moved into the row, or the row focus call fails silently. |
| E5 | `RequestInsideEditorShowsEditorMenu` | Green (interaction) | While editing, the editor owns the context menu. | Edit row 3 `Name`; the editing `TextBox` has a local `EditorMenu`. Right-click its center: exactly `EditorMenu` opens; `CellMenu` and `RowMenu` don't; still editing; value unchanged. | TableView menu hijacks the editor or ends the edit. |
| E6 | `RequestOnOtherRowWhileEditingUsesThatRowsMenu` | Red | Editing one cell must not disable menus elsewhere. | Edit row 3 `Name`; UIA on row 6 `Office`: `RowMenu` opens for person 6. | The editing check is global. |
| E8 | `RowStyleMenuAppliesBodySideEffects` | Red | Body side effects apply even when an app-set row menu wins. | Current cell, focus and selection on row 1. Implicit row style `RowStyleMenu`; UIA on row 5 / `Office`: `RowStyleMenu` opens, current cell row 5 `Office`, focus on that cell (E4), selection still {row 1}. | The native row flyout bypasses the TableView handler, so commands reading the current cell act on row 1. |
| E7 | `NoCandidateRequestStillSetsCurrentCell` | Green (provisional) | A request interacts with a cell, like a click, even when no TableView menu exists. | No menus; request on row 5 `Name` → current cell row 5 `Name`; request bubbles to the ancestor. | Side effects skipped without a candidate. Provisional: re-check against WPF `DataGrid` behavior. |

## F. DataContext, sharing and recycling

| ID | Test | Status | Intent | Expected | Failure means |
|---|---|---|---|---|---|
| F1 | `SharedCellMenuSeesRequestedRowItem` | Green | One shared flyout serves every row with the right item. | Open `CellMenu` on row 2 (items see person 2), close, then on row 7 (items see person 7). | Stale `DataContext` from the previous open. |
| F2 | `MenuCommandActsOnRequestedItem` | Green | End-to-end: the command parameter is the right item. | `RowMenu` "Remove" binds `CommandParameter="{Binding}"`. Invoke on row 4 → person 4 removed, others intact. | Commands act on the wrong item. |
| F3 | `RecycledRowMenuSeesNewItem` | Green | Recycling does not leak the old item. | 300 items. Capture the row container for item 0 and open `RowMenu` on it. Scroll until that same container instance shows a different item (precondition; fail if it was never reused). Open `RowMenu` on it again → items see the new item. | Container-level state not refreshed. |
| F4 | `RowTemplateMenuSmoke` | Green | Smoke only (framework behavior): a row-template flyout opens with its row's item. | Row template with `RowTemplateMenu`; UIA on row 3 → `RowTemplateMenu`, items see person 3. | The row handler preempts the template flyout. |
| F5 | `HeaderMenuSeesColumn` | Green | Header menu items can reach their column. | `HeaderMenu` item binds to the column; invoking "Hide" hides the `Name` column only. | Header menu `DataContext` wrong. |

## G. Robustness

| ID | Test | Status | Intent | Expected | Failure means |
|---|---|---|---|---|---|
| G1 | `ItemRemovedDuringFocusMoveSuppressesMenu` | Green | A target invalidated while focus moves is not retargeted. | Focus row 1 first (so the request must move focus). Row 5's `GettingFocus` handler removes item 5 and records that it ran. UIA on row 5 / `Name`: handler ran exactly once; zero `Opening` events; no exception. | Menu opens for the wrong item, or the request throws. |
| G2 | `ColumnRemovedDuringFocusMoveSuppressesMenu` | Green | A removed column is not a target. | Focus row 1 first. Row 5's `GettingFocus` handler removes the `Name` column and records that it ran. UIA on row 5 / `Name`: handler ran once; `CellMenu` does not open; no exception. | Dangling column reference. |
| G3 | `ReentrantRequestDuringOpeningIsSuppressed` | Green | The busy guard blocks a request raised while one is being processed. | From `CellMenu`'s `Opening` handler (row 2), synchronously call `ShowContextMenu` on the row 5 / `Name` cell peer. Exactly one `Opening` (row 2), and `CellMenu`'s items see person 2. | The guard leaks, so a nested request retargets or opens a second menu. |
| G5 | `ColumnTagOnContentDoesNotRedirect` | Red | Only the cell wrapper's column counts. | `Department` template element has `Tag="{x:Bind NameColumn}"`. A request on `Department` must not open `CellMenu` of `Name`. | Tag walk accepts any element. |

## H. Accessibility

| ID | Test | Status | Intent | Expected | Failure means |
|---|---|---|---|---|---|
| H2 | `FocusReturnsToTargetAfterClose` | Green (interaction) | Narrator users land back where the menu was. | Keyboard request on row 3 `Name`, Escape: focus on the row 3 `Name` cell. | Focus lost or restored to the table. |

Accessibility matrix (references, not separate tests): cell peer `ShowContextMenu` → A1, A2;
row peer → B6; header peer → C1, C6; keyboard → B1–B5, C5.

---

## Mutation check

Each realistic break should fail at least one test.

| Mutation | Caught by |
|---|---|
| Cell and row candidates swapped | A1 |
| Missing cell candidate not falling back to row | A2, B5 |
| Native `TableView.ContextFlyout` fallback removed | A3, C2, D1 |
| Handled set when nothing shown | A4 |
| Candidate cached across requests | A5 |
| Content DFS removed or run after properties | A7, A9, A17, C3 |
| DFS checks the root only | A9 |
| DFS doesn't skip collapsed elements / text defaults / nested controls | A11, A12, A14, A15 |
| Row-level request falls back to `CurrentColumn` | B1, B2, B6 |
| Keyboard ignores focused cell | B3, B4 |
| Chevron mapped to first cell | B7 |
| Header path runs body side effects | C4 |
| `SetCurrentCell` or focus step removed | E1, E3 |
| Selection changed by request | E2 |
| Global editing check | E6 |
| Wrong anchor (`Target` identity) | A1 (cell), A2 and B6 (row), C1 (header) |
| Stale item on a shared or recycled menu | F1, F3 |
| Invalidation retargets instead of suppressing | G1, G2 |
| Busy guard removed or leaked | G3 |
| Cell menus resolved after the row's native flyout | A16 |
| Body side effects skipped when an app row menu wins | E8 |

## Suggested order

1. Write the Red tests first and watch each fail for the stated reason: A15, A16, B7, C6, E4,
   E6, E8, G5. Then fix the product code.
2. Write the Green tests. Before relying on each one, break the product code locally
   (mutation check) and confirm the test fails.
3. Write D2 and confirm it passes with both menus set. Re-check E7 against a WPF `DataGrid`
   app.

Removed during review: the old A6 (merged into A5), G4 (framework popup lifetime and GC, not
TableView logic), and the H1/H3 duplicates (kept as matrix references).
4. Map the sample self-check cases to these IDs. Keep the sample harness for manual/visual
   checks and touch, which TAEF cannot inject.
