# TableView Context Menu Review Tracking

Tracks the 10 review comments listed in [deliverable 64745988](https://dev.azure.com/microsoft/OS/_workitems/edit/64745988).
Item numbers match the consolidated review-comment list, not the original PR comment numbering.
Checked items record their implemented resolution or by-design decision and validation below. Build validation does not imply runtime tests were executed.

Source reviews:

- [PR #12306](https://github.com/microsoft/microsoft-ui-xaml/pull/12306), reviewer **Saharsh979**: items 1-7.
- [PR #12270](https://github.com/microsoft/microsoft-ui-xaml/pull/12270), reviewer **godlytalias**: items 8-10.

## Checklist by type

Check an item only after investigating the current implementation, addressing applicable feedback or recording a by-design decision, and recording validation in its detail section.

| Done | Item | Type | Review comment | Task |
|---|---|---|---|---|
| [x] | 1 | Documentation / contract | Update context-menu specifications and editing rules | [64745989](https://dev.azure.com/microsoft/OS/_workitems/edit/64745989) |
| [x] | 2 | Focus / interaction | Clarify or unify focus behavior for template-owned context flyouts | [64745990](https://dev.azure.com/microsoft/OS/_workitems/edit/64745990) |
| [x] | 3 | Focus / interaction | Preserve focus and scroll position when no menu resolves - closed as by design; focus follows interaction | [64745991](https://dev.azure.com/microsoft/OS/_workitems/edit/64745991) |
| [x] | 5 | Focus / interaction | Keep focus inside TableView when hiding a column through its menu | [64745993](https://dev.azure.com/microsoft/OS/_workitems/edit/64745993) |
| [x] | 4 | Routing / keyboard / accessibility | Resolve row-template menus for keyboard and UIA requests | [64745992](https://dev.azure.com/microsoft/OS/_workitems/edit/64745992) |
| [x] | 7 | Routing / keyboard / accessibility | Anchor table-fallback menus at the focused cell for keyboard and UIA | [64745995](https://dev.azure.com/microsoft/OS/_workitems/edit/64745995) |
| [x] | 8 | Editing / flyout lifecycle | Suppress group-header and row native menus when edit commit is vetoed | [64746047](https://dev.azure.com/microsoft/OS/_workitems/edit/64746047) |
| [x] | 9 | Editing / flyout lifecycle | Retain editor-flyout subscription after cancelled Closing | [64746048](https://dev.azure.com/microsoft/OS/_workitems/edit/64746048) |
| [x] | 6 | Virtualization / stale identity | Reject requests from recycled group-header automation peers | [64745994](https://dev.azure.com/microsoft/OS/_workitems/edit/64745994) |
| [x] | 10 | Virtualization / stale identity | Prevent open group menus from retargeting after header recycling | [64746049](https://dev.azure.com/microsoft/OS/_workitems/edit/64746049) |

## Build validation

Final `amd64chk` builds completed successfully. `snip.exe` was used only to filter build output:

| Command | Result |
|---|---|
| `.\initrun.ps1 C:\configs\tools\snip.exe -- .\build.cmd /q` | Passed: full product and test-binary build; no test execution |
| `.\initrun.ps1 C:\configs\tools\snip.exe -- msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:WinUIVersion=3.0.0-dev` | Passed: sample built against the freshly packed local WinUI package |
| `powershell.exe -NoProfile -File .\Samples\TableViewSampleApp\tools\Update-Snippets.ps1 -Tags ContextMenus -Check` | Passed: generated context-menu snippet is current |
| `git diff --check` | Passed |

The first full build found missing `Microsoft.Internal.WinUITestResources` fonts; running
`.\init.ps1` restored them. Compilation also caught an incorrect flyout event-sender type
and unsupported test-fixture property references; both were corrected before the successful
full rebuild. Build-generated unrelated metadata ordering changes were discarded.

**Runtime tests were not executed, as explicitly requested.** The added regression cases
compile, but their runtime results remain unverified. Checklist completion records
implemented, build-checked resolutions, not runtime sign-off.

## Documentation / contract

### 1. Update context-menu specifications and editing rules

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12306#discussion_r4306199677) on `controls/dev/TableView/TableView.idl`.

**Feedback:** API spec omits `CellContextFlyout` and `HeaderContextFlyout`; functional spec still lists per-row/cell context menus as future work. The existing rule that moving focus off a cell commits editing does not describe focus moving into an editor-owned flyout.

**Resolution to evaluate:** Update API and functional specifications with menu resolution order, `DataContext` rules, keyboard/UIA targets, and editor-owned flyout behavior. Keep the documented contract aligned with the outcomes of the behavioral items below.

**Investigation:** Compared `TableView.idl`, `TableView_ContextMenu.cpp`, `TableViewRow.cpp`, and `TableView_EditingInput.cpp` with existing context-menu API and interaction test expectations.

**Resolution:** Updated the [API specification](../../api-specs/TableView/TableView-spec.md#context-menus) with both flyout properties and their dependency-property identifiers, menu precedence, binding context, input targeting, focus behavior, and current routing/lifecycle exceptions. Updated the [functional specification](TableView-functional-spec.md#context-menus) to reflect implemented menu support. Corrected editing focus-loss rules in both specifications and the API editing appendix. Other review items are tracked separately; no runtime behavior changed.

**Validation:** Documentation compared against the implementation and existing test source; checked diff whitespace, API declarations, relative links, and checklist completeness. No build or runtime tests run for this documentation-only change.

## Focus / interaction

### 2. Clarify or unify focus behavior for template-owned context flyouts

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12306#discussion_r4306199680) on `controls/dev/TableView/TableView.idl`.

**Feedback:** The statement "The cell takes focus" does not hold for a `ContextFlyout` inside a cell template. The framework opens that menu and marks the request handled before the cells-host handler can call `FocusContextMenuTarget`. At review time, `RightClickOnTemplateCellOpensTemplateMenu` explicitly skipped its focus assertion for this reason.

**Resolution to evaluate:** Explicitly document the template-owned exception, or focus the cell on right-button press so menu tiers behave consistently. Decide the intended contract before changing behavior.

**Decision:** Unify mouse/pen right-button focus before native menu opening. Do not move focus after a handled context request: the popup can already have saved the previous focus target and run its Opening callbacks.

**Implementation:** `TableViewRow::OnPointerPressed` resolves a live owned data-cell target for an unhandled right-button press. A focus-only TableView entry point reuses existing editing suppression, target validation, reentrancy protection, and context-menu focus logic, including source/target revalidation after focus callbacks. The press stays unhandled and does not arm selection. Child-handled presses, nested tables, and active editors retain their existing ownership; focus already inside the target cell is preserved. Touch/hold, keyboard, UIA, and header behavior are unchanged. IDL and both specifications describe the scoped behavior.

**Regression coverage:** Updated `RightClickOnTemplateCellOpensTemplateMenu` to start with a different selected/focused row and assert target-cell focus at Opening, unchanged native menu target and selection, target-cell focus after Escape, and subsequent Down navigation from the requested row in the same column. Current-cell state is internal, not a public property; the test checks observable focus/navigation rather than adding API for test access. Its template root deliberately tags another column: pointer resolution must identify the real cells-host child, never trust an application's `Tag`. Both source walking and hit-test fallback now use that validated wrapper.

**Validation:** Full product/test-binary build passed; see [build validation](#build-validation). Runtime tests were not run, at the user's request.

### 3. Preserve focus and scroll position when no menu resolves

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12306#discussion_r4306199683) on `controls/dev/TableView/TableView_ContextMenu.cpp`.

**Feedback:** `ProcessContextMenuRequest` calls `FocusContextMenuTarget` even when no cell, row, or content flyout resolves. This can change the current cell, drill into cell navigation, and bring the target into view before returning without showing a menu. Previously, a non-left pointer press with no menu did nothing.

**Original proposal:** Leave focus, current-cell state, navigation level, and scroll position unchanged when no applicable menu exists. Preserve any focus move required by an actual native `TableView.ContextFlyout` fallback.

**Decision:** Closed as by design. A context gesture is still interaction with the targeted cell, even when no menu is configured. For a valid request reaching TableView's routing, move focus and the current cell to that target independently of whether a flyout resolves; do not gate the focus move on menu availability. Selection remains unchanged. Existing editing suppression, stale-target checks, and native template-owned handling remain unchanged.

**Visibility clarification:** A pointer request targets an on-screen element; this path does not realize an arbitrary off-screen row to open a menu. However, an on-screen cell can be only partially visible. `FocusVisibleCellInternal` calls `StartBringIntoView`, so normal focus-driven scrolling is still possible. The decision retains that existing focus behavior rather than promising no scrolling.

**Resolution:** Kept runtime implementation unchanged. Updated both specifications to describe no-menu focus movement as intentional, not an outstanding defect.

**Validation:** Inspected `ProcessContextMenuRequest`, `FocusContextMenuTarget`, `FocusRealizedRowCell`, and `FocusVisibleCellInternal`; confirmed focus processing precedes the no-candidate return and uses the existing bring-into-view helper. Documentation-only decision; no build or runtime tests run.

### 5. Keep focus inside TableView when hiding a column through its menu

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12306#discussion_r4306199689) on `Samples/TableViewSampleApp/Pages/ContextMenusPage.xaml.cs`.

**Feedback:** `HideColumn` runs during the menu item's `Click`, before the flyout closes. Focus is still inside the flyout, so the focus guard in `OnColumnVisibilityChanged` does not find focus inside the collapsing header. On closure, the hidden header cannot regain focus, and the framework can move focus to the first focusable element outside the table.

**Resolution to evaluate:** Collapse the column after `Closed`, or move focus to the next visible header before hiding it. Preserve the sample's "Never strand focus" rule.

**Investigation:** `MenuFlyoutItem::Invoke` raises `Click` before closing the menu. `OnColumnVisibilityChanged` only applies header focus recovery when focus is inside the collapsing header, not inside its flyout.

**Implementation:** Each header menu now records a pending hide on `Click` and performs it from `Closed`, clearing the flag before calling `HideColumn`. Escape/light-dismiss without selecting Hide does not collapse a column. This retains the header until normal menu focus restoration, allowing the existing TableView focus recovery to run when it collapses.

**Validation:** Regenerated and checked `ContextMenus.cs.txt` with the repository's Windows PowerShell snippet generator. Separate sample build passed; see [build validation](#build-validation). Runtime tests were not run, at the user's request.

## Routing / keyboard / accessibility

### 4. Resolve row-template menus for keyboard and UIA requests

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12306#discussion_r4306199685) on `controls/dev/TableView/TableView_ContextMenu.cpp`.

**Feedback:** For a row target, `FindRowContextFlyout` checks only the row itself, despite its comment claiming to cover row-template elements. Shift+F10 and the Menu key originate on the focused `TableViewRow`, so its template-root and cells-host children never see those requests. The row automation peer follows the same resolution path. A row-template menu that works for pointer requests can therefore fall back to `TableView.ContextFlyout` for keyboard/UIA.

**Resolution to evaluate:** Include the row template root when resolving a row target, maintaining the intended menu precedence across pointer, keyboard, and UIA requests.

**Investigation:** A row-only request starts on the row, above its template children, while cell requests already walk the ancestor chain above their wrapper.

**Implementation:** Row-only lookup starts at the validated cells host and walks through its template ancestors to the row. This finds cells-host/template-root menus before a row-style menu without descending into unrelated cells or choosing a column menu.

**Regression coverage:** `RowPeerResolvesTemplateMenusBeforeRowMenu` checks the template-root menu over a row menu, then a nearer cells-host menu over both.

**Validation:** Full product/test-binary build passed; see [build validation](#build-validation). Runtime tests were not run, at the user's request.

### 7. Anchor table-fallback menus at the focused cell for keyboard and UIA

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12306#discussion_r4306199697) on `controls/dev/TableView/TableView_ContextMenu.cpp`.

**Feedback:** When only `TableView.ContextFlyout` applies, resolution returns no candidate and lets the framework show the table's flyout with `ShowAt(TableView)`. Keyboard/UIA requests have no pointer position, so the menu does not use the focused-cell placement promised by the description.

**Resolution to evaluate:** Route non-pointer table-fallback requests through `ShowContextFlyout` with the focused cell as the exclusion rectangle, or explicitly document the table-tier exception. Keep tests and IDL consistent with the chosen contract.

**Investigation:** The existing non-pointer placement helper already translates cell bounds into a larger menu owner's coordinates; only the table fallback bypassed it.

**Implementation:** Non-pointer requests resolve `TableView.ContextFlyout` as the final local candidate, both before focus callbacks for stale-request handling and again after focus settles. `ShowContextFlyout` uses the requested body/header cell bounds while preserving the table as `Target` and DataContext source. Pointer requests retain native table fallback; ancestor bubbling still applies when the table has no menu.

**Regression coverage:** Existing body-cell and column-header table-fallback API cases now assert popup geometry relative to the requested cell, not only which menu opened.

**Validation:** Full product/test-binary build passed; see [build validation](#build-validation). Runtime tests were not run, at the user's request.

## Editing / flyout lifecycle

### 8. Suppress group-header and row native menus when edit commit is vetoed

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12270#discussion_r4287682162) on `controls/dev/TableView/TableViewGroupHeader.cpp`.

**Feedback:** If an edit commit is vetoed while focus moves to a group header, Shift+F10 can open its native `ContextFlyout` without passing through the child hook, even though editing remains active.

**Resolution to evaluate:** Apply suppression to owner-originated requests, including the row hook, before the framework opens a native flyout.

**Investigation:** A native request raised directly on a row/header bypasses descendant routing hooks, and native owner handling runs before the owner's normal event handlers. Handling KeyDown does not prevent the framework's later context-request generation.

**Implementation:** Rows and group headers observe their effective `ContextFlyout` property and keep a weak-owner `Opening` subscription on the current flyout. The guard checks the actual target, then cancels opening with `Hide` while editing is suppressed. A temporary final `Closing` handler prevents an application cancellation from overriding this policy before a popup is visible. Application bindings/style values are never cleared or replaced; shared flyouts on unrelated targets and editor-owned flyouts remain available. Native `Opening`/`Closing` callbacks can still run even though no popup opens.

**Regression coverage:** Direct owner-originated requests after a vetoed commit cover rows and group headers, including an application `Closing` veto. A shared row menu is also opened on an unrelated button while editing remains active.

**Validation:** Full product/test-binary build passed; see [build validation](#build-validation). Runtime tests were not run, at the user's request.

### 9. Retain editor-flyout subscription after cancelled Closing

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12270#discussion_r4287682481) on `controls/dev/TableView/TableView_EditingInput.cpp`.

**Feedback:** The handler is removed after `Closing` even when closure is cancelled and the flyout stays open. With animations disabled, Escape in a `CommandBarFlyout` can then reach `CancelEdit` and discard the draft.

**Resolution to evaluate:** Retain the subscription until the flyout actually closes, including the cancelled-Closing path.

**Investigation:** Revoking the `Closing` handler on its first invocation loses the Escape-dismiss marker if that invocation is cancelled and the menu remains open.

**Implementation:** Retain `Closing` for every close attempt. Revoke both flyout subscriptions when `Closed` actually fires, then queue the existing focus-loss decision.

**Regression coverage:** An animation-free editor-owned `CommandBarFlyout` cancels its first `Closing`. The interaction test verifies the next Escape closes only the menu and a separate subsequent Escape still cancels the edit.

**Validation:** Full product/test-binary build passed; see [build validation](#build-validation). Runtime tests were not run, at the user's request.

## Virtualization / stale identity

### 6. Reject requests from recycled group-header automation peers

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12306#discussion_r4306199691) on `controls/dev/TableView/TableViewGroupHeaderAutomationPeer.cpp`.

**Feedback:** `ShowContextMenuCore` checks `AreContextMenusSuppressed()` but not whether the group header is still realized. Recycled headers intentionally retain their owner and can remain parented, allowing a retained automation peer to open a style menu for a group the header no longer represents. Existing pooled-peer coverage handles rows and cells, not group headers.

**Resolution to evaluate:** Reject unavailable headers when `GetElementIndex(header) < 0`, consistently with the peer's existing `ElementNotAvailable` behavior.

**Investigation:** The peer already implements the required repeater-index availability check in `Row()`, but `ShowContextMenuCore` did not call it.

**Implementation:** Validate `Row()` before editing suppression or native menu routing. A pooled header now raises `ElementNotAvailable` instead of opening either a retained style menu or table fallback.

**Regression coverage:** `PooledGroupHeaderPeerShowContextMenuOpensNoMenu` retains the peer and attached menu, removes all groups, and expects an exception without any menu opening.

**Validation:** Full product/test-binary build passed; see [build validation](#build-validation). Runtime tests were not run, at the user's request.

### 10. Prevent open group menus from retargeting after header recycling

**Source:** [Original comment](https://github.com/microsoft/microsoft-ui-xaml/pull/12270#discussion_r4287682815) on `controls/dev/TableView/TableView_Grouping.cpp`.

**Feedback:** An open group menu retains a `TableViewGroupInfo` instance. Recycling its header for another group mutates the same object, so menu commands can target the new group rather than the group for which the menu opened.

**Resolution to evaluate:** Preserve the requested group's identity for the lifetime of the open menu, or close the menu before recycling its header.

**Tracking note:** The GitHub thread is marked resolved, but the deliverable keeps this item because remaining defects were reported. Source investigation below confirmed the projection-retargeting path. Unlike item 6, this item concerns an already-open menu rather than a stale automation peer opening a new menu.

**Investigation:** Preparation always mutated the previous `TableViewGroupInfo`, even when its group key changed. The flyout copies its target's DataContext object onto its presenter when opening; mutating that retained object retargeted commands.

**Implementation:** Refresh the existing projection only for the same group key. A different group gets a fresh projection, while open menus retain the original object and group identity. Same-group metadata/expansion bindings remain live. No forced menu closure or application `Closing` override is needed for recycling.

**Regression coverage:** `OpenGroupMenuKeepsOriginalGroupAfterHeaderRecycling` opens a group menu, scrolls until its header represents another group, and verifies the retained projection still has the original key; if the menu remains open, its item still sees that original projection.

**Validation:** Full product/test-binary build passed; see [build validation](#build-validation). Runtime tests were not run, at the user's request.

## Scope exclusions

The deliverable explicitly excludes original PR #12306 comment 8 concerning API ownership overlap with [PR #12286](https://github.com/microsoft/microsoft-ui-xaml/pull/12286).
This checklist covers the deliverable's 10 tracked comments, not every comment on the source PRs.
