# TableView interaction test plan

Companion to **[`TableView-api-test-plan.md`](TableView-api-test-plan.md)**. That document owns every behaviour reachable
programmatically; this one owns the gesture routes into them, plus what only real input can reach.

The authoring protocol for both is **[`controls\dev\TableView\AGENTS.md`](../../../controls/dev/TableView/AGENTS.md)**. Its
six steps apply here unchanged — in particular step 3 (derive expectations from the API and design spec, never from the
implementation) and step 5 (never adjust a test to match the implementation).

## Why a separate document

Interaction tests are a different cost class, not just a different folder. They need a TestUI page, a running app, and real
input injection; they are slower, flakier, and harder to diagnose than an API test asserting the same thing. Keeping them in
their own backlog makes the split visible and stops interaction tests being written for behaviour an API test already covers.

**Current size of the split: no API items remain; 44 interaction items do.** Every API category is now written, so this
backlog is what is left of the plan. It grew from 35 when the coverage rule below changed: behaviour reachable both ways is
now tested both ways, which added nine gesture-route tests to categories that already had API coverage.

## Coverage rule

**When a behaviour is reachable both programmatically and by gesture, it is tested both ways.** The two tests make
different claims and fail for different reasons:

- The **API test** owns the *state machine* — every transition, permutation and negative case. It is fast and
  deterministic, so it is where breadth lives.
- The **interaction test** owns the *route* — that real input actually reaches that state machine. Hit-testing, capture,
  key routing, focus, and the handlers that translate input into API calls are all invisible to an API test. A control can
  pass every API test and be entirely unusable by a person.

This replaces the earlier rule, which said to write only the API test when a behaviour was reachable both ways. That rule
optimized for run time and lost real coverage: `IInvokeProvider` on a header proves the sort state machine works, and says
nothing about whether clicking the header hits it.

### What this does *not* mean

It does not mean every API test gets a twin. The interaction test exists per **gesture route**, not per assertion:

- One test per route is enough. Pointer-click-sorts is one interaction test; the ascending/descending/none cycle,
  `CanSort` gating and event ordering stay API-only, because once the click provably reaches `SortByColumn` those
  permutations cannot fail differently under a pointer.
- A behaviour with **no** gesture route stays API-only: default values, DP round-trips, resource and theme lookup,
  virtualization bounds, `ExpandAllGroups`, shaping algebra.
- A behaviour with **no** programmatic route stays interaction-only: pointer capture, hover states, system focus visuals,
  `ToggleRequested`, markup-compiled XAML.

### Still true: do not assert through a page readout

An interaction test must assert the product's own state — a property, a peer, or the visual tree — after driving real
input. Reading a `TextBlock` the test page updated in its own code-behind asserts what the page chose to display, and
fails for reasons unrelated to the behaviour. The test page provides *input surface*, not the assertion.

### Where the routes are

Before adding an item, know which of these it exercises; each is a distinct piece of plumbing, and they are the reason
this document exists:

1. **Pointer hit-testing to a cell.** `dev-spec:286` — the walk from `OriginalSource` up to the cell wrapper, with a
   hit-test fallback when the row itself is the source.
2. **Pointer capture.** Held between `BeginDrag` and `EndDrag` during a resize; nothing in the peer surface opens or
   observes it.
3. **Key routing.** `dev-spec:165` — `TableView` listens to bubbling `KeyDown` so template-column descendants handle
   input first; only *unhandled* keys navigate.
4. **Focus.** `dev-spec:296-301` — focus moves are deliberately split across `EndCellEdit` / `AbandonCellEdit` and the
   focus-loss commit is posted to the dispatcher, precisely because mutating focus inside a framework callback trips the
   re-entrancy assertion (`0xc0000420`).
5. **Hover.** Visual states and tooltip popups driven from pointer enter/exit.
6. **Markup compilation.** `x:Bind` and generated `XamlTypeInfo`, which only a real page produces.

## Infrastructure required
None of this exists yet; the first interaction test pays for all of it.

- `controls\dev\TableView\TestUI\TableViewPage.xaml` and `.xaml.cs` — the test page, with readouts for selection, sort state
  and column widths so assertions can read state without a debugger.
- `controls\dev\TableView\InteractionTests\TableViewTests.cs`
- `controls\dev\TableView\InteractionTests\TableView_InteractionTests.projitems`
- Wiring into `controls\test\MUXControls.Test\MUXControls.Test.csproj`.

### Page readouts, and why they are readouts

Two page elements exist purely so out-of-process tests can observe state that product finding #13 puts out of reach. Both
are read-only `TextBox`es, because `Edit.Value` re-reads through the Value pattern while a `TextBlock`'s UIA name is
served from a cache that can go stale mid-test.

- **`EditColumnReportTextBlock`** — the page's `BeginningEdit` handler appends `"<ColumnHeader>;"` on every edit.
  `TableViewBeginningEditEventArgs.Column` is public IDL, so this is instrumentation over the product contract, not a
  private hook. It **appends** rather than overwrites so a test can distinguish an F2 that did nothing from one that
  re-reported the same column.
- **`RowStateLogTextBlock`** + **`HookRowStatesButton`** — the button hooks the first realized row's `CommonStates`
  group (in-process visual-tree walk, which creates no automation peers and so does not trip #13) and the handler
  appends every transition. A transition *log* rather than a live read because any interaction that read the state
  would itself move the pointer off the row.
- **`GroupHeaderStateLogTextBlock`** + **`GroupToggleReportTextBlock`** + **`HookGroupHeadersButton`** — the same
  pattern for §4. One button hooks `ToggleRequested` on **every** realized `TableViewGroupHeader` and the
  `CommonStates` group of the **first** one. The state log appends transitions exactly like the row log. The toggle
  report writes one `"<key>|<key>;"` entry per raise: the key is read on entry, the handler then mutates the header
  (`sender.IsExpanded = !sender.IsExpanded`), and the key is read again — the re-entrancy case `TableView.idl:350-355`
  says the args exist for. Because there is one entry per raise, "exactly once per activation" is assertable.

- **`EditorProbeTextBlock`** — one entry per opened edit, `"<EditorType>|<EditorAutomationName>|<EditorText>|focus=<bool>;"`,
  written by `ProbeEditor`: an **in-process** visual-tree walk posted from `BeginningEdit` at Low dispatcher priority, so
  the editor is generated, laid out and focused before it is read. This is what unblocked §5. It creates no automation
  peers, so finding #13 is never touched — the same inversion the row and group-header state hooks use. A cell's display
  visual is a `TextBlock`, so the only `TextBox` under the table is the open editor. The focus field walks up from the
  focused element, so an editor whose inner part takes focus still counts.
- **`EditEndReportTextBlock`** — appends `"<EditAction>;"` (`Commit` or `Cancel`) from the public `CellEditEnding` args.
- **`FirstItemNameTextBlock`** — the first bound item's `Name`, updated from the item's own `PropertyChanged`. This is the
  **data** side of an edit, not a product state readout: it says whether the edit reached the item, which is the half a
  `CellEditEnding` report cannot prove.

`Key.F2` was added to `controls\test\testinfra\MUXTestInfra\Common\KeyboardHelper.cs`; the shared enum had `F4`, `F6` and
`F10` but not `F2`, which is TableView's documented begin-edit key (`functional-spec:61`). §5 needs it too.

A TestUI page is also the only route to markup-compiled XAML (`x:Bind`, XamlTypeInfo), so any future test needing those lands
here regardless of input.

---

## Run status and open failures (interaction tier)

### The Pivot was poisoning the whole suite (resolved)

The test page used to host its four tables in a `Pivot`. Navigating to the page a **second** time asserted
`0xC0000420` inside **`Microsoft.UI.Xaml.Phone.dll`** — the Pivot, not TableView. Every crash killed the host
(`0xE0434352` in `AssemblyCleanup`) and took the *following* test with it, producing a pass → crash → cascade rhythm
that accounted for 8 of 11 keyboard failures and manufactured several "defects" that did not exist.

Replacing the Pivot with four visibility-toggled host grids switched by `GoTo{Basic,Rtl,Grouped,Scrolling}Button` took
the keyboard suite from **4/15 to 11/15 with zero crashes**. Two earlier diagnoses are retracted by this:

- **`EndKeyMovesToLastRow` is not a defect.** It passes. It had only ever been observed as cascade damage.
- **The `FindElement.ByName` theory was wrong.** Every crashing test happened to call `ByName`, but only because every
  `ByName` call site was a page-switch helper, i.e. a *second* page interaction. Moving those helpers to `ById` was kept
  (cheaper, and it avoids product finding 13 by construction) but it fixed nothing on its own.

This is a reportable Pivot bug — repro: host content in a `Pivot`, navigate to the page twice — but it is out of
TableView's scope.

### Pointer tier: 2/10 (run `ptrrun1`) — one new product finding, one crash of ours, two open

**Product finding #14 — `GetClickablePoint` on a `TableViewRow` peer access-violates the app.** Four of the eight
failures were `0xC0000005` in `Microsoft.UI.Xaml.dll`, and they line up exactly with the UIA call, not with the gesture:
`InputHelper.LeftClick(row)` and `InputHelper.MoveMouse(row, x, y)` both ask UIA for the element's clickable point and
both crashed, while the *same click on the same row* expressed as a bounding-rectangle offset did not. UIA computes a
clickable point by walking into the element's children, which is the same peer-manufacturing path as finding #13. This
also **re-instates the `0xC0000005` row-click verdict that was retracted above** — it survives the Pivot removal, it is
real, and it is a peer bug rather than an input bug.

*Test-side rule, same shape as #13's:* never click or hover a row through a UIA-resolved point. `ClickRow` /
`HoverRow` in `TableView_Pointer_InteractionTests.cs` go through `BoundingRectangle` instead. Note the offsets
`InputHelper` takes are **centre-relative**, confirmed against the passing `DragColumnBoundary` helper in the layout file.

**Resolved as a test bug — F2 after a pointer press.** `PointerPressEstablishesCurrentCellForKeyboardEditing` read an
empty `BeginningEdit` report after pressing inside `Age` and then F2. **Verified by hand in the running app: a click
followed by F2 does open an editor**, so the product is fine. `ptrrun2` then identified the actual cause: the click was
issued as a centre-relative offset and never landed on the table at all (see the `ptrrun2` section). The first attempt
at a fix — clearing the element cache before reading the report — was wrong *and* harmful, and is what introduced the
`0xC0000420` crash. Both are now replaced by absolute-point input.

**Open — keyboard group activation looks like it loses focus on collapse.** **Reproduced by hand in the running app:
keyboard activation of a group header does not toggle it, while a pointer click does** — so this is a product defect,
not a harness artefact. The row counts say where it goes wrong: repeated Enter
went `9 → 6 → 3`, i.e. the second Enter collapsed a *different* group rather than re-expanding the first, and `Right`
after `Left` left the count at 6. Meanwhile the pointer-click toggle passes both ways. The hypothesis is that the header
loses keyboard focus when its own group collapses, so the next key lands elsewhere. Both keyboard tests now assert
`HasKeyboardFocus` immediately after the collapse, which turns an ambiguous count into a direct answer next run.

**Fixed test bug — stale element cache.** `CollapseAllGroupsReconcilesGestureExpandedGroup` read `9` rows after the
page's CollapseAll button, failing its own precondition. The collapse is driven from *outside* the table, so nothing
inside the tree invalidated MITA's cache. `CountRows` now calls `ElementCache.Clear()` first. API
§9 `VerifyCollapseAllGroupsHidesDataRowsKeepsHeaders` covers the same projection in-process and passes, which is what
makes a cache artefact more likely than a product break here.

> **Retracted — this was product finding #16, not a cache artefact.** The cache fix changed nothing, and the group
> header peer was later shown to still report `Expanded` after the call. The "API twin passes" argument was the weak
> step: that test asserts the projection, not the realized control. See the `ptrrun3` section.

Passing: `GroupHeaderPointerClickTogglesRowVisibility`, `ExpandAllGroupsReconcilesGestureCollapsedGroup`.

### Test-page hazard: a `Checked` handler that runs during `InitializeComponent`

Run `kbrun5` failed **all 18** tests identically, every one of them inside `TestSetupHelper` with
`COMException (0x802B000A)` — `E_XAMLPARSEFAILED`. The page never loaded, so no assertion in the suite ever ran.

Cause was the newly added `CanUserSortColumnsCheckBox`. It carries `IsChecked="True"` in markup, and setting that
DP during parse raises `Checked` immediately — before `InitializeComponent` has assigned the `BasicTableView`
field further down the document. The handler dereferenced `BasicTableView`, threw, and the throw surfaced as a
parse failure for the whole page. `IsReadOnlyCheckBox` never hit this only because it is authored `IsChecked="False"`,
which raises nothing.

Rule for this page: **any `Checked`/`Unchecked`/`SelectionChanged` handler wired to an element whose initial value is
authored in markup must null-guard the fields it touches.** A total, identical, load-time failure across a whole
suite is this class of bug until proven otherwise — read the first `Error:` line, not the count.

### Pointer tier: 0/12 (run `ptrrun2`) — one harness root cause, one confirmed product defect

Two independent faults, both diagnosed from this run.

**Root cause of the click failures: the offset overloads are the wrong tool.** `PointerClickSelectsAndFocusesRow`,
`PointerClickOnSecondRowMovesSelection` and `PointerClickInSelectionModeNoneSelectsNothingButMovesFocus` all failed on
"nothing was selected / focus did not move", and the F2 test read an empty `BeginningEdit` report. The log shows the
click executing at `(-293, 0)` — a *centre-relative* offset computed from the row's width. That assumption is wrong:
`InputHelper`'s offset overloads anchor on `obj.GetClickablePoint()` (see `LeftMouseButtonDown`, `MoveMouse`), while
`UIObject.Click(button, dx, dy)` anchors differently again — `RatingControlTests:647-649` passes
`(1.5 * itemWidth, height / 2)`, which only makes sense from the upper-left corner. Either way the click landed off the
table, so every assertion downstream of it was measuring nothing.

Fix: **all pointer input in the TableView tests is now absolute screen points** built from `BoundingRectangle` and
issued through `PointerInput.Move` / `Press` / `Release`. This also sidesteps finding #14 by construction, since no
UIA point is ever resolved. The WebView2 tests adopted the same workaround for the same `GetClickablePoint` fault
(their Task 30555367 comments).

This supersedes the `ptrrun1` reading of the F2 test. The empty report was never staleness — the click simply never
reached a cell, so no current cell existed for F2 to act on. The user's manual check (click, then F2, editor opens) was
the product telling us the harness was wrong.

**`ElementCache.Clear()` is not safe on this page.** `ptrrun2` added an app crash the previous run did not have:
`0xC0000420` (assertion) in `Microsoft.UI.Xaml.dll`, fired immediately after page load and before the F2 test's first
`Verify`. The only thing that runs there is `ReadEditReport()`, whose first statement was `ElementCache.Clear()`.
Clearing forces the next `FindElement` to re-walk the entire visual tree, and that walk descends into `TableViewRow`
children — the peer-manufacturing path of finding #13. Reproduced in isolation with a single-test `te.exe` run, and
a bare page load on a fresh app does *not* crash, which rules the page itself out. All `ElementCache.Clear()` calls
outside `CountRows` have been removed; UIA re-queries children on access, so re-resolving the rows host is enough to
see a reordered or collapsed table.

> **Amended after `ptrrun3`: the blame above is too narrow.** Removing our `ElementCache.Clear()` calls did not remove
> the crash, and `ptrrun3` supplies the full stack: `0xC0000420` fires inside `ElementCache.Refresh()` →
> `DepthFirstDescendantsNavigator`, called from `FindElement.ById` in **`TestSetupHelper.Dispose()`** looking up the
> back button. *The harness clears and re-walks the tree on its own, at teardown, in every test.* Our calls were one
> trigger among several, not the cause. The rule stands — do not add `ElementCache.Clear()` on a page hosting a
> TableView — but the underlying fault is the tree walk itself, and `AGENTS.md` already lists `0xC0000420` at teardown
> as a known intermittent assert. Re-run before reading anything into it.

**Confirmed product defect #15 — a group header loses keyboard focus when its own group collapses.**
`GroupHeaderArrowKeysExpandAndCollapse` collapsed the group with `Left` (rows `9 → 6`, header reports `Collapsed`,
both correct) and then failed the new assertion that the header still holds keyboard focus. That is the direct
confirmation of the hypothesis raised after `kbrun4`: the next key lands somewhere else, which is why a second `Enter`
collapsed a *different* group and why `Right` could not re-open what `Left` had just closed. The pointer route toggles
both ways and passes, so the fault is in what the key path does to focus, not in the toggle itself. A user who closes
a group from the keyboard cannot reopen it.

**Not yet re-measured.** Everything else in the file failed downstream of the crash (`RowPointerOverEntersHoverState`
and `GroupHeaderKeyboardActivationTogglesRowVisibility` were killed by the crashed host, `0xE0434352` RPC failures) or
passed every assertion and then failed on teardown against a dead app (`GroupHeaderPointerClickTogglesRowVisibility`
verified all six of its assertions first). The two new double-click tests never ran a single assertion.

### Pointer tier: 5/12 (run `ptrrun3`) — the absolute-point rewrite worked; two real failures left

**The input fix is confirmed.** Every test that failed in `ptrrun2` purely because the click missed the table now
passes: `PointerClickSelectsAndFocusesRow`, `PointerClickOnSecondRowMovesSelection`,
`PointerClickInSelectionModeNoneSelectsNothingButMovesFocus`, `PointerPressEstablishesCurrentCellForKeyboardEditing`
(the F2 test, closing that thread for good) and the new `PointerDoubleClickDoesNothingWhenReadOnly`. Absolute screen
points from `BoundingRectangle` are the supported way to drive this control.

**Correction to how these logs must be read: `Verify.*` throws here.** `AGENTS.md` states that a failed `Verify` logs
and continues. It does not — `ptrrun3` shows execution jumping straight from a failed `Verify.IsLessThan` into
`TestSetupHelper.Dispose()`, and the log carries an unhandled `WEX.TestExecution.VerifyFailureException`. **Treat the
first failure in a test as terminal**; every later assertion in that test is unmeasured, not passing. Tests that need
two observations to be diagnosable must therefore *take and log both values before asserting either* —
`CollapseAllGroupsReconcilesGestureExpandedGroup` now does exactly that.

**Open product finding #16 (candidate) — `CollapseAllGroups()` does not collapse in the live control.**
`CollapseAllGroupsReconcilesGestureExpandedGroup` fails its precondition with `IsLessThan(9, 9)`: after the page button
calls `GroupedTableView.CollapseAllGroups()`, all nine data rows are still realized. Everything that would make this a
harness artefact has been ruled out:

- **Not staleness.** `CountRows` clears the cache, and the sibling `ExpandAllGroupsReconcilesGestureCollapsedGroup`
  drives the *identical* route — page button outside the table, then `CountRows` — and does observe the count change.
- **Not a page bug.** `OnCollapseAllGroupsClick` is a bare `GroupedTableView.CollapseAllGroups()` call.
- **Not a fixture difference.** The page builds its source as `TableViewSource.From(items).GroupBy(...)` assigned to
  `ItemsSource` (`TableViewPage.xaml.cs:124,130`), which is the same shape as the API suite's `CreateGroupedTable()`.
- **Not the toggle mechanism.** The gesture route collapses the same group on the same table correctly (`9 → 6`).

The API twin `VerifyCollapseAllGroupsHidesDataRowsKeepsHeaders` passes, but it asserts the **projection**
(`ProjectedRowLabels`) rather than realized containers — so a break that reaches the user and not the projection is
exactly what it cannot see. Per Step 5 the test stays failing. The reordered assertions above will say next run whether
the group *state* flipped while the rows stayed realized (a de-realization bug) or nothing moved at all (a no-op bug);
that answer decides how #16 is filed.

> **Answered by the per-test isolation runs below: it is a no-op.** The reordered assertion reports
> `AreEqual(Collapsed, Expanded)` — after `CollapseAllGroups()` the first group header still says **Expanded**, and all
> nine rows are still realized. Nothing moved, so this is not a de-realization bug. **Finding #16 is confirmed**, with
> this shape:
>
> - `ExpandAllGroups()` on the same page, same table, same button route **passes every assertion** (`Collapsed →
>   Expanded`, `6 → 9`). The two bulk APIs are asymmetric.
> - Held UIA peers are *not* stale across a bulk operation — the `ExpandAllGroups` run proves a held header peer
>   updates correctly across the full Rebuild/Reset that a bulk call performs. That rules out the last harness theory.
> - The difference in state at the call site is that `CollapseAllGroups` is invoked on a **pristine** grouped table
>   (no group has ever been toggled), whereas the passing `ExpandAllGroups` is invoked when one group has been
>   gesture-collapsed, i.e. when `RowExpansionModel` holds an exception. That is the most likely axis for a repro.
>
> Source reading did **not** find the fault, and two tempting explanations were checked and eliminated:
> `RowExpansionModel::m_defaultExpanded` is initialised `true` (`RowExpansionModel.h:83`), so
> `SetDefaultExpanded(false)` really does move the default and raise an `AffectsAllKeys` change; and
> `GroupedSourceAdapter::OnExpansionChanged` routes any `AffectsAllKeys` change to a full `Rebuild()`
> (`GroupedSourceAdapter.cpp:252`). The page wiring is also clean — `OnCollapseAllGroupsClick` is a bare
> `GroupedTableView.CollapseAllGroups()` and the two buttons are separately wired (`TableViewPage.xaml:51-58`).
> The break is somewhere between that `Rebuild()` and the realized tree, and needs a product owner. **Repro: load a
> grouped TableView, touch nothing, call `CollapseAllGroups()` — headers stay Expanded and every data row stays
> visible.** Note the API twin cannot see this: it calls the same method in the same order and passes, because it
> asserts `ProjectedRowLabels` rather than the realized control.

**~~Finding #15 confirmed a second time.~~ Still ACTIVE, scope narrowed.** `GroupHeaderArrowKeysExpandAndCollapse`
failed its keyboard-focus assertion twice here; both runs took focus with UIA `SetFocus`. On the **Tab** route the test
passes outright (`iso_kbgrp4`), so the keyboard-user path is fine. The **`SetFocus` (assistive-technology) path still
loses focus across a collapse** — #15 stays open and still owes a test in the accessibility section.

**Teardown crash is the harness, not us, and it is expensive.** `GroupHeaderPointerClickTogglesRowVisibility` verified
**all six** of its assertions and then crashed in `TestSetupHelper.Dispose()` (stack in the amended `ptrrun2` note
above). The dead host then took the next tests with it via `0xE0434352`, which is why four tests have *no result* at
all: `GroupHeaderKeyboardActivationTogglesRowVisibility`, `ExpandAllGroupsReconcilesGestureCollapsedGroup`,
`RowPointerOverEntersHoverState` and `PointerDoubleClickBeginsEditWhenEditable`. **Run the grouped and editing tests
individually** (`te.exe /select:`) until this is mitigated — a cascade costs 3-4 results per run and invites false
diagnoses.

`RowPointerOverEntersHoverState` additionally hit `Win32Exception (5)` during `TestSetupHelper`'s own navigation click.
That is a locked/RDP-detached session refusing input injection. It is an environment condition, never a result.

Still never executed a single assertion: `HeaderClicksFollowTheColumnSortCycle` (lives in the keyboard file, which has
not been run since the `E_XAMLPARSEFAILED` fix).

### Per-test isolation runs — the four unmeasured tests now have verdicts

Each run in its own `te.exe` process, so no cascade could reach it. **Pointer tier is now 7 genuinely passing.**

| Test | Verdict |
| --- | --- |
| `PointerDoubleClickBeginsEditWhenEditable` | **Passes.** `AreEqual(Age;, Age;)` — a double-click begins an edit on the column under the pointer. Closes the last of the four requested tests. |
| `RowPointerOverEntersHoverState` | **Passes.** State log `hooked;PointerOver;Normal;`. The `ptrrun3` failure was the locked-session `Win32Exception (5)`, exactly as called. |
| `ExpandAllGroupsReconcilesGestureCollapsedGroup` | **Product-correct.** All seven assertions pass; reported `[Failed]` *only* because the app crashed afterwards in teardown. |
| `GroupHeaderKeyboardActivationTogglesRowVisibility` | ~~Real failure — finding #15.~~ **Passing on the Tab route** (all ten assertions, `iso_kbgrp3`). Finding #15 is **not** retracted: the `SetFocus` (assistive-technology) route still loses focus across a collapse and still owes its own test. |

**The teardown crash is not intermittent on the grouped tests — it is reliable.** All three grouped tests crashed
`0xC0000420` in isolation, on a fresh app, with nothing else running. Every crash happens *after* the test's own
assertions, in `TestSetupHelper.Dispose()`. So `AGENTS.md`'s "known intermittent teardown assert" advice does not apply
here: re-running does not clear it. Consequences to respect when reading any grouped-test result:

- A grouped test's `[Failed]` verdict means nothing on its own — **read the `Verify:` lines and find the first
  `Error:`**. If every `Verify` passed, the test passed and only teardown failed.
- Never run grouped tests in the same process as tests you care about; the dead host fails the next ones with
  `0xE0434352`.

This is worth its own product investigation: a UIA tree walk at teardown reliably asserting on a page hosting a
grouped TableView is the same family as findings #13 and #14 (peer manufacturing for `TableViewRow`/group children).

### §4 group-header input: 5/5 after the last two items landed

Isolation runs `s4a` and `s4b`, after the page grew `HookGroupHeadersButton`, `GroupHeaderStateLogTextBlock` and
`GroupToggleReportTextBlock`.

| Test | Route | Verdict |
| --- | --- | --- |
| `GroupHeaderPointerClickTogglesRowVisibility` | pointer | Passes (`ptrrun3`) |
| `GroupHeaderKeyboardActivationTogglesRowVisibility` | keyboard (Tab) | Passes (`iso_kbgrp3`) |
| `GroupHeaderArrowKeysExpandAndCollapse` | keyboard (Tab) | Passes (`iso_kbgrp4`) |
| `GroupHeaderPointerAndPressedVisualStates` | pointer | **Passes** (`s4a`) — log reads `hooked;PointerOver;Pressed;PointerOver;Normal;` |
| `GroupHeaderPointerToggleRaisesToggleRequestedWithGroupKey` | pointer | **Passes** (`s4b`) |
| `GroupHeaderKeyboardToggleRaisesToggleRequestedWithGroupKey` | keyboard (Tab) | **Passes** (`s4b`) |

Three things the new passes settle, beyond their own claims:

1. **`ToggleRequested` fires exactly once per activation on both routes**, and carries the activated group's own key —
   proven against two *different* headers on the pointer leg, so a constant key cannot explain the result.
2. **The key survives a re-entrant handler.** The page handler mutates `sender.IsExpanded` between the two reads and
   both reads match, which is the exact scenario `TableView.idl:350-355` says the args exist for. It also means a
   handler mutating the header mid-event does **not** trip the re-entrancy assert (`0xC0000420`) — worth knowing given
   findings #13/#14 sit in that family.
3. **The header's `CommonStates` machine is complete**, including the intermediate `Pressed → PointerOver` step on
   release. Asserting the whole transition string, not substrings, is what proves no step was skipped.

Still open in this area: **finding #15** (UIA `SetFocus` route loses focus across a collapse), owned by
`GroupHeaderKeepsFocusAcrossCollapseWhenFocusedThroughUia` in `TableView_Accessibility_InteractionTests.cs`, which fails
by design until the product is fixed.

### §5 editing gestures: 10/10, and finding #13 no longer blocks the section

All ten §5 tests measured passing: nine in run `s5run1`, the tenth (`FocusLossCommitsEdit`) in `s5run2` after a harness
fix. A later whole-section rerun (`s5run4`) reported ten failures, all `System.ComponentModel.Win32Exception (5): Access
is denied` — input injection into a locked session, never a result.

| Test | Verdict |
| --- | --- |
| `PointerDoubleClickBeginsEditWhenEditable` | passing |
| `PointerDoubleClickDoesNothingWhenReadOnly` | passing |
| `TextColumnDoubleClickCreatesTextBox` | passing |
| `TextColumnF2CreatesTextBox` | passing |
| `TemplateColumnEditorUsesCellEditingTemplateContent` | passing |
| `EditorReceivesInitialValue` | passing |
| `EditorGetsFocusOnBeginEdit` | passing |
| `EnterKeyCommitsEdit` | passing |
| `EscapeKeyCancelsEdit` | passing |
| `FocusLossCommitsEdit` | passing (after harness fix) |

What these settle:

1. **The editor route is observable after all.** §5 was recorded as blocked because no *out-of-process* client can reach a
   cell. The block was on the observation technique, not on the behaviour: an **in-process** probe posted from
   `BeginningEdit` reads the editor without creating a single automation peer, so finding #13 is untouched. The same
   inversion already worked for row and group-header visual states; it should be tried before any future item is called
   untestable.
2. **Commit and cancel are correct on all three routes** — `Enter`, `Escape` and focus loss — against *both* the
   control's own `CellEditEnding` account and the bound item's value. The dangerous failure here is a cancel that still
   writes, and it does not happen.
3. **Both begin-edit gestures produce a real, focused, pre-populated editor**, and a template column uses its authored
   `CellEditingTemplate` rather than the built-in one.

One harness lesson, paid for once: `FocusLossCommitsEdit` first failed because its focus target had been pushed off the
end of its `StackPanel` and reported an empty rectangle, so the click landed at `(0, 0)` and never moved focus — which
read exactly like "focus loss does not commit". Any test clicking a page element by its bounds should assert the bounds
are non-empty first.

**PR parity.** The PR's 29 interaction tests contain **no editing tests at all** — its editing coverage is entirely in the
API tier. §5 owes it nothing.

### §6/§7/§8 layout tier: 4/8 (run `s678run11`) — two confirmed product findings (#17, #19), two tests unmeasurable at this tier (#20)

| Test | Verdict |
| --- | --- |
| `PointerResizeDragChangesColumnWidth` | passing |
| `UnloadDuringPendingResizeLeavesNoWedgedState` | passing |
| `VerticalScrollKeepsHeaderSticky` | passing — but vacuous about the header; see finding #20 |
| `RightToLeftKeyboardNavigationMirrors` | passing |
| `PointerResizeEscapeCancelsResize` | **FAILING — product finding #17** |
| `HorizontalScrollKeepsHeaderAligned` | **FAILING — finding #20: not measurable at this tier** |
| `FrozenColumnStaysPinnedUnderPointerScroll` | **FAILING — finding #20: not measurable at this tier** |
| `RightToLeftResizeMirrors` | **FAILING — product finding #19 (reinstated, fully characterised)** |

**Finding #17 — `Escape` does not cancel a pointer resize.** Re-measured in run `s678run11`: authored 160, and 245
after a drag cancelled with `Escape` — **85px** off, i.e. the cancel did nothing. `PointerResizeDragChangesColumnWidth`
passes in the same run (160 → 236), so the gesture and the gripper hit-test are both sound; the defect is the cancel
path alone. This is the **only** finding in this tier that has survived every harness correction, and it has now
reproduced identically across four runs.

**Finding #18 (original text, superseded by the retraction below) — the header band did not appear to track
horizontal body scroll.** With the body scrolled from `H=0` to `H=51`, the `ScrollCity` header reported
`Left=805` unchanged and `FrozenName` `625`. This was read as the `PART_HeaderScroller` sync failing off the input
path. The `HeaderH=` readout has since disproved that reading — see below.

**Finding #19 — REINSTATED and now fully characterised (run `s678run11`). It is a product defect.** It was briefly
withdrawn when manual testing reported RTL resize "working"; the rewritten, boundary-aimed gesture shows manual
testing was reading the right symptom and drawing the wrong conclusion. Measured at the `RtlName`/`RtlCity` boundary
(x=963):

| Press point | Drag | Result |
| --- | --- | --- |
| 965 — `RtlName`'s side of the boundary | 80px left | nothing (`RtlName` 180→180, `RtlCity` 220→220) |
| 961 — `RtlCity`'s side of the boundary | 80px left | **`RtlCity` 220→144** (`RtlName` unchanged) |
| 1141 — `RtlName`'s screen-right edge (table's outer edge) | 80px right | **`RtlName` 180→256** |

So **every gripper sits on its column's physical-right edge**, exactly as under LTR: `RtlCity`'s at 963, `RtlName`'s
at 1143. And the delta is **physical, not logical** — a leftward drag *shrank* `RtlCity`, a rightward drag *grew*
`RtlName`. Both halves of the pointer path are unmirrored, and consistently so, which is precisely why it looks
right by hand: every visible boundary still has a gripper and still follows the pointer. What is wrong is **which
column resizes**. Dragging the `RtlName`/`RtlCity` boundary reading-order-forward must widen `RtlName`, the
reading-order-first column; instead it shrinks `RtlCity`. That also puts the two input paths in direct
contradiction — `RightToLeftKeyboardNavigationMirrors` passes, so Left *widens* `RtlName` — and `dev-spec:131`
requires "positive always grows in reading order and **both input paths agree**".

Two suspects in the product, one for each half:
- **Placement.** `TableView.cpp:1540` stamps `logicalEndAlignment = Left` under RTL on the gripper
  (`:1993`), on the reasoning at `:304` and `:1535` that the header cell's subtree "does not observe the ambient
  FlowDirection auto-flip". The measurement says it *does* observe it, so the explicit swap is a **double mirror**
  and lands the gripper back on the physical-right (leading) edge.
- **Delta.** `ResizeGripper::OnManipulationDelta` (`:201`) negates only when the gripper and its measurement frame
  disagree on direction. Here both are RTL, so nothing is negated — on the assumption that
  `Cumulative().Translation` in an RTL container's space is already mirrored. The measured signs say it is not.

**Finding #18 — RETRACTED as stated; the header sync DOES run.** The `HeaderH=` readout added for exactly this
question answers it: body and header scrollers move **together**, `H=0;V=0;HeaderH=0` → `H=51;V=0;HeaderH=51`.
`PART_HeaderScroller` is a distinct `ScrollViewer` resolved by name and read directly, so this is not the body's
offset echoed back. The earlier claim — "the header band does not track horizontal body scroll" — is therefore
false.

**Finding #20 — scroll position is not observable through these header peers' `BoundingRectangle`, so §7 cannot
decide anything at this tier.** This replaces the stale-`AutomationElement` theory that was offered after #18 was
retracted. That theory is **ruled out**, not merely disfavoured: `FindColumnHeader` walks `tableView.Children`
live on every call, so those rectangles were never cache-served, and the `ElementCache.Clear()` added to test it
crashed the app — the next `FindElement.ById` misses, `ElementCache.Refresh()` walks `window.Descendants` reading
`.Name` on every node (`FindElement.cs:384-423`), and naming a `TableViewRow` peer manufactures cell peers and
trips finding #13 (`0xC0000420`). The calls have been removed; no assertion changed.

What the product actually does, read from source: frozen pinning is `element.Translation`
(`TableViewCellsPanel.cpp:331-333`), a **composition** property that never reaches a UIA rectangle; the unfrozen
neighbour is moved only by the scroller and additionally left-**clipped** (`:341-370`), and a `Clip` does not
shrink a rectangle either; the header band follows via `headerScroller.ChangeView` (`TableView.cpp:707`). The
decisive evidence is that two headers whose required behaviour is *opposite* — `ScrollCity` must move,
`FrozenName` must not — both reported unchanged `Left` (805→805, 625→625) while `HeaderH` went 0→51. That is one
blind measurement, not two coincident defects. The §7 tests are **left as written and failing** as the record;
the real assertions are API ones on `TransformToVisual` and the clip geometry. See §7.0 for the full write-up,
including two behaviours no test covers: pinning is prefix-only (`:321-325`) and LTR-only (`:282-298`).

**Lesson (the third and fourth ways a test in this tier has lied).** Alongside "an empty rectangle fakes movement"
and "nothing moved passes when nothing scrolled":
1. **A press aimed at an assumed edge cannot falsify a claim about where that edge is.** Any test whose subject is
   the *placement* of an affordance must aim at a **measured** boundary and report which element responded —
   otherwise a miss and a defect are indistinguishable. That mistake cost finding #19 a full retraction-and-
   reinstatement cycle.
2. **Not every failing measurement has a fixable cause — some observables are blind.** The successor to lesson 1
   was "a cached `AutomationElement` reports pre-scroll geometry, so call `ElementCache.Clear()`". **Do not do
   that in this suite:** the refresh it forces reads `.Name` across the whole tree and takes the app down through
   finding #13. It was also the wrong diagnosis (finding #20) — the rectangles were live all along and simply do
   not reflect composition translations, clips, or `ScrollViewer` offset. Before adding machinery to make a
   measurement "fresher", check the product is moving the element by something a rectangle can see; if it is
   not, the assertion belongs in the API tier and no harness change will rescue it.

**The meta-lesson, restated after both reversals ran their course.** The "most failures here are harness faults"
reading was itself a phase, and it over-corrected: of the four failures, **two are product defects** (#17 and #19,
both of which survived every harness correction aimed at them) and **two are unmeasurable at this tier** (#20).
Zero are now "the harness missed and the product is fine". What survives is narrower and more useful than the
count: before a failure is written up either way it needs a **positive control** — a reading that proves the
gesture reached the control and that the measurement can see the thing being asserted. `HeaderH=` and the
boundary probe are both that kind of control. `HeaderH=` overturned finding #18 and then, by showing two
opposite-behaviour headers reporting the same non-movement, produced #20; the boundary probe retracted #19 and
then reinstated it with a full characterisation. A control does not take a side — it just stops the argument from
being about the harness.

**Finding #14 extended — `InputHelper.Pan` and `InputHelper.RotateWheel` access-violate on a TableView peer.** Both
resolve their anchor through `IUIAutomationElement::GetClickablePoint`, which crashes on a TableView or row peer.
Peer-free replacements: `MouseWheelInput.RotateWheel(int)` (wheels at the current pointer position, so park the
pointer with `PointerInput.Move(Point)` first) and coordinate-based `InputHelper.LeftMouseButtonDown` / `MoveMouse` /
`LeftMouseButtonUp`. **Absolute points are mandatory for every TableView input in this suite.**

#### How to scroll a TableView from a test, and the two dead ends

Reaching the scroll routes took four runs. What works, and what does not:

- **Vertical — mouse wheel.** `PointerInput.Move(centre)` then `MouseWheelInput.RotateWheel(-3 * 120)`. Measured
  `V=0 → 116`. This is the only route needed for the vertical axis.
- **Horizontal — drag the `ScrollBar` thumb.** The body scroller exposes `Name='Horizontal' Class='ScrollBar'` as a
  direct child alongside the rows (bounds `626,758,502×12` on the Scrolling fixture). A 30px mouse drag of its thumb
  moved the body `H=0 → 51`. Enumerating the *scroller's* children is safe; only a **row's** children trip #13.
- **Dead end 1 — `SinglePointGesture.Current.Pan` / `.PressAndDrag`.** These drive the **mouse**, so a press-and-drag
  inside the body is a drag-*select*, not a pan: the offsets did not move at all (`ScrollCity Left 805 → 805`). Touch
  panning exists as `InputHelper.Pan`, but only against a `UIObject`, which is the crashing call.
- **Dead end 2 — Shift + wheel.** Not a horizontal scroll here: with Shift held the body scrolled **vertically**
  (`H=0;V=39`). MITA exposes no horizontal wheel at all — `MouseWheelInput` has only `RotateWheel(int)` and
  `RotateWheel(UIObject, int)`.

#### Vacuous passes: two ways a scroll test lies

1. **An empty rectangle fakes movement.** An element scrolled out of the viewport reports an **empty**
   `BoundingRectangle`, so its `Left` reads `0`. Any magnitude-only check (`|after − before| > 10`) passes when the
   element *vanishes* — which is how `HorizontalScrollKeepsHeaderAligned` first "passed". Scroll amounts are now
   deliberately small, and every assertion pairs an **on-screen guard** with a **directional** check.
2. **"Nothing moved" passes when nothing scrolled.** `VerticalScrollKeepsHeaderSticky` asserts the header does *not*
   move, so it passed for four runs while the input was reaching nothing at all. The fix is a precondition that
   proves the body scrolled — and an out-of-process client cannot read a scroll offset, because the scroller is an
   internal template part with no automation surface. So **the page publishes it**: `ScrollOffsetTextBlock` reports
   `PART_BodyScroller`'s `HorizontalOffset`/`VerticalOffset`, exactly the §5 in-process-probe pattern.
   - Resolve the part on **`LayoutUpdated`, not `Loaded`**: the Scrolling host starts `Collapsed`, so the TableView's
     `Loaded` fires before it is ever measured and therefore before `OnApplyTemplate` runs. Hooking `Loaded` reported
     `<no PART_BodyScroller>` forever. The handler unsubscribes itself once the part is found.

**Still to verify.** The readout was extended to publish `PART_HeaderScroller`'s own horizontal offset
(`HeaderH=…`) so finding #18 can be stated as "body moved, header scroller did not" without inferring it from header
peer bounds. The app is built with it; the confirming run was cut short and has not been read.

### §9/§10 tooltip and UIA-client tier: 3/6 — two new product findings (#21, #22), one blockage retracted

| Test | Verdict |
| --- | --- |
| `HeaderToolTipAppearsOnHover` | passing |
| `CellToolTipAppearsOnHover` | passing |
| `RecycledRowShowsCurrentToolTipOnHover` | passing |
| `VerifyStructureChangedEventsReachAUiaClient` | **FAILING — product finding #21** |
| `VerifyAxeScanPasses` | **FAILING — 24 errors: product finding #22** (predicted to be blocked by #13; it was not) |
| `VerifyTableIsNavigableByAUiaClient` | **FAILING — product finding #13**: headers (5) and rows (12) walk fine; the app fail-fasts at the row → cell edge |

**Finding #21 — a column added or removed at runtime raises no `StructureChanged`, so no UIA client learns the grid
changed shape.** Measured: `sort=True addColumn=False removeColumn=False`. The sort probe is a positive control and it
fired, which is what makes the other two readable — listener registered, peer raised, provider marshalled, client
notified, all on the same waiter in the same run. The raise exists only on the shaping path (`TableView.cpp:1021-1046`)
and for group expansion (`TableViewAutomationPeer.cpp:119`); column changes route through `QueueRebuildHeaders`
(`:1775/:1786`) and raise nothing. Full write-up in §10.3.

**Finding #22 — every row peer is nameless and exposes one degenerate child instead of cell peers.** 24 Axe errors, all
on the TableView, all on the same 12 elements — which is *every* row in `BasicTableView`: 12 × `NameNotNull` on the
`TableViewRow` peers themselves, and 12 × `BoundingRectangleNotNull` on the single child each row exposes, a child with
no control type, no class name, no name and no rectangle. Full write-up in §10.1.

**Finding #13 is client-dependent, and that is new information.** Axe enumerates a row's children and survives, getting
one empty placeholder; MITA asks the same question and the app fail-fasts, with `Children.Count` returning 0 *after*
the crash. The walk is otherwise sound — 5 header peers and 12 row peers resolve correctly — so the break is precisely
at the row → cell edge. Leading hypothesis: the placeholder and the crash are one defect at two levels of demand,
observing it is safe and asking the provider to populate it is not. See §10.2, including the experiment that would
settle it.

**Lesson (the fifth way this tier could have lied, and the discipline that stopped it).** `addColumn=False` on its own
is worthless — it is equally consistent with "the product is silent" and "this client never listens". The same
positive-control discipline that produced #20 and characterised #19 is what turned a null reading into a finding here.
By this point the pattern is not a per-section tactic but the tier's entry requirement: **no failure gets written up
without a reading, in the same run, that proves the channel it came through was live.**

**Countervailing lesson, and it has now cost twice: a blockage asserted from reasoning is a hypothesis.** §9's two cell
items sat deferred behind finding #13 on the reading "hovering a cell needs a cell peer" — #13 actually forbids
*descending into* a row, and a cell can be pointed at by composing the column's x from its header with the row's y from
the row peer. Both were written; both pass. Then `VerifyAxeScanPasses` was recorded as blocked because "Axe walks the
whole provider tree, so it enters a row and dies" — it walked all 12 rows *and their children* and the app survived,
and the run produced finding #22 plus a narrowing of #13 itself (§10.2). **Run the thing before recording it as
blocked.** Every remaining item deferred on #13 should be re-checked rather than left on its original reading.



Under **PowerShell 7 the script's filter is silently discarded and the entire suite runs** — ~1680 tests instead of the
dozen you asked for. Measured with the identical command line:

| Shell | `runtests.ps1 "*TableViewPointerInteractionTests*" -list` |
| --- | --- |
| `powershell.exe` (5.1) | **12** tests |
| `pwsh` 7.6.6 | **1680** tests |

Cause is argument quoting, not the query. Line 272 invokes `` .\te.exe ... "/select:`"$queryArgs`"" ``, i.e. a string
with *embedded* double quotes. Windows PowerShell 5.1 passes that to a native exe in the form TAEF expects. PowerShell 7
does not, and **neither `$PSNativeCommandArgumentPassing` mode helps**: `Windows` drops the quotes, so TAEF reads part of
the query as a file name (`No test files were found that matched the pattern "Name='**TableView...'"`), while `Standard`
escapes them (`Failed to ... match this file expression: "/select:\"`). In both cases TAEF emits a **warning, not an
error**, and then runs every test in the DLL list — which is why the failure looks like "my filter was ignored" rather
than a crash.

Two safe ways to run a subset:

```powershell
# 1. Windows PowerShell 5.1 explicitly, even from a pwsh 7 prompt:
powershell.exe -NoProfile -Command ".\runtests.ps1 '*TableViewPointerInteractionTests*' -HostingMode:None -SkipPackageUninstall"

# 2. Drive TAEF directly - no wrapper quoting involved, works from any shell:
.\te.exe Test\MUXControls.Test.dll /p:SkipConsoleWindowMinimize "/select:@Name='*TableViewPointerInteractionTests*'"
```

Add `/list` (or `-list`) first and **count what comes back** before committing to a run. A "single test" run that starts
printing ColorPicker results is this bug.

The query wildcards are *not* the problem: `runtests.ps1` prepends `*` to whatever you pass, and TAEF treats the
resulting `**Foo*` exactly like `*Foo*` — both select the same 12 tests when the argument survives intact.

### Tooling hazard: `CreateTestPayload.ps1` defaults to x86

`test\CreateTestPayload.ps1 -Configuration chk` **exits 16 (success) while refreshing only `TestPayload\x86chk`** —
`-Platform` defaults to `x86`. `TestPayload\x64chk`, which `runtests.ps1` is driven from, is left untouched, so a
freshly built test DLL silently does not ship and a run re-executes the previous binary. Symptom: `-list` does not
show a test you just compiled, while the name *is* present in `BuildOutput\bin\amd64chk\Test\MUXControls.Test.dll`.
Always pass **`-Platform x64`**, and confirm by diffing the payload DLL's `LastWriteTime` against the build output's
(robocopy preserves the source timestamp, so a match proves the copy happened).

### Keyboard tier: 14/15 (run `kbrun4`), no crashes

`kbrun3`, the first run after the Pivot came out, was 11/15. Its three remaining failures were all **test bugs, now
fixed and verified** — `TabIntoTableFocusesARowWithoutSelecting`, `PageDownMovesByViewport`, `PageUpMovesByViewport`.
One failure is left and it is a real product defect:

- `HeaderEnterTogglesSort` — **real product defect.** Enter on a focused column header does nothing; the selected row
  stays put (`964 -> 964`). Evidence in §2 below.

Both classes of test bug are recorded because both are easy to repeat:

- **Measuring a scroll with a screen coordinate.** Both paging tests measured the focused row's
  `BoundingRectangle.Top`. Paging **scrolls**, so the newly focused row lands at roughly the same screen Y and the delta
  reads ~0 even when paging works perfectly. The owner verified both keys by hand, which is what forced the re-read.
  Rewritten against `ScrollImplementation(rowsHost).VerticalScrollPercent` they now pass, with exactly the shape the
  contract predicts: from the top, one Down scrolls **0%** (the next row is already on screen) while PageDown moves
  **3.10%** of the 200-item list — one viewport — a second PageDown reaches **6.19%**, and PageUp returns to **2.95%**.
  The row peer exposes no `Name`, `AutomationId` or `PositionInSet`, so the scroll offset is the only identity-free
  measure of travel available out of process.
- **Asserting "nothing selected" after a navigation key.** `TabIntoTableFocusesARowWithoutSelecting` checked selection
  *after* pressing Down. Selection follows focus (`KeyboardFocusMoveCarriesSelection` proves it), so the key had
  legitimately selected. Any such assertion must be made **before** a navigation key.

Thirty-two tests are written across four files: `TableViewTests.cs` (§0, §10), `TableView_Keyboard_InteractionTests.cs`
(§1, §2, 15 tests), `TableView_Pointer_InteractionTests.cs` (§3, §4) and `TableView_Layout_InteractionTests.cs`
(§6, §7, §8). Only the keyboard file has been re-run since the Pivot was removed. **The pointer and layout files still
carry their pre-Pivot-removal results below and must be re-run** — several of their failures were almost certainly the
same cascade, and their Scrolling/Rtl/group tests are the likeliest to be unblocked.

**Pre-Pivot-removal results for §3/§4/§6/§7/§8 (stale, re-run before trusting):**

**Passing (7):** `TestPageLoadsAndRendersStaticTable`, `DownArrowMovesFocusToNextRow`, `UpArrowMovesFocusToPreviousRow`,
`HomeKeyMovesToFirstRow`, `HeaderKeyboardResizeChangesWidth`, `PointerResizeDragChangesColumnWidth`,
`UnloadDuringPendingResizeLeavesNoWedgedState`.

**The theme: a TableView row peer is not usable by a real UIA client.** Three independent symptoms, all on the row peer,
none visible to the in-proc §12 tests:

1. **Children enumeration asserts.** Product finding 13 in the API plan. `0xC0000420` in `Microsoft.UI.Xaml.dll` the moment
   a client asks a `TableViewRow` for its children; `TableViewRowAutomationPeer::GetChildrenCore` manufactures a fresh
   `TableViewCellAutomationPeer` per call.
2. **Asking a row for a clickable point access-violates the app.** `0xC0000005` in `Microsoft.UI.Xaml.dll`, raised from
   `IUIAutomationElement::GetClickablePoint` inside `UIObject.Click`. This is what takes down all seven §3/§4 tests: the
   ordinary MITA click idiom is unusable against a row. Coordinate-injected input avoids it.
3. **`SelectionItem` does not reach the client.** `SelectionItemImplementation.IsAvailable` is false for a row peer
   out of process, while API §12 `VerifyRowPeerSelectionItemPatternTracksSelectionMode` confirms the same peer returns a
   non-null `ISelectionItemProvider` in proc. Selection is therefore unobservable to AT, and unobservable to any
   interaction test that wants to assert it.

Taken together these say the row peer is correct when called directly and broken when marshalled — precisely the claim
§10 was written to make, now with three separate proofs rather than one.

**Real defects found by tests that did run:**

- `HeaderEnterTogglesSort` — **column headers have no keyboard route to sort.** `TableView.cpp:1642` wires sorting to
  `headerCell.Tapped` **only**, and the header cell is an anonymous `Grid` built in `RebuildHeaders` — a `Grid` does not
  raise `Tapped` from a key press the way a `Button` raises `Click`. The header takes focus and its peer advertises
  `IInvokeProvider`, so Narrator can sort; a sighted keyboard-only user cannot.
  `HeaderPointerClickTogglesSortAndUpdatesIndicator` passes on the same column, so the state machine is fine and only the
  key route is missing. **Spec gap:** `dev-spec:133` makes the header cell the keyboard target but routes only Left/Right
  (resize) and never mentions Enter, so the expectation's authority is the accessibility baseline, not a sentence.
  Contrast group headers, which *are* keyboard-operable: `TableViewGroupHeader::OnKeyDown` handles Enter and Space
  (`TableViewGroupHeader.cpp:203-213`).
- `PointerResizeEscapeCancelsResize` — Escape during a pointer drag leaves the column 87px wider instead of restoring the
  authored `GridLength`, against `dev-spec:129`. The API tier's programmatic cancel passes, so the break is in the
  pointer/capture path only.
- `PageDownMovesByViewport` — was reported as crashing the app; that was the Pivot, now removed. The test itself was
  also measuring the wrong thing. See the keyboard tier note above.
- `HorizontalScrollKeepsHeaderAligned` — crashed with `0xC0000420` in isolation, immediately after page load and before
  the test's first gesture. **Very likely the same Pivot crash** (it was the Scrolling pivot item); re-run before
  treating it as a TableView defect.

**Cascade note for anyone reading a raw log.** A crashed app poisons the rest of the TAEF run: the following tests report
`System.ComponentModel.Win32Exception (5): Access is denied` from `InjectMouseInput`, and the fixture ends with
`0xE0434352` in `AssemblyCleanup`. Those are artefacts, not findings. Injection recovers after roughly a minute. Always
re-run a suspected failure **in isolation** before triaging it.
## Item format

Every item below carries the four fields required by **AGENTS.md step 4**, plus a **Status** line because this tier has
run results worth keeping next to the expectation:

```
- [x] `TestName` — **Status:** passing / FAILING (why) / written, unverified
  - **Description:** what the test does.
  - **Expected result:** the concrete, observable outcome — specific values, not "works correctly".
  - **Failure means:** what a failure tells us is broken, in product terms.
  - **Remarks:** provenance of the expectation, and anything that weakens it. Omit only when the IDL or dev spec
    states the expectation outright.
```

`§0`, `§1` and `§2` are written out in this form. **`§3` onward are still in the older one-line prose form and must be
converted as each category comes up for review** — do not add a new item in the old shape.

## 0. Page load

- [x] `TestPageLoadsAndRendersStaticTable` — **Status:** passing.
  - **Description:** Navigates to the TestUI page, whose TableView and columns are authored in **compiled** markup, and
    inspects the resulting tree.
  - **Expected result:** the page navigates, the header row shows the authored columns, and at least one row of cells is
    realized.
  - **Failure means:** TableView is usable from `XamlReader.Load` but not from a compiled page — a generated
    `XamlTypeInfo` or `x:Bind` break.
  - **Remarks:** API §1 parses markup at runtime, so no API test can see this. Cheapest test in the document and the one
    most likely to catch a packaging or metadata regression.

## 1. Keyboard navigation

- [x] `DownArrowMovesFocusToNextRow` — **Status:** passing.
  - **Description:** Focuses the first realized row of `BasicTableView` and presses Down.
  - **Expected result:** the second row reports `HasKeyboardFocus`.
  - **Failure means:** the bubbling `KeyDown` route to row navigation is broken, so a keyboard user cannot move down the
    table at all.
  - **Remarks:** `dev-spec:165` lists Down among the six keys that move focus between rows. Asserts on the row's own
    focus state, not on a page readout.
- [x] `UpArrowMovesFocusToPreviousRow` — **Status:** passing.
  - **Description:** Focuses the second realized row and presses Up.
  - **Expected result:** the first row reports `HasKeyboardFocus`.
  - **Failure means:** reverse row navigation is unimplemented or clamps at the anchor, so movement is one-way.
  - **Remarks:** `dev-spec:165`. Kept separate from the Down test because a one-directional break is a real and common
    failure shape, not a permutation of the same code path.
- [x] `HomeKeyMovesToFirstRow` — **Status:** passing.
  - **Description:** Focuses a middle row (index 2), then presses Home.
  - **Expected result:** row index 0 reports `HasKeyboardFocus`.
  - **Failure means:** Home is unhandled, or jumps only one row, so there is no way to return to the top of a long table
    by keyboard.
  - **Remarks:** `dev-spec:165` lists Home and End together.
- [x] `EndKeyMovesToLastRow` — **Status:** passing.
  - **Description:** Focuses row 0 of `BasicTableView` (12 items, all realized), then presses End.
  - **Expected result:** the last realized row reports `HasKeyboardFocus`.
  - **Failure means:** End is unhandled or stops at the realized window, so the bottom of a table is unreachable.
  - **Remarks:** `dev-spec:165`. **Previously recorded here as a product defect; that is retracted** — it was cascade
    damage from the Pivot crash and the test passes unchanged once the Pivot is gone. Left as a warning about triaging
    a failure that follows a crashed test.
- [x] `PageDownMovesByViewport` — **Status:** passing (0% after one Down vs 3.10% after PageDown).
  - **Description:** On the 200-item `ScrollingTableView` (`Height` 300), focuses the top row, records
    `VerticalScrollPercent` after a single Down, returns Home, presses PageDown and records it again.
  - **Expected result:** a row still holds focus, and the percent after PageDown exceeds the percent after one Down by
    more than one point — a single Down keeps the next row already on screen and scrolls nothing, so any real paging
    separates the two.
  - **Failure means:** PageDown behaves like a single Down or does nothing, so a long table can only be walked one row
    at a time.
  - **Remarks:** `dev-spec:165`. **The first version of this test was wrong and its "defect" is retracted:** it measured
    the focused row's `BoundingRectangle.Top`, but paging *scrolls*, so the newly focused row lands at roughly the same
    screen Y and the delta reads ~0 even when paging works perfectly. The owner verified both keys by hand. The row peer
    exposes no `Name`, `AutomationId` or `PositionInSet`, so `VerticalScrollPercent` is the only identity-free measure of
    travel available out of process — meaning this test proves *the view paged*, and infers the focus move from it rather
    than observing the destination row directly. That is the weakness to fix if the row peer ever gains an index.
- [x] `PageUpMovesByViewport` — **Status:** passing (6.19% after two PageDowns, 2.95% after PageUp).
  - **Description:** Pages down twice on `ScrollingTableView` to earn headroom, then presses PageUp.
  - **Expected result:** the precondition percent after two PageDowns is above 1, a row still holds focus after PageUp,
    and the percent drops by more than one point.
  - **Failure means:** PageUp does not reverse PageDown, so paging is one-way.
  - **Remarks:** same measurement limitation and same retraction as `PageDownMovesByViewport`.
- [x] `TabMovesFocusOutOfTable` — **Status:** passing.
  - **Description:** Focuses row 0 — deliberately row 0 rather than the last row, which can be clipped so `SetFocus` does
    not stick — then presses Tab.
  - **Expected result:** `AfterTableButton` reports `HasKeyboardFocus`.
  - **Failure means:** the table traps Tab and walks its own cells, so a keyboard user cannot get past the control.
  - **Remarks:** `dev-spec:165` describes row-oriented keyboard handling; a table that made every cell a tab stop would
    satisfy no sentence in the spec but would be unusable. Pairs with `ShiftTabMovesFocusOutOfTableUpstream`.
- [x] `KeyboardFocusMoveCarriesSelection` — **Status:** passing.
  - **Description:** Data-driven over Down / Up / Home / End: presses each key from a known anchor and observes both
    focus and selection. No `Select` call sets up the state — the key itself is expected to establish it.
  - **Expected result:** after the key, the target row holds focus **and** reports itself selected.
  - **Failure means:** focus and selection have split — the user arrows to a row that looks focused while the control
    still acts on the old one, which is the classic "Enter hits the wrong row" bug.
  - **Remarks:** **Spec gap, recorded deliberately.** `dev-spec:165` says unhandled arrows "move focus between rows" and
    says nothing about selection; the assertion's authority is the owner's design decision, not the document, and the
    dev spec should be amended to state it. Replaces `ArrowKeysMoveFocusWithoutChangingSelection`, which asserted the
    opposite reading of the same silence — note that the reference PR `!15971489` takes that opposite position, so this
    is a genuine design difference and not an oversight. **Now proven in the product**, which imposes a rule on every
    other test in this document: an assertion that "nothing is selected" must be made **before** any navigation key,
    because navigation keys are entitled to select. Uses the `TryGetIsSelected` helper, which fails loudly when neither
    the `SelectionItem` pattern nor the property channel answers, so "cannot observe" never reads as "not selected".
- [x] `KeyboardNavigationWithSelectionDisabledMovesFocusOnly` — **Status:** passing.
  - **Description:** Sets `SelectionMode.None` through the page's `SelectionModeComboBox`, then drives the same
    navigation keys.
  - **Expected result:** the target row takes focus; no row reports itself selected.
  - **Failure means:** the mode gate lives only inside `Select` while the keyboard path writes selection behind it.
  - **Remarks:** the keyboard twin of §3's `PointerClickInSelectionModeNoneSelectsNothingButMovesFocus`; API §6 owns the
    programmatic half and neither of those touches key routing, which is why all three exist. The combo item's
    `AutomationProperties.Name` is `SelectionModeNone`, not `None` — an earlier failure here was the test using the
    display string.
- [x] `LeftAndRightArrowsDoNotMoveRowFocus` — **Status:** passing.
  - **Description:** Focuses a middle row of `BasicTableView`, presses Left, then Right.
  - **Expected result:** the same row still reports `HasKeyboardFocus` after each key.
  - **Failure means:** the control has grown horizontal cell navigation that the spec does not describe, which would
    change the whole keyboard model (and would need a `CurrentCell` concept the public API does not have).
  - **Remarks:** `dev-spec:165` enumerates exactly six keys — Up, Down, Home, End, PageUp, PageDown — and calls keyboard
    handling "row-oriented". This pins the *boundary* of the contract rather than a behaviour inside it, which is why a
    cheap negative test earns its place.
- [x] `ShiftTabMovesFocusOutOfTableUpstream` — **Status:** passing.
  - **Description:** Focuses the first row, then presses Shift+Tab.
  - **Expected result:** no row holds focus and the control authored before the table (`DummyButton`) does.
  - **Failure means:** reverse tab order walks the table's internals even though forward order does not.
  - **Remarks:** ItemsView keeps the same forward/backward pair (`TabIntoItemsViewWithoutCurrentItem` /
    `ShiftTabIntoItemsViewWithoutCurrentItem`), which is the precedent for treating the two directions as separate
    concerns rather than one permutation.
- [x] `TabIntoTableFocusesARowWithoutSelecting` — **Status:** passing.
  - **Description:** Starts on the control before the table and presses Tab (up to three times) to enter it, checking
    selection **on entry**, before any navigation key.
  - **Expected result:** focus lands inside the table — on the table or a row — rather than skipping the control, and
    nothing is selected merely by entering. A following arrow key must then focus a row.
  - **Failure means:** either the table is not a tab stop at all, or entry silently selects — the asymmetry that makes
    keyboard-only use feel wrong.
  - **Remarks:** adopted from ItemsView, which tests entry and exit separately. **Its first failure was a test bug, not
    a defect:** it checked "nothing selected" *after* pressing Down, and selection legitimately follows focus. The entry
    assertion is deliberately loose about *what* takes focus, because whether the table or its first row is the tab stop
    is unstated.
- [x] `GroupHeaderArrowKeysExpandAndCollapse` — **Status:** **passing on the Tab route** (`iso_kbgrp4`; the `[Failed]`
      verdict is the teardown crash). **Moved into `TableView_Keyboard_InteractionTests.cs`**. Finding #15 remains
      **active** for the UIA `SetFocus` route — see the note above.
  - **Description:** Focuses a group header, presses Left to collapse and Right to expand, and repeats each key to check
    it is a no-op rather than a toggle. Asserts the header still holds keyboard focus across its own collapse.
  - **Expected result:** the header's `ExpandCollapse` state and the number of realized rows move together; a repeated
    key changes neither; focus stays on the header throughout.
  - **Failure means:** group headers respond to Enter/Space only, so a keyboard user navigating with arrows cannot open
    a group.
  - **Remarks:** **Spec gap** — grouping keyboard behaviour is unstated. The expectation comes from platform convention
    as implemented by TreeView (`TreeViewKeyDownLeftToRightTest` asserts exactly this, including the RTL mirror). The
    idempotence half is the part that would catch a naive `IsExpanded = !IsExpanded` handler. **`ptrrun2` result:**
    `Left` collapses correctly (`9 → 6`, header reports `Collapsed`) and then the header **loses keyboard focus**, so
    `Right` never reaches it. The focus assertion added after `kbrun4` is what converted an ambiguous row count into
    this verdict. Reproduced by hand by the user.

**Also keyboard, specified in §4:** `GroupHeaderKeyboardActivationTogglesRowVisibility` — Enter and Space on a focused
group header collapse and expand it, each key exercised as a full pair, with the `ExpandCollapse` peer and the realized
row count agreeing at every step. **Now lives in `TableView_Keyboard_InteractionTests.cs`** (region "4. Group header
input (keyboard leg)"); only the pointer leg stays in the pointer file. One file per input device, one failure mode per
test.

**Finding #15 is ACTIVE, not retracted.** What changed is coverage, not the verdict. The Tab route now proves the
keyboard *user* path works — `TabToFirstGroupHeader` focuses the page's `GoToGroupedButton` and presses Tab (bounded at
40) until the header reports `HasKeyboardFocus`; the header is the **22nd** tab stop. On that route **every assertion
in both tests passes** (`iso_kbgrp3`, `iso_kbgrp4`): Enter, Space, Left and Right each toggle `9 ↔ 6` rows with the
`ExpandCollapse` peer agreeing, the repeated arrow key is a no-op, and the header keeps keyboard focus across its own
collapse. The `[Failed]` verdicts are the grouped-test teardown crash (`0xC0000420`) only.

**What remains open (#15):** when focus is placed on the header by UIA `IUIAutomationElement::SetFocus` — the route an
assistive technology takes — the header **loses focus across its own collapse**, three times measured. A screen reader
does not Tab; it calls `SetFocus`. So the defect still has a real user behind it, and the Tab route does not cover that
user. Do **not** close #15 on the strength of the Tab-route pass.

**Why the tests moved to Tab anyway:** the claim each test makes is about *keyboard* routing, and `SetFocus` is a
provider call, not a keyboard gesture — it skipped the tab-stop resolution the test should have been proving. That is a
test-design reason, standing on its own. It is **not** justified by anything in the product source; an earlier draft of
this note argued from `CaptureFocusedGroupHeaderForRestore`, which is exactly the "shape the test to the implementation"
move the protocol forbids. Retracted.

**Still owed for #15:** ~~a test that takes focus the AT way~~ **written** — `GroupHeaderKeepsFocusAcrossCollapseWhenFocusedThroughUia`
in the new `TableView_Accessibility_InteractionTests.cs`. It places focus with UIA `SetFocus`, presses Enter, and takes
all three observations before asserting any of them. Measured (`iso_a11y1`): `SetFocus` lands focus (precondition
passes), the collapse is correct (`9 → 6`, header reports `Collapsed`), and the header then reports
**`kept focus = False`**. **Left failing** per Step 5 — this is the clean, isolated repro of #15, and the only test that
covers the assistive-technology route.

**Rule learned:** never take focus with UIA `SetFocus` when the test's claim is about keyboard behaviour. `SetFocus` is
a provider call, not an input gesture, and it produced a false product defect that survived three "confirmations".
A second crash was found and fixed on the way: `FindElement.*` called **after** the grouped table is realized forces
`ElementCache.Refresh()` to re-walk the whole tree, which descends into `TableViewRow` children and asserts the app
(finding #13). The Tab helper reuses the button `SelectGroupedPivotAndGetTable` already resolved.

**Considered against the other controls' keyboard suites and deliberately not written:**

- **Gamepad / D-pad navigation.** TreeView drives `GamepadHelper.PressButton` with `DPadLeft` and `LeftThumbstickLeft`
  alongside its key tests. Rejected here: `dev-spec:165` describes `KeyDown` routing only, and TableView states no gamepad
  or XY-focus contract. Adding one would assert a behaviour no document claims. Revisit if the spec grows a gamepad story.
- **Fast repeated keystrokes.** ItemsView keeps a whole family (`KeyDownInItemsViewAndLinedFlowLayoutWithFastKeystrokes`,
  `KeyPageDownIn…`) because rapid repeat races element realization — a real risk for a virtualized table too. Deferred,
  not rejected: it was originally blocked behind the Page Down crash, which turned out to be the Pivot and is gone.
  Write it once the rewritten paging tests have a clean run to build on.
- **RTL keyboard navigation for rows.** TreeView pairs every key test with a `ChangeFlowDirection` twin because its
  navigation is horizontal. Row navigation here is vertical, so Up/Down/Home/End do not mirror. The RTL keyboard case that
  *does* exist — the horizontal resize step — is covered by §8 `RightToLeftKeyboardNavigationMirrors`.
- **Selection-mode variations of the tab tests.** ItemsView runs its tab-navigation tests once per selection mode. Not
  repeated here: tab order is a focus concern, and the selection-mode gate is already asserted directly by
  `KeyboardNavigationWithSelectionDisabledMovesFocusOnly`. Running the tab pair twice would be duplication, not coverage.

## 2. Header input

- [x] `HeaderEnterTogglesSort` — **Status:** FAILING — real product defect, no keyboard route to sort. Confirmed in
      `kbrun4`: the selected row does not move (`964 -> 964`).
  - **Description:** Selects the bottom row of `BasicTableView`, focuses the `Age` column header, presses Enter, then
    locates the still-selected row and compares its position.
  - **Expected result:** the sort runs, so the selected row moves upward by more than 2px — the row's identity is
    tracked through selection because the row peer has no index to read.
  - **Failure means:** keyboard input never reaches the sort, so a sighted keyboard-only user cannot sort a column at
    all.
  - **Remarks:** **Confirmed product defect.** `TableView.cpp:1642` wires sorting to `headerCell.Tapped` **only**, and
    the header cell is an anonymous `Grid` built in `RebuildHeaders` — a `Grid` does not raise `Tapped` from a key press
    the way a `Button` raises `Click`. The header takes focus and its peer advertises `IInvokeProvider`, so Narrator can
    sort; the keyboard cannot. `HeaderPointerClickTogglesSortAndUpdatesIndicator` passes on the same column, which
    isolates the break to the key route. **Spec gap:** `dev-spec:133` makes the header cell the keyboard target but
    routes only Left/Right (resize) and never mentions Enter, so the expectation's authority is the accessibility
    baseline, not a sentence — the spec should be amended alongside the fix. Contrast `TableViewGroupHeader::OnKeyDown`
    (`TableViewGroupHeader.cpp:203-213`), which does handle Enter and Space.
- [x] `HeaderKeyboardResizeChangesWidth` — **Status:** passing — 8px step, 32px with Shift.
  - **Description:** Focuses the `Name` header and drives the keyboard resize path, plain and with Shift held, measuring
    the header's width after each.
  - **Expected result:** the plain step widens the column, and the Shift step widens it by strictly more.
  - **Failure means:** the resize key route is dead, or the Shift multiplier is not applied, leaving no keyboard way to
    size a column.
  - **Remarks:** `dev-spec:133` and `dev-spec:127`. The test asserts the *relationship* between the two steps rather
    than the literal 8/32px values, so a deliberate change to the step size does not produce a false failure; the
    observed values are recorded in the status line instead.
- [x] `HeaderClickDoesNotSortColumnWithCanSortFalse` — **Status:** written, unverified.
  - **Description:** Clicks the `ReadOnlyCity` header of `BasicTableView`, which is authored `CanSort="False"`, then —
    in the same test — clicks the `Age` header, which is `CanSort="True"`.
  - **Expected result:** the first two clicks leave row order untouched (the selected item's row stays within 2px of
    where it was); the following two clicks on `Age` do reorder, lifting the selected item.
  - **Failure means:** the click handler does not consult the per-column gate, so an app that opted a column out of
    click-to-sort gets sorted anyway.
  - **Remarks:** stated outright in the IDL — `TableView.idl:156-158`, *"Per-column opt-out for the click-to-sort UX.
    Default true. When false the header click handler ignores this column; programmatic `SortByColumn` still works."*
    That sentence is **about the click**, so no API test can make the claim: API §7 can only assert the other half
    (programmatic sort still works). The `Age` leg is a **negative control** and is the point of the test: without it,
    a click that misses the header entirely, or a table that stopped sorting altogether, would pass.
- [x] `HeaderClickDoesNotSortWhenCanUserSortColumnsIsFalse` — **Status:** written, unverified. Added `CanUserSortColumnsCheckBox` to the test page.
  - **Description:** Unchecks the page's new `CanUserSortColumnsCheckBox`, clicks the `Age` header twice, then re-checks
    it and clicks twice again.
  - **Expected result:** while unchecked, row order does not change; once re-checked, the same clicks reorder the rows.
  - **Failure means:** the control-wide gate is not consulted on the click route, so `CanUserSortColumns="False"` does
    not actually stop a user sorting.
  - **Remarks:** `TableView.idl:574-578` — *"header clicks do not sort; programmatic sorting still works."* Same
    reasoning as the per-column test, different property and a different code path, which is why these are two tests and
    not one: a gate implemented in one place and forgotten in the other is the exact regression being caught. The
    re-enable leg is the negative control. Needs `CanUserSortColumnsCheckBox` on the test page (default checked) — a
    static `False` table could show the gate holding but never that it lifts. That default-checked authoring is what
    broke `kbrun5`: `IsChecked="True"` raises `Checked` during `InitializeComponent`, so `OnCanUserSortColumnsToggled`
    must null-guard `BasicTableView`. See the parse-failure note in the run status above before touching this checkbox.
- [x] `HeaderClicksFollowTheColumnSortCycle` — **Status:** written, unverified. Needs `SortCycleComboBox` and the
      `Score` column on the test page.
  - **Description:** Sets the `Score` column's `SortCycle` to `DescendingAscendingNone` from the page, selects the row
    holding the **highest** score (authored at source index 5, so it is neither first nor last), then clicks the `Score`
    header three times, reading the selected row's index after each click.
  - **Expected result:** click 1 puts the tracked row at index **0** (Descending), click 2 at the **last** index
    (Ascending), click 3 back at index **5** (None — source order restored).
  - **Failure means:** repeated header clicks ignore the column's authored cycle — either they always walk
    Ascending→Descending, or the trailing `None` step is dropped, so a user cannot click back to source order.
  - **Remarks:** `TableView.idl` on `SortCycle` — the cycle "describes how one column responds to being clicked again",
    is per-column, and is "read at click time, so a change needs no rebuild", which is what makes a page combo box a
    legitimate driver. The **`None` step is the reason this needs the `Score` column at all**: `Age` and `Name` are
    authored in ascending source order, so for them "Ascending" and "None" produce the identical order and the third
    click is unobservable. `Score` is authored non-monotonic (`((i * 7) % 12) + 1`) so all three steps land the tracked
    row on three different indices. Tracking is by **selection**, which follows the item across a re-order
    (`TableView.idl:531-546`) — the same technique the other sort tests use, for the same reason: sort-state text is
    localized and finding #5 leaves it empty.
- [x] `HeaderResizeDragDoesNotSortTheColumn` — **Status:** written, unverified.
  - **Description:** Drags the `Name` column's trailing edge to widen it, then checks both the width and the row order.
  - **Expected result:** the column is wider **and** the rows have not reordered.
  - **Failure means:** a completed resize drag also runs the sort handler, so every column resize silently reorders the
    table — a bug a user would hit constantly and that the existing resize test cannot see, because it only measures
    width.
  - **Remarks:** **Spec-silent, reasoned.** `dev-spec:121-129` gives the gripper its own gesture protocol
    (`BeginDrag` → `DragDelta` → `EndDrag`) and `TableView.cpp:1642` puts a `Tapped` handler on the header cell the
    gripper sits inside; whether a manipulation that ends over the header also raises `Tapped` is exactly the kind of
    thing neither document settles. Platform convention (WPF/WinForms DataGrid, the Community Toolkit sizer) is that a
    drag is not a click. Pairs with §8 `PointerResizeDragChangesColumnWidth` and deliberately does **not** replace it:
    that one owns "the drag reaches the width engine", this one owns "the drag does not also reach the sort". The
    width check here is only a precondition proving the drag happened.
- [x] `HeaderPointerClickTogglesSortAndUpdatesIndicator` — **Status:** passing.
  - **Description:** Clicks a sortable header with real pointer input and observes the resulting sort and indicator.
  - **Expected result:** one click moves `SortDirection` one step through the column's `SortCycle`, row order follows,
    and the indicator's visual state matches.
  - **Failure means:** header hit-testing or the click handler is broken even though the sort state machine is fine.
  - **Remarks:** the API twin (§7.5 `VerifyHeaderInvokeTogglesSortAndUpdatesIndicator`) drives `IInvokeProvider` and
    never touches the pointer path; the full sort cycle stays API-only, per the one-test-per-route rule — but the
    `CanSort` / `CanUserSortColumns` **gates do not**, because the IDL words both of them as statements about the
    click. Its passing is what proves the keyboard failure above is a routing defect and not a broken sort.

**Considered for §2 and deliberately not written here:**

- **Header pointer-over / pressed visual states.** The column header cell is an anonymous `Grid` built in
  `RebuildHeaders` with no `VisualStateManager` groups, so there is no state to assert and no document claiming one
  should exist. Rejected, not deferred: writing it would invent a contract. (The *group* header is a real templated
  control and does get `GroupHeaderPointerAndPressedVisualStates` in §4.)
- **`HeaderToolTip` appearing on hover.** Real behaviour, real gap, but §9 owns tooltip-hover as a category; added there
  as `HeaderToolTipAppearsOnHover` rather than split across two sections. The `Name` column of `BasicTableView` already
  carries `HeaderToolTip="The person's name"`.
- **Double-click a gripper to auto-size.** No such API exists on `TableViewColumn`, so there is nothing to route to.

## 3. Pointer selection and focus

**The `0xC0000005` row-click verdict recorded here previously was retracted, then RE-INSTATED by `ptrrun1`.** The
retraction reasoning was sound at the time — it had been measured while the `Pivot` was crashing the app — but the crash
reproduces cleanly post-Pivot and is now product finding #14 above: it is the UIA **clickable-point** call on a row peer
that access-violates, not the click. See the run-status section for the isolation.

- [x] `PointerClickSelectsAndFocusesRow` — **Status:** written, **passing**.
  - **Description:** Parks keyboard focus on `DummyButton`, then left-clicks the first realized row of `BasicTableView`.
  - **Expected result:** the row reports `HasKeyboardFocus`, `DummyButton` no longer does, and the row peer's
    `SelectionItem` pattern reads `IsSelected == true`.
  - **Failure means:** hit-testing or the row's `PointerPressed` handler never reaches selection or focus — the control
    is unusable with a mouse even though `Select()` passes in API §6.
  - **Remarks:** `dev-spec:284` (the press establishes the row's participation in selection) and `:296-301` (pointer
    focus lands on the **row**, not a cell). Parking focus elsewhere first is what stops the focus assertion passing
    vacuously.
- [x] `PointerClickOnSecondRowMovesSelection` — **Status:** written, **passing**.
  - **Description:** Clicks row 0, asserts it is selected, then clicks row 1 and re-reads both rows.
  - **Expected result:** row 1 is selected and holds focus; row 0 reads `IsSelected == false`.
  - **Failure means:** the pointer route writes selection additively or fails to clear the previous row, so a mouse user
    can hold two selected rows in a `SelectionMode.Single` control.
  - **Remarks:** separate from the test above by **assertion**, not scenario (step 2.4): that one proves a click reaches
    selection at all, this one proves the single-selection invariant survives the pointer route. API §6 proves the
    invariant programmatically; a gate implemented in `Select()` but bypassed by the pointer handler passes there and
    fails here, which is the whole point of keeping both.
- [x] `PointerClickInSelectionModeNoneSelectsNothingButMovesFocus` — **Status:** written, **passing**.
  - **Description:** Switches `SelectionModeComboBox` to `None`, parks focus on `DummyButton`, then clicks row 0.
  - **Expected result:** nothing is selected — the row peer withholds `SelectionItem`, and if it is wrongly advertised,
    `IsSelected` is still false — while keyboard focus does move to the clicked row.
  - **Failure means:** the `None` gate lives only inside `Select()` and the pointer handler writes selection behind it;
    or the row refuses focus under `None`, making a display-only table keyboard-unreachable.
  - **Remarks:** `TableView.idl:536` — `None` is "display-only". The focus half is the deliberate asymmetry: display-only
    restricts *selection*, not reachability.
- [x] `PointerPressEstablishesCurrentCellForKeyboardEditing` — **Status:** written, **passing**.
  - **Description:** Clicks `BasicTableView` at an x-offset inside the **`Age`** column, presses F2, and reads the column
    the page's `BeginningEdit` handler reports; then repeats on the **`Name`** column. Each leg escapes the editor first
    so the next begins clean. The second leg also re-presses while the row already holds focus.
  - **Expected result:** the reported column is `Age` after the `Age` press and `Name` after the `Name` press — the
    column under the pointer, in both the fresh-row and already-focused-row cases.
  - **Failure means:** the pointer never establishes the current cell, so per `dev-spec:284` keyboard editing "silently
    fails" or edits the wrong column — the precondition every §5 keyboard-edit item stands on.
  - **Remarks:** `dev-spec:284` ("the same handler is the only place a pointer establishes the current cell") and `:286`
    (once the row has focus the press arrives with the row as source and resolution falls back to hit-testing) — which is
    why the second leg is not a repeat of the first. **The `Age` leg is the load-bearing one:** a keyboard-only fallback
    exists that resolves to the first editable column, which is `Name`, so a `Name`-only test would pass on a control
    that ignores the pointer entirely. Observed through the page's `BeginningEdit` report rather than by finding the
    editor, because locating an editor means descending into a row's children and that fail-fasts the app (finding #13).
    `TableViewBeginningEditEventArgs.Column` is public IDL surface, so this is instrumentation, not a private hook.
- [x] `RowPointerOverEntersHoverState` — **Status:** written, **passing** (isolation run).
  - **Description:** Hooks the first realized row's `CommonStates` group through a page button, moves the mouse over the
    row, then moves it away to an unrelated control, and reads the page's state log.
  - **Expected result:** the log shows the row entering `PointerOver` on hover and returning to `Normal` on exit.
  - **Failure means:** the row never enters its hover state, so rows give no pointer feedback.
  - **Remarks:** `dev-spec:95` names the row's `CommonStates` — `Normal`, `PointerOver`, `Pressed`, `Disabled` — so the
    expectation is the spec's, not the implementation's. Moved from API §5.5: `m_isPointerOver` is set only from the
    row's own pointer handlers, so no programmatic route enters the state. A **log** rather than a live read because
    reading the state requires an interaction that would itself move the pointer off the row; recording transitions as
    they happen removes that race.

### Considered and deliberately not written

- **`RowKeyboardFocusShowsFocusVisual`.** There is no observation channel. The row template has no `FocusStates` group
  and sets `UseSystemFocusVisuals` (`dev-spec:95`), so the visual is drawn by the framework *outside* the control's tree:
  no visual state to log, no UIA property, nothing short of a pixel comparison — and a pixel comparison would be testing
  XAML's focus-visual rendering, not TableView (step 2.3). The part TableView actually owns is that a row can take
  keyboard focus, and §1 asserts that in every navigation test.
- **`FocusReanchorsAcrossReshape`.** Deferred as a **spec gap**, not as work. Neither spec says where focus goes when a
  re-shape moves or drops the focused row; `dev-spec:25` puts the grouped/hierarchical focus model in a later release.
  What *is* stated — selection tracks the item across a re-shape (`TableView.idl:531-546`) — is already covered by API
  §8's `VerifySelectionReanchorsAcrossAReshape`. Writing a focus expectation here would invent a contract and then
  enshrine whatever the control happens to do, which step 5 exists to prevent. Raise it against the spec first.

## 4. Group header input

- [x] `GroupHeaderPointerAndPressedVisualStates` — **Status:** **passing** (isolation run `s4a`). Written as
      `GroupHeaderPointerAndPressedVisualStates` in `TableView_Pointer_InteractionTests.cs`.
  - **Description:** Hooks the first realized group header's `CommonStates` group through the new
    `HookGroupHeadersButton`, then drives one pointer gesture end to end: move onto the band, press, release, move off
    the table. The whole gesture runs before the log is read once at the end.
  - **Expected result:** the transition log reads exactly `hooked;PointerOver;Pressed;PointerOver;Normal;`.
  - **Failure means:** the group header gives no hover or press feedback, so a pointer user cannot tell the band is
    interactive before clicking it.
  - **Remarks:** **Spec-backed** — `TableView.idl` documents the header's states as
    "CommonStates Normal|PointerOver|Pressed|Disabled", so the names come from the contract, not the implementation.
    Not redundant with `RowPointerOverEntersHoverState` (different control, different template) nor with API §9's
    `ExpansionStates` test (different state group, reached programmatically). The API tier *cannot* hold this: these
    states are set only from the header's own pointer handlers, the same reason `TableView_Rows_APITests.cs:664`
    records `VerifyRowPointerOverVisualState` as not implementable there. Asserting the full string rather than
    substrings is what catches a missing intermediate transition — a header that jumps straight from `Normal` to
    `Pressed` gives no hover affordance. **Adds `Pressed` coverage the row test does not have.**

- [x] `GroupHeaderToggleRequestedCarriesTheGroupKey` — **Status:** **passing on both routes** (isolation run `s4b`).
      Written as **two methods, one per route**:
      `GroupHeaderPointerToggleRaisesToggleRequestedWithGroupKey` in `TableView_Pointer_InteractionTests.cs` and
      `GroupHeaderKeyboardToggleRaisesToggleRequestedWithGroupKey` in `TableView_Keyboard_InteractionTests.cs`.
  - **Description:** Hooks `ToggleRequested` on every realized group header. The page handler records the key, then
    **mutates the header** (`sender.IsExpanded = !sender.IsExpanded`), then records the key again, writing
    `"<key>|<key>;"` per raise — so the number of entries *is* the number of raises. The pointer leg clicks the first
    header's band and then the **second** header's band; the keyboard leg tabs to the first header and presses Enter,
    then Space.
  - **Expected result:** pointer — `hooked;Redmond|Redmond;` after the first band click, then
    `hooked;Redmond|Redmond;Seattle|Seattle;` after the second. Keyboard — `hooked;Redmond|Redmond;` after Enter, then
    `hooked;Redmond|Redmond;Redmond|Redmond;` after Space. Each activation raises the event exactly once and both
    readings of the key match, including the one taken after the handler re-entered.
  - **Failure means:** a handler cannot tell which group the user activated, so any per-group behaviour built on the
    event is impossible; or the event fires twice per gesture, which double-applies whatever the handler does.
  - **Remarks:** **Spec-backed** — `TableView.idl:350-355` states the key rides on the args "so a handler that
    re-enters and mutates the header still sees the key that was actually activated", which is only observable if the
    handler actually re-enters; the page handler therefore does. **Moved out of API §9.3 after measurement**
    (`TableView_Grouping_APITests.cs:575`): `RequestToggle` is raised solely from `OnKeyDown` and `OnPointerReleased`
    (`TableViewGroupHeader.cpp:214`, `:290`), while the peer's Expand/Collapse takes `RequestExpansion` — there is no
    input-free route. Split per route for the same reason as `GroupHeaderActivationExpandsAndCollapsesRows`. The
    pointer leg drives **two different headers** deliberately: a constant key would satisfy a single-header test. The
    keys come from the page fixture (`Cities = { Redmond, Seattle, Bellevue }` in source order), not from the product.
    Row counts are **not** asserted here — the re-entrant mutation deliberately double-toggles, and
    `GroupHeaderActivationExpandsAndCollapsesRows` already owns the reshape claim.

**PR parity for §4: nothing owed.** The reference PR's interaction files are `TableViewTests.cs` (19),
`ColumnResizeGripperTests.cs` (2) and `SortIndicator_InteractionTests.cs` (8) — none of them touches a group header, so
this section has no PR-side gap to close.

- [x] `GroupHeaderActivationExpandsAndCollapsesRows` — **Status:** written as **two methods in two files**:
      `GroupHeaderPointerClickTogglesRowVisibility` (click) in `TableView_Pointer_InteractionTests.cs`, and
      `GroupHeaderKeyboardActivationTogglesRowVisibility` (Enter and Space) in
      `TableView_Keyboard_InteractionTests.cs`. **Both legs pass.** The pointer leg's six assertions pass
      (`ptrrun3`); the keyboard leg's ten pass on the Tab focus route (`iso_kbgrp3`). Both report `[Failed]` only
      from the grouped-test teardown crash. Finding #15 stays **active** for the UIA `SetFocus` route, which is not
      covered by either leg.
  - **Description:** Activates the first group header of the grouped table three ways — clicking the header band,
    pressing Enter with it focused, pressing Space with it focused — and toggles each one back again. The keyboard leg
    asserts the header can take focus as a precondition.
  - **Expected result:** each activation drops the realized row count below the baseline and the header's
    `ExpandCollapseState` reads `Collapsed`; each second activation restores the **exact** baseline count and the state
    reads `Expanded`. Enter and Space are each exercised as a full collapse/expand pair.
  - **Failure means:** the gesture never reaches `TableView::ToggleGroupExpansion` — a keyboard or pointer user cannot
    open or close a group even though the API can.
  - **Remarks:** split into two methods because pointer and key routing are separate failure modes and a single method
    would hide which one broke; that is the one-test-per-*route* rule, not duplication. API §9 drives the same reshape
    through the group header peer's `IExpandCollapseProvider`, which enters at `RequestExpansion`, while both gestures
    enter at `RequestToggle` (`TableViewGroupHeader.cpp:214`, `:290`) — different entry points, so neither substitutes
    for the other. Asserting the row count returns to the *exact* baseline is what catches a collapse that re-realizes a
    different number of rows. `GroupHeaderArrowKeysExpandAndCollapse` covers Left/Right separately and is listed in §1.

**All three came from Category 9 rather than the original §11.** The rest of Category 9 is in the API
plan, driven through the group header peer's `IExpandCollapseProvider` — which reaches the reshape by a different entry
point than a gesture does, so the two tiers overlap here by design rather than by duplication.

## 5. Editing gestures

Moved from API §10. Each needs a real gesture and none is reachable through the UIA `IValueProvider` route that carries the
rest of the editing category — see §10.0 of the API plan for why that route covers the other 17 items.

> **Was blocked by product finding #13, now unblocked.** Every item below has to observe an editor *inside a cell*, and an
> out-of-process client cannot reach a cell: asking a `TableViewRow` peer for its children fail-fasts the app. The way
> round is the same one §3 and §4 already use for visual states — observe **in process**. The page's `BeginningEdit`
> handler posts a visual-tree walk (`ProbeEditor`) that finds the open editor and writes its type, automation name, text
> and focus state into `EditorProbeTextBlock`. An in-process walk creates no automation peers, so #13 is never touched.
> Commit and cancel outcomes come from the public `CellEditEnding` args (`EditEndReportTextBlock`) and from the bound
> item's own `PropertyChanged` (`FirstItemNameTextBlock`). Nothing here resolves a cell or editor **peer**, so the
> constraint still holds as written; only the conclusion that these items were untestable was wrong.

- [x] `PointerDoubleClickBeginsEditWhenEditable` — **Status:** written, **passing** (isolation run).
  - **Description:** Double-clicks `BasicTableView` at a row-relative x inside the editable `Age` column and reads the
    page's `BeginningEdit` report.
  - **Expected result:** the report reads `Age;` — the double-click opened an edit, on the column under the pointer.
  - **Failure means:** the documented primary begin-edit gesture does not work, or opens on the wrong column.
  - **Remarks:** `functional-spec:61` — "double-click / double-tap and `F2` begin an edit". `dev-spec:284` adds that this
    is driven from `PointerPressed` with click-count tracking rather than a `DoubleTapped` handler, precisely so that a
    row that marks the press handled for selection does not silently kill the gesture — which is the regression this
    catches. Verified by hand before writing: a double-click does open an editor.
- [x] `PointerDoubleClickDoesNothingWhenReadOnly` — **Status:** written, **passing**.
  - **Description:** Double-clicks inside the `ReadOnlyCity` column (authored `IsReadOnly="True"`), then double-clicks
    `Age` as a positive control.
  - **Expected result:** the report stays empty after the read-only column — **no** `BeginningEdit` is raised at all —
    and then reads `Age;` after the editable one.
  - **Failure means:** per-column `IsReadOnly` is not consulted on the gesture route, so a user can edit a column the app
    marked read-only.
  - **Remarks:** the positive control is what stops this passing vacuously — without it, a double-click that missed the
    table entirely would look like correct gating. Note the assertion is on `BeginningEdit` *not firing*: the column gate
    is checked in `TryResolveCurrentCellForEdit`, before the event, so a correct implementation is silent rather than
    cancelling.

- [x] `TextColumnDoubleClickCreatesTextBox` — **Status:** written, **passing**.
  - **Description:** Double-clicks the editable `Name` cell of the first row and reads the editor probe's type field.
  - **Expected result:** the probe reports `TextBox`.
  - **Failure means:** the edit state machine runs but no editor appears, so the user has no way to type.
  - **Remarks:** deliberately distinct from `PointerDoubleClickBeginsEditWhenEditable`, which proves the *gesture* reaches
    the state machine (event fired, right column). This proves the state machine then produces the **editing visual**
    `TableView.idl:208` names. An implementation that raised the event and swapped in nothing passes the former and fails
    this one.
- [x] `TextColumnF2CreatesTextBox` — **Status:** written, **passing**.
  - **Description:** Single-clicks the `Name` cell to make it current, presses `F2`, then reads both the begin-edit report
    and the editor probe.
  - **Expected result:** the report reads `Name;` and the probe reports `TextBox`.
  - **Failure means:** the second documented begin-edit gesture is broken, or opens on the wrong cell.
  - **Remarks:** `functional-spec:61` names `F2` alongside double-click. A separate test because the two routes enter from
    different handlers — `OnKeyDown` and `OnPointerPressed` — so a regression can break one and leave the other intact.
- [x] `TemplateColumnEditorUsesCellEditingTemplateContent` — **Status:** written, **passing**.
  - **Description:** Double-clicks inside the `Template` column (which authors a `CellEditingTemplate` whose root carries
    `AutomationProperties.Name="TemplateCellEditor"`) and reads the probe's automation-name field.
  - **Expected result:** the probe reports `TemplateCellEditor`.
  - **Failure means:** a template column falls back to a built-in editor and the app's authored editing UI is ignored.
  - **Remarks:** `TableView.idl:208-218` — `CellEditingTemplate` is "the supported way to customise an editor". Not
    API-testable: such a column deliberately does not advertise the Value pattern, so `SetValue` cannot open it (API §10.1
    covers that advertisement gate). The assertion is on the automation **name**, not the type, because both editors are
    `TextBox`es — type alone would not prove the template was used. The test also guards that the column is inside the
    row's bounds first, so a click that fell short cannot read as a template failure.
- [x] `EditorReceivesInitialValue` — **Status:** written, **passing**.
  - **Description:** Double-clicks the first row's `Name` cell and reads the probe's text field.
  - **Expected result:** `Person 0` — the seeded value of that item.
  - **Failure means:** the editor opens blank, so committing an untouched edit silently erases the cell.
  - **Remarks:** not API-testable: `SetValue` overwrites the initial value inside the same call, so there is no moment at
    which an API test could observe it. The probe runs at Low dispatcher priority, i.e. after the editor is generated and
    laid out, which is the earliest moment the value is observable at all.
- [x] `EditorGetsFocusOnBeginEdit` — **Status:** written, **passing**.
  - **Description:** Double-clicks the first row's `Name` cell and reads the probe's focus field.
  - **Expected result:** `focus=True`.
  - **Failure means:** the editor exists but typing goes to the table's key handling instead of into the cell.
  - **Remarks:** every keyboard leg below silently depends on this, which is why it is asserted on its own rather than
    inferred from them. The probe walks up from the focused element, so an editor whose inner content part takes focus
    still counts — the contract is "focus is in the editor", not "the editor is the focused element".
- [x] `EnterKeyCommitsEdit` — **Status:** written, **passing**.
  - **Description:** Opens the `Name` editor, selects all, types `Renamed`, presses `Enter`, then reads the
    `CellEditEnding` report and the bound item's `Name`.
  - **Expected result:** the report reads `Commit;` and the item's `Name` is `Renamed`.
  - **Failure means:** `Enter` does not close the edit, or closes it without writing the value through.
  - **Remarks:** two observations, both required and both taken before either is asserted (`Verify` throws here). The
    control's own account of the outcome and the data are independent claims: an implementation that reported `Commit`
    without pushing the value passes the first and fails the second.
- [x] `EscapeKeyCancelsEdit` — **Status:** written, **passing**.
  - **Description:** Same setup, types `Discarded`, presses `Escape`.
  - **Expected result:** the report reads `Cancel;` and the item's `Name` is still `Person 0`.
  - **Failure means:** a cancel that still wrote the value — silent data loss, the worst failure in this section.
  - **Remarks:** the mirror of `EnterKeyCommitsEdit`. Select-all before typing so the assertion is on an exact value
    rather than a concatenation.
- [x] `FocusLossCommitsEdit` — **Status:** written, **passing** (after one harness fix, see run status).
  - **Description:** Opens the `Name` editor, types `CommittedByFocusLoss`, then clicks `AfterTableButton` — a no-op
    button outside the table — and reads the same two readouts.
  - **Expected result:** the report reads `Commit;` and the item's `Name` is `CommittedByFocusLoss`.
  - **Failure means:** a user who types and then clicks elsewhere loses the edit.
  - **Remarks:** the gesture is a real click on a real button, which is how an editor loses focus in practice. The test
    asserts the focus target has a non-empty bounding rectangle first: the first run clicked at `(0, 0)` because the
    button it used had been pushed off the end of its panel, and that read as a product failure.

## 6. Pointer resize

The programmatic drag lifecycle is already covered by API §4 through `ResizeGripper.BeginDrag` / `TryDrag` / `EndDrag`. These
three exist for the pointer plumbing on top of it — hit-testing the gripper, capture, cancel, and capture release. All three
run on the Basic table's `Name` column and share the `DragColumnBoundary` helper, which presses one pixel inside the header's
trailing edge (MITA offsets are **centre**-relative, so the offset is `Width/2 - 1`) and moves in absolute screen points —
`InputHelper.Pan` and anything else that resolves an anchor via `GetClickablePoint` access-violates on a TableView peer
(finding #14).

**Run status:** 2 of 3 passing. `PointerResizeEscapeCancelsResize` fails on product finding #17.

- [x] `PointerResizeDragChangesColumnWidth` **(passing)** — Dragging the gripper with the pointer resizes the column.
  - **Description:** On the Basic table (`CanUserResizeColumns="True"`, every column `CanResize="true"` by default —
    `TableView.idl:140, :469`), reads the `Name` header's width, presses the column boundary and drags 80px right, then
    re-reads the width.
  - **Expected result:** the width grows by more than 20px. The assertion is on the **delta**, not an absolute:
    bounding rectangles are screen-relative, and the claim is that the drag reached the width engine at all, not that it
    grew by exactly 80. Slack covers the gripper width, the 0.5 DIP deadband and manipulation rounding.
  - **Failure means:** gripper hit-testing, pointer capture, or the manipulation→`TryDrag` handler is broken even though
    the width engine itself is fine — API §4 already proves `BeginDrag`/`TryDrag`/`EndDrag` write the width.
  - **Remarks:** `dev-spec:127` ("the table owns … clamping the reported delta to MinWidth/MaxWidth, writing Width") plus
    the `:129` protocol. Positive delta grows in **reading order** (`dev-spec:131`), which under LTR is rightward — §8's
    `RightToLeftResizeMirrors` is the same claim with the mirror applied. This test is also the control for #17: it passes
    in every run that #17 fails, which localises the defect to the cancel path.
- [x] `PointerResizeEscapeCancelsResize` **(FAILING — product finding #17; Escape does not restore the authored width)**
  - **Description:** Presses the gripper, drags the column wider, presses `Escape` **while the pointer is still down**, then
    releases, and compares the width against the authored one.
  - **Expected result:** the width returns to the authored value (tolerance 8px).
  - **Failure means:** a user cannot back out of a resize; the drag is committed the moment it moves.
  - **Remarks:** `dev-spec:129` cancel semantics. Re-measured in every run since: the width is **85px** off the authored
    value after `Escape`, i.e. the cancel does nothing at all. `PointerResizeDragChangesColumnWidth` passes in the same
    run, so the gesture and the gripper hit-test are both fine — this is the cancel path alone. Left failing by design.
- [x] `UnloadDuringPendingResizeLeavesNoWedgedState` **(passing)** — Unloading the TableView mid-drag releases pointer
  capture and leaves no stuck resize state. *Moved from API §16.4.*
  - **Description:** Clicks `DelayedUnloadButton`, which removes the Basic table at +1.5s and re-adds the same instance at
    +3.0s. Immediately begins a real resize drag on the `Name` boundary and holds the pointer **down** for 2.2s, so the
    unload lands mid-gesture. Continues the move and releases into empty space, waits for the reload, re-queries the
    control, and performs an ordinary 80px resize drag.
  - **Expected result:** the post-reload drag widens the column by more than 20px — identical to
    `PointerResizeDragChangesColumnWidth`.
  - **Failure means:** capture or resize state survived the teardown and wedged the gripper, so the control comes back
    alive but no longer resizable — a leak a user hits by scrolling a list that virtualizes mid-drag.
  - **Remarks:** a "pending resize" is a captured pointer between `BeginDrag` and `EndDrag`. The API can drive that
    lifecycle, but only a real drag holds the **capture**, which is the half that can wedge, and nothing in the peer
    surface opens or observes capture — so this fails the admission rule's peer test and belongs here. `dev-spec:129`
    lists "a canceled contact, the header rebuilt mid-drag" among the torn-down gestures the primitive must survive; an
    unload is exactly that teardown. Two harness details matter: the moves after the unload use **absolute points** so
    nothing dereferences the now-detached header peer, and the old `UIObject` is stale after the remove/add so the
    control and header are both re-queried. `StatusTextBlock` ("Unloaded"/"Reloaded") is page state used only as a timing
    story — it is deliberately never asserted; the assertion is the width change of the reloaded control.
    **Watch:** failed once in run `s678run7` (236 vs 256) and passed either side, so it may be timing-sensitive on the
    fixed 2.2s/2.0s waits. If it flakes again, stabilise it on the status readout rather than on wall-clock sleeps.

## 7. Scrolling

Kept here by decision rather than by necessity: a programmatic `ChangeView` would not exercise the wheel, drag and inertia
offsets these have to survive. Note the deliberate contrast with API §15, where programmatic scroll *is* correct because
virtualization is a function of offset, not of input.

### §7.0 Finding #20 — scroll position is not observable through these peers' `BoundingRectangle`

**This supersedes the stale-`AutomationElement` theory that replaced finding #18. That theory is now dead, and the
three tests below are left exactly as written, failing, as its record.** All three assert on a header peer's
`BoundingRectangle` before and after a real scroll; the product moves headers by mechanisms that a UIA rectangle
does not report, so the measurement can never move regardless of whether the product is correct.

Read out of the product source:

- **Frozen pinning is a composition property.** `TableViewCellsPanel::ApplyFrozenColumnLayout`
  (`TableViewCellsPanel.cpp:275`) pins a leading-frozen cell with
  `ElementCompositionPreview::SetIsTranslationEnabled` + `element.Translation({+horizontalOffset, 0, 0})` and
  `Canvas.ZIndex = 1` (`:331-333`). A composition translation is applied below the XAML layout/transform chain that
  feeds UIA, so the pinned element's `BoundingRectangle` reports its **unpinned** position. "Stayed pinned" is
  therefore not expressible out of process — and worse, it is indistinguishable from "did not move at all".
- **The unfrozen neighbour both moves and is clipped.** It moves only because the scroller carries it; nothing
  translates it. Additionally `:341-370` gives it a left `RectangleGeometry` clip,
  `clipLeft = max(0, leadingFrozenWidth + horizontalOffset - panelX)` with
  `clipWidth = max(0, cellWidth - clipLeft)`, so the part sliding under the pinned band is hidden and a fully
  covered cell gets a zero-width clip. A `Clip` does not shrink `BoundingRectangle` either.
- **The header band follows the body by scrolling, not by layout.** `OnBodyScrollerViewChanged` calls
  `headerScroller.ChangeView(bodyHOffset, nullptr, nullptr, true)` (`TableView.cpp:707`). Measured in run
  `s678run11`, that offset arrives — the page reported `HeaderH=0 → 51` — while **both** header peers reported
  an unchanged `Left` (`ScrollCity 805→805`, `FrozenName 625→625`). Two headers whose product-side behaviour is
  *opposite* (one must move, one must not) reporting the identical "no change" is the signature of a measurement
  that is blind to scroll offset, not of two coincident product bugs.

Consequences, recorded rather than acted on:

1. **The three tests below stay as they are, failing.** They are not adjusted to match the implementation
   (Step 5) and they are not deleted: a test that fails because its only observable is blind is evidence about
   the observable, and the next reader needs to find that evidence attached to the test that produced it.
2. **The real assertions belong in the API tier**, where `TransformToVisual` against the TableView sees the
   composition translation and the clip. Raise as API items: leading-frozen screen x is invariant under
   horizontal scroll; the first unfrozen column's clip geometry matches `leadingFrozenWidth + offset - panelX`;
   the header scroller's offset tracks the body's.
3. **Pinning is prefix-only and LTR-only**, neither of which any test here covers: the first non-`Leading`
   column ends the frozen prefix so later `Leading` columns are *not* pinned (`:321-325`), and under RTL pinning
   is skipped entirely with translation, z-index and clip reset (`:282-298`). Both are API items.

**Separately — `ElementCache.Clear()` has been removed from all three tests (harness fix, no assertion touched).**
It was added on the stale-cache theory and it crashed the app: the next `FindElement.ById` misses, runs
`ElementCache.Refresh()`, which walks `window.Descendants` reading `.Name` on **every** node
(`FindElement.cs:384-423`); computing a `TableViewRow` peer's name manufactures cell peers and trips product
finding #13, killing the host with `0xC0000420` mid-test. `SelectPivotItem` already avoids name-based search for
exactly this reason. It also bought nothing: `FindColumnHeader` walks `tableView.Children` live on every call, so
those rectangles were never served from that cache — which is what rules the stale-cache theory out rather than
merely disfavouring it.

- [ ] `HorizontalScrollKeepsHeaderAligned` **(FAILING — cause identified, finding #20; kept as written)**
  - **Description:** Selects the Scrolling table (520px viewport, ~840px of columns), drags the body scroller's
    horizontal `ScrollBar` thumb 30px right, then re-reads the `ScrollCity` header's `Left`. The page publishes
    `PART_BodyScroller`'s offsets **and** `PART_HeaderScroller`'s own offset so the test can prove the body
    really scrolled and tell a sync failure from a reporting failure.
  - **Expected result:** the body's horizontal offset increases **and** the header is still on screen with its
    `Left` decreased by more than 10px — headers track the body.
  - **Failure means:** the header↔body horizontal sync never runs off the input path, so headers and cells
    visibly separate as soon as a user scrolls sideways.
  - **Remarks:** superseded by §7.0 (finding #20). Measured in run `s678run11`: `H=0;V=0;HeaderH=0` →
    `H=51;V=0;HeaderH=51`, so the header scroller *does* move in lockstep with the body — the sync runs, and the
    finding #18 retraction stands. Yet `ScrollCity` reported `Left=805` both before and after. The stale
    `AutomationElement` theory that was offered next is now **ruled out**, not merely disfavoured:
    `FindColumnHeader` walks `tableView.Children` live on each call, so nothing here was ever cache-served, and
    the `ElementCache.Clear()` added to test it crashed the app through finding #13 instead. What remains is that
    a `ScrollViewer` offset applied by `ChangeView` is not reflected in these header peers' `BoundingRectangle`,
    so the assertion's only observable cannot see the thing it asserts. Manual testing reports horizontal
    scrolling working, which agrees with `HeaderH=51`. **Left failing on purpose**; the movement claim is
    re-raised as an API item on `TransformToVisual` per §7.0.
    The full plan claim ("in lockstep with cells") keeps its finding-#13 limitation: a cell's x is unreachable
    out of process, so this asserts the header half.
- [ ] `VerticalScrollKeepsHeaderSticky` **(passing, and the pass is vacuous — see §7.0)**
  - **Description:** Wheels 3 notches down over the Scrolling table and re-reads the `FrozenName` header's `Top`.
  - **Expected result:** the body's vertical offset increases **and** the header's `Top` moves by ≤4px.
  - **Failure means:** the header participates in vertical scrolling and slides off the top of the control.
  - **Remarks:** measured `H=0;V=0;HeaderH=0` → `H=0;V=116;HeaderH=0` with `Top` fixed at 473. The offset
    precondition is what makes this non-vacuous *about the body*: an earlier version "passed" while nothing
    scrolled at all. The header half of it is vacuous, though — this is a **negative** assertion, and §7.0 shows
    the measurement is blind to scroll position in the first place, so it would report "the header did not move"
    whether or not the header moved. Read the pass as "the wheel reaches the body scroller", which is real, and
    not as evidence of stickiness. `ElementCache.Clear()` was added here on the stale-cache theory and has been
    removed with it. The stickiness claim moves to the API tier with the rest of §7.0; this test is **kept as
    written** so the vacuity is recorded where it happened rather than silently repaired.
- [ ] `FrozenColumnStaysPinnedUnderPointerScroll` **(FAILING — not provable at this tier; see §7.0)** — A
      `FrozenEdge.Leading` column stays pinned while the body is scrolled horizontally.
  - **Expected:** the body scrolls, the unfrozen `ScrollCity` header shifts, and the frozen `FrozenName` header's
    on-screen x does not move.
  - **Failure means:** frozen-column layout is applied from the programmatic scroll path only. API §4's
    `VerifyLeadingFrozenColumnStaysPinnedDuringHorizontalScroll` sets the offset directly and so never exercises
    real input.
  - **Remarks:** superseded by §7.0 (finding #20), and this is the item that identifies it. The test fails on its
    **precondition**, not its subject: the body scrolled (`H=0 → 51`, `HeaderH=0 → 51`) but `ScrollCity` reported
    `Left 805→805` and `FrozenName` `625→625`, so "the unfrozen column actually shifted" could not be
    established. Two headers that must behave *oppositely* reporting the same non-movement is what exposed the
    measurement rather than the product. Beyond that, the subject itself is unobservable here: pinning is
    `element.Translation` (`TableViewCellsPanel.cpp:331-333`), a composition property that never reaches a UIA
    `BoundingRectangle`, and the neighbour's occlusion is a `Clip` (`:341-370`), which does not shrink one
    either — so even a perfectly working product reports "nothing moved". The precondition is what stops that
    from being scored as a pass, and it must stay for that reason. **Kept as written and failing**; the real
    assertion is an API one on `TransformToVisual` plus the clip geometry, per §7.0.

## 8. Right-to-left

- [ ] `RightToLeftResizeMirrors` **(FAILING — product defect, finding #19 reinstated and re-localized to the pointer path)**
  - **Description:** Switches to the `Rtl` pivot (the TableView authors `FlowDirection="RightToLeft"` in XAML),
    measures the boundary `RtlName` shares with `RtlCity`, presses **that boundary** and drags leftward, then
    re-reads both columns' widths.
  - **Expected result:** `RtlName` — the reading-order-**first** column, laid out at the screen right — grows by
    more than 20px, exactly as the LTR rightward drag grows a column. RTL mirrors the *direction* of the
    gesture, not its outcome.
  - **Failure means:** the resize affordance did not move with the mirrored layout, so under RTL the gripper is
    no longer on the boundary the user sees between two columns, or the pointer delta is not mirrored.
  - **Remarks:** the rewritten test has now run, and it falsifies the retraction rather than confirming it. The
    three-probe gesture did its job: the run reports *which* element responded, so this is no longer an argument
    about where a press landed.

    Measured layout (RTL, screen coordinates): `RtlName` X=963 W=180 (spans 963–1143, screen-rightmost, and so
    reading-order **first**), `RtlCity` X=743 W=220 (743–963), `RtlAge` X=625 W=118 (625–743). The
    `RtlName`/`RtlCity` boundary is therefore x=963.

    | Probe | Press | Drag | Result |
    |---|---|---|---|
    | 1 | boundary from `RtlName`'s side (+2, x≈965) | −80px (leftward) | nothing: `RtlName` 180→180, `RtlCity` 220→220 |
    | 2 | boundary from `RtlCity`'s side (−2, x≈961) | −80px (leftward) | `RtlCity` **220→144**, `RtlName` unchanged |
    | 3 | `RtlName`'s physical-**right** outer edge (x≈1141) | +80px (rightward) | `RtlName` **180→256** |

    Two independent conclusions follow, and together they say the pointer resize path ignores `FlowDirection`
    entirely:

    - **Gripper placement is LTR-shaped.** The band on the x=963 boundary belongs to `RtlCity` — i.e. to the
      column on the boundary's *physical left* — which is each column's **physical-right** edge, not its
      reading-order-trailing edge. Under a mirrored layout `RtlName`'s trailing edge *is* x=963, so that boundary
      should have been `RtlName`'s. Probe 3 is the confirmation the plan pre-registered as the one signature that
      would still mean "the gripper was left on the physical-right edge as if the layout were LTR": `RtlName`'s
      own gripper is stranded on the **table's outer edge**, where there is no neighbouring column at all, and
      dragging that outer edge resizes a column — which no user would predict.
    - **The pointer delta is not mirrored either.** Probe 2 shrank on a leftward drag and probe 3 grew on a
      rightward one: screen-rightward grows, exactly as under LTR. dev-spec:131 requires positive to grow in
      **reading order**, which under RTL is leftward.

    **This corrects the source reading recorded in the previous revision of this entry.** That reading argued
    from `ResizeGripper.cpp:177/:201` that no negation is applied because the gripper and its
    `ManipulationContainer` agree on direction, and that `Cumulative().Translation` therefore already arrives in
    the RTL container's mirrored space. The first half is evidently true — no negation is applied — but the
    second half is not: the translation behaves as screen-space. The two mistakes cancelled into a prediction
    that the run has now contradicted.

    **`RightToLeftKeyboardNavigationMirrors` passing is what localizes the bug.** That path mirrors on the
    gripper's *own* `FlowDirection` (`ResizeGripper.cpp:348`) and works, which proves the gripper does see
    `RightToLeft`. So this is not "RTL never reaches the gripper" — it is the pointer path specifically, on both
    its placement and its delta.

    The earlier "manual testing reports RTL resize working" note is **superseded**; it is what motivated the
    retraction, and the instrumented run disagrees with it. Note *why* it read as working: the two unmirrored
    halves are consistent, so every visible boundary still has a gripper and still follows the pointer. The
    defect is **which column resizes**, which is invisible unless you watch the widths. Product suspects for each
    half — the `logicalEndAlignment = Left` double mirror at `TableView.cpp:1540`/`:1993`, and the unnegated
    delta at `ResizeGripper.cpp:201` — are recorded in the tier summary above.
  - **Lesson:** a press aimed at an assumed edge cannot falsify a claim about where that edge is. Aim input at a
    **measured** boundary and let the run report which element responded — and keep the last-resort probe, since
    here it is the probe that produced the diagnosis rather than merely the verdict.
- [x] `RightToLeftKeyboardNavigationMirrors` **(passing)** — Under RTL, horizontal keyboard resize is mirrored.
  - **Description:** Under `FlowDirection.RightToLeft`, focuses the header and presses Left then Right.
  - **Expected result:** `Left` widens the column (reading-order growth) and `Right` shrinks it — measured
    `IsGreaterThan(228, 182)` then `IsLessThan(180, 226)`.
  - **Failure means:** the keyboard path reads the raw arrow key instead of resolving it against flow direction.
  - **Remarks:** this passes, and the keyboard path is a *different* code path from the pointer one
    (`TryKeyboardStep` mirrors on the gripper's own `FlowDirection`, `ResizeGripper.cpp:348`, while the pointer
    path mirrors via the manipulation container's space, `:201`). Finding #19 was briefly withdrawn as a harness
    fault and is now **reinstated** against the pointer path, so this test recovers its original role as the
    localizing evidence: it proves the gripper *does* see `RightToLeft`, which is what makes the pointer path's
    failure to mirror a pointer-path defect rather than a missing `FlowDirection`.

## 9. Tooltip hover

API §14 asserts the `ToolTip` object TableView attached to a cell or header, and the `HelpText` its peer reports. Neither
opens a popup: `ToolTipService` shows on pointer dwell, and nothing programmatic triggers that. These cover the showing,
which is the part a user experiences.

> **The two cell items are no longer blocked — the blockage dissolved once it was stated precisely.** Finding #13 bites on
> *descending into* a row peer. Hovering a cell does not require that: a cell's position is the intersection of a **column**
> and a **row**, and each of those is readable from a peer that is already safe. The column's x comes from its header
> (`tableView.Children[0].Children`, the descent §6–§8 use throughout); the row's y comes from the row peer itself
> (`rowsHost.Children`, one hop, never asking the row for children — exactly what §3's pointer tests already do). Headers
> and cells share `TableViewCellsPanel`'s column geometry, so the header's horizontal centre is over that column in every
> row. Reading the tooltip afterwards was never the blocked part either: per §9.0 the popup sits on a sibling layer. All
> three §9 items are therefore written.

All three items **pass**. Two results worth carrying forward:

1. **§9.0's third point is confirmed in practice.** A cell is reachable by composing the column header's x with the row
   peer's y — no cell peer, no crash. Both cell items ran clean, including the one that scrolls. This is the general
   escape from finding #13 for any future item that needs to *point at* a cell, and it should be reused rather than
   rediscovered.
2. **Tooltips survive recycling correctly.** `RecycledRowShowsCurrentToolTipOnHover` passing means `CellToolTipBinding`
   is re-evaluated when a container is recycled onto a new item, not wired once at creation.

> **Still to be filled in from the run logs:** which layer the popup actually landed on. `FindOpenToolTip` searches the
> windowed layer (`UIObject.Root.Children`) first and the in-frame layer second, logging both, precisely so the first
> run would settle it. The tests pass either way, so this is a documentation gap rather than a correctness one — but the
> answer belongs here, and the search can be narrowed to the one real layer once it is known.

### §9.0 No §9 item is blocked, and why the blockage was narrower than it looked

Finding #13 bites on **descending into a row or cell peer**, not on "reading a tooltip". Three things follow, and the
third retires the two items that were deferred:

1. **A header tooltip never involves a row.** The dwell target is a header cell, which `FindColumnHeader` already reaches
   safely by walking `tableView.Children[0].Children` and no further — the same descent §6–§8 have used throughout.
2. **The popup is not in the TableView's subtree at all.** A `ToolTip` is hosted on a sibling popup layer, so reaching it
   never requires descending through the control that contains the rows.
3. **A cell can be *pointed at* without being *resolved*.** The constraint is on asking a row for its children, not on
   hovering one. Composing the hover point from the column header's x and the row peer's y addresses any cell in the
   table while touching only peers that §3 and §6–§8 already read safely.

What is genuinely unsafe is the *search API*, not the tree. **`FindElement.ById` / `ByName` must not be used for this.**
They miss, call `ElementCache.Refresh()`, and that walks `window.Descendants` reading `.Name` on **every** node
(`FindElement.cs:384-423`) — which is exactly the row-peer descent finding #13 kills the app for. This is the same trap
that took down the §7 run. Use a scoped search instead: `UIObject.Root.Children.Find(...)` / `TryFind(...)` matches only
among **top-level windows** without descending into app content, which is how `Application.cs:129/:554` and
`TabViewTearOutTests.cs:110` already find windows in this repo. Resolve the popup that way first, then search **within
that popup's** small subtree for the text.

**The route has to be established by the first run, not assumed.** `AnnotatedScrollBarInteractionTests.cs:273` finds a
tooltip by `FindElement.ById("PART_DetailLabelToolTip")`, which proves a tooltip *is* reachable from the app window in
this infrastructure but says nothing about which layer it sits on, because the global walk would find it either way.
So the first version must **log the tree it searched and where it found the popup**, and that result gets written here.
If the popup turns out to be windowed, it is a `UIObject.Root` child; if it is in-frame, it is under the app window and
the scoped search has to start there instead. Either way the walk must never enter the rows repeater.

**Do not copy TabView's tooltip tests.** `ToolTipDefaultTest`, `ToolTipUpdateTest` and `ScrollButtonToolTipTest`
(`TabViewTests.cs:690/:709/:785`) all call `PressButtonAndVerifyText(...)`: they press a page button and read a
`TextBlock` the page wrote. That is the shape AGENTS.md Step 1 rules out — it asserts what the page chose to display, not
what the control did — and none of them opens a popup, so they do not cover this tier's subject at all.

- [x] `HeaderToolTipAppearsOnHover` **(new — the header half) — PASSING**
  - **Description:** On the Basic pivot, hovers the `Name` column header — which authors
    `HeaderToolTip="The person's name"` (`TableViewPage.xaml:102`) — dwells for the `ToolTipService` delay, then finds
    the opened popup by the scoped route in §9.0 and reads its text. Moves the pointer off the header and re-checks that
    the popup is gone.
  - **Expected result:** a popup appears carrying the text `The person's name`, and it dismisses when the pointer
    leaves the header. The `Age` column, which authors no `HeaderToolTip`, opens nothing on the same dwell.
  - **Failure means:** the header tooltip is attached but never shown — dwell, hit-testing, or the `ToolTipService`
    wiring on the header cell is broken. None of that is visible to API §14, which asserts the attached `ToolTip`
    object and never opens it.
  - **Remarks:** the attachment itself is already covered and must **not** be re-asserted here —
    `VerifyStringHeaderToolTipProducesToolTipWithThatText`, `VerifyNonStringHeaderToolTipContentIsHosted` and
    `VerifyNullOrEmptyHeaderToolTipProducesNoToolTip` (`TableView_ToolTips_APITests.cs:585/:618/:645`) own it. The
    gesture route is the only uncovered part, per Step 1's "per gesture route, not per assertion". The negative
    `Age` check is included because it is the cheap way to prove the dwell actually ran: a dwell that never fires and
    a column that correctly has no tooltip are indistinguishable otherwise — the same positive-control discipline
    `HeaderH=` supplied in §7.
    Note the contract detail worth not tripping over: `HeaderToolTip` is **not** a `Binding` — `TableView.idl:116-118`
    says so outright ("a header is not bound against a row, so there is nothing to defer"), and headers rebuild
    wholesale with no refresh path (`TableViewToolTipHelpers.h:291`). Nothing in this item should imply a per-row
    binding; that is `CellToolTipBinding`'s story, below. Headers also pass `publishHelpText: false` (`:303`), so
    unlike a cell the header publishes no `AutomationProperties.HelpText` — do not assert one.

- [x] `CellToolTipAppearsOnHover` **(new — was deferred; unblocked by §9.0's third point) — PASSING**
  - **Description:** On the Basic pivot, hovers row 0's `Name` cell — the `Name` column authors
    `CellToolTipBinding="{Binding Name}"` (`TableViewPage.xaml:101`) — dwells for the `ToolTipService` delay, reads the
    opened popup, then moves off and re-checks that it is gone. The cell is addressed by composing the `Name` header's x
    with row 0's y; no cell peer is resolved.
  - **Expected result:** a popup appears carrying **that row's** value, `Person 0` (`TableViewPage.xaml.cs:114`), and it
    dismisses on exit. The `Age` column, which authors no `CellToolTipBinding`, opens nothing on the same dwell.
  - **Failure means:** cells advertise per-row detail that never reaches a user — or reaches them carrying the wrong
    row's value. API §14 sees neither: it asserts the attached `ToolTip` and the peer's `HelpText`, and never opens a
    popup.
  - **Remarks:** the `Name` column sets **both** a `HeaderToolTip` and a `CellToolTipBinding`, which is what makes the
    second assertion a real discriminator rather than a restatement: if the hover landed on the header band, or if the
    header's tooltip were attached column-wide, the text would read `The person's name`. The negative `Age` check runs
    **first**, for the same positive-control reason as the header item — a dwell that never fires and a column that
    correctly has no tooltip are otherwise indistinguishable.

- [x] `RecycledRowShowsCurrentToolTipOnHover` **(new — was deferred; unblocked by §9.0's third point) — PASSING**
  - **Description:** On the Scrolling pivot, hovers the top row's `FrozenName` cell and records the tooltip, drags the
    body scroller's vertical `ScrollBar` thumb past the end of its track to pin the list at maximum offset, then hovers
    the bottom-most fully-visible row's cell in the same column.
  - **Expected result:** the second tooltip reads `Seattle` — the bottom row at maximum scroll is item 199, and
    `199 % 3 == 1` over `{Redmond, Seattle, Bellevue}` (`TableViewPage.xaml.cs:93/:119`). The first reads `Redmond`
    (item 0).
  - **Failure means:** the tooltip binding is evaluated once at container creation and never re-evaluated, so tooltips
    go stale the moment a user scrolls. A reading of `Redmond` after the scroll is that failure exactly. This is the
    failure most visible to a user and least visible to any assertion made on freshly realized rows.
  - **Remarks:** `FrozenName` displays `{Binding Name}` but tooltips `{Binding City}` (`TableViewPage.xaml:177-181`), so
    the tooltip is deliberately **not** the text in the cell — a stale tooltip cannot be mistaken for a correct one that
    merely matches what is on screen. Both sample points are at the **ends** of the list on purpose: identifying a
    mid-scroll row's item out-of-process would need either a cell peer (finding #13) or post-scroll geometry arithmetic
    (finding #20), and the scroll extreme identifies the item without either. The row search rejects rows whose
    rectangles fall outside the table's bounds, so if row rects were to not follow the scroll the test reports a harness
    limitation rather than hovering a meaningless coordinate. The page's `ScrollOffsetTextBlock` is read only as a
    **precondition** that the body moved — never as the subject of an assertion (AGENTS.md Step 1).

## 10. Accessibility scan and out-of-proc UIA

Added after surveying what the repo's own interaction tier actually holds. §12 (API) asserts what each *peer* returns;
nothing so far asserts that a real UIA **client** — a separate process walking the tree through the provider stack — sees
the same thing. Those are different claims, and only the second is what Narrator does.

- [x] `VerifyAxeScanPasses` **(written, FAILING — 24 errors: product finding #22. The prediction that #13 would block it is RETRACTED.)** — An Axe scan of the TableView test page
      reports no issues. **Repo convention:** 12 controls already run `AxeTestHelper.TestForAxeIssues()` from their
      interaction tests (Expander, ItemsView, TabView, TreeView and others), and it exists at no tier but this one.
      `TableViewPage` was already registered as `TableView-Axe` (`TableViewPage.xaml.cs:90`), so only the test method was
      missing. **Failure means** the table trips a general accessibility rule — contrast, missing name, bad role — that
      per-peer assertions do not look for.
  - **Measured result:** the scan **completed without crashing** and reported **24 errors**, all of them on the
    TableView: 12 × `NameNotNull` and 12 × `BoundingRectangleNotNull`. See finding #22.
  - **This item was recorded as "blocked by finding #13" and that was wrong.** The reasoning was that Axe walks the whole
    provider tree, so it would enter a row and take the app down. It enumerated all 12 rows *and their children* and the
    app survived — which is itself a result, because it narrows #13 (see §10.2). The lesson generalises: a blockage
    asserted from reasoning rather than from a run is a hypothesis, and this is the second one in this plan to fall
    (§9's cell items were the first).

- [x] `VerifyTableIsNavigableByAUiaClient` **(written, FAILING — product finding #13: the app fail-fasts with `0xC0000420` when a client asks a row peer for its children)** — From out of process, find the TableView by name, then walk to a column header
      and a cell through the real provider tree. **Failure means** the peers are correct in-proc but the tree does not
      marshal or connect — a class of break that is invisible to every §12 test, because those never cross a process
      boundary.
  - **Measured result:** the walk is sound until it reaches a row. Header host → **5** header peers, first one named
    `Name`. Rows host → **12** row peers. Then `firstRow.Children.Count` → the app fail-fasts with `0xC0000420` and the
    count comes back **0**. The crash is logged *before* the assertion, so the zero is the post-mortem value, not a
    reading of an empty row.
  - **The three levels of the tree fail differently, and that is the useful part.** Columns are fine, rows are fine as
    *elements*, and the break is entirely at the row → cell edge. Whatever is wrong is in the row's child production,
    not in the TableView's peer, the header band, or row realization.
  - **Operational note: run this test and `VerifyAxeScanPasses` last.** The fail-fast takes the app down mid-test, which
    then fails `TestCleanup` (`GoBack` cannot find the Back button), crashes `te.processhost.exe` with `0xE0434352`, and
    fails `AssemblyCleanup`. Anything scheduled after it in the same invocation is running against a restarted app at
    best.
- [x] `VerifyStructureChangedEventsReachAUiaClient` **(new — written, FAILING: product finding #21)**
  - **Description:** Arms a `StructureChangedEventWaiter` on the TableView, then drives three shape changes and records
    which of them reach the client: a sort (click the `Name` header), an `AddColumnButton` invoke, and a
    `RemoveColumnButton` invoke.
  - **Expected result:** all three raise `StructureChanged`.
  - **Measured result:** `sort=True addColumn=False removeColumn=False`. The positive control passed and both subjects
    failed — see finding #21 in §10.3.
  - **Failure means:** assistive technology never learns the grid changed shape — it keeps reading the old column set,
    announcing a column that is gone or staying silent about one that appeared.
  - **Remarks:** §12.6 records that automation *events* are not API-testable — an in-proc peer test can call the raise
    method but cannot show anything arrived — which is why this lands here. The waiter registers at `Scope.Element`,
    never `Subtree`: subtree registration is an invitation for UIA to walk into the rows, which is the descent finding
    #13 fail-fasts on. Element scope is also where the events actually arrive, since they are raised on the TableView's
    own peer. **The sort is a positive control, not a subject**, and this run is what earns the finding: it fired, so
    the listener was registered, the provider marshalled, and the client heard it. Without it, `addColumn=False` would
    have been indistinguishable from a client that never listens.

### §10.1 Product finding #22 — row peers are nameless, and expose a single degenerate child instead of cell peers

`VerifyAxeScanPasses` reported 24 errors. Every one is on the TableView, and they are two rules over the same 12
elements — `BasicTableView` holds exactly 12 items (`TableViewPage.xaml.cs:112-115`), so this is **every row, not an edge
case**:

| Rule | Count | Element | Measured |
| --- | --- | --- | --- |
| `NameNotNull` — "the Name property of a focusable element must not be null" | 12 | `Microsoft.UI.Xaml.Controls.Tabular.TableViewRow`, control type `DataItem` | `Name` is empty |
| `BoundingRectangleNotNull` — "an on-screen element must not have a null BoundingRectangle" | 12 | the row's **sole child** | no control type, no class name, no name, no rectangle |

Read from the scan's own `el.snapshot`, not inferred from the log text.

**Two distinct defects, and the second is the more serious.**

1. **A row has no `Name`.** Its control type is `DataItem` and it is focusable, so a screen reader landing on a row
   announces nothing identifying. The row's own rectangle is fine (`625,513,1006,41`) — it is on screen and correctly
   placed; it is simply anonymous.
2. **A row exposes one empty child, where the contract says one cell peer per visible column.** The child carries no
   control type, no class name, no name and no bounding rectangle. That is not a cell — it is a placeholder that
   satisfies nothing. `VerifyTableIsNavigableByAUiaClient` asserts exactly this shape ("a row peer should expose one cell
   peer per visible column to an out-of-proc client") and the scan now shows what a client actually receives.

These two are almost certainly the same defect seen twice: whatever fails to produce real cell peers is plausibly what
leaves the row with nothing to build a name from. `TableViewRowAutomationPeer::GetChildrenCore` is the place to start,
and finding #13 lives on that same path.

### §10.2 Finding #13 is client-dependent — two clients, two different outcomes, one probable cause

Axe asked all 12 row peers for their children and read their properties. **The app did not fail-fast.** MITA asks the
same question and the app dies. Both measurements are now in hand:

| Client | Asks a row for its children | Result |
| --- | --- | --- |
| Axe (`VerifyAxeScanPasses`) | yes, all 12 rows | survives; each row yields **one** child with no control type, no class name, no name and no rectangle (finding #22) |
| MITA (`VerifyTableIsNavigableByAUiaClient`) | yes, row 0 | **`0xC0000420` fail-fast**; `Children.Count` returns **0**, logged *after* the crash |

So #13 as previously stated — "asking a `TableViewRow` peer for its children crashes the app" — is true of MITA and
false of Axe. The open question from the previous run ("does `Children.Count` return 1 before anything crashes?") is
answered: **no, it returns 0, and the crash comes first.**

**Leading hypothesis, and it unifies #13 with #22.** A row's children appear to be produced incompletely: something
stands in the child position without being a real cell peer. A client that merely *observes* that placeholder gets the
empty element Axe reported. A client that asks the provider to **populate** it — which is what MITA does, via
`Cache.PopulateDefaultCache` → `AutomationElement.GetUpdatedCache` → `IUIAutomationElement::BuildUpdatedCache`, visible
in this run's own stack traces — forces the materialization that fail-fasts. On that reading the degenerate child and
the crash are the same defect observed at two different levels of demand, and repairing
`TableViewRowAutomationPeer::GetChildrenCore` so it returns one real cell peer per visible column closes both.

**Confirming experiment, before anything is restated as fact.** Ask a row for its children through a request that
prefetches *no* properties and see whether the count comes back as 1 without a crash. If it does, the trigger is
property population, not enumeration, and #13 should be re-stated in those terms — which also means every item parked
behind it needs re-checking against the narrower rule, not the old one.

**Consequence for the items parked behind #13.** At least one — `VerifyAxeScanPasses` — was never blocked at all. The
§9 cell items were the other. Everything else deferred on #13 should be re-checked against what is now known rather
than left on the original reading.

### §10.3 Product finding #21 — a column added or removed at runtime notifies no UIA client

Measured by `VerifyStructureChangedEventsReachAUiaClient` on its first run:

| Probe | `StructureChanged` reached the client? |
| --- | --- |
| Sort (click the `Name` header) | **yes** |
| `AddColumnButton` | **no** |
| `RemoveColumnButton` | **no** |

**The sort probe is what makes the other two readable.** It proves the whole chain was live for the same waiter, in the
same run, against the same element: a listener was registered — which matters, because the product checks
`AutomationPeer::ListenerExists` before it raises at all (`TableView.cpp:1024`) — the peer raised, the provider
marshalled across the process boundary, and the client was notified. So `addColumn=False` is the product staying silent,
not the test failing to listen.

**Where it goes missing.** `StructureChanged` is raised from exactly two places: `OnTableViewSourceShapingChanged` for
shaping (`TableView.cpp:1021-1046`, splitting `ChildrenReordered` from `ChildrenInvalidated`) and
`RaiseStructureChangedForGroupExpansion` (`TableViewAutomationPeer.cpp:119`). A column added or removed at runtime goes
through `QueueRebuildHeaders` (`TableView.cpp:1775/:1786`) instead, which rebuilds the header band and every row's cells
and raises nothing. Both gaps are symmetric — add and remove take the same route — which points at that one path rather
than at anything specific to insertion or deletion.

**Why it matters more than the shaping case that is handled.** A re-order changes the order of a row's children; a
column change changes the *set* of them, and the grid's column count with it. A cached client is left describing a
column that no longer exists, or silent about one that appeared — and `GridPattern.ColumnCount` disagreeing with what a
client can enumerate is a stronger break than a stale ordering. The obligation is one the control already accepts
elsewhere; the column path simply does not discharge it.

**Suggested fix:** raise from the column-collection change handler that feeds `QueueRebuildHeaders`, with
`AutomationStructureChangeType::ChildrenInvalidated` — the type `OnTableViewSourceShapingChanged` already uses for a
change that adds or removes children, rather than `ChildrenReordered`. The existing `ListenerExists` guard and the
`FromElement` / `CreatePeerForElement` fallback in that method are the pattern to follow.

**The test stays failing (AGENTS.md Step 5).** The expectation is the contract, and the run is evidence against the
implementation, not against the expectation.

**Deliberately not here: the §13 density and theming tests.** Both were checked against this tier and both belong in the API
plan. Evidence:

- **Density.** `CommonStyles\InteractionTests.RunDensityTests` is a shell — it clicks a button and reads `"Pass"` from a
  `TextBlock`. The real assertions run **in-proc** in the TestUI page's code-behind. It is out-of-proc only because those
  framework controls take density from an app-level `UseCompactResources` / merged-dictionary swap that must happen before
  the page loads. TableView's `Density` is a per-instance dependency property, settable in a plain API test with no app
  restart — so the reason that forced CommonStyles out of process does not apply here. Driving it through a UI readout would
  assert what the page chose to display instead of the property itself.
- **Theming.** WinUI's own theme-parity and theme-swap tests are **API** tests:
  `CommonStyles\APITests.VerifyAllThemesContainSameResourceKeys` and `VerifyVisualTreeExampleLoadAndVerifyForAllThemes`
  swap themes in-proc. §13 is the same shape.
- **Automation peers.** 13 controls assert peer behaviour from `APITests`, including `Repeater\APITests\AccessibilityTests.cs`
  — an entire file named for accessibility that lives in the API tier. What the interaction tier holds instead is the Axe
  scan and cross-process UIA, which is exactly the three items above.
- **High contrast** stays API as resource resolution. Read instead as "renders differently once the OS setting is on", it
  becomes environment-dependent and a *weaker* assertion, not a stronger one.

---

# Appendix: interaction tests in PR `!15971489` (reference only)

The PR's split was 196 API tests to 29 interaction tests — almost exactly the ratio this plan lands on, which is some
evidence the admission rule above is neither too strict nor too loose. The API-side appendix lives in the API plan.

Listed for coverage ideas, not as a port target. Many target APIs and controls that do not exist in this repo.

## `controls\dev\TableView\InteractionTests\TableViewTests.cs` (19)

`TestPageLoadsAndRendersStaticTable`, `AddingColumnAtRuntimeRebuildsHeaderAndCells`,
`SelectFirstButtonUpdatesSelectionReadout`, `SelectAllInMultipleSelectsEveryItem`, `DeselectAllClearsSelection`,
`SwitchingToNoneClearsSelection`, `SetFirstColumnWidth_ActualWidthMatches`,
`SetFirstColumnWidthBelowMin_ActualWidthClampsToMin`, `SetFirstColumnMaxWidthBelowWidth_ActualWidthClampsToMax`,
`ResetColumnWidth_ReadoutReflectsDefaults`, `SortByNameAsc_ColumnReadoutShowsAscendingAndIndexOne`,
`SortByAgeDesc_AfterNameAsc_NameClearsAgeBecomesIndexOne`, `ToggleSortName_CyclesNoneAscendingDescendingNone`,
`ClearSort_AfterSortingByName_ResetsEverything`, `DownArrowMovesSelectionToNextRow`, `UpArrowMovesSelectionToPreviousRow`,
`HomeKeySelectsFirstRow`, `EndKeySelectsLastRow`, `TemplateColumnRendersCustomContent`

**Assessment.** Most of these are not *gesture* tests even under the dual-coverage rule: they click a button on a test page
that calls the API, then read the result out of a `TextBlock` the same page wrote. Selection mode changes, `DeselectAll`,
column width clamping, sort state transitions and `ClearSort` are covered as API tests here (§6, §4, §7). Only the four
arrow/Home/End items drive real input, and §1 covers those — plus the selection-unchanged assertion the PR inverts.

Do not treat this list as a target. The dual-coverage rule asks for the *gesture* where one exists, not for an API call
wrapped in a button: a UI readout asserts what the page chose to display rather than the property itself, and it fails for
reasons that have nothing to do with the behaviour under test.

## `controls\dev\ColumnResizeGripper\InteractionTests\ColumnResizeGripperTests.cs` (2)

`BeginResize_TryResize_EndResize_LifecycleAndClamping`, `TryResizeWithoutBeginResizeIsIgnored`

**Assessment.** N/A as interaction tests here, and arguably misfiled in the PR: both drive the resize lifecycle
programmatically, which is exactly what API §4's `VerifyResizeDragUpdatesColumnWidth` and `VerifyConcurrentResizeDragIsRejected`
already do through the internal `ResizeGripper`. The PR's `ColumnResizeGripper` is a public primitive; this repo's is
internal.

## `controls\dev\SortIndicator\InteractionTests\SortIndicator_InteractionTests.cs` (8)

`SortIndicator_VSM_Ascending`, `SortIndicator_VSM_Descending`, `SortIndicator_VSM_None`,
`SortIndicator_VSM_TransitionAscDesc`, `SortIndicator_ThemeSwitchMidState`, `SortIndicator_KeyboardActivation`,
`SortIndicator_AutomationPeer_Metadata`, `SortIndicator_AutomationPeer_Toggle`

**Assessment.** Six of the eight are API tests by this plan's rule — visual-state names, theme switching mid-state, and
automation peer metadata are all inspectable programmatically, and this plan places them in API §12.5 and §13. Only
`SortIndicator_KeyboardActivation` is genuinely input, and it is covered by §2 above (`HeaderEnterTogglesSort`) since this
repo's `SortIndicator` is internal and has no independent activation surface.

**Already stolen:** `SortIndicator_ThemeSwitchMidState` — switching theme *while* a transition is in flight — is now API
§13's `VerifyThemeChangeDuringSortIndicatorTransitionIsCoherent`, and it passes.
