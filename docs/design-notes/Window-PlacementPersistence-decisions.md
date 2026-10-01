# Window placement persistence decisions and history

> **Proposal history, not a current implementation checklist.** The
> [API proposal](../../specs/window-placement-persistence-spec.md) owns the public surface, and the
> [feature specification](./Window-PlacementPersistence.md) owns detailed behavior and requirements.
> Historical implementation reports below retain their original dates, limitations, and test totals.
> They are not new verification, release approval, or evidence that every scenario now passes.
>
> **AI-assisted document:** AI makes mistakes; technical claims require human review.

## Navigation and record conventions

- [Adopted request-based design](#adopted-window-opt-in-and-request-based-placement-design)
- [API-shape rationale](#api-shape-rationale)
- [Launch-monitor investigation](#launch-monitor-hint-investigation)
- [Deferred launcher show-command proposal](#launcher-show-command-scenarios-deferred)
- [Wanted unpackaged storage dependency](#wanted-dependency-unpackaged-default-applicationdata)
- [Implementation history](#implementation-history)
- [Public display host decision](#public-display-api-host-boundary-decision-2026-09-24)
- [Required-options decision](#required-options-decision-2026-09-24)

Reasons and alternatives belong here; operative rules belong in the linked specification.
Quoted or relocated historical instructions such as "implemented", "still unimplemented",
"must", and "run the tests" describe that record's checkpoint. They are not new work requests
or competing current requirements. Later updates can supersede an earlier report.

The original notes mix dated headings and undated additions. Undated updates retain their source
context rather than receiving invented dates. A capture-history update is dated 2026-09-26 in the
source, later than this refactor request; it is preserved as supplied, not independently verified.
The [coverage index](./Window-PlacementPersistence.md#coverage-and-open-gates) distinguishes named
evidence, partial coverage, unknown acceptance, and open design gates.

## Adopted window opt-in and request-based placement design

_Design decision (2026-09-23, revised): Window-level persistence opt-in with request-based
placement policy is now the primary proposal, not a
competing alternative. Adoption of this shape is not a claim of implementation or backend
validation. The [public API declaration](../../specs/window-placement-persistence-spec.md#public-api-surface)
defines the current surface._

_Design decision (2026-09-24): Hidden placement application and automatic startup placement are
now separate. `TryApplyInitialPlacement` is repeatable and no longer ends the one-time
placement pass; the initial-placement phase ends only at first display.
`WindowShowOptions.SkipInitialPlacement` (default `false`) lets a first `Show` display the window
at its current placement. Automatic-save enrollment moves entirely to first display, so hidden
application no longer establishes save eligibility. See
[Why an explicit skip option rather than a Showing event?](#why-an-explicit-skip-option-rather-than-a-showing-event)._

The earlier proposal coupled automatic persistence to assigning an id and extended `Activate`.
It retained initial options on the window and staged placement for a later hidden-show operation.
The adopted design puts the positive `UseAutomaticPlacementPersistence` property on
`Window`, defaults it to `true`, and uses a non-empty id as the setup-time opt-in. Explicit `false`
permits grouping without automatic storage. Placement policy is passed to `Show` or `TryApplyInitialPlacement`. An intermediate
draft put the Boolean on `WindowShowOptions` and required existing apps to migrate from
`Activate`; that decision is superseded. Initial `Show` and opted-in `Activate` now share
automatic placement. Activate keeps default policy and its activation semantics; it reads no
retained options. Hidden application applies now, is repeatable, and does not end the
initial-placement phase; display remains a separate call, and
`WindowShowOptions.SkipInitialPlacement` lets that first display preserve the prepared placement.
`DoNotActivate` replaces the activation enum.
No public copying method or copy constructor is provided. Capture and loading return detached
values, and initial placement application retains its coherent snapshot guarantee.

There is no general-purpose placement setter for already-displayed windows and no public
monitor-adjustment, cascade, serialization, or save operation. Expert customization can use
`LoadForPersistPlacementId` without taking ownership of storage.

### Existing-app adoption

Id-only adoption lets existing apps keep initial `Activate`. This avoids requiring a display-call
migration merely to remember placement, while `Show` provides explicit reason, skip, and
per-call activation policy. The current sequence and example live in the
[API primary workflows](../../specs/window-placement-persistence-spec.md#primary-workflows)
and [feature phase rules](./Window-PlacementPersistence.md#use-the-initial-placement-phase).

### Why apply without showing?

The strongest scenario is restoring overlapped placement before selecting another presenter.
The intended sequence is to apply while hidden, switch to full-screen, then display without
flashing the windowed state, while retaining the windowed placement for the return transition.
S37 requires retaining a maximized return state as well as the selected monitor and normal
restore bounds. The reviewed `FullScreenPresenter` implementation calls `ShowWindow(SW_RESTORE)`
when attaching to a minimized or maximized window, which can show and activate it before
full-screen sizing. Hidden placement application alone therefore does not establish a
hidden, non-activating presenter transition; this integration remains open.
The method name includes `Initial` because this is not a lifetime repositioning API. It applies
only while the window is undisplayed, during the initial-placement phase.
Keep `TryApplyInitialPlacement` for now and review that naming concern explicitly.

Non-activating restart ordinarily needs only `Show(options)` with `ApplicationRestart`,
not a separate Boolean or hidden step. Waiting for data to load also does not by itself
require early placement application. Inspecting restored geometry before arranging content
or related windows is another possible use, but needs a concrete scenario.
Hidden min/max/snap application and desktop restoration remain backend review gates, not
proven capabilities of this shape.

These placement scenarios, not general notification or tool-window presentation, motivate
the display operations. Existing `AppWindow.Show(false)` can also reveal after successful
hidden application, but does not itself run the initial-placement pipeline.

### Customize framework-stored placement without owning storage

The detached loader separates inspection from native application. Apps can change one selection
decision without replacing framework storage or reimplementing monitor/DPI adjustment. The
[primary-monitor example](./Window-PlacementPersistence.md#prefer-the-primary-monitor-when-the-saved-monitor-is-missing)
is the canonical extended example; the
[capture and supply rules](./Window-PlacementPersistence.md#capture-change-and-supply-placement)
own the behavior.

### Remaining contract and backend review

**Historical review disposition from the original API draft.** Decided and unfinished items are
retained to show how the design evolved. The current readiness index is
[coverage and open gates](./Window-PlacementPersistence.md#coverage-and-open-gates).

Two items from the previous revision are now decided and are no longer review gates:
`Activate()` gets no skip affordance, and a never-displayed window never saves. The
`SkipInitialPlacement` name and its `E_INVALIDARG` rejections are retained. See
[Why Activate() gets no skip affordance](#why-activate-gets-no-skip-affordance) and
[Why a never-displayed window never saves](#why-a-never-displayed-window-never-saves).
Late configuration and detached-loader errors are also decided (2026-09-24): enrollment is fixed
at first display, the current Boolean gates future saves only for enrolled windows, and
unexpected detached-load failures preserve their HRESULT. See
[the configuration rationale](#why-keep-enrollment-fixed-but-allow-saving-to-be-suspended) and
[the loader rationale](#why-propagate-unexpected-detached-load-errors).
The pre-display abort/retry boundary is decided as well (2026-09-24): a surviving, undisplayed
window keeps its phase open and retries with fresh snapshots, and a never-displayed window
remains ineligible to save. See
[Aborting before display](./Window-PlacementPersistence.md#aborting-before-display) and
[Why a pre-display abort can retry](#why-a-pre-display-abort-can-retry).
The items below remain open.

- Keep restore/save coupling as a separate future discussion. This revision does not adopt
  a persistence-mode enum, save-only switch, separate group id, or additional overload.
- Review configuration intent versus direct `AppWindow`/native first display. The current
  boundary still bypasses enrollment even with the Boolean `true`; no new interception is promised.
- Can hidden application avoid display and activation for normal, maximized, minimized, and
  snapped placement on every supported backend? Do not show and then hide to simulate it.
- Validate the proposed no-retry policy for failed hidden attempts and the exact boundary
  between successful adjusted placement and partial-application fallback. A failed operation
  is not atomic and must not be described as rolled back.
- Confirm the safe fallback state for each backend and the meaning of a successful hidden
  min/max/snap result. The Boolean and void Show fallback rules in the API pages describe
  intended behavior, not proof that the engine supplies it.
- Verify S37's hidden presenter transition and maximized return state, not only the preceding
  placement call. Hidden application clarifies sequencing but does not fix presenter behavior.
- Explicit `start /min` and `start /max` handling is deferred, not a prerequisite for adopting
  this shape. Retain native startup compatibility and placement visibility/activation safeguards.
- Monitor-policy simplification remains under discussion, not adopted. Keep the existing
  Default/Launch/ApplicationRestart monitor and state policy, peer-before-saved precedence,
  and maximized cascade policy until separately decided.

## API-shape rationale

The [public value model](../../specs/window-placement-persistence-spec.md#defaults-and-value-model)
owns property definitions and defaults. These subsections explain the choices and rejected
alternatives; their behavior summaries are decision context rather than a second API reference.

### Why use documentation terms rather than an EasyMode or ExpertMode API?

Apps can adopt one capability at a time. An app might use automatic saving with explicit initial
placement, or use cascade discovery with its own store. Two mutually exclusive API modes would
hide those combinations. The documentation introduces easy mode first, then explains expert-mode
customization using the same properties and methods.

### Why put the opt-in on Window?

Persistence is window configuration shared by initial `Show`, `Activate`, and hidden application.
This lets existing apps retain `Activate` after setting only the id. The Boolean defaults to
`true` rather than being changed by the id setter, so assignment order cannot override an explicit
opt-out. An empty id disables automatic persistence without error and preserves legacy activation.
Neither setter starts I/O or applies placement. The initial operation snapshots the Boolean and id
before callbacks and records enrollment when both permit persistence. No `WindowShowOptions`
property is retained.

Later display is not a second placement operation. Ignoring later placement-specific fields
prevents placement replay. The current Boolean and id gate each future save without changing
first-display enrollment. Clearing the id suspends saves and group membership without deleting
data; changing it alone cannot override explicit `false`.

### Why keep enrollment fixed but allow saving to be suspended?

_PM decision (2026-09-24): Accept late setter changes, but do not create late enrollment._

| Alternative | Tradeoff |
|---|---|
| Reject changes after first display, or silently freeze the value | Prevents a close handler from opting out of a save; silent freezing also makes assignment misleading |
| Recompute enrollment from the current Boolean/id | Supports late save-only adoption, but can overwrite an existing slot with a window that never opted in at first display; it also changes the direct-native-display boundary |
| Keep enrollment fixed and check the current Boolean/id at each save | Preserves setup-time opt-in and native-display behavior while allowing an enrolled window to suspend/resume saving |

Choose the third option. Initial `false` or an empty id means no enrollment, even if later
assignments make both properties look opted in. A non-empty first-display id is not a permanent
write destination: an enrolled window still saves under its current id. Missing storage at
first display does not remove enrollment or preclude a later eligible save.

This is not a new save-only mode or a way to replay restoration. Callers that need automatic
saving without initial restore can opt in before first display and use explicit placement or
`SkipInitialPlacement`. Snapshotting the Boolean together with the id also lets accepted-close
handlers suppress a save without allowing callbacks during capture to redirect or cancel it.
Property changes between hidden attempts and first display remain ordinary setup changes.

### Why propagate unexpected detached-load errors?

_PM decision (2026-09-24): Preserve the original HRESULT outside the specified null cases._

Returning null for every error would make a missing record indistinguishable from allocation,
deployment, or programming failures. Replacing all unexpected errors with `E_FAIL` would retain
an error signal but discard useful diagnosis and the language projection's specific exception.
Preserving the original HRESULT lets the app distinguish these failures without a new public
result type. The null cases still support normal first-run and unavailable-store behavior.

Automatic restore and explicit load share classification, not error handling. Automatic restore
continues to its documented fallback and reports diagnostics; an explicit load reports the error
to its caller. Neither path repairs, deletes, or creates storage while reading. Private identity,
backend, and reader results must therefore carry the original error code through to the caller.

### Why reuse PersistPlacementId for cascading?

Repeated instances of the same remembered window role already share an id. Reusing it keeps
the identity setup to one assignment. An expert-mode app can retain that identity without
opting into storage and request cascading independently through `CascadeBehavior.Enabled`.

This version deliberately couples storage identity and cascade-group identity. It does not let
different persistence ids join a separate shared cascade group. Apps that need app-specific
isolation in the unpackaged namespace include that distinction in their id.

### Why preserve explicit placement when cascading is Enabled?

Explicit placement may encode an app-selected monitor, size, or state. Requesting cascading
should not silently discard that customization. `Enabled` permits normal-position adjustment
on the chosen target monitor, while `Automatic` preserves explicit-placement precedence.
Without explicit placement, using the live peer's complete placement follows the PlacementEx
previous-window workflow and reflects where the user has moved the running instance.

### Why use an editable detached value?

The optional value is mutable because editing a captured placement is a common operation.
The six placement-state values combine normal, maximized, or snapped placement with minimization
and its restore target. The enum avoids invalid combinations of native maximize, minimize, and
arrange flags; it is not a flags enum or a model of every aspect of window state. Visibility,
activation, and presenter modes remain separate. The
[public enum description](../../specs/window-placement-persistence-spec.md#windowplacementstate-enum)
defines the combinations.
Current resizability is live window configuration, not a durable property copied from another
window. No native off-screen opt-out is exposed in this version.

Capture and loading return detached objects that the app can edit directly. Applying placement
takes a coherent snapshot, so later edits do not affect that operation. No public copying method
is needed for these scenarios. A [C++/WinRT copy constructor][Copy-construction guidance] copies
a reference rather than the underlying object's contents.

`Placement = null` in an initial request selects the permitted peer/stored/fallback chain
instead of explicit data. It does not disable automatic restore. A non-null value replaces
automatic loading while retaining opted-in saving. There is no staged value to clear.
Hidden application returns success for application, not acceptance of deferred work or durable
saving. It is repeatable and does not end the initial-placement phase, so the first display still
decides final placement unless it sets `SkipInitialPlacement`.

### Why prefer Show while retaining Activate adoption?

`Show` is the preferred display entry point for the new placement contract. It carries the
placement policy and per-display activation option, and can reveal a prepared minimized window
without restoring it. Non-activating restart and hidden preparation before presenter changes
motivate this distinction, rather than general focus-management features.

`Activate` keeps its existing reveal, restore-from-minimization, and activation meaning.
When initially opted in through the window property, it also selects automatic placement with
default reason/cascade policy. It does not read a retained options object or become a
non-activating restart display. Existing apps can retain the call; use Show options or hidden
application before Activate for custom placement policy.

### Why are placement options initial-only but DoNotActivate per-Show?

Source, geometry, reason, and cascading are decided once. Persistence enrollment is snapshotted
at first display; late Boolean changes gate only future eligible saves. Later display must not move a window
that the user or app has changed or replay a cascade. `DoNotActivate` instead describes the current display request, including
a reveal after hidden application. Apps do not need to clear retained initial options because
the window retains no such property.

Hidden application is a distinct operation rather than a Show request that suppresses display.
Its name states that it applies now and only during the initial-placement phase. A first `Show`
with `SkipInitialPlacement = true` reveals the result without rerunning placement; a first `Show`
or `Activate` without skip applies the automatic pass instead.

### Why an explicit skip option rather than a Showing event?

Preserving prepared placement needs a way to tell the first display not to place the window. One
alternative is a cancelable `Showing` event that the app handles to suppress the automatic pass.
We prefer the explicit option for these reasons:

- The caller already owns the sequence. The code that calls `TryApplyInitialPlacement` is usually
  the code that calls `Show`, so it can pass the option directly.
- An event adds a new contract: ordering against XAML content loading, reentrancy during the
  event, what cancellation means for `Show` callers who did not subscribe, and what happens when
  the first display comes from `AppWindow` or a native call.
- An option is statically checkable. Combining it with an explicit `Placement`, or passing it to
  `TryApplyInitialPlacement`, fails with `E_INVALIDARG` at the call site.
- `Activate()` takes no options, so an event would be the only way to skip from `Activate`. We
  would rather document that `Activate()` cannot skip than add an event to enable it.

The cost is that hidden preparation followed by a plain `Show()` or `Activate()` replaces the
prepared placement. `Show` can express the skip; `Activate()` cannot. The next section resolves
that asymmetry.

### Why Activate() gets no skip affordance

`Activate()` takes no parameter and reads no retained options, so an opted-in first `Activate()`
always runs the automatic placement pass. If the app already prepared the window with
`TryApplyInitialPlacement`, that pass replaces the prepared placement. We considered these ways
to close the gap.

| Option | Assessment |
|---|---|
| Keep `Activate()` optionless and document the `Show`-then-`Activate` sequence | Adopted |
| Add an `Activate(WindowShowOptions)` overload | Rejected. It duplicates `Show(options)` with different activation semantics and grows the API surface for one recovery case. `Show(options)` already expresses every field such an overload would carry. |
| Skip the automatic pass implicitly after a successful hidden application | Rejected. It makes the first-display contract depend on the hidden-application success boundary, which is still a backend review gate. A partial application would decide whether the first display places the window. |
| Skip the automatic pass implicitly after any `TryApplyInitialPlacement` call | Rejected. It removes the retry path, and a WinRT Boolean cannot distinguish an unset `SkipInitialPlacement` from an explicit `false`, so the caller could not ask for the pass again. |
| End the initial-placement phase on hidden application | Rejected. It contradicts the repeatable-until-displayed contract and makes one failed hidden attempt permanently forfeit automatic placement. |
| Add a retained `Window` skip property or a cancelable `Showing` event | Rejected earlier. See the section above and [Why are placement options initial-only but DoNotActivate per-Show?](#why-are-placement-options-initial-only-but-donotactivate-per-show) |

The deciding argument is asymmetric cost. Every implicit-skip variant removes the documented
retry path while hidden application is still unproven for maximized, minimized, and snapped
placement. A caller that loses the automatic pass has no way to ask for it back. A caller that
calls `Activate()` in the wrong order loses only the prepared placement and still gets a placed,
displayed window.

The exposure is also narrow. The gap needs a window that is opted in, is prepared with
`TryApplyInitialPlacement`, and then uses `Activate()` rather than `Show` for its first display.
An opted-out window's `Activate()` never ran a placement pass, and an app that never prepares a
hidden window has nothing to lose.

To keep the gap from being silent during development, the implementation traces a debug
diagnostic when an opted-in first `Activate()` follows at least one `TryApplyInitialPlacement`
call on the same window. This is development output only. It is not an event, not a failure, and
not part of the behavioral contract, and apps must not depend on it.

### Why a pre-display abort can retry

A first display reserves the operation before its callbacks run, so no callback can start a
second placement pass. Earlier revisions described that reservation as the end of the phase,
while the lifecycle tables ended the phase only on actual display. The two descriptions differ
when a validated attempt aborts before the window is visible and the window survives.

| Option | Assessment |
|---|---|
| The phase stays open; the next call runs a fresh pass | Adopted |
| The reservation permanently consumes the opportunity | Rejected. The window is still hidden, so nothing the user can see happened, yet the app would lose automatic placement for the life of the window with no way to ask for it back. |
| The reservation consumes the opportunity, but record enrollment anyway | Rejected. It creates a window that is enrolled and never displayed. That state exists only to be filtered out at save time, and any bug in that filter writes placement the user never saw. |
| Retry the pass, but lock enrollment to the aborted attempt's snapshot | Rejected. It splits one contract into two rules with different snapshot times for no benefit. The retry already reads the same properties. |
| Expose the phase state so apps can decide | Rejected. It adds API surface for a failure the app can handle by calling `Show` again. |

Three properties decide this. First, the phase is defined by display, not by intent, everywhere
else in this document: validation failures and hidden application both leave it open. Second,
enrollment is recorded at first display, so with no display there is nothing to record and no
stale snapshot to carry. Third, saving already requires an actual display, so a retry cannot
make a never-displayed window save under any of these options.

The repeat cost is bounded. Each attempt is app-driven and does the same work as a first `Show`,
which is the same position hidden application is already in: repeatable while the window stays
hidden. An app that loops on a failing `Show` has a larger problem than placement.

### Why a never-displayed window never saves

Automatic saving requires enrollment at first display and an actual display, so a window that is
prepared with `TryApplyInitialPlacement` and never displayed never saves. This removes a
capability an earlier revision allowed. We audited the scenarios that could have depended on it.

| Scenario | Needs saving a never-displayed window? | Result |
|---|---|---|
| Prepare hidden, reveal with a skipping `Show`, then close | No. First display enrolls and establishes eligibility. | Saves |
| Prepare hidden, move the still-hidden window, reveal, then close | No. Hidden position and size changes are captured at save time. | Saves |
| Display a window, hide it, then close it while hidden | No. Enrollment came from the first display, and hiding does not suspend saving. | Saves |
| Prepare hidden, then close or sign out without ever displaying | Yes | No save |
| Preallocate hidden windows and track their layout | Yes | No save. Use `TryGetPlacement` with app-owned storage. |

Only the last two rows lose behavior, and in both of them saving is undesirable:

- Hidden preparation usually applies the value that was just loaded, so saving it back is a no-op
  at best. When that value was adjusted for the current monitor, work area, or DPI, saving it back
  writes the adjusted rectangle over the original. An app that repeatedly prepares windows it
  never displays would ratchet its saved placement toward the adjusted result.
- When hidden application returns `false`, the window holds fallback geometry that the user never
  saw and never chose. Saving that would overwrite real saved placement with a default.

The removal therefore protects stored data rather than costing a scenario. An app that must
persist geometry for windows it never displays can capture with `TryGetPlacement` and write to
its own storage, as in [Supply placement from app-owned data](./Window-PlacementPersistence.md#supply-placement-from-app-owned-data).

### Why use application identity rather than package family alone?

The default packaged app-data store is package-family scoped. A [package family][Package identity]
is stable across package versions and architectures, but a package can contain multiple
applications. Package-family scope alone would make two such applications using `"MainWindow"`
overwrite the same placement slot.

WinUI adds an application-specific namespace so each app can choose its placement ids
independently. Multiple processes of the same app still share slots. This does not require a
new public setting or make the app responsible for prefixing ids with deployment information.

The implementation should resolve the registered packaged application identity using
[GetCurrentApplicationUserModelId][Current application identity] and scope its private storage
accordingly, for example with an application-specific container above the existing placement-id
keys. Use the registered identity, not a display name, process id, window title, or versioned
package full name.

If identity cannot be resolved, automatic persistence is unavailable for that operation.
Falling back to package-family-only storage would reintroduce the collisions this rule prevents.
Likewise, a missing application-scoped value must not fall back to an older package-wide slot
whose owning application cannot be established.

### Why is there no API to delete saved placement?

Automatic placement uses the packaged app's local app-data store. Windows removes that store
when the app is uninstalled, so apps do not need a placement-specific API for uninstall cleanup.
App updates preserve the store.

Deleting selected entries while the app remains installed is a separate scenario, such as
resetting one window's saved layout or removing an unused document id. We are leaving that API
out of this version. We can revisit it if app scenarios show a need for a dedicated deletion API.

Clearing `PersistPlacementId` suspends future saving and group membership without erasing
saved data. For an initially opted-in window, assigning a non-empty id again permits future
eligible saves. Explicit Boolean opt-out preserves grouping without automatic storage.


## Launch-monitor hint investigation

**Historical investigation, 2026-09-25.** No new launch experiments were performed for this refactor.
The public `Launch` policy remains in the
[feature specification](./Window-PlacementPersistence.md#choose-launch-or-restart-policy).

The `STARTUPINFO` preferred-monitor hint was not observed on any measured launch path. A
`GetStartupInfoW` probe recorded `hStdOutput=0x0` for a direct process launch, an `explorer.exe`
launch, and a `ShellExecute` launch. The only measured launch that populated the field was one with
redirected standard output, which sets `STARTF_USESTDHANDLES` and means the field is a file handle
rather than a monitor. WinUI ignores the field when that flag is set.

Two questions remain open, and neither is settled by the measurements above:

| Open question | Status |
|---|---|
| Whether a packaged (MSIX/AUMID) activation supplies the hint | Not measured. [Related bug 58693034](https://task.ms/58693034) is a possible lead, not a confirmed explanation. |
| Whether a Start menu, taskbar, jump list, or desktop shortcut launch supplies the hint | Not measured. These paths need an interactive session to exercise. |

Earlier notes described this as a packaged-app propagation problem. The measurements do not support
that scope, because unpackaged shell launches also supplied no hint.

The intended `Launch` policy is unchanged: use a valid hint when available; otherwise use normal
saved-monitor selection. Until a launch path is shown to supply a hint, apps launched on the
measured paths get saved-monitor selection.

The earlier in-line note called this a packaged propagation question and suggested an OS bug if
appropriate. The later inventory above narrows what the measurements support: unpackaged shell
launches also lacked a hint, and the packaged/interactive paths were not measured. Neither note
settles monitor-policy simplification or adopts automatic inference of a main-window role.

## Launcher show-command scenarios (deferred)

_Scope decision (2026-09-22): Explicit launcher show-command handling is deferred for this
version, including `start /min` and `start /max`. The earlier sketch below is retained as
design history, not a requirement or an adoption gate for the adopted API shape. This decision
does not defer saved-state restoration during application restart or decide monitor-hint policy.
The sketch retains historical policy discussion, including activation overrides. Hidden-operation
examples use the current `TryApplyInitialPlacement` spelling for clarity; this does not adopt
the deferred launcher-state merging or its proposed outcomes._

### Earlier alternative: Launch opts into startup show requests

Earlier tentative direction (2026-09-21, now deferred): when a window is initially shown with `Reason = Launch`
and the process was launched with `start /min`, minimize that window while retaining its
restore target. Initial `Activate()` should not immediately undo that minimization. This
would broaden `Launch` from selecting monitor policy to opting into startup show requests,
without introducing a main-window designation.

If revisited, determine whether `/max` should follow the same rule, whether the request affects
every `Launch` window or only one, and how to handle prior consumption by a splash window or
other window without unexpected native replay. This direction was not adopted by the main
contract or the earlier sketch below; its instruction-ownership rules remain unresolved.

### Applicability in the earlier sketch

In these tables, an **applicable instruction** is a startup instruction that the native integration
has confirmed should affect this window (HWND). The integration must also know how to handle it
without Windows applying it unexpectedly later. The reviews did not establish how to do this.
The tables depend on solving that problem; a window's first `Show()` call is not enough to prove it.

### Proposed precedence

Initial-placement policy and launcher instructions have different purposes. Placement supplies
the previous geometry and state. A launcher can request that this opening start minimized or
maximized. An ordinary launch should not discard a remembered maximized or snapped state.

The proposal works out the complete operation before changing the native window:

1. Validate initial options and resolve the calling method's overrides.
2. Select explicit, live-peer, stored, or fallback placement under the main pipeline's rules,
   including the disable flag and cascade policy. Apply reason policy, cascade adjustment,
   saved-minimization normalization, and current-display adjustments.
3. Merge an applicable launcher instruction into the planned state. `Reason = Launch` does not
   grant ownership of it. Neither does being the first window created by WinUI.
4. Combine the effective app options with the launcher's visibility and activation restrictions.
   Either can suppress display or activation; neither can remove the other's restrictions.
5. Execute only operations that preserve those restrictions, including during intermediate
   native transitions and fallback.

A launcher instruction that suppresses activation does not change the reason-policy row.
WinUI first selects that row using the effective app options, then adds the launcher's restrictions.

An applicable maximize request selects `Maximized` while retaining the normal rectangle, not
the snap rectangle, as the later restore position. An applicable minimize request retains the
unminimized restore target: normal, maximized, or snapped. If the selected placement is already
minimized, its existing restore target is retained rather than treating minimized as a restore
target itself.

Resolve snap availability during preparation. If snap cannot be retained, record normal as the
restore target. A retained snapped target remains subject to current system capabilities when
the window is later restored. Re-enabling snapping does not recreate history already discarded.

The launcher instruction is separate from saved minimization. Initial `Activate()` normalizes
saved minimization and overrides the app's hidden/non-activating options, but an applicable
launcher minimize request can still make this first display minimized. The pipeline must not
immediately undo that request with a second restore operation. A later `Activate()` is an
explicit request to restore the minimized window and request activation. An activation request
does not itself guarantee a non-minimized foreground window.

### Proposed command categories

This is a placement-aware translation of startup input, not literal equivalence to every raw
[ShowWindow command][ShowWindow startup behavior]. In particular, normal-style startup input
does not reset a remembered maximized or snapped placement.

| Startup input | Planned placement state | Additional display restriction |
|---|---|---|
| No `STARTF_USESHOWWINDOW` | Keep the reason-policy result | None |
| `SW_SHOWNORMAL`/`SW_NORMAL` (1), `SW_SHOW` (5), or `SW_RESTORE` (9) | Keep the reason-policy result; ordinary launch in this sketch | None |
| `SW_SHOWMAXIMIZED`/`SW_MAXIMIZE` (3) | Maximized with the normal restore rectangle | None |
| `SW_SHOWMINIMIZED` (2) | Minimized with the selected restore target | Target activation is permitted if effective app options permit it |
| `SW_MINIMIZE` (6) or `SW_SHOWMINNOACTIVE` (7) | Minimized with the selected restore target | Do not request target activation |
| `SW_SHOWNOACTIVATE` (4) or `SW_SHOWNA` (8) | Keep the reason-policy result | Do not request target activation |
| `SW_HIDE` (0) | Keep the reason-policy result while hidden | No display or activation |

The state-preserving treatment of commands 1, 4, and 9 is an intentional placement policy, not
a description of their raw Win32 effects. The operation does not synthesize extra focus transfers
to reproduce those effects. In particular, effective `DoNotActivate` continues to prohibit focus
transfer throughout the operation.

Minimization does not imply non-activation: command 2 permits activation, unlike command 7.
`start /min` is scenario shorthand, not a substitute for testing the exact startup command.
The native [STARTUPINFO contract][STARTUPINFO show command] excludes `SW_SHOWDEFAULT` from
`wShowWindow`; it is not another ordinary startup value in this table. Values not covered here,
including `SW_FORCEMINIMIZE`, require an explicit handling decision before this sketch is promoted.

### Proposed launch outcomes

These rows assume an overlapped window, a startup instruction known to apply to it, and a backend
that can safely apply the planned state. Activation follows the command table and effective app
options. Safe fallback may not reproduce the saved state exactly, as described below.

| Selected placement | Applicable launcher instruction | Proposed initial result |
|---|---|---|
| `Normal` | Ordinary launch | Normal at the adjusted normal rectangle |
| `Maximized` | Ordinary launch | Maximized; retain the adjusted normal rectangle for restore |
| `Snapped` | Ordinary launch | Snapped if supported; otherwise normal |
| `MinimizedFromMaximized` | Ordinary launch with ordinary placement policy | Maximized after saved-minimization normalization |
| `Maximized` | Minimize | Minimized; restoring returns to maximized |
| `MinimizedFromMaximized` with ordinary placement policy | Minimize | Normalize to maximized, then minimize with maximized as the restore target |
| `Snapped` | Minimize | Minimized; retain snapped as the restore target where supported |
| `Normal` | Maximize, such as a shortcut configured to run maximized | Maximized; retain the adjusted normal rectangle for restore |
| Explicitly staged `Snapped` | Maximize | Maximized; restoring returns to the normal rectangle, not the old snap rectangle |
| `MinimizedFromMaximized` on non-activating application restart | Ordinary launch | Keep minimized and attempt the saved desktop |
| Same non-activating restart | Maximize | Maximize without activation and attempt the saved desktop |
| No usable placement | Minimize or maximize | Use fallback geometry; apply the launcher state only through a safe supported operation |
| Selected placement could not be applied | Minimize or maximize | Use safe fallback and valid current bounds; do not reload automatic placement or claim the failed state was applied |
| Any placement | Instruction already handled by a splash window or another window | WinUI does not replay it; use this window's selected placement policy |

Two precedence choices are deliberate proposals:

- Explicit placement wins over automatic loading, not over an applicable launcher instruction.
  The instruction can change the planned state without modifying the app's staged object.
- Restart policy preserves saved minimization unless an independently applicable explicit
  launcher instruction overrides it. A current maximize request wins over historical minimization,
  but does not cancel `DoNotActivate` or the restart desktop policy. Without that instruction, an
  all-minimized restart inventory remains minimized.

This keeps one state-precedence rule rather than introducing a restart-specific exception.
Assigning `ApplicationRestart` to several windows does not make a startup instruction applicable
to each of them. If revisited, the qualifications would need reconciliation with the current
conceptual pipeline, restart policy table, and display and hidden-application API pages.

### Hidden preparation and later reveal

These hidden results depend on first solving startup-instruction handling: WinUI must know the
instruction applies to this window and how to avoid an unexpected later application. The proposal
then resolves the launcher state during the hidden initial operation, rather than saving a command
to replay when showing the window later. In this sketch, "a skipping `Show()`" means
`Show(new WindowShowOptions { SkipInitialPlacement = true })`, which reveals the prepared state
without rerunning placement. The window stays hidden throughout, with no
`VisibilityChanged` or activation events caused by preparation. Successfully changing a hidden
window's state does not prove that Windows has finished handling the startup instruction.

| Initial operation and placement | Applicable launcher instruction | Proposed prepared state | Later call and result |
|---|---|---|---|
| `TryApplyInitialPlacement(options)`, saved maximized | Minimize | Hidden, minimized, restore target maximized | A skipping `Show()` reveals minimized without activation |
| Same preparation | Minimize | Hidden, minimized, restore target maximized | `Activate()` after that skipping `Show()` restores to maximized and requests activation |
| `TryApplyInitialPlacement(options)`, saved normal | Maximize | Hidden, maximized | A skipping `Show()` reveals maximized and requests activation |
| `TryApplyInitialPlacement(options)`, no usable placement, safe state operation available | Minimize | Hidden, minimized using fallback geometry | A skipping `Show()` reveals minimized without activation |
| Hidden operation falls back to geometry-only preparation | Minimize or maximize | Hidden at valid current geometry/state, not a claimed min/max result | Later calls use the actual prepared state without retrying placement |
| Hidden initial operation has handled the instruction, then another window is displayed | Any handled instruction | No deferred startup instruction | WinUI does not reapply it to the other window |

The no-replay proposal applies to WinUI's handling. Your app can separately request native startup
handling again with `ShowWindow(SW_SHOWDEFAULT)`; these APIs do not block or intercept every
later native operation your app makes.

Hidden preparation does not establish automatic-save eligibility. Actual display does: a first-run
window shown maximized using fallback geometry can save that placement on an accepted close. If the
app needs a later non-activating reveal of a non-minimized window, it uses
`AppWindow.Show(false)` because later display calls ignore initial options.

### Conditional future launcher review gates

The following gates were part of the deferred review, not requirements to implement launcher-state
merging in this version. Existing native startup compatibility and visibility/focus safeguards
remain current requirements in the
[feature fallback section](./Window-PlacementPersistence.md#safe-fallback-and-review-gates).

- For future explicit command handling, establish whether the native startup instruction is still
  applicable and how its native handling is completed. `GetStartupInfo` provides creation-time
  input, not the target HWND or consumption status. Neither a per-window first-show flag nor a
  WinUI-only process latch accounts for arbitrary earlier native or `AppWindow` activity.
- If explicit command handling is added, carry startup-derived restrictions through the outer WinUI display, activation, and XAML
  focus paths, not just the placement helper. In particular, a no-activate startup variant must
  not be followed by an unconditional activation request.
- Test each command category in a fresh process, with and without stored data, with direct
  native/AppWindow display, an earlier splash window, two UI threads, and partial application failure.
  For hidden cases, reveal the prepared window and then display another window to detect replay.
  Record native visibility/focus changes and XAML events, not only the final screenshot.
- This state-merging sketch covers overlapped windows. It does not authorize changing an
  app-selected full-screen or compact-overlay presenter. Define any interaction with native
  launcher state for those presenters before promoting the sketch.

**Historical-only wording:** the sketch's final `AppWindow.Show(false)` guidance predates the
current per-Show `DoNotActivate` rule. Its hidden-save wording must not be read as enrollment:
the adopted design enrolls only at first display. Deferred precedence tables are not public
guarantees and require reconciliation with the current API before any future promotion.

## Wanted dependency: unpackaged default ApplicationData

**Open dependency, not an adopted API extension.** The
[API scope](../../specs/window-placement-persistence-spec.md#compatibility-and-scope) continues to
limit automatic storage to packaged applications. These notes preserve the desired future
capability and unresolved design questions.

We still want a Windows App SDK API that lets a framework obtain the process's default
`ApplicationData` without knowing whether the app is packaged or how an unpackaged app chose its
storage identity. The intended flow is:

1. The unpackaged app establishes its default storage identity through Windows App SDK before a
   framework first needs storage.
2. WinUI queries the default `ApplicationData`, ideally through `GetDefault()`, and uses
   `LocalSettings` without a XAML-specific store setter.
3. The app owns the unpackaged identity and its data-cleanup lifecycle. WinUI does not infer identity
   from an executable, window class, or shell AUMID.

The earlier name `SetDefaultForUnpackaged(publisher, product)` illustrates possible setup, not a
committed API. The required capability is framework discovery of the configured default; the final
setup/query shape, availability, and failure contract need Windows App SDK agreement.

`GetForUnpackaged(publisher, product)` is related but insufficient by itself: it requires the caller
to already know the app's publisher/product values and does not register the result as the process
default. WinUI must not invent those values or add a competing public storage-configuration API.

Before enabling automatic unpackaged persistence, settle default initialization timing, repeated or
late configuration, thread safety, missing-default behavior, and runtime/deployment requirements.
The storage layer must also provide enough stable identity/scope to keep app data separate across
processes and users without borrowing the cascade feature's shared unpackaged namespace. The
application-digest container below describes the packaged path; an unpackaged namespace is not
implicitly derived from a nonexistent registered packaged application identity.

Apply the same resolved default and namespace rules to automatic loading, detached loading, and
saving. Decide how configuration interacts with cached successful store resolution so a window
cannot load from one store and silently save into another. Record those decisions in the API spec
before broadening its packaged-only contract. If the dependency is unavailable, retain that
packaged-only behavior rather than dropping the unpackaged-default-store goal.

### Related Windows App SDK work

A quick PR and source check on **2026-09-23** found:

| PR | Status at review | Relevance |
|---|---|---|
| [#6277: ApplicationData unpackaged support](https://github.com/microsoft/WindowsAppSDK/pull/6277) | Merged 2026-04-01 | Implements unpackaged support through `GetForUnpackaged(publisher, product)`; does not supply the configured process default |
| [#6378: Packaged/unpackaged implementation refactor](https://github.com/microsoft/WindowsAppSDK/pull/6378) | Merged 2026-05-27 | Separates implementation types; not a default-store registration API |
| [#6558: Unpackaged LocalSettings path fix](https://github.com/microsoft/WindowsAppSDK/pull/6558) | Merged 2026-06-12 | Fixes the explicit unpackaged store's backing path; not default-store discovery |
| [#6731: ApplicationData path optimization](https://github.com/microsoft/WindowsAppSDK/pull/6731) | Open | Related maintenance, not the missing setup/query capability |

The inspected [ApplicationData IDL](https://github.com/microsoft/WindowsAppSDK/blob/main/dev/ApplicationData/ApplicationData.idl)
still documents package identity as a requirement for `GetDefault()`. Its
[implementation](https://github.com/microsoft/WindowsAppSDK/blob/main/dev/ApplicationData/M.W.S.ApplicationData.cpp)
resolves the current package family, while `GetForUnpackaged` constructs an instance for explicitly
supplied publisher/product values. The search did not find a PR adding the wanted unpackaged default.
This is a dated search result, not proof that no differently named proposal exists or that a merged
change is available in a particular released package.

## Virtual-desktop refresh scheduling

The implementation design chooses a posted registered window message rather than a helper thread.
These are the recorded reasons for that choice; the operative scheduling/cache design lives in
[the feature specification](./Window-PlacementPersistence.md#cache-optional-virtual-desktop-identity-safely).

| Choice | Reason |
| --- | --- |
| Posted, not sent | The documented shell failure is `RPC_E_CANTCALLOUT_ININPUTSYNCCALL`, which only happens inside an input-synchronous call. A posted message is dispatched by the window's own pump, so it is a safe call-out point. |
| Registered message | Avoids colliding with anything the app posts to the same window. |
| After display, not inline | The shell commonly fails with `TYPE_E_ELEMENTNOTFOUND` until the taskbar knows about the window. |
| No helper thread | Removes the COM apartment and thread-lifetime work a background query would need. The UI thread already meets the helper's apartment requirement. |

## Implementation history

**Archive begins here.** Status statements, commands, test results, source findings, and handoffs
below are relocated from the original design notes. They retain evidence detail, including
failures and exclusions. They do not imply that a run occurred during this refactor.

### First implementation unit

The private binary record codec and saved-snapshot structural validator are implemented in
[`WindowPlacementRecord.h`](../../dxaml/xcp/dxaml/lib/WindowPlacementRecord.h) and
[`WindowPlacementRecord.cpp`](../../dxaml/xcp/dxaml/lib/WindowPlacementRecord.cpp).
The dedicated native `WindowPlacementRecordTests` class covers golden records for all six states,
optional metadata, versioning, bounded TLV parsing, malformed/duplicate fields, and checked geometry.
The codec compiles into the framework's WinRT foundation library; the isolated test project compiles
the same source without requiring a window, storage, or a display configuration.

This unit consumes and produces **decoded binary bytes**, not settings strings. Base64 transport
(including its encoded-length bound), storage/identity/retention, public API projections, capture,
native placement, and window lifecycle integration remain unimplemented in this unit. No window
currently calls the codec. The internal snapshot types are not public API declarations, and
implementing this unit does not ratify the proposed durable format or establish end-to-end
persistence support.

Validation on 2026-09-23: `.\initrun.ps1 .\build.cmd /q` succeeded for `amd64chk`.
`.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <vm> '*WindowPlacementRecordTests*'`
refreshed the test payload and passed all 13 tests (none failed, blocked, or skipped). These are
data-only tests; they do not establish native placement, multi-monitor, or variable-DPI coverage.

### Second implementation unit

**Historical checkpoint.** The later formatting review supplies its dated validation.

Base64 transport and the private container/value names are implemented in
[`WindowPlacementStorageFormat.h`](../../dxaml/xcp/dxaml/lib/WindowPlacementStorageFormat.h) and
[`WindowPlacementStorageFormat.cpp`](../../dxaml/xcp/dxaml/lib/WindowPlacementStorageFormat.cpp).
The transport wraps the existing codec rather than repeating the wire format: `EncodeRecordText`
encodes a record to base64, and `DecodeRecordText` enforces the
[encoded-length bound](./Window-PlacementPersistence.md#envelope-and-bounds) before allocating a decode buffer, rejects
non-canonical base64, and then calls `DecodeRecord`. The naming helpers implement the
[proposed container and value names](./Window-PlacementPersistence.md#proposed-packaged-container-and-value-names), including the
uppercase Base32 SHA-256 digests, the 16-character readable slug, and the `id` fallback.
`BCryptHash` supplies the platform SHA-256.

The dedicated native `WindowPlacementRecordTextTests` and `WindowPlacementStorageNameTests` classes
cover golden encoded text, all three padding lengths, malformed and non-canonical text, the
encoded-length bound, version reporting, golden digests for ASCII, non-ASCII, case-distinct, and
surrogate-containing ids, non-normalized ids, slug rules and truncation, embedded NUL code units,
empty-id rejection, and the documented maximum name lengths.

This unit still performs **no storage I/O**. It resolves no application identity, reads and writes
no settings, and enforces no retention. Callers must supply the registered application id and the
raw placement id. Public APIs, capture, native placement, and lifecycle integration remain
unimplemented, and no window calls this code.

### Third implementation unit

The internal [`WindowPlacementStore`](../../dxaml/xcp/dxaml/lib/WindowPlacementStore.h) adapter now
owns shared reader outcomes, complete-value replacement, positive save-sequence allocation,
per-namespace save coordination, and best-effort retention of the 32 newest placement records.
Its settings backend is a narrow ABI boundary, with
[`ApplicationDataPlacementSettingsBackend`](../../dxaml/xcp/dxaml/lib/WindowPlacementStore.h)
providing the packaged `ApplicationData.LocalSettings` implementation. Reads look up existing
containers and values without creating them; writes create the placement container and replace the
complete string value. Isolated tests can inject missing containers, wrong value types, corrupt or
oversized records, replacement failures, cleanup failures, unrelated values, recency ordering,
sequence overflow, and HRESULT failures. A real backend test covers the no-package unavailable
classification. Packaged identity resolution uses `GetCurrentApplicationUserModelId` and never
falls back to package-wide or unpackaged storage. This unit does not expose a public API or perform window
integration.

Validation on 2026-09-23: `.\initrun.ps1 .\build.cmd /q` succeeded for `amd64chk`, and
`.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <vm> '*WindowPlacement*Tests*'` passed all 29
tests in the isolated project (none failed, blocked, or skipped). These remain data-only tests.

Validation on 2026-09-24: the `amd64chk` build succeeded, and
`*WindowPlacementStoreTests*` passed all 12 tests on the `ge_current-260828-Desktop` VM.

### Public Window placement capture API

**Historical implementation report.** Scope and current public requirements remain in the API proposal.

The WinUI contract 12 `Window.TryGetPlacement` API is implemented for desktop windows.
It synchronously captures the effective current placement and returns an independent
`WindowPlacement` object. UWP returns `false` because placement capture is not supported
by that implementation. The dedicated `WindowPlacementPersistenceTests` integration
coverage validates the desktop API against a real WPF-hosted window.

## Dependency-ordered implementation plan

**Historical rolling plan, including updates through the source's 2026-09-25 checkpoint.**
The opening completion statement is the original report, not acceptance of all public scenarios.
The detailed native records and their limitations remain necessary to interpret it.

Steps 1 through 6 are implemented. Step 7 is implemented except for its display-environment
gates. Each step below keeps its original scope
statement followed by a status note, so a reviewer can check the claim against the named source.
Each step has a narrow owner boundary so it can be implemented and
committed in one developer round without treating isolated helpers as feature completion.

1. **Packaged store adapter and retention.** Build an internal adapter over the packaged
   `ApplicationData.LocalSettings` ABI. Resolve the registered application identity with
   `GetCurrentApplicationUserModelId`, derive the application and placement names with the existing
   formatting helpers, and read or replace complete string values without creating containers on
   load. Return the five reader outcomes in this document. Add the per-namespace save/cleanup
   guard, positive save-sequence allocation, whole-value publication, and best-effort 32-record
   trimming. Cover missing/type-invalid/oversized/corrupt/unsupported values, sequence ordering,
   failure between replacement and deletion, and unrelated-value isolation with a dedicated store
   test class. This step depends on the record codec and formatting helpers.
   _Implemented (2026-09-24): `ApplicationDataPlacementSettingsBackend` uses the Windows
   `ApplicationData` ABI, preserves phase-specific HRESULT classifications, does not create
   containers while loading, and creates them only for writes. The dedicated store tests cover
   injected failures and the real no-package backend result._
2. **PlacementEx adapter and policy seam.** Replace the current policy implementation's duplicated
   monitor selection, normal/snap adjustment, and cascade arithmetic with a thin adapter over
   `PlacementEx`. The adapter should construct and validate the engine's working placement, call
   `FindClosestMonitor`, `MoveToMonitor`, and `Cascade` (or the corresponding native operation),
   and translate only the WinUI-owned state, constraint, launch-hint, explicit-cascade, and
   visibility/activation decisions. Do not retain a second geometry engine for production behavior.
   Keep storage and policy independent. Unit tests may use an injected adapter seam, but expected
   geometry must come from the imported engine or an explicitly documented engine-compatible test
   double. Add coverage for differently sized monitors where center-distance selection differs from
   `MonitorFromRect`, work-area/DPI changes, constraints, explicit position-only cascading, and
   no-source-retry-after-apply. This step depends on the store reader only for source data.
   _Step 2A implemented: `WindowPlacementAdapter.h/.cpp` provides checked translation between
   `NativeRequest` and a `PlacementEx` working value._
   _Step 2B implemented: `WindowPlacementPolicy.cpp` no longer carries its own geometry or topology
   algorithms. `SelectMonitor` and `ComputePlacement` call `TrySelectEngineMonitor`,
   `TryCreateEnginePlacement`, `TryMoveEngineToMonitor`, `TryCascadeEngine`, and
   `TryReadEnginePlacement`. The policy unit retains only the WinUI-owned decisions: state mapping,
   cascade permission, constraint clamping, and native request shaping._
   _Step 2C progress (2026-09-24): native application is wired for explicit/saved placement and
   launch fallback. See [native application status](#native-application-status-2026-09-24) for
   capability restrictions, actual validation, and remaining acceptance work._
3. **Capture adapter and placement cache.** Integrate `PlacementEx::GetPlacement` with
   `DesktopWindowImpl` state caches. Preserve meaningful overlapped state while hidden, track
   minimized-from-snapped history, retain complete pre-full-screen snapshots, capture physical
   coordinates, and safely cache optional virtual-desktop identity. Add focused native tests for
   hidden moves, presenter transitions, failed/partial application, message-safe capture, and
   HWND lifetime correlation. This step depends on the private snapshot and native adapter.
   _Not accepted (2026-09-23): the landed `WindowPlacementCapture` re-implements capture with raw
   Win32 instead of `PlacementEx::GetPlacement`, and its state, coordinate-space, minimized, and
   hidden handling are wrong. See the
   [capture and DesktopWindowImpl integration review](#capture-and-desktopwindowimpl-integration-review-2026-09-23)._
   _Reworked and implemented (2026-09-24): capture now runs on `PlacementEx::GetPlacement` plus
   `TryReadEnginePlacement`. See the
   [capture rework](#capture-rework-on-the-placement-engine-2026-09-24)._
4. **Public API projections and detached loading.** Add the `WindowPlacement`, state/options/enums,
   and `Window` property/method projections in the generated/API declaration pattern used by the
   repository. Implement validation-before-side-effects, snapshot independence, C#/C++/WinRT
   projection behavior, and `LoadForPersistPlacementId` through the shared store reader. Detached
   loading must not consult a window, select peers, adjust or apply placement, change phase state,
   or enroll saving. Add a dedicated projection/API test class. This step depends on the store
   reader and private snapshot types.
   _Implemented: XamlOM projects `WindowPlacement`, `WindowShowOptions`, and their three enums. The
   value objects support constructor validation, permissive editing, nullable fields, thread-safe
   properties, and private coherent request snapshots. `Window` exposes `PersistPlacementId`,
   `UseAutomaticPlacementPersistence`, `Show(WindowShowOptions)`, and `TryApplyInitialPlacement`
   through `WindowImpl`, with desktop behavior in `DesktopWindowImpl` and no-op desktop-only
   members on `UWPWindowImpl`. `WindowPlacementFactory::LoadForPersistPlacementIdImpl` reads
   through the shared store without consulting a window, selecting peers, adjusting placement, or
   enrolling saving. `WindowPlacementApiTests` covers the managed projection and
   `WindowPlacementPublicTests` covers request validation and independent native copies._
   _C++/WinRT runtime coverage decision (2026-09-24): Do not add a dedicated C++/WinRT test lane
   for this implementation unit. Such a lane would verify that a C++/WinRT header generated from
   the locally built winmd can compile and consume the projected signatures, and could expose
   C++/WinRT-specific header code-generation or value-marshaling defects. The managed projection
   tests already exercise the public projection at runtime, while the ABI tests exercise the
   underlying WinRT interface, factory, validation, and value behavior. Because the C++/WinRT
   header is a thin wrapper over that same ABI, the proposed lane would add header-consumer
   validation but no distinct placement behavior or ABI coverage for this unit. Header generation
   remains a build concern; the raw ABI lane is the runtime boundary for this repository._

   All three step 4 prerequisites are resolved.

   | Prerequisite | Resolution |
   | --- | --- |
   | Unexpected loader errors | `LoadResult` carries the failing HRESULT. `ReadPersistPlacement` turns a failed load into a failing HRESULT, and `LoadForPersistPlacementIdImpl` returns `loaded.Error` for `LoadStatus::Unexpected` while still mapping missing, unavailable, and invalid records to a null result. |
   | Late persistence configuration | Late setters are accepted. First-display enrollment is fixed, and each save snapshots the current Boolean and id. An initially empty id or an explicit `false` cannot enroll later. Covered by the `WindowPlacementPersistenceTests` persist-id and opt-out cases. |
   | Real packaged storage | `ApplicationDataPlacementSettingsBackend` supplies the concrete Windows `ApplicationData` backend, with storage-backed coverage for load-without-container-creation and application isolation. |

5. **Initial-placement coordinator and cascading.** Wire `Show`, `Activate`, and
   `TryApplyInitialPlacement` to a coordinator that snapshots request/id/Boolean state before
   callbacks, validates reentrancy and options, selects explicit peer/saved/fallback sources, and
   applies the native policy. Track phase-open, in-progress, ever-displayed, first-display
   enrollment snapshot, and close-save completion separately. Keep hidden-application outcomes
   local to each attempt rather than using them as lifecycle state.
   `TryApplyInitialPlacement` remains repeatable while hidden; only first display ends the phase.
   A first `Show` with `SkipInitialPlacement` skips loading, peer selection, cascading, and fallback
   repositioning, still ends the phase, and rejects non-null placement or try-apply requests.
   Add lifecycle tests for repeated hidden application, skip and non-skip first display, later-show
   option ignoring, direct native display, reentrancy, invalid combinations, and both activation
   choices. This step depends on the public projections and native/capture adapters.
   _Implemented: `DesktopWindowImpl` owns a `WindowPlacementCoordinator` and implements
   `IWindowPlacementCoordinatorHost`. `Show`, `Activate`, and `TryApplyInitialPlacement` route
   through the coordinator, which tracks phase-open, in-progress, ever-displayed, and enrollment
   state separately._
6. **Saving and teardown integration.** Add accepted-close, destruction backstop, and confirmed
   session-end hooks in `DesktopWindowImpl`, snapshot after close handlers, perform bounded
   synchronous save attempts while state is valid, and keep cancellation/failure non-fatal.
   Enforce the first-display visibility requirement and current-Boolean/id snapshot for every save. Add
   storage-backed lifecycle tests for accepted/cancelled close, destruction, session-end,
   id changes, clear/reassign, duplicate-save suppression, and save failures. This step depends on
   the coordinator, capture adapter, and store retention implementation.
   _Implemented: `DesktopWindowImpl` calls `OnAcceptedClose` after close handlers, `OnDestroy` from
   both the managed destroy path and `WM_DESTROY`, and `OnSessionEnd` on a confirmed `WM_ENDSESSION`.
   Each save snapshots the current Boolean and id, requires prior display, and is non-fatal on
   failure. `WindowPlacementPersistenceTests` covers the opt-out, late opt-out, close-time id,
   initially empty id, per-id isolation, and replacement cases against real packaged storage._
7. **End-to-end and display validation.** Add a dedicated Window Placement integration test class
   and test app coverage for process-A/process-B round trips, package/application isolation,
   concurrent same-id saves, cascade precedence, saved minimization and snapping, and unchanged
   legacy behavior. Run the targeted isolated tests and the new window tests on the VM. Establish a
   controlled multi-monitor/variable-DPI lane before claiming those criteria; unsupported display
   capabilities must be reported as skipped rather than passing.
   _Implemented except for the display-environment gates (2026-09-25)._

   | Criterion | Coverage |
   | --- | --- |
   | Process-A/process-B round trip | `WindowPlacementCrossProcessTests`, exact bounds with no tolerance, plus per-id isolation across processes |
   | Package and application isolation | `WindowPlacementStorageIsolationTests::RecordWrittenUnderAnotherApplicationIdentityIsNotVisible` |
   | Concurrent same-id saves | `WindowPlacementStorageIsolationTests::ConcurrentSavesForOneIdLeaveOneCoherentRecord` |
   | Cascade precedence | `WindowPlacementExplicitCascadeTests` on one display, in both the managed and isolated classes |
   | Saved minimization | `WindowPlacementPersistenceTests::MinimizedPlacementSurvivesCloseAndRestore` with `ApplicationRestart`; `WindowPlacementApplicationTests::SavedMinimizationRequiresApplicationRestart` |
   | Saved snapping | `WindowPlacementApplicationTests::SavedSnapApplicationOnANewWindow` and `MinimizedFromSnappedRestoresToSavedSnapBounds`; see the [snapped first-show diagnosis](#native-application-status-2026-09-24) |
   | Unchanged legacy behavior | `LegacyShowDoesNotMoveOrResizeTheWindow` and `LegacyActivateDoesNotMoveOrResizeTheWindow` |
   | Live two-display peer enumeration | Open. Needs a VM with two interactive displays |
   | Multi-monitor/variable-DPI lane | Open, and narrowed to selection and relocation between two monitors. Environment not selected. See [what the lane still owns](#what-the-multi-monitorvariable-dpi-lane-still-owns) |

#### What the multi-monitor/variable-DPI lane still owns

A single display cannot reach these production paths. Earlier versions of this table claimed more
than that; the parts one display *can* reach are now covered and listed below. The lane exists for
the rest, and the environment it needs is in
[multi-monitor and DPI environment to establish](./Window-PlacementPersistence.md#multi-monitor-and-dpi-environment-to-establish).

| Production path | Why one display cannot cover it |
| --- | --- |
| `PlacementEx::FindClosestMonitor` choosing *among several connected displays* | Selection matches the saved device name first, then falls back to `MonitorFromRect`. With one monitor both branches return the same monitor, so which monitor wins is unobservable. A reordered display, or a saved position that lands between two displays, still needs two. |
| `PlacementEx::MoveToMonitor` *between two monitors* | Only two displays can show that the engine relocates a window onto a different monitor's work area and adopts that monitor's device name. |

The stale-display and DPI-change halves of those two rows are already covered on one display, because
neither depends on a second monitor:

| Covered on one display | Test |
| --- | --- |
| A saved display that is no longer connected takes the `MonitorFromRect` fallback and still produces a valid request for the connected monitor | `WindowPlacementApplicationTests::AStaleSavedDisplayNameFallsBackToTheConnectedMonitor` |
| A record saved at a different display scale is rescaled by `MoveToMonitor` | `WindowPlacementApplicationTests::APlacementSavedAtAHigherDpiRescalesOnTheConnectedMonitor` |

`MoveToMonitor` rescales from the placement's own saved work area and DPI to the target monitor's, not
from one monitor to another. So a record saved at 200% and restored at 100% on the same display hits
the rescale in production. `ComputePlacement` calls the move unconditionally, and the test measures a
440x340 saved rectangle coming back as 220x170 at half the saved DPI.

Track-size scaling used to be a third row. It converts the `WM_GETMINMAXINFO` limits from the
window's DPI to the target monitor's DPI, and on one display those two values are equal, so the
conversion is an identity. That arithmetic is WinUI-owned rather than a `PlacementEx` call, so it is
now the named function `WindowPlacementPersistence::TryScaleTrackSize`. `TryComputeNativeRequest`
calls it once per limit and fails the request if any call fails.
`WindowPlacementApplicationTests::TrackSizesScaleToTheTargetMonitorDpi` covers identity, scale up,
scale down, rounding, zero and negative limits, an unusable DPI, and `MulDiv` overflow on a single
display. Only the two live engine paths above still need two displays.

`WindowPlacementPolicyTests` does cover the WinUI-owned arithmetic against synthetic topologies: a
96 DPI primary and a 192 DPI secondary, in `RelocatesAndScalesForLowerDpiMonitor`,
`ScalesSizeUpForHigherDpiMonitor`, `WorkAreaChangeKeepsRelativePosition`,
`MatchesSavedDeviceNameEvenWhenGeometryChanged`, `MissingMonitorSelectsLargestIntersection`, and
`MissingMonitorFallsBackToNearestNotPrimary`. That coverage does not stand in for the rows above.
`TryComputeNativeRequest` builds `Topology topology{{target}}` from the one monitor the engine
already chose, so `TrySelectEngineMonitor`'s multi-monitor branches and the `LaunchMonitorHint`
branch of `ComputePlacement` run only in tests. Read those tests as proof of the arithmetic, not of
live monitor selection.

_Production save wiring:_ `DesktopWindowImpl` constructs the placement
coordinator after the desktop window is created and uses the packaged `ApplicationData` backend
through `PlacementStore` for accepted-close and teardown saves. UWP windows provide no-op
implementations for the desktop-only persistence surface. Native placement application is
implemented; see [native application status](#native-application-status-2026-09-24). The
process-A/process-B round trip, package/application isolation, concurrent same-id saves, and
cascade precedence on one display are covered. Saved maximization round trips through real
packaged storage. Saved minimization also round trips with exact restore bounds when first
`Show` uses `ApplicationRestart`. The earlier skip used `Default`, which intentionally restores
saved minimization to normal; it was not an OS refusal. Unchanged legacy behavior is covered by
`LegacyShowDoesNotMoveOrResizeTheWindow` and `LegacyActivateDoesNotMoveOrResizeTheWindow`: a window
that sets no persist id, no opt-in, and no show options keeps the exact position and size the
application gave it, so the automatic pass cannot move or cascade a window with no source. Applying
a saved `Snapped` record to a never-shown window is declined by the OS, not by WinUI; the measured
result is in the [snapped first-show diagnosis](#native-application-status-2026-09-24). Still open
for step 7: live two-display peer enumeration, and the multi-monitor/variable-DPI lane, whose exact
remaining scope is in [what the lane still owns](#what-the-multi-monitorvariable-dpi-lane-still-owns).

`WindowPlacementCrossProcessTests` runs two method-isolated hosts with
`runtests 'WindowPlacementCrossProcessTests.*' -PreservePackageRegistration`.
The runner registers the package once, refuses an existing registration, and removes its
registration and restores the manifest after the run. The class is excluded from ordinary runs.
Real save/load and per-id isolation work across distinct processes with the same AUMID, and the
full round trip restores the saved bounds exactly. The class asserts exact bounds with no
tolerance and passes 2/2 on `ge_current-260828-Desktop`. The earlier Y=71 to Y=76 drift was a
redundant same-monitor monitor action; see
[native application status](#native-application-status-2026-09-24).

The late-Boolean/enrollment and pre-display abort contracts are implemented. The detached loader
propagates unexpected packaged-storage HRESULTs while still mapping missing, unavailable, and
invalid records to a null result. Follow the reader and save rules below.
The unpackaged default `ApplicationData` dependency remains open. Keep the
packaged-only automatic-storage contract until Windows App SDK provides a supported default-store
capability. Do not expose a public store override or make a hidden application enroll saving.
The `Activate`-after-hidden-preparation hazard is decided: the automatic pass runs, there is no
implicit skip or `Activate` overload, and a never-displayed window never saves. Implement those
as specified rather than reopening them.

#### Codec review (2026-09-23)

Reviewed commit `a8c1c7b1f`: the record header and implementation, dedicated test class, and
project/solution integration. No correctness findings in this unit against the
[private record format](./Window-PlacementPersistence.md#private-record-format) and the API spec's
[data-validity rules](./Window-PlacementPersistence.md#data-validity).

The review covered little-endian field and GUID encoding, the 4 KiB envelope bound, checked
edge/dimension conversion, saved-work-area intersection, all six state mappings, optional defaults,
version handling, and duplicate-field validation before last-one-wins assignment. The tests include
independent golden bytes and malformed-input cases. Failed decoding does not publish partial output.
Tracked call sites are confined to the tests; this unit does not change window behavior.
This was a source review, not a new build or VM run, and does not extend the coverage reported above.

Related open work: [PR #11982](https://github.com/microsoft/microsoft-ui-xaml/pull/11982) removes
`MetadataResetter.cpp` from the same foundation project, outside the codec addition.
[PR #11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) changes `DesktopWindowImpl`
sizing and closed-window behavior, which matters for later native/lifecycle integration.
Neither inspected change duplicates the codec.

#### Store-adapter formatting review (2026-09-23)

Reviewed commit `61ad247ca`: `WindowPlacementStorageFormat.h/.cpp`, `WindowPlacementStorageFormatTests.cpp`,
and the project integration. No correctness defects found. Two hardening changes were made during
the review, and both build and pass on the VM.

| Area | Result |
| --- | --- |
| Base64 strictness | Correct and canonical. Rejects null/empty text, non-multiple-of-four length, out-of-alphabet characters, whitespace, embedded NUL, misplaced `=`, three or more `=`, and non-zero unused tail bits. One record has exactly one encoding. |
| Encoded-length bound | Checked before the decode buffer is allocated. 5,464 characters allow 4,098 decoded bytes, two over the 4 KiB ceiling; `DecodeRecord` rejects the excess, so the slack is not reachable. |
| Encode path | Bounded implicitly: `IsValid` caps the device name at 31 code units, so the largest record is 194 bytes, or 260 characters. Nothing can be written that cannot later be read. |
| Base32 digest | Matches uppercase RFC 4648 without padding. 256 bits produce 52 characters, the accumulator cannot overflow, and the final character carries one real bit plus four zero bits, so it is always `A` or `Q`. The golden digests in the tests match that prediction, which is independent evidence they were not generated by this code. |
| Slug and lengths | Matches the [proposed names](./Window-PlacementPersistence.md#proposed-packaged-container-and-value-names): ASCII `[A-Za-z0-9]` only, at most 16 characters, `id` fallback, 57-character container names, 73-character value names. |
| Case-insensitive backends | Safe. Two distinct ids can only produce names that match case-insensitively if their digests collide, and the container name is pure uppercase Base32. |
| `BCryptHash` with `BCRYPT_SHA256_ALG_HANDLE` | The right choice for the framework DLL. It is a one-shot call with no provider handle to open, cache, or close, and no CNG cleanup on unload. The pseudo-handle needs Windows 10 1703, below the supported floor. `OneCoreUAP.Lib` already exports `BCryptHash`, so the framework DLL link needs no new library; `bcrypt.lib` in the isolated test project is redundant but harmless. |

Changes made: `MaximumEncodedCharacters` is now derived from `MaximumRecordBytes` instead of a
hand-copied 5,464, so the two cannot drift; `BCRYPT_SUCCESS` replaces a raw `status >= 0`. Tests
were added for the derived bound and for `EncodeRecordText` leaving its output untouched on failure.

Validation on 2026-09-23: `.\initrun.ps1 .\build.cmd /q` succeeded, and
`*WindowPlacementRecordTextTests*` (8) and `*WindowPlacementStorageNameTests*` (9) passed on the VM
with none failed, blocked, or skipped. These remain data-only tests: no storage I/O, no window, and
no display configuration.

#### Capture and DesktopWindowImpl integration review (2026-09-23)

Reviewed commit `79c2f629a`: `WindowPlacementCapture.h/.cpp`, the `DesktopWindowImpl` cache,
`WindowPlacementCaptureTests.cpp`, and the project integration. **Step 3 is not accepted.** The unit
does not implement
[Maintain a coherent overlapped snapshot](./Window-PlacementPersistence.md#maintain-a-coherent-overlapped-snapshot) or
[Cache optional virtual-desktop identity safely](./Window-PlacementPersistence.md#cache-optional-virtual-desktop-identity-safely),
and it re-implements work the step 2 adapter already does. Two hazards were fixed during the review;
everything else is returned to the Developer.

The core problem is that `TryCaptureFromWindow` re-derives placement from `GetWindowRect`,
`GetWindowPlacement`, `MonitorFromPoint`, `IsIconic`, and `IsZoomed` instead of calling
`PlacementEx::GetPlacement` through the adapter. `TryApplyNativeRequest` already calls
`GetPlacement(hwnd, &captured, CaptureFlags::SkipVirtualDesktopId)` followed by
`TryReadEnginePlacement`, so there are now two capture implementations that disagree. The engine
already reports `Arranged`, `RestoreToArranged`, `RestoreToMaximized`, the arrange rectangle, the
device name, and the DPI. The hand-written state machine tries to rebuild those signals and gets
them wrong.

| Area | Finding |
| --- | --- |
| Engine reuse | Blocking. Duplicate capture path. This contradicts the adopted "prioritize PlacementEx reuse" decision and the step 3 plan text, which says to integrate `PlacementEx::GetPlacement`. |
| Engine configuration | Blocking, fixed during review. `WindowPlacementCapture.h` included `User32Utils.h` **without** `USE_VIRTUAL_DESKTOP_APIS`, while `WindowPlacementAdapter.h` includes it **with** that define. `PlacementEx` declares `GUID virtualDesktopId` only under that define, so the same class had two layouts and two sets of inline functions in one binary. That is an ODR violation across `WindowPlacementAdapter.cpp`, `WindowPlacementCapture.cpp`, and `DesktopWindowImpl.cpp` in both the foundation DLL and the isolated test binary. |
| Coordinate space | Blocking. `NormalRect` is taken from `WINDOWPLACEMENT::rcNormalPosition`, which is in Win32 **workspace** coordinates for a top-level non-tool window. `WorkArea` and the fallback rectangle are in screen coordinates. The spec requires physical pixels in the virtual screen coordinate system and names workspace coordinates as the wrong space. Mixing the two also makes the `IsValid` normal/work-area intersection test unreliable. |
| DPI virtualization | Blocking. Nothing establishes a physical-pixel coordinate context. On a thread that is not per-monitor DPI aware, `GetWindowRect` and `GetWindowPlacement` return virtualized geometry, which is the failure the design note warns about. |
| Minimized capture | Blocking. The monitor is chosen from the center of `GetWindowRect`. A minimized window reports roughly `(-32000, -32000)`, so `MonitorFromPoint` picks a monitor unrelated to the window, `WorkArea` comes from that monitor, and the normal/work-area intersection check then fails. Minimized capture cannot produce a valid snapshot. Use the normal rectangle or `MonitorFromWindow`. |
| Restore history | Blocking. A previous state of `MinimizedFromMaximized` or `MinimizedFromSnapped` is not matched by the minimized branch, so the next capture downgrades it to plain `Minimized`. Minimizing sends both `WM_MOVE` and `WM_SIZE`, so the history is destroyed inside a single minimize. This breaks the spec's saved-state restore table. |
| Snapped detection | Blocking. `State::Snapped` is only reachable when the previous snapshot is already `Snapped`, and nothing ever sets it first. `Snapped` and `MinimizedFromSnapped` are therefore unreachable from capture. The commit message's "simple heuristics" is a 50-pixel, DPI-unaware comparison that only re-confirms a state that can never be entered. |
| Hidden state | Blocking. There is no last-meaningful-show-state tracking. State is derived only from live window styles plus the previous snapshot. `IsWindowVisible` was read into an unused local and has been removed. |
| Virtual desktop id | Blocking. Every capture calls `VirtualDesktopId.reset()`, so any safely cached id is discarded on the next `WM_MOVE`. The design requires overlaying the last cached non-empty id. |
| Presenter transitions | Missing. No complete pre-full-screen overlapped snapshot is retained, and capture does not detect full-screen or compact overlay. |
| `noexcept` and allocation | Blocking. `TryCaptureFromWindow` is `noexcept` but builds a `std::wstring` and appends to a `std::u16string`. An allocation failure calls `std::terminate`. `monitorInfo.szDevice` is already a bounded null-terminated array, so no intermediate string is needed. |
| `IsBadReadPtr` | Fixed during review. `CaptureEffectivePlacement` called `::IsBadReadPtr(this, sizeof(*this))`. That API is banned, is not thread-safe, can trip guard pages, and is meaningless on `this`: if `this` were invalid the call itself would already be undefined. |
| Dead code | The snap branch tests `SnapRect.has_value()` twice, and its `placement.SnapRect = previousSnapshot->SnapRect` assignment is immediately overwritten below. |
| Failure reporting | Capture failure leaves the cache silently stale. Leaving stored placement unchanged is correct per the spec, but there is no diagnostic, so a permanently failing capture looks identical to a window that never moved. |

Test coverage does not support the commit message. Four of the six tests never call
`TryCaptureFromWindow`. `TracksStateTransitionsMinimizedFromSnapped`,
`TracksStateTransitionsMinimizedFromMaximized`, and `CapturesDeviceNameConversion` assign a field to
a local `Snapshot` and then assert that the field holds what was just assigned;
`ValidatesSnapshotDimensions` re-tests `IsValid`, which `WindowPlacementRecordTests` already covers.
Only the two negative HWND cases exercise the new code, and neither reaches capture logic. The
claimed state-transition and device-name behavior is untested. Nothing covers the
`DesktopWindowImpl` cache, hidden moves, presenter transitions, or HWND lifetime correlation, all of
which the step 3 plan lists.

Changes made during the review: `WindowPlacementCapture.h` no longer includes the PlacementEx
headers and takes only `<windows.h>`; `WindowPlacementCapture.cpp` includes
`WindowPlacementAdapter.h` so the engine has one configuration; the `IsBadReadPtr` call and the
unused visibility local are removed. These fix the ODR and banned-API hazards only. They do not make
step 3 correct, and capture should be rewritten on `PlacementEx::GetPlacement` with real tests before
step 5 or step 6 consumes the cache.

#### Capture rework on the placement engine (2026-09-24)

Capture was rewritten on the engine. `WindowPlacementCapture` is now a per-window cache whose only
job is the state the engine cannot report from an HWND. Every blocking finding above is resolved.

`TryCapture(HWND, PresenterKind)` does this:

1. Reject a null or destroyed window.
2. Reset tracked state if the bound HWND changed, and take a new lifetime token.
3. Return false for a non-overlapped presenter, which keeps the last valid overlapped snapshot.
4. Enter a per-monitor-DPI-aware scope, so the engine reports physical pixels.
5. Call `PlacementEx::GetPlacement(hwnd, &captured, CaptureFlags::SkipVirtualDesktopId)`.
6. Reject a full-screen window, then normalize the engine value into the request vocabulary.
7. Translate with `TryReadEnginePlacement`, reconcile with the cached snapshot, and overlay the
   cached virtual desktop id.

| Owner | Responsibility |
| --- | --- |
| Engine | Normal rectangle in screen coordinates, work area, device name, DPI, `Arranged` plus the arrange rectangle, `RestoreToMaximized`, and the monitor for a minimized window. |
| WinUI | Last meaningful show state, snapped restore history, last valid overlapped snapshot, cached virtual desktop id. |

Two translation details are required, because the engine's capture vocabulary is wider than the
request vocabulary the adapter validates:

| Detail | Handling |
| --- | --- |
| Show-command aliases | `GetWindowPlacement` reports `SW_SHOWMINIMIZED`, `SW_SHOWMAXIMIZED`, and other aliases. `TryReadEnginePlacement` only accepts `SW_NORMAL`, `SW_MAXIMIZE`, `SW_MINIMIZE`, and `SW_HIDE`, so capture canonicalizes first. Without this, minimized capture fails outright. |
| Flags a request cannot carry | `SW_HIDE` only validates as `NoChange` with `KeepHidden` and `NoActivate`, and neither maximize nor hide may carry `Arranged` or a restore flag. Capture drops those combinations and recovers the state from the cached snapshot instead. |

`TryReconcile` is a static function that takes the engine value, window visibility, and the previous
snapshot. It is separate from the window so the state rules, including the snapped states that
cannot be staged reliably in a test, are testable without one. The rules:

| Engine value | Result |
| --- | --- |
| `Maximize` | `Maximized`. |
| `Minimize` with `RestoreToMaximized` | `MinimizedFromMaximized`. The engine flag wins over tracked history. |
| `Minimize` with `RestoreToArranged` | `MinimizedFromSnapped` with the engine arrange rectangle. |
| `Minimize` after a tracked snapped state | `MinimizedFromSnapped` with the tracked snap rectangle. `GetPlacement` cannot reconstruct this from an already minimized window, and minimizing sends both `WM_MOVE` and `WM_SIZE`, so the history has to survive repeated captures. |
| `Minimize` otherwise | `Minimized`. |
| `NoChange` (`SW_HIDE`) | The last meaningful state, with new geometry. A hidden window can still move. |
| `Normal` with `Arranged` | `Snapped` with the engine arrange rectangle. |
| `Normal`, not visible, tracked `Snapped` | `Snapped`. `IsWindowArranged` goes false as soon as the window is hidden, so hiding alone must not replace meaningful state. |
| `Normal` otherwise | `Normal`. |

A snapped state that ends up without a snap rectangle degrades to `Normal` or `Minimized` rather
than failing capture. The result is gated on `IsValid` and the output is replaced only on success,
so a failed capture always leaves the cache on its last good snapshot.

Virtual desktop identity stays out of capture. `TryCapture` passes `SkipVirtualDesktopId`, because
the query is a cross-apartment COM call that can fail inside a window message handler. A caller
that obtains an id safely calls `TrySetVirtualDesktopId(lifetimeToken, id)` with the token it
started from. A stale token or an empty id is rejected, so a late result cannot attach to a closed
or recycled HWND. `Detach()` invalidates outstanding tokens at close while leaving the snapshot
readable for a save during teardown.

`DesktopWindowImpl` now owns a `WindowPlacementCapture` instead of a bare `std::optional<Snapshot>`.
It derives the presenter kind from `AreNewWindowingApisEnabled` and `AppWindowPresenterSupportsSizing`,
and calls `Detach()` from `OnClosed` before `CloseImpl`. `CaptureEffectivePlacement` is no longer
`noexcept`, which is the fix for the allocation-in-`noexcept` finding: this library has no `catch`
anywhere, so adding a handler would be dead code.

`WindowPlacementCaptureTests.cpp` was replaced. Thirteen tests cover the reconciliation rules
against a synthetic engine value and capture against a real top-level window: shown capture,
hidden moves, the maximize/minimize/restore chain including the double restore through maximized,
presenter transitions, capture after window destruction, and virtual desktop id lifetime
correlation across two windows. All 91 Window Placement tests pass on the VM.

Still open as of this review (2026-09-24), and deliberately not addressed here. The status column
records what later work did with each row:

| # | Item | Note | Status |
| --- | --- | --- | --- |
| 1 | Failure reporting | Capture failure was silent. A permanently failing capture looked like a window that never moves. This needed a diagnostic, not a behavior change. | Closed. `PlacementFailureScope` records `PlacementFailureCategory::Capture` in `WindowPlacementPublic.cpp` and on both capture paths in `WindowPlacementApplication.cpp`. `WindowPlacementDiagnosticsTests` verifies the reported `Save`/`Capture` pair. |
| 2 | Full-screen snapshots | Capture rejects a full-screen window and keeps the last overlapped snapshot, which is the required behavior. Entering and exiting full screen through `PlacementEx` is step 5 work. | Capture side closed (2026-09-26). `WindowPlacementCaptureTests::FullScreenWindowKeepsThePreFullScreenSnapshot` enters full screen with `PlacementEx::EnterFullScreen`, asserts `GetPlacement` really reports `FullScreen`, then verifies `TryCapture` fails, the complete pre-full-screen snapshot survives, no lifetime token is consumed, and capture resumes after `ExitFullScreen`. Still open: the apply path does not handle `FullScreen`. That is unreachable from persistence today, because capture never lets a `FullScreen` record be saved, so it only matters if a future caller supplies one. |

#### Code review of the capture rework (2026-09-24)

The rework is a large improvement. Capture now runs on `PlacementEx::GetPlacement`, the coordinate
space and the minimized monitor are right, the reconciliation rules are stated and tested, the
lifetime token keeps a late virtual-desktop result off a recycled HWND, and the tests exercise real
behavior instead of their own setup. Every blocking finding from the previous review is resolved.

Three new findings, plus three minor ones.

| # | Severity | Area | Finding |
| --- | --- | --- | --- |
| 1 | Blocking | Detach on a cancelable close | `Detach()` runs from `OnClosed`, which is the **`WM_CLOSE` handler**, before the close is known to be accepted. `CloseImpl` raises `Window.Closed` and returns without closing when a handler sets `Handled` (`DesktopWindowImpl.cpp`). `WM_CLOSE` never reaches `DefWindowProc`, so on a cancelled close the window stays alive and detached. The next `WM_MOVE` or `WM_SIZE` calls `TryCapture` with the same HWND, sees `hwnd != m_window` because `Detach()` set `m_window` to null, and **resets `m_placement` and `m_virtualDesktopId`**. |
| 2 | Should fix | Exceptions out of a window procedure | Dropping `noexcept` from `CaptureEffectivePlacement` does not fix the allocation hazard; it relocates it somewhere worse. The function is reached from `OnMoved` and `OnSizeChanged`, which the window procedure dispatches directly. Letting `std::bad_alloc` unwind out of a Win32 callback frame is undefined behavior. `noexcept` at least gave a deterministic fail-fast at a known frame. "This library has no `catch` anywhere, so a handler would be dead code" is an argument for where the handler goes, not for letting an exception escape into user32. |
| 3 | Should fix | Capture frequency | Capture runs on every `WM_MOVE` and every `WM_SIZE` with no coalescing. Each pass does a COM QI (`AppWindowPresenterSupportsSizing` -> `TryGetOverlappedPresenter`), two thread-DPI-context switches, `GetWindowPlacement`, `MonitorData::FromWindow` (`GetMonitorInfo` plus `GetDpiForMonitor`), `GetWindowLongPtr`, `GetWindowRect`, and `DwmGetWindowAttribute` when the window is arranged. A drag or a resize produces hundreds of those per second. The same file already coalesces exactly this burst for the restored-size feature in `ScheduleUpdateLastRestoredClientSize`. |
| 4 | Minor | Show-command aliases | `TryCanonicalizeShowCommand` accepts `SW_SHOW` and `SW_SHOWNOACTIVATE` as normal aliases but not `SW_SHOWNA`. If it ever appears, capture fails permanently and silently for as long as the window stays in that state. |
| 5 | Minor | DPI scope failure | When both `SetThreadDpiAwarenessContext` calls fail, `PhysicalCoordinateScope` has no effect and capture stores virtualized coordinates as if they were physical. The scope exists for coordinate correctness, so capture should fail instead of recording a wrong value. |
| 6 | Minor | Conflicting restore flags | `IsValidRequest` rejects `RestoreToMaximized` together with `RestoreToArranged`, so if the engine ever reported both, capture would fail wholesale. `TryCanonicalizeShowCommand` already drops unrepresentable flag combinations; it should drop `RestoreToArranged` when `RestoreToMaximized` is set. `GetPlacement` never sets `RestoreToArranged` today, so this is defensive only. |

Finding 1 in detail. The consequences of the wipe are not limited to the cached desktop id:

| Lost on a cancelled close | Effect |
| --- | --- |
| Last valid overlapped snapshot | On a non-overlapped presenter, `TryCapture` rebinds and wipes **before** the presenter check, then returns false. The snapshot is gone and cannot be rebuilt while the presenter stays full-screen or compact overlay, so a later save has no placement at all. This is the exact invariant the cache exists to hold. |
| Snapped and hidden history | The next capture runs with `previous == nullptr`. A previously snapped window that is currently hidden reconciles to `Normal`, so a cancelled close silently changes what gets saved. |
| Cached virtual desktop id | Discarded, and `TrySetVirtualDesktopId` also fails for every outstanding token until the rebind. Best-effort state, so this alone is minor. |

Two changes are needed, and each wants a test:

1. Detach when the close is actually accepted, not on `WM_CLOSE`. The end of `CloseImpl` after
   `m_bIsClosed = true`, or `Shutdown()`, is the right place. `Shutdown()` is the safer of the two
   because it also covers a teardown that does not come through `WM_CLOSE`.
2. Make `TryCapture` refuse to capture from the HWND it was detached from, instead of treating it as
   a new window and wiping the cache. A detached-flag check is enough. The current rebind-on-change
   rule is right for a genuinely different HWND and should stay.

Step 6 will hit this directly: "cancelled close" is already one of its listed acceptance cases.

Test gaps behind these findings, as of this review (2026-09-24). Both were closed by the fixes in
the next note:

| Gap | Note | Status |
| --- | --- | --- |
| Detach then recapture the same HWND | No test covered it. `CorrelatesVirtualDesktopIdWithWindowLifetime` detaches and then captures a **different** window, where wiping is the wanted behavior, so it passed while finding 1 was live. | Closed by `RejectsRecaptureOfDetachedWindow` in `WindowPlacementCaptureTests.cpp`. |
| Presenter-kind derivation | Nothing covered the `AreNewWindowingApisEnabled` / `AppWindowPresenterSupportsSizing` mapping in `DesktopWindowImpl`. The mapping reads correctly: `AppWindowPresenterSupportsSizing` is an `IOverlappedPresenter3` QI, which fails for FullScreen and CompactOverlay. | Closed for the mapping itself. It now lives in `ResolvePresenterKind`, and `ResolvesPresenterKindWithoutQueryingWhenFeatureIsOff` and `ResolvesPresenterKindFromSizingSupportWhenFeatureIsOn` cover both inputs and the zero-query path. Still untested: that `DesktopWindowImpl` passes those two specific inputs at its two call sites, which needs a real window with a non-overlapped presenter. |

Accepted as correct during this review, for the record: the `RestoreToArranged` branch in
`TryReconcile` is unreachable from a real `GetPlacement`, which only sets `RestoreToMaximized`,
`Arranged`, `AllowSizing`, `FullScreen`, and `VirtualDesktopId`. The code and the design note both
say so, and the branch is covered by a synthetic value, so it is defensive, not dead. The union of
`NonRequestFlags` and the adapter's mapped flags covers every declared `PlacementFlags` value, so no
engine flag can silently reach `TryReadEnginePlacement` and fail it.

#### Capture review fixes (2026-09-24)

The three findings and three minor items above are addressed:

| Area | Implementation |
| --- | --- |
| Close and retirement | `Shutdown()` detaches the cache. A cancelled `WM_CLOSE` no longer changes capture lifetime. Detach retains the bound HWND and rejects recapture of it, including after repeated detach calls. A successful capture of a different HWND starts a new binding and invalidates the old token. |
| Failed capture | Translation and reconciliation finish before publishing the snapshot, binding, or token. A failed capture of a different HWND no longer discards the existing snapshot or desktop id. |
| Exception boundary | `TryCapture`, `CaptureEffectivePlacement`, and scheduling are `noexcept`. This takes the review's fail-fast alternative: allocation failure terminates rather than unwinding through a native callback. It does not promise recovery from allocation failure or enable exceptions in the product library. Ordinary capture failures still return false without changing the cache. |
| Coalescing | `ScheduleWindowStateUpdate` shares the former restored-size callback and lifetime sentinel across `WM_MOVE` and `WM_SIZE`. One pending callback captures placement and, when requested, updates restored client size. Drag completion still captures synchronously. Missing or rejected dispatch falls back to synchronous capture; enqueue rejection is logged. |
| Callback lifetime | `Shutdown()` disarms the shared sentinel regardless of the new-windowing feature switch. Capture also runs with that switch off, so its callbacks need the same teardown protection. |
| Normalization | Normal aliases include `SW_SHOWNA`. Failure to enter either physical-coordinate DPI scope rejects capture. A maximized restore flag takes precedence over an arranged restore flag. |

`ResolvePresenterKind` retains the existing mapping to `AppWindowPresenterSupportsSizing` and
short-circuits the presenter query when the new APIs are disabled. Unit tests cover both enabled
results and the disabled zero-query path. The dedicated capture class also covers same-HWND
retirement, failed replacement without cache/token changes, normal show aliases, and conflicting
restore-state reconciliation.

The runtime and isolated placement test project build in `amd64chk`. All 97 placement tests and
seven existing presenter/deferred-sizing integration tests pass on the local VM. The integration
selection includes the simulated drag-end case and real FullScreen/CompactOverlay transitions.
Those tests protect the shared sizing behavior; they do not expose the private placement cache.
Native alias tests can encounter commands already normalized by Windows. DPI-context failure and
allocation failure were not fault-injected.

Public capture and saving must capture synchronously when they need current geometry, not assume
that a queued callback has run. Storage-backed accepted/cancelled-close coverage remains part of
step 6. The full-screen hidden-application gate and public projections remain separate work.

Related open work: [PR #11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) changes
closed-window sizing in the same implementation files. Keep its accepted-close size snapshot and
closed-state guard when integrating these changes; this capture fix does not replace that work.

#### Code review of the coordinator production wiring (2026-09-24)

Reviewed commits `4c2aa5c53`, `3d07f3a5c`, and `d362c7900`: the coordinator lifecycle,
`DesktopWindowImpl`'s `IWindowPlacementCoordinatorHost` implementation, and the packaged save path.
Three defects were found and fixed during the review. The remaining gaps are pre-existing step 5
and step 6 work, listed below and sent back to the Developer.

| Finding | Detail | Fix |
| --- | --- | --- |
| `Display` never reported success | `ShowWindow` returns the window's *previous* visibility, so the first display always returned `FALSE`. `EndFirstDisplayOperation` treated that as a pre-display abort, so the phase never closed, the window never enrolled, no save could ever happen, and `Show`/`Activate` returned false to the app. | `Display` now calls `ShowWindow` and reports the resulting state through `IsVisible()`. It also null-checks the HWND, which `IsVisible` already did. |
| Saves used the enrolled id | The host wrote to `m_enrollmentId`, the id captured at first display. The [save sequence](./Window-PlacementPersistence.md#save-sequence) requires a fresh id and Boolean snapshot per attempt, and an enrolled window is allowed to save to a different id. | The host snapshots `m_persistPlacementId` and `m_useAutomaticPlacementPersistence` itself, and skips the attempt when the id is empty. `IWindowPlacementCoordinatorHost::TryCapturePlacementAndSave` now takes no arguments, so the enrolled id cannot be reintroduced as a write destination. The coordinator still gates on enrollment. |
| A failed save blocked the backstop | `m_saveAttempted` was set before the write, so a failed accepted-close save suppressed the destruction backstop. The [lifecycle hooks](./Window-PlacementPersistence.md#lifecycle-hooks) require marking a save complete only after a successful write. | Renamed to `m_saveCompleted` and set only on success. A completed save is still never repeated. New test `FailedCloseSaveIsRetriedByTheDestroyBackstop`. |

Two smaller changes: the save now calls `CaptureEffectivePlacement()` before reading the cache, so
it does not depend on a queued state-update callback having run, matching the synchronous-capture
rule above. An added `try`/`catch` was removed after it failed to compile: `DesktopWindowImpl.cpp`
is built without exceptions, and this file follows the same fail-fast-on-allocation-failure
convention as the rest of the capture work.

Still open as of this review, and not introduced by these commits. Each row is closed by later
work; see the note that follows this one.

| # | Gap | Owner |
| --- | --- | --- |
| 1 | `ApplyPlacement` is a stub returning false, so no placement is ever applied natively. | Step 5 / 2C |
| 2 | `ShowImpl(options)` and `TryApplyInitialPlacementImpl` ignore `WindowShowOptions`, so `SkipInitialPlacement`, `DoNotActivate`, `Reason`, and `CascadeBehavior` are silently dropped. | Step 5 |
| 3 | `Show`, `Activate`, and `TryApplyInitialPlacement` do not route through the coordinator's `Show`/`Activate` entry points, so the first-display pass never runs from ordinary app code. | Step 5 |
| 4 | `NotifyNativeDisplayed` has no production call site, so a native or `AppWindow` display does not end the phase. | Step 5 |
| 5 | No `WM_DESTROY` or `WM_ENDSESSION` hook. Only `CloseImpl`, `Shutdown`, and the destructor save. | Step 6 |

Validation: `wrtdxamlfoundation.vcxproj` and the isolated placement project build in `amd64chk`.
All 130 `*WindowPlacement*` tests pass on the VM with none failed, blocked, or skipped. No window
round-trip or storage-backed lifecycle coverage exists yet; that remains step 7.

#### Display routing through the coordinator (2026-09-25)

Closes gaps 2, 3, and 4 above: options are read, the three entry points route through the
coordinator, and a native display ends the phase. Gap 1 is also closed, by the separate native
application work. `DesktopWindowImpl::ApplyPlacement` is no longer a stub. It applies pending
client sizing, supplies a deferred peer anchor, and calls
`WindowPlacementPersistence::TryApplyPlacement`, which reaches `PlacementEx::SetPlacement`. See
[native application status](#native-application-status-2026-09-24). Gap 5 is closed by the
step 6 `WM_DESTROY` and `WM_ENDSESSION` hooks.

| Change | Detail |
| --- | --- |
| `Display` learns the operation | `IWindowPlacementCoordinatorHost::Display` now takes the existing `CoordinatorOperation` plus `doNotActivate`. Show and Activate have different native behavior, and the host had no other way to tell them apart. `TryApplyInitialPlacement` never displays, so it never reaches this callback. |
| `Show`/`Activate`/`TryApplyInitialPlacement` route through the coordinator | `ShowImpl()` forwards to `ShowImpl(nullptr)`. All three entry points now call the matching coordinator method and then surface any native failure through a new `m_lastDisplayResult` member, because `Display` returns `bool`, not `HRESULT`. |
| Options are read, with phase-dependent validation | `CopyShowOptions` QIs the sealed `WindowShowOptions` and reuses its existing `CopyInitialRequest`, which validates. After the phase closes, only `DoNotActivate` is read and the ignored fields are not validated. |
| `TryApplyInitialPlacement` checks state before validating | The spec requires the displayed / in-progress check to come first, so a late call returns `false` instead of throwing `E_INVALIDARG`. |
| `WM_SHOWWINDOW` ends the phase | A native or `AppWindow` display now calls `NotifyNativeDisplayed`. The hook filters on `lParam == 0`, which means an explicit `ShowWindow` call rather than an owner-driven change. The coordinator's in-progress guard makes the message sent by its own `Display` a no-op. |
| A later `Activate` is not a no-op on a visible window | The post-phase `Activate` path used to early-return when the window was already visible. Per [Control display and activation](../../specs/window-placement-persistence-spec.md), a later `Activate()` still restores a minimized window and requests activation, so it always runs the display step. `Show` keeps its visible-window no-op. |
| The legacy `ShowImpl` error path is gone | It treated `ShowWindow`'s previous-visibility return as a failure. Routing through the coordinator makes that path unreachable; `Display` now reports success through `IsVisible()`. |

Validation: both projects build in `amd64chk`. The new `WindowPlacementDisplayRoutingTests` class
adds 10 tests covering first Show, first Activate, `SkipInitialPlacement`, `DoNotActivate`, later
Show on hidden and visible windows, later Activate on a visible window, the hidden-application
path, a nested `NotifyNativeDisplayed`, a native display before any framework call, and retry after
an aborted display. All 140 `*WindowPlacement*` tests pass on the VM.


### Architect handoff (2026-09-23)

**Historical ownership note.** The coordinator architecture now lives in the
[feature implementation design](./Window-PlacementPersistence.md#coordinator-ownership-and-operation-boundaries).

**Architect handoff (2026-09-23):** The current codec, store, and placement-policy units do not own
phase or enrollment state. No existing helper needs a consumed-opportunity flag migrated. Step 5
should add one private coordinator owned by `DesktopWindowImpl`, rather than independent lifecycle
flags in the public projection, store, and native adapter.

### Step 2A: engine-value seam

**Historical adapter checkpoint and subsequent correction.**

`WindowPlacementAdapter.h/.cpp` now includes the engine through `User32Utils.h` in the foundation
and isolated-test builds. `TryCreateEnginePlacement` validates rectangles, device names, state
flags, and optional metadata before publishing a working value. `TryReadEnginePlacement` reads
that value back after engine edits, checks representable dimensions, and rejects unsupported
flags or show commands. Both leave the output unchanged on failure. The existing storage format
and backend-neutral policy types are unchanged.

The adapter enables the virtual-desktop field and disables the unused string/registry serialization
in both builds. It does not call the shell or replay startup settings. The missing imported
Window Action compatibility header is replaced by local SDK-compatible type declarations;
WinUI's SDK and minimum Windows target are unchanged. The native API remains dynamically resolved.

The dedicated `WindowPlacementAdapterTests` class covers all six policy states, call-time flags,
optional identity, malformed input/output, checked rectangle conversion, and Window Action layout.
Synthetic monitor data feeds the real `PlacementEx::MoveToMonitor`; separate tests read back
`Cascade` and `KeepRectOnMonitor` results. No second adjustment implementation was added.

Validation on 2026-09-23: the foundation and isolated Window Placement test projects build for
`amd64chk`. The VM run of `*WindowPlacement*Tests*` passes all 77 tests, including the nine adapter
tests, with none failed, blocked, or skipped. These do not establish native application, physical
capture, real multi-monitor behavior, virtual-desktop operations, or downlevel capability.

Step 2B still must replace the existing policy's duplicate geometry and monitor-selection code.
`PlacementEx::Cascade()` currently reads system metrics itself; any injected caption/frame offset
must enter the engine rather than become another WinUI cascade implementation. Engine adjustment
uses native-width arithmetic, so the integration must also guard intermediate representability,
not merely validate the input and final rectangles.

##### Status correction (2026-09-24)

The paragraph above is stale. Step 2B has landed. `WindowPlacementPolicy.cpp` no longer carries its
own geometry or monitor-selection code: `ComputePlacement` delegates through
`TrySelectEngineMonitor`, `TryCreateEnginePlacement`, `TryMoveEngineToMonitor`, `TryCascadeEngine`,
and `TryReadEnginePlacement`. Treat the engine as the single geometry implementation.

Explicit-placement cascading is now wired end to end. The policy always implemented the
[explicit-cascade rule](./Window-PlacementPersistence.md#cascade-explicitly-supplied-placement),
but production passed no peer anchor, so the branch was unreachable. Three changes fix that.

| Layer | Change |
| --- | --- |
| `WindowPlacementApplication.h` | New `PeerAnchorSource` seam: a captureless-lambda-compatible function pointer plus a `void*` context. Added as a trailing defaulted parameter on `TryApplyPlacement`; replaces the `std::optional<Rect>` parameter on `TryComputeNativeRequest`. |
| `WindowPlacementApplication.cpp` | Resolves the anchor *after* the target monitor is known, and only when cascading is permitted and the source is `PlacementSource::Explicit`. Validates the returned rectangle before feeding `policy.Cascade.PeerAnchorNormalRect`. |
| `DesktopWindowImpl.cpp` | Peer search split into `TryFindPlacementPeer(placementId, requiredDeviceName, snapshot)`. A monitor mismatch now continues the Z-order walk instead of aborting it, so a lower peer on the target monitor is still found. `ApplyPlacement` binds a self-referencing `PeerAnchorSource`. |

The ordering matters: the spec requires the target monitor first, then the peer search restricted to
that monitor. The coordinator has neither an `HWND` nor a monitor, so it cannot do the lookup.
`DesktopWindowImpl` is both the placement host and the caller of `TryApplyPlacement`, so it supplies
the callback directly. `IWindowPlacementCoordinatorHost` is unchanged.

`WindowPlacementExplicitCascadeTests` covers the seam: the anchor is requested only for explicit
placement under `CascadeBehavior::Enabled` and a non-restart reason; the requested device name
matches the resolved target monitor; the cascaded result keeps the explicit size, state, and monitor
and moves only its position; and a missing or invalid anchor leaves the explicit result untouched.

Validation on 2026-09-24: `Microsoft.UI.Xaml` and the isolated Window Placement test project build
for `amd64chk`. The full isolated suite passes 169 of 169 with none failed, blocked, or skipped.
This run was single-display. The spec's multi-monitor requirement - a peer on a different monitor
must be skipped while a lower-Z peer on the target monitor is used - is **not covered** and needs a
multi-monitor lane.

Update: that requirement is now covered on a single display. The walk itself moved into
`WindowPlacementPersistence::TryFindAcceptablePlacementPeer`, which pulls candidates from a
`PlacementPeerCandidateSource` (`bool TryGetNext(void*, Snapshot&)`) and applies
`IsAcceptablePlacementPeer` to each one. `DesktopWindowImpl` supplies the production source, which
walks Z order and yields up to two candidates per marked window: the framework placement cache,
then a direct `WindowPlacementCapture` for windows this process does not own. Tests script the
candidate order directly, so `PeerWalkSkipsOffMonitorCandidatesAndKeepsWalking` proves an
off-monitor candidate at the top of Z order is skipped and a lower matching candidate is returned.
Only live Z-order enumeration itself still needs real multi-monitor hardware.

At the initial step 2A checkpoint, step 2C still had to read live topology, prove native
hidden/non-activating capability, and capture
effective physical geometry after application. `TryReadEnginePlacement` is not that capture:
neither backend's `SetPlacement` in/out value is a reliable post-apply snapshot. The existing
policy's hidden-state checks and the seam's flag translation do not prove backend safety.

### Native application status (2026-09-24)

**Historical rolling record.** Explicit 2026-09-25 diagnoses and undated presenter updates are
retained below. The source's attribution of a snapped first-show refusal to the OS is a measured
diagnosis on the named environment, not proof of all OS/backend behavior.

`DesktopWindowImpl::ApplyPlacement` now calls the separate `WindowPlacementApplication` unit.
It rejects closed/non-overlapped windows, applies pending client sizing before the placement pass,
and refreshes the restored-size cache afterward. The application unit takes the copied explicit
snapshot first; otherwise, an opted-in nonempty id uses the existing packaged reader once.
A selected source is never replaced after computation or application failure. With no source,
only a valid Launch monitor hint can migrate current fallback geometry; that does not report
successful selected placement to `TryApplyInitialPlacement`.

Production monitor selection uses `PlacementEx::FindClosestMonitor` or a valid launch monitor
handle. Current HWND style, `WM_GETMINMAXINFO`, target DPI, and snapping policy feed the existing
policy/engine adapter. Physical-coordinate scope is shared with capture. The adapter now rejects
a conservative envelope of unrepresentable native-width adjustment intermediates before calling
`MoveToMonitor`; it does not duplicate the geometry algorithm.

Native application calls `PlacementEx::SetPlacement`. The imported API has an optional
`allowLegacyFallback` argument, defaulting to the existing behavior for other callers. Hidden,
non-activating, and virtual-desktop requests require Window Action and prohibit legacy fallback,
including after a Window Action failure. The legacy path can show/activate before repairing
state, so it cannot establish these contracts. Hidden non-normal requests continue to return
false before application. There is no rollback or second-source retry after a partial failure.

`PlacementEx::SetPlacementWithApplyWindowAction` treats a same-monitor restore specially. When the
placement is not full screen or arranged, and the window's current monitor has the same work area
and DPI as the target monitor, the call omits the move-to-monitor action and the historical work
area and DPI modifiers. The saved normal rectangle is already in that monitor's coordinate space,
so those modifiers only add a frame adjustment that shifts the restored position. This also covers
maximized and minimized placements because their normal rectangle is the restore position. A
different monitor, work area, or DPI keeps the existing monitor action and modifiers, which lets a
placement from a removed or changed monitor be migrated.
`WindowPlacementApplicationTests` covers a nonzero-origin restore with an unchanged monitor.

The application path refreshes the existing capture cache from the HWND after both success and
failure; success requires native success and effective capture. It never treats `SetPlacement`'s
mutable argument as the effective snapshot. Successful minimized-from-snapped application can
supply otherwise unobservable restore history; actual geometry/state still use the capture path.
Virtual-desktop identity is not synchronously queried or inferred from the request.

First `Show` now suppresses activation for ApplicationRestart even when placement fails or is
skipped. Nonactivating display uses `SW_SHOWNA`, not the restoring `SW_SHOWNOACTIVATE`, so it does
not undo a successfully applied maximized state. A skipping initial Show still handles ordinary
pending client sizing.

**Validation:** `amd64chk` foundation, isolated placement tests, the native XAML DLL, and the full
repository build succeed. A refreshed-payload run on `ge_current-260828-Desktop` of
`*WindowPlacement*Tests*` passes **164/164**, with zero failed,
blocked, or skipped. The new real-HWND `WindowPlacementApplicationTests` class had 13 methods at
that run: hidden/no-activation behavior, actual capture, native states, guarded legacy fallback,
current constraints, malformed/extreme inputs, destruction during application, source precedence
and single load, repeated hidden saved application, and coordinator-to-native application/skip.
It now has 18, including same-monitor nonzero-coordinate restore coverage, a comparison of saved
minimized application under Default and ApplicationRestart, and restore-to-snap bounds coverage.
Display-routing coverage also exercises restart suppression after failure and skip.

**Minimized first-show diagnosis (2026-09-25):** the packaged round-trip test omitted `Reason`.
`TryApplyPlacement` passes that Default reason to `TryComputeNativeRequest`, which uses
`MapState` in `WindowPlacementPolicy.cpp` to map saved `Minimized` to `Normal`, as the spec requires.
`BuildNativeRequest` therefore selects `NativeShowCommand::Normal`, and `TryCreateEnginePlacement`
sets `SW_NORMAL`. `DoNotActivate` suppresses activation; it does not preserve minimization.
With `ApplicationRestart`, those same steps select `Minimize` and `SW_MINIMIZE` (6).
`PlacementEx::SetPlacementWithApplyWindowAction` calls `CWindowAction::SetMinimized`, which sets
`WAK_PLACEMENT_STATE` and `WPS_MINIMIZED`, before `action.Apply(hwnd)` calls the native API.

On `ge_current-260828-Desktop`, `SavedMinimizationRequiresApplicationRestart` records
`reason=0 engineShow=1 outcome=1 iconic=0 visible=1` and
`reason=2 engineShow=6 outcome=1 iconic=1 visible=1`. Here outcome 1 is `Applied`, which requires
`PlacementEx::SetPlacement` to return true and capture to succeed. NoActivate prohibits legacy
fallback, so this also proves that `action.Apply` returned true (the native API returned nonzero).
Both cases preserve exact restore bounds and produce zero activations.
The later `DesktopWindowImpl::ShowWindowForDisplay` uses `SW_SHOWMINNOACTIVE` for an iconic window;
it does not undo the minimized state. The packaged test now requests ApplicationRestart and
asserts the restored presenter is Minimized rather than skipping: saved and restored bounds are
`107,100 640x480`, at 96 DPI on work area `0,0 1024x720`. The native application class passes
18/18 and the packaged persistence class passes 18/18, with no failures or skips.
No product-policy change was needed.

**Snapped first-show diagnosis (2026-09-25):** this is an OS limitation, not a product gap. WinUI
builds the arrange request correctly. `MapState` maps saved `Snapped` to `TargetState::Snapped` when
the user snapping setting is on, `ComputePlacement` derives the snap rect from the saved arrange
rect, `BuildNativeRequest` sets `Flags.Arranged` with `NativeShowCommand::Normal`, and
`TryCreateEnginePlacement` sets `PlacementFlags::Arranged` with `SW_NORMAL`. Note that
`SetPlacementWithApplyWindowAction` treats an `Arranged` request as never being a same-monitor
placement, so it always issues `SetMoveToMonitor` along with `SetArranged`.

`SavedSnapApplicationOnANewWindow` records the result on `ge_current-260828-Desktop` with
`NoActivate`, which prohibits legacy fallback and makes the native return value the only thing that
decides the outcome.

| Saved state | snapping | Request flag | Engine show | Outcome | Observed |
| --- | --- | --- | --- | --- | --- |
| `Snapped` | 1 | `Arranged` | 1 (`SW_NORMAL`) | 3 (`FailedAfterStart`) | flags 8, iconic 0, visible 0 |
| `MinimizedFromSnapped` | 1 | `RestoreToArranged` | 6 (`SW_MINIMIZE`) | 1 (`Applied`) | show 2, flags 8, iconic 1, visible 1 |

So `action.Apply` returns false for `SetArranged` on a window that has never been shown, while
`SetMinRestoreToArranged` on the same window succeeds. Observed flags 8 is `AllowSizing` alone;
`GetPlacement` only reports `Arranged` for a window that is actually arranged, and
`RestoreToArranged` is a request-only concept that it never reports. The declined case leaves the
window hidden with zero shows and zero activations, and `AllowsSourceRetry` is false, so it does not
trigger another peer search or storage load. No product or policy change was made.

**Validation update (2026-09-25):** `MinimizedFromSnapped` restores to the saved snapped bounds.
`MinimizedFromSnappedRestoresToSavedSnapBounds` logs `saved=(0,0,512,720)` and
`observed=(0,0,512,720)` on `ge_current-260828-Desktop`, and verifies the post-restore
`PlacementEx::GetPlacement` arrange rectangle exactly. The commands were:

```text
.\initrun.ps1 msb /q dxaml\xcp\dxaml\lib\unittests\WindowPlacement\Microsoft.UI.Xaml.Tests.Isolated.WindowPlacement.vcxproj
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop 'WindowPlacementApplicationTests::*' '-SkipPackageUninstall'
```

The targeted run passes 18/18 with zero failed, blocked, or skipped. Live peer discovery now
publishes the specified per-group HWND marker and enumerates both framework-owned and foreign-process
windows. New-window snapped application is still declined by the OS, as measured above. Real
multi-monitor/mixed-DPI transitions, actual downlevel systems, virtual-desktop moves, and launch
hint scenarios still need acceptance coverage. The coordinator
inbox task is complete for the implemented integration boundary; these environment-specific
scenarios remain open acceptance gates.

Presenter transitions now have runtime coverage.
`WindowPlacementPersistenceTests::FullScreenPresenterKeepsAndSavesThePreFullScreenPlacement`
shows an enrolled window at 640x480, switches the AppWindow to the FullScreen presenter,
confirms the window really resized to the monitor work area, and then verifies that
`TryGetPlacement` still reports the pre-full-screen rectangle in the `Normal` state and that
closing while full screen saves that same rectangle. That exercises the whole production chain -
`AppWindowPresenterSupportsSizing` to `ResolvePresenterKind` to the non-overlapped early return in
`WindowPlacementCapture::TryCapture` to the retained cache - rather than only the isolated
`ResolvesPresenterKind*` unit tests. The test reports a skip, not a pass, if the environment
refuses the presenter change or does not resize the window.

There are two ways to be full screen, and they leave capture through different doors. The
presenter test above covers the AppWindow FullScreen presenter, where `ResolvePresenterKind`
reports `NonOverlapped` and `TryCapture` returns at the presenter check. An app that instead
removes its own `WS_CAPTION` and `WS_THICKFRAME` and fills the monitor rect still reports an
overlapped presenter, so it reaches the `PlacementFlags::FullScreen` guard further down.
`WindowPlacementCaptureTests::FullScreenWindowKeepsThePreFullScreenSnapshot` covers that second
door and asserts the same retention invariant.

Related open PRs checked during self-review:
[11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) changes the pending-size and
restored-size helpers that this path calls; revalidate sizing when integrating those changes.
[11772](https://github.com/microsoft/microsoft-ui-xaml/pull/11772) changes first-display preparation
and [11587](https://github.com/microsoft/microsoft-ui-xaml/pull/11587) changes window redirection.
They touch `DesktopWindowImpl`, but do not supply this placement application implementation.

## Historical acceptance and diagnostics notes

**Archived from the original design's diagnostics/acceptance section.** This section contains
implementation reports and named evidence, not the current acceptance checklist. The latter is
in the [feature specification](./Window-PlacementPersistence.md#implementation-acceptance-criteria).
No separate run date was supplied for the undated reports below.

### Failure reporting checkpoint

Status: implemented through the internal `ReportPlacementFailure` seam. Detached loading,
automatic restore (including apply), and saving report once at their operation boundary.
An explicit placement pass uses the `Apply` operation. Reports contain only fixed operation and
category values and an HRESULT; `S_OK` means the category has no native error. The bool-only
placement engine supplies no normalized HRESULT, so its failures use the category alone.
`WindowPlacementFailure` uses the existing XAML ETW provider; debug builds also emit a breadcrumb.
Missing records and successful operations emit neither.

### Prepared-placement warning checkpoint

Status: implemented in `WindowPlacementCoordinator`. The coordinator records that a hidden
application pass ran, and reports once per coordinator on the first opted-in `Activate()` while the
initial placement phase is still open. Debug builds emit developer output; release builds emit none.

| Decision | Reason |
| --- | --- |
| Arm the diagnostic only when `TryApplyInitialPlacement` reached `ApplyPlacement` | Calls rejected for a closed, visible, reentrant, or invalid request never prepared anything |
| Report after the operation starts, before the automatic pass | A rejected `Activate()` prepares nothing to discard, so it reports nothing |
| Report at most once per coordinator | The message is about the window, not about each call |
| Track state per coordinator instance, not globally | Keeps the flag thread-affine with the coordinator and needs no reset between tests |

This is development output only. It is not an event, not a failure, and not part of the behavioral
contract, so apps must not depend on it.

### Recorded acceptance status

The earlier notes recorded prototype experiments and version-specific unpackaged storage results.
Those reports are not evidence that this checkout implements the current design or that the
packaged application namespace and revised lifecycle have been validated. Re-establish the relevant
storage and deployment evidence for this implementation.

Leave the hidden-state and backend fallback decisions open until their owners
resolve them. The late-property, detached-load error, and pre-display abort decisions are now
specified, implemented, and covered:

| Decision | Covering tests |
| --- | --- |
| Late property changes | `WindowPlacementPersistenceTests::SaveUsesThePersistIdSetAtCloseTime`, `::SaveUsesTheGeometrySetByACloseHandler`, `::LateAutomaticPersistenceOptOutCancelsTheSave`, `::EmptyPersistIdAtFirstDisplayNeverEnrollsSaving`, `::LateRequestsRejectNullButIgnoreFieldsInNonNullOptions` |
| Detached-load errors | `WindowPlacementDiagnosticsTests::DetachedIdentityFailureReportsOriginalErrorOnce`, `::DetachedReadFailureReportsAndPropagatesOriginalError` |
| Pre-display abort | `WindowPlacementCoordinatorTests::AbortedFirstShowLeavesPhaseOpenAndRetriesWithFreshSnapshot`, `::AbortedSkippingFirstShowDoesNotEnroll`, `::AbortedFirstActivateLeavesPhaseOpen`, `::AbortThenCloseNeverEnrollsOrDisplays`, `WindowPlacementDisplayRoutingTests::AbortedDisplayIsRetriedAsAFirstDisplay` |

Backend concurrency and durability are no longer open items:

| Property | Covering tests |
| --- | --- |
| Two concurrent saves at the same id publish exactly one whole snapshot, never a mix | `WindowPlacementStoreTests::ConcurrentSavesPublishOneWholeSnapshot`; `WindowPlacementStorageIsolationTests::ConcurrentSavesForOneIdLeaveOneCoherentRecord` against real packaged storage |
| A load interleaved with a save returns the old record or the new one, never a partial read | `WindowPlacementStoreTests::LoadDuringSaveSeesOldOrNewWholeSnapshot` |

The store-layer tests hold the backend to the contract stated under
[Multiple windows and processes](./Window-PlacementPersistence.md#multiple-windows-and-processes): each value read and each value
replacement is atomic. `ConcurrentSavesForOneIdLeaveOneCoherentRecord` is what shows the real
packaged `ApplicationData` backend honors that contract, so the two layers only prove the end-to-end
property together.

Storage format ratification, cache policy, and virtual-desktop thread integration remain
implementation review items. The proposed unpackaged-default-store capability and automated
multi-monitor/variable-DPI environment also remain explicit dependencies; neither is supplied by the
current design notes.

The original phrase "concurrency and durability are no longer open" is retained above as a
historical assessment. Its cited tests establish coherent whole-value read/replacement, not
general power-loss durability, every shutdown behavior, or all deployment and lifetime criteria.
The current evidence index uses that narrower interpretation.

### Null or empty `persistPlacementId` and the out-parameter

`WindowPlacement.LoadForPersistPlacementId` rejects a null or empty `persistPlacementId` with
`E_INVALIDARG`. WinRT canonicalizes an empty string to a null `HSTRING`, so both inputs take the
same path.

On that argument-rejection path the out-parameter is not guaranteed to be written. The generated
stub runs its `ARG_NOTNULL` check before `*ppResult = {}`, so it returns before clearing anything.
ABI callers must initialize the out-parameter themselves and must not read it after an error. The
projections are unaffected: C# and C++/WinRT turn the failure into an exception and never hand back
a value.

A non-null id does reach the implementation, which clears the out-parameter before doing any work.
A valid but unused id therefore returns `S_OK` with a null placement.

`WindowPlacementAbiTests::DetachedLoadOfEmptyOrNullIdReturnsInvalidArgumentAndNoObject` covers all
three cases.

## Public Show and Hide projection (2026-09-25, Developer)

**Historical implementation and test report.**

The spec declares `public void Show();` and `public void Hide();` on `Window`, but both methods
only existed on the private `IWindowPrivate` interface. They are now projected onto the public
contract-12 surface.

| Change | Detail |
| --- | --- |
| XamlOM model | Added `Show()` and `Hide()` at `[Version(12)]` on `Window` |
| ABI naming | The existing options overload is now `ShowWithOptions` in the IDL; parameterless `Show()` is the `[DefaultOverload]` |
| `DesktopWindowImpl::HideImpl` | Implemented; was `E_NOTIMPL` |
| `Window::ShowImpl` | Now forwards to `m_spWindowImpl->ShowImpl()`; was `E_NOTIMPL` |

`Hide` behavior follows "Control display and activation" and the "Window.Hide method" section:

- Fails with `E_INVALID_OPERATION` when the window is closed.
- No-ops when the window is already hidden.
- Captures effective placement before hiding, so a later save or `TryGetPlacement` reports the
  geometry the window was hidden from.
- Does not save, does not close, and does not open or end the initial-placement phase. The
  `WM_SHOWWINDOW` hook only notifies display when `wParam` is true, so hiding stays inert.
- A hidden window drops out of live peer discovery on its own, because that enumeration already
  skips windows that fail `::IsWindowVisible`.

Validation: full `amd64chk` build succeeded. On VM `ge_current-260828-Desktop`, the new
`WindowShowHideTests` class passed 9 of 9, and the broader `*WindowPlacement*` suite passed 169 of
170. The single failure was the already-documented transient `coreclr.dll` crash in
`FirstActivateUsesPersistenceConfiguration`, which passed when re-run alone.

The UWP scope question is resolved by the decision below, and the public/private dispatch
separation is implemented and covered. See the
[implementation record](#implementation-record-developer-2026-09-24).

## Public display API host boundary decision (2026-09-24)

**Decision record.** The normative supported-host and validation contract lives in the
[API compatibility](../../specs/window-placement-persistence-spec.md#compatibility-and-scope)
and [error](../../specs/window-placement-persistence-spec.md#errors-and-threading) sections.
The alternatives, source risk, and follow-up evidence are preserved here.

**Decision:** keep the public display APIs desktop-only, with a runtime host check.
For a UWP-backed or framework dummy `Microsoft.UI.Xaml.Window`, otherwise valid public
`Show()`, `Show(WindowShowOptions)`, and `Hide()` calls must fail with `E_NOTIMPL` without
changing visibility, activation, placement, persistence, or audio state. Keep the methods
in the common contract metadata. Normal thread and argument validation still applies.
This does not add support to classic `Windows.UI.Xaml.Window` or change `Activate()`.

Being outside the feature's supported scope is not permission to expose an unrelated
private operation as successful public hiding. The current source establishes the risk:

- `Window::Window` still creates `UWPWindowImpl` for the framework's first/dummy window.
  A real UWP window is feature-gated, but the implementation is not dead code.
- `Window.g.h` forwards public `IWindow12::Hide` to `WindowGenerated::Hide`, which also
  implements `IWindowPrivate::Hide`. `Window::HideImpl` forwards both to the same backend.
- `UWPWindowImpl::HideImpl` calls `TearDownAudioGraph` when a sound service exists and
  otherwise returns success. It does not hide a window.
- Both UWP Show overloads already return `E_NOTIMPL`. The new `WindowShowHideTests`
  class uses WPF hosting and desktop windows, so it does not cover this host boundary.

Alternatives considered:

| Option | Decision |
| --- | --- |
| Leave UWP behavior undocumented because it is unsupported | Rejected. Public Hide can report success and perform private lifecycle work instead of hiding. |
| Implement public showing/hiding for UWP | Rejected for this feature. CoreWindow lifecycle and audio semantics need a separate design; desktop placement is the current scope. |
| Hide the entire new interface on non-desktop instances | Rejected. That would also change discovery of unrelated placement members on `IWindow12`. |
| Reject only public display calls on non-desktop instances | Selected. It bounds the feature and preserves the private lifecycle contract. |

Developer follow-up: separate public Hide dispatch from `IWindowPrivate::Hide` using the
model/code-generation path, not a hand edit to generated files. Keep the projected public
name `Hide()` and preserve private UWP audio cleanup. An unconditional `E_NOTIMPL` in the
existing shared `UWPWindowImpl::HideImpl` is not an acceptable fix. Check the underlying
window implementation, not package identity: unpackaged desktop windows remain supported.
Do not suppress errors with a successful no-op.

Acceptance requires coverage of public rejection on a non-desktop/dummy instance, unchanged
private Hide behavior with and without a sound service, and the existing desktop Show/Hide
behavior. Verify both ABI dispatch paths so a passing desktop projection test cannot mask a
shared private implementation. Where a host-specific case cannot run, record the limitation
and leave that acceptance item open. This decision was based on source review. The
[implementation record](#implementation-record-developer-2026-09-24) below reports the runtime
isolation as implemented and exercised.

Related open work: [PR #11240](https://github.com/microsoft/microsoft-ui-xaml/pull/11240)
proposes `TrySetForeground` and explicitly defers Show. It overlaps the display/activation
design, but does not supply a UWP implementation or resolve this boundary.
[PR #11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) modifies
`DesktopWindowImpl` sizing and closed-window behavior; preserve that work when integrating.

### Implementation record (Developer, 2026-09-24)

**Historical evidence, including limitations and a non-green aggregate run.**

Implemented as decided. Three parts:

**1. Generator: separate C++ names from ABI names.**

`DXamlNameAttribute` gained an optional `IdlName`. A method can now keep its projected ABI
name while getting a distinct generated C++ entry point:

```csharp
[DXamlName("HidePublic", IdlName = "Hide")]
public void Hide() { }
```

`MemberDefinition` gained `CppNameOverride` and a derived `CppName`. The framework header,
body, and forwarder templates emit `CppName`; the IDL template emits `IdlMemberInfo.Name`.
This is a general capability, not a `Window` special case.

Renaming the model member also makes `Helper.ImplementInterface` stop treating the public
member as a duplicate of `IWindowPrivate.Show`/`Hide`, so the private interface methods are
implemented again. The generated dispatch is now:

| ABI entry | Generated C++ | Partial-class impl |
| --- | --- | --- |
| `IWindowPrivate::Show` / `Hide` | `WindowGenerated::Show` / `Hide` (IFACEMETHOD override) | `Window::ShowImpl` / `HideImpl` (unchanged) |
| `IWindow12::Show` / `Hide` | `WindowGenerated::ShowPublic` / `HidePublic` | `Window::ShowPublicImpl` / `HidePublicImpl` |

The two paths also get distinct telemetry names (`Window_Show` vs `Window_ShowPublic`).

**2. Product code: the host check.**

`WindowImpl` gained `SupportsPublicDisplayApis()`: `true` in `DesktopWindowImpl`, `false` in
`UWPWindowImpl` (which also backs the framework dummy window). `Window::ShowPublicImpl`,
`Window::HidePublicImpl`, and `Window::ShowWithOptionsImpl` return `E_NOTIMPL` before doing
anything when the check fails. `UWPWindowImpl::HideImpl` and its audio cleanup were not
touched, and `IWindow12` is not gated as a whole.

**3. Coverage.**

| Acceptance item | Status |
| --- | --- |
| Existing desktop Show/Hide behavior | Covered. `WindowShowHideTests`, 9/9 pass on the VM, and they now run through `ShowPublic`/`HidePublic`. |
| Public dispatch is separate from private dispatch | Covered at build time. `Window_Partial.cpp` static-asserts that `WindowGenerated::ShowPublic`/`HidePublic` exist, so a codegen regression that collapses the names fails the build instead of silently routing public `Hide()` into the private path. |
| Public rejection on a non-desktop/dummy instance | Covered. `WindowDisplayApiHostTests` (native foundation target, `RunAs=UAP`) proves `Show()`, `Show(options)`, and `Hide()` all return `E_NOTIMPL` on the non-desktop instance. No test hook was needed: under UAP hosting `Window::Current` already routes to `DXamlCore::GetDummyWindowNoRef()`, which is backed by `UWPWindowImpl`. |
| Unchanged private `Hide` with and without a sound service | Covered. `PrivateHideSucceedsWithoutSoundPlayerService` and `PrivateHideSucceedsWithSoundPlayerService` call `IWindowPrivate::Hide` on the same instance whose public `Hide()` is gated. `PublicAndPrivateHideDispatchSeparately` proves both paths on one object. |

**`WindowDisplayApiHostTests`**, 7/7 pass on the VM:

| Test | What it proves |
| --- | --- |
| `PublicShowReturnsNotImplementedOnNonDesktopHost` | `IWindow12::Show()` returns `E_NOTIMPL`, visibility unchanged. |
| `PublicShowWithOptionsReturnsNotImplementedOnNonDesktopHost` | `ShowWithOptions(options)` returns `E_NOTIMPL`; `ShowWithOptions(nullptr)` returns `E_INVALIDARG`. |
| `PublicHideReturnsNotImplementedOnNonDesktopHost` | `IWindow12::Hide()` returns `E_NOTIMPL`, visibility unchanged. |
| `PublicAndPrivateHideDispatchSeparately` | On one object, public `Hide()` is gated and `IWindowPrivate::Hide()` still succeeds. |
| `PrivateHideSucceedsWithoutSoundPlayerService` | Private `Hide` succeeds when no sound service was ever created. |
| `PrivateHideSucceedsWithSoundPlayerService` | Private `Hide` succeeds after `ElementSoundPlayer` forces the service to exist, and the service is still usable afterwards. |
| `PlacementMembersRemainAvailableOnNonDesktopHost` | The rest of `IWindow12` is not gated and reports "nothing persisted". |

Two limitations worth recording. `UWPWindowImpl::HideImpl` calls `TearDownAudioGraph()`, which a
test cannot observe directly, so the sound-service test verifies the service is still readable and
settable afterwards rather than asserting the teardown itself. And `Window.Visible` can legitimately
fail on the dummy window, so the tests snapshot `{succeeded, hr, visible}` and assert the snapshot is
identical before and after, which holds whether the read succeeds or fails.

**4. Null options are rejected at the public boundary.**

Writing these tests settled an open question about the SAL on the `options` parameter. The generated
`WindowGenerated::ShowWithOptions` and `WindowGenerated::TryApplyInitialPlacement` both start with
`ARG_NOTNULL(pOptions, "options")`, so an external caller passing null gets `E_INVALIDARG` before the
host gate runs. That is the enforced public contract, and `_In_` in `Window.g.h` matches it. The
`_In_opt_` on `Window_Partial`, `DesktopWindowImpl`, and `UWPWindowImpl` is also correct, because
`ShowImpl` calls `ShowWithOptionsImpl(nullptr)` internally to mean "no options". The two annotations
describe different layers and neither needs to change.

The [PM decision below](#required-options-decision-2026-09-24) retains this public boundary:
default hidden preparation requires an empty `WindowShowOptions`, not null or a new overload.

**5. The private path is deliberately ungated.**

`Window::ShowImpl` and `Window::HideImpl` (the `IWindowPrivate` path) forward straight to
`m_spWindowImpl` with no host check. That is intentional and preserves the existing `IWindowPrivate`
lifecycle contract. On a non-desktop host the forward still ends in `UWPWindowImpl::ShowImpl`, which
returns `E_NOTIMPL` on its own, so the observable result is unchanged from before this work.

Regression run: `*WindowPlacement*` on the VM, 170/171 pass. The one failure is the known
`FirstActivateUsesPersistenceConfiguration` flake, which passes when re-run alone.

### Required-options decision (2026-09-24)

**Decision and historical developer handoff.** The handoff's coverage request is not a test result.
The API proposal owns the final signatures and argument checks.

**PM decision:** Keep `TryApplyInitialPlacement(WindowShowOptions options)` as the only hidden
application signature. Require non-null options for both it and `Show(options)`. Use
`TryApplyInitialPlacement(new WindowShowOptions())` for default hidden preparation and `Show()`
for default display. This resolves the SAL follow-up; it is not final public API review approval.

| Option | Benefit | Cost and decision |
| --- | --- | --- |
| Require a non-null options object; retain the current signatures | Both options methods have the same argument rule; matches the generated boundary and existing hidden-preparation examples | Default hidden preparation requires an object. Selected: hidden preparation already requires explicit lifecycle sequencing, and the default request has a direct expression. |
| Accept null as defaults | Avoids constructing an options object for default hidden preparation | Adds a second representation of defaults and changes public argument validation. Making only hidden application nullable would also make the two options methods inconsistent. Not selected. |
| Add `TryApplyInitialPlacement()` | Gives hidden preparation the same convenience overload as `Show()` | Adds a public overload and generated ABI surface only for shorthand; it does not add a placement capability. Not selected for this version. |

An empty options object has `Placement = null`, `Reason = Default`, and
`CascadeBehavior = Automatic`. With a non-empty id and automatic persistence enabled, it selects
a peer, saved value, or fallback under existing policy. It does not mean "saved placement only";
set `CascadeBehavior = Disabled` to avoid selecting a peer. No separate load is required.
Hidden application remains repeatable and does not enroll saving. A revealing first `Show`
still needs `SkipInitialPlacement = true` to preserve preparation.

Null **options** and null **Placement** have different meanings. The former is `E_INVALIDARG`
(`ArgumentException` in .NET) on every public options call, including late or reentrant calls.
The latter is a valid field value meaning no explicit placement. Generated required-argument
checks run before thread, closed-window, host, and coordinator checks. Only fields of a non-null
request can be ignored by late/reentrant policy. Rejection performs no placement, display,
storage, or enrollment work and does not consume the initial-placement phase.

The implementation may continue using null internally as a default request. Do not change the
public SAL to `_In_opt_` or expose that internal convention. Source inspection of
`WindowGenerated::ShowWithOptions` and `WindowGenerated::TryApplyInitialPlacement` confirms their
`ARG_NOTNULL` checks already implement the chosen boundary. The previous spec text promising
null-as-default behavior was inconsistent with those entry points and is now corrected.

**Developer handoff:** No signature or product behavior change is requested. Add public
projection coverage for both null arguments, rejection before side effects, a valid request
after rejection, and late/reentrant null rejection versus ignored fields in non-null requests.
Cover hidden preparation with no explicit placement, a known saved placement, and cascading
disabled; separately retain default peer-selection coverage. Check HRESULT/exception mapping in C# and C++/WinRT.
The existing native non-desktop Show test covers only one null-argument path; it does not
establish desktop hidden-application coverage. Run the new tests on the VM.

## Superseded wording and unresolved boundaries

The source material contained several descriptions that must not be treated as competing rules:

- An old cross-process passage said a peer-derived hidden application "counts as applied placement
  for save eligibility." The later first-display decision supersedes that wording: hidden
  application never enrolls or makes a never-displayed window save.
- Earlier reader and lifecycle paragraphs said HRESULT-carrying boundaries or save hooks still
  needed implementation, while later dated reports record them as implemented. The feature now
  states required behavior without retaining stale "today's implementation" claims.
- An older pipeline sentence ended the phase after validation. The adopted abort/retry decision
  ends it only on actual display and discards a surviving pre-display abort's enrollment decision.
- The original acceptance list describes explicit hidden customization as preserved by a skipping
  Show "before presenter selection and display." The operative workflow selects the presenter
  after hidden application and before the revealing Show; S37 remains open for the whole transition.
- The original S11 says, "After hidden restart application, `Activate()` restores prepared
  minimization without reloading, unlike `Show`, which preserves it." The adopted phase rules and
  S24 require a skipping first `Show` before that later activation; immediate initial `Activate`
  runs a fresh default-policy pass. The catalog retains S11's restart/minimization outcome and
  states the required display sequence rather than repeating the inconsistent shortcut.
- S37, downlevel hidden/non-activating fallback, launch-monitor delivery, and unpackaged default
  storage remain unresolved. This reorganization neither chooses a fallback matrix nor creates
  new platform guarantees.

[SetPropW]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setpropw
[EnumWindows]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumwindows
[EnumDisplayMonitors]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumdisplaymonitors
[GetMonitorInfoW]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getmonitorinfow
[MONITORINFO primary flag]: https://learn.microsoft.com/windows/win32/api/winuser/ns-winuser-monitorinfo
[RegisterApplicationRestart]: https://learn.microsoft.com/windows/win32/api/winbase/nf-winbase-registerapplicationrestart
[Copy-construction guidance]: https://learn.microsoft.com/windows/apps/develop/cpp-winrt/consume-apis#dont-copy-construct-by-mistake
[App data lifetime]: https://learn.microsoft.com/windows/apps/develop/data/store-and-retrieve-app-data
[ShowWindow startup behavior]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-showwindow
[STARTUPINFO show command]: https://learn.microsoft.com/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfow
[CreateWindowEx startup behavior]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-createwindowexw
[WINDOW_ACTION state model]: https://learn.microsoft.com/windows/win32/winmsg/winuser/ns-winuser-windowaction
[ApplyWindowAction requirements]: https://learn.microsoft.com/windows/win32/winmsg/winuser/nf-winuser-applywindowaction
[Package identity]: https://learn.microsoft.com/windows/apps/desktop/modernize/package-identity-overview#package-family-name
[Current application identity]: https://learn.microsoft.com/windows/win32/api/appmodel/nf-appmodel-getcurrentapplicationusermodelid
