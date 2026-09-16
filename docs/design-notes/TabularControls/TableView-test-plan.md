# TableView API test plan

> **Interaction tests live in a separate document:**
> **[`TableView-interaction-test-plan.md`](TableView-interaction-test-plan.md)** (29 items).
> This document owns every behaviour reachable programmatically — 207 remaining items plus everything already written.
> Before adding anything to the interaction plan, apply its admission rule: peers, blocked states and primitive drivers make
> most apparently gesture-bound behaviour API-testable.

## Problem and approach

Create TableView tests from current repository state, using PR `!15971489` only as reference material. Do not port files wholesale. Define coverage categories first, then add tests area-by-area against the current public surface in `controls\dev\TableView\TableView.idl`, `controls\dev\TableView\TableViewSource.idl`, and internal primitives used by TableView (`ResizeGripper`, `SortIndicator`).

Current state:

- TableView product code exists under `controls\dev\TableView`.
- TableView API tests are wired into `controls\test\MUXControlsTestApp` via `controls\dev\TableView\APITests\TableView_APITests.projitems`.
- No interaction tests or TestUI pages exist for TableView yet.
- Current TableView API is narrower than PR `!15971489`: only `None`/`Single` selection, `Select`/`Deselect`/`IsSelected`/`DeselectAll`, read-only `SelectedItem`/`SelectedIndex`, cell-scoped `CommitEdit`/`CancelEdit`, no public row edit API, no public multiple selection, no public column reorder/autosize.

## Test files

- `controls\dev\TableView\APITests\TableView_APITests.projitems` — done
- `controls\dev\TableView\APITests\TableViewTests.cs` — done (initialization tests)
- `controls\dev\TableView\APITests\TableView_Columns_APITests.cs` — done (category 3)
- `controls\dev\TableView\APITests\TableView_Sizing_APITests.cs` — done (category 4)
- `controls\dev\TableView\APITests\TableView_DataBinding_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Selection_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Sorting_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableViewSource_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Grouping_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Editing_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_AutomationPeer_APITests.cs` — planned
- `controls\dev\TableView\APITests\TableView_Theming_APITests.cs` — planned
- `controls\dev\TableView\InteractionTests\TableViewTests.cs` — later; tracked in the interaction plan
- `controls\dev\TableView\TestUI\TableViewPage.xaml` + `.xaml.cs` — later; tracked in the interaction plan

## Test authoring protocol

The protocol is **[`controls\dev\TableView\AGENTS.md`](../../../controls/dev/TableView/AGENTS.md)**.
It is the single source of truth and is loaded automatically when working in the TableView tree.

Six mandatory steps, in order, before any test code is written:

1. **Classify** each item as an API test or an interaction test.
2. **Check for redundancy** against existing TableView tests, sibling items, platform guarantees, and the assertion itself. Record every drop as `(dropped)` with a reason.
3. **Derive expectations from the IDL and design spec**, never from the implementation. Spec silent means `(needs spec decision)`, not an invented assertion.
4. **Write the expectation down first** in the four-field form below.
5. **Never adjust a test to match the implementation.** A failure means the product is wrong by default.
6. **Build, verify the names are in the shipped DLL, run the tests.** The agent *can* run TAEF — via a scheduled task with an interactive logon; see AGENTS.md Step 6.

Item format used throughout this document:

```
- [x] `TestName`
  - **Description:** what the test does.
  - **Expected result:** concrete, observable outcome - specific values, not "works correctly".
  - **Failure means:** what a failure tells us is broken, in product terms.
  - **Remarks:** provenance of the expectation and anything that weakens it. Omit only when the
    expectation is stated outright in the IDL.
```

**Remarks** carries the part an IDL cannot express. An IDL declares types, default values, and
content properties; it cannot state layout behavior, event ordering, recycling contracts, precedence
between two properties, or the result of a mutation. Expectations about those are still legitimate,
but they rest on something weaker than a `MUX_DEFAULT_VALUE`, and the item must say which: a spec
line, a stated invariant, a deliberate deviation from platform convention, spec silence, or
acknowledged debt. See AGENTS.md Step 4 for the full guidance.

### Known debt against this protocol

Category 1 and 3.1 were implemented before steps 2-4 existed. They have since been rewritten into the four-field form, but two items still pin **observed behavior** rather than a stated contract, and are flagged in their **Remarks**:

- **1.7 empty state** - derived from the three branches of `TableView::UpdateEmptyState`. There is no empty-state spec anywhere in the repo.
- **3.1 `VerifySameColumnInstanceAddedTwiceBehavesPerContract`** - the duplicate-instance contract was resolved by reading `TableViewColumn::SetOwningTableViewInternal`.

Neither is precedent for skipping steps 2-5.

---
## Current status

Last full run: `x64chk`, filter `*TableView*` — **Total 264, Passed 251, Failed 13, Blocked 0**. Categories 2, 7, 9, 15 and 16 were added after that run and were verified in isolation (`*TableViewDataBindingTests*` — **26 / 26 / 0**; `*TableViewSortingTests*` — **35 / 35 / 0**; `*TableViewGroupingTests*` — **20 / 18 / 2**; `*TableViewVirtualizationTests*` — **3 / 3 / 0**; `*TableViewNegativePathTests*` — **11 / 10 / 1**), so the next full run is expected at **359 / 343 / 16**.
All sixteen failures are product bugs, listed below; the `ValidateLoadUnload` intermittent passed in
this run. The four §13 tests that mutate
`Application.Current.Resources` still disturb no other class: the full-suite result is exactly the
sum of the isolated per-class runs (§12 40/43, §13 12/16, §14 19/20, §8 47/48).

| Category | State |
|---|---|
| 1. Initialization, defaults, XAML activation | 30 written, 30 passing (`ValidateLoadUnload` is an intermittent — see below) |
| 2. Data binding and data source projection | 19 written, 19 passing — **complete** (26 TAEF cases; 3 tests are data-driven) |
| 7. Sorting | 30 written, 30 passing — **complete** (35 TAEF cases; 2 tests are data-driven; no product findings) |
| 9. Grouping and group headers | 20 written, 18 passing, 2 failing on one product finding — **complete** |
| 16. Error handling, edge cases, reentrancy | 6 written, 5 passing, 1 failing on one product crash — **complete** (backlog narrowed from 17; see §16.0) |
| 15. Virtualization and performance safety | 3 written, 3 passing — **complete** (backlog narrowed from 8; four items were platform or duplicate coverage — see §15.0) |
| 3. Columns and headers | 31 written, 29 passing, 2 failing |
| 4. Column sizing, layout, resize, frozen columns | 31 written, 29 passing, 2 failing — **complete** |
| 5. Rows, cells, and visual states | 12 written, 12 passing — **complete** |
| 6. Selection | 16 written, 16 passing — **complete** |
| 8. TableViewSource data shaping | 48 written, 47 passing, 1 failing — **complete** |
| 10. Editing | 17 written, 17 passing — **complete** (8 further items are gesture-bound and live in §11) |
| 12. Accessibility and automation | 43 written, 40 passing, 3 failing — **complete** |
| 13. Theming, resources, density | 16 written, 12 passing, 4 failing — **complete** |
| 14. Tooltips | 20 written, 19 passing, 1 failing — **complete** |
| 11 | not started (interaction only) |

> **Known intermittent: `TableViewTests.ValidateLoadUnload`.** Fails in roughly one full-suite run in
> three and passes every time it is run in isolation (verified again after Category 5 landed: failed
> in the suite, passed alone on the next run). It is order- or timing-sensitive, not a regression from
> any category added so far. **Do not treat it as a Category 5 failure**, and do not "fix" it by
> loosening the assertion — it needs its own investigation into what leaks between tests.

> **Known intermittent: a `0xC0000420` assert in `Microsoft.UI.Xaml.dll`.** Seen once during the first
> full run after Category 10 landed, in *Setup* of the test following a passing one, which TAEF then
> reports as `Blocked` plus "stopped communicating with the test host process". The immediately
> following full run was clean, and the isolated Category 10 run passed 17/17 twice. This is the same
> unroot-caused assert already tracked in the appendix as `VerifyDisposalDoesNotAssertInTrackerTeardown`
> — it is a teardown-ordering assert in the framework, not a Category 10 failure. **A `Blocked` result
> is a crashed host, not a failed assertion: re-run before reading anything into it.**

### Remaining work, classified

44 items remain, **all of them interaction tests** — every category that can be driven
programmatically now has tests. One further item, the `0xC0000420` teardown pin, is parked in §16.0 until the crash has a repro.

The 44 interaction items now live in
[`TableView-interaction-test-plan.md`](TableView-interaction-test-plan.md); the rows below are the
classification as of the split, kept so the ratio stays visible from this document. The rows mirror that document's own
sections, because the earlier per-category rows had drifted out of step with it.

| Interaction plan section | Open | API | Interaction |
|---|---:|---:|---:|
| 0. Page load (markup-compiled) | 1 | 0 | 1 |
| 1. Keyboard navigation | 8 | 0 | 8 |
| 2. Header input | 3 | 0 | 3 |
| 3. Pointer selection and focus (includes the focus re-anchor item moved out of §8.7) | 6 | 0 | 6 |
| 4. Group header input (includes `ToggleRequested` moved out of §9.3) | 3 | 0 | 3 |
| 5. Editing gestures | 10 | 0 | 10 |
| 6. Pointer resize (includes the unload-mid-drag item moved out of §16.4) | 3 | 0 | 3 |
| 7. Scrolling | 3 | 0 | 3 |
| 8. Right-to-left | 2 | 0 | 2 |
| 9. Tooltip hover | 2 | 0 | 2 |
| 10. Accessibility scan and out-of-proc UIA | 3 | 0 | 3 |
| **Total** | **44** | **0** | **44** |

Categories 8, 12, 13 and 14 were re-planned in detail after this table was first written: §12 went from 28
backlog lines to 43 specified tests (mostly by splitting merged assertions and adding the `ResizeGripper`
peer, which nothing covered), §13 from 19 to 16 (by folding three per-theme "brushes resolve" tests
into one parity test and dropping two unreachable paths), §14 from 13 to 20, and §8 from 39 to 48
(three items dropped with reasons, the rest split). The other rows are still one-line backlog
items awaiting the same treatment, so expect similar movement.

Four items look gesture-bound and are not. Each has a programmatic route that reaches the same code:

- **§7.5 `VerifyHeaderInvokeTogglesSortAndUpdatesIndicator`** — `TableViewColumnHeaderAutomationPeer`
  exposes `IInvokeProvider`, so `Invoke()` drives the header's sort toggle without a gesture. §11's
  `HeaderEnterTogglesSort` separately owns the keyboard path; they are not duplicates.
- **§9.3 `VerifyGroupHeaderToggleRequestedFires`** — reachable through the group header peer's
  `IExpandCollapseProvider`.
- **§15 scroll-dependent items** — programmatic `ChangeView` is correct here, because virtualization is
  a function of *offset*, not of input. This is deliberately different from §11's scroll items, which
  stayed interaction tests because what they verify is survival of wheel/drag/inertia offsets.
- **§16 resize items** — `ResizeGripper` exposes `BeginDrag` / `TryDrag` / `EndDrag`, already used by the
  Category 4 resize tests.

The single interaction test outside §11 is **§9.5 `VerifyGroupHeaderPointerAndPressedVisualStates`**:
pointer-over is set only from the header's own pointer handlers, so there is no programmatic route —
the same finding that moved two Category 5.5 items to §11.

**Three further interaction-only items were added after surveying the repo's own interaction tier**
(`§9` of the interaction plan): an Axe scan, an out-of-proc UIA tree walk, and a client-side
structure-changed event waiter. None duplicates §12 — §12 asserts what each peer *returns* in-proc,
these assert that a separate process walking the real provider stack sees the same thing, which is
what Narrator actually does. The Axe scan exists at no other tier: 12 controls run
`AxeTestHelper.TestForAxeIssues()` and every one of them does it from `InteractionTests`.

**Two categories whose classification depends on how the item is worded. Settle this when writing the
subsection, not while implementing:**

- **§14 (tooltips)** is API *only* if each item asserts on the tooltip **object** — `ToolTipService.GetToolTip`
  returns (or does not return) a `ToolTip` whose `Content` is X, plus the matching UIA `HelpText`.
  Reworded as "a tooltip appears on hover" every item becomes an interaction test. Nothing in the
  current list requires hover; keep it that way.
- **§13 high-contrast items** are API as **resource resolution and visual-state key inspection**. Read
  instead as "renders differently once the OS High Contrast setting is on", they stop being interaction
  tests and become environment-dependent ones — which is both harder to run and a weaker assertion.

**Challenged and upheld: §13 density and §12 automation stay API tests.** The question is fair — the
repo *does* have density tests in `CommonStyles\InteractionTests` — but the precedent does not
transfer:

- `CommonStyles\InteractionTests.RunDensityTests` is a shell: it clicks a button and reads `"Pass"`
  from a `TextBlock`. The real assertions run **in-proc** in the TestUI page's code-behind. It is
  out-of-process only because those framework controls take density from an app-level
  `UseCompactResources` / merged-dictionary swap that must happen before the page loads. TableView's
  `Density` is a per-instance dependency property — no app restart, no page needed. Driving it through
  a UI readout would assert what the page chose to display rather than the property.
- WinUI's own theme tests are API tests: `CommonStyles\APITests.VerifyAllThemesContainSameResourceKeys`
  and `VerifyVisualTreeExampleLoadAndVerifyForAllThemes` swap themes in-proc. §13 is the same shape.
- 13 controls assert automation-peer behaviour from `APITests`, including
  `Repeater\APITests\AccessibilityTests.cs` — a file named for accessibility that lives in the API
  tier. What the interaction tier uniquely holds is the Axe scan and cross-process UIA, which is
  exactly the three items added above.

Open failures, all triaged in place under their items. **Every one is a product bug, not test debt.**

1. `VerifyCollapsedColumnRemovesCellsFromLayout` — collapsed column keeps its full 120px track.
2. `VerifyRestoringColumnVisibilityRestoresCells` — same root cause, collapse half only.
3. `VerifyNonFiniteMinMaxWidthDoesNotCorruptActualWidth` — `MinWidth = +infinity` puts **infinity into `ActualWidth`**, which the header host and every row panel then arrange to. The other three pathological cases are handled. See §4.2.
4. `VerifyFrozenColumnsMirrorInRightToLeft` — frozen columns are **not pinned at all under RTL**; `ApplyFrozenColumnLayout` returns early. Measured: the frozen cell moves by exactly the same displacement as its non-frozen neighbour. See §4.6.
5. `VerifyColumnHeaderHelpTextReportsSortStateOnlyForSortableColumns` **and `VerifyHeaderPeerHelpTextCombinesToolTipAndSortState`** — **no TableView localized string resolves at all.** `ResourceAccessor.cpp:8-12` hardcodes the `Microsoft.UI.Xaml/Resources` subtree, but the Tabular binary's `AppxPriInitialPath` is `Microsoft.UI.Xaml.Controls.Tabular` (`Microsoft.UI.Xaml.Controls.Tabular.vcxproj:23`), so every `TryGetLocalizedString` there returns empty. Widest-blast-radius finding in this set. The §14 test measures the same defect from the other side: with a tooltip set, header help text is `'The person's display name'` at **all three** sort directions — the sort strings and the join format both resolve empty, so the peer returns the tooltip alone. One fix clears both. See §12.4 and §14.3.
6. `VerifyTemplateColumnCellNameUsesContentAutomationName` — template-column cells are **silent to AT**. `GetCellValueText` peers `border.Child()`, which is the `ContentPresenter` returned by `TableViewTemplateColumn::GenerateElementCore`, not the template root. See §12.3.
7. `VerifyColumnHeaderPeerNameFallsBackToRealizedHeaderForTemplateHeaders` — template **headers** are likewise unnamed. The header cell is a bare `Grid` and `AutomationProperties::SetName` is applied only for string headers (`TableView.cpp:1564-1568`); a `Grid` peer does not descend for a name. Same fix shape as (6). See §12.4.
8. `VerifyGridLineBrushOverrideChangesRenderedSeparators` and `VerifyDensityResourceOverrideWins` — **app-scope resource overrides lose to merged theme dictionaries.** `LookupElementResource` (`TableView.cpp:103-140`) searches `LookupInThemeDictionaries(appResources, …)` before `appResources.TryLookup(key)`, inverting normal XAML precedence. `TableView-dev-spec.md:227-231` makes overriding a `TabularSurface*` key *the* supported customisation path, so the documented story does not work. See §13.2 and §13.3.
9. `VerifyResizeGripperSeparatorResourcesApply` and `VerifySortIndicatorForegroundResourceApplies` — the primitives' advertised styling keys live in their **default-style pages** (`ResizeGripper.xaml`, `SortIndicator.xaml`) rather than a `*_themeresources.xaml`, so a `{ThemeResource}` in the style resolves locally and an app-scope override can never win. `TABULAR_BINARY_EMITS_THEME_RESOURCES` *is* defined, so the stated placement rationale may be stale. See §13.2.
10. `VerifySortKeyPropertyChangeDoesNotMoveTheRowUntilACollectionChange` — **the sorted flat path never re-shapes a row whose sort key changed, not even on the next collection change.** `ShapedItemsSource.cpp:468-471` states the invariant as *"an in-place mutation of a row's sort-key field that raises only INotifyPropertyChanged leaves the row at its old sort position **until the next collection change**"*. The first half holds; the second does not. Measured, sorted ascending by `Name`: renaming `Asha` → `Zara` correctly leaves her at row 0, but the subsequent `Add("Bea")` inserts only `Bea` at its correct position and leaves `Zara` stranded at index 1 — the projection is `[Bea, Zara, Diego, Ines, Mei, Owen, Rafa]` instead of `[Bea, Diego, Ines, Mei, Owen, Rafa, Zara]`. A collection change on this path is an **incremental insert**, not a re-shape, so a stale row is stale forever. The grouped path does not share the defect: `VerifyGroupKeyPropertyChangeFollowsTheSameInvariant` shows `RebuildGrouped` re-bucketing the stale row into correct source order on the next collection change, which is the behaviour the invariant describes. **The two shaping paths disagree, and the sorted one contradicts its own written contract.** The IDL is silent on all of this — see the §8 spec-change requests. See §8.6.

11. `VerifyGroupHeaderTemplateChangeAfterLoadUpdatesLive` and `VerifyClearingGroupHeaderTemplateRevertsToTheStyleDefault` — **a `GroupHeaderTemplate` change never reaches already-realized group headers.** `TableView::PrepareGroupHeaderElement` applies the app template, and `ClearGroupHeaderElement` clears it, only while a container is being prepared or recycled (`TableView_Grouping.cpp:688-697`, `:748-766`); no property-changed handler re-applies it to headers that are already on screen. Setting the property after load leaves every realized band on the old template, and clearing it back to `null` leaves them on the app template — directly contradicting `TableView.idl:381-383`, which describes the override as "a local value that `ClearValue` reverts back to the Style setter". Headers do pick the new template up when they are next recycled, so the symptom is a mixed screen after a theme or view switch. See §9.2.

12. `VerifyNonFiniteWidthConstraintsDoNotCorruptActualWidth#metadataSet4` — **an infinite `MinWidth` crashes the process during layout.** Setting `MinWidth = double.PositiveInfinity` on a column and running one layout pass takes down the test host with `0xC000027B` before any assertion runs. The column's `MinWidth` is read and used as the resolved floor with no finite guard (`TableView_Layout.cpp:37-47` returns it straight through), so an infinite floor reaches `Measure` and fail-fasts. `MaxWidth` already defaults to infinity per `TableView.idl`, so non-finite constraint values are in-contract inputs, and `ActualWidth` is documented as "the resolved, MinWidth/MaxWidth-clamped pixels" — a value that can never legitimately be infinite. The finite counterparts (`NaN` on either property, `-∞` on `MinWidth`, `+∞` on `MaxWidth`) all resolve correctly, so the missing guard is specifically on the `MinWidth` floor. See §16.1.

13. `VerifyTableIsNavigableByAUiaClient` (interaction tier) — **an out-of-process UIA client crashes the app the moment it asks a row for its children.** The client walk reaches `TableViewRowAutomationPeer::GetChildrenCore`, which manufactures a fresh `TableViewCellAutomationPeer` per visible column with `winrt::make<TableViewCellAutomationPeer>(cellFE, ...)` on every call (`TableViewRowAutomationPeer.cpp:130-175`). XAML fail-fasts with `0xC0000420` in `Microsoft.UI.Xaml.dll` inside `MUXControlsTestApp.exe`. Measured boundary: the TableView peer, the header host and its four header peers, and all twelve `TableViewRow` peers enumerate correctly; the very next level down kills the process, and it does so for every column kind — plain text, read-only, and template alike, so it is not tooltip- or template-specific. **This is the widest interaction-tier blocker found so far:** any test that names, clicks, or reads a cell from out of process takes the app down with it, which is most of the interaction backlog. The in-proc §12 peer tests do not see it because they never drive the provider stack across a process boundary — exactly the gap §10 was added to cover. Note the stop code is `STATUS_ASSERTION_FAILURE`, which only fires in a chk build, so a fre build may well survive the duplicate peers and merely leak them plus desynchronise runtime IDs — quieter, not correct.

> **Retracted: `VerifyRowSelectedVisualStateAndDisabledPrecedence` was NOT a product bug.** It was
> listed here as "a disabled TableView renders its rows as enabled". That was wrong — the test was
> under-settled, and an ancestor-driven `IsEnabled` change simply lands one layout pass later than the
> test looked. It passes now. The full retraction and the two lessons are in §5.5; the short version
> is that **the test failed alone and passed in the suite, and that order-dependence was the tell.**
>
> **Three of the seven failures recorded at the end of Category 5 turned out to be test bugs** (this
> one plus the two cell-factory tests). Treat a new failure as a test bug until an order-independent,
> fully-settled run says otherwise.

> **Resolved: the two former cell-factory failures were test bugs, not product bugs.**
> `VerifyTemplateColumnGenerateElementUsesCellTemplate` and `VerifyCustomColumnElementUpdatesOnRecycle`
> both failed with `0xC000027B` and were originally triaged as product bugs. Re-triaged against the
> note below: both were mine. The first asserted that `GenerateElement` on an **unattached** column
> returns inflated content — an expectation with no spec basis; it now asserts the real contract (a
> `ContentPresenter` carrying the column's `CellTemplate`, and *not* a `TextBlock` fallback), with
> inflation left to §5.2. The second needed the double-settle after `ChangeView` (see §5.3) and was
> raising an NRE out of a bad null-check. Both pass.
>
> **Generalized lesson — `Verify.IsNotNull` logs and continues, it does not throw.** Dereferencing the
> value on the next line raises a `NullReferenceException`, which surfaces as `0xC000027B` and *hides
> the assertion that actually failed*. Never write `Verify.IsNotNull(x, ...)` followed by a use of `x`.
> Use `if (x == null) { Verify.Fail(...); continue; }` (or `return`) instead. To diagnose: read the
> last `Verify:` line before the crash, then look at the statement immediately after it.

> **Note on `0xC000027B` when writing new tests.** That crash code is `STATUS_STOWED_EXCEPTION`: a
> managed exception crossing the WinRT boundary, which TAEF reports as a test-host crash with no
> managed stack. It is *not* automatically a product bug. While writing §4.5 it was produced by a
> plain `InvalidCastException` in the test itself — casting a header cell `Grid` to `Control` to
> read `IsTabStop`, which in WinUI lives on `UIElement`, not `Control`. If a new test produces this
> code, suspect the test first: check the last `Verify:` line in the log and look at the statement
> immediately after it.

> **Note on `0x8001010E` (`RPC_E_WRONG_THREAD`).** Creating a `DependencyObject` — a `Brush`, a
> `Style`, a `DataTemplate` — at the top of a test method puts it on the TAEF thread, and the first
> UI-thread use of it fails. Declare the field `null` outside and construct it inside the first
> `RunOnUIThread.Execute`. Cost one round trip in §5.3 and §5.4.

**Running the tests:** the agent *can* run TAEF. See AGENTS.md Step 6; direct `TE.exe` invocation
fails with `0x8007000E` and must be relaunched through a scheduled task with an interactive logon.

### Spec corrections owed, from decisions already taken

Both Category 4 blockers have been decided and their tests are now written. Each decision leaves the
IDL saying something that is no longer true, and those edits are outstanding:

1. **Auto is shrink-capable.** `TableView.idl:122` still says Auto "grows within a data set", which
   reads as monotonic. That wording, the `ResetColumnDesiredWidths` comments, and the dead
   `TableViewColumn::m_desiredWidth` accumulator are all leftovers of the abandoned grow-only policy
   and should be cleaned up together. See §4.3.
2. **`FrozenEdge.Leading` is a logical edge and mirrors under RTL.** The IDL says only "Leading pins
   left", which is wrong under RTL rather than merely incomplete. The product does not implement the
   mirroring at all today; failure 6 above tracks it. See §4.6.

Category 5 adds three more, none of them blocking:

3. **`RowBackground` / `AlternatingRowBackground` precedence.** The IDL says only "Opt-in row
   banding; null brushes preserve the theme row background" and never states that `RowBackground` is
   the base for *every* row while `AlternatingRowBackground` overrides odd rows only when set. See §5.4.
4. **`CommonStates` precedence.** The IDL lists all eight states but not which wins when a row is
   both selected and disabled. See §5.5.
5. **The template cell's `Content` binding is inert** — dead code guarded by a comment that
   misdescribes the live mechanism. See the finding in §5.2.

---
## Legend

- `[x]` written and passing
- `[!]` written but failing or blocked
- `[ ]` not written yet
- `(deferred)` blocked on API that does not exist in this repo today; tracked, not written
- `(needs spec decision)` behavior is undefined in the IDL and the design spec; must not be tested until the expectation is decided

---

## 1. Initialization, defaults, and XAML activation

<details>
<summary>Show 30 items &mdash; 30 written, 30 passing</summary>

Status: in progress. File: `TableViewTests.cs`.

### 1.1 Construction smoke

- [x] `VerifyTableViewConstructs`
  - **Description:** Construct `TableView` with no parent, template, or items source.
  - **Expected result:** No throw. Instance is a `Control` (`TableView.idl:433` declares `unsealed runtimeclass TableView : Control`). `Columns` is non-null and `Count == 0` — the IDL exposes `Columns` as a get-only collection, so the constructor must create it.
  - **Failure means:** The type cannot be created outside a XAML tree, or `Columns` is lazily created on first access. Either breaks code-behind construction and every `tableView.Columns.Add(...)` before load.

- [x] `VerifyTextColumnConstructs`
  - **Description:** Construct `TableViewTextColumn` standalone.
  - **Expected result:** No throw. Derives from `TableViewColumn`. `Binding` is null.
  - **Failure means:** The column subclass is not independently constructible, or it self-assigns a binding, which would make an unconfigured column silently render data.

- [x] `VerifyTemplateColumnConstructs`
  - **Description:** Construct `TableViewTemplateColumn` standalone.
  - **Expected result:** No throw. Derives from `TableViewColumn`. `CellTemplate` is null.
  - **Failure means:** The column subclass is not independently constructible, or it fabricates a default template, hiding the "no template supplied" case the author needs to see.

- [x] `VerifyColumnConstructs`
  - **Description:** Construct the base `TableViewColumn` directly.
  - **Expected result:** No throw. Derives from `DependencyObject`, **not** `FrameworkElement` — `TableView.idl:98` declares `unsealed runtimeclass TableViewColumn : Microsoft.UI.Xaml.DependencyObject`.
  - **Failure means:** The base column became abstract or changed base type. `DependencyObject` is load-bearing: columns are data descriptors, not elements, and must never enter the visual tree or carry layout/`DataContext` state.

- [x] `VerifyRowConstructs`
  - **Description:** Construct `TableViewRow` with no owning TableView.
  - **Expected result:** No throw. Derives from `Control`. `IsSelected` is `false` — `TableView.idl:425` declares `[MUX_DEFAULT_VALUE("false")] Boolean IsSelected { get; }`.
  - **Failure means:** The row requires an owner to construct, which would block re-templating and any standalone row style, or an ownerless row reports itself selected.

- [x] `VerifyCellsPanelConstructs`
  - **Description:** Construct `TableViewCellsPanel` standalone.
  - **Expected result:** No throw. Derives from `Panel`. `Children.Count == 0`.
  - **Failure means:** The panel cannot be instantiated from markup. Since it is the declared host for both `PART_HeaderHost` and each row's cells host, a re-templating app could not author it.

### 1.2 Default property values

All expected values below come from `[MUX_DEFAULT_VALUE(...)]` in `TableView.idl`. Where the IDL declares no default, the expected value is the CLR/WinRT zero value for the type (null for references, false for `Boolean`).

- [x] `VerifyDefaultPropertyValues`
  - **Description:** Read every public TableView property on a freshly constructed, never-loaded instance.
  - **Expected result:** `HeadersVisibility == Column` (`:458`), `GridLinesVisibility == All` (`:463`), `CanUserResizeColumns == true` (`:468`), `CanUserSortColumns == true` (`:577`), `Density == Standard` (`:490`), `IsReadOnly == true` (`:495`), `SelectionMode == Single` (`:538`), `SelectedIndex == -1` (`:545`). `ItemsSource`, `SelectedItem`, `EmptyTemplate`, `GroupHeaderTemplate`, `RowBackground`, `AlternatingRowBackground` are null; `IsEditing` is false; `Columns` is empty.
  - **Failure means:** A shipped default changed. Each one is an API break for existing markup: `IsReadOnly == true` in particular is the editing opt-in gate, and flipping it would make every app's table silently editable.

- [x] `VerifyColumnDefaultPropertyValues`
  - **Description:** Read every public `TableViewColumn` property on a fresh instance with no owner.
  - **Expected result:** `Width == GridLength(120.0, Pixel)` (`:125`, `c_widthDefault`), `MinWidth == 20.0` (`:129`), `MaxWidth == double.PositiveInfinity` (`:133`), `ActualWidth == 120.0` (`:144`), `CanResize == true` (`:139`), `CanSort == true` (`:159`), `SortCycle == AscendingDescending` (`:166`), `SortDirection == None` (`:186`), `IsReadOnly == false` (`:205`), `FrozenEdge == None` (`:149`), `Visibility == Visible` (`:153`), `SortMemberPath == string.Empty`. `Header`, `HeaderTemplate`, `HeaderTemplateSelector`, `HeaderToolTip`, `CustomSortComparer`, `CellEditingTemplate`, `CellToolTipBinding` are null.
  - **Failure means:** A column default changed. `ActualWidth == 120.0` before any layout is the notable one — it must equal `Width`'s default so a never-measured column still reports a usable width to the header host.
  - **Note:** `IsReadOnly` defaults **false** on the column but **true** on the control. That asymmetry is intentional (the control is the master gate) and the test pins both sides so neither drifts toward the other.

- [x] `VerifyTextColumnDefaultPropertyValues`
  - **Description:** Read `TableViewTextColumn`-specific properties plus a sample of inherited base defaults.
  - **Expected result:** `Binding` and `CellToolTipBinding` are null; inherited defaults match `VerifyColumnDefaultPropertyValues`.
  - **Failure means:** The subclass overrode a base default in its constructor, so a text column and a base column disagree on width, sortability, or visibility for no declared reason.

- [x] `VerifyTemplateColumnDefaultPropertyValues`
  - **Description:** Read `TableViewTemplateColumn`-specific properties plus inherited base defaults.
  - **Expected result:** `CellTemplate` and `CellEditingTemplate` are null; inherited defaults match the base.
  - **Failure means:** Same as above, and additionally that a template column might render something before the app supplies a template.

- [x] `VerifyRowDefaultPropertyValues`
  - **Description:** Read `IsSelected` on a standalone `TableViewRow` through both the CLR property and `GetValue(IsSelectedProperty)`.
  - **Expected result:** Both return `false` (`:425`).
  - **Failure means:** The CLR getter and the DP disagree, so a style trigger or binding on `IsSelectedProperty` sees a different selection state than app code does.

### 1.3 Dependency property identity

- [x] `VerifyDependencyProperties`
  - **Description:** Read every `static DependencyProperty ...Property` declared on `TableView` in the IDL.
  - **Expected result:** Every identifier is non-null.
  - **Failure means:** A DP static was removed or failed to initialize. Any XAML `Setter`, `Binding`, or `VisualState` targeting it would fail at parse time in every consuming app.

- [x] `VerifyDependencyPropertyBacking`
  - **Description:** For each settable TableView DP, round-trip **both** directions: `SetValue(dp, v)` then read the CLR getter, and set the CLR property then read `GetValue(dp)`. For read-only `SelectedItem`/`SelectedIndex`, check the getter agrees with `GetValue`. Reach `Columns` through `ColumnsProperty`.
  - **Expected result:** Both directions return the assigned value for every settable DP; read-only DPs agree with their getters; `GetValue(ColumnsProperty)` is reference-equal to `Columns`.
  - **Failure means:** A CLR property is not actually backed by its DP — a hand-written getter/setter that reads a field instead. Styles and bindings would then diverge from code-behind, which is invisible in single-direction tests and is exactly why both directions are checked.

- [x] `VerifyColumnDependencyProperties`
  - **Description:** Read every DP static on `TableViewColumn`, `TableViewTemplateColumn`, `TableViewRow`, and `TableViewGroupHeader`.
  - **Expected result:** Every identifier is non-null.
  - **Failure means:** Same as `VerifyDependencyProperties`, for the column and row surface.

- [x] `VerifyColumnDependencyPropertyBacking`
  - **Description:** Two-way round-trip for every settable column, group header, and row DP; getter agreement for read-only `ActualWidth`, `SortDirection`, and `TableViewRow.IsSelected`.
  - **Expected result:** All settable DPs round-trip both ways; read-only DPs agree with their CLR getters.
  - **Failure means:** As above. `ActualWidth` and `SortDirection` matter most — both are written internally via the read-only DP key and read by templates, so a disagreement makes the header show a different sort glyph than the control's actual sort state.
  - **Note:** `Binding` (on `TableViewTextColumn`) and `CellToolTipBinding` are **CLR properties, not DPs**, so they are excluded from backing assertions. That is deliberate in the API: a DP would evaluate the `Binding` instead of storing it.

### 1.4 XAML activation

- [x] `VerifyXamlActivationWithInlineColumns`
  - **Description:** `XamlReader.Load` a `<TableView>` whose children are two `TableViewTextColumn` elements, the second written as `<TableViewTextColumn>Role</TableViewTextColumn>`.
  - **Expected result:** Parse succeeds with no recursion or crash. `Columns.Count == 2` with no `<TableView.Columns>` property-element wrapper — `TableView.idl:433` declares `[contentproperty("Columns")]`. The second column's `Header` is `"Role"` — `TableView.idl:98` declares `[contentproperty("Header")]` on `TableViewColumn`.
  - **Failure means:** One of the two content-property declarations was lost. Every documentation sample and every app's existing markup that omits the wrapper stops compiling.

- [x] `VerifyXamlActivationWithoutColumns`
  - **Description:** `XamlReader.Load` a bare `<TableView />`, then load it into the tree.
  - **Expected result:** Parse succeeds, `Columns.Count == 0`, and the template still applies (named parts resolve).
  - **Failure means:** The control requires at least one column to activate, so an app that populates `Columns` from a view model at runtime cannot author the control in markup.

- [x] `VerifyXamlActivationWithTemplateColumn`
  - **Description:** `XamlReader.Load` a `TableViewTemplateColumn` with an inline `<CellTemplate><DataTemplate>...` and load it.
  - **Expected result:** Parse succeeds and `CellTemplate` is non-null after load.
  - **Failure means:** The template is dropped between parse and load. Template columns are the primary extensibility point, so this is the difference between the feature working and silently rendering blank cells.

These use `XamlReader.Load`, which exercises the **runtime** XAML parser only. The markup-compile path (XamlTypeInfo generation, `x:Bind`, compile-time content-property resolution) is not covered by any API test and needs a compiled TestUI `.xaml` page. Tracked as a gap; fold into the TestUI work for category 11.

### 1.5 Template application

- [x] `VerifyTemplatePartsAfterTemplateApplication`
  - **Description:** Load a TableView with the default style and resolve each named part the IDL documents.
  - **Expected result:** `PART_HeaderRow`, `PART_HeaderHost`, `PART_BodyScroller`, `PART_BodyContent`, `PART_RowsRepeater`, and `PART_EmptyStatePresenter` all resolve to non-null elements.
  - **Failure means:** Either the default style did not apply at all, or a documented part was renamed/removed. `TableView.idl:435` states that a re-template must supply these parts, so this is a published contract — renaming one silently breaks every app that re-templates.
  - **Note:** This subsumes a separate "default style resolves" test: the parts cannot resolve unless the style from `TabularControlsResources` applied.
- `(dropped)` `VerifyDefaultStyleResolves` — redundant with the above. Genuine style coverage for `TableViewRow`, `TableViewGroupHeader`, `SortIndicator`, and `ResizeGripper` lives in 13.1, where nothing covers it today.
- `(dropped)` `VerifyApplyTemplateIsIdempotent` — near-vacuous as specified: `FrameworkElement.ApplyTemplate()` returns false and no-ops once a template is applied, so calling it twice asserts nothing. The path that can actually re-enter `OnApplyTemplate` is a `Template`/`Style` swap; the repo covers that only through interaction tests with dedicated TestUI pages (`ProgressBarReTemplatePage` + `ProgressBarTests.ReTemplateChangeStateTest`). Revisit as an interaction test if re-templating becomes a supported scenario worth guarding.

Repo precedent reviewed for this area: no control has a "default style resolves" test. The closest equivalent is snapshot verification via `VisualTreeTestHelper.VerifyVisualTree(root:, verificationFileNamePrefix:)` against a master in `controls\test\MUXControlsTestApp\verification\` (46 masters; used by ColorPicker, AutoSuggestBox, ComboBox, TreeView, PersonPicture, NavigationView, CalendarView). Deferred for TableView: the template is still changing, and every edit would force regenerating the master. Revisit once the template stabilizes by adding `verification\TableView.xml`.

### 1.6 Load, unload, reload

- [x] `ValidateLoadUnload`
  - **Description:** Load a TableView with columns and a selected row, unload it, reload it, and reparent it to a different host.
  - **Expected result:** Template parts resolve again after each reload; `Columns` is unchanged; the selection made before unload is still reported after reload; `Loaded` and `Unloaded` strictly alternate and never nest.
  - **Failure means:** The control does not survive being moved in the tree — a normal occurrence in `NavigationView`, tabs, and virtualized hosts. Selection loss specifically means a user's row selection disappears on tab switch.
  - **Remarks:** Named after `Repeater\APITests\ViewportTests.ValidateLoadUnload`, which it is modeled on. **Order-dependent:** passes in isolation but fails in a full `*TableView*` run, where `Loaded`/`Unloaded` are observed nesting and the post-reparent API check reads a stale value. This is test debt, not a product bug — the suite shares one visual tree root and this test reparents across hosts. Fix the test's isolation before reading anything into a full-suite failure here, and always re-run it alone before triaging it.

- [x] `VerifyRepeatedLoadUnloadDoesNotLeakSubscriptions`
  - **Description:** Five load/unload cycles against `CountingItemsSource`, an instrumented `IList, INotifyCollectionChanged` with explicit event accessors that counts live listeners.
  - **Expected result:** The listener count never exceeds 1, never goes negative, and ends at its starting value.
  - **Failure means:** The control subscribes on load without a matching unsubscribe on unload. Each cycle leaks a handler that keeps the whole control alive and makes one source change fire N times — an unbounded leak in any app that shows the table repeatedly.
  - **Note:** The count tracks live `ItemsSourceView` instances, since XAML's view is what actually subscribes, not the control directly.

- [x] `VerifyUnloadWhileItemsSourcePendingDoesNotCrash`
  - **Description:** Five create/load/unload/collect cycles that tear down before first layout settles, so the rows pipeline still has queued work when unload runs.
  - **Expected result:** No crash, no assert, control is collectable.
  - **Failure means:** Teardown does not cancel or guard queued pipeline work, so a callback runs against a partly-destroyed control. This is a use-after-free class of bug and shows up in the field as a random crash on fast navigation.
  - **Note:** Modeled on `InkCanvasTests.InkCanvasDestroyedBeforePendingCallbacksDoesNotCrash`.

Repo precedent reviewed for this area: strong, and **all API tests**. `Repeater\APITests\ViewportTests.ValidateLoadUnload` is the gold standard (reparents between hosts, counts load/unload, bounds subscriber count). `InkCanvasTests.InkCanvasDestroyedBeforePendingCallbacksDoesNotCrash` and `InkCanvasLoadUnloadDoesNotCrash` cover teardown-during-pending-work. Leak coverage uses a `WeakReference` plus a bounded GC loop (`RepeaterTests.VerifyRepeaterDoesNotLeakItemContainers`, `ScrollViewerAdapterTests.CheckLeaks`). The only interaction-test unload precedent is `TeachingTipTests.TargetUnloadingClosesTeachingTip`, and that is there because it observes a popup, not because unload needs UIA.

Note: `IdleSynchronizer.Wait()` ticks the UI thread, which can mask teardown races. `ViewportTests:613` uses `Task.Delay(16 * 3).Wait()` instead for exactly that reason. If these tests pass but a teardown crash is still suspected, retry with an untickied wait before concluding the path is clean.

### 1.7 Empty source and EmptyTemplate

> **Remarks (applies to every item in 1.7) — protocol debt, step 3.**
> There is no empty-state spec anywhere in the repo, and the IDL declares only that `EmptyTemplate`
> is a `DataTemplate` DP. An IDL cannot express *when* an empty state is entered or left, what
> happens on the null-template opt-out, or whether the transition is live — so these expectations
> were derived from the three branches of `TableView::UpdateEmptyState` and therefore pin observed
> behavior rather than a stated contract. The behavior they pin is reasonable and self-consistent,
> which is why the tests were kept. But if the product changes any of these three branches on
> purpose, these tests will fail for a legitimate reason, and the right resolution is to write the
> empty-state contract down first and then update the tests to match *the contract* — never to match
> the new code. Not precedent for any other subcategory.

`TableView::UpdateEmptyState` (`TableView.cpp:1084`) has exactly three branches, and every test below is pinned to one of them:

1. `EmptyTemplate == null` → presenter collapsed, presenter `ContentTemplate` **nulled**, repeater visible. Unconditional opt-out.
2. `EmptyTemplate != null` and the row count is 0 (**including when there is no `ItemsSourceView` at all** — `isEmpty` defaults to `true`) → presenter takes the template, gets non-null content so the `ContentControl` inflates, becomes visible; repeater collapsed.
3. `EmptyTemplate != null` and the row count is non-zero → presenter collapsed, repeater visible. Note this branch does **not** clear `ContentTemplate`, so no test may assert that it does.

Re-evaluation is driven from four call sites only: `RefreshRowsPipeline` (which `OnApplyTemplate`, repeater `Loaded`, and shaping verbs all route through), `OnEmptyTemplatePropertyChanged`, `OnEmptyStateItemsSourceCollectionChanged`, and `TableView_Sort.cpp:554`. The collection-changed subscription is wired **only while `EmptyTemplate` is non-null**, so the live-update tests are also subscription-lifetime tests.

The contract every test below asserts is two-sided: **exactly one** of the empty-state presenter and the rows repeater is visible at any time.

- [x] `VerifyEmptyTemplateShowsForEmptyItemsSource`
  - **Description:** Set `EmptyTemplate`, set `ItemsSource` to an empty `List<Person>`, load.
  - **Expected result:** The empty-state presenter is visible with inflated non-null content; the rows repeater is collapsed.
  - **Failure means:** An app that sets an empty-state template gets a blank control instead of its "no data" message — the feature does not work at all.
  - **Note:** Previously crashed with `0xC0000420`; root cause was a stale `Microsoft.UI.Xaml.Controls.Tabular.dll` loaded from the extracted component package rather than the local build, **not** a product bug. The `OverwriteTabularWithLocalBuild` post-build target in `MUXControlsTestApp.csproj` prevents it recurring.

- [x] `VerifyNullItemsSourceShowsEmptyTemplate`
  - **Description:** Set `EmptyTemplate` but leave `ItemsSource` null.
  - **Expected result:** Same as above — presenter visible, repeater collapsed.
  - **Failure means:** "No source assigned yet" is treated differently from "source is empty". Both are empty from the user's point of view, and the async/late-binding case (source arrives after load) is the common one.

- [x] `VerifyEmptyTemplateHiddenWhenItemsPresent`
  - **Description:** Set `EmptyTemplate` with two items in the source.
  - **Expected result:** Presenter collapsed, repeater visible.
  - **Failure means:** The empty-state message covers real data.

- [x] `VerifyEmptyTemplateAppearsWhenSourceBecomesEmpty`
  - **Description:** Remove the last item from a loaded `ObservableCollection`.
  - **Expected result:** Presenter becomes visible, repeater becomes collapsed, without reassigning `ItemsSource`.
  - **Failure means:** Empty state is evaluated only at load. A table the user filters or deletes down to zero rows shows nothing rather than the empty message.

- [x] `VerifyEmptyTemplateDisappearsWhenItemAdded`
  - **Description:** Add an item to a loaded, empty `ObservableCollection`.
  - **Expected result:** Presenter becomes collapsed, repeater becomes visible.
  - **Failure means:** The transition works in one direction only, so the empty message stays pinned over the first row that arrives. Kept as a separate test from the above precisely because a one-directional regression is possible.

- [x] `VerifySettingEmptyTemplateWhileEmptyShowsIt`
  - **Description:** Load with an empty source and **no** template, then assign `EmptyTemplate` after load.
  - **Expected result:** The presenter becomes visible immediately.
  - **Failure means:** `EmptyTemplate` is only honored if set before load, breaking any app that assigns it from a style, a theme change, or a view model.

- [x] `VerifyClearingEmptyTemplateRestoresRows`
  - **Description:** Set `EmptyTemplate`, then set it back to null.
  - **Expected result:** Presenter collapsed, repeater visible, **and** the presenter's `ContentTemplate` is null.
  - **Failure means:** Clearing the template only hides the presenter without releasing it. The inflated content stays alive behind a collapsed element — a leak, and a stale template that reappears if the presenter is ever shown again. This is the only test asserting the `ContentTemplate` clear.

- [x] `VerifyNullItemsSourceWithoutEmptyTemplateShowsNothing`
  - **Description:** Load with a null source and no `EmptyTemplate` at all.
  - **Expected result:** No empty presenter content, repeater visible, no crash.
  - **Failure means:** The control fabricates an empty-state UI that the app never opted into, or null-derefs on the no-template path.

All eight assert through the shared `VerifyEmptyStateVisibility` helper, which checks the contract **two-sided** — presenter visible implies repeater collapsed and vice versa. Asserting only the presenter would pass while the control showed both surfaces at once.

⚠️ **Protocol debt (step 2).** These expectations were derived from the three branches of `TableView::UpdateEmptyState` (`TableView.cpp:1084`), not from a spec. `TableView.idl` declares `EmptyTemplate` but documents no behavior for it, and there is no empty-state design note in the repo. The behaviors asserted are the reasonable ones and match what any consumer would expect, but they are **not** spec-derived and must be re-validated if a spec is ever written. The one place this bit: branch 3 (`EmptyTemplate != null`, rows present) does **not** clear `ContentTemplate`, and no test asserts that it does — that omission is a concession to current behavior, not a stated contract.

Repo precedent reviewed for this area: **none exists.** No other control in `controls\dev` has an empty-state, placeholder, or no-results template, and no test file outside TableView mentions one. TableView is the first, so there is no established shape to follow. Placement is **API tests**: every part of the contract is observable through `Visibility`, `ContentTemplate`, and the inflated visual tree, with no input, hit-testing, or UIA involved, so an interaction test would add app-navigation flake for nothing.

- `(covered)` `VerifyEmptyTemplateSetAfterLoadAppliesLive` — leftover from the first draft of this checklist; `VerifySettingEmptyTemplateWhileEmptyShowsIt` above is the same test against the same `OnEmptyTemplatePropertyChanged` path.

**Category 1 is complete.** 30 tests in `TableViewTests.cs` across 1.1-1.7, minus the two dropped 1.5 items. All 30 pass.


</details>

---

## 2. Data binding and data source projection

<details>
<summary>Show 19 items &mdash; 19 written, 19 passing (26 TAEF cases with data variations) &mdash; <strong>complete</strong></summary>

File: `TableView_DataBinding_APITests.cs`.

### 2.0 Boundary with Category 8

These two categories look alike and are not. **Category 2 is the binding pipeline; Category 8 is the shaping algebra.**

- **§2 subject = `TableView`.** An *unshaped* source reaching rendered cells: `ItemsSource` accepts it, a column `Binding`
  populates a `TextBlock`, a `CellTemplate` receives the row item, a collection notification updates the realized visual tree.
  Assertions are on **rows and cells**.
- **§8 subject = `TableViewSource`.** `From`, `Filter`, `GroupBy`, `Sort` and their composition — testable with no `TableView`
  in the tree at all. Assertions are on **the projection**.

Four items sit on the seam. Owner is fixed here so neither category writes the other's test:

| Seam | Owner |
|---|---|
| `VerifyItemsSourceRoundtripsTableViewSource` (§2.1) vs §8.1 construction | §2 owns "a `TableView` accepts a `TableViewSource` and renders it". §8 owns what `From` itself accepts and rejects. |
| §2.4 live observable updates vs §8.6 live updates while shaped | §2 covers add/insert/remove/replace/move/reset on an **unshaped** source, asserting on rows. §8.6 covers the same notifications **while a filter, grouping or sort is active**, asserting on the projection. The shaped cases are the ones with real risk. |
| `VerifyObservableResetRebuildsAllRows` (§2.4) vs a source-level reset | §2. §8 adds a case only where a reset must be survived *by active shaping*. |
| `VerifyDuplicateItemInstancesAreHandled` (§2.6) vs §8 duplicate row identity | §2 asserts the benign case: the same instance twice yields two independent rows. §8 owns the documented **failure** mode where shaping requires unique object identity. |

Rule of thumb when adding to either: if the test would still make sense with **no shaping applied**, it belongs in §2.

### 2.1 ItemsSource shapes

- [x] `VerifyItemsSourceShapesRenderOneRowPerItem` *(data-driven: `List`, `ObservableCollection`, `TableViewSource`)*
  - **Description:** Assigns each supported source shape to `ItemsSource`, then reads the property back and inspects the realized rows.
  - **Expected result:** `ItemsSource` returns the same object instance that was assigned, and the rows repeater projects exactly one row per item in source order, with each row's `DataContext` being the corresponding item.
  - **Failure means:** The control either loses or copies the source it was handed, or its normalization step reorders or drops items — every other data test in this category rests on this and would report a misleading symptom.
  - **Remarks:** Merged from three separate plan items that differed only in the object constructed. The three shapes are genuinely different code paths inside `AdoptItemsSource` — a `TableViewSource` is adopted directly, anything else is projected through one — so the variation is not cosmetic. TAEF inline data is used (`[TestProperty("data:SourceKind", ...)]`); precedent for in-proc API tests is `NavigationView_ApiTests\NavigationViewTests.cs:253`, and `APITestBase` already reads `TestContext.DataRow`. `TableView.idl:458` declares only `Object ItemsSource`, so the *one row per item* half rests on the design intent that TableView renders its source, not on a documented sentence.
- [x] `VerifyItemsSourceSwapReplacesAllRows`
  - **Description:** Loads a table over one collection, then assigns a different collection with different items and different length.
  - **Expected result:** Every realized row carries an item from the new collection; no row's `DataContext` is an item of the discarded one, and the row count matches the new collection.
  - **Failure means:** Rows from a discarded data set survive a source swap, so the table shows data the app has already replaced.
  - **Remarks:** §6 owns what a swap does to *selection* (`VerifySelectionRemainsCoherentAfterItemsSourceSwap`); this owns what it does to *rows*. Both are kept because selection re-anchoring and row rebuilding are separate mechanisms and a regression could break either alone.
- [x] `VerifyItemsSourceSetToNullClearsRows`
  - **Description:** Loads a populated table, then sets `ItemsSource` back to `null`.
  - **Expected result:** The projection is empty (`ItemsSourceView` is `null` or reports zero), and no leftover row container is arranged anywhere inside the control's bounds.
  - **Failure means:** Clearing the source leaves stale rows on screen — the worst version of a stale-data bug, because the app has explicitly said there is nothing to show.
  - **Remarks:** Distinct from §1.7, which starts at `null` and asserts the *empty-state* surfaces. This asserts the populated → null transition, which goes through `OnItemsSourcePropertyChanged` rather than through initial load. The expectation was corrected once during authoring: the first version asserted *zero realized row containers*, which failed. That is `ItemsRepeater` behaviour and not a TableView defect — cleared containers stay parented for reuse (`RecyclePool::PutElementCore`) and are arranged at `ItemsRepeater::ClearedElementsArrangePosition` = `(-10000, -10000)` (`ItemsRepeater.cpp:23`). The assertion now describes what a user can see, which is what the contract actually is.
- [ ] ~~`VerifyItemsSourceRoundtripsNull`~~ **(dropped — already covered)**
  - §1.7 `VerifyNullItemsSourceWithoutEmptyTemplateShowsNothing` loads with `ItemsSource = null` and asserts the repeater is collapsed and the empty presenter absent. The only part not covered there is the DP read-back, and a `DependencyProperty` returning what was assigned through the same code path is the platform's guarantee, not TableView's (Step 2, rule 3).

### 2.2 Text column binding

- [x] `VerifyTextCellTextFollowsTheBindingPath` *(data-driven: `Simple`, `Dotted`, `Invalid`, `None`)*
  - **Description:** One realized text column per case, with `Binding` set to a simple path, a nested path, a path no property answers, and `null`; reads `TextBlock.Text` from the realized cell.
  - **Expected result:** Simple → the property value (`"Asha"`); Dotted → the nested value (`"Studio"`); Invalid → empty string; None → empty string. No case shows the item's `ToString()`.
  - **Failure means:** Cells show the wrong value, or — for the last two cases — the type name of the data item, which is the classic unbound-cell symptom and is worse than blank because it looks like data.
  - **Remarks:** Merged from four plan items that share one observation and differ only in the expected text. `TableView.idl:299` declares `Binding` as a CLR property *"so Binding-typed XAML values route through the setter"* but documents no rendering contract, so the expectations are reasoned: with no binding the product never calls `SetBinding`, and an unresolvable path is XAML's own silent-failure behaviour. The Invalid and None cases are partly **guarding the platform** rather than TableView (Step 2, rule 3) — they are kept as cheap variations because a future hand-rolled value extraction would turn both into crashes, and that is exactly the regression they would catch. Say so, do not present them as contract tests.
- [x] `VerifyTextColumnBindingChangeAfterLoadUpdatesRealizedCells`
  - **Description:** Replaces `Binding` on a loaded, realized text column with a binding to a different property.
  - **Expected result:** Every realized cell in that column shows the new property's value; other columns are untouched.
  - **Failure means:** A live column reconfiguration is ignored until something else rebuilds the rows, so an app that re-points a column sees stale cells.
  - **Remarks:** `Binding` is a CLR property, not a DP, so there is no property-changed callback to rely on — the rebuild has to be explicit in the setter. That makes this the weakest link in the text-column path and the reason it is a separate test rather than a variation above. Spec silence: *(needs spec decision)* on whether a post-load `Binding` change is supported at all, or whether apps are expected to rebuild the column.

### 2.3 Template column binding

- [x] `VerifyTemplateColumnWithNoCellTemplateRendersEmptyCell`
  - **Description:** A `TableViewTemplateColumn` with `CellTemplate` left `null`, realized over a populated source.
  - **Expected result:** The cell wrapper hosts a presenter with no `ContentTemplate` and renders no text; specifically it does not fall back to the data item's `ToString()`.
  - **Failure means:** A column an app has not finished configuring leaks type names into the grid.
  - **Remarks:** `TableView.idl:310-312` documents `CellTemplate` as *"Template used to generate display cell content"* and says nothing about its absence. The no-`ToString()` expectation is reasoned from the same principle as §2.2 and is flagged the same way.
- [x] `VerifyCellTemplateChangeAfterLoadRebuildsRealizedCells`
  - **Description:** Assigns a different `CellTemplate` to a loaded template column whose cells are already realized.
  - **Expected result:** Realized cells carry the new template and render through it; the row's `DataContext` is unchanged.
  - **Failure means:** `CellTemplate` is read once at realization, so a live template swap silently does nothing.
  - **Remarks:** `CellTemplate` *is* a DP with `MUX_PROPERTY_CHANGED_CALLBACK(TRUE)` (`TableView.idl:311`), so unlike §2.2's `Binding` there is a documented notification hook — which makes a failure here a clear product bug rather than a spec question. §4 `VerifyCellTemplateChangeRemeasuresAutoColumn` asserts the *sizing* consequence of the same change; this asserts the content consequence. Both kept: a rebuild that forgot to invalidate measure, or a measure invalidation with no rebuild, are different regressions.
- [ ] ~~`VerifyTemplateColumnCellTemplateReceivesRowItem`~~ **(dropped — already covered)**
  - §5.2 `VerifyTemplateCellRendersRowItem` already asserts that the generated `ContentPresenter` carries the column's `CellTemplate` and resolves the row's data item through its inherited `DataContext`, with a written note on why `DataContext` and not `Content` is the thing to assert. Re-asserting it here would be the same observation on the same element.

### 2.4 Live source updates

- [x] `VerifyObservableInsertPlacesRowAtThatPosition` *(data-driven: `Start`, `Middle`, `End`)*
  - **Description:** Inserts one item into a loaded `ObservableCollection` at the front, the middle, and the end.
  - **Expected result:** The projected row sequence equals the collection's sequence after each insert, and the inserted item's row sits at exactly the inserted index.
  - **Failure means:** Rows and items disagree on order after an insert — the table is showing a different list from the one the app holds.
  - **Remarks:** Merged from two plan items (`Add` at the end and `Insert` at an index) plus a front case, since appending is just the last index and the assertion is identical. §8.6 owns the same notification **while shaping is active**, where the projected index is not the source index; per §2.0 this one is deliberately unshaped.
- [x] `VerifyObservableRemoveRemovesOnlyThatRow`
  - **Description:** Removes a middle item from a loaded observable source.
  - **Expected result:** That item has no row; every other item still has one, in the same relative order; the row count drops by exactly one.
  - **Failure means:** A removal takes a neighbour with it or leaves a phantom row bound to a detached item.
- [x] `VerifyObservableReplaceUpdatesRowInPlace`
  - **Description:** Assigns a new item over an existing index (`collection[i] = newItem`).
  - **Expected result:** The row at that index carries the new item; the rows either side carry their original items.
  - **Failure means:** A replace is handled as a remove-plus-add at the wrong position, or rebuilds the whole list — the second is only observable as neighbours losing identity, which this asserts.
- [x] `VerifyObservableMoveReordersRows`
  - **Description:** Moves an item from one index to another in the loaded observable source.
  - **Expected result:** The projected row sequence matches the collection's new order.
  - **Failure means:** `Move` notifications are dropped or mishandled, so a reordering app sees no change.
  - **Remarks:** Asserts only the resulting order, **not** that containers were reused rather than rebuilt. The original plan item said "without rebuilding the whole list", which is not observable from public API — container identity across a repeater's own recycling is not a contract. Narrowed deliberately.
- [x] `VerifyObservableResetRebuildsAllRows`
  - **Description:** Calls `Clear()` and repopulates, producing a reset notification followed by adds.
  - **Expected result:** No row carries an item from before the reset; the final rows match the final contents exactly.
  - **Failure means:** A reset leaves stale rows, which is the most common shape of this bug because reset carries no per-item information.
- [x] `VerifyRapidObservableMutationsSettleToExpectedRows`
  - **Description:** A burst of interleaved inserts, removes and replaces in a single UI-thread turn, settled once at the end.
  - **Expected result:** The final row sequence equals the final collection contents.
  - **Failure means:** The control coalesces notifications incorrectly, so a run of changes ends in a state no single change would produce.
  - **Remarks:** The unshaped twin of §8.6 `VerifyBurstOfMutationsWhileShapedSettlesCorrectly`. Both kept: that one exercises re-shaping, this one exercises only the repeater feed, and a coalescing bug in the feed would be masked by a full re-shape.

### 2.5 Item property change notification

- [x] `VerifyItemPropertyChangeUpdatesItsBoundTextCell`
  - **Description:** Raises `INotifyPropertyChanged` for a bound property on an item that has a realized row.
  - **Expected result:** That cell's text becomes the new value; other rows and columns are unchanged.
  - **Failure means:** Cells are one-shot snapshots, so an app that edits its model outside the grid sees nothing.
  - **Remarks:** §14 `VerifyItemPropertyChangeUpdatesCellToolTipInPlace` asserts the same notification reaching the *tooltip*, which is a separately installed binding; §8 `VerifySortKeyPropertyChangeDoesNotMoveTheRowUntilACollectionChange` asserts the same notification deliberately **not** re-shaping. This is the third distinct consumer — the cell text itself — and nothing asserts it today.
- [x] `VerifyItemPropertyChangeOnAnUnrealizedItemIsIgnored`
  - **Description:** Over a source large enough to virtualize, raises a property change on an item far below the viewport, then scrolls to it.
  - **Expected result:** Nothing is thrown at the time of the change, and when the row is finally realized it shows the new value.
  - **Failure means:** The control subscribes to items it is not showing and either crashes or renders stale data once the row arrives.
  - **Remarks:** The original item said only "does not crash", which is close to unfailable. Strengthened with the scroll-and-check half so the test can actually fail.
- [x] `VerifyItemPropertyChangeAfterSourceSwapDoesNotTouchNewRows`
  - **Description:** Swaps `ItemsSource` to a second collection, then raises property changes on items belonging to the *discarded* collection.
  - **Expected result:** No realized cell changes, and nothing is thrown.
  - **Failure means:** Subscriptions to the old data set outlive the swap — both a stale-render bug and a leak, since the control keeps the discarded items alive.
  - **Remarks:** `AdoptItemsSource` states detaching the previous source as its reason for existing (*"without this it keeps a back-pointer to this control"*), which makes this an invariant the product asserts about itself (Step 3, source 3), not an inference.

### 2.6 Error and edge inputs

- [x] `VerifyUnsupportedItemsSourceThrowsAndLeavesTheControlUsable`
  - **Description:** Assigns a non-collection value (a boxed `int`) to `ItemsSource`, then assigns a valid collection.
  - **Expected result:** The assignment throws, and the subsequent valid assignment renders normally.
  - **Failure means:** Either a bad source is swallowed and the table silently shows nothing, or it wedges the control so a later valid source cannot recover.
  - **Remarks:** `AdoptItemsSource` calls `TableViewSource::From` *"deliberately unguarded … Let it surface"*, and §8 `VerifyFromUnsupportedSourceThrows` already covers `From` in isolation. This one asserts what the *control* does with that throw, which is the part an app sees. The throw crosses a DP-changed callback, so **if TAEF reports a host crash rather than a catchable exception, that is the finding** — record it and mark the item, do not soften the assertion.
- [x] `VerifyThrowingEnumeratorLeavesControlUsable` *(needs spec decision)*
  - **Description:** Assigns a collection whose enumerator throws partway through, then assigns a valid collection.
  - **Expected result:** The control does not crash the host, and the subsequent valid source renders normally.
  - **Failure means:** One misbehaving data source is fatal to the control rather than to the operation.
  - **Remarks:** Nothing in the IDL or the design notes covers a throwing source, and the shaping stack's only documented exception policy is for *filter predicates*, not enumeration. Flagged accordingly: if this cannot be expressed without pinning today's behaviour, it is dropped with that reason rather than weakened.
- [x] `VerifyDuplicateItemInstancesProduceIndependentRows`
  - **Description:** An unshaped source containing the same object instance twice.
  - **Expected result:** Two rows, both with that instance as `DataContext`, rendering the same values; the table is fully functional.
  - **Failure means:** The control assumes item identity is unique even when no shaping is applied, which would make duplicate-bearing collections unusable for no stated reason.
  - **Remarks:** The deliberate counterpart to §8 `VerifyDuplicateItemObjectThrowsWhenShaped`. `TableViewSource.idl` requires unique object identity only *"when a shaping verb is active"*, so the benign case has to stay benign — together the two tests pin the boundary, and either alone would let it drift.


</details>

---

## 3. Columns and headers

<details>
<summary>Show 31 items &mdash; 31 written, 29 passing, 2 failing (product bugs)</summary>

File: `TableView_Columns_APITests.cs`.

### 3.1 Columns collection

**Classification: all API tests.** Every item is reachable by mutating `Columns` programmatically and reading the resulting visual tree. No input, hit-testing, or UIA is involved.

Header cells and row cells both carry a `Tag` pointing at the `TableViewColumn` that produced them, which is how these tests map rendered elements back to columns.

- [x] `VerifyColumnsCollectionIsObservable`
  - **Description:** Cast `Columns` to `IObservableVector<TableViewColumn>` and record the notifications raised by Add, Insert(0), RemoveAt(0), and Clear.
  - **Expected result:** The cast succeeds. Exactly 4 notifications, in order: `ItemInserted@0`, `ItemInserted@0`, `ItemRemoved@0`, `Reset`.
  - **Failure means:** `Columns` is not observable, or reports the wrong change kind or index. `TableView.idl:452` states the ABI exposes `IVector` but the backing vector is observable *because the control relies on it for live column updates*. If this breaks, every incremental column update degrades to a full rebuild or to no rebuild at all — and the failure is silent, since the collection itself still behaves correctly.

- [x] `VerifyAddColumnAddsHeaderAndCells`
  - **Description:** Add a column to a loaded TableView.
  - **Expected result:** One additional header cell appears, tagged with the new column, at the end; every realized row gains one cell for that column, also at the end.
  - **Failure means:** Columns added after load do not render — the table is effectively static once shown.

- [x] `VerifyInsertColumnPlacesHeaderAtIndex`
  - **Description:** Insert a column at index 1 of a two-column loaded TableView.
  - **Expected result:** 3 header cells; the header at index 1 is the inserted column; every row's cells map to the same columns in the same order.
  - **Failure means:** Insert is treated as append. The right set of columns renders in the wrong order, which is a data-correctness bug — values appear under the wrong headings.

- [x] `VerifyRemoveColumnRemovesHeaderAndCells`
  - **Description:** Remove the middle column of three.
  - **Expected result:** 2 header cells remain, neither tagged with the removed column; no row contains a cell for it; remaining order is preserved.
  - **Failure means:** Removed columns leave orphaned headers or cells behind, so the table renders a column the app has deleted.

- [x] `VerifyClearColumnsRemovesAllHeaders`
  - **Description:** Call `Columns.Clear()` on a loaded two-column TableView.
  - **Expected result:** 0 header cells and 0 cells in every realized row.
  - **Failure means:** `Reset` is not handled, only incremental add/remove. Clear is the standard way to rebuild a column set from a view model, and a stale header host after Clear is a visible corruption.

- [x] `VerifyReplaceColumnSwapsHeaderAndCells`
  - **Description:** Assign a new column to `Columns[0]` via the indexer.
  - **Expected result:** Header at index 0 is the replacement; header at index 1 is the original untouched column; the replaced column has no header; rows match.
  - **Failure means:** `ItemChanged` is unhandled or handled as remove+append, so a replace either does nothing or reorders the surviving columns.

- [x] `VerifyColumnsChangedBeforeLoadAppliesOnLoad`
  - **Description:** Add, add, insert, and remove columns on a TableView with no template and no visual tree, then load it.
  - **Expected result:** After first layout the header host and every row match the final 2-column state exactly.
  - **Failure means:** The control only tracks columns once a template is applied. Building a table in code-behind before adding it to the tree — the normal pattern — produces a header host that disagrees with `Columns`.

- [x] `VerifyHeaderAndRowCellsStayInSyncAfterColumnMutation`
  - **Description:** Apply add, insert, remove, replace, and clear in sequence, re-checking after each that the `PART_HeaderHost` child list and every realized row's `PART_CellsHost` child list have the same length and map to the same columns in the same order.
  - **Expected result:** The two lists agree after every single mutation, not just at the end.
  - **Failure means:** Headers and row cells have desynchronized. Headers and cells are rebuilt by two **independent** reactions to the same `Columns.VectorChanged`, so one path can run while the other does not, or they can disagree on index. The user-visible result is data under the wrong heading.
  - **Note:** Deliberately **structural**, not a pixel-offset check. `TableViewCellsPanel` is used as both the header host and each row's cells host, and arranges children by accumulating `ActualWidth` in `Columns` order — so alignment *is* the two lists agreeing. Comparing `TransformToVisual` offsets would mostly re-test XAML's arrange while importing layout-timing, DPI-rounding, and scroll-offset flake.

- [x] `VerifySameColumnInstanceAddedTwiceBehavesPerContract`
  - **Description:** Add the same `TableViewColumn` instance at two indices of one `Columns` collection.
  - **Expected result:** `Columns.Count == 2`; two header cells, both tagged with that instance; two cells per row; header and row lists still agree.
  - **Failure means:** A duplicated column entry corrupts layout or crashes, rather than rendering twice.
  - ⚠️ **Protocol debt (step 2).** The IDL says nothing about duplicate entries. The expectation was resolved by reading `TableViewColumn::SetOwningTableViewInternal` (`TableViewColumn.cpp:373`), which rejects re-own only by a *different* `TableView` and returns `true` for the same owner — so the "throws" alternative the checklist originally hedged on does not exist. This test currently pins **observed behavior**, not a stated contract. If a spec is written and says duplicates should be rejected, this test changes.

  This is deliberately a **structural** check, not a pixel-offset one. `TableViewCellsPanel` is used as both `PART_HeaderHost` and each row's cells host (`TableViewCellsPanel.h:15`), and its `ArrangeOverride` positions children in both by accumulating `column.ActualWidth()` in `Columns` order. Alignment is therefore a consequence of the two hosts holding the same columns in the same order, not an independent property worth measuring — comparing `TransformToVisual` offsets would mostly re-test XAML's arrange while importing layout-timing, DPI-rounding, and scroll-offset flake.

  The real risk it guards is that headers and row cells are rebuilt by two **independent** reactions to the same `Columns.VectorChanged`: `TableView::RebuildHeaders()` (`TableView.cpp:1503`) and `TableViewRow::OnColumnsVectorChanged` (`TableViewRow.cpp:296`). If one path runs and the other does not, or they disagree on index, the two hosts desynchronize. That is observable directly and deterministically from the child lists.
- [x] `VerifySameColumnInstanceAddedTwiceBehavesPerContract` — The same `TableViewColumn` object added at two indices of one `Columns` collection produces two independent header/cell sets and does not corrupt layout.

  Contract resolved from source: `TableViewColumn::SetOwningTableViewInternal` (`TableViewColumn.cpp:373`) rejects a re-own only by a *different* `TableView`; re-owning by the same control returns `true`. So a duplicate entry is owned, tracked, and rendered like any other entry — the "throws" branch does not exist. Test asserts two headers and two cells per row for one column object.

  - **Remarks:** **Protocol debt — step 3 violation, flagged not excused.** The expectation is derived from `.cpp`, not from the IDL or a spec, because neither says anything about duplicate instances in one collection. It therefore pins today's behavior, and if the product later decides duplicates are a programming error (the way *cross*-TableView re-ownership already is, via `MUX_ASSERT_MSG` at `TableViewColumn.cpp:381`), this test will fail for a legitimate reason. If that happens it is the one case where changing the test is correct — but only after the contract is written down. Not precedent for any other item.

### 3.2 Column ownership

**Classification: API test.** No public owning-TableView accessor exists in `TableView.idl`, so ownership is only observable through what renders.

- [x] `VerifyRemovedColumnCanBeReAdded`
  - **Description:** Remove a column from a loaded TableView, then add the same instance back.
  - **Expected result:** After re-adding, the column has a header again and a cell in every realized row, and the header/cell lists agree.
  - **Failure means:** Ownership is released on removal but never re-acquired, so a column instance is single-use. Any app that toggles columns by removing and restoring them gets a permanently blank column on the second show.

- `(dropped)` `VerifyColumnAddedToTwoTableViewsBehavesPerContract` — **step 2 and step 3.** `TableView.idl` states no contract for a column owned by two controls, so there is no spec-derived expectation to assert. The only statement anywhere is `MUX_ASSERT_MSG(false, "TableViewColumn re-owned without removing it from the previous TableView.Columns first")` (`TableViewColumn.cpp:381`), which declares the scenario a **programming error**, not a supported one. Deliberately driving an asserted-illegal path would also fire the assert and take down the chk test host. If multi-owner ever becomes supported, this needs a spec first.
- `(dropped)` `VerifyRemovedColumnStopsAffectingRows` — **step 2.** Split across two areas that already own it: §3.1 `VerifyRemoveColumnRemovesHeaderAndCells` already asserts the removed column has no header and no cells, and §7 owns "removing a sorted column clears sort state". Nothing was left for this test to assert on its own.
- `(deferred)` `GetOwningTableView()` round-trip tests. PR `!15971489` tests this; the accessor is not in this repo's IDL.

### 3.3 Header content

**Classification: all API tests.** Header content is read back from the `ContentPresenter` inside each header cell; no input or UIA is involved.

⚠️ **Spec correction.** This checklist previously contained `VerifyHeaderTemplateTakesPrecedenceOverSelector`, which asserted the standard XAML `ContentPresenter` convention. `TableView.idl:178` states the opposite: *"Optional header template; HeaderTemplateSelector takes precedence."* The item below is inverted to match the spec. This is worth flagging to reviewers, because TableView deviates from `ContentControl` here and the deviation is easy to "fix" by accident.

- [x] `VerifyHeaderRendersStringObjectAndNull`
  - **Description:** One test, three cases: `Header` set to a string, to a non-string object, and to null.
  - **Expected result:** String renders as header text; the object is presented as content (the presenter's `Content` is that instance); null produces an empty header cell and no crash. A header cell exists in all three cases.
  - **Failure means:** `Header` is assumed to be a string. A non-string header throws or renders `ToString()` where the app expected a template, and a null header crashes header rebuild.
  - **Note:** Merged from three separate items per step 2.2 — they differed only in the value assigned to one property.

- [x] `VerifyHeaderChangeAfterLoadUpdatesLive`
  - **Description:** Change `Header` on a column of a loaded TableView.
  - **Expected result:** The rendered header content updates to the new value without touching `Columns`.
  - **Failure means:** Headers are snapshotted at load. A header bound to a localized or view-model-driven string never updates.

- [x] `VerifyHeaderTemplateApplies`
  - **Description:** Set `HeaderTemplate` on a column and load.
  - **Expected result:** The header cell's presenter has that `ContentTemplate`, and the template's content appears in the header.
  - **Failure means:** `HeaderTemplate` is ignored, so rich headers (icons, glyphs, wrapped text) are impossible.

- [x] `VerifyHeaderTemplateSelectorApplies`
  - **Description:** Set only `HeaderTemplateSelector` and load.
  - **Expected result:** The presenter's `ContentTemplateSelector` is the supplied selector and the selected template's content appears.
  - **Failure means:** The selector is ignored, so per-column header presentation driven by column type or state does not work.

- [x] `VerifyHeaderTemplateSelectorTakesPrecedenceOverHeaderTemplate`
  - **Description:** Set **both** `HeaderTemplate` and `HeaderTemplateSelector` on the same column.
  - **Expected result:** The presenter has the selector applied and **no** `ContentTemplate` — the selector wins, per `TableView.idl:178`.
  - **Failure means:** TableView silently reverted to the `ContentControl` convention where the template wins. Any column that sets both would then render the wrong header, and the documented behavior in the IDL becomes a lie.
  - **Remarks:** **Deliberate inversion of the platform convention, and the plan originally had it backwards.** The item was first written as `VerifyHeaderTemplateTakesPrecedenceOverSelector`, asserting the usual `ContentControl` rule. `TableView.idl:178` states the opposite outright — "Optional header template; HeaderTemplateSelector takes precedence" — so the item was inverted before any code was written. This is the clearest case of step 3 paying for itself: reading the implementation would have produced the same answer, but reading the IDL produced it *first* and made it authoritative. The in-code comment on this test must stay, or a future reader will "fix" it back to the convention.

- [x] `VerifyHeaderTemplateChangeAfterLoadUpdatesLive`
  - **Description:** Replace `HeaderTemplate` on a loaded column.
  - **Expected result:** The rendered header rebuilds with the new template.
  - **Failure means:** The `HeaderTemplate` property-changed callback does not trigger a header rebuild.

- [x] `VerifyHeaderTemplateSelectorChangeAfterLoadUpdatesLive`
  - **Description:** Replace `HeaderTemplateSelector` on a loaded column.
  - **Expected result:** The rendered header rebuilds and re-runs selection.
  - **Failure means:** The selector's property-changed callback does not trigger a rebuild. Kept separate from the test above because they are **different DP callbacks** — a regression can plausibly break one and not the other, which is the step 2.4 exception.

`HeaderToolTip` is a header-content property but is not tested here: its rendering lives in 14.2 and its UIA projection in 12.4. 1.2 and 1.3 already cover its default and dependency property backing.

### 3.4 Headers visibility

`TableViewHeadersVisibility` in this repo has exactly two values, `None` and `Column` (`TableView.idl:29`). PR `!15971489` additionally has row headers, which is why it carries `ColumnHeaderVisibility_None_BothHeadersCollapsed` and `ColumnHeaderVisibility_Column_OnlyColumnHeaderVisible`. There is no second header surface here, so those two tests have no analogue and should not be added.

- [x] `VerifyHeadersVisibilityColumnShowsHeaderRow`
  - **Description:** Load with the default `HeadersVisibility.Column`.
  - **Expected result:** `PART_HeaderRow` is `Visibility.Visible` and has non-zero height.
  - **Failure means:** The default renders no headers, which makes the control unusable out of the box.

- [x] `VerifyHeadersVisibilityNoneCollapsesHeaderRow`
  - **Description:** Set `HeadersVisibility.None` and load.
  - **Expected result:** `PART_HeaderRow` is `Visibility.Collapsed` and contributes zero height — the body starts at the top of the control.
  - **Failure means:** `None` only hides the header without reclaiming its space, leaving a blank band. `Collapsed` versus `Hidden` is the whole point of the value.

- [x] `VerifyHeadersVisibilityChangeAfterLoadUpdatesLive`
  - **Description:** Toggle `HeadersVisibility` both directions on a loaded control.
  - **Expected result:** The header row's visibility and the body's offset update after each toggle.
  - **Failure means:** The property is read only at template application, so an app cannot hide headers in response to state.

### 3.5 Column visibility

**Classification: all API tests.**

- [!] `VerifyCollapsedColumnRemovesCellsFromLayout`
  - **Description:** Set `Visibility.Collapsed` on one column of a loaded, multi-column TableView.
  - **Expected result:** The collapsed column contributes zero width, its header and cell elements are not visible, and the remaining columns close up. The column remains in `Columns`.
  - **Failure means:** Hiding a column either does nothing or leaves a gap where it was. Column show/hide is a headline table feature; a reserved blank strip is a visible bug.
  - **Remarks:** **Currently failing** — header and cells both report `ActualWidth == 120` (the authored default) instead of `0`. The IDL declares `TableViewColumn.Visibility` but cannot state what collapsing does to layout, so the expectation is reasoned from the ordinary XAML meaning of `Visibility.Collapsed` — an element that takes no space — applied to the column's header and cell elements. That is a strong convention but it is *not* written down for TableView anywhere. Per step 5 the product is wrong by default; the alternative reading, that `TableViewColumn.Visibility` is intended to hide content without reclaiming the track, needs a spec decision before this expectation is touched.

- [x] `VerifyCollapsedColumnPreservesColumnState`
  - **Description:** Record a column's `Width`, `ActualWidth`, `Header`, and `SortDirection`, collapse it, and read them back.
  - **Expected result:** All state is unchanged while hidden; the column object is not reset or detached.
  - **Failure means:** Collapsing is implemented as a disguised remove, so a hidden column loses its authored width and its sort state. The user hides a column, shows it again, and their sizing is gone.
  - **Remarks:** Passing. Independent of the layout question above — it asserts only that the column object survives, which holds either way.

- [!] `VerifyRestoringColumnVisibilityRestoresCells`
  - **Description:** Collapse a column, then set `Visibility` back to `Visible`.
  - **Expected result:** Header and cells reappear at the original index with the previously recorded width and state; header/cell lists agree.
  - **Failure means:** The show direction is broken, or the column returns at the wrong position or a default width. Kept separate from the collapse test because hide and show are different transitions and a regression can break only one — the step 2.4 exception.
  - **Remarks:** **Currently failing on the collapse half only** — the assertion that trips is the intermediate "width is 0 while collapsed" check, i.e. the same product question as `VerifyCollapsedColumnRemovesCellsFromLayout`, not a defect in the restore path. Resolve the collapse expectation first; this one very likely clears with it. Do not split or weaken the intermediate check to make the restore direction go green on its own.

- `(dropped)` `VerifyHeadersStayAlignedWithCellsAfterVisibilityChange` — **step 2.** Both hosts run the one shared `TableViewCellsPanel::ArrangeOverride`, which gives a collapsed column width 0 in each, so this restated `VerifyCollapsedColumnRemovesCellsFromLayout`. A structural child-list check does not rescue it either: the collapsed column stays in both hosts and only its width changes.

### 3.6 Cell element factory and custom columns

**Classification: all API tests.** `GenerateElement` is public and callable directly; the override path is reachable from a test-local derived column.

**This subcategory was missing entirely and is the largest gap found when comparing against the PR and the IDL.** `TableViewColumn.GenerateElement(Object dataItem)` is public and `GenerateElementCore(Object dataItem)` is `overridable` (`TableView.idl:194` and `:201`), which makes deriving a custom column a supported extensibility point. PR `!15971489` has a single test here, `TableViewTextColumn_GenerateElementProducesTextBlock`, which only covers the built-in column and never exercises the override.

The IDL states the contract directly (`TableView.idl:198-201`):

> The returned element must bind reactively to its inherited DataContext (the row data item) and must not set a local DataContext: rows are recycled and their DataContext is updated in place, so cells refresh through inheritance/bindings. dataItem is provided for initial setup only; baking it in as static content (or a local DataContext) will show stale data after recycle.

That is a spec-stated, testable contract, and it is currently unguarded.

- [x] `VerifyTextColumnGenerateElementProducesTextBlock`
  - **Description:** Call `GenerateElement(item)` directly on a `TableViewTextColumn`.
  - **Expected result:** A non-null `TextBlock`.
  - **Failure means:** The built-in text cell shape changed. Every style, automation peer, and test that reaches for the cell's `TextBlock` breaks.

- [x] `VerifyGenerateElementReturnsNewElementPerCall`
  - **Description:** Call `GenerateElement` twice with the same data item.
  - **Expected result:** Two distinct instances (not reference-equal).
  - **Failure means:** The column caches one element and hands it to every row. A `FrameworkElement` cannot have two parents, so this manifests as rows losing their cells as later rows realize.

- [x] `VerifyGenerateElementDoesNotSetLocalDataContext`
  - **Description:** For each built-in column type, check `ReadLocalValue(FrameworkElement.DataContextProperty)` on the generated element.
  - **Expected result:** `DependencyProperty.UnsetValue` — no local `DataContext` was set.
  - **Failure means:** The built-in columns violate the very contract the IDL imposes on third-party columns. Cells would show stale data after recycle, and the documented rule loses its reference implementation.

- [x] `VerifyCustomColumnGenerateElementCoreIsCalled`
  - **Description:** Derive a test-local column overriding `GenerateElementCore`, add it to a loaded TableView, and count invocations.
  - **Expected result:** The override is invoked at least once per realized row, and `GenerateElement` routes to it.
  - **Failure means:** The overridable is not actually called on the realization path, so the entire custom-column extensibility point is decorative.

- [x] `VerifyCustomColumnElementRendersInCell`
  - **Description:** Check the visual tree for the element the custom column returned.
  - **Expected result:** That element is inside the cell wrapper of the row, in the custom column's position.
  - **Failure means:** The override is called but its result is discarded or reparented wrongly, so custom cells do not appear.

- [x] `VerifyCustomColumnElementUpdatesOnRecycle`
  - **Description:** A custom column whose element binds to the inherited `DataContext` (the shape `Samples\TableViewSampleApp\ScoreBarColumn.cs` demonstrates); scroll a long source far enough to recycle rows, then read the cells.
  - **Expected result:** Recycled cells show the **new** row's data.
  - **Failure means:** The recycle contract quoted above is broken — rows are recycled without their `DataContext` being updated in place, so every correctly-written custom column shows stale data. This is the single highest-value test in the subcategory because the failure is silent and data-corrupting.
  - **Remarks:** Expectation quoted verbatim from `TableView.idl:198-201`, which requires the element to "bind reactively to its inherited DataContext… rows are recycled and their DataContext is updated in place" — a stated contract, not an inference, so it was never a candidate for adjustment. **Was failing; the product was exonerated and the test fixed.** Two test bugs, both mine: (1) the test sampled rows in the window after `ChangeView` returns but before the repeater finishes re-assigning `DataContext` to recycled containers, reporting a transient unbound state as stale data — fixed with the double-settle (`ChangeView` → `IdleSynchronizer.Wait()` → `UpdateLayout()` → `IdleSynchronizer.Wait()`) now standard in §5.3; (2) the `0xC000027B` was a cascade, not an assert — `Verify.IsNotNull` followed by a dereference raised an NRE that hid the real assertion. See the lesson in Current status.

- [x] `VerifyTemplateColumnGenerateElementUsesCellTemplate`
  - **Description:** Call `GenerateElement` on a `TableViewTemplateColumn` with a `CellTemplate` set.
  - **Expected result:** The returned element is a `ContentPresenter` carrying that `CellTemplate` as its `ContentTemplate`, and is **not** a `TextBlock`.
  - **Failure means:** Template columns silently degrade to text, discarding the app's template.
  - **Remarks:** **Was failing; the expectation was wrong, not the product.** The original test asserted that `GenerateElement` on an **unattached** column returns *inflated* template content. Nothing in the IDL promises that, and the suspicion recorded here at the time — that `GenerateElement` may legitimately return a container whose template inflates on load rather than on return — proved correct. That made the old assertion a step-3 violation: an expectation taken from how the code might work rather than from the contract. Rewritten to the contract the IDL does support — the right element type carrying the right template, with no text fallback. Inflation and the item actually rendering are covered by §5.2, where a row is loaded and can legitimately be measured.


- [x] `VerifyGenerateElementWithNullDataItemDoesNotCrash`
  - **Description:** Call `GenerateElement(null)`.
  - **Expected result:** Returns without throwing.
  - **Failure means:** A null item anywhere in a source throws inside row realization, taking down the app rather than rendering a blank cell. Null items are legal in a `List<T>` of reference types.
  - ⚠️ The IDL does not say whether the return may be null. The test asserts **only** "does not throw" and deliberately does not pin the return value; pinning it would be inventing a contract (step 3).

Open risk **resolved**: a C# class can derive from `TableViewColumn` and override `GenerateElementCore`. Verified two ways. First, the projected metadata in `Microsoft.WinUI.dll` shows the CsWinRT composable pattern — `TableViewColumn` is unsealed with a `protected .ctor(DerivedComposed)`, and both `GenerateElementCore(Object)` and `GetSortMemberPathCore()` project as `protected virtual`. `TableViewTextColumn` and `TableViewTemplateColumn` are likewise unsealed with `DerivedComposed` constructors, so the built-ins can also be derived from. Second, `Samples\TableViewSampleApp\ScoreBarColumn.cs` derives from `TableViewColumn` and overrides both methods, and the sample builds clean. The CS0436 dual-projection concern does not apply: the composable pattern is emitted into the winmd, so the test app's local projection has the same shape.

`ScoreBarColumn` is also the reference for correct cell-factory behavior: it binds `ProgressBar.Value` with `SetBinding` against the **inherited** `DataContext`, never assigns `DataContext`, and never bakes `dataItem` in as static content. `VerifyCustomColumnElementUpdatesOnRecycle` should be written against a column of that shape, with its negative counterpart — a column that does bake in `dataItem` — used only if we decide to pin the failure mode rather than the contract.



</details>

---

## 4. Column sizing, layout, resize, and frozen columns

<details>
<summary>Show 31 items &mdash; 31 written, 29 passing, 2 failing (product bugs)</summary>

File: `TableView_Sizing_APITests.cs`. **31 written, 29 passing, 2 failing (both product bugs).**

### Comparison against PR `!15971489`

Of our 31 tests, **4 have a PR equivalent and 27 are new.**

| Ours | PR equivalent |
|---|---|
| `VerifyActualWidthFollowsPixelWidth` | `TableViewColumn_ActualWidthFollowsWidth` |
| `VerifyActualWidthClampsToMinWidth` | `TableViewColumn_ActualWidthClampsToMinWidth` + `_ActualWidthFollowsMinWidthChange` |
| `VerifyActualWidthClampsToMaxWidth` | `TableViewColumn_ActualWidthClampsToMaxWidth` + `_ActualWidthFollowsMaxWidthChange` |
| `VerifyInvertedMinMaxResolvesToMinWidth` | `TableViewColumn_PathologicalMinGreaterThanMaxFavoursMin` |

The PR repeats those same four assertions again as interaction tests (`SetFirstColumnWidth_ActualWidthMatches`, `SetFirstColumnWidthBelowMin_ActualWidthClampsToMin`, `SetFirstColumnMaxWidthBelowWidth_ActualWidthClampsToMax`, `ResetColumnWidth_ReadoutReflectsDefaults`) driven through TestUI buttons. Nothing in them needs real input, so they are covered here at API level and are not planned as interaction tests.

**What is new here, and why the PR has no equivalent:**

| Area | New tests | Why the PR has nothing |
|---|---|---|
| Star sizing | 4 | The PR has no star-width test of any kind. |
| `GridLength.Auto` resolution | 3 | The PR only tests `AutoSizeColumn`, a separate on-demand API. It never tests Auto as a width mode. |
| Frozen columns | 5 | No `FrozenEdge` coverage at all. |
| Resize through TableView | 11 | The PR tests `ColumnResizeGripper` as a standalone primitive (12 API + 2 interaction tests) and never tests the integration — that a drag actually changes the owning column, that cancel restores the authored `GridLength`, that a second concurrent drag is rejected, or that resizing converts Auto/Star to Pixel. |
| Layout invalidation | 2 | None. |
| Non-finite `MinWidth`/`MaxWidth` | 1 | The PR's pathological test covers only `Min > Max`, not the non-finite inputs — which is where the live bug is. |
| Provisional width for unresolved Auto/Star | 1 | None. |

**PR tests in this area deliberately not brought across:**

- `TableViewColumn_WidthDefaultsTo120`, `_ResizeDefaults`, `_CanResizeRoundtrip` — defaults and roundtrips, already covered in categories 1 and 3. Not duplicated here.
- `_ActualWidthFollowsMinWidthChange`, `_ActualWidthFollowsMaxWidthChange` — folded into the clamp tests, which set the bound *after* `Width`, so the live-change path is the path under test. Separate tests would assert the same code twice.
- `P212_AutoSizeColumn_NoOpWhenNoRowsRealized`, `_FitsRealizedCellContent`, `_RespectsMaxWidthClamp`, `P212_AutoSizeAllColumns_HitsEveryColumn` — `AutoSizeColumn`/`AutoSizeAllColumns` do not exist in this repo's IDL.
- The 12 `ColumnResizeGripper` API tests and 2 interaction tests — that is a public primitive with `BeginResize`/`TryResize`/`EndResize` and its own clamped `Value`. Here `ResizeGripper` is internal with a different surface, so the coverage is expressed through TableView instead.

### Sticky header — classification decided, stays in §11

The PR has three sticky-header tests as **API** tests: `TableView_StickyHeader_TemplatePartsAreNamed`,
`TableView_StickyHeader_HeaderTracksBodyHorizontalOffset`, and
`TableView_StickyHeader_HeaderDoesNotMoveOnVerticalScroll`.

Our plan carries two of these in §11 as **interaction** tests
(`HorizontalScrollKeepsHeaderAligned`, `VerticalScrollKeepsHeaderSticky`).

**Decided: they stay in §11.** They *could* be written as API tests — §4.6 already scrolls
`PART_BodyScroller` programmatically and reads header cell positions, which is the same technique —
but a programmatic `ChangeView` is not what a sticky header has to survive. The behavior worth
guarding is the header holding position under real scrolling: pointer wheel, drag, touch inertia,
and the composition-driven offsets those produce, none of which a `ChangeView(disableAnimation: true)`
reproduces. An API version would assert the easy half and give false confidence about the half that
actually breaks.

Do not re-file these into §4. `TemplatePartsAreNamed` is a separate matter and is already covered by
the category 1 template-part checks.

### 4.1 Width property surface — **dissolved, see remarks**

**Classification: API tests** (no visual tree needed — `Width`, `MinWidth`, and `MaxWidth` each call `TableViewColumn::UpdateActualWidth` from `OnPropertyChanged` with no owner and no load, `TableViewColumn.cpp:229-238`).

**Step 2 outcome: this subcategory does not survive the redundancy check.** `VerifyColumnDependencyPropertyBacking` (§1.3, passing) already drives a full CLR-property *and* `DependencyProperty` get/set roundtrip over `Width` (`GridLength(200)` → `GridLength(300)`), `MinWidth` (`30` → `40`), and `MaxWidth` (`400` → `500`). Three of the five items restated it outright.

- `(dropped)` `VerifyColumnWidthRoundtripsPixelValue` — **step 2.** Exactly `VerifyColumnDependencyPropertyBacking`'s `WidthProperty` case; `GridLength(200)` *is* a pixel `GridLength`.
- `(dropped)` `VerifyColumnMinWidthRoundtrip` — **step 2.** Exactly its `MinWidthProperty` case.
- `(dropped)` `VerifyColumnMaxWidthRoundtrip` — **step 2.** Exactly its `MaxWidthProperty` case.
- `(dropped)` `VerifyColumnWidthRoundtripsAuto`, `VerifyColumnWidthRoundtripsStar` — **step 2.3, don't test the platform.** A `GridLength` surviving a trip through a `DependencyProperty` is WinRT struct boxing, not TableView behavior. What *is* TableView behavior is what `ActualWidth` does while the unit type is non-Pixel, so the pair is merged and relocated to 4.2 as `VerifyActualWidthIsProvisionalForAutoAndStar`, which asserts something the control actually owns.

**Net: 5 items → 0 here, 1 relocated to 4.2.**

**Remarks:** Worth stating why the roundtrip items looked reasonable when the plan was written: they were drafted before §1.3 existed. They are dropped on redundancy, not because the surface is unimportant — if `VerifyColumnDependencyPropertyBacking` is ever narrowed to stop covering the three width DPs, these must come back.

Also settled here, because it invalidates a later item: **`GridLength` cannot hold a non-finite or negative value at all.** `GridLengthFactory::FromPixels` (`dxaml\xcp\dxaml\lib\GridLength_Partial.cpp:79-84`) and the primitive constructor (`dxaml\xcp\components\primitiveDependencyObjects\GridLength.cpp:20-29`) both fail `E_INVALIDARG` for `NaN`, infinity, and negatives. So the planned §4.2 item `VerifyNonFiniteWidthIsIgnoredOrClamped` is **not writable against `Width`** — the platform rejects the input before TableView sees it, and asserting that is testing XAML. `MinWidth`/`MaxWidth` are plain `Double` DPs with no validation callback, so they *can* take `NaN`/infinity, and that is where the real, TableView-owned question lives. Item is rewritten accordingly in 4.2.

### 4.2 ActualWidth resolution

**Classification: all API tests, and all but one need no visual tree.** `Width`, `MinWidth`, and `MaxWidth` share one branch of `TableViewColumn::OnPropertyChanged` (`TableViewColumn.cpp:229-238`) that calls `UpdateActualWidth()` synchronously. A bare `new TableViewTextColumn()` resolves `ActualWidth` with no owner, no template, and no layout pass — so these tests are deterministic and free of layout timing.

**Spec basis.** `TableView.idl:121-123` is unusually explicit: "Width carries GridLength intent; ActualWidth is the resolved, MinWidth/MaxWidth-clamped pixels… Pixel = exact; Auto = widest realized cell…; Star = a proportional share of the body viewport after fixed columns." `TableView.idl:142-144` adds that `ActualWidth` is "a read-only DP written by the column's width resolve".

- [x] `VerifyActualWidthFollowsPixelWidth`
  - **Description:** Set a pixel `Width` on an unattached column and read `ActualWidth`.
  - **Expected result:** `ActualWidth` equals the pixel value exactly, with no owner and no layout pass.
  - **Failure means:** "Pixel = exact" is not honoured, or `ActualWidth` resolution was made to depend on being loaded — which would leave every unattached column reporting a stale 120.
  - **Remarks:** Grounded directly in `TableView.idl:121-123`. Deliberately unattached: proving resolution does *not* require a visual tree is part of the contract, since `ResolveColumnWidths` only runs for owned, visible columns.

- [x] `VerifyActualWidthClampsToMinWidth`
  - **Description:** Drive the lower clamp by both available routes in one test — set `Width` below `MinWidth`, then separately raise `MinWidth` above an existing `Width`.
  - **Expected result:** `ActualWidth == MinWidth` in both cases; `Width` itself is unchanged and still reports the authored value.
  - **Failure means:** The clamp is missing or is applied to `Width` instead of `ActualWidth`, so a column can render narrower than its stated minimum, or the author's intent is silently overwritten.
  - **Remarks:** The two routes were separate plan items (`VerifyActualWidthFollowsMinWidthChange`) but enter the *same* `if` in `OnPropertyChanged`, so step 2.4's "can break one route only" exception does not apply and they are merged. Asserting `Width` is untouched is the part that matters: `Width` is intent, `ActualWidth` is resolution, and conflating them is the likely regression.

- [x] `VerifyActualWidthClampsToMaxWidth`
  - **Description:** The upper-bound mirror: set `Width` above `MaxWidth`, then separately lower `MaxWidth` below an existing `Width`.
  - **Expected result:** `ActualWidth == MaxWidth` in both cases; `Width` unchanged.
  - **Failure means:** As above, at the upper bound.
  - **Remarks:** Absorbs `VerifyActualWidthFollowsMaxWidthChange` for the same reason.

- [x] `VerifyInvertedMinMaxResolvesToMinWidth`
  - **Description:** Set `MaxWidth` below `MinWidth` and read `ActualWidth`.
  - **Expected result:** `ActualWidth == MinWidth`. `MinWidth` wins the contradiction.
  - **Failure means:** The clamp was written as `std::clamp(v, MinWidth(), MaxWidth())` without the guard, which is **undefined behavior** when `lo > hi` — not merely a wrong number.
  - **Remarks:** Renamed from `VerifyMinWidthGreaterThanMaxWidthIsDeterministic`, which asserted only "deterministic" — untestable as written, because any single observation looks deterministic. The concrete value is recoverable from a *stated invariant* rather than from behavior: `UpdateActualWidth` says "Keep std::clamp well-defined even when MinWidth exceeds MaxWidth" and uses `hi = std::max(lo, MaxWidth())`, making `MinWidth` the result by construction. The IDL does not state which bound wins, so this is a **spec gap** — the value is right but undocumented, and it should be written into the IDL.

- [!] `VerifyNonFiniteMinMaxWidthDoesNotCorruptActualWidth`
  - **Description:** Assign `double.NaN` and `double.PositiveInfinity` to `MinWidth` and to `MaxWidth`, and read `ActualWidth` after each.
  - **Expected result:** `ActualWidth` stays a finite number and the authored pixel `Width` is not lost. `NaN` must never reach `ActualWidth`.
  - **Failure means:** A `NaN` bound poisons the resolved width and propagates into every cell panel's arrange, which produces an invisible or infinitely-wide column rather than a clean rejection.
  - **Remarks:** **Rewritten from `VerifyNonFiniteWidthIsIgnoredOrClamped`, which was not writable.** `GridLength` refuses `NaN`, infinity, and negatives at construction (`GridLength_Partial.cpp:79-84`; `primitiveDependencyObjects\GridLength.cpp:20-29`), so `Width` can never carry a non-finite value and asserting that tests XAML, not TableView. `MinWidth`/`MaxWidth` are plain `Double` DPs with **no** `MUX_PROPERTY_VALIDATION_CALLBACK`, so they accept anything — that is the reachable, TableView-owned hazard.
    This is also a **product inconsistency worth raising regardless of the test result.** The resize path guards explicitly — `const double lo = (std::isfinite(col.MinWidth()) && col.MinWidth() >= 0.0) ? col.MinWidth() : 0.0;` (`TableView.cpp:2063-2067`) — while `UpdateActualWidth` (`TableViewColumn.cpp:427-430`) does not guard at all. Two clamp sites, two different rules for the same pathological input. Expect this test to expose that asymmetry; per step 5 the divergence is the product's problem to resolve, not the test's.
  - **FAILING — confirmed product bug.** Run of `2026-09-15` produced:

    | Assignment | Resulting `ActualWidth` |
    |---|---|
    | `MinWidth = NaN` | `150` — authored width preserved, fine |
    | **`MinWidth = +infinity`** | **`∞`** |
    | `MaxWidth = NaN` | `20` — falls to the `MinWidth` default, fine |
    | `MaxWidth = -infinity` | `20` — inverted-bounds guard catches it, fine |

    Exactly one case escapes: an infinite `MinWidth` becomes the clamp's `lo`, and since `hi = std::max(lo, MaxWidth())` is also infinite, the clamp returns infinity. `ActualWidth` is what the header host and every realized row panel arrange each cell to, so this is not a cosmetic wrong number — it propagates an infinite width into layout. The authored `Width` does survive in all four cases, so the damage is confined to the resolved value.

    **Fix direction (product, not test):** apply the same guard `UpdateActualWidth` is missing and the resize path already has. Doing it in one shared helper would also remove the standing inconsistency between the two clamp sites. Leave the test failing until then.

- [x] `VerifyActualWidthIsProvisionalForAutoAndStar`
  - **Description:** Set `Width` to `GridLength.Auto` and to a star `GridLength` on an unattached column; read `ActualWidth` and `Width` back.
  - **Expected result:** `Width` reports the unit type that was set (`Auto` / `Star`), and `ActualWidth` is the provisional `c_widthDefault` of `120` — not `0`, not `NaN`, not infinity.
  - **Failure means:** Either the control coerces a non-Pixel `Width` back to Pixel (destroying authored sizing intent), or an unresolved Auto/Star column reports a degenerate `ActualWidth` that the header host and every row panel would then arrange to.
  - **Remarks:** **Relocated from 4.1**, where it was two items asserting only `GridLength` roundtrip — that part is WinRT struct boxing, not TableView. The testable TableView behavior is the provisional value: `UpdateActualWidth` substitutes `c_widthDefault.Value` for any non-Pixel unit type (`TableViewColumn.cpp:421-425`) because real Auto/Star resolution needs measured content or a viewport, neither of which exists yet. The IDL documents what Auto and Star *mean* but is silent on the provisional value, so the `120` is a **spec gap** — grounded in `MUX_DEFAULT_VALUE("120.0")` on `ActualWidth`, which at least makes it consistent with the declared default.

### 4.3 Auto and star sizing

**Classification: all API tests**, but unlike 4.2 these require a loaded, laid-out control with a finite viewport — `ResolveColumnWidths` only runs from `TableView::MeasureOverride`, only for columns this TableView owns and that are `Visible`, and Star needs `PART_BodyScroller.ViewportWidth()`.

> #### ✅ Resolved spec contradiction — Auto is shrink-capable
>
> The two authoritative sources disagreed about a documented behavior. **Decided: Auto is shrink-capable**, and `VerifyAutoWidthGrowsAsWiderRowsRealize` is written to that.
>
> - **`TableView.idl:122`** — "Auto = widest realized cell (**grows within a data set**)". Reads as monotonic: once wide, stays wide until the data set changes. **This wording is stale and should be corrected.**
> - **`TableView_Layout.cpp:26-28`** — "Auto is **shrink-capable**: each measure pass re-derives the width from the currently pulled measured max… so a column narrows when its widest content shrinks (CGrid parity) **rather than latching a grow-only maximum**." This is the decided behavior.
>
> The implementation already matches the second: `const double desired = pulledMeasuredMax;` (`TableView_Layout.cpp:196`) ignores any previous value. Measured 60 → 355 → 60.
>
> **Cleanup owed alongside the IDL fix.** The file contradicts *itself*: `ResetColumnDesiredWidths` is still described as resetting "the monotonic Auto desired widths" and a "grow-only accumulator" (`TableView_Layout.cpp:70-84`), and `TableView.h:837-839` repeats it. `TableViewColumn::m_desiredWidth` is written by `SetDesiredWidthInternal`/`ResetDesiredWidthInternal` but its getter `DesiredWidthInternal()` (`TableViewColumn.h:81`) **is never called anywhere in the repo** — it is dead state, and the only remaining effect of `ResetColumnDesiredWidths` is its `InvalidateMeasure()`.

- [x] `VerifyAutoWidthFitsWidestRealizedCell`
  - **Description:** An Auto column over a source whose rows have clearly different text lengths, all realized.
  - **Expected result:** `ActualWidth` accommodates the widest realized cell and is strictly greater than what the narrowest cell alone would need.
  - **Failure means:** Auto sizing ignores measured cell content, so Auto silently behaves like the 120px default and wide content clips.
  - **Remarks:** Unaffected by the contradiction above — both readings agree Auto fits the widest *currently realized* cell. Assert a relative relationship, never an absolute pixel value: the resolved width is layout-rounded against `XamlRoot.RasterizationScale()` (`TableView_Layout.cpp:104-113`), so exact numbers are DPI-dependent and would be flaky.

- [x] `VerifyAutoWidthAccountsForHeaderWidth`
  - **Description:** An Auto column whose header text is much longer than any cell value.
  - **Expected result:** `ActualWidth` is at least the header's measured width — the header participates in Auto sizing.
  - **Failure means:** Only row cells are measured, so a long header clips in a column sized to short data.
  - **Remarks:** Also covers a real conditional: the header contributes **only when headers are shown**, guarded by `ShouldShowColumnHeaders()` (`TableView_Layout.cpp:186-191`). Consider extending this test with `HeadersVisibility.None` to pin that the header then stops contributing — the comment states the cache is stale when headers are hidden, which makes it a correctness rule, not an optimization.

- [x] `VerifyAutoWidthGrowsAsWiderRowsRealize`
  - **Description:** Scroll a long source so a much wider row realizes, then scroll it back out of realization.
  - **Expected result:** The column widens while the wide row is realized, and **narrows again** once that row is recycled. Auto tracks the widest *currently realized* cell in both directions rather than latching a high-water mark.
  - **Failure means:** Either Auto does not respond to newly realized content at all (wide cells clip), or it latches its maximum for the life of the data set — so one wide row anywhere in a long source permanently steals width from every other column even while it is nowhere near the viewport.
  - **Remarks:** **Decision taken: Auto is shrink-capable.** This resolves the contradiction recorded below. `TableView.idl:122`'s "grows within a data set" reads as monotonic and is **stale wording that should be corrected** — it is the only remaining statement of the abandoned grow-only policy, alongside the comments on `ResetColumnDesiredWidths` (`TableView_Layout.cpp:70-84`, `TableView.h:837-839`) and the dead `TableViewColumn::m_desiredWidth` accumulator, whose getter `DesiredWidthInternal()` is never called anywhere in the repo. All of that is unfinished cleanup and should follow the IDL fix.
    Measured: **60 → 355 → 60** with the wide row scrolled in and back out.
    The two readings are indistinguishable on the way up, so the test travels in both directions; that is what gives it its value. Both directions assert a realization precondition first, because a scroll that failed to realize (or failed to recycle) the wide row would make the corresponding assertion vacuously true.
    The final assertion is an **upper bound, not an equality**, and that is a correctness point rather than a tolerance fudge: Auto tracks the widest *currently realized* cell, and the realized set after scrolling back is not guaranteed to be identical to the initial one, so a slightly narrower result is correct. An equality assertion passed in isolation (60/60) and failed in a full-suite run (55 vs 60) purely from realized-set variance. What the contract actually forbids is the column staying wide, or drifting wider.

- [x] `VerifyStarWidthDividesRemainingViewport`
  - **Description:** One fixed pixel column plus two Star columns with different weights, in a host with a known finite width.
  - **Expected result:** The Star columns split the viewport left after the fixed column **in proportion to their weights** — a `2*` column is about twice a `1*` column, and the visible columns together fill the viewport.
  - **Failure means:** Star degenerates to equal shares or to the 120px provisional default, so proportional column layout does not work at all.
  - **Remarks:** Assert the *ratio* and a tolerance, not exact pixels — the viewport basis is layout-rounded before distribution (`TableView_Layout.cpp:241`) and each width is rounded again. Must run against a host with a real finite width; see the unbounded case below.

- [x] `VerifyStarWidthClampsAtBoundAndRedistributes`
  - **Description:** Two Star columns where one has a `MinWidth` (or `MaxWidth`) that its proportional share would violate.
  - **Expected result:** The constrained column is fixed at its bound, and the remaining Star columns re-divide the space that is left — not a uniform proportional squeeze that ignores the bound.
  - **Failure means:** Bounds are applied as a final clamp after distribution, so the columns no longer fill the viewport exactly and a gap or an overflow appears.
  - **Remarks:** **New item — the plan had no coverage of the clamp-and-redivide loop at all**, which is the most intricate logic in the whole sizing engine (`TableView_Layout.cpp:245-273`, an iterative drop-and-redistribute explicitly modeled on WPF's `ComputeStarColumnWidths`). The IDL says only "a proportional share of the body viewport after fixed columns" and does not describe bound interaction, so the redistribution behavior is a **spec gap**; the expectation here is the WPF/CGrid convention the code names as its model.

- [x] `VerifyZeroStarColumnTakesNoWidthUnlessMinWidthSet`
  - **Description:** A `0*` column, first with the default `MinWidth`, then with an explicitly-set `MinWidth`.
  - **Expected result:** With the default, the column resolves to `0`. With an explicit `MinWidth`, it resolves to that `MinWidth`.
  - **Failure means:** `0*` either takes space it should not, or an explicit `MinWidth` on a `0*` column is ignored.
  - **Remarks:** **New item.** Behavior is deliberate and subtle: `MinWidthForStarFactor` (`TableView_Layout.cpp:37-48`) distinguishes the *default* `MinWidth` from an **explicitly set** one by probing `ReadLocalValue(...) == DependencyProperty.UnsetValue`, stating "WPF gives 0* a zero share. Preserve that for the default MinWidth, but still honor an explicitly-set MinWidth." Local-value-vs-default is invisible through the ordinary property getter, so nothing else in the suite can catch a regression here. Entirely undocumented in the IDL — **spec gap**.

- [x] `VerifyStarWidthInUnboundedHostFallsBackDeterministically`
  - **Description:** Host the TableView where its width is unconstrained (a width-to-content parent), with star columns.
  - **Expected result:** Star columns hold finite, positive widths; nothing is arranged at an infinite width and the table's own width stays finite.
  - **Failure means:** An unbounded host produces a degenerate or infinite column width — a hard layout failure rather than a graceful fallback.
  - **Remarks:** **The stated mechanism turned out not to be observable, and the item was narrowed accordingly during implementation.** The code describes the fallback as "leave Star columns at their provisional width" when the viewport is non-finite or unknown (`TableView_Layout.cpp:229-232`). But `PART_BodyScroller` is an ordinary `ScrollViewer`, and once arranged it always reports a **finite** `ViewportWidth`. Measured with `1*` and `3*` columns in a horizontal `StackPanel`: `viewport=240` (the two provisional widths, which the table adopted as its own desired width), and the star division then ran **normally** against it — `1* → 60`, `3* → 180`. The guarded early-return never fired.
    So the branch guarded by `!(viewport > 0.0) || isinf(viewport)` is effectively only the *pre-layout, not-yet-measured* case, not the width-to-content case its comment describes. The first draft of this test asserted the provisional `120` and would have **passed for the wrong reason** with a single `1*` column, because dividing a 120px viewport among one star column also yields 120; the two-column form is what exposed it.
    Two follow-ups, neither blocking: the comment should be corrected to describe the case it actually covers, and it is worth deciding whether a width-to-content host *should* self-size to the provisional widths and divide them (current behavior, deterministic and finite) or is merely falling into that by accident.

- `(dropped)` `VerifyStarWidthRespectsFixedColumnOverflow` — **step 2.** Fixed columns wider than the viewport drive `available` to `std::max(0.0, viewport - fixedTotal)` = `0`, after which every Star column clamps to its `MinWidth`. That is the same clamp path `VerifyStarWidthClampsAtBoundAndRedistributes` already exercises, reached by a different arithmetic route rather than different code.

### 4.4 Layout invalidation

**Classification: API tests.** All three planned items observe `ActualWidth` or arranged size after a property change, which needs a loaded control but no input.

- [x] `VerifyHeaderChangeRemeasuresAutoColumn`
  - **Description:** Replace a loaded Auto column's `Header` with much longer text.
  - **Expected result:** The column's `ActualWidth` grows to fit the new header without any further interaction.
  - **Failure means:** The `Header` property-changed callback rebuilds the header visual but does not re-run the width resolve, so the new header clips until something else invalidates measure.
  - **Remarks:** Distinct from §4.3's `VerifyAutoWidthAccountsForHeaderWidth` under the step 2.4 exception: that one proves the header participates in the *initial* resolve, this one proves the *change notification* path re-runs it. They fail independently — `TableViewColumn.cpp:254-258` routes `Header`/`HeaderTemplate`/`HeaderTemplateSelector` through a separate branch whose stated job is "Re-render headers and recompute Auto widths after header content changes".

- [x] `VerifyCellTemplateChangeRemeasuresAutoColumn`
  - **Description:** Replace a loaded `TableViewTemplateColumn.CellTemplate` with one whose content is materially wider.
  - **Expected result:** The Auto column's `ActualWidth` grows to fit the new cell content.
  - **Failure means:** `NotifyCellContentChanged` regenerates cells without re-resolving widths, so app-driven template swaps clip.
  - **Remarks:** Exercises a third, separate route (`TableViewColumn.cpp:249-252`). Worth pairing with the shrink direction — a *narrower* replacement template should narrow the column — but only once the 4.3 contradiction is resolved, since that is precisely the disputed behavior.

- `(dropped)` `VerifyDensityChangeRemeasuresRowsAndHeaders` — **step 2.** §13 already owns "Density changes row/header min height and padding for Compact/Standard/Comfortable", and the observable is identical: read row/header height after setting `Density`. Framing it here as "invalidates measure" and there as "changes height" describes one assertion twice. Kept in §13, which is where the density resource keys (`DensitySuffix`, `DensityRowMinHeightFallback`, `DensityCellPaddingFallback`) are already under test.

**Remarks on this subcategory.** It is thinner than it looks, because the invalidation machinery is largely self-proving: `RequestColumnWidthResolve` calls `InvalidateMeasure()` **synchronously** from a descendant panel's measure, and the header comment explains it "converges without a debounce" within the same layout tick (`TableView_Layout.cpp:57-70`). There is no queued/async state to catch mid-flight, so there is nothing to test beyond "the observable width ended up right", which each item above already asserts. Resist adding tests that reach for `InvalidateMeasure` call counts — that tests the implementation, not the contract.

### 4.5 Resize gating and gesture

> #### Classification verdict: **9 of 10 are valid API tests; exactly 1 is not.**
>
> `ResizeGripper` (`controls\dev\ResizeGripper\ResizeGripper.idl`) exposes the entire gesture programmatically — `BeginDrag()`, `TryDrag(totalDelta)`, `EndDrag(canceled)`, `TryKeyboardStep(key)` — and states the intent outright: "Raise the same events a pointer drag would, **so a keyboard step is indistinguishable from the gesture**." No pointer input is required to drive a resize end to end.
>
> **Reachability from C# is confirmed, not assumed.** `ResizeGripper` is `[MUX_INTERNAL]` in `MU_PRIVATE_CONTROLS_NAMESPACE` = `Microsoft.UI.Private.Controls`, and is deliberately stripped from the *merged* Tabular winmd. But `MUXControlsTestApp.csproj:186-188` takes its `CsWinRTInputs`/`Reference` from `dll-tabular\Unmerged\Microsoft.UI.Xaml.Controls.Tabular.g.winmd`, which **does** contain `ResizeGripper`, `BeginDrag`, `TryDrag`, `EndDrag`, `TryKeyboardStep`, `KeyboardIncrement`, `DragOrientation`, and the two event-args classes. Repo precedent for testing an interaction primitive this way is exact: `Interactions\ButtonInteraction\APITests` and `Interactions\SliderInteraction\APITests` both `using Microsoft.UI.Private.Controls`.
>
> The gripper is reachable from a test via `TableView::FindResizeGripperInCell` semantics — it is appended directly to the header cell's `Children` (`TableView.cpp:2121`), so a visual-tree walk for `ResizeGripper` finds it.
>
> **The single exception is the Shift multiplier**, detailed in its item below.

**Anchoring contract** (`TableView::AppendResizeGripperVisual`, `TableView.cpp:1975-2122`): `DragStarted` captures `startValue = column.ActualWidth()` and `startWidth = column.Width()`; `DragDelta` writes `column.Width(GridLengthHelper::FromPixels(clamp(startValue + TotalDelta, lo, hi)))`; `DragCompleted` reverts to `startWidth` when canceled. The column owns the width; the gripper owns no value and no bounds.

- [x] `VerifyCanUserResizeColumnsFalseHidesGripper`
  - **Description:** Load a TableView with `CanUserResizeColumns = false` and inspect every header cell.
  - **Expected result:** No `ResizeGripper` exists in any header cell, and each header cell reports `IsTabStop == false`.
  - **Failure means:** The master switch does not suppress the affordance, so a resize can still be started on a table that opted out.
  - **Remarks:** The `IsTabStop` half is not incidental — `headerIsResizable` drives gripper creation, `IsTabStop`, and `UseSystemFocusVisuals` from one expression (`TableView.cpp:1560-1562`), with the stated rationale that a non-resizable header "costs a Tab press for nothing". Asserting both pins the tab-order consequence, which is otherwise only caught by a keyboard interaction test.

- [x] `VerifyPerColumnCanResizeFalseHidesGripper`
  - **Description:** One column with `CanResize = false` among resizable siblings.
  - **Expected result:** That column's header has no gripper and is not a tab stop; the sibling columns still have theirs.
  - **Failure means:** The per-column opt-out is ignored, or it is applied table-wide.
  - **Remarks:** Grounded in `TableViewColumn.idl:136` — "Gates the resize affordance for this column only; the owner's `CanUserResizeColumns` gates all of them." Note both properties gate the **affordance** only; neither is documented to block a programmatic `Width` assignment, so do **not** add a test asserting that `CanResize = false` prevents setting `Width`.

- [x] `VerifyResizeDragUpdatesColumnWidth`
  - **Description:** `BeginDrag()`, `TryDrag(+delta)`, `EndDrag(false)` on a column's gripper.
  - **Expected result:** `Width` becomes a **Pixel** `GridLength` equal to the starting `ActualWidth` plus the delta, and `ActualWidth` follows.
  - **Failure means:** The drag does not reach the column, or the delta is applied to the wrong anchor — the classic bug being incremental application of each `Delta` instead of absolute `TotalDelta`, which drifts over a gesture.
  - **Remarks:** `TotalDelta` is measured from where the drag began (`ResizeGripper.idl`), and the handler applies it to the captured `startValue`, not to the current width. A test that steps `TryDrag` several times within one `BeginDrag`/`EndDrag` pair and asserts the final width against the *original* anchor is what catches drift; a single-step drag would pass either way.

- [x] `VerifyResizeDragRespectsMinWidth`
  - **Description:** Drag far narrower than `MinWidth`.
  - **Expected result:** `Width` stops at exactly `MinWidth` and does not go below.
  - **Failure means:** A drag can size a column below its stated minimum, which the layout resolve would then clamp anyway — producing a `Width`/`ActualWidth` disagreement the user can see as a stuck gripper.

- [x] `VerifyResizeDragRespectsMaxWidth`
  - **Description:** Drag far wider than `MaxWidth`.
  - **Expected result:** `Width` stops at exactly `MaxWidth`.
  - **Failure means:** As above, at the upper bound.
  - **Remarks:** Kept separate from the `MinWidth` case under step 2.4 — `lo` and `hi` are computed by different expressions (`hi` additionally applies `std::max(lo, ...)`), so one can regress alone.

- [x] `VerifyResizeCancelRestoresAuthoredWidth`
  - **Description:** `BeginDrag()`, `TryDrag(delta)`, then `EndDrag(true /* canceled */)`.
  - **Expected result:** `Width` is restored to the exact `GridLength` authored before the gesture — including its original `GridUnitType`, so an `Auto` column returns to `Auto`, not to a pixel value.
  - **Failure means:** Cancelling leaves the mid-drag pixel width behind, permanently converting an Auto/Star column to Pixel because the user pressed Escape.
  - **Remarks:** The unit-type half is the whole point and is easy to miss: the handler restores `state->startWidth`, which is the captured `GridLength`, not `startValue`, which is the resolved pixel number.

- [x] `VerifyResizePressWithoutMovementDoesNotPinAutoColumn`
  - **Description:** `BeginDrag()` then `EndDrag(true)` on an **Auto** column with no intervening `TryDrag`.
  - **Expected result:** `Width` is still `GridUnitType.Auto` — untouched. Nothing was written, so nothing is reverted or pinned.
  - **Failure means:** Merely touching the gripper converts an Auto column to a fixed pixel width, silently destroying auto-sizing on a stray click.
  - **Remarks:** **New item — the plan had no coverage of this.** The guard is explicit and its rationale is stated: the revert runs "Only when a write actually happened, so a press that never moved cannot pin an Auto/Star column" (`TableView.cpp:2098-2103`), backed by `state->didWrite`. A cancel-path test that starts from a Pixel column cannot detect a regression here, because reverting a pixel width to itself is invisible.

- [x] `VerifyConcurrentResizeDragIsRejected`
  - **Description:** Begin a drag on column A's gripper, then call `BeginDrag()` on column B's gripper while A's is still in flight.
  - **Expected result:** The second gesture is refused — B's gripper ends immediately as canceled, B's `Width` is unchanged, and A remains the single active drag.
  - **Failure means:** Two concurrent resizes run at once. The stated consequence is worse than a double-write: the control tracks exactly one `m_activeColumnResizeDrag`, so Escape could no longer reach the orphaned gesture.
  - **Remarks:** **New item.** Contract stated at `TableView.cpp:2020-2038`: "One resize at a time. Manipulation arbitrates per element, so a second contact on a DIFFERENT gripper would otherwise run a concurrent drag that Escape could not reach." Reachable only programmatically — with real input this needs genuine multi-touch, so an API test covers a case an interaction test realistically cannot.

- [x] `VerifyResizeConvertsStarOrAutoToPixelWidth`
  - **Description:** Complete a real (moved, then released) drag on an Auto column and on a Star column.
  - **Expected result:** Both end with `Width.GridUnitType == Pixel` at the dragged value; the column no longer participates in Auto or Star sizing.
  - **Failure means:** A resized Auto/Star column snaps back on the next measure pass, so the drag appears not to hold.
  - **Remarks:** Behavior follows from `GridLengthHelper::FromPixels` in the `DragDelta` handler, but the IDL never states that resizing changes the sizing *mode* — a real **spec gap**, and a user-visible one, since it silently opts the column out of the proportional layout the app author chose.

- [x] `VerifyKeyboardStepAdjustsWidthAndMirrorsInRightToLeft`
  - **Description:** Call `TryKeyboardStep(VirtualKey.Left)` and `(Right)` on a gripper, under `FlowDirection.LeftToRight` and again under `RightToLeft`.
  - **Expected result:** One step changes `Width` by exactly `KeyboardIncrement` in the direction the arrow points **visually** — so under RTL, `Left` still shrinks. Keys off the drag axis (`Up`/`Down` for a horizontal gripper) return `false` and change nothing.
  - **Failure means:** Keyboard resize moves the wrong way under RTL, or an off-axis key is swallowed and blocks focus navigation.
  - **Remarks:** Merges the plan's `VerifyKeyboardResizeAdjustsWidth` and `VerifyKeyboardResizeMirrorsInRightToLeft` — the mirror is three lines inside the same method (`ResizeGripper.cpp:347-351`), so they cannot regress independently, and the RTL assertion is meaningless without the LTR baseline in the same test. The off-axis `false` return is included because `TryKeyboardStep`'s documented contract is "False when the key is not on this axis." Fully API-testable: `FlowDirection` is a settable property and no physical key is involved.
    **Which visual direction grows under RTL is a spec gap, and the test deliberately does not pin it.** `ResizeGripper.idl` says only "Only horizontal mirrors under RTL". The comment at the mirror claims "Left always shrinks visually", which does not obviously match the code: `Left` starts at `-1`, is negated to `+1` under RTL, and the host adds that to the width — measured as LTR `Left = -16`, RTL `Left = +16`. So under RTL the column *grows* on `Left`, whatever the comment intends by "visually". Rather than freeze either reading, the test asserts the one thing the IDL and the code agree on — the RTL outcome is the opposite of the LTR outcome, at the same magnitude. **Someone should decide what the RTL direction ought to be and write it into the IDL**, after which this test can be tightened to an absolute direction.

- [x] `VerifyKeyboardStepHonorsKeyboardIncrement`
  - **Description:** Set `KeyboardIncrement` to a custom value, then to `0` and to `NaN`, and take one step in each case.
  - **Expected result:** The custom value is used as the step; `0` and `NaN` fall back to the default increment rather than producing a zero-width or `NaN` step.
  - **Failure means:** A host-set step size is ignored, or a degenerate increment reaches the width write and corrupts the column.
  - **Remarks:** **New item, and the API-testable substitute for the Shift multiplier below.** `EffectiveKeyboardIncrement` (`ResizeGripper.cpp:308-316`) sanitizes: non-finite or zero → `c_defaultKeyboardIncrement`, otherwise `max(|value|, c_dragDeadband)`. None of that is in the IDL beyond "Step size is host policy, so it is settable" — a **spec gap**, though the sanitization is defensible on its face.

- `(moved to interaction plan)` `VerifyKeyboardResizeShiftMultiplierAppliesLargerDelta` — **not a valid API test.** The multiplier is selected by `IsShiftKeyDown()`, which reads **physical** keyboard state via `InputKeyboardSource::GetKeyStateForCurrentThread(VirtualKey::Shift)` (`ResizeGripper.cpp:22-26`). `TryKeyboardStep` takes only a `VirtualKey` and offers no way to inject a modifier, so an API test can never exercise the `c_largeIncrementMultiplier` (4.0) branch — it would silently test the unshifted path and pass while proving nothing. This is the one item in §4.5 that genuinely requires real input, and it is the textbook case for step 1's "interaction only when unreachable programmatically". `VerifyKeyboardStepHonorsKeyboardIncrement` covers the step-size arithmetic; only the modifier read is left for §11.

- `(dropped)` `VerifyResizeGripperEscapeCancelsDrag` — **step 2, not proposed but considered and rejected.** `TableView::CancelColumnResizeDrag` just calls `EndDrag(true)`, which `VerifyResizeCancelRestoresAuthoredWidth` already covers. Only the *Escape key routing* is untested, and that is keyboard input — §11, not here.

### 4.6 Frozen columns

**Classification: all API tests.** Pinning is applied as `UIElement.Translation` + `Canvas.ZIndex` + `Clip` on the cell elements (`TableViewCellsPanel::ApplyFrozenColumnLayout`, `TableViewCellsPanel.cpp:275+`), all of which are readable properties. Horizontal scrolling is driven programmatically through `PART_BodyScroller.ChangeView(offset, null, null, true)` — it is a `ScrollViewer`. No pointer input needed.

**Spec basis.** `TableView.idl:4` — "Column pinning edge; None scrolls naturally, Leading pins left, Trailing is reserved." `TableViewColumn.idl:147` — "Pins the column during horizontal scroll; Leading is implemented, Trailing reserved."

- [x] `VerifyLeadingFrozenColumnStaysPinnedDuringHorizontalScroll`
  - **Description:** A `FrozenEdge.Leading` first column; scroll the body horizontally and read the header cell's and row cells' `Translation`.
  - **Expected result:** The frozen column's cells counter-translate by the scroll offset so they hold their viewport position, while non-frozen cells do not translate. The header and the row cells move together.
  - **Failure means:** The pinned column scrolls away with the rest of the table — the feature simply does not work — or header and body pin differently, which tears the column apart visually.
  - **Remarks:** Assert header and rows in the same test: they are pinned by two different call sites (`ApplyFrozenColumnLayout` on the header host vs `TableViewRow::RefreshFrozenColumnLayout` per row, `TableView_Columns.cpp:86-95`) against one shared offset, and the stated reason they share it is "so they stay aligned". Divergence between the two is the failure worth catching.

- [x] `VerifyMultipleLeadingFrozenColumnsStayPinnedInOrder`
  - **Description:** The first three columns all `Leading`; scroll and inspect their pinned positions, then collapse one of them.
  - **Expected result:** All three stay pinned, side by side, in declaration order, with no overlap — all translated by the same scroll offset. Collapsing one must not disturb the pinning of its surviving siblings.
  - **Failure means:** Multiple frozen columns collapse onto each other or reorder, because the band width is computed from the wrong accumulation.
  - **Remarks:** Band width comes from `ComputeLeadingFrozenWidth` summing `ActualWidth` over the contiguous prefix (`TableView_Columns.cpp:43-64`), which does skip non-`Visible` columns.
    **The planned assertion that a collapsed frozen column reserves no band width was dropped during implementation, on redundancy rather than on principle.** It fails — the header cell keeps its 160px width — but for the *same* reason as §3.5's `VerifyCollapsedColumnRemovesCellsFromLayout` and `VerifyRestoringColumnVisibilityRestoresCells`: a column collapsed after the headers are built does not collapse its existing header cell. Keeping it here would have produced a third failure signal for one bug without adding information, and the band-width contribution cannot be observed separately while the cell width itself is wrong. It is tracked in §3.5; **re-add it here once that bug is fixed**, since the band interaction is genuinely frozen-specific.

- [x] `VerifyNonContiguousLeadingFrozenOnlyFreezesPrefix`
  - **Description:** Columns 0 and 2 set to `Leading`, column 1 left `None`; scroll horizontally.
  - **Expected result:** Only column 0 is pinned. Column 2 scrolls normally despite its `Leading` flag.
  - **Failure means:** A stray `Leading` flag in the middle of the collection pins a column over the scrolled content, overlapping cells and corrupting the band.
  - **Remarks:** Stated twice as deliberate — "Only a contiguous leading prefix (from column 0) is frozen. A Leading flag on a non-prefix column is ignored so it cannot corrupt the pinned-band layout" (`TableView_Columns.cpp:48-49`) and again in the panel (`TableViewCellsPanel.cpp:302-304`). The IDL does **not** mention the contiguity rule at all — a **spec gap**, and a surprising one for an app author who sets `Leading` on an arbitrary column and sees it ignored.

- [x] `VerifyTrailingFrozenEdgeIsReservedNoOp`
  - **Description:** Set `FrozenEdge.Trailing` on a column and scroll.
  - **Expected result:** The value roundtrips on the property, and the column behaves exactly like `None` — no pinning, no translation, no clip.
  - **Failure means:** A reserved enum value has accidentally acquired behavior, which would become a compatibility burden the moment `Trailing` is actually implemented.
  - **Remarks:** Both the enum and the property say "Trailing is reserved", and `ComputeLeadingFrozenWidth` `break`s on any value that is not `Leading`, so `Trailing` falls out as non-frozen. This is one of the rare cases where asserting *absence of behavior* is the right test, because the value is public API today.

- [!] `VerifyFrozenColumnsMirrorInRightToLeft`
  - **Description:** A `Leading` frozen column under `FlowDirection.RightToLeft`, scrolled horizontally.
  - **Expected result:** The frozen column **pins to the visual right edge** of the viewport and holds that position while the rest of the row scrolls, exactly mirroring the LTR behavior. The frozen header cell stays aligned with its frozen body cells.
  - **Failure means:** `FrozenEdge` is silently LTR-only. An app that declares a frozen column and then runs in Arabic or Hebrew loses it with no error, no warning, and no way to detect the difference from the API.
  - **Remarks:** **Decision taken: `Leading` is a logical edge and must mirror, pinning to the right under RTL.** Of the three resolutions originally open — mirror it, document it as disabled, or document `FrozenEdge` as LTR-only — the first was chosen, so this is a product bug rather than an undocumented limitation.
    **FAILING — confirmed product bug.** `ApplyFrozenColumnLayout` opens with an RTL branch that clears `Translation`, `ZIndex` and `Clip` on every child and returns, on the stated grounds that "Frozen-column pin math is LTR-only; under RTL it would pin/clip the wrong edge. Skip pinning in RTL so scrolling remains plain and uncorrupted" (`TableViewCellsPanel.cpp:282-298`). Measured: the frozen cell's visual right edge moved `691 → 891` under a 200px scroll — the **same** displacement as its non-frozen neighbour (`531 → 731`). It is not pinned at all; it scrolls like any other column.
    **The stated rationale does not hold, and the fix looks smaller than the comment implies.** XAML applies RTL as one mirroring transform at the element where `FlowDirection` *changes*; inside the subtree children still arrange in ordinary left-to-right logical coordinates, and the scroll displacement measured in that logical space is identical to LTR (the neighbour cell moved by exactly `-offset`). So `Translation.X = horizontalOffset` is already the correct counter-translation under RTL, and the mirror to the visual right edge then falls out of the flip for free. The likely correct change is to delete the early return.
    **Measurement note, worth keeping:** a first draft measured cell positions with `TransformToVisual` against an ancestor *inside* the RTL subtree and got unmirrored logical coordinates, which made a precondition ("the first column is arranged to the right of the second") fail for reasons that had nothing to do with the bug. The test now hosts the RTL table inside an explicitly `LeftToRight` wrapper and measures against that, putting the mirror boundary between the measurement frame and the cells. It also takes the max of both transformed horizontal corners rather than `x + width`, since under a mirror an element's local left edge maps to its visual right.


</details>

---

## 5. Rows, cells, and visual states

<details>
<summary>Show 18 items &mdash; 12 written, 12 passing, 6 dropped or moved to the interaction plan</summary>

File: `TableView_Rows_APITests.cs` (new). **12 written, 11 passing, 1 failing (a product bug).**

### Comparison against PR `!15971489`

**All 12 are new. The PR has no equivalent for any of them.**

The PR's row and cell coverage is three assertions, and all three sit in other categories here:

| PR test | Where it lands here |
|---|---|
| `TableViewRow_IsSelectedDefaultsToFalse` | §1.2 defaults |
| `TableViewRow_GetOwningTableViewDefaultsNull` | §3 — and `(deferred)`, the accessor is not in this repo's IDL |
| `TableViewTextColumn_GenerateElementProducesTextBlock` | §3.4 cell factory |

Its one interaction test in this area, `TemplateColumnRendersCustomContent`, asserts that a template
column renders *something*; §5.2's `VerifyTemplateCellRendersRowItem` asserts it renders the right
item, and found the inert `Content` binding doing it.

**Nothing in the PR covers any of:** the row template contract, `DataContext` inheritance down the
cell path, cell hit-testing, text trimming, recycling correctness for any column type, row banding,
`GridLinesVisibility`, or the `CommonStates` machine. The last of those is where the product bug in
§5.5 was hiding — a disabled TableView that still renders as interactive, which no test in either
suite would previously have caught.

The PR's nearest neighbours are `HighContrastVisualStateBrushKeysAreWired` (a resource-key check that
belongs in §13.2, not a state-machine test) and
`Virtualization_LargeSourceKeepsRealizedRowsBoundedAfterScroll` (§15 — it counts realized rows, and
never checks that their *contents* are correct, which is what §5.3 does).

### 5.1 Row structure

**Classification: all API tests.** Rows realize through `PART_RowsRepeater` under a normal layout
pass; nothing here needs real input.

**Spec basis is unusually strong for this subsection.** `TableView.idl:399-417` carries the row
template contract in full — the four required parts, what each is for, and the `CommonStates` list —
because "MIDL3 does not surface `[TemplatePart]`, so this block is it". Expectations below cite it
directly rather than reasoning from the template.

**Step 2 outcome: two of the four planned items are dropped and one is rewritten.** Category 3's
`VerifyEveryRowMatchesHeaders` helper already asserts, on every realized row, that cell count equals
header cell count *and* that the cell at each index belongs to the same column as the header at that
index. It runs inside several §3 tests. Restating either half here would add run time and no signal.

- [x] `VerifyRowTemplatePartsExist`
  - **Description:** Realize a row and look up all four named parts from the applied template.
  - **Expected result:** `PART_RootBorder`, `PART_CellsHost`, `PART_CellForegroundPresenter` and `PART_SelectionIndicator` all resolve, with the types the contract names — `Border`, `Panel`, `ContentPresenter`, `UIElement` respectively.
  - **Failure means:** The default template has drifted from the documented contract. Because every one of these is a `Storyboard` target for the `Selected*` states, a missing part does not fail at parse — it fails when the state is first applied, which is the moment a user selects a row.
  - **Remarks:** `TableView.idl:402-410` marks all four **REQUIRED** and states the deferred-failure reason outright: "an unresolved Storyboard target fails when the state is applied, not when the template is parsed." That is what makes a structural test worth writing here rather than relying on the visual-state tests in §5.5 to catch it — those only cover the states they exercise, and `SelectedDisabled` is easy to leave untested.

- [ ] `(dropped)` `VerifyRowCellCountMatchesVisibleColumns`
  - **Reason:** Redundant with §3's `VerifyEveryRowMatchesHeaders`, which asserts row cell count against header cell count on every realized row.
  - **Also misstated, and worth correcting so it is not re-proposed:** a row holds one cell per **owned** column, not per *visible* column. `RebuildCells` generates a wrapper for every column the TableView owns and sets `cellWrapper.Visibility(column.Visibility())` (`TableViewRow.cpp:757-760`); a collapsed column keeps its cell and its identity. The behavior this item was reaching for is already §3.5's `VerifyCollapsedColumnRemovesCellsFromLayout`.

- [ ] `(dropped)` `VerifyRowCellOrderMatchesColumnOrder`
  - **Reason:** Fully covered by the per-index column-identity assertion inside `VerifyEveryRowMatchesHeaders`. That helper compares the actual owning column of each cell, not just a count, so ordering cannot regress without it failing.

- [x] `VerifyCellPathInheritsRowDataContext`
  - **Description:** On a realized row, assert the row's `DataContext` is its backing item, then read the **local** value of `DataContextProperty` on the cell wrapper `Border`, on `PART_CellsHost`, and on the generated cell element inside the wrapper.
  - **Expected result:** The row's `DataContext` is the source item. Every element on the cell path returns `DependencyProperty.UnsetValue` for its local `DataContext` — nothing below the row sets one.
  - **Failure means:** Something on the cell path has taken a local `DataContext`, which shadows inheritance. Cells would then keep showing the *previous* item's data after the row is recycled, and nothing in the recycling path would correct it, because the recycle relies on updating the row's `DataContext` and letting inheritance carry it down.
  - **Remarks:** **Replaces the planned `VerifyRowDataContextIsRowItem`**, which on its own only restates a precondition §3's `VerifyProbeCellsMatchRowData` already asserts. The stronger statement is available because the product declares it as an invariant rather than leaving it implicit: "No local DataContext: the cell inherits the row's DataContext once appended… This is a **load-bearing invariant**: nothing on the cell path (wrapper Border, `PART_CellsHost`, or the built-in cell elements) may set a local DataContext, or it would shadow inheritance and the cell would show stale data after recycle" (`TableViewRow.cpp:769-777`). Step 3 admits invariants the product states *as* invariants, which this is.
    This is deliberately a test of the **mechanism**, not the symptom. §5.3 asserts that recycled cells show the new values; this asserts the property that makes that possible. They fail together today, but a future change could satisfy §5.3 by re-pushing data on recycle — which is exactly what the comment says was removed, because pushing layout-affecting data onto live in-tree cells during the repeater's measure pass tripped a re-entrancy assert (`0xc0000420`) on scroll. This test is what stops that regression from being reintroduced silently.

- [x] `VerifyCellWrapperIsHitTestableAcrossItsPadding`
  - **Description:** Hit-test a point inside a cell's padding — inside the wrapper's bounds but outside the generated `TextBlock` — using `VisualTreeHelper.FindElementsInHostCoordinates`, and check the cell wrapper is among the results.
  - **Expected result:** The hit test reaches the cell wrapper for that column. The whole cell rectangle responds, not just the text within it.
  - **Failure means:** Clicks landing in a cell's padding resolve no column. Row selection and double-click-to-edit would then work only over the glyphs themselves and silently do nothing across most of a wide cell — a defect that looks like flaky input rather than a layout bug.
  - **Remarks:** This pins a **fixed bug**, and the fix is a single line with the rationale attached: `cellWrapper.Background(TransparentBrush())`, because "A Border with a null Background does not hit-test, so without this only the generated content itself (a TextBlock, which is as wide as its text) would respond to a press. A click anywhere in the cell's padding resolved no column at all: no current cell, and double-click-to-edit silently did nothing on most of the cell's area" (`TableViewRow.cpp:762-768`).
    Asserted through a real hit test rather than by checking `Background != null`, deliberately: the brush is the current *mechanism*, and a test that asserts the mechanism would have to be rewritten if the fix ever moved to `IsHitTestVisible` or a different element, while a hit-test assertion survives that. Use a point offset into the wrapper's bounds but clear of the text — the cell padding at Standard density gives room, and the test should assert its chosen point really is outside the `TextBlock` bounds so it cannot pass vacuously.

### 5.2 Cell content

**Classification: all API tests.**

**Step 2 outcome: 4 planned → 2 written.** One merges into another; one is already owned by §13.3.

- [x] `VerifyGeneratedTextCellMetrics`
  - **Description:** Realize a text column over a source with one value far wider than its column, and inspect the generated `TextBlock`.
  - **Expected result:** `VerticalAlignment == Center`, `TextTrimming == CharacterEllipsis`, `FontWeight == Normal`. The over-long value is trimmed, not wrapped: the `TextBlock` stays one line high while its text exceeds the column width.
  - **Failure means:** Generated cells do not carry the standard grid look. Concretely, losing `CharacterEllipsis` makes over-long values wrap and grow the row instead of clipping — every row with long text becomes a different height and the table stops looking like a grid.
  - **Remarks:** **Merges the planned `VerifyTextCellIsVerticallyCentered` and `VerifyTextCellTrimsOverflowText`.** Both properties are set side by side in one method with one stated intent — "Cell content is left-aligned and vertically centered within the row (standard grid look)" (`TableViewTextColumn.cpp:33-48`) — so they cannot regress independently and two tests would differ only in the property read.
    Not expressible in the IDL: `GenerateElementCore` is `overridable` and declares a return type, not the metrics of what the built-in implementation returns. The expectation rests on that comment plus the "standard grid look" intent, which is weaker than a `MUX_DEFAULT_VALUE` — recorded here so the next reader knows this pins a deliberate visual decision rather than a contract.

- [ ] `(dropped)` `VerifyTextCellUsesDensityPadding`
  - **Reason:** §13.3 already owns `VerifyDensityAffectsCellPadding`, and the observable is identical — set `Density`, read the generated `TextBlock`'s `Padding`. Asserting here that padding equals the Standard default would additionally hard-code `8,4,8,4`, which is a theme resource (`TableViewCellPadding`) an app is allowed to override.

- [x] `VerifyTemplateCellRendersRowItem`
  - **Description:** Realize a `TableViewTemplateColumn` with a `CellTemplate`, then assert the generated `ContentPresenter` is the cell wrapper's child, that it resolves the row's data item, and that the inflated template renders that item's values.
  - **Expected result:** The presenter is the wrapper `Border`'s `Child`, carries the column's `CellTemplate`, resolves the row's item, and the `TextBlock` inside the inflated template shows that item's `Name`.
  - **Failure means:** A template column renders against the wrong item, or against nothing. Since `CellTemplate` is the entire extensibility story for non-text columns, this would make every custom cell blank or stale.
  - **Remarks:** **Rewritten twice.** The plan first had `VerifyTemplateCellHostsTemplateContent`, which asserted only that the content is a child of the wrapper — true but shallow. That was replaced by `VerifyTemplateCellContentTracksRowItemNotPresenter`, asserting `presenter.Content == item`, on the strength of the product's stated rationale for binding `Content` to the *wrapper's* inherited `DataContext` rather than the presenter's own (`TableViewRow.cpp:841-848`, `TableViewTemplateColumn.cpp:24-27`). **That test failed, and running it produced the finding below.** The expectation was over-specified: it asserted an implementation mechanism the IDL never promises, which is exactly what step 3 forbids. The test now asserts the contract — the template resolves and renders the row's item — and leaves `Content` unasserted.

> #### 🐞 Finding — the template cell's `Content` binding is inert
>
> Measured on a realized template cell: `wrapper.DataContext` = the item, `presenter.DataContext` =
> the item, `ContentTemplate` set, **`Content` = null**, **rendered text = the item's value**.
>
> The cell renders correctly, but not by the mechanism the product says it uses. `AttachCellContent`
> installs a `Binding` with `Source` = the cell wrapper and `Path` = `DataContext`
> (`TableViewRow.cpp:849-858`), and it never produces a value: the binding is set while the wrapper
> is still detached and its `DataContext` is null, and `Content` stays null from then on. What
> actually carries the item to the template is plain `DataContext` inheritance down to the presenter.
>
> **Not user-visible today**, which is why it is recorded here rather than left as a failing test.
> `ContentPresenter` only pins its `DataContext` to its `Content` when `Content` is set, so with
> `Content` permanently null the self-referential freeze the comment warns about cannot occur either.
> Recycling works, by inheritance, and §5.3 covers it.
>
> **It is still a defect worth fixing**, in one of two directions — dead code guarded by a comment
> that misdescribes the live mechanism is a trap for the next change:
> 1. Remove the binding and correct both comments to say inheritance carries the item; or
> 2. Make the binding work, if `Content` is wanted for automation or for app-authored
>    `{TemplateBinding Content}` scenarios.
>
> Whichever is chosen, note that §5.1's `VerifyCellPathInheritsRowDataContext` is what currently
> guards the real mechanism. Do not "fix" recycling by re-pushing `Content` per recycle without
> re-reading that item.

### 5.3 Recycling

**Classification: all API tests.** Recycling is driven by scrolling `PART_BodyScroller`, which §4.6 already does programmatically.

**Step 2 outcome: 5 planned → 3 written.** Category 3 already covers the `DataContext` half of recycling in `VerifyProbeCellsMatchRowData` and `VerifyCustomColumnElementUpdatesOnRecycle`.

- [ ] `(dropped)` `VerifyRecycledRowUpdatesDataContext`
  - **Reason:** §3's `VerifyProbeCellsMatchRowData` asserts exactly this — that each realized row after scrolling carries its own item as `DataContext`. §5.1's `VerifyCellPathInheritsRowDataContext` additionally guards the inheritance that makes it work.

- [x] `VerifyRecycledCellsShowNewItemValues`
  - **Description:** Scroll a long source so rows are recycled, then read the realized cells of both a `TableViewTextColumn` and a `TableViewTemplateColumn` and compare them against the items now at those positions.
  - **Expected result:** Every realized text cell's `Text` and every template cell's `Content` matches the item that row now represents. No cell shows a value belonging to an item from before the scroll.
  - **Failure means:** Recycled rows display stale data — the single most visible virtualization bug there is, and one that only appears after scrolling, so it survives every static test.
  - **Remarks:** **Merges the planned `VerifyRecycledRowUpdatesTextCells` and `VerifyRecycledRowUpdatesTemplateCells` into one table-driven test.** They are kept together rather than dropped as duplicates of §3's custom-column recycle test because the three column types reach their content by genuinely different routes — `TextColumn` through `BindingOperations.SetBinding` on `TextBlock.Text`, `TemplateColumn` through a `Content` binding sourced from the wrapper, and §3's `ProbeColumn` through an app-authored override. A regression in one route would not break the others, which is the step 2 rule 4 justification for keeping all three.

- [x] `VerifyRecycledRowRebandsForItsNewIndex`
  - **Description:** With `RowBackground` and `AlternatingRowBackground` both set, capture which realized rows carry which brush, scroll far enough to recycle, and re-read.
  - **Expected result:** After scrolling, each realized row's `Background` matches the parity of the index it now occupies, not the index it had before.
  - **Failure means:** Banding is computed once at realization instead of on recycle, so scrolling produces visibly wrong stripes — two adjacent rows sharing a colour, or the pattern inverting as the user scrolls.
  - **Remarks:** Banding is the one row visual that is *index*-dependent rather than item-dependent, which is why it needs its own recycle test while the others are covered by the data assertions above. The product marks it as such: "Index-dependent (parity), so it must refresh when the row's position changes on recycle" (`TableViewRow.cpp:916-917`). Note the brush is resolved from `GetElementIndex` on the repeater, so a row whose index cannot be resolved keeps no banding at all — the test must assert it sees resolved indices rather than an all-unbanded result.

- [x] `VerifyRecycledRowDoesNotInheritPreviousSelectionVisual`
  - **Description:** Select a row, scroll far enough that its container is recycled onto an unselected item, and inspect the row now using that container.
  - **Expected result:** The recycled row reports `IsSelected == false` and sits in the `Normal` visual state. Exactly one realized row is in a `Selected*` state at a time — and only if the selected item is itself still realized.
  - **Failure means:** Selection visuals are attached to the container rather than the item, so scrolling paints selection onto arbitrary unrelated rows. The user sees rows highlight and un-highlight as they scroll, with the highlight tracking position rather than data.
  - **Remarks:** `TableViewRow.IsSelected` is a read-only DP "written only by the owning TableView" (`TableView.idl:423-426`), so the expectation is that the owner rewrites it on recycle rather than the row clearing itself. Asserting both the DP and the visual state matters: they are updated by two different calls (`SetIsSelectedInternal` sets the DP then calls `UpdateVisualState`), so a regression could leave one correct and the other stale.

### 5.4 Row backgrounds and gridlines

**Classification: all API tests.**

**Step 2 outcome: 7 planned → 3 written.** The four `GridLinesVisibility` items collapse into one table-driven test, and the two background items merge into one that covers the stated precedence rule.

- [x] `VerifyRowBackgroundAloneFillsEveryRowAndAlternatingStripesOddRows`
  - **Description:** Set only `RowBackground` and read every realized row's `Background`; then additionally set `AlternatingRowBackground` and re-read.
  - **Expected result:** With `RowBackground` alone, **every** row carries that brush — odd rows included. After `AlternatingRowBackground` is also set, even rows keep `RowBackground` and odd rows take `AlternatingRowBackground`.
  - **Failure means:** `RowBackground` is being treated as the even-row brush rather than the base for all rows, so setting it alone stripes the table — odd rows fall through to the theme background and the app gets banding it never asked for.
  - **Remarks:** **Merges the planned `VerifyRowBackgroundApplies` and `VerifyAlternatingRowBackgroundAppliesToOddRows`**, because the interesting behavior is the *precedence between them*, which neither item tested on its own. The rule is stated explicitly, with its lineage: "RowBackground is the base for every row; AlternatingRowBackground overrides odd rows only when set (WPF DataGrid parity). Setting RowBackground alone must fill all rows uniformly, not stripe odd rows transparent" (`TableViewRow.cpp:930-933`). The phrasing "must" makes this an invariant the product asserts about itself, which step 3 admits.
    The IDL says only "Opt-in row banding; null brushes preserve the theme row background" (`TableView.idl:471`) — true but silent on precedence. **Worth adding the precedence rule to the IDL**; it is the kind of thing an app author hits immediately.

- [x] `VerifyRowBackgroundChangeAfterLoadUpdatesLive`
  - **Description:** Change `RowBackground` and `AlternatingRowBackground` on a loaded control with rows already realized.
  - **Expected result:** Realized rows repaint to the new brushes without a reload; clearing both back to null returns them to the theme background.
  - **Failure means:** Banding is applied only at row realization, so an app that themes a table at runtime gets a half-updated table until something forces re-realization.
  - **Remarks:** Both DPs carry `MUX_PROPERTY_CHANGED_CALLBACK(TRUE)` (`TableView.idl:472-476`), so a live update is the declared contract rather than an inference. The null-clearing half is included because `RefreshRowBackground` clears the local value before deciding — the path that restores the theme background is separate from the path that sets a brush, and only clearing exercises it.

- [x] `VerifyGridLinesVisibilityDrawsExpectedSeparators`
  - **Description:** Walk all four `TableViewGridLinesVisibility` values, and for each read the row's `BorderThickness` (the horizontal separator) and each cell wrapper's `BorderThickness` and `BorderBrush` (the vertical separators).
  - **Expected result:** `All` → row bottom border non-zero **and** cell right borders non-zero with a non-null brush. `Horizontal` → row bottom border non-zero, cell borders zero. `Vertical` → row border zero, cell right borders non-zero. `None` → both zero.
  - **Failure means:** `GridLinesVisibility` does not control what it names. The two axes are driven by two different mechanisms — the row's own `BorderThickness` versus a per-cell `Border` — so getting one right and the other wrong is the likely regression, and it renders as a table with, say, row dividers that cannot be turned off.
  - **Remarks:** **Merges all four planned per-value items**, per step 2: they differ only in the enum value passed and the expected pair of thicknesses, which is a table, not four tests.
    Assert thickness is non-zero rather than equal to a specific value. The "on" case comes from `ClearValue` falling back to the `TableViewRow` style's `BorderThickness="0,0,0,1"` (`TableView.xaml:126`), which is a themeable resource an app may legitimately restyle; pinning `0,0,0,1` would make an app-level restyle look like a product regression. The *off* case is a genuine zero and is asserted exactly.
    Enum values are deliberately non-alphabetical — `All=0, Horizontal=1, None=2, Vertical=3` — because they "Mirror WPF DataGridGridLinesVisibility names and values for persisted casts" (`TableView.idl:35-44`). The test should walk them by name, never by numeric order.

### 5.5 Visual states

**Classification: 2 API tests, 2 moved to the interaction plan.** `CommonStates` is entered from `UpdateVisualState`, which is reachable programmatically for selection and `IsEnabled` but not for pointer or focus.

State is read via `VisualStateManager.GetVisualStateGroups(PART_RootBorder)` — the groups live on the template root, and reading `CurrentState.Name` asserts the state itself rather than a brush that several states happen to share.

- [x] `VerifyRowSelectedVisualStateAndDisabledPrecedence`
  - **Description:** Select a row and read its current `CommonStates` state; then set `IsEnabled = false` on the TableView while that row is still selected and read it again.
  - **Expected result:** Selecting gives `Selected`. Disabling while selected gives `SelectedDisabled` — not `Disabled`, and not `Selected`. Unselected rows go `Normal` then `Disabled`.
  - **Failure means:** The state machine collapses two orthogonal conditions. A selected row in a disabled table would render either as enabled-and-selected (misleading — it looks interactive) or as plain disabled (the user loses track of what was selected).
  - **Remarks:** **Merges the planned `VerifyRowSelectedVisualState` and `VerifyDisabledTableViewVisualState`**, because the contract worth testing is the precedence *between* them, which neither covered alone. Both the state list and the single-group design are in the IDL: "Selected states share this one group so nothing depends on GoToState call order" (`TableView.idl:411-417`), which is exactly why a wrong precedence is a silent product bug rather than a crash. The precedence itself — disabled wins — is stated in the product as "Disabled wins, for ListViewItem parity" (`TableViewRow.cpp:465-466`); the IDL lists the eight states but does not say which wins, so **the precedence rule should reach the IDL**.
    Also asserts that `IsEnabled` set on the *TableView* reaches the rows, which is what an app actually does; rows do not have `IsEnabled` set on them individually.
    **Was reported as a product bug; that diagnosis was wrong — it was a test bug.** See the correction below.

> #### ✅ Correction — the "disabled TableView renders its rows as enabled" finding was WRONG
>
> This was previously recorded here as a confirmed product bug with direct user impact. **It is not a
> product bug.** It was an insufficient settle in the test, and the retraction is kept in full because
> the way it was reached is the lesson.
>
> The original measurement was real: after `tableView.IsEnabled = false`, `row0.IsEnabled=False` and
> `row1.IsEnabled=False` but the states were still `Selected` and `Normal`. The error was concluding
> from that snapshot that `UpdateVisualState` is *never* called for an ancestor-driven `IsEnabled`
> change. It is called — one full layout + idle pass later than the change.
>
> **What exposed it:** the test failed when run alone but **passed** in the full suite and when run
> with its sibling row classes. A test whose result depends on what ran before it is not reporting a
> product fact. The deciding experiment was one extra `UpdateLayout()` + `IdleSynchronizer.Wait()`
> after setting `IsEnabled`: with it the test passes in isolation, deterministically. The sibling
> classes' extra message pumping had been supplying that pass for free.
>
> The state converges and a real app renders it on the next frame, so there is nothing to fix in the
> product. The second settle is now in the test with a comment telling the next reader not to remove
> it.
>
> **Two generalizable lessons.**
> 1. **An order-dependent result is never evidence of a product bug.** Before filing one, run the test
>    alone *and* with its neighbours. Divergence means the test is under-settled, not that the product
>    is broken. This is the second time in this category that a missing settle imitated a product
>    defect — see §5.3's recycle double-settle, which has the identical shape.
> 2. **A correct diagnostic can still support a wrong conclusion.** The permanent `IsEnabled` log did
>    its job — it correctly ruled out a propagation failure — and that made the remaining explanation
>    feel proven. It was not: "never happens" and "has not happened *yet*" are indistinguishable in a
>    single snapshot, and only a second observation later in time can separate them.
>
> **Left failing per step 5.** The expectation comes from the IDL's own `CommonStates` list, which
> declares `Disabled` and `SelectedDisabled` as states this control enters.


- [ ] `(moved to interaction plan)` `VerifyRowPointerOverVisualState`
  - **Reason:** `m_isPointerOver` is set only from `OnPointerEntered`/`OnPointerExited`. There is no programmatic way to enter the state, and calling `GoToState` from the test would assert the template rather than TableView's decision to enter it (step 2 rule 3). Belongs with the other pointer items in §11.

- [ ] `(moved to interaction plan)` `VerifyRowFocusVisualOnKeyboardFocus`
  - **Reason:** The row template has **no** `FocusStates` group; the style sets `UseSystemFocusVisuals="True"` (`TableView.xaml:127`), so the focus visual is drawn by the framework outside the control's visual tree and is not observable from an API test. `Focus(FocusState.Keyboard)` is callable, but the only thing an API test could then assert is that `FocusState` round-tripped — which is testing XAML, not TableView.



</details>

---

## 6. Selection

<details>
<summary>Show 16 items &mdash; 16 written, 16 passing</summary>

File: `TableView_Selection_APITests.cs`.

**All API tests.** Every item drives the control through `Select` / `Deselect` / `DeselectAll` /
`SelectionMode` and reads back `SelectedIndex`, `SelectedItem`, `IsSelected`, `TableViewRow.IsSelected`
and `SelectionChanged`. No test in this category needs real input; the click and keyboard gestures
from the original backlog are interaction tests and live in §11.

**Step 2 outcome: 27 planned → 16 written.** Dropped and merged:

- `VerifySelectionDefaults` — **dropped, already covered.** `TableViewTests.cs:132-134` asserts
  `Single` / `-1` / `null` on a fresh control.
- `VerifySelectionModeRoundtrip` — **dropped, already covered.** `TableViewTests.cs:326-328` runs it
  through `VerifySettableDependencyProperty`, which is strictly stronger (it also checks the DP path).
  `SelectedItem` / `SelectedIndex` read-only-ness is covered at `:335-338`, the DP statics at
  `:247-249`, and `TableViewRow.IsSelected`'s default and read-only-ness at `:92`, `:223-225`, `:426-427`.
- `VerifySwitchingModeAfterLoadIsHonored` — **dropped, not a separable behavior.** Both §6.1 tests
  already change the mode after load; a test that only asserts "the change took effect" has no
  failure mode of its own.
- `VerifyDeselectByIndexClearsSelection` + `VerifyDeselectUnselectedIndexIsNoOp` → merged into
  `VerifyDeselectClearsOnlyMatchingIndex`. One contract — `Deselect` is index-scoped — and the merged
  test states it better than either half.
- `VerifyDeselectAllClearsSelection` + `VerifyDeselectAllIsIdempotent` → merged into
  `VerifyDeselectAllClearsAndIsIdempotent`. The second call is one extra line on the first test.
- `VerifyIsSelectedToleratesInvalidIndex` + `VerifySelectOutOfRangeIsTolerated` +
  `VerifySelectNegativeIndexIsTolerated` → merged into
  `VerifyInvalidIndicesAreSafeAndLeaveSelectionCoherent`. One contract across all three entry points.
- `VerifyInsertBeforeSelectedRowKeepsSelectedItem` + `VerifyRemoveBeforeSelectedRowKeepsSelectedItem`
  → merged into `VerifySelectionTracksItemAcrossInsertAndRemoveAbove`. Same invariant, two mutations.
- `VerifySelectionChangedFiresWithAddedItems` + `VerifySelectionChangedFiresWithRemovedItems` →
  merged into `VerifySelectionChangedReportsAddedAndRemovedItems`. One handler, one sequence; the
  removed half is only observable as the *second* selection's event anyway.
- `VerifySelectionChangedDoesNotFireForSameSelection` + `VerifySelectionChangedDoesNotFireOnNoOpDeselect`
  → merged into `VerifySelectionChangedDoesNotFireForNoOpChanges`.
- `VerifyRowIsSelectedClearedOnDeselectAll` → folded into `VerifyRowIsSelectedFollowsOwnerSelection`
  as its final step.

> **Two backlog names were renamed because they pre-judged an unspecified outcome.**
> `VerifyItemsSourceSwapClearsSelection` and `VerifyResetClearsSelection` both asserted "clears".
> The IDL says nothing about what selection does across a source swap or a `Reset` — clearing and
> re-anchoring the same item by identity are both defensible. Writing "clears" into the test would
> have invented a contract (step 3) and, as it happens, would have failed against a deliberate
> design. Both are now scoped to the invariant the IDL *does* state, plus one genuinely strong
> assertion each (see their Remarks).

### Comparison with PR `!15971489`

The PR carries 18 selection API tests (appendix, `TableViewTests.cs`). **8 of our 16 have a PR
equivalent; 8 are new.** Five PR tests have no counterpart here by design.

| PR test | Here |
|---|---|
| `TableView_SelectByIndexUpdatesDPs` | `VerifySelectByIndexUpdatesSelectedItemAndIndex` — also pins **reference identity** of `SelectedItem` |
| `TableView_DeselectByIndexClearsDPs` | `VerifyDeselectClearsOnlyMatchingIndex` — also pins that a non-matching index is a no-op |
| `TableView_SelectingSecondIndexInSingleClearsFirst` | `VerifySelectingSecondIndexClearsFirst` — also checks row chrome |
| `TableView_NoneClearsSelection` | `VerifySelectionModeNoneClearsExistingSelection` |
| `TableView_NoneIgnoresExplicitSelect` | `VerifySelectionModeNoneIgnoresSelectCall` |
| `TableView_ItemsSourceSwapClearsSelection` | `VerifySelectionRemainsCoherentAfterItemsSourceSwap` — **rescoped**, see its Remarks |
| `TableView_SelectOutOfRangeIsTolerated` | `VerifyInvalidIndicesAreSafeAndLeaveSelectionCoherent` — broader (6 invalid values × 3 entry points) |
| `TableView_SelectionChangedFiresOnSelect` | `VerifySelectionChangedReportsAddedAndRemovedItems` — also asserts `RemovedItems` and rejects swapped collections |
| `TableView_SelectionChangedDoesNotFireForSameSelection` | `VerifySelectionChangedDoesNotFireForNoOpChanges` — adds the no-op `Deselect` case |
| `TableView_SelectionDefaultsAreCorrect`, `TableView_SelectionModeRoundtrip`, `TableViewRow_IsSelectedDefaultsToFalse` | Category 1 |

**New here, no PR equivalent:**

- `VerifyDeselectAllClearsAndIsIdempotent` — the PR only covers `DeselectAll` in an *interaction* test.
- `VerifyIsSelectedReflectsSelection` — the PR never asserts `IsSelected` against the other two projections.
- `VerifySelectionTracksItemAcrossInsertAndRemoveAbove` — the PR's nearest is the Gap-suite
  `SelectionChangedReportsAddedRemovedAndSourceChangesRebaseSelection`, which bundles rebasing into an
  event test rather than pinning the item-anchored invariant directly.
- `VerifyRemovingSelectedItemClearsSelection`
- `VerifySelectionRemainsCoherentAfterCollectionReset` — and see its test-construction note; this path
  is easy to write in a way that silently covers nothing.
- `VerifySelectionChangedFiresOnceForModeChangeClear` — the PR checks that `None` clears, but not that
  it raises, nor that it raises exactly once.
- `VerifyRowIsSelectedFollowsOwnerSelection` — the PR only checks the row's *default*.

**PR tests with no counterpart here, deliberately:**

- `TableView_SelectAllInMultipleSelectsEverything`, `TableView_SelectAllInSingleModeIsClampedToOne`,
  `TableView_SelectRangeInMultiple`, `TableView_MultipleToSingleClampsToOne` — deferred; this release
  has no `Multiple`/`Extended`, `SelectAll` or `SelectRange`.
- `TableView_SelectedIndexDPDispatchesIntoModel` and
  `TableView_InvalidSelectionDPCoercesToModelConsistency` — **not applicable.** The PR's `SelectedIndex`
  was settable, so it needed tests for writing through the DP and for coercing an invalid write. Here
  both projections are read-only (`TableView.idl:542-546`) and the control is their only writer, so
  there is no write path to coerce. This is a genuine API difference, not a coverage gap.

### 6.1 Selection mode gating
Spec basis: `TableView.idl:536` — `SelectionMode` is "On by default, matching ItemsView, ListView and
WPF's DataGrid. **None = display-only**", and `:20` defines `None = 0`. "Display-only" is the whole
contract: in `None` a table shows data and nothing else.

- [x] `VerifySelectionModeNoneIgnoresSelectCall`
  - **Description:** Set `SelectionMode = None` on a loaded table, then call `Select(1)`.
  - **Expected result:** `SelectedIndex` stays `-1`, `SelectedItem` stays null, `IsSelected(1)` is false, and no realized row reports `IsSelected`.
  - **Failure means:** `None` is not display-only — an app that opted out of selection can still be driven into a selected state programmatically, and the row will render selected chrome the app never asked for.
  - **Remarks:** Strong. "None = display-only" is stated in the IDL, and a mode that does not gate the primary selection entry point has no meaning.

- [x] `VerifySelectionModeNoneClearsExistingSelection`
  - **Description:** Select row 1 in `Single`, then switch `SelectionMode` to `None`.
  - **Expected result:** `SelectedIndex` returns to `-1`, `SelectedItem` to null, and the previously selected realized row reports `IsSelected == false`.
  - **Failure means:** Turning selection off leaves a row rendered as selected and `SelectedItem` pointing at it, so a display-only table shows selection chrome permanently with no API left to clear it.
  - **Remarks:** Moderate-to-strong. The IDL states `None = display-only` but does not spell out what happens to a pre-existing selection. Display-only is unachievable if a stale selection survives the switch, so this is the only coherent reading. Note `SelectionMode` carries `[MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]` in the IDL, which confirms the mode change is meant to *do* something rather than only gate future calls.

### 6.2 Single-selection API

Spec basis: `TableView.idl:531-552` — "Row-scoped, single item. `SelectedItem` and `SelectedIndex` are
read-only projections of the selection and **stay coherent with each other**. Drive selection through
`Select` / `Deselect` / `DeselectAll`, matching ItemsView." Also `:541` "The selected data item, or
null" and `:544` "The selected item's index, or -1 when nothing is selected".

- [x] `VerifySelectByIndexUpdatesSelectedItemAndIndex`
  - **Description:** Call `Select(1)` on a loaded table over a known list.
  - **Expected result:** `SelectedIndex == 1`, `SelectedItem` is the same object as `items[1]` (reference identity), and `IsSelected(1)` is true while `IsSelected(0)` is false.
  - **Failure means:** The two projections disagree, or `SelectedItem` is a copy/wrapper rather than the app's own item — an app that compares `SelectedItem` against its own collection by reference gets no match and the selection appears empty.
  - **Remarks:** Strong; coherence is stated verbatim. Reference identity specifically is grounded in "the selected **data item**" — a projection that hands back an internal wrapper is not the data item. §10's editing path is known to wrap items internally (`UnwrapEditingDataItem` exists), which is exactly why this is worth pinning.

- [x] `VerifySelectingSecondIndexClearsFirst`
  - **Description:** `Select(0)`, then `Select(2)`.
  - **Expected result:** `SelectedIndex == 2`, `SelectedItem == items[2]`, `IsSelected(0)` is false, and the row for index 0 reports `IsSelected == false` while the row for index 2 reports true.
  - **Failure means:** `Single` accumulates selections — two rows render selected at once while `SelectedIndex` can only report one, so the visible state and the API permanently disagree.
  - **Remarks:** Strong. "Row-scoped, **single item**" plus a scalar `SelectedIndex`/`SelectedItem` surface admits no other reading.

- [x] `VerifyDeselectClearsOnlyMatchingIndex`
  - **Description:** `Select(1)`, then call `Deselect(0)` (not selected), then `Deselect(1)` (selected).
  - **Expected result:** After `Deselect(0)` the selection is untouched (`SelectedIndex == 1`). After `Deselect(1)` it is cleared (`-1` / null).
  - **Failure means:** `Deselect` ignores its argument and clears whatever is selected, so a stale or racing call for an old row silently wipes the user's current selection.
  - **Remarks:** Moderate. The IDL declares `Deselect(Int32 index)` but does not state that a non-matching index is a no-op. An index parameter that does not scope the operation would be meaningless, and `DeselectAll()` already exists for the unscoped case — so the parameter must mean something. Flagged because it is an inference from the signature, not stated prose.

- [x] `VerifyDeselectAllClearsAndIsIdempotent`
  - **Description:** `Select(1)`, call `DeselectAll()` twice, counting `SelectionChanged` raises.
  - **Expected result:** Selection is `-1` / null after the first call; the second call changes nothing and raises no additional `SelectionChanged`.
  - **Failure means:** Either clearing does not work, or a redundant clear raises a spurious event — a handler that rebuilds a details pane on every `SelectionChanged` does redundant work, and one that toggles state flips it wrongly.
  - **Remarks:** Clearing is strong (the method name and `:552` state it). Idempotence is moderate — see the shared note under §6.4 on no-op suppression.

- [x] `VerifyIsSelectedReflectsSelection`
  - **Description:** With row 1 selected, call `IsSelected` for every index in the source.
  - **Expected result:** True for 1 only, false for all others; the result agrees with `SelectedIndex` throughout.
  - **Failure means:** `IsSelected` is a third projection that can disagree with the other two, so the three public ways of asking "is this row selected" give different answers.
  - **Remarks:** Strong. Coherence is stated for `SelectedItem`/`SelectedIndex`; `IsSelected(Int32)` is declared alongside them as part of the same row-scoped surface and cannot reasonably be exempt.

- [x] `VerifyInvalidIndicesAreSafeAndLeaveSelectionCoherent`
  - **Description:** With row 1 selected, call `IsSelected`, `Select` and `Deselect` with `-1`, `int.MinValue`, `count` and `int.MaxValue`. Re-read all three projections after each call.
  - **Expected result:** No call throws. `IsSelected` returns false for every invalid index. After every call the projections stay coherent — `SelectedIndex` is either `-1` or a valid index, and `SelectedItem` is null exactly when `SelectedIndex` is `-1` and is otherwise the item at that index. A `Select` with an index past the end does not change the existing selection.
  - **Failure means:** An index arriving from a stale event, a cleared collection, or a converted `-1` sentinel either throws out of a property setter or silently corrupts the selection. The `SelectedItem`-without-`SelectedIndex` case is the dangerous one: a data-bound details pane keeps showing an item the table no longer reports as selected.
  - **Remarks:** "Does not throw" and coherence are strong (coherence is stated; the IDL declares no exceptions on any of the three methods). **"Out-of-range `Select` preserves the existing selection" is an inference and the one to revisit if this fails** — the IDL does not say whether an unresolvable index is rejected or coerced to "clear". The reasoning: `Select` is declared as a selecting operation, `DeselectAll()` already covers clearing, and a call that cannot select anything should not be able to *un*select something. Note `Select(-1)` is deliberately included in the "does not throw / stays coherent" assertions but its *outcome* is left unpinned, because "select nothing" and "reject" are equally defensible for a negative index.

### 6.3 Selection across source mutation

Spec basis: the coherence invariant at `TableView.idl:532-534` plus `:541` "The selected **data item**".
The IDL says nothing more specific about collection changes, so these tests are deliberately scoped to
what it does state. See each item's Remarks.

- [x] `VerifySelectionTracksItemAcrossInsertAndRemoveAbove`
  - **Description:** Over an `ObservableCollection`, select the item at index 2. Insert a new item at index 0, then remove an item above the selection.
  - **Expected result:** `SelectedItem` remains the same object throughout. `SelectedIndex` shifts to 3 after the insert and back to 2 after the remove, always agreeing with the item's real position in the collection.
  - **Failure means:** The selection is anchored to an index rather than an item, so inserting a row above the selection silently moves the selection to a *different* record. In an editing or delete flow this is data-destructive: the user acts on a row they did not select.
  - **Remarks:** Moderate-to-strong. The IDL calls `SelectedItem` and `SelectedIndex` "read-only projections of **the selection**" — the selection is the primary thing and both are projections of it. Combined with "the selected data item", that makes the *item* the anchor and the index the derived value, which is exactly what this asserts. Still an inference: the IDL never mentions collection changes.

- [x] `VerifyRemovingSelectedItemClearsSelection`
  - **Description:** Select an item, then remove that item from the `ObservableCollection`.
  - **Expected result:** `SelectedItem` is no longer the removed object. Selection is coherent afterwards — if `SelectedIndex` is not `-1`, `SelectedItem` is the item now at that index and is present in the collection.
  - **Failure means:** `SelectedItem` keeps a reference to an item that is no longer in the source. The control pins a deleted record alive, and an app that saves `SelectedItem` writes back a row the user deleted.
  - **Remarks:** Strong on the part it asserts — an object that is not in the source cannot be "the selected data item". Deliberately does **not** assert *what* it becomes: clearing to `-1` and re-anchoring to the neighbour that shifted into the slot are both defensible and the IDL picks neither. Pinning one would invent a contract.

- [x] `VerifySelectionRemainsCoherentAfterItemsSourceSwap`
  - **Description:** Select a row, then assign a brand-new list (different objects) to `ItemsSource`.
  - **Expected result:** No crash. Selection is coherent: `SelectedItem` is null exactly when `SelectedIndex == -1`; if non-null it is an object actually present in the **new** source and at the reported index; and it is never an object from the old source.
  - **Failure means:** The selection survives into a dataset it does not belong to — the classic "stale selection after refresh" bug, where `SelectedItem` points into the previous page of data.
  - **Remarks:** Scoped deliberately. **Renamed from `VerifyItemsSourceSwapClearsSelection`**, which asserted an outcome the IDL never states. Clearing on swap and re-anchoring by identity (when the same object appears in both sources) are both legitimate; this test pins only the invariant that rules out the actual bug, and so passes under either design.

- [x] `VerifySelectionRemainsCoherentAfterCollectionReset`
  - **Description:** Over an `ObservableCollection`, select an item, then raise a `Reset` by calling `Clear()` and re-adding — first a batch that still contains the selected object, then a batch that does not.
  - **Expected result:** Both cases stay coherent and do not crash. In the second case — the selected object is gone — `SelectedItem` must not be that object.
  - **Failure means:** A `Reset`, which is what a filter, a refresh or a re-query produces, leaves `SelectedItem` referencing a record that is no longer in the data.
  - **Remarks:** Scoped deliberately. **Renamed from `VerifyResetClearsSelection`.** A `Reset` destroys index information but not item identity, so re-anchoring a still-present item is a legitimate design and asserting "clears" would have been an invented contract that failed against it. The second half — a vanished item must not remain selected — is strong and is where the real failure mode lives.

> #### ⚠️ Test-construction note — `Clear()` + `Add()` does not test what it looks like it tests
>
> The first version of this test produced its "reset that preserved the selected item" case with
> `items.Clear()` followed by re-adding the same objects. It passed — and covered nothing. `Clear()`
> raises its `Reset` **while the collection is empty**, so at the moment the notification is handled
> the selected item is genuinely absent and no control could preserve it. The observed result was
> `SelectedIndex == -1`, which satisfied the coherence assertion and looked like a pass.
>
> The test now uses a `ResettableCollection<T>.ReplaceAll` helper that mutates through `Items` and
> then raises **one** `Reset` with the new contents already in place — the shape a refresh, re-query
> or filter change actually produces. With that, the path is real: reversing an 8-item list moved the
> selection from index 2 to index **5**, i.e. the selection survived the reset and followed its item.
> The product has substantial machinery for exactly this (`m_stickySelectedItem`,
> `OnSelectionSourceReset`, `DrainPendingSelection`) and none of it had been executed.
>
> **Generalizable:** when a test asserts behavior across a collection notification, confirm the
> collection is in the state the scenario describes *at the instant the notification is raised*, not
> merely afterwards. A passing coherence assertion does not prove the interesting path ran.


### 6.4 Selection events

Spec basis: `TableView.idl:554` declares `SelectionChanged` using the **platform**
`Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs`, not a bespoke type. Reusing the platform type
is itself the contract: the args mean what they mean on `Selector` and `ListView`, or a handler could
not be shared between them — which the implementation notes as the explicit reason for the choice.

- [x] `VerifySelectionChangedReportsAddedAndRemovedItems`
  - **Description:** Subscribe, then `Select(0)` followed by `Select(2)`, recording each event's `AddedItems` and `RemovedItems`.
  - **Expected result:** The first event carries `items[0]` in `AddedItems` and an empty `RemovedItems`. The second carries `items[2]` in `AddedItems` and `items[0]` in `RemovedItems`. Both collections contain at most one item.
  - **Failure means:** The delta is wrong or the two collections are swapped, so a handler doing incremental work — detaching from the old item, attaching to the new — corrupts its own state. Swapped collections are especially bad because the count is right and the code appears to work.
  - **Remarks:** Strong. The platform args type fixes the meaning of both collections. Note the constructor order is `(removedItems, addedItems)`, which is the reverse of the property reading order and an easy place for the product to be wrong — a good reason to assert both explicitly rather than only counts.

- [x] `VerifySelectionChangedDoesNotFireForNoOpChanges`
  - **Description:** `Select(1)`, then with a counter armed: `Select(1)` again, `Deselect(0)` (not selected), and `IsSelected(1)`.
  - **Expected result:** The counter stays at zero across all three, and the selection is unchanged at the end.
  - **Failure means:** The event fires when nothing changed. Handlers commonly reload a details pane or push navigation on `SelectionChanged`; a spurious raise makes re-selecting the current row flicker or re-enter, and a toggle-style handler inverts its state.
  - **Remarks:** Moderate. The IDL does not state no-op suppression, but an event named `SelectionChanged` carrying an added/removed delta has no meaningful payload when nothing changed — it would have to report two empty collections. Consistent with `Selector`, whose args type this reuses.

- [x] `VerifySelectionChangedFiresOnceForModeChangeClear`
  - **Description:** Select a row, arm a counter, then set `SelectionMode = None`.
  - **Expected result:** Exactly one `SelectionChanged`, carrying the previously selected item in `RemovedItems` and an empty `AddedItems`.
  - **Failure means:** Either the clear is silent — a handler never learns the selection went away and keeps showing a detail pane for an unselectable row — or it raises more than once, indicating the clear is implemented as several passes over the same transition.
  - **Remarks:** Moderate, and depends on §6.1's clear-on-`None` expectation being right. If that one is re-specified, revisit this one with it.

### 6.5 Row selection surface

Spec basis: `TableView.idl:423-426` — `TableViewRow.IsSelected` is a "Read-only DP written only by the
owning TableView … Drives the Selected* states."

- [x] `VerifyRowIsSelectedFollowsOwnerSelection`
  - **Description:** On a loaded table, walk realized rows after `Select(1)`, after `Select(2)`, and after `DeselectAll()`.
  - **Expected result:** Exactly one row reports `IsSelected == true` after each `Select` and it is the row whose `DataContext` is the selected item; every row reports false after `DeselectAll()`.
  - **Failure means:** Row chrome and the control's selection diverge — either no row lights up for a programmatic selection, or a stale row stays lit after the selection moves, showing two selected rows in a single-selection control.
  - **Remarks:** Strong; the IDL states the DP is written by the owning TableView and drives the selected visual states. Distinct from §5.3 (which checks a *recycled* row does not carry `IsSelected` to a new item) and from §5.5 (which checks the visual *state name* and disabled precedence). This one covers the plain propagation path neither of those exercises.

### 6.6 Deferred

- `(deferred)` Multiple/Extended selection modes, `SelectedItems`, `SelectedItemIndices`, `SelectAll`, `SelectRange`, Ctrl/Shift range gestures. Not in current IDL. `TableView.idl:14-16` confirms the enum values are appended so these stay additive.
- `(→ §11)` Click-to-select, Ctrl-click to toggle, and keyboard Up/Down/Home/End/PageUp/PageDown selection. These need real input: the toggle path reads live keyboard modifier state rather than taking it from event args, so no API-level call can exercise it.



</details>

---

## 7. Sorting

<details>
<summary>Show 30 items &mdash; 30 written, 30 passing (35 TAEF cases with data variations) &mdash; <strong>complete</strong></summary>

File: `TableView_Sorting_APITests.cs`.

### 7.0 Boundary with Category 8

Category 8 already covers `TableViewSource.Sort` — ordering by path, by dotted path, by key selector, clearing an axis, and add-while-sorted. **Category 7 owns the control-level sort API**: the sort state that lives on `TableViewColumn.SortDirection`, the three verbs and their documented return values, the `Sorting`/`Sorted` events, the click-to-sort gates, and the reconciliation rule between the two surfaces. A test belongs here if it would still be meaningful with `TableViewSource` never mentioned in the test body.

| Overlap | Where it lives |
|---|---|
| Ordering by a property path | §8 owns the *source verb*. §7 owns ordering reached through a **column**, because the key path is resolved by `GetSortMemberPathCore()` first — a different code path with its own failure mode. |
| `ClearSort` | Both surfaces declare one. §8 tests `TableViewSource.ClearSort`; §7 tests `TableView.ClearSort()`, which also has to reset every column's `SortDirection`. |
| Add while sorted | §8 asserts the projection. §7 asserts only that a column-driven sort survives the mutation — one test, not two. |
| Header invoke and UIA sort metadata | §12 owns the peer surface (`VerifyInvokePatternIsWithheldWhenTheColumnCannotSort`, `VerifyColumnHeaderHelpTextReportsSortStateOnlyForSortableColumns`). §7 keeps only *invoking* the header and observing that the sort state and the `SortIndicator` follow. |

### 7.1 Column sort surface

- [x] `VerifyColumnSortDefaults`
  - **Description:** Constructs a `TableViewColumn` and a `TableViewTextColumn` that have never been added to a table, and reads every sort-related property.
  - **Expected result:** `CanSort` is `true`, `SortCycle` is `AscendingDescending`, `SortMemberPath` is empty, `CustomSortComparer` is `null`, and `SortDirection` is `None`.
  - **Failure means:** A table would arrive pre-sorted, or silently refuse to sort, before an app has configured anything.
  - **Remarks:** Every default here is written into `TableView.idl` as a `MUX_DEFAULT_VALUE` attribute (`:157`, `:165`, `:185`), so this is a direct spec assertion rather than an inference.
- [x] `VerifyGetSortMemberPathReturnsExplicitPath`
  - **Description:** Sets `SortMemberPath` on a probe column deriving from `TableViewColumn` that exposes a public passthrough to `base.GetSortMemberPathCore()`, and calls it.
  - **Expected result:** The explicit path is returned verbatim.
  - **Failure means:** The base column is transforming or ignoring the path an app supplied, and every column sort keyed on it is guessing.
  - **Remarks:** `TableView.idl:193` states the base column "returns `SortMemberPath` verbatim". `GetSortMemberPathCore` is `overridable`, so it projects as `protected virtual` and cannot be called from a test class; the probe-column passthrough is the only direct route. Deriving a column in C# is already proven — see the note in §3.6.
- [x] `VerifyGetSortMemberPathFallsBackToTextColumnBindingPath`
  - **Description:** Leaves `SortMemberPath` empty on a `TableViewTextColumn` probe whose `Binding` points at `Name`, and calls the passthrough.
  - **Expected result:** `"Name"` is returned — the text column falls back to `Binding.Path.Path`.
  - **Failure means:** The single most common authoring shape (a text column with no explicit sort path) is unsortable, which would make most of §7.3 vacuous in real apps.
  - **Remarks:** Spelled out twice in the IDL, on `SortMemberPath` (`:175`) and on `GetSortMemberPathCore` (`:192`).
- [x] `VerifyBaseColumnWithNoPathAndNoComparerHasNoSortKey`
  - **Description:** Calls the passthrough on a base-column probe with neither `SortMemberPath` nor `CustomSortComparer` set.
  - **Expected result:** An empty string is returned.
  - **Failure means:** A column with nothing to sort by reports a key anyway, and the sort pipeline would sort by a path that does not exist instead of declining.
  - **Remarks:** `TableView.idl:175-177`: "the base column returns an empty string and the column is only sortable if it supplies a `CustomSortComparer`." The *behavioural* half of that sentence is `VerifyColumnWithNoSortKeyDoesNotReorderRows` in §7.3.
- [x] `VerifyCustomColumnSortMemberPathOverrideDrivesSort`
  - **Description:** A custom column overrides `GetSortMemberPathCore()` to return a property the column never mentions in any `Binding`, and the table is sorted by it.
  - **Expected result:** Rows order by the overridden property.
  - **Failure means:** `GetSortMemberPathCore` is public extension surface that the sort pipeline never actually consults — the override would be callable but inert.
  - **Remarks:** The only test proving the `overridable` member is wired into the pipeline rather than merely existing. Declared at `TableView.idl:193`.
- [ ] ~~`VerifyCanSortRoundtrip`~~, ~~`VerifySortCycleRoundtrip`~~, ~~`VerifySortMemberPathRoundtrip`~~, ~~`VerifyCustomSortComparerRoundtrip`~~ **(dropped — platform guarantee)**
  - Four separate tests asserting that a plain `DependencyProperty` returns what was assigned to it. That is the platform's contract, not TableView's (Step 2, rule 3). Each of these properties is *used* by a behavioural test below — `CanSort` by `VerifyToggleSortDirectionRespectsCanSort`, `SortCycle` by `VerifyToggleSortDirectionCyclesPerSortCycle`, `SortMemberPath` by `VerifySortBySortMemberPathOrdersRows`, `CustomSortComparer` by `VerifyCustomSortComparerOrdersRows` — so a broken setter still fails a test, and fails it for a reason worth reading.

### 7.2 Programmatic sort state

- [x] `VerifySortByColumnSetsDirection` *(data-driven: `Ascending`, `Descending`, `None`)*
  - **Description:** Calls `SortByColumn(column, direction)` for each `SortDirection` and reads `column.SortDirection` back.
  - **Expected result:** The column reports the requested direction; `None` leaves it at `None`.
  - **Failure means:** The sort state an app sets is not the state the control holds, so the header glyph and every subsequent toggle start from the wrong place.
  - **Remarks:** Merged from three plan items differing only in the enum value. `TableView.idl:585-586`: "Clears any other sort state, then sets this column's `SortDirection`. Pass `SortDirection.None` to clear."
- [x] `VerifySortByColumnReturnsWhetherStateChanged`
  - **Description:** Calls `SortByColumn` to apply a sort, then calls it again with the same column and the same direction, checking the returned `Boolean` each time.
  - **Expected result:** `true` the first time, `false` the second — and `false` when clearing a column that is already `None`.
  - **Failure means:** The documented return value is decorative, so an app cannot tell a real state change from a no-op without re-reading the whole table.
  - **Remarks:** `TableView.idl:587` — "Returns true when the sort state changed." Return values are easy to leave unimplemented (`return true;`) and nothing else in the suite would notice.
- [x] `VerifySortByColumnReplacesPreviousColumnSort`
  - **Description:** Sorts column A ascending, then sorts column B descending, and reads both columns.
  - **Expected result:** B reports descending and A reports `None`; the rows are ordered by B.
  - **Failure means:** Two columns claim to be the active sort at once — the table shows two glyphs and the row order matches at most one of them.
  - **Remarks:** Single-column sorting is stated in two places: `TableView.idl:180-181` ("sorting is single-column: applying a sort clears any previous one") and `:585`.
- [x] `VerifyToggleSortDirectionCyclesPerSortCycle` *(data-driven: all four `TableViewSortCycle` values)*
  - **Description:** Sets one `SortCycle` on a column, then calls `ToggleSortDirection` repeatedly, recording `SortDirection` after each call through a full lap plus one.
  - **Expected result:** `AscendingDescending` walks Asc → Desc → Asc; `AscendingDescendingNone` walks Asc → Desc → None → Asc; `DescendingAscending` walks Desc → Asc → Desc; `DescendingAscendingNone` walks Desc → Asc → None → Desc.
  - **Failure means:** Repeated header clicks do not do what the column's declared policy says — including the two-state cycles never returning to their first state, which would strand a user in one direction.
  - **Remarks:** The four values are the whole of `enum TableViewSortCycle` (`TableView.idl:72-78`); the names define the sequences. Running one lap **plus one** is the point: it proves the cycle wraps rather than terminating.
- [x] `VerifyToggleSortDirectionRespectsCanSort`
  - **Description:** Sets `CanSort = false`, calls `ToggleSortDirection`, and inspects the return value, the column state and the row order.
  - **Expected result:** Returns `false`, `SortDirection` stays `None`, and the rows keep source order.
  - **Failure means:** A column an app declared unsortable sorts anyway.
  - **Remarks:** `TableView.idl:155-156` scopes `CanSort` to "the click-to-sort UX … programmatic `SortByColumn` still works". `ToggleSortDirection` is documented as the *cycling* verb the click handler uses (`:589-590`), so it is the click path in API form. If the product instead cycles it, that is a finding to record, not an expectation to soften.
- [x] `VerifyClearSortClearsEveryColumnAndRestoresSourceOrder`
  - **Description:** Sorts a column, then calls `ClearSort()`, and reads every column plus the row order.
  - **Expected result:** Returns `true`, every column reports `None`, and rows are back in source order.
  - **Failure means:** Clearing the sort leaves either a stale glyph or a stale order — a table that claims to be unsorted while showing sorted rows.
  - **Remarks:** `TableView.idl:592-593` — "Clears every column's sort state in one batch. Returns true when anything was cleared." Distinct from §8's `VerifyClearSortRestoresSourceOrder`, which clears the *source* axis and knows nothing about columns.
- [x] `VerifyClearSortReturnsFalseWhenNothingIsSorted`
  - **Description:** Calls `ClearSort()` on a never-sorted table, then twice in a row after a real sort.
  - **Expected result:** `false` when there was nothing to clear, `true` for the first real clear, `false` for the second; no exception in any case.
  - **Failure means:** Idempotence is broken, or the return value cannot be used to detect a real change — both of which make `ClearSort` unsafe to call defensively.
  - **Remarks:** Folds the old `VerifyClearSortIsIdempotent` into the return-value assertion, since the safe-no-op claim and the `false` return are the same claim.
- [x] `VerifyCanUserSortColumnsFalseStillAllowsProgrammaticSort`
  - **Description:** Sets `CanUserSortColumns = false`, then calls `SortByColumn`.
  - **Expected result:** The sort applies — the column reports the direction and rows reorder.
  - **Failure means:** A control-wide UX gate has silently disabled the API, so an app that ships its own sort UI cannot sort at all.
  - **Remarks:** `TableView.idl:577-579` — "When false no sort affordance is built and header clicks do not sort; programmatic sorting still works."
- [x] `VerifyCanUserSortColumnsFalseBuildsNoSortAffordance`
  - **Description:** Loads a table with `CanUserSortColumns = false` and inspects the header for a `SortIndicator`; then sets it back to `true` and re-inspects.
  - **Expected result:** No sort affordance is present while the gate is false, and one appears once it is true.
  - **Failure means:** The table advertises sorting it will not perform on click — the affordance is the user's only signal that a header is interactive.
  - **Remarks:** The visible half of the same IDL sentence; the invocable half is §12's `VerifyInvokePatternIsWithheldWhenTheColumnCannotSort`. Whether "no affordance" means absent or collapsed is not specified, so the test accepts either and says so in its message.

### 7.3 Sort ordering results

- [x] `VerifySortBySortMemberPathOrdersRows`
  - **Description:** Sorts ascending by a column whose `SortMemberPath` names a string property, and reads the projected row order.
  - **Expected result:** Rows are ordered by that property.
  - **Failure means:** The column sort surface resolves a key but never applies it — the state changes and nothing moves.
  - **Remarks:** Not redundant with §8's `VerifySortByPathOrdersRows`: this one enters through `SortByColumn` and the key comes from `GetSortMemberPathCore()`, so it covers the resolution step §8 bypasses.
- [x] `VerifySortDescendingReversesOrder`
  - **Description:** Sorts the same column ascending, records the order, then sorts descending.
  - **Expected result:** The descending order is the exact reverse of the ascending order.
  - **Failure means:** Descending is not the inverse comparison — typically a comparer that ignores the direction, which looks correct in one direction only.
  - **Remarks:** Asserting against the recorded ascending order rather than a hardcoded list keeps the two assertions from drifting apart if the fixture changes.
- [x] `VerifyCustomSortComparerOrdersRows`
  - **Description:** Assigns an `ITableViewSortComparer` implementing a deliberately non-alphabetical order (by string length, then ordinal), sorts by that column, and counts the `Compare` calls.
  - **Expected result:** Rows follow the comparer's order, and `Compare` was called at least once.
  - **Failure means:** A supplied comparer is ignored in favour of a default comparison, which is invisible whenever the two orders happen to agree — hence the deliberately unnatural order.
  - **Remarks:** `TableView.idl:86-89`; the comparer returns `<0`, `0` or `>0` and "must be a pure function of its inputs", so the test comparer does nothing but compare.
- [x] `VerifyCustomSortComparerTakesPrecedenceOverSortMemberPath`
  - **Description:** Sets both `SortMemberPath` and a `CustomSortComparer` that produce different orders on the same data, then sorts.
  - **Expected result:** The comparer's order wins.
  - **Failure means:** The documented precedence is inverted, so adding a comparer to an already-pathed column would silently do nothing.
  - **Remarks:** `TableView.idl:179-180` — "Takes precedence over `SortMemberPath`." The fixture must make the two orders differ, or the test proves nothing.
- [x] `VerifySortIsStableForEqualKeys`
  - **Description:** Sorts by a property several items share, and compares the relative order of each tied group against source order.
  - **Expected result:** Tied items keep their source-relative order.
  - **Failure means:** Rows shuffle unpredictably between sorts of the same data — the classic unstable-sort symptom, and the reason a second sort on an unrelated column appears to scramble the table.
  - **Remarks:** Stability is not stated in the IDL. The test asserts it anyway, as a quality-of-implementation expectation worth pinning; if it fails, it is a spec question to answer rather than a test to delete.
- [x] `VerifySortSurvivesItemAddition`
  - **Description:** Sorts by a column over an `ObservableCollection`, adds an item whose key belongs in the middle, and reads the order.
  - **Expected result:** The sort is still in force, the column still reports its direction, and the new item is at its sorted position rather than at the end.
  - **Failure means:** Either the sort is dropped on mutation, or new items accumulate out of order — both leave a table that claims to be sorted and is not.
  - **Remarks:** Merged from two plan items (`VerifySortSurvivesItemAddition` and `VerifySortAppliesToItemAddedAfterSorting`) that were the same scenario read twice. §8's `VerifyAddWhileSortedLandsAtTheSortedPosition` covers the source-verb equivalent.
- [x] `VerifyColumnWithNoSortKeyDoesNotReorderRows`
  - **Description:** Calls `SortByColumn(column, Ascending)` on a base column with no `SortMemberPath` and no `CustomSortComparer`.
  - **Expected result:** The rows keep source order, and the control does not throw.
  - **Failure means:** A column with nothing to sort by either reorders rows by something unspecified, or takes the app down for a configuration the IDL describes as merely unsortable.
  - **Remarks:** The behavioural half of `TableView.idl:175-177`. What `SortDirection` should read afterwards is *not* specified — the test asserts the row order, logs the resulting direction, and leaves the question in the plan rather than inventing an answer.

### 7.4 Sort events

- [x] `VerifySortingFiresBeforeStateChangeWithColumnAndDirection`
  - **Description:** Handles `Sorting`, and inside the handler reads the trigger column's current `SortDirection` and the args' `Column` and `Direction`.
  - **Expected result:** The handler sees the column still at its *previous* direction, `Column` is the trigger column, and `Direction` is the one about to be applied.
  - **Failure means:** The event is raised after the fact, so the documented "cancel it to own the ordering" contract cannot work — a handler would be cancelling a sort that already happened.
  - **Remarks:** Merged from two plan items; both claims are observable from one handler invocation. `TableView.idl:595-596` and `:243-249`.
- [x] `VerifySortingCancelLeavesOrderAndSortStateUnchanged`
  - **Description:** Sets `Cancel = true` in the `Sorting` handler and inspects the row order and every column's `SortDirection` afterwards.
  - **Expected result:** Neither changes.
  - **Failure means:** Cancellation is ignored, so an app that owns its own ordering gets the control's ordering on top of it.
  - **Remarks:** Merged from two plan items. `TableView.idl:251-254` says the control's sort state, the header glyph and the `Sorted` event are *all* suppressed, so the glyph is worth asserting alongside the state.
- [x] `VerifySortedFiresAfterSuccessfulSort`
  - **Description:** Handles `Sorted` and, inside the handler, reads the column state and the projected row order.
  - **Expected result:** Raised exactly once per successful sort, with `Column` and `Direction` matching, and the new order already applied when the handler runs.
  - **Failure means:** `Sorted` is not a reliable "the table is now in its new order" signal, which is the only thing it is for.
  - **Remarks:** `TableView.idl:598-599` — "Raised AFTER a sort-state change has been applied. Not cancellable." Asserting the order *inside* the handler is what makes this different from asserting it after the call returns.
- [x] `VerifySortedDoesNotFireWhenCanceled`
  - **Description:** Cancels in `Sorting` and counts `Sorted` invocations.
  - **Expected result:** Zero.
  - **Failure means:** An app that cancelled a sort is told the sort completed.
  - **Remarks:** Stated directly at `TableView.idl:252-253`.
- [x] `VerifySortEventsUseNullColumnSentinelForClearSort`
  - **Description:** Sorts a column, then calls `ClearSort()` with both events handled, and inspects the args of each.
  - **Expected result:** Both `Sorting` and `Sorted` are raised with `Column == null` and `Direction == None`.
  - **Failure means:** An app cannot distinguish a clear-all from a per-column change, or the clear happens silently — either way its own ordering drifts out of sync with the control's.
  - **Remarks:** The sentinel is documented three times (`TableView.idl:245`, `:261`, `:583`) and has no coverage at all in the current backlog.

### 7.5 Sort edge cases

- [x] `VerifyDuplicateSortMemberPathsTrackColumnIdentity`
  - **Description:** Builds two columns with the *same* `SortMemberPath`, sorts the second, and reads both columns' `SortDirection`.
  - **Expected result:** The sorted column reports its direction and the other reports `None`, even though they name the same property; the rows are ordered by that property.
  - **Failure means:** Sort state is keyed on the path instead of on column identity, so two glyphs light up for one sort — the exact shape the IDL calls out as common (a formatting column beside a ranking column).
  - **Remarks:** `TableView.idl:177-181` describes duplicate paths at length: they "neither collide nor prevent the table from being built", and state is "keyed on column identity rather than on this path".
- [x] `VerifyRemovingSortedColumnClearsSortState`
  - **Description:** Sorts a column, removes it from `Columns`, and inspects the control afterwards — row order, the remaining columns' state, and whether a further sort still works.
  - **Expected result:** The control does not throw, no remaining column claims the sort, and the table remains sortable.
  - **Failure means:** The control holds a sort keyed on a column that is gone — at best a stale order, at worst a dangling reference on the next sort.
  - **Remarks:** Not covered by the IDL. What happens to the *row order* is genuinely unspecified (restore source order, or keep the ordering with no owning column), so the test asserts the parts that are not in doubt, logs the resulting order, and raises the question in the plan.
- [x] `VerifySortReconcilesWithTableViewSourceSort`
  - **Description:** Applies a column sort and a `TableViewSource.Sort` on different properties in both orders, checking the row order and the column's `SortDirection` after each write.
  - **Expected result:** Exactly one axis is ever in force and the last writer wins; when the source is the last writer, the column's indicator is cleared.
  - **Failure means:** The two surfaces stack instead of reconciling, producing an order that matches neither and a glyph that lies.
  - **Remarks:** `TableView.idl:571-573` — "This and `TableViewSource.Sort` reconcile rather than stack: exactly one sort axis is ever in force, and the last writer wins." This is the item deliberately moved out of §8 (`VerifyLastSortWriterWinsBetweenSourceAndColumn`), and it pairs with §8's `VerifySortByKeySelectorClearsColumnSortIndicators`, which already covers the source-wins-last direction of the indicator half.
- [x] `VerifyHeaderInvokeTogglesSortAndUpdatesIndicator`
  - **Description:** Invokes the column header through its automation peer's `Invoke` pattern and inspects the column's `SortDirection`, the row order, and the header's `SortIndicator`.
  - **Expected result:** The direction advances by one step of the column's `SortCycle`, the rows reorder to match, and the indicator reflects the new direction.
  - **Failure means:** The primary sort gesture is disconnected from the sort pipeline, or the glyph does not follow the state — the table sorts with no visible reason, or shows an arrow that does not match the order.
  - **Remarks:** Scoped down from the original item: the UIA *sort metadata* half belongs to §12, which already owns `VerifyColumnHeaderHelpTextReportsSortStateOnlyForSortableColumns`. §7 keeps invoke → state → glyph. Invoke is reachable in-proc through the peer, so this stays an API test.

</details>

---

## 8. TableViewSource data shaping

<details>
<summary>Show 51 items &mdash; 48 written, 47 passing, 1 failing (product bug), 3 dropped</summary>

File: `TableViewSource_APITests.cs`.

> **Boundary with Category 2:** see §2.0. In short — §2 owns the unshaped source → rendered rows pipeline and asserts on the
> visual tree; §8 owns the shaping operators and asserts on the projection. If a test would still make sense with no shaping
> applied, it belongs in §2. §8.6 exists specifically for the notification cases that §2.4 covers unshaped.

### 8.0 Classification, and how the projection is actually observed

**All of §8 is API tests.** Every verb is a method call and every assertion is on the projected row
sequence.

**Correction to §2.0's wording.** §2.0 says §8 is "testable with no `TableView` in the tree at all".
That is wrong, and it was wrong when written: `TableViewSource` projects **only** the static `From`
and the fluent verbs. `GetItemsSourceView()` and `GetRowMetadata()` are internal C++ members
(`TableViewSource.h`), and `Rows()` belongs to `ShapedItemsSource`, which is not projected at all.
From managed code there is **no public route to the projection except a `TableView` bound to the
source**. The rest of §2.0 stands: §8 still owns the shaping algebra, and asserts on the projected
row sequence rather than on rendered cell content.

The observation route, used by every test here:

- `tableView.FindVisualChildByName("PART_RowsRepeater")` as `ItemsRepeater`.
- `repeater.ItemsSourceView.Count` — the projected row count, **including group header rows**. This is
  the projection itself, not a realization artefact, so it is immune to virtualization.
- `repeater.TryGetElement(i)` — the realized container at projected index `i`: a `TableViewRow`
  (`DataContext` is the item) or a `TableViewGroupHeader` (`DataContext` is a `TableViewGroupInfo`).
  Item counts are kept small and the host tall so every row is realized; a test that finds a null
  element fails rather than skipping.

Two consequences worth stating once:

- A `TableViewSource` test still needs `TabularControlsResources` merged and a loaded control, so
  these are UI-thread tests like every other file here.
- Where a §8 assertion could be read as "the control rendered it", the assertion is written against
  the **projected sequence** (`ItemsSourceView`), never against cell text. Cell text is §2.

### 8.1 Construction and acceptance

- [x] `VerifyFromProjectsAListInSourceOrder`
  - **Description:** `TableViewSource.From(List<Person>)`, bound to a `TableView`, projects one row per item.
  - **Expected result:** `ItemsSourceView.Count == 3` and the three row `DataContext`s are the list items in list order.
  - **Failure means:** `From` does not accept the most common source shape, or the unshaped projection is not a faithful 1:1 mirror.
  - **Remarks:** `TableViewSource.idl` lists `IVector<Object>` / `IBindableVector` / `IIterable<Object>` / `IBindableIterable` as accepted. §2.1 owns "a `TableView` accepts a `TableViewSource`"; this owns "`From` accepts a list".
- [x] `VerifyFromProjectsAnIterableOnlySource`
  - **Description:** Passes a LINQ `IEnumerable<Person>` — iterable, not a vector — to `From`.
  - **Expected result:** Every item is projected, in enumeration order.
  - **Failure means:** The accepted-shapes list in the IDL overstates what `From` takes; an app following the documented contract gets an empty table.
  - **Remarks:** `IIterable<Object>` / `IBindableIterable` are named in the IDL as accepted shapes. The interesting half is that an iterable has no change notification, so this also pins that a non-observable source is legal.
- [x] `VerifyFromTracksAnObservableCollection`
  - **Description:** `From(ObservableCollection<Person>)`, then adds an item **with no shaping verb applied**.
  - **Expected result:** The projection grows to include the new item at its source position.
  - **Failure means:** `From` did not subscribe to the source, so an unshaped `TableViewSource` is a dead snapshot.
  - **Remarks:** Distinct from §2.4, which sets an `ObservableCollection` on `ItemsSource` directly — there the subscription is `ItemsSourceView`'s, here it is `TableViewSource`'s own. The two subscriptions are different code and can regress independently.
- [x] `VerifyFromNullThrows`
  - **Description:** `TableViewSource.From(null)`.
  - **Expected result:** Throws `ArgumentException` (E_INVALIDARG) — not a null source, not an empty projection.
  - **Failure means:** The documented fail-fast contract is not honoured, and a null source becomes a silently empty table.
  - **Remarks:** `TableViewSource.idl` whole-class note: *"Null items, predicate, or keySelector throws E_INVALIDARG. (Note this differs from TableView's command surface, which no-ops on bad input.)"*
- [x] `VerifyFromUnsupportedSourceThrows`
  - **Description:** `From` with a value that implements none of the accepted collection interfaces (a boxed `int`, and a plain POCO).
  - **Expected result:** Throws `ArgumentException` for both.
  - **Failure means:** An unsupported source is accepted and silently produces no rows — the exact failure the IDL's *"Anything else throws E_INVALIDARG"* exists to prevent.
- [x] `VerifyFreshSourceIsUnshaped`
  - **Description:** A source with no verb called on it, over a deliberately unsorted list.
  - **Expected result:** Projected order is source order, count equals source count, no group header rows appear.
  - **Failure means:** `From` applies shaping nobody asked for.
  - **Remarks:** Also the baseline every other test in this file compares against, so it is worth having fail loudly on its own.
- [x] `VerifyVerbsChainAndBothShapesApply`
  - **Description:** Chains `From(...).Filter(p).Sort(path, Ascending)` in one expression and binds the returned source.
  - **Expected result:** The projection is filtered **and** sorted; chaining does not lose the earlier verb.
  - **Failure means:** The fluent surface is broken — a returned source is not the shaped one, which would make every documented usage example wrong.
  - **Remarks:** The IDL returns `TableViewSource` from every verb but does not say the return is the *same* instance, so the test asserts the chained result behaves correctly rather than asserting reference identity. Deliberate: asserting `AreSame` would freeze an implementation detail the IDL does not state.

### 8.2 Filtering

- [x] `VerifyFilterReducesProjectionToMatchingItems`
  - **Description:** Filters six people down to the three whose `Role` is `"Engineer"`.
  - **Expected result:** `ItemsSourceView.Count == 3`, and the three rows are exactly the engineers in their original relative order.
  - **Failure means:** The filter axis does not reach the projection.
  - **Remarks:** Relative order is asserted too, because a filter must not reorder — nothing in the IDL grants it that.
- [x] `VerifyClearFilterRestoresEveryItem`
  - **Description:** Filters, then calls `ClearFilter()`.
  - **Expected result:** All six rows return, in source order.
  - **Failure means:** `ClearFilter` is a no-op or leaves a partially applied predicate — the documented way to un-filter does not work.
  - **Remarks:** `TableViewSource.idl`: *"Use ClearFilter() to remove an existing filter rather than passing a pass-everything one."* That sentence makes `ClearFilter` the supported path, so it has to be the tested one.
- [x] `VerifyReplacingTheFilterReevaluatesEveryItem`
  - **Description:** Applies a `Role == "Engineer"` filter, then a second `Filter()` with a disjoint predicate (`Role == "Designer"`).
  - **Expected result:** Only designers are projected. The two predicates do **not** conjoin into an empty projection.
  - **Failure means:** `Filter` stacks instead of replacing, so a second filter can never widen the set and an app has no way back except `ClearFilter`.
  - **Remarks:** Stated as an invariant in the product's own class comment (`TableViewSource.h`): *"Single, always-first predicate: each Filter() replaces the previous predicate and runs before sort/group shaping."* Not expressible in the IDL — the IDL has one `Filter` verb and cannot say what a second call does. **This is a spec-change request:** the replace-vs-conjoin rule belongs in the IDL comment, because it is the difference between an empty table and a correct one.
- [x] `VerifyNullFilterPredicateThrows`
  - **Description:** `Filter(null)`.
  - **Expected result:** Throws `ArgumentException`; the projection is unchanged afterwards and a later valid filter still works.
  - **Failure means:** Null is treated as "no filter", which the IDL explicitly routes to `ClearFilter()` instead.
  - **Remarks:** Same whole-class E_INVALIDARG note as `From(null)`.
- [x] `VerifyThrowingFilterPredicateLeavesSourceUsable` *(needs spec decision)*
  - **Description:** Applies a predicate that throws for one specific item, then clears it and applies a valid one.
  - **Expected result:** No crash, and after `ClearFilter()` + a valid `Filter()` the projection is exactly the valid predicate's result.
  - **Failure means:** An app bug inside a predicate corrupts or wedges the source permanently, rather than costing only the rows it was asked about.
  - **Remarks:** The test deliberately does **not** assert what happens to the throwing item. Nothing in the IDL or the spec says whether a throwing predicate excludes the item, includes it, or aborts the shape, and freezing today's choice would be exactly the mistake Step 3 forbids. **Spec-change request:** state the throwing-predicate rule.
- [x] `VerifyFilterAppliesToItemsAddedLater`
  - **Description:** Filters to engineers, then adds one engineer and one designer to the observable source.
  - **Expected result:** Only the engineer appears; the projection grows by exactly one.
  - **Failure means:** The filter is applied once at declaration rather than maintained — a table that quietly drifts out of shape as data arrives.
- [x] `VerifyRemovingAFilteredOutItemLeavesProjectionUnchanged`
  - **Description:** Filters to engineers, then removes a designer from the source.
  - **Expected result:** The projected sequence is identical — same count, same items, same order.
  - **Failure means:** Excluded items still perturb the projection, which shows up in the control as rows flickering or losing selection for a change the user cannot see.

### 8.3 Grouping

- [x] `VerifyGroupByProjectsOneHeaderPerDistinctKey`
  - **Description:** Groups six people by `Role` over three distinct roles.
  - **Expected result:** `ItemsSourceView.Count == 9` (3 headers + 6 items); each header is a `TableViewGroupHeader` whose `TableViewGroupInfo.Key` is the role, immediately followed by exactly the items with that role.
  - **Failure means:** Grouping does not reach the projected row axis, or headers and members are interleaved wrongly.
  - **Remarks:** §9 owns what a group header *renders* and what its peer exposes; this owns the projected sequence — that headers exist, one per key, in the right place.
- [x] `VerifyGroupByPreservesFirstAppearanceGroupOrder` *(needs spec decision)*
  - **Description:** Groups a list whose keys first appear in a non-alphabetical order.
  - **Expected result:** Groups are projected in order of first appearance in the source, not sorted by key.
  - **Failure means:** Group order is arbitrary, so a grouped table reorders itself for reasons the app cannot predict or control.
  - **Remarks:** The IDL does not state a default group order. It states only the *relative* rule — *"Sorts requested before GroupBy establish the group order"* (`ShapedItemsSource.cpp`, mirrored from the Sort contract) — which implies an unsorted default must come from the source, and first-appearance is the only source-derived order available. Thin reasoning; **spec-change request:** state the default group order in `TableViewSource.idl`.
- [x] `VerifyGroupByWithIdentitySelectorGroupsReferenceKeysByValue`
  - **Description:** Group key is a reference type (a `Department` object) minted fresh per item, with a `groupIdentitySelector` returning its name.
  - **Expected result:** Items with equal department *names* land in one group — the number of groups equals the number of distinct names, not the number of key instances.
  - **Failure means:** The identity selector is ignored, and the documented escape hatch for reference-type keys does nothing.
  - **Remarks:** `TableViewSource.idl`: *"groupIdentitySelector is OPTIONAL: null selects the built-in value-type group identity … a reference-type group key with no selector fails fast at projection time."* The **negative** half of that sentence is not testable here — see the drop below.
  - **Status: passing after a test fix.** First run failed on the key text, not the grouping: three groups were formed correctly, but `TableViewGroupInfo.KeyText` for a reference key is the key's `ToString()`, and the test's `Department` had none, so every header read as the type name. Fixed by giving `Department` a `ToString()` returning `Name`, which is what a real app would do. **Lesson: a reference-type group key needs `ToString()` for the default header template to say anything useful** — arguably itself worth a doc note.
- [x] ~~`VerifyReferenceGroupKeyWithoutIdentitySelectorFailsFast`~~ **(dropped — not runnable in chk)**
  - The fail-fast path in `RebuildGrouped` raises `MUX_ASSERT_MSG(false, ...)` *before* it throws. In a `chk` build that assert takes down the test host, so the test cannot report a result; in a `fre` build it would pass. Per the AGENTS.md rule about deliberately driving an asserted path, it is not written. The positive half is covered by the test above. **Worth revisiting** if the assert is downgraded to a log — the throw itself is well specified and easy to assert.
- [x] `VerifyClearGroupByRestoresFlatRows`
  - **Description:** Groups, then calls `ClearGroupBy()`.
  - **Expected result:** `ItemsSourceView.Count` returns to the item count, no element is a `TableViewGroupHeader`, and the order is the source order.
  - **Failure means:** Grouping cannot be undone, or undoing it leaves orphaned header rows in the projection.
- [x] `VerifyNullGroupKeySelectorThrows`
  - **Description:** `GroupBy(null)`.
  - **Expected result:** Throws `ArgumentException`; a subsequent valid `GroupBy` still works.
  - **Failure means:** Null is silently treated as "ungroup", which the IDL explicitly routes to `ClearGroupBy()`.
  - **Remarks:** `TableViewSource.idl`: *"keySelector: required, non-null (throws E_INVALIDARG when null; use ClearGroupBy() to remove grouping)."*
- [x] `VerifyGroupsSurviveAnAddToAnExistingGroup`
  - **Description:** While grouped, adds an item whose key matches an existing group.
  - **Expected result:** The group's member count grows by one, the item is projected inside that group's span, and no new header appears.
  - **Failure means:** A live add under grouping either lands outside its group or duplicates a header.
  - **Remarks:** The new-group case is §8.6's; this is the in-place case, which takes a different path in the engine (bucket append vs. bucket creation).

### 8.4 Sorting

- [x] `VerifySortByPathOrdersRows`
  - **Description:** `Sort("Name", Ascending)` then `Sort("Name", Descending)` over an unsorted list.
  - **Expected result:** Ascending yields the names in ordinal ascending order; descending yields the exact reverse.
  - **Failure means:** The path sort axis does not reach the projection, or direction is ignored.
  - **Remarks:** `TableViewSource.idl` calls the path overload *"the preferred form"* and says it is evaluated *"by the same binding-based evaluator a column uses for its SortMemberPath"*.
- [x] `VerifySortByDottedPathOrdersRows`
  - **Description:** Sorts on a nested path (`"Department.Name"`).
  - **Expected result:** Rows are ordered by the nested value.
  - **Failure means:** The path evaluator is not the binding evaluator the IDL promises, so *"a path that displays also sorts"* is false.
  - **Remarks:** Expectation quoted from `TableViewSource.idl`: *"dotted paths and indexers included"*.
- [x] `VerifySortByKeySelectorOrdersRows`
  - **Description:** Sorts by a computed key no property path expresses (name length, then a tie-break the test controls).
  - **Expected result:** Rows are ordered by the computed key.
  - **Failure means:** The key-selector overload is not wired to the pipeline, removing the only route for computed and multi-field sorts.
- [x] `VerifySortDirectionNoneRemovesThatAxis`
  - **Description:** Sorts ascending by `Name`, then re-declares the same path with `SortDirection.None`.
  - **Expected result:** Source order is restored.
  - **Failure means:** `None` seeds an axis instead of removing one — the IDL's stated distinction — so an app cannot unsort a single axis.
  - **Remarks:** `TableViewSource.idl`: *"SortDirection.None removes that key's sort axis rather than seeding a stage, so sorting every axis to None leaves the source unsorted."*
- [x] `VerifyClearSortRestoresSourceOrder`
  - **Description:** Declares two sort axes, then calls `ClearSort()`.
  - **Expected result:** Source order is restored in full — both axes go, not just the last one.
  - **Failure means:** `ClearSort` clears only the primary axis, leaving a table that is still sorted after the app unsorted it.
- [x] `VerifyFirstDeclaredSortAxisIsPrimary`
  - **Description:** `Sort("Role", Ascending)` then `Sort("Name", Ascending)` over data where the two disagree.
  - **Expected result:** Rows are grouped by role first, with names ascending inside each role — i.e. the **first** declared axis is primary and the second breaks ties.
  - **Failure means:** Multi-axis precedence is inverted, which silently produces a differently ordered table than every WPF-derived app expects.
  - **Remarks:** `TableViewSource.idl`: *"When several axes are active the FIRST one declared is the primary sort and each later axis breaks ties within the previous, matching WPF DataGrid's SortDescriptions order."*
- [x] `VerifyResortingAPathKeepsItsAxisPosition`
  - **Description:** Declares `Role` then `Name`, then re-declares `Role` as `Descending`.
  - **Expected result:** `Role` is still the primary axis (now descending) with `Name` still breaking ties; it does **not** move to last place, and no third axis appears.
  - **Failure means:** Re-sorting an existing axis stacks a duplicate or demotes the axis, so toggling a sort direction reorders the whole precedence chain.
  - **Remarks:** Two IDL sentences: *"re-sorting an existing axis keeps its position"* and *"Re-sorting the same path replaces that axis in place rather than adding a second one."*
- [x] `VerifyEmptySortMemberPathThrows`
  - **Description:** `Sort("", Ascending)`.
  - **Expected result:** Throws `ArgumentException`; the projection is unchanged.
  - **Failure means:** An empty path is accepted and produces an axis that can never order anything.
  - **Remarks:** `TableViewSource.idl`: *"Throws E_INVALIDARG when sortMemberPath is empty."*
- [x] `VerifyNullSortKeySelectorThrows`
  - **Description:** `Sort((TableViewKeySelector)null, Ascending)`.
  - **Expected result:** Throws `ArgumentException`.
  - **Failure means:** The whole-class null contract has a hole in the sort overload.
- [x] `VerifySortByKeySelectorClearsColumnSortIndicators`
  - **Description:** Sorts a column through `TableView.SortByColumn`, then declares an anonymous key-selector sort on the source.
  - **Expected result:** Every column's `SortDirection` is `None` afterwards — no indicator is left describing an order that no longer holds.
  - **Failure means:** A header keeps claiming a sort the rows no longer follow, which is a correctness bug for sighted users and for the UIA sort metadata alike.
  - **Remarks:** `TableViewSource.idl`: *"The axis is ANONYMOUS: nothing names a property, so a bound TableView cannot attribute it to a column and clears every sort indicator instead of leaving one describing a sort that is no longer primary."* The only §8 test that asserts on a column rather than the projection, because the contract is stated about columns.

### 8.5 Composition

- [x] `VerifyFilterAndSortCompose`
  - **Description:** Filters to engineers and sorts by `Name` descending.
  - **Expected result:** Only engineers, in descending name order — the filtered-out items influence neither membership nor order.
  - **Failure means:** The two axes are applied in the wrong order or one clobbers the other.
  - **Remarks:** `TableViewSource.h` states the ordering invariant: the filter *"runs before sort/group shaping"*.
- [x] `VerifySortBeforeGroupByOrdersTheGroups`
  - **Description:** Declares `Sort("Role", Descending)` **before** `GroupBy(Role)`.
  - **Expected result:** The group headers are projected in descending role order.
  - **Failure means:** A sort declared before grouping does not establish group order, so an app has no way to order its groups.
  - **Remarks:** From the ordering rule the grouped rebuild states as an invariant: *"Sorts requested before GroupBy establish the group order. Sorts requested after GroupBy are applied per bucket."* Not in the IDL — **spec-change request**, because the observable behaviour of a two-verb chain depends on declaration order and nothing public says so.
- [x] `VerifySortAfterGroupByOrdersWithinEachGroup`
  - **Description:** The same two verbs in the opposite order: `GroupBy(Role)` then `Sort("Name", Ascending)`.
  - **Expected result:** Group order is unchanged (first appearance), and members are name-ascending **inside** each group; no item crosses a group boundary.
  - **Failure means:** A post-group sort flattens or reorders across groups, destroying the grouping.
  - **Remarks:** Same source as above. Keeping both directions is the point: the pair is what makes the ordering rule observable at all.
- [x] `VerifyFilterGroupAndSortCompose`
  - **Description:** All three verbs at once — filter out one role, group by another field, sort inside the groups.
  - **Expected result:** The exact projected sequence, headers included, asserted element by element.
  - **Failure means:** The verbs compose pairwise but not in threes — typically a rebuild path that re-applies one axis against the wrong intermediate vector.
  - **Remarks:** The one test here that asserts the whole sequence literally, deliberately: composition bugs show up as a *plausible* order, which a spot check passes.
- [x] ~~`VerifyLastSortWriterWinsBetweenSourceAndColumn`~~ **(dropped — owned by §7)**
  - §7 already has `VerifySortReconcilesWithTableViewSourceSort` for the last-writer-wins rule between `TableView.SortByColumn` and `TableViewSource.Sort`. Two tests for one rule, differing only in which side writes last, is the redundancy Step 2.4 rejects. §7 owns it because the rule is about the *control's* sort state.

### 8.6 Live updates while shaped

- [x] `VerifyAddWhileFilteredRespectsThePredicate`
  - **Description:** Adds a matching and a non-matching item while a filter is active.
  - **Expected result:** Only the matching item enters the projection.
  - **Failure means:** Live adds bypass the filter axis.
- [x] `VerifyAddWhileSortedLandsAtTheSortedPosition`
  - **Description:** Adds an item whose key belongs in the middle while a sort is active.
  - **Expected result:** It is projected at its sorted position, not appended.
  - **Failure means:** The incremental sorted-insert path splices at the source index — the exact bug `TryApplyIncrementalSortedChange` exists to avoid.
- [x] `VerifyAddWhileGroupedCreatesTheMissingGroup`
  - **Description:** Adds an item whose key matches no existing group.
  - **Expected result:** A new header appears with that key and the item inside it; existing groups keep their members and relative order.
  - **Failure means:** A new key is dropped, misfiled into an existing group, or forces a full reorder of the groups.
  - **Remarks:** Pairs with §8.3's existing-group add; the two take different engine paths.
- [x] `VerifyRemoveWhileShapedDropsOnlyThatRow`
  - **Description:** Removes a projected item while filtered and sorted.
  - **Expected result:** Count drops by one, order of the remaining rows is unchanged.
  - **Failure means:** A removal rebuilds the projection differently than the incremental path, so rows move for a change that should be local.
- [x] `VerifyReplaceWhileShapedReevaluatesTheNewItem`
  - **Description:** Replaces an item with one that both fails the filter and would sort elsewhere.
  - **Expected result:** The replacement is evaluated as a new item — it is excluded by the filter, and the projection's remaining order is unchanged.
  - **Failure means:** Replace is treated as an in-place value update and the new item inherits the old one's position and membership.
- [x] `VerifyMoveWhileSortedDoesNotChangeProjectedOrder`
  - **Description:** Moves an item within the source while a sort is active.
  - **Expected result:** The projected order is byte-for-byte identical before and after.
  - **Failure means:** Source position leaks into a sorted projection.
- [x] `VerifyResetWhileShapedRebuildsUnderTheSameShape`
  - **Description:** Clears and refills an `ObservableCollection` (raising Reset) while a filter and a sort are active.
  - **Expected result:** The new contents appear filtered and sorted — the shape survives the reset.
  - **Failure means:** A reset drops the declared shape, so the table silently reverts to source order.
  - **Remarks:** §2.4 owns reset on an **unshaped** source; this is the case §2.0 reserved for §8 — a reset that must be survived *by active shaping*.
- [x] `VerifySortKeyPropertyChangeDoesNotMoveTheRowUntilACollectionChange`
  - **Description:** While sorted by `Name`, renames an item through `INotifyPropertyChanged` only, asserts the position, then raises any collection change and asserts again.
  - **Expected result:** The row stays at its old sorted position after the property change, and moves to the correct position after the collection change.
  - **Failure means:** Either the documented invariant is broken (the row moved early, which means rows can reorder under the user's cursor), or re-shaping never happens at all (the row never moves).
  - **Remarks:** **This inverts the original plan item, which expected the row to move.** The product states the opposite as an invariant, in `ShapedItemsSource.cpp`: *"Invariant: re-sorting is driven by collection notifications on this path. An in-place mutation of a row's sort-key field that raises only INotifyPropertyChanged leaves the row at its old sort position until the next collection change, matching XAML ItemsControl sources."* That is an invariant the product asserts about itself and matches the platform precedent it names, so it is a legitimate source under Step 3. **Spec-change request:** this belongs in `TableViewSource.idl`, since it is the single most surprising thing about the shaping contract.
  - **Status: FAILING — product bug (open failure 10).** The first half holds; the second does not. The collection change is an incremental insert, so the stale row is never re-placed: saw `[Bea, Zara, Diego, Ines, Mei, Owen, Rafa]`, expected `[Bea, Diego, Ines, Mei, Owen, Rafa, Zara]`. `VerifyGroupKeyPropertyChangeFollowsTheSameInvariant` proves the grouped path *does* re-shape on the next collection change, so the two paths disagree. Left failing.
- [x] `VerifyFilterMembershipPropertyChangeFollowsTheSameInvariant`
  - **Description:** The same experiment for filter membership: change a filtered property via INPC only, then raise a collection change.
  - **Expected result:** Membership is unchanged by the property change and correct after the collection change.
  - **Failure means:** Filter membership and sort position follow different re-shape rules, which is worse than either rule alone — an app cannot reason about when its shape is true.
  - **Remarks:** The invariant above is stated for sorting. Applying it to filtering is an extrapolation, flagged as such: *(needs spec decision)* on whether one rule covers all axes.
- [x] `VerifyGroupKeyPropertyChangeFollowsTheSameInvariant` *(needs spec decision)*
  - **Description:** The same experiment for a group key.
  - **Expected result:** The row stays in its old group until a collection change, then moves.
  - **Failure means:** Grouping diverges from the stated re-shape rule.
  - **Remarks:** Same extrapolation as above. Kept because a grouped row that half-moves — item relocated, header counts stale — is a visible corruption, and this is the only test that would catch it.
  - **Status: passing after a test fix, and it is the control for open failure 10.** The grouped path behaves exactly as the invariant describes: no move on the INPC change, full re-bucket with refreshed counts on the next collection change. The first run failed only on the expected sequence — `RebuildGrouped` rebuilds every bucket from **source order**, so the re-bucketed row takes its source position inside the group (`Diego, Mei, Rafa, Owen`) rather than being appended. Expectation corrected. **That behaviour is the direct counter-example to the sorted flat path**, which never re-shapes at all.
- [x] `VerifyBurstOfMutationsWhileShapedSettlesCorrectly`
  - **Description:** Twelve interleaved adds, removes and replaces in one UI-thread turn, under an active filter and sort.
  - **Expected result:** No crash, and the final projection equals the shape computed from the final source contents.
  - **Failure means:** The re-entrancy and coalescing guards (`m_isRefreshing`, `m_isApplyingIncrementalChange`, `m_pendingRefresh`) do not converge — the projection ends up describing an intermediate state that no longer exists.
  - **Remarks:** The expected value is computed in the test from the final source, not hardcoded, so the test stays true if the data changes.

### 8.7 Identity and state

- [x] `VerifyDuplicateItemObjectThrowsWhenShaped`
  - **Description:** Puts the same `Person` instance in the source twice, then applies a filter that keeps both.
  - **Expected result:** Throws `ArgumentException` when the shaping verb materializes the projection.
  - **Failure means:** Two rows backed by one object are projected, and every identity-anchored operation — selection, focus re-anchoring, incremental updates — silently addresses the wrong row.
  - **Remarks:** `TableViewSource.idl`: *"the same object in two rows fails fast at materialization"*. §2.6 owns the **benign unshaped** case (the same instance twice yields two independent rows), per the §2.0 seam table; this owns the shaped failure. Also pins *where* it fails: the verb call, not `From`, because an unshaped mirror needs no identity.
- [x] `VerifySelectionReanchorsAcrossAReshape`
  - **Description:** Selects a row, then changes the sort so that row moves.
  - **Expected result:** `SelectedItem` is still the same object and `SelectedIndex` is its **new** projected index.
  - **Failure means:** Selection is index-anchored, so a reshape silently moves the selection to a different record — a data-loss-shaped bug in an editable table.
  - **Remarks:** `TableViewSource.idl`: *"Rows are identified by their item's OBJECT identity … So selection and focus re-anchor across a reshape, but NOT across an item being re-created."*
- [x] `VerifySelectionIsNotReanchoredAcrossItemRecreation`
  - **Description:** Selects a row, then replaces that item with an equal-valued **new** instance.
  - **Expected result:** The selection does not follow the new instance — it is cleared or moved off, per the IDL's "but NOT across an item being re-created".
  - **Failure means:** Identity is being derived from value rather than object identity, which contradicts the stated row-identity model and would make the duplicate-object rule above incoherent.
  - **Remarks:** The negative half of the same sentence. Asserts only that the selection is not the new instance — the IDL does not say whether it clears or moves, so the test does not pin which. *(needs spec decision)* on the exact post-condition.
- [x] ~~`VerifyFocusReanchorsAcrossReshape`~~ **(dropped — moved to the interaction plan)**
  - Focus re-anchoring needs a real focused element surviving a reshape; setting focus programmatically on a virtualized row that is about to be re-projected is exactly the kind of assertion that passes for the wrong reason in-proc. Recorded in the interaction plan next to the other focus items.


</details>

---

## 9. Grouping and group headers

<details>
<summary>Show 20 items &mdash; 20 written, 18 passing, 2 failing on one product finding &mdash; <strong>complete</strong></summary>

File: `TableView_Grouping_APITests.cs`.

### 9.0 Boundary with Categories 8 and 12

Grouping is already partly covered, and §9 must not re-test either neighbour:

| Already covered | Where |
|---|---|
| `GroupBy` projection — one header per distinct key, group order, identity selector, `ClearGroupBy`, add-creates-group, group-key property change | §8, on `TableViewSource`. §9 assumes the projection is right and tests what the **control** does with it. |
| Group header **automation** — ExpandCollapse pattern, its state, idempotent expand/collapse, `GridItem` spanning, peer name, control type `Group` | §12. §9 keeps the *visual and layout* halves: what the header renders and how wide it is. |

What is left for §9 is the `TableViewGroupInfo` projection itself, group header content templating, the control-level expand/collapse verbs, the header's `ToggleRequested` event, and the header's visual states.

### 9.1 TableViewGroupInfo

- [x] `VerifyGroupInfoReportsKeyItemCountAndLevel`
  - **Description:** Groups a source by a string property and reads `Key`, `ItemCount` and `Level` off the `TableViewGroupInfo` carried as each group header's `Content`.
  - **Expected result:** `Key` is the group key, `ItemCount` is the number of members of that group, and `Level` is `0`.
  - **Failure means:** The header's binding source misreports the group it belongs to, so every default and app-authored header template shows the wrong thing.
  - **Remarks:** Merged from three plan items reading three properties of one object. `TableView.idl:335-337` defines all three, including "`Level` — 0 for single-level v1", which is why `Level` is asserted as a constant rather than as a computed depth.
- [x] `VerifyGroupInfoExpandabilityDefaults`
  - **Description:** Reads `IsExpandable` and `IsExpanded` on a freshly grouped source.
  - **Expected result:** Every group with members is expandable and starts expanded.
  - **Failure means:** A grouped table opens fully collapsed, or offers no expander at all — either way the data is unreachable without an app writing code the control should not need.
  - **Remarks:** `TableView.idl:338` says `IsExpandable` is "false when the group can't expand (e.g. empty)"; groups produced by `GroupBy` always have at least one member, so the expectation here is the positive case. The negative case is not reachable through `GroupBy` — see the drop below.
- [x] `VerifyGroupInfoKeyTextAndItemCountText`
  - **Description:** Reads `KeyText` and `ItemCountText` for a known group.
  - **Expected result:** `KeyText` is the key's display string and `ItemCountText` contains the member count.
  - **Failure means:** The built-in header content template, which binds these two, renders blanks or raw type names.
  - **Remarks:** `TableView.idl:341-344` — "Culture-formatted display strings for the built-in content template. Computed lazily." The exact `ItemCountText` wording is a localized resource, so the test asserts it is non-empty and contains the count rather than pinning a sentence. §8 already established that a reference key needs `ToString()`; this fixture uses string keys.
- [x] `VerifyGroupInfoRaisesPropertyChangedOnExpandCollapse`
  - **Description:** Subscribes to `PropertyChanged` on a group's `TableViewGroupInfo`, then collapses that group.
  - **Expected result:** `PropertyChanged` is raised for `IsExpanded`, and the property reads back collapsed.
  - **Failure means:** An app-authored header template binding `IsExpanded` (to rotate its own chevron, for example) never updates.
  - **Remarks:** `TableViewGroupInfo` implements `INotifyPropertyChanged` in its own declaration (`TableView.idl:333`), and the class comment says it is "updated in place and raises PropertyChanged, so recycling does not re-evaluate every binding in the header template".
- [x] `VerifyGroupInfoItemCountUpdatesInPlaceWithNotification`
  - **Description:** Adds an item to an existing group of a live source and watches the same `TableViewGroupInfo` instance.
  - **Expected result:** The group the header is bound to reports the new `ItemCount`, and `PropertyChanged` was raised for `ItemCount` on the instance that was bound before the mutation.
  - **Failure means:** Either the count goes stale, or the group projection is rebuilt without notifying — which is the exact thing the "updated in place" note exists to prevent, and which silently breaks every binding in a realized header.
  - **Remarks:** Notification is the assertable half. Instance identity is **not**: the info objects are pooled and rewritten across groups, so a reference captured for group 0 can legitimately be carrying another group's values afterwards — that pooling is what `TableView.idl:329-330` means by "updated in place … so recycling does not re-evaluate every binding". State is therefore re-read through the header, not through the captured reference.

### 9.2 Group header rendering

- [x] `VerifyDefaultGroupHeaderShowsKeyAndCount`
  - **Description:** Loads a grouped table with no `GroupHeaderTemplate` and reads the text rendered inside the first group header.
  - **Expected result:** The key text appears, and the member count appears.
  - **Failure means:** A grouped table renders anonymous bands — the user sees the rows split up with no indication of what splits them.
  - **Remarks:** The default template is seeded by the header's own `Style` (`TableView.idl:381-383`). Asserting rendered text rather than the template object is what makes this different from §9.1.
- [x] `VerifyGroupHeaderTemplateApplies`
  - **Description:** Sets `TableView.GroupHeaderTemplate` to a template with a recognisable marker bound to `KeyText`, and inspects a realized header.
  - **Expected result:** The header's `ContentTemplate` is that template and the marker is rendered.
  - **Failure means:** The documented customisation point does nothing.
  - **Remarks:** `TableView.idl:380-383`: `GroupHeaderTemplate` "overrides it as a local value". The template binds `KeyText`, which also proves the `Content` really is the `TableViewGroupInfo`.
- [x] `VerifyGroupHeaderTemplateChangeAfterLoadUpdatesLive`
  - **Description:** Replaces `GroupHeaderTemplate` on a loaded, grouped table.
  - **Expected result:** Realized headers rebuild against the new template.
  - **Failure means:** Template changes only take effect on the next realization, so a theme or view switch leaves a mix of old and new headers on screen.
  - **Remarks:** Mirrors §2.3's `VerifyCellTemplateChangeAfterLoadRebuildsRealizedCells` — the same class of bug, one level up. **FAILING — product finding.** `TableView::PrepareGroupHeaderElement` applies `GroupHeaderTemplate` only while preparing a container (`TableView_Grouping.cpp:690-697`), and nothing re-applies it when the property changes, so already-realized headers keep the old template until they are recycled. Test left failing.
- [x] `VerifyClearingGroupHeaderTemplateRevertsToTheStyleDefault`
  - **Description:** Sets `GroupHeaderTemplate`, confirms it applied, then clears it back to `null`.
  - **Expected result:** Headers return to rendering the default key-and-count content.
  - **Failure means:** Customisation is one-way — an app that clears the property is left with blank header bands.
  - **Remarks:** `TableView.idl:381-383` states the relationship precisely: the override is "a local value that `ClearValue` reverts back to the Style setter". This item is new; the original backlog had no revert case at all. **FAILING — same product finding as the item above.** The `ClearValue` branch exists (`TableView_Grouping.cpp:694-697`) but only runs at prepare time, so clearing the property leaves realized headers on the app template.
- [x] `VerifyGroupHeaderSpansAllColumns`
  - **Description:** Measures a realized group header against the table's column set.
  - **Expected result:** The header's width covers the full set of columns rather than the width of one column.
  - **Failure means:** Group bands render as a cell-sized stub, which reads as a data row rather than as a section break.
  - **Remarks:** §12's `VerifyGroupHeaderGridItemSpansAllVisibleColumns` asserts the *UIA* `ColumnSpan`. This is the layout half, which a UIA-only assertion would not catch. The yardstick is the **sum of column widths**, not the row's `ActualWidth`: the band sizes itself in `TableViewGroupHeader::MeasureOverride` from the visible-column sum (`TableView_Grouping.cpp:768-779`) while a row's border stretches on to the viewport edge, so a header-versus-row comparison fails on a table wider than its columns.

### 9.3 Expand and collapse

- [x] `VerifyExpandAndCollapseAllOnUngroupedSourceAreNoOps`
  - **Description:** Calls `ExpandAllGroups()` and `CollapseAllGroups()` on a flat, ungrouped source.
  - **Expected result:** Neither throws, and the rows are unchanged.
  - **Failure means:** A generic view-model that calls these unconditionally takes the app down, or silently hides the rows of a table that has no groups.
  - **Remarks:** Merged from two plan items testing one sentence: "Both are no-ops when the source is not grouped" (`TableView.idl:557-559`).
- [x] `VerifyCollapseAllGroupsHidesDataRowsKeepsHeaders`
  - **Description:** Calls `CollapseAllGroups()` on a grouped table and reads the projected rows.
  - **Expected result:** Only group headers remain projected — one per group, no data rows — and each group reports `IsExpanded == false`.
  - **Failure means:** Collapse is cosmetic: the chevrons turn but the rows stay, which is worse than no collapse at all on a large table.
  - **Remarks:** Read through the repeater projection rather than through realized containers, so virtualization cannot make a partial collapse look complete.
- [x] `VerifyExpandAllGroupsRestoresDataRows`
  - **Description:** Collapses everything, then calls `ExpandAllGroups()`.
  - **Expected result:** The full header-and-row projection returns, in the same order as before the collapse.
  - **Failure means:** Rows are lost or reordered by a collapse/expand round trip.
  - **Remarks:** Asserts against the sequence captured before the collapse, so the round trip is compared with its own starting point rather than a hardcoded list.
- [x] `VerifyToggleSingleGroupUpdatesOnlyThatGroup`
  - **Description:** Collapses one group through its header's ExpandCollapse provider, and inspects every group.
  - **Expected result:** That group's rows disappear; every other group keeps its rows and its expanded state.
  - **Failure means:** Per-group state is actually global, so collapsing one section collapses the table.
  - **Remarks:** `TableViewGroupHeader.IsExpanded` (`TableView.idl:384-385`) is a **state mirror, not an actuator** — setting it updates the header and its group info but never reshapes, because the reshape runs through the owner (`TableView::ToggleGroupExpansion`, reached from `RequestToggle`/`RequestExpansion`). The input-free actuator is therefore the header peer's `Collapse()`/`Expand()`. §12 owns the peer's own contract; §9 only uses it to drive the control.
- **Dropped:** `VerifyGroupHeaderToggleRequestedCarriesTheGroupKey` &mdash; moved to the interaction plan. `TableViewGroupHeader::RequestToggle` is called only from `OnKeyDown` (Enter/Space) and `OnPointerReleased`; the automation peer's Expand/Collapse takes the separate `RequestExpansion` route by design (`TableViewGroupHeader.cpp:156-225,290`). There is no input-free way to reach `ToggleRequested`, and weakening the assertion to "the event type exists" would prove nothing. `TableView.idl:350-355` still stands: the key is carried "so a handler that re-enters and mutates the header still sees the key that was actually activated" &mdash; verified under gesture in the interaction plan.
- [x] `VerifyCollapsedGroupStatePersistsAcrossSourceUpdate`
  - **Description:** Collapses one group, then adds an item to a *different* group.
  - **Expected result:** The collapsed group is still collapsed and still contributes no data rows.
  - **Failure means:** Any background data update silently re-expands everything the user collapsed.
  - **Remarks:** §8 already proves the projection survives the add; what is new here is that the expansion state survives with it.

### 9.4 Group edge cases

- [x] `VerifySingleItemGroupRenders`
  - **Description:** Groups a source where one key has exactly one member.
  - **Expected result:** That group projects a header and exactly one data row, and reports `ItemCount == 1`.
  - **Failure means:** Single-member groups are elided or folded into a neighbour — a plausible off-by-one in group-boundary logic.
  - **Remarks:** The fixture deliberately contains one single-member group and two larger ones, so boundary handling is exercised at both ends.
- [x] `VerifyNonExpandableGroupIgnoresToggle`
  - **Description:** Sets `IsExpandable = false` on a realized header, then attempts to collapse it.
  - **Expected result:** The group stays expanded and its rows stay projected.
  - **Failure means:** The expandability flag is decorative — a header that shows no expander still collapses, leaving no way to get the rows back.
  - **Remarks:** `IsExpandable` is a settable DP on the header (`TableView.idl:386-387`), which is the only public route to a non-expandable group: `GroupBy` never produces one.
- [x] `VerifyGroupRemovedWhenLastItemRemoved`
  - **Description:** Removes the only member of a group from a live grouped source.
  - **Expected result:** That group's header disappears from the projection; the other groups are untouched.
  - **Failure means:** Empty group bands accumulate as data is filtered or deleted.
  - **Remarks:** The mirror of §8's `VerifyAddWhileGroupedCreatesTheMissingGroup`, which has no removal counterpart anywhere today.
- [ ] ~~`VerifyEmptyGroupRendersHeaderOnly`~~ **(dropped — not reachable)**
  - A group exists only because an item produced its key, so `GroupBy` cannot produce an empty group; the empty case named at `TableView.idl:338` has no public route. The reachable part of that sentence — a header that declares itself non-expandable — is covered by `VerifyNonExpandableGroupIgnoresToggle`.

### 9.5 Group header visual states

- [x] `VerifyGroupHeaderExpansionVisualStates`
  - **Description:** Reads the header's `ExpansionStates` group after collapsing and after re-expanding.
  - **Expected result:** `Collapsed` while collapsed, `Expanded` while expanded.
  - **Failure means:** The chevron does not track the state it describes, so the band lies about whether rows are hidden.
  - **Remarks:** `TableView.idl:372` names the group and both states. The glyph itself is "driven by `ExpansionStates`, not by code" (`:368`), so the state name is the right thing to assert.
- [x] `VerifyGroupHeaderExpandabilityVisualStates`
  - **Description:** Reads the `ExpandabilityStates` group with `IsExpandable` true and false.
  - **Expected result:** `Expandable` and `NotExpandable` respectively.
  - **Failure means:** A group that cannot expand still advertises an expander.
  - **Remarks:** `TableView.idl:373`.
- `(moved to interaction plan)` `VerifyGroupHeaderPointerAndPressedVisualStates` — `PointerOver` and `Pressed` are set only from the header's own pointer handlers, so there is no programmatic route into either state. Same finding that moved two §5.5 items. Tracked as `GroupHeaderPointerAndPressedVisualStates`.

</details>

---

## 10. Editing

<details>
<summary>Show 17 items &mdash; 17 written, 17 passing (8 further items are gesture-bound, see the interaction plan)</summary>

File: `TableView_Editing_APITests.cs`.

### 10.0 Classification: how editing is reachable without a gesture

`TableView.idl:513-516` states that *"editing starts from the user: double-click, or F2 on the current cell. There is no
programmatic `BeginEdit` in this release, and therefore no public current-cell surface for one to target."* Read alone, that
sentence pushes the whole category into §11, because every other editing API — `CommitEdit`, `CancelEdit`, `BeginningEdit`,
`CellEditEnding`, validation — presupposes an edit that is already open.

Two public routes reach the edit lifecycle without an input gesture. Both are spec surface, not implementation back doors.

**Route 1 — UIA `IValueProvider` on the cell peer.** `TableViewCellAutomationPeer` offers `PatternInterface.Value`, and its
`SetValue` runs the real sequence: `BeginEdit(item, column)` → locate the editing element → set its text → `CommitEdit()`. The
source comment states the intent directly: *"Drive the real edit lifecycle rather than writing the source directly, so a
`BeginningEdit` handler can still veto and `CellEditEnding`/validation still run — a programmatic set must not be able to do what
a user cannot."* A test that calls `SetValue` therefore exercises the same path a double-click does, minus the gesture.

The pattern is offered only when `SupportsValuePattern()` holds: the column is not `IsReadOnly`, the owner is not `IsReadOnly`,
the column is a `TableViewTextColumn`, and the column has no `CellEditingTemplate`. That gate is itself the specified behaviour —
*"advertising it elsewhere tells assistive technology it can set a value, then fails after opening an edit"* — so the negative
cases are assertable as `IsReadOnly == true` on the peer and a thrown `SetValue`, without needing an edit to open at all.

**Route 2 — a validation-blocked commit holds the edit open.** When `INotifyDataErrorInfo` reports an error for the edited
property, the commit returns false and, per the IDL, *"a veto or failed validation leaves the edit open and `IsEditing` true."*
That is the only API-reachable way to obtain a **persistently open** edit, and it is what makes the cancel-action, source-reset
and unload teardown tests writable as API tests. Tests that need an open edit should reach it this way and say so.

**Consequence for this category.** 16 of the 26 backlog items are API tests. The 8 that remain gesture-bound move to §11. Two
are dropped as already covered.

### 10.0.1 Dropped and merged

- **Dropped — `VerifyEditingDefaultsAreReadOnly`.** Fully covered by Category 1: `TableViewTests.cs:130-131` asserts
  `IsReadOnly == true` and `IsEditing == false`, `:160-161` asserts `TableViewColumn.IsReadOnly == false` and
  `CellEditingTemplate == null`, `:246`/`:272-273` assert the DP statics, and `:323-325`/`:388-393` assert the settable
  roundtrips. Re-asserting them here would add a second owner for one contract.
- **Merged — `VerifyEditGestureIsIgnoredWhenReadOnly` + `VerifyEditEnabledWhenControlAndColumnAreWritable`** into
  `VerifyReadOnlyTableViewOffersNoValuePatternAndRejectsEditing` and its writable counterpart. The backlog phrased the negative
  case as a gesture; the specified observable is that the control refuses to open an edit, which the peer expresses directly and
  more strongly (the refusal is visible to assistive technology, not merely to a mouse).
- **Reframed — `VerifyTemplateColumnUsesCellEditingTemplate`.** The editor *content* is gesture-bound (§11). What is API-testable
  is the specified gate: a column carrying a `CellEditingTemplate` must not advertise the Value pattern. Kept here under that
  name.
- **Reframed — `VerifyCancelEditRestoresOriginalValue`.** Split. The no-edit-open return value is API-testable
  (`VerifyCommitAndCancelEditReturnFalseWhenNoEditIsOpen`), and the restore-to-original semantics are covered by the validation
  rollback test, which the IDL requires to leave the item holding its pre-edit value. Cancelling a *user-typed* edit stays in
  §11.
- **Note — the backlog said `TableViewTemplateColumn.CellEditingTemplate`. That is wrong.** `CellEditingTemplate` is declared on
  the base `TableViewColumn` (`TableView.idl:208-210`), so every column type has it, including `TableViewTextColumn` — which is
  why `TableViewTextColumn::GenerateEditingElementCore` defers to it when set.

### 10.0.2 Comparison with PR `!15971489`

The PR's editing coverage is five tests, all in `TableView_Gap_APITests.cs`:

| PR test | Here |
| --- | --- |
| `ValidationFailureBlocksCommitUntilCanceled` | Split across `VerifyValidationErrorBlocksCommitAndLeavesItemUnchanged`, `VerifyClearingValidationErrorAllowsTheCommit` and `VerifyCancelEditClosesAValidationBlockedEditWithCancelAction`. The PR asserts blocked-then-cancel; the recovery-by-valid-value half is new here, and is the half an app depends on. |
| `ForcedEditTerminationOnItemsSourceResetIgnoresEndingCancelAndDoesNotCrash` | `VerifyItemsSourceResetWhileEditingClosesEditSafely`, strengthened: "does not crash" is a weak assertion, so this one additionally requires `IsEditing` to return to false and a *subsequent* edit on the new source to succeed. |
| `ForcedEditTerminationOnUnloadDiscardsInvalidEditAndDoesNotCrash` | `VerifyUnloadWhileEditingClosesEditSafely`, strengthened the same way (reload, then edit again). |
| `CellEditEndingCancelActionRestoresOriginalValue` | `VerifyCellEditEndingCancelBlocksCommitAndLeavesEditOpen` covers the veto; the *restore* half needs a user-typed value in a live editor and is §11. |
| `RowEditEndingCancelActionRestoresOriginalValue` | N/A — row-scoped editing and `RowEditEnding` are not in this release's API. |

New here, with no PR equivalent: everything gate-related (all five §10.1 tests), both §10.3 tests, three of the four §10.4
event tests, and the reentrancy test. The PR had no coverage of the UIA editing surface at all, which is this release's only
programmatic route into an edit.

### 10.0.3 Diagnostic finding: peer error messages do not reach the caller

`TableViewCellAutomationPeer::SetValue` throws `winrt::hresult_error` with carefully worded text — *"This cell is read-only."*,
*"The value was not accepted."*, *"This cell's editor does not support setting a text value."* — which distinguish four
different refusals. A C# caller sees only the generic HRESULT string: `Not implemented` for `E_NOTIMPL` and `Unspecified error`
for `E_FAIL`. The custom text is lost, so tests here assert only *that* the call threw, never why.

Not filed as a product bug: it is a diagnostics gap rather than a behaviour one, and the cause is likely that the messages are
not originated via `RoOriginateError`. Worth revisiting, because the four refusals are genuinely different conditions and an app
debugging a failed `SetValue` currently cannot tell them apart.



- [x] `VerifyReadOnlyTableViewOffersNoValuePatternAndRejectsEditing`
  - **Description.** Build an editable text column on a `TableView` left at its default `IsReadOnly == true`. Ask the cell peer
    for `PatternInterface.Value`, read `IValueProvider.IsReadOnly`, and call `SetValue`.
  - **Expected result.** No Value pattern is offered, the peer reports `IsReadOnly == true`, `SetValue` throws, the bound item
    keeps its original value, and `TableView.IsEditing` stays false.
  - **Failure means.** The control advertises an editing capability it will not honour. Assistive technology would present the
    cell as editable and then fail mid-edit, and `IsReadOnly` — the IDL's stated opt-in gate, *"while true (the default) no cell
    can be edited, whatever the column says"* — is not gating anything.
  - **Remarks.** The IDL specifies the gate; the peer's advertisement rule is specified in the peer source comment rather than
    the IDL, which is the weaker of the two grounds. The item-unchanged and `IsEditing` assertions rest on the IDL alone.

- [x] `VerifyWritableTableViewAndColumnOfferValuePattern`
  - **Description.** Set `TableView.IsReadOnly = false` with the column left writable, then query the cell peer.
  - **Expected result.** `PatternInterface.Value` is offered and `IValueProvider.IsReadOnly` is false.
  - **Failure means.** Editing cannot be started at all through the accessibility surface even when the app has opted in, so a
    keyboard-only or screen-reader user has no route to a feature mouse users have.
  - **Remarks.** Pairs with the test above; together they pin both directions of the opt-in.

- [x] `VerifyPerColumnReadOnlyBlocksOnlyThatColumn`
  - **Description.** With `TableView.IsReadOnly = false`, mark the first column `IsReadOnly = true` and leave the second
    writable. Query both cell peers in the same row and attempt `SetValue` on both.
  - **Expected result.** The read-only column's peer offers no Value pattern and its `SetValue` throws with the item unchanged;
    the writable column's peer accepts the value and writes it through.
  - **Failure means.** Column-level read-only is either ignored (a column the app marked non-editable can be edited — silent data
    corruption) or over-applied (one read-only column disables editing for the whole row).
  - **Remarks.** This is the test that distinguishes per-column gating from control-level gating; neither single-column test can.

- [x] `VerifyColumnWithCellEditingTemplateOffersNoValuePattern`
  - **Description.** Give an otherwise-editable `TableViewTextColumn` a `CellEditingTemplate`, then query the cell peer.
  - **Expected result.** The Value pattern is not offered and the peer reports `IsReadOnly == true`.
  - **Failure means.** The control would advertise a text-value contract over an editor it cannot read or write as text. `SetValue`
    would open an edit, fail to find a `TextBox`, and abandon it — the exact failure the guard exists to prevent.
  - **Remarks.** The column is still editable by gesture; only the *text-value* pattern is withheld. The test must not be read as
    "a `CellEditingTemplate` disables editing". Grounded in the peer source comment, not the IDL.

- [x] `VerifyIsEditingIsTrueOnlyWhileAnEditIsOpen`
  - **Description.** Observe `TableView.IsEditing` before an edit, from inside a `CellEditEnding` handler during one, and after
    it closes.
  - **Expected result.** False, then true, then false.
  - **Failure means.** `IsEditing` does not describe the state the IDL says it describes. An app using it to gate commands, or to
    decide whether to allow navigation, would act on a stale answer.
  - **Remarks.** The IDL is explicit that `IsEditing` also covers the opening and closing windows — *"an app querying it from a
    `BeginningEdit` handler therefore sees true even if that handler goes on to cancel"* — so a `CellEditEnding` observation is
    inside the specified true window, not an implementation detail.

### 10.2 Editors

All gesture-bound; see §11. The one API-testable item from this subcategory,
`VerifyColumnWithCellEditingTemplateOffersNoValuePattern`, is listed under §10.1.

- `(interaction)` `VerifyTextColumnDoubleClickCreatesTextBox`
- `(interaction)` `VerifyTextColumnF2CreatesTextBox`
- `(interaction)` `VerifyTemplateColumnEditorUsesCellEditingTemplateContent`
- `(interaction)` `VerifyEditorReceivesInitialValue` — the editor's pre-populated value cannot be observed through `SetValue`,
  which overwrites it in the same call.
- `(interaction)` `VerifyEditorGetsFocusOnBeginEdit`

### 10.3 Commit and cancel

- [x] `VerifySetValueCommitsThroughToTheBoundItem`
  - **Description.** On a writable text column, call `IValueProvider.SetValue("<new>")` on a cell peer and read the bound
    property off the source item. Also read `IValueProvider.Value` back.
  - **Expected result.** The item's property holds the new value, `Value` reflects it, the displayed cell text updates, and
    `IsEditing` is false afterwards.
  - **Failure means.** The primary purpose of editing does not work: a completed edit does not reach the data. Either the
    write-back binding is not applied, or the commit closes the edit without transferring.
  - **Remarks.** `TableViewTextColumn` builds its editing binding as TwoWay/Explicit regardless of the display binding's mode, so
    the test's display binding may be OneWay without weakening the assertion.

- [x] `VerifyCommitAndCancelEditReturnFalseWhenNoEditIsOpen`
  - **Description.** Call `CommitEdit()` and `CancelEdit()` on a loaded `TableView` with no edit in progress, in both the
    read-only and writable configurations.
  - **Expected result.** Both return false every time, `IsEditing` stays false, no editing event is raised, and no item value
    changes.
  - **Failure means.** Either the call throws — an app cannot safely call `CommitEdit()` defensively before navigating away — or
    it returns true, telling the app an edit was closed when none existed.
  - **Remarks.** The IDL says these return *"false when it did not close"*. With nothing open, nothing closed, so false is the
    coherent reading — but the IDL does not address the empty case explicitly. If the product intends true here, this is a spec
    gap to close rather than a test to change.

- `(interaction)` `VerifyEnterKeyCommitsEdit`
- `(interaction)` `VerifyEscapeKeyCancelsEdit`
- `(interaction)` `VerifyFocusLossCommitsEdit`

### 10.4 Edit events

- [x] `VerifyBeginningEditReportsItemAndColumn`
  - **Description.** Handle `BeginningEdit`, then drive an edit on a known row and column via `SetValue`.
  - **Expected result.** The handler runs exactly once, `args.Item` is reference-equal to the source item for that row,
    `args.Column` is reference-equal to the column object, and `args.Cancel` defaults to false.
  - **Failure means.** An app cannot tell which cell is about to be edited, so it cannot implement per-cell policy — the only
    reason the event carries these values. Reference inequality on `Item` would additionally mean the control hands out a wrapper
    rather than the data item.
  - **Remarks.** `UnwrapEditingDataItem` exists in the product, so identity is worth pinning rather than assuming.

- [x] `VerifyBeginningEditCancelPreventsTheEdit`
  - **Description.** Set `args.Cancel = true` in a `BeginningEdit` handler, then call `SetValue`.
  - **Expected result.** `SetValue` throws, the item is unchanged, `IsEditing` is false afterwards, and no `CellEditEnding` is
    raised — nothing was opened, so nothing ends.
  - **Failure means.** The veto is advisory only. An app enforcing a business rule ("this row is locked while syncing") would
    find the edit proceeding anyway. A raised `CellEditEnding` would additionally mean the control reports the close of an edit
    that never opened.
  - **Remarks.** That `CellEditEnding` must not fire is inferred from the events' plain meaning, not stated in the IDL.

- [x] `VerifyCellEditEndingReportsCommitAction`
  - **Description.** Handle `CellEditEnding` and drive a successful `SetValue`.
  - **Expected result.** Fires once with `EditAction == Commit`, `Item` and `Column` matching the edited cell, `Cancel` false by
    default, and `IsEditing == true` while the handler runs.
  - **Failure means.** An app cannot distinguish a commit from a cancel at the one point the IDL gives it to do so, making
    save-on-commit logic impossible to write correctly.
  - **Remarks.** `TableViewEditAction` has exactly two values, `Commit = 0` and `Cancel = 1` (`TableView.idl:59-63`), so asserting
    the enum value is a complete assertion.

- [x] `VerifyCellEditEndingCancelBlocksCommitAndLeavesEditOpen`
  - **Description.** Set `args.Cancel = true` in a `CellEditEnding` handler, then call `SetValue`.
  - **Expected result.** `SetValue` throws, the item keeps its pre-edit value, and `IsEditing` remains **true** — the edit stayed
    open. A subsequent `CancelEdit()` from a run without the veto closes it and returns true.
  - **Failure means.** Either the veto does not hold the value back (a validation handler cannot block bad data), or the edit
    closes anyway — which the IDL explicitly contradicts: *"a veto or failed validation leaves the edit open and `IsEditing`
    true."* A closed edit after a veto would strand the user's typed value with nowhere to correct it.
  - **Remarks.** This is the one place the IDL states the stay-open rule outright, which is what licenses the same assumption in
    the validation tests below.

### 10.5 Validation and teardown

- [x] `VerifyValidationErrorBlocksCommitAndLeavesItemUnchanged`
  - **Description.** Bind to an item implementing `INotifyDataErrorInfo` that reports an error for the edited property whenever
    the candidate value is invalid. Drive `SetValue` with an invalid value.
  - **Expected result.** `SetValue` throws, `IsEditing` stays true (the edit is held open), and the item's property holds its
    **pre-edit** value — not the rejected one.
  - **Failure means.** Invalid data reaches the item. The rollback exists precisely so a rejected value is not left on the
    object; without it the app holds data its own validator rejects, while the UI shows something else.
  - **Remarks.** The IDL does not mention `INotifyDataErrorInfo` by name; it says only that failed validation leaves the edit
    open. That validation is `INotifyDataErrorInfo`-shaped, and that errors are scoped to the edited property rather than the
    whole object, are both product decisions with no spec text. **Spec gap** — worth stating in the IDL, since an app cannot
    implement validation against an unwritten contract.

- [x] `VerifyClearingValidationErrorAllowsTheCommit`
  - **Description.** After the blocked commit above, drive `SetValue` again with a value the validator accepts.
  - **Expected result.** It succeeds, the item takes the new value, `IsEditing` returns to false, and `CellEditEnding` reports
    `Commit`.
  - **Failure means.** A validation failure is terminal — the cell is wedged and the user can never satisfy the validator. This
    is the recovery half of the contract, and the more damaging half to get wrong.
  - **Remarks.** Also guards the property-scoped error lookup: an object-level `HasErrors` check would keep blocking here if any
    unrelated property still had an error.

- [x] `VerifyCancelEditClosesAValidationBlockedEditWithCancelAction`
  - **Description.** Hold an edit open via a validation failure, then call `CancelEdit()` directly.
  - **Expected result.** Returns true, `CellEditEnding` fires with `EditAction == Cancel`, `IsEditing` becomes false, and the item
    still holds its pre-edit value.
  - **Failure means.** There is no way out of a blocked edit other than satisfying the validator. An app offering a "discard"
    affordance could not implement it.
  - **Remarks.** The only API route to a `Cancel`-action `CellEditEnding`, since every other cancel path is gesture-driven. A
    normal cancel is itself vetoable per the product; this test does not veto it.

- [x] `VerifyItemsSourceResetWhileEditingClosesEditSafely`
  - **Description.** Hold an edit open via a validation failure, then replace `ItemsSource` wholesale.
  - **Expected result.** `IsEditing` returns to false, the control stays responsive, the new items render, and a later edit on the
    new source works normally.
  - **Failure means.** The control is wedged at `IsEditing == true` with an editor left over a row that no longer exists — a stale
    editor over unrelated data, and every later edit rejected as re-entrant.
  - **Remarks.** The IDL does not describe reset-during-edit. The expectation here is derived from `IsEditing`'s stated meaning:
    if no edit is in flight, it must not read true. **Spec gap.**

- [x] `VerifyUnloadWhileEditingClosesEditSafely`
  - **Description.** Hold an edit open via a validation failure, then unload the `TableView` from the visual tree.
  - **Expected result.** Unload completes without throwing, and reloading yields a control with `IsEditing == false` that can
    begin and commit a fresh edit.
  - **Failure means.** Teardown during an edit leaks or wedges state. In a paged app this is navigating away mid-edit, which is
    ordinary user behaviour, not an edge case.
  - **Remarks.** **Spec gap**, same as above. Note §5.5's lesson: if this fails, run it alone *and* with siblings before calling
    it a product bug.

- [x] `VerifyReentrantEditCallsAreRejectedAndLeaveStateCoherent`
  - **Description.** Call `CommitEdit()` and `CancelEdit()` from inside `BeginningEdit` and from inside `CellEditEnding`, then
    verify the control still works afterwards.
  - **Expected result.** Every re-entrant call returns false without throwing, the outer edit completes on its own terms, and a
    subsequent independent edit begins and commits normally.
  - **Failure means.** An app that calls `CommitEdit()` from a handler — a natural thing to try — corrupts the edit state machine
    or recurses. The IDL anticipates exactly this: `IsEditing` covers the opening and closing windows *"because the control uses
    this to reject re-entrant edit operations from inside consumer callbacks."*
  - **Remarks.** The IDL states that re-entrant operations are rejected but not what the rejected call returns. False is the
    reading consistent with *"false when it did not close"*.

### 10.6 Deferred

- `(deferred)` Row-scoped editing, `RowEditEnding`, async deferrals, public `BeginEdit(item, column)`, `CurrentItem`, multi-cell
  transactions.
- `(deferred)` `GenerateEditingElement`, `PrepareCellForEdit`, `CommitCellEdit` and `CancelCellEdit` are internal this release
  (`TableView.idl:216-219`), so a custom column cannot override the editor path from a test.


</details>

---

## 11. Keyboard and pointer interaction

<details>
<summary>Show section &mdash; entirely relocated to the interaction plan</summary>

**Moved out.** This category now lives in **[`TableView-interaction-test-plan.md`](TableView-interaction-test-plan.md)** —
29 items across keyboard navigation, header input, pointer selection, group header input, editing gestures, pointer resize,
scrolling and RTL.

It was separated because interaction tests are a different cost class: they need a TestUI page, a running app and real input
injection, and they are slower and flakier than an API test asserting the same thing. Keeping them in one place makes the
207-to-29 split visible and stops interaction tests being written for behaviour an API test already covers.

**Before moving anything here, read that document's admission rule.** Four checks have each already rescued items that
looked gesture-bound: an automation peer route, a programmatically reachable blocked state, a driver on the primitive
(`ResizeGripper.BeginDrag`), and asking whether the assertion is really about input at all.

</details>

## 12. Accessibility and automation

<details>
<summary>Show 43 items &mdash; 43 written, 40 passing, 3 failing (product bugs)</summary>

File: `TableView_AutomationPeer_APITests.cs`.

### 12.0 Classification

**All 43 items are API tests.** Nothing here needs input: every peer is constructible or reachable on
the UI thread, and a pattern that is *withheld* is as observable as one that is offered.

Repo precedent is direct and strong — 12 `APITests` files exercise automation peers versus 5
`InteractionTests` files, and `Repeater\APITests\AccessibilityTests.cs` is a *dedicated* automation API
test file using exactly the route below. The 5 interaction files that touch peers (Expander,
RatingControl, ScrollPresenter ×2, TreeView) use a peer as a *means* to drive input, never as the
subject.

#### How each peer is reached

| Peer | Route |
|---|---|
| `TableViewAutomationPeer` | `FrameworkElementAutomationPeer.CreatePeerForElement(tableView)` |
| `TableViewRowAutomationPeer` | `CreatePeerForElement(row)` on a realized row |
| `TableViewCellAutomationPeer` | `rowPeer.GetChildren()` — one per **visible** column, in visible order |
| `TableViewGroupHeaderAutomationPeer` | `CreatePeerForElement(groupHeader)` |
| `TableViewColumnHeaderAutomationPeer` | **`new TableViewColumnHeaderAutomationPeer(tableView, column)`** |
| `SortIndicator` / `ResizeGripper` peers | `CreatePeerForElement(element)`, then assert through the base `AutomationPeer` members |

Two of those need explaining.

**The column header peer is virtual.** It has no element of its own — its `Owner()` is the *TableView*,
and it is manufactured per column (`TableViewAutomationPeer::GetOrCreateColumnHeaderPeer`). The
table-level route, `IGridProvider.GetColumnHeaders()`, hands back `IRawElementProviderSimple`, and
there is no public provider→peer conversion (`PeerFromProvider` is protected). Direct construction is
therefore the only way to assert on header naming, help text, or set position — and the IDL makes the
constructor public (`TableView.idl:670`) precisely because the peer is not otherwise reachable.

**Providers are opaque to a C# test.** Everything returning `IRawElementProviderSimple` —
`GetItem`, `GetColumnHeaders`, `GetSelection`, `ContainingGrid`, `SelectionContainer` — can be
asserted for *null / non-null / count / reference identity* and nothing more. Tests that need to
inspect the target assert on a directly constructed peer instead, and say so. Do not invent raw UIA
pattern ids to prise providers open; no test in this repo does that.

#### Cross-category ownership

Three seams, resolved here so neither side is written twice (AGENTS.md step 2):

- **Value pattern on cells → Category 10, not here.** §10.1 already owns the full advertisement gate
  with four tests (`VerifyReadOnlyTableViewOffersNoValuePatternAndRejectsEditing`,
  `VerifyWritableTableViewAndColumnOfferValuePattern`, `VerifyPerColumnReadOnlyBlocksOnlyThatColumn`,
  `VerifyColumnWithCellEditingTemplateOffersNoValuePattern`). §12 asserts no Value-pattern behaviour.
- **Help text content → Category 14, not here.** §12 owns *peer mechanics* (which pattern, which
  control type, which name). §14 owns the *tooltip→HelpText mapping*, including the duplicate
  suppression at `TableViewCellAutomationPeer::GetHelpTextCore` and the tooltip/sort-state join in the
  header peer. §12 keeps only the sort-state half of header help text, because that is a sorting
  concern with no tooltip in play.
- **Sort invoke → split with §7.5.** §7.5 owns the *positive* (`Invoke()` toggles the sort and updates
  the indicator). §12.4 owns the *withholding* (a column that cannot sort offers no Invoke pattern).
  Rule of thumb: if the assertion is about what happens to the **data**, it is §7; if it is about what
  the **peer advertises**, it is §12.

### 12.1 TableView peer patterns and identity

- [x] `VerifyTableViewPeerAdvertisesGridTableAndItemContainer`
  - **Description:** Query `GetPattern` for `Grid`, `Table`, and `ItemContainer` on the TableView peer.
  - **Expected result:** All three return non-null, and each is the peer itself (structural patterns
    are not forwarded elsewhere).
  - **Failure means:** A screen reader cannot address the control as a grid at all. `RowCount`,
    `ColumnCount`, `GetItem` and column-header enumeration all become unreachable, which is a complete
    accessibility outage for the control rather than a degradation.
  - **Remarks:** `TableView.idl:617-624` declares all three interfaces on the runtimeclass, so this is
    the published contract. Merged from three backlog items (step 2.2) — they differ only in the enum
    value passed.

- [x] `VerifySelectionPatternTracksSelectionMode`
  - **Description:** Query the `Selection` pattern with `SelectionMode.Single`, then with
    `SelectionMode.None`.
  - **Expected result:** Offered under `Single`; withheld (`null`) under `None`.
  - **Failure means:** Under `None`, an AT client is told the grid is selectable and every `Select()`
    it issues is silently refused — the client has no way to discover the refusal. Under `Single`, the
    opposite: selection is invisible to AT even though the control supports it.
  - **Remarks:** **Half of this rests on weaker ground.** `TableView.idl:619` declares
    `ISelectionProvider` unconditionally, so the *positive* half is IDL-backed. The conditional
    withholding under `None` is stated only as a product-code rationale, not in the IDL or the design
    spec. It matches UIA guidance (do not advertise a pattern you cannot honour) and the in-box
    precedent, so the test asserts it — but `(needs spec decision)` on whether withholding is the
    intended contract, because a reader of the IDL alone would conclude the pattern is unconditional.

- [x] `VerifySelectionProviderReportsSingleSelectSemantics`
  - **Description:** With nothing selected and then with one row selected, read `CanSelectMultiple`,
    `IsSelectionRequired`, and `GetSelection()`.
  - **Expected result:** `CanSelectMultiple == false`; `IsSelectionRequired == false`;
    `GetSelection()` is empty with no selection and has exactly one provider with a row selected.
  - **Failure means:** `CanSelectMultiple == true` makes AT offer multi-select affordances the control
    will refuse. `IsSelectionRequired == true` tells AT that selection can never be empty, which is
    false — `DeselectAll()` is public. An empty array where a selection exists loses the selected row
    from the AT's model.
  - **Remarks:** `TableView.idl:531-554` limits `SelectionMode` to `None`/`Single` this release, which
    is what makes `CanSelectMultiple == false` a contract rather than an implementation detail.
    A selected row that is scrolled out of realization reports as *unselected* here — pin the selection
    on a realized row so the test does not accidentally assert the virtualization behaviour instead.

- [x] `VerifyScrollPatternIsForwardedToBodyScrollerNotThePeer`
  - **Description:** Query the `Scroll` pattern on the TableView peer with enough rows to scroll.
  - **Expected result:** Non-null, and **not** the TableView peer itself — the provider belongs to the
    body scroller.
  - **Failure means:** Returning the peer itself would mean the peer claims `IScrollProvider` without
    implementing it, and every scroll call would fail at the cast. Returning null means Narrator cannot
    scroll the grid, so rows below the fold are unreachable by AT.
  - **Remarks:** The negative half is IDL-backed: `TableView.idl:617-624` lists four interfaces and
    `IScrollProvider` is **not** among them, so the peer must not answer as itself. That the pattern is
    nonetheless answered by forwarding is not in the spec; it is reasoned from the UIA requirement that
    a scrollable container expose Scroll. **The PR asserts a plain "Scroll pattern exists"
    (`VerifyScrollPatternOnTableViewPeer`), which would pass even in the broken self-claiming case** —
    the identity check is the part worth having.

- [x] `VerifyTableViewPeerControlTypeIsDataGrid`
  - **Description:** Read `GetAutomationControlType()` on the TableView peer, and on the row and cell
    peers.
  - **Expected result:** TableView → `DataGrid`; row → `DataItem`; cell → `DataItem`.
  - **Failure means:** Control type drives how Narrator announces the element and which reading mode it
    enters. A `Group` or `Pane` where `DataGrid` is expected loses table reading entirely.
  - **Remarks:** `TableView-dev-spec.md:170` states the row peer is a `DataItem` outright. `DataGrid`
    for the table and `DataItem` for the cell are UIA convention, not spec.

- [x] `VerifyPeerClassNamesAreDistinctAndOwnerAligned`
  - **Description:** Collect `GetClassName()` from all five TableView peers.
  - **Expected result:** All five non-empty and pairwise distinct, each ending in its owner's type name
    (`TableView`, `TableViewRow`, `TableViewCell`, `TableViewColumnHeader`, `TableViewGroupHeader`).
  - **Failure means:** Two peers sharing a class name makes them indistinguishable to AT automation
    scripts and to UIA-driven test tooling, which commonly select by class name.
  - **Remarks:** **Deliberately does not pin exact qualification, and that is a finding, not caution.**
    The five peers are inconsistent: `TableView` and `TableViewRow` use `hstring_name_of<T>()` (whatever
    the framework yields, matching every other MUX control), while the cell, column-header,
    group-header, `SortIndicator` and `ResizeGripper` peers return hardcoded short literals — the
    primitives with an explicit comment that an internal namespace must not reach AT. Asserting one
    exact form would freeze half of them into the wrong convention. `(needs spec decision)`: pick one
    convention for the five, then tighten this test to equality. No repo test asserts `GetClassName()`
    today, so there is no precedent to follow.

### 12.2 Grid coordinates

Governed by one unusually explicit spec paragraph, `TableView-dev-spec.md:329`: the UIA grid model
counts and exposes **only visible columns**, so `ColumnCount`, `GetItem`, the column-header set and
every column index are on a visible-column basis — a deliberate divergence from WPF's
`DataGridAutomationPeer`, which indexes the full collection. That paragraph is the source for the two
hidden-column tests and for the visible-column basis everywhere else in this category.

- [x] `VerifyGridRowAndColumnCountsMatchVisibleGrid`
  - **Description:** With 3 columns and 10 items, read `RowCount` and `ColumnCount`.
  - **Expected result:** `RowCount == 10` (all items, not just realized rows); `ColumnCount == 3`.
  - **Failure means:** If `RowCount` reported only realized rows, Narrator would announce "row 3 of 7"
    on a 200-item grid and stop navigating at the realization boundary — the classic virtualized-grid
    accessibility bug.
  - **Remarks:** That `RowCount` spans the whole source rather than the realized window is not stated
    in the spec; it is reasoned from the UIA grid contract, which is defined over the logical grid.

- [x] `VerifyGridGetItemReturnsProviderForRealizedCell`
  - **Description:** Call `GetItem(0, 1)` for a realized row.
  - **Expected result:** A non-null provider.
  - **Failure means:** Narrator cell addressing ("move to column 2") fails on rows that are plainly on
    screen — the most basic grid navigation there is.

- [x] `VerifyGridGetItemRealizesOffscreenRow`
  - **Description:** With 200 items, call `GetItem` for a row far outside the realization window.
  - **Expected result:** A non-null provider; the row is realized on demand.
  - **Failure means:** AT can only reach rows the user has already scrolled to, making the rest of the
    grid invisible to a screen reader.
  - **Remarks:** Spec silent. Reasoned from the UIA grid contract, which has no notion of realization —
    the alternative (returning null) would need `VirtualizedItemPattern` to be honest, and the control
    does not implement it. The realization is deliberately **bounded** to the single requested row, so
    a test asserting "the whole source realized" would be asserting the opposite of the intent.

- [x] `VerifyGridGetItemOutOfRangeReturnsNull`
  - **Description:** Call `GetItem` with negative indices, with `row == RowCount`, and with
    `column == ColumnCount`.
  - **Expected result:** `null` in every case, no exception, control still usable afterwards.
  - **Failure means:** An AT client probing grid bounds — which they do — crashes the app or corrupts
    the realization window.
  - **Remarks:** UIA permits either `null` or a failure HRESULT for invalid coordinates. This pins
    `null`. `(needs spec decision)` on which is intended, though `null` is the safer of the two and
    matches what a C# caller can reasonably handle.

- [x] `VerifyHiddenColumnsAreSkippedInGridCoordinates`
  - **Description:** Collapse the middle of three columns; read `ColumnCount` and enumerate a row
    peer's children.
  - **Expected result:** `ColumnCount == 2`; the row peer yields 2 cell peers; the collapsed column has
    no addressable cell.
  - **Failure means:** A collapsed, zero-width column is surfaced as an addressable cell, so Narrator
    stops on an invisible empty cell while moving across the row — exactly the outcome the spec says
    this design exists to prevent.
  - **Remarks:** `TableView-dev-spec.md:329` states this outright, including that it intentionally
    diverges from WPF. Note the deliberate asymmetry to test against: the **visual** layer still
    generates a hidden cell per collapsed column, so the cell-host child count and the UIA child count
    legitimately differ. Do not "fix" a failure by counting panel children.

- [x] `VerifyHiddenColumnShiftsSubsequentColumnIndices`
  - **Description:** With columns A, B, C, collapse A and read the `GridItem.Column` of the cells for B
    and C.
  - **Expected result:** B reports 0 and C reports 1 — indices to the right shift down by one.
  - **Failure means:** Column indices and `ColumnCount` disagree, so a client that enumerates
    `0..ColumnCount-1` either misses the last column or addresses past the end.
  - **Remarks:** Same spec sentence, different assertion: the count test above proves the *size* of the
    model, this proves the *mapping*. Kept separate because a regression that renumbers without
    recounting (or vice versa) breaks exactly one of them.

- [x] `VerifyTableProviderReportsRowMajorAndNoRowHeaders`
  - **Description:** Read `RowOrColumnMajor` and `GetRowHeaders()`.
  - **Expected result:** `RowMajor`; `GetRowHeaders()` empty.
  - **Failure means:** `ColumnMajor` makes Narrator read down columns instead of across rows, which is
    wrong for every data set this control targets. A non-empty row-header array would contain providers
    for elements that do not exist.
  - **Remarks:** Spec silent on `RowOrColumnMajor`; reasoned from the control being row-oriented
    throughout (`TableView-dev-spec.md:165` describes keyboard handling as row-oriented). The empty
    row-header set follows from TableView having no row-header element at all.

### 12.3 Cell and row peers

- [x] `VerifyRowPeerExposesOneCellPerVisibleColumn`
  - **Description:** Read `GetChildren()` on a realized row peer with 3 columns, one collapsed.
  - **Expected result:** 2 `TableViewCellAutomationPeer` children, in visible-column order.
  - **Failure means:** The row is an opaque leaf to AT — a screen reader can reach the row but not the
    values inside it, which is the whole point of a grid.
  - **Remarks:** `TableView-dev-spec.md:170` states the row peer "exposes its cell peers as children".
    Assert the cast to `TableViewCellAutomationPeer` rather than using `as` and skipping: a null child
    must fail loudly (AGENTS.md `0xC000027B` note).

- [x] `VerifyCellPeerGridItemCoordinates`
  - **Description:** For the cell at visible row 1, visible column 2, read `Row`, `Column`, `RowSpan`,
    `ColumnSpan`.
  - **Expected result:** `1`, `2`, `1`, `1`.
  - **Failure means:** A cell reports a position inconsistent with the one `GetItem` used to find it,
    so Narrator's "row 2, column 3" announcement contradicts where the user actually is.
  - **Remarks:** `TableView.idl:676-683` declares `IGridItemProvider` on the cell peer. Spans of 1 are
    not spec'd; TableView has no merged cells, so anything else would be meaningless.

- [x] `VerifyCellPeerTableItemReportsContainingGridAndColumnHeader`
  - **Description:** Read `ContainingGrid()`, `GetColumnHeaderItems()`, and `GetRowHeaderItems()`.
  - **Expected result:** `ContainingGrid()` non-null; exactly one column-header provider; row-header
    array empty.
  - **Failure means:** Narrator cannot announce which column a cell belongs to, which is how a user
    orients themselves in an unfamiliar table.
  - **Remarks:** `TableView.idl:676-683` declares `ITableItemProvider`. Only non-null and count can be
    asserted — providers are opaque (see 12.0).

- `(covered by §10.1)` `VerifyValuePatternOnTextCellPeer` — the Value-pattern advertisement gate is
  owned in full by Category 10, which has four tests over it including both read-only routes and the
  `CellEditingTemplate` case. Nothing left to assert here.

- [x] `VerifyCellNameCombinesColumnHeaderAndValue`
  - **Description:** With header `"Name"` and cell text `"Ada"`, read the cell peer's `GetName()`.
  - **Expected result:** `"Name, Ada"`.
  - **Failure means:** Narrator announces a bare value with no column context, so moving across a row
    reads "Ada, 36, London" with no indication of what those are.
  - **Remarks:** `TableView.idl:676` specifies the format outright: *"Cell peer supplies per-cell
    grid/table structure and `{column header}, {cell value}` names"*. The separator (comma + space) is
    not localized; that is worth raising, but the test pins what the IDL says.

- [x] `VerifyCellNameFallsBackWhenEitherPartIsMissing`
  - **Description:** Read the cell name for a column with a non-string header (so there is no textual
    prefix), and for a cell whose value is empty.
  - **Expected result:** Value alone in the first case; header alone in the second. Never a dangling
    `", "`.
  - **Failure means:** A leading or trailing comma is announced literally by Narrator ("comma Ada"),
    and a name that is only punctuation is worse than no name at all.
  - **Remarks:** Spec states the composed form but not the degenerate cases; reasoned from the composed
    form being meaningless when a part is empty.

- [x] `VerifyTemplateColumnCellNameUsesContentAutomationName`
  - **Description:** Give a template column a cell template whose root carries an
    `AutomationProperties.Name`; read the cell peer's name.
  - **Expected result:** The name incorporates the template content's automation name, not `ToString()`
    of the item and not an empty string.
  - **Failure means:** Every template column is silent to AT — a common shape (a cell holding a button,
    a rating, a status glyph) reads as nothing at all.
  - **Remarks:** Spec silent; reasoned from the text-column case being named and template columns
    having no reason to be a second-class citizen. New relative to the PR, which has no template-column
    naming test. **FAILING — product finding (kept failing).** `TableViewCellAutomationPeer::GetCellValueText`
    reads `border.Child()` and peers that element, but `TableViewTemplateColumn::GenerateElementCore`
    returns a `ContentPresenter`, so the peer computed is the presenter's (empty name), never the
    template root's. The product comment ("Template content uses the standard UIA name computation")
    shows the intent is to work; the fix is to descend into the presenter's realized content.

- [x] `VerifyRowPeerSelectionItemPatternTracksSelectionMode`
  - **Description:** Query `SelectionItem` on a row peer under `Single`, then under `None`.
  - **Expected result:** Offered under `Single`; withheld under `None`.
  - **Failure means:** AT offers a per-row "select" action that the control will refuse, with no way for
    the client to know in advance.
  - **Remarks:** `TableView.idl:629-633` declares `ISelectionItemProvider` on the row peer
    unconditionally, so — as with 12.1's selection test — the withholding half is reasoned from UIA
    guidance rather than stated. Same `(needs spec decision)`.

- [x] `VerifyRowPeerIsSelectedMatchesControlSelection`
  - **Description:** Select row 2 through the control; read `IsSelected` on every realized row peer.
  - **Expected result:** True for row 2 only; false elsewhere. Tracks a later `DeselectAll()`.
  - **Failure means:** Narrator announces the wrong row as selected, or announces no selection at all.

- [x] `VerifyRowPeerSelectDrivesTheControlAndAddToSelectionDoesNotToggle`
  - **Description:** Call `Select()` on a row peer, then `AddToSelection()` on the row that is already
    selected.
  - **Expected result:** `Select()` makes that row the selection. `AddToSelection()` on the already
    selected row leaves it selected — it does not toggle it off.
  - **Failure means:** If `Select()` toggled, an AT client calling it twice would clear the selection,
    which UIA explicitly forbids: `Select` means "make this the selection", never "clear it".
  - **Remarks:** Spec silent; the single-select `AddToSelection == Select` mapping follows in-box
    precedent (`ListViewItemAutomationPeer` behaves the same rather than failing). New relative to the
    PR, which asserts no row-peer selection *actions* at all.

- [x] `VerifyRowPeerRemoveFromSelectionOnlyClearsWhenItIsTheSelection`
  - **Description:** Select row 1, then call `RemoveFromSelection()` on row 3's peer.
  - **Expected result:** Row 1 stays selected — the call is a no-op for a row that is not the current
    selection.
  - **Failure means:** A stale provider held by an AT client across a selection change can wipe out a
    selection the user has since moved elsewhere. This is a real sequence, not a synthetic one: clients
    routinely cache providers.
  - **Remarks:** Spec silent; reasoned from the operation being scoped to "this row". New relative to
    the PR.

- [x] `VerifyRowPeerSelectionContainerIsTheTableView`
  - **Description:** Read `SelectionContainer` on a row peer.
  - **Expected result:** Non-null.
  - **Failure means:** AT cannot navigate from a row back to the grid that owns it, breaking the
    selection model's parent link.
  - **Remarks:** Only non-null is assertable (opaque provider). Weak on its own — kept because it costs
    two lines inside a test class that is already constructing everything it needs.

- `(needs spec decision)` `VerifyCellRuntimeIdIsStableAcrossRealization` — the PR asserts this
  (`Automation_CellRuntimeIdIsStableAcrossRealization`), but there is no contract here to assert
  against. Cell peers are manufactured fresh on every `GetChildren()` call and their runtime id derives
  from the owning element, which `ItemsRepeater` recycles to a *different item* — so stability across
  scroll-out and scroll-back is not merely unimplemented, it is not obviously desirable. The
  deliberate, documented stability work went into the **column header** peer instead, which is covered
  by `VerifyColumnHeaderPeersAreDistinctAndStable` in 12.4. Decide whether cells need identity before
  writing a test that would freeze today's accident into a contract.

### 12.4 Header and group peers

- [x] `VerifyColumnHeaderPeerNameUsesStringHeader`
  - **Description:** Construct a header peer for a column with `Header = "Name"`; read `GetName()`.
  - **Expected result:** `"Name"`.
  - **Failure means:** Columns are announced as nothing, or as the TableView's own name — which is what
    the base implementation would produce, since the peer's `Owner()` is the TableView. Every column
    would then read identically.

- [x] `VerifyColumnHeaderPeerNameFallsBackToRealizedHeaderForTemplateHeaders`
  - **Description:** Use a non-string `Header` rendered through `HeaderTemplate`, with an
    `AutomationProperties.Name` on the template root. Read the peer's name after the header realizes.
  - **Expected result:** The realized header's automation name — not the TableView's name, and not
    empty.
  - **Failure means:** Template headers are unnamed, so a table whose headers are icons or rich content
    is unnavigable by column.
  - **Remarks:** Spec silent on the fallback order; reasoned from string headers being the documented
    primary source and template headers needing *some* name. **FAILING — product finding (kept
    failing).** The peer's fallback (`TableViewColumnHeaderAutomationPeer::GetNameCore`) does reach the
    realized header cell, but that cell is a bare `winrt::Grid` (`TableView.cpp:1553`) and
    `AutomationProperties::SetName` is applied to it **only when `GetColumnHeaderText` returns a
    non-empty string** (`TableView.cpp:1564-1568`). A `Grid` peer does not descend into its children
    for name computation, so `GetName()` returns empty for every template header. Same class of gap as
    `VerifyTemplateColumnCellNameUsesContentAutomationName`: the fallback must descend into the header
    `ContentPresenter`'s realized content.

- [x] `VerifyColumnHeaderPeersAreDistinctAndStable`
  - **Description:** Enumerate `GetColumnHeaders()` twice on a 3-column table, and compare the runtime
    ids of directly constructed peers for each column.
  - **Expected result:** Three providers each time; runtime ids pairwise distinct; the same column
    yields the same peer identity across the two enumerations.
  - **Failure means:** Identical runtime ids across headers is a **UIA protocol violation** — the
    headers become indistinguishable to AT, and clients that key caches on runtime id will conflate
    them. Rebuilding peers on every enumeration breaks provider identity for any client holding one.
  - **Remarks:** This is the one identity contract the product states *as* an invariant, with the
    reasoning written out in `TableViewColumnHeaderAutomationPeer::GetRuntimeIdCore`: the owner-derived
    id the base class would supply is identical for every column, so a self-contained per-column id is
    built instead. That is an invariant, not inferred behaviour (AGENTS.md step 3.3). The stability half
    depends on the peer cache in `TableViewAutomationPeer`, which is replaced wholesale on each
    enumeration — a column that is removed or hidden legitimately loses its peer, so test stability only
    across enumerations where the column survives unchanged.

- [x] `VerifyColumnHeaderAutomationIdPrefersAuthoredId`
  - **Description:** Set `AutomationProperties.AutomationId` on a realized header cell; read the peer's
    automation id. Then read it for a column with no authored id.
  - **Expected result:** The authored id wins. Without one, a non-empty generated id that differs
    between columns.
  - **Failure means:** An app that labels its headers for UI automation testing finds its ids silently
    ignored; or, without the generated fallback, headers are unaddressable before their templates
    realize.

- [x] `VerifyColumnHeaderPositionInSetAndSizeOfSetUseVisibleColumns`
  - **Description:** With 3 columns and the first collapsed, read `GetPositionInSet()`/`GetSizeOfSet()`
    for the remaining two.
  - **Expected result:** `1 of 2` and `2 of 2` — 1-based, visible-column basis.
  - **Failure means:** Narrator announces "column 2 of 3" while only two columns exist, so the user is
    told to look for a column that is not there.
  - **Remarks:** `TableView-dev-spec.md:329` names `PositionInSet`/`SizeOfSet` explicitly among the
    values that are on a visible-column basis, which makes this directly spec-backed rather than
    inferred from the count tests in 12.2.

- [x] `VerifyInvokePatternIsWithheldWhenTheColumnCannotSort`
  - **Description:** Query `Invoke` on the header peer with `CanUserSortColumns = false`, and again
    with the column's `CanSort = false`.
  - **Expected result:** Withheld in both cases; offered when both gates are open.
  - **Failure means:** AT offers "invoke to sort" on a column the control will not sort — the action
    appears to succeed and nothing happens, which is worse than the affordance being absent.
  - **Remarks:** `TableView.idl:666-670` declares `IInvokeProvider` unconditionally; the two gates come
    from the sorting contract. §7.5 owns the positive path (invoke actually sorts) — see 12.0.

- [x] `VerifyColumnHeaderPeerIsALeafHeaderItem`
  - **Description:** Read `GetAutomationControlType()` and `GetChildren()` on a header peer.
  - **Expected result:** `HeaderItem`; zero children.
  - **Failure means:** Because the peer's `Owner()` is the TableView, a base-class `GetChildren()` would
    return the **entire TableView subtree** under every column header — the whole grid duplicated once
    per column in the AT tree. This is the specific catastrophe the override exists to prevent, which is
    why the empty-children assertion is worth as much as the control-type one.

- [x] `VerifyColumnHeaderHelpTextReportsSortStateOnlyForSortableColumns`
  - **Description:** Read help text for a sortable column in each `SortDirection`, and for a
    non-sortable column.
  - **Expected result:** Ascending/Descending/None sort text for the sortable column, varying with
    direction; **no** sort text for the non-sortable one.
  - **Failure means:** A column that cannot be sorted announces a sort state, implying an affordance
    that does not exist.
  - **Remarks:** Assert that the three sortable strings are non-empty and pairwise distinct rather than
    matching literals — they are localized resources, and pinning English text makes the test a
    localization tripwire. §14 owns the tooltip/sort-state combination and duplicate suppression.
    **FAILING — product finding (kept failing).** All three sort strings come back empty:
    `ResourceAccessor.cpp:8-12` hardcodes `LOC_PREFIX L"Microsoft.UI.Xaml"` and looks up the subtree
    `Microsoft.UI.Xaml/Resources`, but the Tabular binary's `AppxPriInitialPath` is
    `Microsoft.UI.Xaml.Controls.Tabular` (`Microsoft.UI.Xaml.Controls.Tabular.vcxproj:23`). Every
    `TryGetLocalizedString` in the Tabular binary therefore returns empty and `GetHelpTextCore`
    silently falls back to the base. `TableView.vcxitems:91` declares the `.resw` correctly, so this is
    a lookup-path mismatch, not a missing resource — and it means **no** TableView localized string
    resolves, not just these three. Corroborated by `ResizeGripperAutomationPeer` passing only because
    it carries hardcoded English fallback literals.

- [x] `VerifyGroupHeaderPeerExposesExpandCollapseUnconditionally`
  - **Description:** Query `ExpandCollapse` on the peer of an expandable group header and of a
    non-expandable one.
  - **Expected result:** Offered in **both** cases. The non-expandable group reports
    `ExpandCollapseState.LeafNode` rather than dropping the pattern.
  - **Failure means:** A pattern that appears and disappears as data changes forces AT clients to
    re-query, and in-box peers (Expander, NavigationViewItem) do not behave that way — so clients are
    not written to expect it.
  - **Remarks:** **Spelled out in the IDL itself** (`TableView.idl:649-655`): *"ExpandCollapse is
    unconditional (a non-expandable group reports LeafNode rather than dropping the pattern)"*, with the
    in-box precedent named. One of the strongest expectations in this category.

- [x] `VerifyGroupHeaderExpandCollapseStateMatchesIsExpanded`
  - **Description:** Toggle `IsExpanded` on a group and read `ExpandCollapseState`.
  - **Expected result:** `Expanded` / `Collapsed` tracking the property; `LeafNode` when the group is
    not expandable.
  - **Failure means:** Narrator announces "collapsed" for a group whose rows are visible, or the
    inverse.

- [x] `VerifyGroupHeaderExpandAndCollapseAreIdempotent`
  - **Description:** Call `Expand()` twice in a row on the same peer, then `Collapse()` twice.
  - **Expected result:** Expanded after both `Expand()` calls; collapsed after both `Collapse()` calls.
  - **Failure means:** If the calls were resolved into a toggle, two `Expand()` calls in one client turn
    would queue two toggles and leave the group **collapsed** — the exact opposite of what was asked.
    Repeated calls are normal AT client behaviour, so this is a live failure mode, not a contrived one.
  - **Remarks:** The product states the requirement as an invariant ("ExpandCollapsePattern requires
    Expand/Collapse to be idempotent") and explains that the mutation lands on a later turn, so a guard
    reading `IsExpanded()` cannot make it safe. Test both calls **within one UI-thread turn** — settling
    between them destroys the condition being tested and the test becomes vacuous.

- [x] `VerifyGroupHeaderGridItemSpansAllVisibleColumns`
  - **Description:** Read `Row`, `Column`, `RowSpan`, `ColumnSpan` on a group header peer with 3
    columns, then collapse one and re-read.
  - **Expected result:** `Column == 0`, `RowSpan == 1`, `ColumnSpan == 3`, falling to 2 when a column is
    collapsed. `Row` is the header's index in the flattened row list.
  - **Failure means:** A `ColumnSpan` of 1 places the group header in the first column only, so AT
    reports a one-cell row where the user sees a full-width band — and cells to its right appear to
    belong to a row that has none.
  - **Remarks:** `TableView.idl:649-652` describes the band as having "no GridItem coordinates for a
    band that spans every column" in justifying the separate peer type; the merged-cell shape is stated
    in the product as the intended model. Visible-column basis is consistent with 12.2.

- [x] `VerifyGroupHeaderPeerNameCombinesKeyAndCount`
  - **Description:** Read `GetName()` on a group header peer for a group with a key and item count.
  - **Expected result:** Key text and count text, in that order, separated by a single space; key alone
    when there is no count text.
  - **Failure means:** Groups announce as "Group" or as nothing, so a user cannot tell which group they
    have entered.
  - **Remarks:** The name is read from the live `TableViewGroupInfo` projection rather than the visual
    tree, deliberately, so it reflects current state rather than the last render — a test that mutates
    the group and re-reads without a layout pass is still valid. Category 9 owns the *values* of
    `KeyText`/`ItemCountText`; this owns only their composition.

- [x] `VerifyGroupHeaderControlTypeIsGroup`
  - **Description:** Read `GetAutomationControlType()`.
  - **Expected result:** `Group`.
  - **Failure means:** `DataItem` would make Narrator read the band as a data row, so the user hears a
    row that has no cells.

### 12.5 Primitive peers

Both primitives are `MUX_INTERNAL` but are projected into the test app — `TableView_Sizing_APITests.cs`
already drives `ResizeGripper` directly. Assert through the base `AutomationPeer` members
(`GetName`, `GetAutomationId`, `GetClassName`, `GetAutomationControlType`, `IsControlElement`,
`IsContentElement`), which need no cast to the peer type.

- [x] `VerifySortIndicatorPeerIsDecorativeAndExcludedFromBothViews`
  - **Description:** Create a peer for a `SortIndicator` and read `IsControlElement()`,
    `IsContentElement()`, and `GetAutomationControlType()`.
  - **Expected result:** Both view flags `false`; control type `Image`.
  - **Failure means:** Narrator stops on the sort glyph as a separate element and announces it, so the
    user hears the sort state twice — once from the header's help text and once from a stray image.
  - **Remarks:** The PR has the same test (`VerifyAutomationPeerExcludedFromControlAndContentViews`), so
    this is confirmed-by-both rather than new. Merged from two backlog items.

- [x] `VerifyResizeGripperPeerNameIncludesOwningHeaderText`
  - **Description:** Set `OwnerName` on a gripper; read the peer's name.
  - **Expected result:** The name contains the owner text **and** the localized gripper name, composed.
    With `OwnerName` empty, the bare gripper name.
  - **Failure means:** Every gripper in the table announces identically ("Resize gripper"), so a user
    moving across the header row cannot tell which column they are about to resize.
  - **Remarks:** Assert containment, not an exact string — the format comes from a localized resource
    with a hardcoded fallback, and pinning either form makes the test a localization tripwire.

- [x] `VerifyResizeGripperPeerNameIgnoresTagAndAncestors`
  - **Description:** Put a decoy string in the gripper's `Tag` and in an ancestor's name, leaving
    `OwnerName` unset; read the peer's name.
  - **Expected result:** Neither decoy appears. The name is the bare localized gripper name.
  - **Failure means:** The primitive publishes whatever it happens to find in the tree it was dropped
    into — the product calls this out as "a silent wrong name is an accessibility bug", and silent is
    the operative word: nothing else would catch it.
  - **Remarks:** The product states the sourcing rule as an invariant (read from the declared
    `OwnerName` property, never `Tag`, never an ancestor). `Tag` is a general-purpose slot a host may
    already be using, which is what makes the decoy realistic. New relative to the PR.

- [x] `VerifyResizeGripperPeerControlTypeAndEmptyAutomationId`
  - **Description:** Read `GetAutomationControlType()` and `GetAutomationId()` with nothing authored.
  - **Expected result:** `Thumb`; automation id empty.
  - **Failure means:** `Slider` would make AT offer value and range operations the gripper does not
    have — it reports drag distance and owns no value. A constant automation id would be **duplicated
    across every column's gripper**, violating the requirement that ids be unique among siblings.
  - **Remarks:** Both choices are stated as deliberate in the product with their rationale ("Not a
    Slider"; "Empty rather than a constant"), which makes them invariants rather than incidental.

- [x] `VerifyAuthoredAutomationNameWinsOnGripper`
  - **Description:** Set `AutomationProperties.Name` on the gripper *and* an `OwnerName`; read the name.
  - **Expected result:** Exactly the authored name, with no owner text composed into it.
  - **Failure means:** An app that names its grippers gets a corrupted name — its text with the
    framework's appended to it — and has no way to opt out.

### 12.6 Automation events — not API-testable

- `(dropped)` Structure-changed events on sort (`ChildrenReordered`), on virtualization reset and group
  expansion (`ChildrenInvalidated`), and the group header's `ExpandCollapseState` property-changed
  event.
  - **Reason:** The product gates these on `AutomationPeer.ListenerExists(...)`, which is false unless a
    real UIA client is attached. An API test cannot attach one, so the raise path never executes and any
    test over it would be vacuous — it would pass identically if the events were deleted. Reaching them
    needs a UIA client harness, which this repo does not have. Recorded rather than silently omitted so
    it is not re-proposed; revisit if such a harness is ever added.

### 12.7 Comparison with PR `!15971489`

The PR's automation coverage is `TableView_AutomationPeer_APITests.cs` (13 tests) plus 3 automation
tests inside `TableView_Gap_APITests.cs`.

**Covered by both (9).** Grid, Table, Selection, ItemContainer and Scroll patterns on the table peer;
`SelectionItem` on the row peer; `Value`, `GridItem` and `TableItem` on the cell peer; `Invoke` and
name on the header peer; `GetItem` force-realizing an offscreen row; hidden columns skipped in grid
coordinates; group rows exposing ExpandCollapse. In three of these our version is materially stronger:

- **Scroll.** The PR asserts only that a Scroll pattern exists. Ours additionally asserts it is *not*
  the TableView peer — the failure the IDL's interface list actually forbids.
- **Selection / SelectionItem.** The PR asserts the pattern is present. Ours asserts it tracks
  `SelectionMode`, which is where the real contract question lives.
- **Value on the cell peer.** Ours is in Category 10, where four tests cover every gate rather than one
  covering the happy path.

**In the PR, not applicable here (2).** `VerifyGridItemPatternOnRowPeer` and
`VerifyScrollItemPatternOnRowPeer` — `TableView.idl:629-633` declares only `ISelectionItemProvider` on
the row peer, and `TableView-dev-spec.md:170` says outright that the row peer has "no grid/table
provider". Both would be testing for something this repo deliberately does not have. Worth knowing:
in the PR's control, the row peer carried grid coordinates.

**In the PR, deliberately not adopted (1).** `Automation_CellRuntimeIdIsStableAcrossRealization` —
see the `(needs spec decision)` entry in 12.3.

**New here, absent from the PR (14).** Row-peer selection *actions* (`Select` not toggling,
`AddToSelection`, scoped `RemoveFromSelection`) — the PR asserts the pattern is present but never calls
it. Column-header runtime-id distinctness and peer-identity stability, the leaf-children override, and
`PositionInSet`/`SizeOfSet` — none of which the PR covers, though its header peer has the same
virtual-peer problem. The `ResizeGripper` peer entirely: the PR's 12 `ColumnResizeGripper` API tests are
all about resize *values* (clamping, min/max coercion) and none touch its automation surface, so the
name-sourcing rule, the `Thumb` control type and the empty automation id are untested there. Group
header `ColumnSpan`, name composition, and `Expand`/`Collapse` idempotency. Template-column cell naming.
Out-of-range `GetItem`.

**One test to steal from the PR's interaction file.** `SortIndicator_ThemeSwitchMidState` — switching
theme while a visual-state transition is in flight. Nothing in this plan covers mid-transition theme
changes. Proposed for 13.2 as a programmatic test rather than an interaction one.


</details>

---

## 13. Theming, resources, density, and high contrast

<details>
<summary>Show 16 items &mdash; 16 written, 12 passing, 4 failing (product bugs)</summary>

File: `TableView_Theming_APITests.cs`.

### 13.0 Classification

**All 16 items are API tests.** Theme and density are resource lookups and layout consequences — both
observable on the UI thread with no input.

Repo precedent is unusually clean here: **4 `APITests` files touch theme/high-contrast resources and
zero `InteractionTests` files do.** `CommonStyles\APITests\CommonStylesTests.cs` owns
`VerifyAllThemesContainSameResourceKeys`, `DumpThemeResources`,
`VerifyVisualTreeExampleLoadAndVerifyForAllThemes` and the per-theme visual-tree checks. There is no
interaction precedent to weigh against.

#### The existing theme-parity test does not cover TableView

`CommonStylesTests.VerifyAllThemesContainSameResourceKeys` builds a `XamlControlsResources` and
compares its theme dictionaries. **TableView's resources are not in it.** They live in
`TabularControlsResources`, a separate dictionary that ships in
`Microsoft.UI.Xaml.Controls.Tabular.dll` and which the test app must merge explicitly (the test app's
`App.xaml` merges only `XamlControlsResources` — see AGENTS.md). So every TableView and
`TabularSurface*` key is currently outside the repo's parity guard.

That makes 13.2's parity test genuinely uncovered rather than a duplicate, and it is the single
highest-value item in this category: a key present in `Default` but missing from `HighContrast` is a
**crash** at resolve time under that theme, not a visual glitch.

For the record, parity holds today — 36 `TabularSurface*` keys in each of `Default`, `Light` and
`HighContrast`, with no set difference in either direction. The test is a guard against the next
addition, not a bug report.

#### Where the values come from

Three dictionaries, with distinct jobs — worth stating because a test that looks in the wrong one
finds nothing:

| Source | Owns |
|---|---|
| `CommonStyles\TabularSurfaces_themeresources.xaml` | The **brush palette** — every `TabularSurface*` key, in `Default`/`Light`/`HighContrast`. The canonical source. |
| `TableView\TableView_themeresources.xaml` | The **metric tokens** only — row heights, font sizes, cell/header padding, the density variants, gripper width. Brushes are deliberately excluded (mirroring them would collide on duplicate keys during theme-XBF emission). |
| `TableView\TableView.xaml` (root) | Last-resort re-resolving `{ThemeResource}` fallbacks for hosts that do not merge `TabularSurfaces`. |

### 13.1 Style resolution

Owns all default-style coverage, including the `VerifyDefaultStyleResolves` item dropped from 1.5.
TableView's own style is already proven by `VerifyTemplatePartsAfterTemplateApplication`; the value
here is the four types nothing else touches.

- `(covered by §1.5)` `VerifyTableViewDefaultStyleResolves` — implied by
  `VerifyTemplatePartsAfterTemplateApplication`, which cannot pass unless the style applied.

- [x] `VerifyRowAndGroupHeaderDefaultStylesResolve`
  - **Description:** Load a grouped TableView; find a realized `TableViewRow` and
    `TableViewGroupHeader`; assert each has a non-null `Template` and that a style-set property took
    effect (row `Padding`, group header `Padding` from `TableViewGroupHeaderPadding`).
  - **Expected result:** Both templates non-null; both padding values match the style.
  - **Failure means:** The container renders as an unstyled `Control` — no background, no gridlines, no
    group-header chrome. Because rows are generated internally rather than by the app, there is no app
    code that would fail first, so this surfaces as "the table looks blank" with no exception.
  - **Remarks:** Asserts the template *and* a setter rather than just `Style != null`: a style can be
    attached and still not be the intended one. Merged from two backlog items (step 2.2) — same
    assertion, different type.

- [x] `VerifySortIndicatorAndResizeGripperDefaultStylesResolve`
  - **Description:** Construct each primitive directly, put it in the tree, `ApplyTemplate()`, assert
    non-null `Template`.
  - **Expected result:** Both resolve.
  - **Failure means:** The sort glyph and the resize affordance are invisible. The gripper still
    functions — its hit target is its own bounds — so resizing silently becomes an invisible
    interaction rather than a broken one, which is harder to notice and worse to use.
  - **Remarks:** Both types are `MUX_INTERNAL` but projected into the test app;
    `TableView_Sizing_APITests.cs` already constructs `ResizeGripper`. The PR has
    `VerifyDefaultStyleResolves` for `SortIndicator`, so half of this is confirmed by both.

### 13.2 Theme resources

- [x] `VerifyTabularSurfaceKeysExistInEveryThemeDictionary`
  - **Description:** Instantiate `TabularControlsResources`; for each of `Default`, `Light` and
    `HighContrast`, compare the key sets in both directions, as `CommonStylesTests` does for
    `XamlControlsResources`.
  - **Expected result:** Identical key sets. Any difference is reported by name, not as a count.
  - **Failure means:** A key defined in one theme and missing from another **crashes on resolve** under
    that theme. It is invisible in the theme the author was working in, which is precisely why it needs
    an automated guard rather than review.
  - **Remarks:** Models `CommonStylesTests.VerifyAllThemesContainSameResourceKeys`, which does **not**
    cover these keys (see 13.0). Report the differing key names — a bare "counts differ" failure leaves
    the next person diffing three dictionaries by hand. Cover the metric tokens from
    `TableView_themeresources.xaml` in the same pass; they have the same failure mode and there is no
    reason to walk the dictionaries twice.

- [x] `VerifyHighContrastBrushesUseSystemColors`
  - **Description:** For every `SolidColorBrush` in the `HighContrast` dictionary, assert its `Color`
    equals the corresponding `SystemColor*` resource value.
  - **Expected result:** Every high-contrast brush traces to a system colour; none is a hardcoded
    literal.
  - **Failure means:** A hardcoded colour in high contrast ignores the user's chosen scheme. For a user
    who requires high contrast this is not a cosmetic defect — it can make a column of text unreadable
    against its own background.
  - **Remarks:** Reasoned from the high-contrast requirement itself rather than a spec sentence; the
    dictionary is written this way throughout today, so the test pins an existing, deliberate property.
    Note `TabularSurfaceGroupHeaderCountOpacity` is `0.7` in Light/Dark and **`1.0`** in HighContrast —
    a deliberate divergence, not a miss, so do not assert opacity parity across themes.

- [x] `VerifyDarkGridLineBrushMatchesTheDocumentedValue`
  - **Description:** Resolve `TabularSurfaceGridLineBrush` from the `Default` (dark) dictionary and
    compare with the inline fallback at the root of `TableView.xaml`.
  - **Expected result:** `#29FFFFFF`, and the fallback agrees.
  - **Failure means:** The two sources drifting apart gives a host that merges `TabularSurfaces` a
    different gridline colour from one that does not — the same app looking different depending on how
    it was packaged, which is near-impossible to diagnose from a bug report.
  - **Remarks:** One of very few exact values the spec states outright:
    `TableView-dev-spec.md:176` — *"Dark `TabularSurfaceGridLineBrush` is `#29FFFFFF`; the C++ fallback
    uses the same 16% white"* — and the dictionary carries the rationale (bumped from 8.2% for
    readability over banding). Pinning a literal colour is normally weak; here the spec names it and the
    test's real job is the cross-source agreement.

- [x] `VerifyGridLineBrushOverrideChangesRenderedSeparators`
  - **Description:** Override `TabularSurfaceGridLineBrush` at app scope before load; inspect the
    rendered row gridline brush.
  - **Expected result:** The override wins.
  - **Failure means:** The documented customisation story fails.
    `TableView-dev-spec.md:227-231` makes overriding a `TabularSurface*` key **the** supported way to
    restyle the table, in place of a monolithic style. If the control caches or hardcodes the brush
    instead of resolving it, that story is fiction.
  - **Remarks:** The gridline brush is held in the per-instance resource cache, so override *before*
    load; overriding afterwards tests the invalidation path, which is the next test's job.
    **FAILING — product finding (kept failing).** `LookupElementResource` (`TableView.cpp:103-140`)
    calls `LookupInThemeDictionaries(appResources, ...)` — which recurses into `MergedDictionaries` —
    **before** `appResources.TryLookup(key)`. That inverts standard XAML precedence, where a directly
    keyed `Application.Resources` entry beats a merged dictionary. So the exact authoring shape the
    spec documents (drop a `TabularSurface*` key into `Application.Resources`) silently loses to the
    control's own merged theme dictionary. Open question: fix the lookup order, or amend the spec.

- [x] `VerifyResizeGripperSeparatorResourcesApply`
  - **Description:** Override `ResizeGripperSeparatorBrush` and `ResizeGripperSeparatorThickness`;
    inspect the gripper's separator visual.
  - **Expected result:** Both overrides take effect.
  - **Failure means:** The primitive's own theming surface is inert, so a host cannot match the
    separator to its design language.
  - **Remarks:** `TableView-dev-spec.md:123` names both keys as part of what "the primitive owns",
    which makes them a published surface rather than internal detail.
    **FAILING — product finding (kept failing).** `ResizeGripperSeparatorBrush` and
    `ResizeGripperSeparatorThickness` are declared in the control's **default-style page**
    (`ResizeGripper.xaml`), not in a `*_themeresources.xaml` merged into application resources. A
    `{ThemeResource}` inside a default style resolves against that style's own dictionary first, so an
    app-scope override can never win. `ResizeGripper.idl` advertises these as "Styling keys", which is
    then untrue. Open question: move the keys into a `*_themeresources.xaml` — the XAML comment cites
    theme-XBF emission constraints for the current placement, but `TABULAR_BINARY_EMITS_THEME_RESOURCES`
    *is* defined (`Microsoft.UI.Xaml.Common.props:21`), so that rationale looks stale.

- [x] `VerifySortIndicatorForegroundResourceApplies`
  - **Description:** Override the sort indicator foreground resource; inspect the glyph.
  - **Expected result:** The override takes effect.
  - **Failure means:** The sort glyph cannot be themed and may end up invisible against a customised
    header background.
  - **Remarks:** **FAILING — product finding (kept failing).** Same mechanism as
    `VerifyResizeGripperSeparatorResourcesApply`: `SortIndicatorForeground` lives in `SortIndicator.xaml`,
    the default-style page, so an app-scope override loses to the style's own dictionary.

- [x] `VerifyThemeChangeAfterLoadReResolvesRowsAndHeaders`
  - **Description:** Load in Light, sample a row background and the gridline brush, switch
    `RequestedTheme` to Dark, settle, re-sample.
  - **Expected result:** The sampled values change to the dark ones.
  - **Failure means:** The per-instance resource cache is not invalidated on theme change, so a table
    stays light-themed inside a dark app until it is recreated. The cache is real and deliberate —
    density metrics and the gridline brush are both cached — so this is the specific risk the
    invalidation path exists to cover, not a hypothetical.
  - **Remarks:** `TableView-dev-spec.md:176` states values "re-resolve across theme (and high-contrast)
    changes", so this is spec-backed. Needs the double settle from AGENTS.md: a theme change reaches
    realized rows one full layout + idle pass after the property changes.

- [x] `VerifyThemeChangeDuringSortIndicatorTransitionIsCoherent`
  - **Description:** Change a column's sort direction and switch theme in the same turn, before the
    indicator's visual-state transition completes; settle and inspect the indicator.
  - **Expected result:** The indicator ends in the state matching the final sort direction, with
    theme-correct brushes. No stuck intermediate state.
  - **Failure means:** A transition interrupted by a theme change leaves the glyph mid-animation or
    carrying the old theme's brush, so the sort affordance shows the wrong direction or is invisible.
  - **Remarks:** **Adopted from the PR** (`SortIndicator_ThemeSwitchMidState`), which has it as an
    interaction test; nothing else in this plan covers a mid-transition theme change. Written here as an
    API test because both triggers are programmatic. Spec silent — this pins robustness, not a stated
    contract, so treat a failure as a real finding but expect a spec conversation with it.

- `(dropped)` `VerifyLightThemeBrushesResolve` / `VerifyDarkThemeBrushesResolve` /
  `VerifyHighContrastBrushesResolve` — three tests differing only in the dictionary name, and each
  weaker than the parity test that replaces them: "every key resolves in Light" passes happily while
  `HighContrast` is missing half of them. Folded into
  `VerifyTabularSurfaceKeysExistInEveryThemeDictionary` (step 2.2).

- `(dropped)` `VerifyHighContrastVisualStateBrushKeysAreWired` — the PR has this
  (`HighContrastVisualStateBrushKeysAreWired`), but as specified it restates the parity test from the
  visual-state side: a state referencing a key that does not resolve is exactly what the parity test
  catches, one step earlier and for every key rather than the subset a visual state happens to name.
  Revisit only if visual states start referencing keys outside the `TabularSurface*` set.

### 13.3 Density

`Density` is one of the better-specified areas in the control, because the presets are literal values
in `TableView_themeresources.xaml` rather than constants in code:

| `TableViewDensity` | Row min height | Cell padding | Header cell padding |
|---|---:|---|---|
| `Compact` | 30 | `8,2,8,2` | `8,2,8,2` |
| `Standard` (default) | 40 | `8,4,8,4` | `8,4,8,4` |
| `Comfortable` | 48 | `8,8,8,8` | `8,8,8,8` |

Standard uses the unsuffixed keys; `Compact`/`Comfortable` use `…Compact`/`…Comfortable` suffixes. The
same values exist as C++ fallbacks for the resource-miss case.

- `(covered by §1)` `VerifyDensityRoundtrip` — `TableViewTests.cs:129` asserts the `Standard` default,
  `:245` the DP static, and `:320-322` the full settable-DP roundtrip through `Compact` and
  `Comfortable`. Nothing left.

- [x] `VerifyRowMinHeightFollowsDensity`
  - **Description:** For each of the three values, set `Density`, settle, read `MinHeight` on a realized
    row.
  - **Expected result:** 30 / 40 / 48.
  - **Failure means:** Density is inert — the property is settable and changes nothing, which is the
    most likely regression here because the row's own `Style` also carries a `MinHeight` setter of 40.
    If the density value stopped being applied as a local value, `Standard` would keep passing and only
    `Compact` and `Comfortable` would fail. Table-driven over all three values for exactly that reason.
  - **Remarks:** Values from `TableView_themeresources.xaml`, not from the code constants — they agree
    today, and `VerifyDensityResourceOverrideWins` below is what proves the resource is the real source.

- [x] `VerifyHeaderBandHeightFollowsDensity`
  - **Description:** Set each density; read the header cell heights.
  - **Expected result:** Header cells use the same density row minimum height as body rows.
  - **Failure means:** The header band and the body rows disagree on height, so the header looks
    misaligned with the grid it labels — most visible at `Compact`, where the difference is 10px.
  - **Remarks:** `TableView-dev-spec.md:83` states it directly: *"Header cells use the density row
    minimum height, so the header band matches body rows."*

- [x] `VerifyCellAndHeaderPaddingFollowDensity`
  - **Description:** For each density, read the generated text cell's `Padding` and a header cell's
    padding.
  - **Expected result:** `8,2,8,2` / `8,4,8,4` / `8,8,8,8` for both.
  - **Failure means:** Rows grow taller but text stays pinned to the top of the cell, so `Comfortable`
    buys empty space instead of breathing room.
  - **Remarks:** Merged from two backlog items — same values, same source, adjacent assertions.
    Horizontal padding is identical across all three densities by design (density is vertical only), so
    a test asserting horizontal change would be asserting a bug.

- [x] `VerifyDensityChangeAfterLoadUpdatesRealizedRows`
  - **Description:** Load at `Standard`, settle, switch to `Compact`, settle; re-read row min height and
    cell padding on already-realized rows.
  - **Expected result:** Existing rows pick up the new metrics; no need to rebuild the control.
  - **Failure means:** Density applies only to rows realized *after* the change, so a table that has
    already rendered shows a mix of densities as the user scrolls — old rows at the old metrics, new
    ones at the new.
  - **Remarks:** Distinct from the tests above, which set density before load. Needs the double settle.

- [x] `VerifyDensityResourceOverrideWins`
  - **Description:** Override `TableViewRowMinHeightCompact` at app scope with a distinctive value; set
    `Density = Compact`; read the row min height.
  - **Expected result:** The override, not 30.
  - **Failure means:** The metrics are hardcoded and the resource keys are decorative. This is the only
    test that can tell the difference: every other density test passes identically whether the value
    came from the resource or from the C++ fallback constant, because the two agree.
  - **Remarks:** Also guards the cache — override before load, since the resolved value is cached
    per instance. **FAILING — product finding (kept failing).** Same root cause as
    `VerifyGridLineBrushOverrideChangesRenderedSeparators`: `LookupElementResource`
    (`TableView.cpp:103-140`) searches `Application.Resources`' merged theme dictionaries before the
    directly keyed entries, so the app-scope `TableViewRowMinHeightCompact` override loses to
    `TableView_themeresources.xaml`. The failure confirms the resource keys resolve from *somewhere*,
    but not from a place an app can override — which is exactly what this test exists to detect.

- [x] `VerifyFontSizeDoesNotVaryWithDensity`
  - **Description:** Read the cell and header font sizes at all three densities.
  - **Expected result:** Unchanged — 14 throughout.
  - **Failure means:** Someone added a density suffix to the font keys, and `Compact` starts shrinking
    text. Density is specified as row height and padding only; shrinking text is a legibility
    regression that would read as intentional to a reviewer.
  - **Remarks:** The product states the exclusion as deliberate (`TableView.h:62`: font sizes resolve
    from fixed, non-density-suffixed keys and are kept out of the density cache "to avoid implying
    otherwise"), and `TableView-dev-spec.md:215` scopes `Density` to "row min-height + built-in
    cell/header padding". A negative test, and new relative to the PR.

- `(dropped)` `VerifyDensityFallbackWhenResourceMissing` — the C++ fallback constants only run when the
  density keys fail to resolve, which needs the merged dictionary removed or replaced mid-test. That
  leaves global resource state broken for every test that follows in the same host, and the payoff is a
  path that only fires in a misconfigured app. `VerifyDensityResourceOverrideWins` covers the half that
  matters (the resource is genuinely consulted).

### 13.4 Comparison with PR `!15971489`

The PR has almost no theming coverage: one test in `TableView_Gap_APITests.cs`
(`HighContrastVisualStateBrushKeysAreWired`), `VerifyDefaultStyleResolves` in the `SortIndicator` API
tests, and `SortIndicator_ThemeSwitchMidState` in its interaction tests. **There is no theme-parity
test, no density test of any kind, and no resource-override test.**

- **Covered by both (1).** `SortIndicator` default style resolution.
- **Adopted from the PR (1).** `SortIndicator_ThemeSwitchMidState` → 13.2's
  `VerifyThemeChangeDuringSortIndicatorTransitionIsCoherent`, converted from an interaction test to an
  API test since both triggers are programmatic.
- **In the PR, folded in (1).** `HighContrastVisualStateBrushKeysAreWired` — subsumed by the parity
  test; see the `(dropped)` entry in 13.2.
- **New here (18).** Everything else: theme-dictionary parity across `TabularControlsResources`,
  high-contrast system-colour sourcing, the documented dark gridline value and its fallback, all three
  resource-override tests, theme re-resolution after load, row/group-header style resolution, and the
  entire density block — all six tests, covering values the PR never asserts at all.

The gap is worth stating plainly: **density is a shipping public property with no test coverage
anywhere in either codebase**, and its presets are exactly the kind of value that drifts silently.


</details>

---

## 14. Tooltips

<details>
<summary>Show 20 items &mdash; 20 written, 19 passing, 1 failing (product bug)</summary>

File: `TableView_ToolTips_APITests.cs` (new — §13's file is already large, and tooltips share no helpers with it).

### 14.0 Classification

**All 20 are API tests.** The category *looks* interaction-bound — a tooltip is a hover affordance — but
every assertion here is about the tooltip **object and its automation projection**, not about the popup
opening:

- `ToolTipService.GetToolTip(element)` returns the attached `ToolTip`, and its `Content` is readable
  without any pointer. Attachment, content, replacement and clearing are all observable directly.
- The accessibility half is peer state: `AutomationProperties.GetHelpText(element)` on the cell wrapper,
  and `GetHelpText()` on the cell and column-header peers.
- The recycling and live-update halves are driven by `ScrollBodyToVerticalOffset` and a source
  `PropertyChanged` — neither needs input.

Nothing in this list requires hover, and per the §14 note in "Remaining work, classified" it must stay
that way: reworded as "a tooltip appears when the pointer rests on the cell", every item would become an
interaction test and assert strictly less.

**Primary source: `TableView-functional-spec.md:78-88`.** That block is unusually complete for this
control — it states opt-in, author precedence, content rules, the accessibility projection *and* its
duplicate suppression, and the recycling contract. Most items below quote it rather than reason from
silence. The IDL adds the two property-level contracts (`TableView.idl:116-119` for `HeaderToolTip`,
`:212-214` for `CellToolTipBinding`).

**Not repeated here.** Defaults and DP identity for both properties are already asserted in §1
(`TableViewTests.cs:151`, `:167`, `:181`, `:264`, `:367`). §12 keeps the sort-state-only half of header
help text (`VerifyColumnHeaderHelpTextReportsSortStateOnlyForSortableColumns`), which has no tooltip in
play; **§14 owns every case where a tooltip and a help text meet.** §15's
`VerifyRecycledCellsUpdateAllVisualState` must not re-assert the tooltip half — 14.1's recycle test owns
it.

**Group-header tooltips are out of scope**: `TableView-functional-spec.md:86` defers them.

### 14.1 Cell tooltips

- [x] `VerifyNoCellToolTipBindingCreatesNoToolTip`
  - **Description:** Realize a table whose columns set no `CellToolTipBinding`; read
    `ToolTipService.GetToolTip` on every cell wrapper.
  - **Expected result:** `null` on every cell — no `ToolTip` object is created.
  - **Failure means:** Every table pays for a tooltip nobody asked for: a `ToolTip` per cell per row,
    allocated and attached during row realization, on the hot virtualization path.
  - **Remarks:** `TableView-functional-spec.md:79` states the cost explicitly — *"No binding means no
    tooltip and no per-cell cost."* That makes this a performance contract, not just a tidiness one.

- [x] `VerifyStringCellToolTipProducesToolTipWithThatText`
  - **Description:** Bind `CellToolTipBinding` to a string property; read the `ToolTip` on the first
    row's cell wrapper and compare its `Content` against that row item's value.
  - **Expected result:** A `ToolTip` whose `Content` is the bound string for *that* row, and a different
    string on the second row.
  - **Failure means:** The tooltip is either absent or shows the wrong row's value — the second is worse,
    because it is only visible on hover and reads as correct until someone checks a second row.
  - **Remarks:** `TableView-functional-spec.md:79`: the binding *"is evaluated against each row's data
    item and its value is the tooltip content."* Asserting two rows is what makes this test able to fail
    on a per-row evaluation bug; one row cannot distinguish it from a constant.

- [x] `VerifyEmptyOrNullCellToolTipValueProducesNoToolTip`
  - **Description:** Two rows whose bound tooltip property is `""` and `null` respectively, in a table
    whose other rows have real values; read the tooltip on each.
  - **Expected result:** No tooltip on either row. The rows with real values still have one.
  - **Failure means:** An empty popup opens over cells with no tooltip text — the classic "blank tooltip"
    bug, and it is worse than no tooltip because it also blocks the content underneath.
  - **Remarks:** `TableView.idl:212-213` states *"null or empty means no tooltip"*. Merged from two
    backlog items: same contract, same assertion, only the value differs — a table of cases, per Step 2.

- [x] `VerifyNonStringCellToolTipContentIsHosted`
  - **Description:** Bind the tooltip to a property returning a `Border` (via a converter that returns a
    fresh element per evaluation); read `ToolTip.Content`.
  - **Expected result:** The `ToolTip.Content` is that element — not a `ToString()` of it, and not empty.
  - **Failure means:** Rich tooltips are unsupported in practice despite being specified, so anything
    beyond plain text has to be built by the app outside the control.
  - **Remarks:** `TableView-functional-spec.md:81`: *"a string, or any content a `ToolTip` can host. A
    `UIElement` is parented by that cell's `ToolTip`, so a converter returns a fresh element per
    evaluation."* The converter is not incidental — one element cannot be parented by two tooltips, so a
    shared instance would fail for reasons unrelated to the behaviour under test.

- [x] `VerifyToolTipValuedCellToolTipIsRejected`
  - **Description:** Bind the tooltip to a property whose value is itself a `ToolTip`; read the cell's
    tooltip.
  - **Expected result:** No tooltip is attached. The cell is left clean rather than carrying a nested one.
  - **Failure means:** A `ToolTip` renders inside a `ToolTip`, and the inner one's placement fights the
    placement the control owns — a visibly broken popup, from a mistake an app makes easily because
    `ToolTipService.SetToolTip` accepts exactly this.
  - **Remarks:** Reasoned from `TableView-functional-spec.md:81` (a `ToolTip` is not *content a `ToolTip`
    can host*) plus the control's own retail diagnostic on this path, which states the outcome as
    *"the element has no tooltip."* Weaker than a spec sentence — recorded as such. **The spec should say
    outright what a `ToolTip`-valued tooltip does; it currently only says what valid content is.**

- [x] `VerifyAuthoredToolTipInCellTemplateIsIndependentOfTheColumnToolTip`
  - **Description:** A template column whose `CellTemplate` root carries its own
    `ToolTipService.ToolTip`, on a column that *also* sets `CellToolTipBinding`. Read both the template
    root's tooltip and the cell wrapper's.
  - **Expected result:** Both survive, with their own distinct contents — the authored one on the
    template root, the column one on the wrapper.
  - **Failure means:** The control walks into app content and overwrites a tooltip it did not attach,
    which silently destroys authored UI and is not recoverable by the app.
  - **Remarks:** `TableView-functional-spec.md:80`: *"a tooltip set inside a cell's own content template
    opens over that content; the control's tooltip covers the rest of the cell, and the control never
    touches a tooltip it did not attach."* Note the *other* half of that sentence — an app tooltip on the
    **wrapper itself** — is not reachable from public API, since the wrapper is created by the row; the
    ownership check that guards it is verifiable only by inspection.

- [x] `VerifyRecycledRowShowsTheNewItemsToolTip`
  - **Description:** Scroll a long source far enough to recycle the first screen of rows, then read the
    tooltip on a recycled row's cell.
  - **Expected result:** The new item's tooltip text. Never the previous item's.
  - **Failure means:** A recycled row shows a stale tooltip — the cell text is right and the tooltip is
    wrong, which is the hardest form of this bug to notice and the one most likely to ship.
  - **Remarks:** `TableView-functional-spec.md:83` states both the guarantee and the mechanism: the
    binding tracks the row's `DataContext`, so recycling re-resolves it through the same inheritance that
    refreshes cell text. **Needs the double settle** (`ScrollBodyToVerticalOffset` already does it) — a
    row sampled before the repeater reassigns `DataContext` is parented but unbound, and has produced a
    false product bug in this control twice.

- [x] `VerifyItemPropertyChangeUpdatesCellToolTipInPlace`
  - **Description:** With an `INotifyPropertyChanged` item, change the bound tooltip property after load
    and re-read the `ToolTip`.
  - **Expected result:** The new text, on the **same** `ToolTip` instance — updated in place, not
    replaced.
  - **Failure means:** A tooltip open at the moment the source changes keeps showing a stale value, and
    the alternative (replacing the `ToolTip`) drops a live popup's child — the shape behind a known
    reentrant-`CPopup` crash the control comments on.
  - **Remarks:** `TableView-functional-spec.md:83`: *"a source `PropertyChanged` updates a live tooltip in
    place."* The instance-identity half is what makes "in place" testable at all; without it this only
    re-asserts that the binding works. **New — this contract had no item in the original backlog.**

- [x] `VerifyCellToolTipIsRestoredAfterAnEditCloses`
  - **Description:** On an editable cell with a tooltip, begin an edit through the cell peer's
    `SetValue`/`IValueProvider` path, then let it commit; re-read the cell's tooltip.
  - **Expected result:** The tooltip is present again, with the current value's text.
  - **Failure means:** A cell loses its tooltip permanently the first time it is edited — a state the
    user cannot get out of without scrolling the row out of view and back.
  - **Remarks:** **Half of this is `(needs spec decision)`.** The spec says nothing about tooltips during
    editing. The control deliberately retracts the tooltip while an editor is live (*"an editor owns its
    cell; a tooltip over a live text box is noise"*) and re-applies it on close, but that is a rationale
    comment, not a stated contract, so the test asserts **only the restoration** — which is safe under
    any reading, because a display cell must have its tooltip. The during-edit state is logged, not
    asserted, until the spec says which it is.

### 14.2 Cell tooltip accessibility

- [x] `VerifyStringCellToolTipIsPublishedAsHelpText`
  - **Description:** With a string tooltip whose text differs from the cell's own value, read
    `AutomationProperties.GetHelpText` on the cell wrapper and `GetHelpText()` on the cell peer.
  - **Expected result:** Both report the tooltip text.
  - **Failure means:** Tooltip text is sighted-only. A screen-reader user gets the truncated cell value
    and no way to reach the full one — and truncation is the reason the feature exists.
  - **Remarks:** `TableView-functional-spec.md:82`: *"string tooltip text is published as the cell's
    `AutomationProperties.HelpText`."* Asserting the element *and* the peer is deliberate: the element is
    where the control publishes, the peer is what a client reads, and 14.3 shows they are not always the
    same.

- [x] `VerifyNonStringCellToolTipPublishesNoHelpText`
  - **Description:** With the rich (`Border`) tooltip from 14.1, read the cell's help text.
  - **Expected result:** Empty. No `ToString()` artefact such as a type name.
  - **Failure means:** Narrator announces `Microsoft.UI.Xaml.Controls.Border` — worse than silence,
    because it is both meaningless and confidently spoken.
  - **Remarks:** `TableView-functional-spec.md:82` scopes publication to *string* tooltip text; the
    control's own debug diagnostic states the consequence — non-string content *"shows on hover but is
    not reported to assistive technology."* A negative test.

- [x] `VerifyCellPeerSuppressesHelpTextThatRepeatsTheCellValue`
  - **Description:** Bind the tooltip to the **same** property the cell displays, so tooltip text and
    cell value are identical; read `GetHelpText()` on the cell peer, and compare with a sibling column
    whose tooltip text differs.
  - **Expected result:** Empty for the duplicate; the real text for the sibling.
  - **Failure means:** Every value is announced twice — *"Asha, Asha"* — on the single most common
    authoring shape for this feature (tooltip the column you truncate).
  - **Remarks:** `TableView-functional-spec.md:82`: the peer *"suppresses it at UIA query time when it
    merely repeats the cell's own text, so the value is not announced twice."* **At query time** is the
    load-bearing phrase and the reason this is a peer-level assertion — the element keeps the help text;
    only the peer withholds it. The sibling column is what stops a peer that returns empty unconditionally
    from passing.

- [x] `VerifyCellPeerKeepsAppAuthoredHelpText`
  - **Description:** On a column with **no** `CellToolTipBinding`, set `AutomationProperties.HelpText` on
    the template root to text equal to the cell's value; read the peer's help text.
  - **Expected result:** The app's text is returned unchanged.
  - **Failure means:** The suppression rule over-reaches and deletes app-authored accessibility text,
    which the app has no way to reinstate.
  - **Remarks:** `TableView-functional-spec.md:82`: *"Suppression is gated on the ownership record, so
    text the app set is never dropped."* This is the negative half of the test above, and the pair only
    works together — either alone is satisfiable by a trivially wrong implementation.

### 14.3 Header tooltips

- [x] `VerifyStringHeaderToolTipProducesToolTipWithThatText`
  - **Description:** Set a string `HeaderToolTip`; read the tooltip on the realized header cell.
  - **Expected result:** A `ToolTip` on the header **cell** (not on its `ContentPresenter`), with that
    text.
  - **Failure means:** Either no header tooltip, or one that covers only the header text — so it does not
    open over the padding or the sort affordance, which is most of the header's hit area.
  - **Remarks:** `TableView-functional-spec.md:85`: the tooltip *"Covers the whole header cell, including
    its padding and sort affordance."* That is why the assertion names the element it is attached to, not
    just its existence.

- [x] `VerifyNonStringHeaderToolTipContentIsHosted`
  - **Description:** Set `HeaderToolTip` to a `Border`; read `ToolTip.Content`.
  - **Expected result:** That element.
  - **Failure means:** Header tooltips are text-only in practice, though `HeaderToolTip` is typed
    `Object` precisely so they are not.
  - **Remarks:** `TableView.idl:119` types it `Object`, and `TableView-functional-spec.md:85` calls the
    value *"the tooltip content"* with no string restriction.

- [x] `VerifyNullOrEmptyHeaderToolTipProducesNoToolTip`
  - **Description:** Leave `HeaderToolTip` unset on one column and set it to `""` on another; read both
    header cells.
  - **Expected result:** No tooltip on either.
  - **Failure means:** Blank popups over headers, and a per-header allocation for columns that opted out.
  - **Remarks:** `TableView.idl:116-117`: *"null or empty means no tooltip."* Merged from one backlog item
    plus the empty-string case, which the IDL names but the original list did not cover for headers.

- [x] `VerifyHeaderToolTipChangeIsReappliedInPlace`
  - **Description:** Change `HeaderToolTip` after load — absent → text A → text B → `null` — reading the
    header cell's tooltip after each step.
  - **Expected result:** Tracks every step, ending with no tooltip.
  - **Failure means:** Header tooltips are fixed at first realization, so any app that sets them from a
    view model or changes them with state gets whatever the first value happened to be.
  - **Remarks:** `TableView-functional-spec.md:85`: *"the value is read when the header is built and
    re-applied in place when it changes — no binding, no invalidation."* `TableView.idl:118` backs this
    independently with `MUX_PROPERTY_CHANGED_CALLBACK(TRUE)`. **New — the original backlog had no
    header-tooltip change test**, which left the entire `PropertyChanged` path uncovered.

- [x] `VerifyHeaderToolTipPublishesNoHelpTextOnTheElement`
  - **Description:** With a string `HeaderToolTip`, read `AutomationProperties.GetHelpText` on the header
    cell **and** `GetHelpText()` on the column-header peer.
  - **Expected result:** Empty on the element; the tooltip text from the peer.
  - **Failure means:** The control publishes help text onto an element no automation client ever reads —
    dead weight that also looks like working accessibility to anyone inspecting the tree, hiding the fact
    that the peer is the only route.
  - **Remarks:** `TableView-functional-spec.md:85`: *"the header peer is virtual, so
    `AutomationProperties.HelpText` on the element would never reach a client."* This is the one place
    where cell and header behaviour deliberately diverge — 14.2 asserts the element *does* carry it for
    cells — so the pair documents the asymmetry as intentional.

- [x] `VerifyHeaderPeerHelpTextCombinesToolTipAndSortState`
  - **Description:** On a sortable column with a string `HeaderToolTip`, read the peer's help text in
    each `SortDirection`.
  - **Expected result:** The help text contains the tooltip text **and** varies with sort direction.
  - **Failure means:** Setting a header tooltip costs the user the sort state, or the reverse — one of
    two independent pieces of information silently displaces the other.
  - **Remarks:** `TableView-functional-spec.md:85`: string header tooltip content is *"joined with the
    sort state when the column has one."* **Expect this to fail on the known localization defect** —
    `ResourceAccessor` looks up the wrong PRI subtree for the Tabular binary, so both the sort strings and
    the join format resolve empty and the peer returns the tooltip alone. That is finding 1 in
    "Open failures", not a second bug; the test stays as written per Step 5, and will pass once the
    resource path is fixed.

- [x] `VerifyHeaderPeerHelpTextDropsAToolTipThatRepeatsTheHeaderName`
  - **Description:** Set `HeaderToolTip` to exactly the column's `Header` string; read the peer's name and
    help text.
  - **Expected result:** The name is the header text; the help text does not restate it.
  - **Failure means:** *"Name, Name"* on every header — the same double-announcement 14.2 guards against
    for cells, reached by an equally natural authoring mistake.
  - **Remarks:** Mirrors the cell rule in `TableView-functional-spec.md:82`; for headers the suppression
    is stated in the peer's contract rather than the spec prose. **The functional spec should state the
    header case explicitly** — it currently describes the join but not the de-duplication.


</details>

---

## 15. Virtualization and performance safety

<details>
<summary>Show 3 items &mdash; 3 written, 3 passing &mdash; <strong>complete</strong> (scope narrowed from 8 backlog lines; see 15.0)</summary>

File: `TableView_Virtualization_APITests.cs`.

### 15.0 Scope: what is TableView's to prove

Row virtualization itself belongs to `ItemsRepeater` + `StackLayout`, and scrolling belongs to
`ScrollViewer`. Re-asserting either here would test another team's control. What **is** TableView's
is the set of ways it could *defeat* the virtualization it sits on — measuring the whole source for
`Auto` width, force-realizing for automation, materializing an entire group projection, or leaking
containers through its own prepare/clear path — plus the one place it deliberately does **not**
virtualize.

Dropped from the original eight-item backlog, with reasons:

- **Dropped:** `VerifyScrollRealizesTargetRows` and `VerifyScrollRecyclesOffscreenRows` &mdash; this is
  `ItemsRepeater` realizing against its realization rect. TableView contributes nothing to it, and the
  existing §5 and §12 tests already scroll and then assert on the rows they get back.
- **Dropped:** `VerifyRecycledCellsUpdateAllVisualState` &mdash; recycling correctness is already covered
  from four directions: §5 `VerifyRecycledCellsShowNewItemValues`, `VerifyRecycledRowRebandsForItsNewIndex`
  and `VerifyRecycledRowDoesNotInheritPreviousSelectionVisual`; §14 `VerifyRecycledRowShowsTheNewItemsToolTip`;
  §3 `VerifyCustomColumnElementUpdatesOnRecycle`. A fifth test asserting all of it at once would add
  coverage of nothing and a second place to update.
- **Dropped:** `VerifyAutoWidthMeasuresOnlyRealizedRows` &mdash; §4 `VerifyAutoWidthFitsWidestRealizedCell`
  and `VerifyAutoWidthGrowsAsWiderRowsRealize` already pin both halves: the width is the widest
  *realized* cell, and a wider off-screen item does not contribute until it realizes.
- **Dropped:** `VerifyManyRowsRenderAndScrollWithoutPathologicalRealization` &mdash; a perf smoke test with
  no defensible threshold. The realized-count bound below is the same guard stated as an invariant
  instead of a timing, which is the part that can fail deterministically.

### 15.1 Row virtualization is not defeated

- [x] `VerifyLargeSourceRealizesBoundedRowCount`
  - **Description:** Binds several thousand items to a fixed-height table and counts the elements the rows repeater actually realizes.
  - **Expected result:** A viewport-sized set plus cache is realized &mdash; a small multiple of the rows that fit, and a tiny fraction of the source &mdash; not one container per item.
  - **Failure means:** Something in TableView forces full realization (an `Auto` width measuring the whole source, an automation walk, a group projection materializing every row), which turns a large table into an unbounded allocation and a frozen UI.
  - **Remarks:** `TableView-dev-spec.md:137` gives the bound: rows whose realization rect intersects the body viewport "plus two viewports of cache on each side". The test derives the viewport row count from a realized row's height rather than hard-coding it, so a density change does not silently invalidate the bound. Counted through `ItemsRepeater.TryGetElement`, **not** by walking the visual tree: cleared containers stay parented to the repeater's panel and are merely arranged off-screen at `(-10000, -10000)`, so a visual-tree count measures the pool, not realization (see §2.4).
- [x] `VerifyRealizedRowCountStaysBoundedAfterLongScroll`
  - **Description:** Scrolls the body through a long series of offsets across the whole source, then re-counts both the realized set and the total `TableViewRow` instances parented under the control.
  - **Expected result:** Both stay inside the same bound as the initial state. The pool does not grow with the distance scrolled.
  - **Failure means:** TableView's own prepare/clear path retains containers &mdash; a handler holding a strong reference, a cell that never returns to its pool &mdash; so a long scroll leaks until the app dies. Unlike the first item, this one is unreachable by inspection and only shows up under sustained use.
  - **Remarks:** The pool count is the point of the second measurement, so this test deliberately *does* walk the visual tree, unlike the item above. The two numbers together separate "realizes too much" from "never lets go".

### 15.2 Columns are deliberately not virtualized

- [x] `VerifySupportedColumnCountRealizesEveryCell`
  - **Description:** Builds a table at the top of the supported column range (50) and checks the header and every realized row, before and after a horizontal scroll to the far edge.
  - **Expected result:** One header cell and one cell wrapper per column, on every realized row, at both scroll positions.
  - **Failure means:** Either the control quietly virtualizes columns &mdash; breaking the frozen-column, auto-width and per-row-alignment guarantees that all assume a full cell set &mdash; or it falls over at the column count it claims to support.
  - **Remarks:** `TableView-dev-spec.md:141` is explicit on both sides: "each realized row renders a cell for every non-null column (collapsed columns get a hidden cell)", and column virtualization is "intentionally not implemented". The same paragraph sets the supported range at "~5–50 columns" and says TableView is "**not** designed for spreadsheet-scale column counts (100+)", so the fixture uses 50 &mdash; the documented ceiling, not a number past it. A 100+ column test would assert behaviour the spec declines to promise.

</details>

---

## 16. Error handling, edge cases, and reentrancy

<details>
<summary>Show 6 items &mdash; 6 written, 5 passing, 1 failing on one product crash &mdash; <strong>complete</strong> (scope narrowed from 17 backlog lines; see 16.0)</summary>

File: `controls\dev\TableView\APITests\TableView_NegativePath_APITests.cs`.

### 16.0 Scope

The backlog for this category was 17 one-line items. Six survive. The rest were either covered
already or had no expectation behind them, and this note records which was which so neither gets
re-proposed.

A negative-path test earns a place here when **all three** hold:

1. The bad input or the reentrant call arrives through **public API** — a settable property, a
   collection the app owns, or an event handler the app wrote. Nothing here may require a gesture.
2. The expected outcome is **stated somewhere**: the IDL, the dev spec, or an invariant one of them
   asserts (`SelectedItem` is the item at `SelectedIndex`; a row exists per non-null column). "Leaves
   coherent state" is not an expectation — it is the absence of one, and a test written against it
   passes no matter what the control does.
3. No test in another category already drives the same path.

Rule 2 is what removed most of the backlog. Six of the original items ended in *"per the collection's
contract"*, *"leaves coherent state"*, or *"propagates or is contained exactly as the contract
states"* — all of which resolve to "whatever the implementation does today". Under Step 3 those are
not tests; they are a decision deferred into a `Verify` call.

**Dropped as already covered:**

- `VerifyThrowingSourceDuringIterationLeavesControlIntact` — `VerifyThrowingEnumeratorLeavesControlUsable` (§2) already sets an `ItemsSource` whose enumerator throws and then re-drives the control.
- `VerifyUnloadDuringPendingEditLeavesNoWedgedState` — `VerifyUnloadWhileEditingClosesEditSafely` (§10) unloads with `IsEditing == true` through the validation-blocked path.
- `VerifySourceMutationDuringBeginningEditIsSafe` and `VerifySourceMutationDuringCellEditEndingIsSafe` — `VerifyItemsSourceResetWhileEditingClosesEditSafely` (§10) already replaces the source under an open edit, which is the same teardown path reached one frame earlier.
- `VerifyReentrantSortCallDuringSortingEventIsCoalesced` — the only reentrancy guarantee the contract makes is the editing one at `TableView.idl:504` ("the control uses this to reject re-entrant edit operations"), and `VerifyReentrantEditCallsAreRejectedAndLeaveStateCoherent` (§10) pins it. "Coalesced" for sort is an implementation detail with no counterpart in the IDL.

**Dropped as not TableView's contract:**

- `VerifyInvalidBindingPathDoesNotCrash` — an unresolvable `Binding` path yielding no value is the XAML binding engine's documented behaviour. TableView adds nothing to it, and Step 2.3 forbids testing the platform.
- `VerifyNonFiniteResizeDeltaIsIgnored` *as written* — there is no public API that takes a resize delta; the value only exists inside a live drag. The testable half of the idea survives as `VerifyNonFiniteWidthConstraintsDoNotCorruptActualWidth` below, which pushes the non-finite value through the `MinWidth`/`MaxWidth` DPs instead.

**Moved to the interaction plan:**

- `VerifyUnloadDuringPendingResizeLeavesNoWedgedState` — a pending resize means a captured pointer mid-drag. No peer or property opens that state, so it fails Step 1's API test and belongs with the other `ResizeGripper` gesture items.

**Parked, not dropped:**

- `VerifyDisposalDoesNotAssertInTrackerTeardown` — intended to pin the `0xC0000420` teardown assert. There is no root cause and no repro sequence yet, so there is nothing to write that would fail if the bug returned. Keep the item in mind when the crash is next seen; do not author a speculative pin.

### 16.1 Malformed input through public API

- [x] `VerifyNullColumnEntryIsSkipped`
  - **Description:** Adds a null entry into `Columns` between two real columns, loads, and inspects the header host and every realized row.
  - **Expected result:** No throw from the insertion or the layout pass. Headers and cells are produced for the two real columns only; every realized row carries exactly two cell wrappers, in the order the real columns appear.
  - **Failure means:** Either the column pipeline dereferences collection entries without a null check — a crash on an app mistake that the design already anticipates — or a null entry consumes a cell slot, which would put every row out of alignment with the header.
  - **Remarks:** The expectation is from the dev spec's virtualization note (`TableView-dev-spec.md:141`), which says a realized row renders a cell "for every **non-null** column". That wording only makes sense if a null entry is reachable and skipped, so this is a stated behaviour rather than an inferred one. The IDL does not say whether the insertion itself should throw instead; if it does throw, that is a defensible contract and this item becomes a spec question rather than a product bug.

- [x] `VerifyNonFiniteWidthConstraintsDoNotCorruptActualWidth`
  - **Description:** Sets `MinWidth` and `MaxWidth` to `NaN` and to positive and negative infinity on a pixel-width column, one case per data row, and reads `ActualWidth` back after layout.
  - **Expected result:** `ActualWidth` stays finite and non-negative in every case. With `MaxWidth = ∞` the authored pixel width is used unchanged, which is the documented default state.
  - **Failure means:** The clamp arithmetic propagates a non-finite value into the resolved width, which reaches `Measure` and poisons the whole row's layout — the failure mode is a blank or infinitely wide table, not a localized bad number.
  - **Remarks:** `TableView.idl` declares `MUX_DEFAULT_VALUE("std::numeric_limits<double>::infinity()")` for `MaxWidth`, so an infinite constraint is in-contract by construction and cannot be dismissed as abuse. `NaN` is not mentioned anywhere; the expectation for it rests on the IDL's description of `ActualWidth` as "the resolved, MinWidth/MaxWidth-clamped pixels", which a `NaN` result would not be. Related to §4's `VerifyPathologicalMinGreaterThanMaxIsDeterministic`, which covers the finite version of the same clamp.

- [x] `VerifyZeroSizedHostDoesNotCrash`
  - **Description:** Loads a TableView with Star, Auto and pixel columns inside a host constrained to zero width and height, then restores the host to a real size.
  - **Expected result:** No crash and no non-finite width. Every column's `ActualWidth` is finite and at least zero while collapsed, and the Star column returns to its proportional share once the host has size again.
  - **Failure means:** Star distribution divides by the available width without guarding zero, which is a crash rather than a layout artifact — and the recovery half tells us whether a zero-size pass permanently poisons cached widths.
  - **Remarks:** The expectation comes from the `Width` documentation in `TableView.idl` — Star is "a proportional share of the body viewport after fixed columns" — applied at the degenerate viewport. A zero-sized host is not exotic: it is what a collapsed parent or a not-yet-measured tab page produces on the first frame.

### 16.2 Mutation from inside an event handler

- [x] `VerifySourceMutationDuringSortEventIsSafe` *(data-driven: `Sorting`, `Sorted`)*
  - **Description:** Removes an item from the `ObservableCollection` source from inside the handler, once for each event, then settles and reads the projected rows back.
  - **Expected result:** No crash. The projected row labels equal the surviving items in the sorted order requested, and the sorted column still reports the direction that was applied.
  - **Failure means:** The sort pipeline holds an index or an iterator across the handler callout, so an app that reacts to its own sort event corrupts the projection — the classic reentrancy bug, and one that only reproduces when the handler mutates.
  - **Remarks:** Both events are documented at `TableView.idl:245` and `:261` as firing around the state change, which is precisely what makes a handler a reentrancy hazard; the IDL does not say mutation is allowed there, so the expectation rests on the general invariant that the projection matches the source once settled. The two events are one test with two data rows because they differ only in when the callout happens relative to the state change.

- [x] `VerifySourceMutationDuringSelectionChangedIsSafe`
  - **Description:** Selects a row, and from inside `SelectionChanged` removes an item positioned above the selected one. A second phase does the same for a column removal instead of an item removal.
  - **Expected result:** No crash. `SelectedIndex` and `SelectedItem` still agree — the item at `SelectedIndex` in the surviving source is `SelectedItem` — or selection is cleared to `-1`/`null` together. After the column removal, headers and row cells are still the same count.
  - **Failure means:** Selection state is written after the handler returns, using indices captured before it, so an app that prunes its list on selection change silently selects the wrong row.
  - **Remarks:** The agreement between the two properties is stated in `TableView.idl:541` ("the selected data item"), and §6's `VerifySelectionTracksItemAcrossInsertAndRemoveAbove` already pins it for mutations from *outside* a handler; the uncovered half is the mutation that happens while the control is mid-notification. The column phase lives here rather than as its own item because it asserts the same invariant against a different collection.

### 16.3 Consumer failure

- [x] `VerifyConsumerThrowInSelectionChangedSurfacesAtTheCaller`
  - **Description:** Attaches a `SelectionChanged` handler that throws, calls `Select`, and catches at the call site; then detaches the handler and drives selection again.
  - **Expected result:** The exception reaches the `Select` caller rather than being swallowed, and the control is still usable afterwards: a subsequent `Select` updates `SelectedIndex`/`SelectedItem`, and rows still render.
  - **Failure means:** Either the control swallows app exceptions — which hides real app bugs and is worse than a crash — or a throwing handler leaves the selection model half-written, so the next legitimate selection lands on the wrong row.
  - **Remarks:** The propagation half is WinRT's normal event behaviour rather than a TableView promise; what this test is really pinning is the *recovery* half, and it exists because PR `!15971489` carried a regression pin for exactly this crash (`VerifyConsumerEventThrow_K2_RegressionPin`). One representative event is enough — `Sorting` reaches the same raiser machinery, and a second copy would not fail independently.

</details>

---

## Work sequence

1. Establish test wiring and minimal API test shell. **Done.**
2. Add initialization/defaults/XAML-load tests. **In progress** — blocked on the EmptyTemplate crash.
3. Add columns/header/layout/sizing tests.
4. Add data binding/source update tests.
5. Add selection tests for current single-selection API.
6. Add sorting tests.
7. Add TableViewSource shaping tests.
8. Add grouping tests.
9. Add theming/resource/density tests.
10. Add automation tests.
11. Add editing tests. **Done** — 17 API tests; 8 gesture-bound items moved to section 11.
12. Add tooltip tests.
13. Add interaction TestUI only for behavior that cannot be reliably verified in API tests.

## Notes and constraints

- Keep tests aligned to current public IDL. Do not copy PR `!15971489` names or APIs that do not exist here.
- Write the API test first because it is faster, more deterministic, and matches existing MUXC patterns — then add the
  gesture test for the route.
- Write **both** tiers when a behavior has both routes: the API test owns the state machine, the interaction test owns the
  proof that real input reaches it. One interaction test per gesture route, not per assertion. Behavior with no gesture
  route stays API-only; pointer capture, hover, system focus visuals and markup-compiled XAML stay interaction-only.
- Avoid weak tests that skip on missing UI or pass when provider lookup returns null.
- Avoid broad catch blocks in tests unless verifying explicit exception behavior.
- If a behavior is not yet part of current API contract, mark it deferred rather than writing a test that cannot compile.

## Build and run loop

TableView ships in `Microsoft.UI.Xaml.Controls.Tabular.dll`, which reaches the test app through the local `Microsoft.WindowsAppSDK.WinUI` component package. Rebuilding only the vcxproj is not enough by itself; `MUXControlsTestApp` now copies the locally built DLL over the package copy after build (`OverwriteTabularWithLocalBuild`).

```
msbuild controls\dev\dll-tabular\Microsoft.UI.Xaml.Controls.Tabular.vcxproj
msbuild controls\test\MUXControlsTestApp\MUXControlsTestApp.csproj /restore
powershell .\test\CreateTestPayload.ps1 -Platform x64 -Configuration chk
TestPayload\x64chk\runtests.cmd *TableViewTests*
```

When a test crashes the host, snapshot the matching PDB before rebuilding. `Microsoft.UI.Xaml.Controls.Tabular.pdb` is overwritten on every product build, and analyzing a dump against a mismatched PDB produces plausible but wrong symbol names.

---

# Appendix: API tests present in PR `!15971489` (reference only)

**Scope.** 196 API tests across 12 files. The PR's 29 interaction tests, in three `InteractionTests` files, are listed in
**[`TableView-interaction-test-plan.md`](TableView-interaction-test-plan.md)** with an assessment of each — most of them are
not interaction tests by this plan's rules and are already covered here as API tests.

Captured from the PR working copy. Many target APIs, controls, and namespaces that do not exist in this repo (`ColumnResizeGripper`, `GroupedSourceAdapter`, `HierarchicalSourceAdapter`, `ShapedCollectionView`, multiple selection, column reorder/autosize). Listed for coverage ideas, not as a port target.

## `controls\dev\ColumnResizeGripper\APITests\ColumnResizeGripperTests.cs` (12)

`DefaultsAreCorrect`, `TryResizeUpdatesValueAndRaisesEvent`, `TryResizeClampsBelowMinimum`, `TryResizeClampsAboveMaximum`, `TryResizeNoOpWhenAtTarget`, `DirectValueAssignmentClampsToMaximum`, `BeginResizeAndEndResizeRoundTrip`, `BeginResizeIsIdempotent`, `EndResizeWithoutBeginIsNoOp`, `ChangingMinimumReclampsCurrentValue`, `SettingMinimumAboveMaximumCoercesMaximum`, `SettingMaximumBelowMinimumCoercesMinimum`

## `controls\dev\GroupedSourceAdapter\APITests\GroupedSourceAdapterTests.cs` (20)

`DefaultsAreCorrect`, `FlattensTwoGroupsWithHeaders`, `IncludeGroupHeadersFalseEmitsItemsOnly`, `EmptyGroupsStillEmitHeaders`, `RefreshEventFiresOnSourceChange`, `GroupReferenceMatchesAcrossEntries`, `OuterAddTriggersRebuild`, `OuterRemoveTriggersRebuild`, `InnerGroupChangeTriggersRebuild`, `DefaultIsExpanded_True_EntriesIncludeAllDataRows`, `CollapseGroup_RemovesDataRowsFromEntries_HeaderStays`, `ExpandGroup_RestoresDataRows`, `IsGroupExpanded_ReflectsExplicitState`, `ToggleGroup_FlipsState`, `CollapseAll_GroupHeadersStay_DataRowsGone`, `ExpandAll_AfterCollapseAll_AllGroupsExpand`, `GroupExpansionChanged_FiresWithGroupArg_OnSingleToggle`, `GroupExpansionChanged_FiresWithNullArg_OnExpandAll`, `DefaultIsExpanded_False_GroupsStartCollapsed_ExplicitExpandOverrides`, `Refresh_Manual_PreservesExplicitIntents`

## `controls\dev\HierarchicalSourceAdapter\APITests\HierarchicalSourceAdapterTests.cs` (14)

`DefaultsAreCorrect`, `CollapsedRootShowsOnlyTopLevelNodes`, `DefaultIsExpandedExpandsEntireTree`, `ExpandAndCollapseToggleVisibility`, `ToggleFlipsState`, `ExplicitCollapseOverridesDefaultExpanded`, `RefreshedEventFiresOnSourceChange`, `LeavesAndEmptyParentsAreFlatLeaves`, `B3_RootObservableMutationUpdatesEntries`, `B3_ExpandedChildrenObservableMutationUpdatesEntries`, `ChildrenPropertyName_ReadsViaCustomPropertyProvider`, `ChildrenPropertyName_EmptyFallsBackToIterableSelf`, `ChildrenPropertyName_ChangeRebuildsTree`, `SortDescriptions_OrderSiblingsAtEveryLevel`

## `controls\dev\ShapedCollectionView\APITests\ShapedCollectionViewTests.cs` (20)

`DefaultsAreSane`, `SetSourceMaterializesView`, `SortAscendingByName`, `SortDescendingByAge`, `MultiKeySortAppliesPriorityOrder`, `GroupByDepartmentClustersItems`, `RefreshRebuildsFromCurrentSource`, `ObservableSourceTriggersAutoRefresh`, `RefreshedEventFiresOnRebuild`, `SortDescriptionMutationRebuildsView`, `NoneSortDirectionIsIgnored`, `LiveSorting_ResortsOnObservedPropertyChange`, `LiveSorting_OffMeansNoReshapeOnPropertyChange`, `LiveShapingCoalesce`, `LiveGrouping_RegroupsOnObservedPropertyChange`, `TogglingLiveSortingResubscribesExistingItems`, `B1_GroupKeysOfDifferentTypesDoNotCollapse`, `InpcOnlySource_TriggersAutoRefresh`, `IncrementalAddRemoveProducesSurgicalVectorChanged`, `IncrementalAddHonoursSortOrder`

## `controls\dev\ShapedCollectionView\APITests\ShapedCollectionView_Composition_APITests.cs` (11)

`Composition_SCV_SortOnly`, `Composition_SCV_SortDescending_Replace`, `Composition_SCV_LiveSort_AddRemove`, `Composition_SCV_GSA_Grouped`, `Composition_SCV_GSA_LiveSort_GroupRebuild`, `Composition_SCV_GSA_SourceMutation`, `Composition_SCV_HSA_Hierarchical`, `Composition_SCV_HSA_ExpandCollapse`, `Composition_SCV_HSA_SourceMutation`, `Composition_SCV_GSA_HSA_FullStack`, `Composition_SCV_GSA_HSA_RapidMutation`

## `controls\dev\SortIndicator\APITests\SortIndicatorTests.cs` (8)

`VerifyDefaultDirectionIsNone`, `VerifyDirectionPropertyRoundTrips`, `VerifyDirectionDependencyPropertyExists`, `VerifyAutomationPeerType`, `VerifyAutomationPeerExcludedFromControlAndContentViews`, `VerifyClassName`, `VerifyDefaultStyleResolves`, `VerifyChevronVisibleAfterDirectionChange`

## `controls\dev\TableView\APITests\TableViewTests.cs` (72)

Defaults and header visibility: `TableView_DefaultsAreCorrect`, `ColumnHeaderVisibility_DefaultIsColumn_ColumnHeaderVisible`, `ColumnHeaderVisibility_None_BothHeadersCollapsed`, `ColumnHeaderVisibility_Column_OnlyColumnHeaderVisible`, `ColumnHeaderVisibility_ChangeAfterLoad_UpdatesLive`

Source and columns: `TableView_ItemsSourceRoundtrip`, `TableView_ColumnsIsObservable`, `TableViewColumn_HeaderRoundtrip`, `TableViewColumn_WidthDefaultsTo120`, `TableViewColumn_ResizeDefaults`, `TableViewColumn_ActualWidthFollowsWidth`, `TableViewColumn_ActualWidthClampsToMinWidth`, `TableViewColumn_ActualWidthClampsToMaxWidth`, `TableViewColumn_ActualWidthFollowsMinWidthChange`, `TableViewColumn_ActualWidthFollowsMaxWidthChange`, `TableViewColumn_PathologicalMinGreaterThanMaxFavoursMin`, `TableViewColumn_CanResizeRoundtrip`, `TableViewTextColumn_BindingRoundtrip`, `TableViewTextColumn_GenerateElementProducesTextBlock`, `TableViewRow_GetOwningTableViewDefaultsNull`, `TableViewColumn_GetOwningTableViewTracksColumnsMembership`, `TableView_XamlLoadDoesNotInfiniteRecurse`

Selection: `TableView_SelectionDefaultsAreCorrect`, `TableView_SelectionModeRoundtrip`, `TableView_SelectByIndexUpdatesDPs`, `TableView_DeselectByIndexClearsDPs`, `TableView_SelectedIndexDPDispatchesIntoModel`, `TableView_SelectAllInMultipleSelectsEverything`, `TableView_SelectAllInSingleModeIsClampedToOne`, `TableView_SelectingSecondIndexInSingleClearsFirst`, `TableView_SelectRangeInMultiple`, `TableView_NoneClearsSelection`, `TableView_MultipleToSingleClampsToOne`, `TableView_NoneIgnoresExplicitSelect`, `TableView_ItemsSourceSwapClearsSelection`, `TableView_SelectOutOfRangeIsTolerated`, `TableView_InvalidSelectionDPCoercesToModelConsistency`, `TableView_SelectionChangedFiresOnSelect`, `TableView_SelectionChangedDoesNotFireForSameSelection`, `TableViewRow_IsSelectedDefaultsToFalse`

Sorting: `TableViewColumn_SortDefaults`, `TableViewColumn_CanSortRoundtrip`, `TableViewColumn_SortMemberPathRoundtrip`, `TableViewTextColumn_SortMemberPathFallsBackToBindingPath`, `TableView_SortDefaults`, `TableView_SortByColumnSetsDirectionAndIndex`, `TableView_SortByColumnReplacesExistingSort`, `TableView_ToggleSortDirectionCyclesNoneAscDescNone`, `TableView_ToggleSortDirectionReplaceClearsOtherColumns`, `TableView_ToggleSortDirectionRespectsCanSort`, `TableView_ClearSortRemovesEverything`, `TableView_SortingEventFiresWithTriggerColumnAndSnapshot`, `TableView_ClearSortFiresSortingEventWithNullTrigger`

Sticky header: `TableView_StickyHeader_TemplatePartsAreNamed`, `TableView_StickyHeader_HeaderTracksBodyHorizontalOffset`, `TableView_StickyHeader_HeaderDoesNotMoveOnVerticalScroll`

Column reorder and autosize: `P212_CanUserReorderColumnsDefaultsToTrue`, `P212_CanUserReorderColumnsRoundtripsThroughDP`, `P212_MoveColumn_ReordersColumnsVector`, `P212_MoveColumn_SameIndexReturnsTrueIdempotentNoOp`, `P212_MoveColumn_OutOfRangeFromIndexReturnsFalse`, `P212_MoveColumn_OutOfRangeToIndexReturnsFalse`, `P212_MoveColumn_GlobalGateBlocks`, `P212_MoveColumn_PerColumnGateBlocks`, `P212_MoveColumn_PreservesColumnState`, `P212_MoveColumn_RebuildsHeaderHostInNewOrder`, `P212_MoveColumn_RaisesColumnReorderedOnSuccess`, `P212_MoveColumn_DoesNotRaiseColumnReorderedOnNoOp`, `P212_AutoSizeColumn_NoOpWhenNoRowsRealized`, `P212_AutoSizeColumn_FitsRealizedCellContent`, `P212_AutoSizeColumn_RespectsMaxWidthClamp`, `P212_AutoSizeAllColumns_HitsEveryColumn`

## `controls\dev\TableView\APITests\TableView_AutomationPeer_APITests.cs` (13)

`VerifyGridPatternOnTableViewPeer`, `VerifyTablePatternOnTableViewPeer`, `VerifySelectionPatternOnTableViewPeer`, `VerifyScrollPatternOnTableViewPeer`, `VerifyItemContainerPatternOnTableViewPeer`, `VerifyGridItemPatternOnRowPeer`, `VerifySelectionItemPatternOnRowPeer`, `VerifyScrollItemPatternOnRowPeer`, `VerifyValuePatternOnTextCellPeer`, `VerifyInvokePatternOnHeaderPeer`, `VerifyGetNameOnColumnHeaderPeer`, `VerifyGridProviderGetItemForceRealizesOffscreenRow`, `VerifyTableItemPatternOnCellPeer`

## `controls\dev\TableView\APITests\TableView_Gap_APITests.cs` (17)

`LiveIncrementalUpdates_SingleItemObservableChangesUpdateRows`, `LiveIncrementalUpdates_MultiItemObservableChangesRebuildRows`, `SelectionChangedReportsAddedRemovedAndSourceChangesRebaseSelection`, `SortingEventCancelLeavesOrderAndSortStateUnchanged`, `CellEditEndingCancelActionRestoresOriginalValue`, `RowEditEndingCancelActionRestoresOriginalValue`, `ValidationFailureBlocksCommitUntilCanceled`, `ForcedEditTerminationOnItemsSourceResetIgnoresEndingCancelAndDoesNotCrash`, `ForcedEditTerminationOnUnloadDiscardsInvalidEditAndDoesNotCrash`, `Automation_GroupRowsExposeExpandCollapsePattern`, `Automation_HiddenColumnsAreSkippedInGridCoordinates`, `Automation_CellRuntimeIdIsStableAcrossRealization`, `HighContrastVisualStateBrushKeysAreWired`, `Virtualization_LargeSourceKeepsRealizedRowsBoundedAfterScroll`, `FilteringPredicateReflectsRowsAndFilterChangeReprojects`, `MutableKeyReplaceUpdatesSortedProjection`, `ReentrantReplaceDuringFilterRebuildIsCoalesced`

## `controls\dev\TableView\APITests\TableView_ConsumerThrow_APITests.cs` (2)

`SelectionChanged_ConsumerThrows_ControlIntact`, `ColumnReordered_ConsumerThrows_ControlIntact`

## `controls\dev\TableView\APITests\TableView_NegativePath_APITests.cs` (2)

`VerifyConsumerEventThrow_K2_RegressionPin`, `VerifySourceThrowsDuringIteration_ControlStateIntact`

## `controls\dev\TableView\APITests\TableView_FaultInjection_APITests.cs` (5)

`FaultInjection_OutOfMemory_ItemsSourcePath`, `FaultInjection_ROE_CLOSED_DispatcherResume`, `FaultInjection_STOWED_EXCEPTION_Continuation`, `FaultInjection_OutOfMemory_ColumnOperation`, `FaultInjection_ROE_CLOSED_SelectionChanged`

## PR coverage not represented in our plan

These exist only because the PR has controls/APIs this repo lacks. Revisit if those APIs land:

- `ColumnResizeGripper` as a standalone public primitive with `BeginResize`/`TryResize`/`EndResize` and value clamping. Our repo has an internal `ResizeGripper` only.
- `GroupedSourceAdapter` and `HierarchicalSourceAdapter` as separate testable adapters. Our grouping goes through `TableViewSource`.
- `ShapedCollectionView` with `SortDescriptions`, live sorting/grouping toggles, and incremental `VectorChanged`.
- Multiple/Extended selection, `SelectAll`, `SelectRange`.
- Column reorder (`MoveColumn`, `CanUserReorderColumns`, `ColumnReordered`) and autosize (`AutoSizeColumn`, `AutoSizeAllColumns`).
- Row-scoped editing (`RowEditEnding`).
- Fault injection harness (`OutOfMemory`, `RO_E_CLOSED`, stowed exception paths).
