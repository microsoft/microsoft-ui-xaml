# TableView — test authoring protocol

Applies to everything under `controls\dev\TableView\APITests` and `controls\dev\TableView\InteractionTests`.

The live backlog, with a per-test entry for every item, is split across two documents:

- [`docs\design-notes\TabularControls\TableView-test-plan.md`](../../../docs/design-notes/TabularControls/TableView-test-plan.md) — the **API** plan, and the default place for new work.
- [`docs\design-notes\TabularControls\TableView-interaction-test-plan.md`](../../../docs/design-notes/TabularControls/TableView-interaction-test-plan.md) — the **interaction** plan. Its admission rule is Step 1 below, applied strictly: an item earns a place there only when no peer, no reachable blocked state, and no primitive driver gets to the behaviour.

This file is the rule set; those files are the work.

**Run these six steps in order for every subsection, before writing any test code.**

---

## Step 1 — Classify each test

Decide **API test**, **interaction test**, or **both**, and record the answer in the subsection.

- **API test** — the default. Drives the control programmatically on the UI thread and asserts on
  public API, the visual tree, or automation peers. Lives in `APITests`, imported into
  `MUXControlsTestApp` via `APITests\TableView_APITests.projitems`.
- **Interaction test** — owns the gesture *route*: real pointer
  input, real keyboard routing, hover tooltips, pointer capture, full-app navigation, or markup-compiled XAML
  (`x:Bind`, XamlTypeInfo). Lives in `InteractionTests` and needs a TestUI page.

**If a behavior is reachable both ways, write both.** They make different claims: the API test owns the
state machine — every transition, permutation and negative case — while the interaction test owns the
proof that real input reaches it. Hit-testing, capture, key routing and focus are invisible to an API
test, so a control can pass every API test and be unusable by a person.

This is not a licence to duplicate. The interaction test exists **per gesture route, not per
assertion**: one pointer-click-sorts test covers the click, and the sort cycle, the `CanSort` gate and
the event ordering stay API-only, because once the click provably reaches `SortByColumn` those cannot
fail differently under a pointer. A behavior with no gesture route (defaults, DP round-trips, resource
lookup, virtualization bounds) stays API-only; one with no programmatic route (capture, hover states,
system focus visuals, markup compilation) stays interaction-only.

An interaction test must assert the product's own state — a property, a peer, or the visual tree —
after driving real input. Reading a value a test page wrote into a `TextBlock` asserts what the page
chose to display, not what the control did.

### Look for an automation-peer route

Under the dual rule this no longer *removes* an interaction item — it adds an API one. Finding the peer
route buys the cheap, exhaustive half of the coverage; the gesture item stays for the route.

A spec sentence like *"editing starts from the user: double-click, or F2"* describes the **gesture**,
not the only door. Automation peers are public API and frequently drive the same internal path a
gesture does, which makes an apparently interaction-only category API-testable.

The worked example is Category 10. `TableViewCellAutomationPeer` implements `IValueProvider`, and its
`SetValue` runs `BeginEdit → write → CommitEdit` deliberately, *"so a BeginningEdit handler can still
veto and CellEditEnding/validation still run — a programmatic set must not be able to do what a user
cannot."* That single method turned 17 of 26 backlog items from interaction tests into API tests.

Peers also let you assert the **negative** gates without opening anything: a pattern that is withheld
(`GetPattern(...) == null`) is itself observable, and usually the specified behaviour.

Getting one in a test: `FrameworkElementAutomationPeer.CreatePeerForElement(row)` →
`TableViewRowAutomationPeer.GetChildren()` → one `TableViewCellAutomationPeer` per **visible** column,
in visible-column order. Assert the cast rather than using `as` and skipping — a null peer must fail
the test loudly.

### Look for a state the API can hold open

Some tests need the control parked mid-operation, which usually looks gesture-only. Check whether a
*failure* path parks it there. A validation error makes `CommitEdit` return false and, per the IDL,
*"leaves the edit open and `IsEditing` true"* — which is what makes "reset while editing" and "unload
while editing" writable as API tests. A blocked operation is a supported way to reach a mid-operation
state.

## Step 2 — Check for redundancy before implementing

A test that cannot fail on its own is worse than no test: it costs run time and review attention
while implying coverage that does not exist. Before implementing any item, check all four:

1. **Against tests already written for TableView.** Search the existing `APITests` files for the
   behavior, not just the test name. If an existing test already asserts it as a precondition or a
   side assertion, the new test is redundant — either drop it, or narrow it to the part that is
   genuinely uncovered.
2. **Against other items in the same subsection.** Several checklist items often collapse into one
   test with a loop or a table of cases. Prefer one strong test over three that differ only in the
   value passed.
3. **Against what the platform already guarantees.** Do not test XAML itself. Do not assert that
   `ApplyTemplate()` is idempotent, that a `DependencyProperty` stores what was set to it through
   the same code path, or that arrange positions children a panel just measured. Test what
   *TableView* decided, not what the framework does underneath it.
4. **Against the assertion, not the scenario.** Two tests that reach the same state by different
   routes are only both worth keeping if a regression could plausibly break one route and not the
   other. When that is the reason, say so in the item's note — otherwise the next reader will
   delete one of them.

Record every drop in the plan as `(dropped)` with the reason. A dropped test with a written reason
does not get re-proposed; a silently omitted one does.

### Categories overlap too — fix ownership in the plan before writing either side

Some plan categories describe the same behaviour from different angles, so the redundancy check has
to look across categories, not just inside one. Category 2 (data binding) and Category 8
(`TableViewSource` shaping) are the live example: they are different subjects — the binding pipeline
versus the shaping algebra — but collection notifications, source roundtrip, reset, and duplicate item
identity appear in both.

When you find a seam, do not silently pick a side while implementing. Write the ownership split into
the plan first, in both categories, with a rule of thumb the next reader can apply to new items (for
§2 / §8 it is: *if the test would still make sense with no shaping applied, it belongs in §2*). This is
recorded as §2.0 and cross-referenced from §8.

## Step 3 — Derive expectations from the API and design spec, never from the implementation

Source of expectation, in priority order:

1. `TableView.idl` / `TableViewSource.idl` — the public contract, including `MUX_DEFAULT_VALUE`,
   `contentproperty`, and documented template parts.
2. The design spec and design notes under `docs\design-notes\TabularControls`.
3. Invariants the product code states *as* invariants ("this must always hold"), not behavior
   inferred from reading a function body.

Reading `.cpp` is allowed **to discover what is testable** — which parts exist, what is tagged,
which branches exist, whether an API is reachable at all. It is **not** allowed as the source of an
expected value.

If the spec is silent and the behavior is genuinely undefined, do not invent an assertion. Mark the
item `(needs spec decision)` and raise it, rather than freezing today's behavior into a test.

## Step 4 — Write the expectation down before writing the code

Every checklist item carries four fields:

```
- [ ] `TestName`
  - **Description:** what the test does.
  - **Expected result:** the concrete, observable outcome — specific values, not "works correctly".
  - **Failure means:** what a failure tells us is broken, in product terms.
  - **Remarks:** where the expectation came from, and anything that weakens it. Omit only when the
    expectation is stated outright in the IDL.
```

If **Failure means** reads as "the test is wrong", or is just the negation of **Expected result**,
the test is not worth writing. Go back to step 2.

### Remarks — recording the strength of an expectation

**Remarks** is where the provenance of the expectation lives. It exists because not every true
statement about a control is expressible in an IDL: an IDL can declare a type, a default value, and
a content property, but it cannot state layout behavior, event ordering, recycling contracts,
precedence between two properties, or what happens on a mutation. Those expectations are still
legitimate — they just rest on something weaker than a `MUX_DEFAULT_VALUE`, and the test must say so.

Use Remarks to record, in decreasing order of strength:

- **Which source the expectation came from**, when it is not the IDL — e.g. "Stated in the recycle
  contract comment at `TableView.idl:198-201`", or "From the design spec, not expressible in IDL".
- **A deliberate deviation from platform convention**, with the line that justifies it — e.g.
  "`TableView.idl:178` inverts the usual `ContentControl` rule: the selector wins, not the template."
- **Spec silence.** Say plainly that the spec does not cover this and name what the expectation is
  reasoned from instead ("derived from the stated invariant that headers and cells share one
  `TableViewCellsPanel` ordering"). Pair with `(needs spec decision)` when the reasoning is thin.
- **Known debt.** If the expectation pins observed behavior because nothing better exists, say so
  explicitly and mark it as debt. Debt entries are *not* precedent for the next test.
- **Anything that makes the test weaker than it looks** — flakiness, order dependence, a value that
  is plausible but unverified, a dependency on a `chk`-only assert.

Remarks is not a summary of the test. If it repeats **Description**, delete it.

A Remark noting spec silence is a request for a spec change, not a licence to skip Step 5. When such
a test fails, the product is still wrong by default — the Remark only tells the next reader which
conversation to have.

## Step 5 — Never adjust a test to match the implementation

When a test fails, the default conclusion is that **the product is wrong**.

A failing test may only change when the *expectation itself* is shown to be wrong against the spec —
and when that happens, write into the plan what the spec says and why the original expectation
misread it.

> "The code does X, so assert X" is never a valid reason to edit a test.

## Step 6 — Build, verify, hand off

1. Build `controls\test\MUXControlsTestApp\MUXControlsTestApp.csproj`.
2. Regenerate the payload: `test\CreateTestPayload.ps1 -Platform x64 -Configuration chk`.
   Exit code 16 from a missing `TestDependencies\crtforwarders` directory is benign.
3. Confirm the new test names are actually in the shipped
   `TestPayload\x64chk\Test\UnpackagedApps\MUXControlsTestApp\MUXControlsTestApp.dll`.
4. Run the tests (see below) and triage every failure under Step 5 before handing off.
5. Mark the plan items.

### Running TAEF from an agent shell

Invoking `TE.exe` directly from the agent's shell **always** fails with:

```
[HRESULT: 0x8007000E] ... Failed to set the authentication info on the local RPC binding.
```

This is not a missing desktop session — the session is interactive (`WinSta0`, session 2, elevated,
`INTERACTIVE` in the token). `TE.exe` talks to `te.processhost.exe` over `ncalrpc` and cannot
authenticate using the token inherited through the agent's process tree. It reproduces through
`runtests.cmd`, through `Start-Process`, and under every `/runas:` mode, so do not bother retrying
those.

Re-launching through a **scheduled task with `LogonType Interactive`** gets a fresh interactive token
and the RPC handshake succeeds. Use the helper in the session workspace:

```powershell
.\Run-TaefTests.ps1 -NameFilter '*TableView*'
```

`/inproc` is not an option here: the tests are `RunAs=UAP` / `UAP:Host=PackagedCWA`, and TAEF rejects
`/inproc` for UAP tests.

A benign `HRESULT 0x800706BE` warning about shutting down the test host is emitted after a clean run
and does not indicate failure.

**Re-run any failure in isolation before believing it.** Tests in this suite share a visual tree and
at least one (`ValidateLoadUnload`) fails in a full-suite run but passes alone. An order-dependent
failure is test debt, not a product bug — but a failure that reproduces in isolation is a product bug
until proven otherwise (Step 5).

### `0xC000027B` is usually your bug, not the product's

A failure reported as

```
[HRESULT 0x800706BE] ... A crash with exception code 0xC000027B occurred in module
"CoreMessagingXP.dll" in the test host process while invoking a test operation.
```

is `STATUS_STOWED_EXCEPTION` — a **managed exception** crossing the WinRT boundary. TAEF surfaces it
as a host crash with **no managed stack and no exception type**, which makes it look like a product
assert. It usually is not.

To locate it: find the **last `Verify:` line** logged for that test, then look at the statement
immediately after it in the source. That is where it threw.

Seen in practice, in order of frequency:

1. **`Verify.IsNotNull` followed by a dereference.** This is the most common cause, and it has cost
   two separate investigations. **TAEF's `Verify.*` methods log a failure and continue — they do not
   throw.** So this:
   ```csharp
   Verify.IsNotNull(cell, "cell should exist");
   var text = cell.Content.ToString();   // NullReferenceException
   ```
   raises an NRE on the next line, which becomes `0xC000027B` and **hides the assertion that actually
   failed** — you see a host crash instead of "cell should exist". Write instead:
   ```csharp
   if (cell == null) { Verify.Fail("cell should exist"); continue; }   // or return
   ```
   Apply the same shape to `Verify.IsTrue` guards whose body depends on the condition holding.
2. **A cast that can never succeed.** A plain `InvalidCastException` from casting a header cell `Grid`
   to `Control` to read `IsTabStop`. In WinUI `IsTabStop` is declared on **`UIElement`**, not
   `Control`, and a header cell is a `Grid` (a `Panel`), so the cast could never succeed. Two tests
   "crashed the test host" for that one line.

Suspect the test first; only conclude product bug after the null-guard and cast candidates are ruled
out.

### `0x8001010E` means you built a DependencyObject on the wrong thread

`RPC_E_WRONG_THREAD` comes from constructing a `DependencyObject` — a `Brush`, `Style`, or
`DataTemplate` — at the **top of the test method**, which runs on the TAEF thread, not the UI thread.
The object is created with the wrong apartment affinity and the first UI-thread use fails. Declare
the field `null` outside and construct it inside the first `RunOnUIThread.Execute`.

### Use the `System.ComponentModel` interfaces, not `Microsoft.UI.Xaml.Data` ones

`INotifyPropertyChanged`, `PropertyChangedEventHandler`, `PropertyChangedEventArgs`,
`INotifyDataErrorInfo` and `DataErrorsChangedEventArgs` do **not** exist under
`Microsoft.UI.Xaml.Data` in the C# projection, even though the product's C++ side consults
`winrt::Microsoft::UI::Xaml::Data::INotifyDataErrorInfo`. CsWinRT maps the `System.ComponentModel`
types onto those WinRT interfaces. Write a test data item against `System.ComponentModel`:

```csharp
public event EventHandler<DataErrorsChangedEventArgs> ErrorsChanged;  // not TypedEventHandler
public IEnumerable GetErrors(string propertyName)                      // not IEnumerable<object>
```

Also note `Windows.Foundation` resolves to `Microsoft.Windows.Foundation` here, so a fully qualified
`Windows.Foundation.TypedEventHandler<,>` does not compile.

### `Blocked` is a crashed host, not a failed assertion

TAEF reports `Blocked` for the test that was in flight when the host process died, together with
"stopped communicating with the test host process". That test is usually **not** the culprit and its
own assertions never ran. Look upward in the log for the crash line (`A crash with exception code
... occurred in module ...`) and note which phase it names — a crash in `Setup` happened between
tests, not inside the blocked one.

`0xC0000420` in `Microsoft.UI.Xaml.dll` is a known intermittent teardown assert in this repo. Re-run
the suite before reading anything into it.


---

## Environment notes that have cost time before

- **Stale Tabular DLL.** `MUXControlsTestApp` loads
  `Microsoft.UI.Xaml.Controls.Tabular.dll` from its own output folder, which is populated from an
  extracted nupkg that only `pack.component.cmd` refreshes. Rebuilding the vcxproj alone never
  reaches the test. The `OverwriteTabularWithLocalBuild` post-build target in the csproj fixes this —
  do not remove it. A whole crash investigation was spent on this once.
- **Do not add Tabular `Content Include` entries** to the csproj; it causes
  `APPX1101: Payload contains two or more files with the same destination path`.
- **Strip `GIT_CONFIG*` from the environment before msbuild**; the shell inherits bad values.
- **Build invocation** (`.\init.cmd`, not `init.cmd`; VS 17 msbuild and bare `dotnet msbuild` fail):
  ```
  & $env:ComSpec /c 'cd /d <repo> && call .\init.cmd x64chk /envonly /notitle && msbuild controls\test\MUXControlsTestApp\MUXControlsTestApp.csproj /v:minimal /nologo'
  ```
- **CS0436 warnings on every Tabular type are expected** — `Microsoft.WinUI` already projects
  TableView and collides with the locally generated CsWinRT projection. The local one wins.
- **`MUX_ASSERT_MSG` fires in chk builds.** A test that deliberately drives a rejected path can take
  down the test host. Check for an assert on the path before writing such a test.
- **`IdleSynchronizer.Wait()` ticks the UI thread**, which can mask teardown races. For
  teardown-timing tests use an untickied wait (`Task.Delay(16 * 3).Wait()`), as
  `Repeater\APITests\ViewportTests` does.
- **TableView needs `TabularControlsResources`** merged into `Application.Current.Resources`; the
  test app's `App.xaml` only merges `XamlControlsResources`.
- **Scroll/recycle tests need a double settle.** `ChangeView` returns before the repeater finishes
  re-assigning `DataContext` to recycled containers. A row sampled in that window is parented but
  unbound, so a transient state gets reported as a product bug. Always:
  ```csharp
  scrollView.ChangeView(...);  IdleSynchronizer.Wait();
  RunOnUIThread.Execute(() => tableView.UpdateLayout());  IdleSynchronizer.Wait();
  ```
  `TableViewRowTestHelpers.ScrollBodyToVerticalOffset` already does this — prefer it.
- **Visual state after an ancestor property change needs the same double settle.** An `IsEnabled`
  change on the TableView reaches the rows and updates their `CommonStates` one full layout + idle
  pass *after* the change. One settle is not enough.

### An order-dependent failure is a test bug, not a product bug

Before filing any product bug, run the test **alone** and **with its sibling classes**. If the result
differs, the test is under-settled — neighbouring tests pump extra messages and silently supply the
settle the test failed to do itself. This has now produced two false product bugs in this control
(§5.3's recycle test and §5.5's disabled-state test), one of which was written up in detail before
being retracted.

Beware the specific trap that made the second one convincing: **a correct diagnostic can still support
a wrong conclusion.** That test logged `IsEnabled` on the rows to rule out a propagation failure, which
it genuinely did — and that made the one remaining explanation ("the state update never happens") feel
proven. It was not. In a single snapshot, *never happens* and *has not happened yet* look identical;
only a second observation later in time separates them.

## Useful structural facts

- Header cells (`PART_HeaderHost` children) and row cell wrappers (`PART_CellsHost` children) are
  both `Tag`-ged with the `TableViewColumn` that produced them. That is the supported way to map a
  rendered element back to its column — the control's own frozen-column and recycling code uses it.
- `TableViewCellsPanel` is used as **both** the header host and each row's cells host, and arranges
  children by accumulating `column.ActualWidth()` in `Columns` order. Header/cell alignment is
  therefore structural, not emergent: assert equal column lists, never pixel offsets.
- Documented template parts (`TableView.idl:435`): `PART_HeaderRow`, `PART_HeaderHost`,
  `PART_BodyScroller`, `PART_BodyContent`, `PART_RowsRepeater`, `PART_EmptyStatePresenter`.
