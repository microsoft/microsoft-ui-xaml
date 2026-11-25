# Loading / Loaded / Unloaded event investigation

Assistant-maintained investigation, TODOs, and experiments. The concise handwritten note remains in
[loading-loaded-unloaded-events.md](loading-loaded-unloaded-events.md).

## Already covered by the handwritten proposal

Comparison as of 2026-11-25. The handwritten proposal owns the contract described below; this file
retains supporting source evidence and tracks implementation work. "Covered" means documented,
not implemented or validated. Legacy Loaded/Unloaded behavior remains separate from the new pair.

| Topic already described | Handwritten section | Supporting material in this investigation |
| --- | --- | --- |
| Existing Loading, Loaded, Unloaded scheduling and IsLoaded's pending-Loaded dependency | [Existing events](loading-loaded-unloaded-events.md#loading--loaded--unloaded-events) | [Loading consumers](#loading-consumers), [legacy compatibility analysis](#compatibility-consequences) |
| Add Loaded2/Unloaded2 rather than changing the legacy pair | [Loaded2 / Unloaded2](loading-loaded-unloaded-events.md#loaded2--unloaded2) | [Primary-feature TODOs](#primary-feature-todos) |
| Delay direct handler execution until the post-layout, pre-render tick checkpoint; avoid Enter/Leave and Popup.Close reentrancy | [Timing](loading-loaded-unloaded-events.md#timing---delayed-synchronous) | [Inline versus tick call-order evidence](#compatibility-of-inline-versus-tick-delivery) |
| Post-layout timing makes template/layout output potentially available, not guaranteed for collapsed elements | [Timing](loading-loaded-unloaded-events.md#timing---delayed-synchronous) | [Confirmed AutoSuggestBox counterexample](#autosuggestbox-confirmed-false-positive-readiness), [layout-output audit](#layout-output-consumers-initial-calculations-and-change-coverage) |
| Preserve pending Loaded2 across Leave and deliver historical attachment/departure occurrences in order, even while currently detached | [Differences](loading-loaded-unloaded-events.md#differences-with-loaded--unloaded) | [Concern 4 and recipient-retention details](#delayed-sync-reassessment-2026-11-24) |
| Defer historical parent/root context and generation counters; current properties do not identify historical context or whether notifications have caught up | [Potential future expansion](loading-loaded-unloaded-events.md#potential-future-expansion) | No generation ID required on current evidence. An event-side ID alone cannot check freshness; an element-side current value would also be needed. |

These are no longer unanswered design questions in this investigation. The detailed audit below
includes earlier alternatives and legacy-handler assumptions; those do not override the handwritten
proposal's choice of a new, ordered, delayed pair.

## Not yet covered by the handwritten proposal

These are additional findings, implementation requirements, or optional follow-ups, not edits requested
for the handwritten note.

| Topic not yet described | Current finding or direction | Detail / tracking |
| --- | --- | --- |
| Unified lifecycle drain and relayout (concern 5) | Use one ordered Loaded2/Unloaded2 queue. Append handler-generated occurrences and drain both kinds; process layout-generated occurrences before rendering too. | [Primary-feature TODOs](#primary-feature-todos) |
| Retain recipients, not just queue entries (implementation part of concern 4) | Leave disables registrations. Keeping a pending occurrence alone does not bypass today's active-handler filter; retain eligible recipients for the new path. Pairing is per occurrence, not a requirement that every subscriber listen to both events. | [Delayed-sync reassessment](#delayed-sync-reassessment-2026-11-24) |
| Request a frame for pending events | Removal can happen in input/async work outside a tick; delivery must request a tick rather than depend on unrelated rendering. | [Primary-feature TODOs](#primary-feature-todos) |
| Preserve actual Leave's transition/keep-alive gates | Popup.Close can return while its child remains in unloading storage. Actual Leave waits for retention reasons to finish; moving notification delivery must not bypass that gate. | [Transition versus queue timing](#compatibility-of-inline-versus-tick-delivery) |
| IsLoaded usability inside Loaded2 | IsLoaded depends on legacy Loaded delivery. Ensure suitable legacy-before-new ordering, or revise the property with compatibility assessment; include handler-driven mutations. XamlRoot alone is not proof of live attachment. | [Primary-feature TODOs](#primary-feature-todos) |
| Current root association can survive detachment | Live-tree Leave deactivates the element without clearing its cached weak visual-tree association. Use a reliable current-attachment check alongside XamlRoot for presentation work. | Getter path: UIElement_Partial.cpp:3229 -> XamlRoot_Partial.cpp:219 -> VisualTree.cpp:1397; tree storage/deactivation: depends.cpp:818,1256,4073 |
| Control migration and rapid-reentry policy (concerns 1-3) | Ordered notifications do not rewind tree mutations or layout/property work. ItemsRepeater needs reset/reconnection adaptation; ScrollPresenter needs viewport/cancellation policy; ColorSpectrum needs generation/cancellation policy. | [Reassessment](#delayed-sync-reassessment-2026-11-24), [control checklist](#control-adaptation-checklist) |
| Close-completion consumers | ContentDialog, flyout handoff, and ApplicationBarService need eligible completion notifications even when listeners were attached only for closing. Their obligations are operation-driven, not necessarily setup performed in Loaded. | [Closure work](#two-distinct-kinds-of-closure-work), [call-order traces](#compatibility-of-inline-versus-tick-delivery) |
| Separate dependency-complete initialization | IsInitialized/InitializedChanged remains a candidate mechanism; Loaded2 does not guarantee template/dependency readiness. Hard descendant dependencies, retemplating, external retry notifications, and API reporting rules need separate design. | [Initialization proposal](#proposed-initialization-api-scope-dependencies-and-naming), [ASB reproduction](#autosuggestbox-confirmed-false-positive-readiness) |
| Loading / x:Bind timing | Generated Loading handlers initialize bindings. Moving initialization to live entry can sample app data earlier, especially for OneTime bindings; first measure is normally deferred but can be forced explicitly. | [Loading consumers](#loading-consumers) |
| Optional inline timing and flyout staging work | Inline Enter/Leave callbacks and late popup child Enter/Leave remain experiments, not the primary contract. Stage-before-Hide is an independent latest-request-wins improvement, not required for delayed Unloaded2. | [Six experiments](#experiment-backlog) |
| Detailed consumer evidence | Loaded/Unloaded responsibility buckets, subscriber inventory, layout/VSM invalidation paths, and source-backed failure traces are supporting research rather than additional API promises. | [Audit](#control-subscriber-and-compatibility-audit-2026-11-19), [Unloaded buckets](#unloaded-scenario-breakdown-2026-11-23), [source references](#source-references) |

The [implementation backlog](#primary-feature-todos) separates contract work already described by the
handwritten proposal from additional work it has not described. All six primary-feature TODOs and six
experiments remain pending. No product implementation is recorded here.

## Supporting evidence and earlier design exploration

The following sections preserve the detailed investigation. Statements about changing the legacy
events or trying inline delivery are historical alternatives, not changes to the current additive
Loaded2/Unloaded2 proposal. New-event delivery is post-layout/pre-render; IsInitialized and inline
delivery are separate experiments. Only the AutoSuggestBox case below records a runtime reproduction.

## Control subscriber and compatibility audit (2026-11-19)

This section records a static source investigation of production code in `dxaml\xcp`, `dxaml\phone`, and
`controls\dev`, including subscriptions to template parts, ancestors, targets, and host elements. Tests, samples,
and generated API event declarations are not counted as control consumers. Source presence does not imply that
every consumer ships in every configuration. TableView consumers are listed separately because they belong to
the adjacent Tabular controls DLL.

The subscriber tables identify dependencies and compatibility risks, not experimentally confirmed failures of
a proposed change. A later focused AutoSuggestBox reproduction is recorded below. No product implementation
change was made as part of this investigation.

### Summary

The events are used as several different contracts, rather than as one consistently paired state machine:

* `Loading` is a pre-template measurement hook and an initialization trigger for compiler-generated `x:Bind`.
* `Loaded` commonly means that templates, bindings, layout, and host relationships are ready to inspect or use.
  Handlers discover parts, set focus, synchronize selection, connect composition resources, and resume deferred work.
* `Unloaded` is not just resource cleanup. Handlers also close popups, complete async operations, advance flyout
  state machines, cancel interactions, maintain group membership, and balance registration counts.

Here, "Loaded is synchronous" means that handlers run synchronously at the delivery point during tick, not that
they run synchronously with the original tree insertion. That distinction reconciles the terminology used above
with comments in controls and issue discussions.

### Loading consumers

No direct production control subscriptions to `FrameworkElement.Loading` were found in the audited MUX and MUXC
runtime source. However, compiler-generated `x:Bind` code is a major consumer:

* For FrameworkElement file roots, the compiler subscribes to `Loading` and calls `Bindings.Initialize()`.
* `Initialize()` performs the initial binding update unless the bindings are already initialized.
* The generated C# and C++/WinRT paths both use this mechanism. Window roots use a different activation path;
  this is not a claim that every binding universe initializes from `Loading`.

Sources: [C# subscription][cs-loading-subscription], [C# handler][cs-loading-handler],
[C# initialization][cs-binding-initialization], [C++/WinRT initialization and handler][cpp-loading-handler].

Currently, [`MeasureCore` raises Loading before applying the template][measure-loading].
Changing when it fires can therefore change when initial binding evaluation, getters, converters, and bound
property assignments execute. Raising it for collapsed elements would initialize bindings that currently wait
until measurement, unless another initialization path has already run.

### MUX subscriber inventory

`L` means `Loaded`; `U` means `Unloaded`. Events may be subscribed on the control itself or on its parts.
Inherited subscriptions also affect derived controls.

| Controls or support code | Events | Handler behavior |
| --- | --- | --- |
| ButtonBase, ListBox, ListBoxItem, ComboBoxItem, ListViewBaseItem | L | Refresh visual states without transitions. Representative: [ButtonBase][button-loaded]. |
| Page, MenuFlyoutPresenter, WindowChrome | L; Page also U | Establish or repair initial focus; Page can notify accessibility clients and removes a layout-bounds callback on unload. [Page][page-loaded], [MenuFlyoutPresenter][menu-presenter-loaded], [WindowChrome][window-chrome-loaded]. |
| AppBar, SplitView | L/U | Set up host/layout subscriptions, registration, focus, overlays, and dismiss layers. Unload tears down overlays/dismiss layers and registrations. [AppBar][appbar-loaded], [SplitView][splitview-loaded]. |
| AutoSuggestBox | L/U | Child Loaded locates the query button, attaches Click, sets its accessibility name, and sizes/positions suggestions. Unloaded removes query-button subscriptions or overlay state, with active/live-tree guards. [Part handlers][autosuggest-parts], [control unload][autosuggest-unloaded]. |
| ComboBox, CommandBar | L/U | Part Loaded realizes/scrolls selection, discovers nested parts, hooks keyboard handling, and sets focus. Unloaded closes dropdown/overflow; CommandBar also restores compact mode. [ComboBox unload][combobox-unloaded], [ComboBox part Loaded][combobox-part-loaded], [CommandBar unload][commandbar-unloaded], [CommandBar part Loaded][commandbar-part-loaded]. |
| ContentDialog, FlyoutBase, ApplicationBarService | L/U | Lifecycle orchestration: focus/position/open notifications on Loaded; close completion, async completion, staged-flyout opening, or popup lifetime management on Unloaded. [ContentDialog][dialog-unloaded], [FlyoutBase][flyout-unloaded], [ApplicationBarService][appbar-service-subscription]. |
| ItemsPresenter | L/U | Find the owning ScrollViewer, attach viewport subscriptions, and delay-load the footer. Unload detaches subscriptions. Loaded first removes any old subscription because Unloaded can be missing. [Handlers][items-presenter-loaded]. |
| ListViewBase, ModernCollectionBasePanel | L and/or U | Resume phased work and deferred scroll commands on Loaded. Panel unload detaches components, removes sticky headers, and invalidates measure. [ListViewBase][listview-loaded], [panel unload][panel-unloaded], [component cleanup][panel-detach]. |
| ScrollViewer, ScrollContentPresenter | L/U | ScrollViewer maintains its own loaded flag and synchronizes DirectManipulation transforms after viewport sizing; unload resets input/indicator state. ScrollContentPresenter unparents headers on unload. [ScrollViewer][scrollviewer-loaded], [ScrollContentPresenter][scroll-content-unloaded]. |
| Popup, MediaTransportControls, TimePicker | L | Position popup; update progress UI and discover/measure CommandBar parts; subscribe to host-window activation. [Popup][popup-child-loaded], [media progress][media-root-loaded], [media CommandBar][media-commandbar-loaded], [TimePicker][timepicker-loaded]. |
| MenuFlyoutItem, CascadingMenuHelper | L | Clear menu interaction state flags. [MenuFlyoutItem][menu-item-loaded], [CascadingMenuHelper][cascading-menu-loaded]. |
| Hub, HubSection | L/U | Maintain loaded flags; process deferred/default-section scrolling and focus; update header styling. [Hub][hub-panel-loaded], [HubSection][hub-section-loaded]. |
| Pivot and slide-in support | L/U | Synchronize parallax after ScrollViewer load, complete animations on unload, and register/unregister slide-in elements with their parent PivotItem. [Pivot load][pivot-loaded], [Pivot unload][pivot-unloaded], [slide-in handlers][pivot-slide-in]. |

Additional framework-wide consumers:

* ElementName binding resolution can retry on Loaded, then disconnect that handler:
  [`BindingExpression::TargetLoaded`][element-name-loaded].
* FrameworkElement uses Unloaded to cancel an open automatic tooltip:
  [`FrameworkElement::OnUnloaded`][tooltip-unloaded].
* `EventTrigger` supports `FrameworkElement.Loaded`:
  [event trigger implementation][event-trigger].

### MUXC subscriber inventory

| Controls or support code | Events | Handler behavior |
| --- | --- | --- |
| AnimatedIcon, ProgressRing, NumberBox, TabViewItem | L | Apply inherited/bound icon state and fallback; refresh animation/VSM state, spin-button placement, or tab separators. AnimatedIcon explicitly waits for bound properties. [AnimatedIcon][animated-icon-loaded], [ProgressRing][progress-ring-loaded], [NumberBox][numberbox-loaded], [TabViewItem][tab-item-loaded]. |
| BreadcrumbBar, BreadcrumbBarItem, AutoSuggestBoxHelper, TwoPaneView | L | Refresh repeater items; discover/configure child parts; attach Click/popup handlers; adjust nested ScrollContentPresenter behavior. [BreadcrumbBar][breadcrumb-loaded], [BreadcrumbBarItem][breadcrumb-item-loaded], [AutoSuggestBoxHelper][autosuggest-helper-loaded], [TwoPaneView][two-pane-loaded]. |
| RadioButtons, SelectorBar, NavigationView's repeaters | L | Reconcile initial selection with realized containers/selection models and update selection animation. [RadioButtons][radio-buttons-loaded], [SelectorBar][selectorbar-loaded], [NavigationView repeaters][navigation-repeater-loaded]. |
| NavigationView, NavigationViewItemBase, NavigationViewItem, TabView | L/U or L | Establish/rebind ancestor and host relationships; title-bar subscriptions; deferred visual updates; tab content, tear-out registration, and nonclient regions. [NavigationView][navigation-lifecycle], [item base][navigation-item-base-loaded], [item][navigation-item-loaded], [TabView][tabview-lifecycle]. |
| CommandBarFlyoutCommandBar, TextCommandBarFlyout | L | Update command states/geometry, establish initial focus, connect backdrop, and invoke the proofing button to open its menu, sometimes after a delay. [CommandBar][commandbar-flyout-loaded], [proofing menu][proofing-loaded]. |
| AnimatedVisualPlayer, WebView2, InkCanvas, SystemBackdropElement | L/U | Attach/rebind composition and host resources, restore content, and disconnect/release on actual unload. Each has stale-event/current-state defenses, directly or through callees. [AnimatedVisualPlayer][animated-player-lifecycle], [WebView2][webview-lifecycle], [InkCanvas load][ink-loaded], [InkCanvas unload][ink-unloaded], [SystemBackdropElement][backdrop-lifecycle]. |
| ScrollPresenter, ItemsView | L/U | Initialize manipulation/scroll-controller plumbing after layout. On actual unload, interrupt pending operations, remove rendering callbacks, and clear navigation/bring-into-view state. Both guard unload with `!IsLoaded()`. [ScrollPresenter][scroll-presenter-lifecycle], [ItemsView][itemsview-lifecycle]. |
| ItemsRepeater | L/U | Maintain Loaded/Unloaded counters to recognize delayed unloads; reset scrollers and realization-window caches when appropriate. [Handlers][repeater-lifecycle]. |
| ParallaxView, ScrollInputHelper, pull-to-refresh ScrollViewer adapter, SwipeControl | L | Discover source/target relationships and initialize composition interactions. SwipeControl initializes once but closes on every reload, including recycling. [ParallaxView][parallax-loaded], [ScrollInputHelper][scroll-input-loaded], [refresh adapter][refresh-adapter-loaded], [SwipeControl][swipe-loaded]. |
| ColorPicker, ColorSpectrum, PersonPicture | U | Cancel outstanding bitmap generation/image reads. ColorSpectrum checks `!IsLoaded()`; ColorPicker and PersonPicture do not. [ColorSpectrum][spectrum-unloaded], [ColorPicker][colorpicker-unloaded], [PersonPicture][person-picture-unloaded]. |
| TeachingTip | U on itself/target; L on target | Close the tip when it or its target unloads; reposition when its target loads. [Close handler][teaching-tip-unloaded], [target Loaded][teaching-tip-loaded]. |
| RadioMenuFlyoutItem | L/U; submenu L | Restore/remove radio-group membership and update submenu checked visual state. [Lifecycle handlers][radio-menu-lifecycle], [submenu handler][radio-submenu-loaded]. |
| ScrollView, ResizeGripper | U | Reset indicators, bring-into-view work, rendering callbacks, and timers; cancel an active resize drag and release pointer capture. ResizeGripper guards against stale unloads. [ScrollView][scrollview-unloaded], [ResizeGripper][resize-unloaded]. |
| InkToolbar, AnnotatedScrollBar | U or L | InkToolbar flushes session telemetry on Unloaded. AnnotatedScrollBar's Loaded subscription is debug-only tracing. [InkToolbar][ink-toolbar-unloaded], [AnnotatedScrollBar][annotated-loaded]. |

Adjacent Tabular consumers:

* TableView uses Loaded/Unloaded to acquire/release source ownership, drain/restore its repeater, manage
  host/theme subscriptions, cancel pending work, and record telemetry. Its unload handler checks `IsLoaded()`
  to avoid tearing down a reparented live control. [Load][table-loaded], [unload][table-unloaded],
  [rows repeater reload][table-repeater-loaded].
* TableViewRow and TableViewColumnHeader use Loaded for geometry and visual states.
  Row Unloaded restores temporarily suppressed cell-content tab stops.
  [Row handlers][table-row-lifecycle], [column header][table-header-loaded].
* TableViewTextColumn uses editor Loaded to discover TextBox internals and refresh editing metrics.
  [Editor handler][table-editor-loaded].
* TableViewCellAutomationPeer uses Unloaded as one path to release a held pre-edit accessibility name.
  [Automation cleanup][table-peer-unloaded].

### Loaded scenario summary and initialization-signal coverage

The discussion now consolidates the observed Loaded buckets into two primary mechanisms:

1. **Correct control initialization:** finish the setup that depends on templates, bindings, realized children,
   and required initialization dependencies before dependent presentation/input.
2. **Entering/tree-context reconciliation:** connect to current owners, ancestors, roots, and hosts.
   Root/host attachment (original bucket 2) is a specialization of structural relationship reconciliation
   (bucket 3), not a separate fundamental mechanism. WebView2 is a concrete root/host example.
   This includes the scenario behind item 5 of the [Reactor wishlist][reactor-wishlist], independently of its proposed API.

**Earlier initialization exploration:** splitting dependency-complete initialization from attachment
remains useful, but is not part of the handwritten Loaded2/Unloaded2 contract. The initial naming
discussion considered retaining Loaded; the current proposal instead adds Loaded2/Unloaded2 and keeps
legacy behavior intact.

When continuing this design, start from **initialized versus entered**: required local/subtree readiness
belongs to IsInitialized/InitializedChanged; attachment/context reconciliation and per-entry actions
belong to the Loaded/entered mechanism. Controls doing both will need to coordinate both conditions.
The reporting/invalidation API, subtree enforcement, and treatment of owned popup/detached dependencies
remain open. This note records an investigation and candidate design, not an implemented product change.

**Contract already covered:** see the handwritten proposal's
[timing](loading-loaded-unloaded-events.md#timing---delayed-synchronous) and
[differences](loading-loaded-unloaded-events.md#differences-with-loaded--unloaded) sections.
New occurrences preserve live Enter/Leave order, including transient attachments; pending Loaded2
survives Leave. The additional queue-draining, frame-request, recipient-retention, IsLoaded integration,
and transition-gate work is tracked [above](#not-yet-covered-by-the-handwritten-proposal).

**U6 flyout handoff:** no further flyout-specific blocker was identified for delayed-sync delivery after
Popup.Close finishes. Presenter Unloaded raises the old flyout's Closed, clears the open-flyout slot, and
opens the staged successor. The tick policy must accommodate that successor's layout and Loaded before
rendering, preserving A.Closed -> B.Opening -> B.Loaded/Opened ordering. Deferred delivery lets the current
stage-after-Hide code finish staging, and avoids reopening the same popup while its old Close still has
it registered in the open-popup list.

**Independent stage-before-Hide experiment:** record B as staged before calling A.Hide.
If A.Closing requests C, the newer C should replace B instead of the older B request overwriting C after
Hide returns. If Hide fails or Closing is canceled, B should remain staged as the outstanding request;
that does not guarantee eventual opening if A never closes. Staging is a single latest-request-wins slot,
not a FIFO queue. This improvement is useful even with delayed-sync Unloaded, but is not required
for the new event pair: delayed completion lets the existing Hide-then-stage sequence finish first.

Preserving pending Loaded2 across Leave is a required
[primary-feature TODO](#primary-feature-todos), not an optional experiment.
The six items in the [experiment backlog](#experiment-backlog) remain pending. Inline Loaded-on-enter,
late Enter in Popup.Open, and coordinated end-of-Leave Unloaded/late child Leave in Popup.Close are
exploratory follow-ups, not prerequisites for the delayed-sync Unloaded first step. The current placement
of bookkeeping after child removal does not prove that all of it must remain there; any future reordering
must preserve actual removal-result, lifetime, animation, focus, and collection invariants.

**U6 ContentDialog reviewed:** its popup-child Unloaded reaches OnFinishedClosing, which clears Popup.Child,
raises Closed, restores temporary state, and completes ShowAsync after clearing the current-operation
pointer. The previously traced Child-clearing conflict concerns inline delivery inside Popup.Close;
it disappears with delivery after Close returns. Preserve final-departure timing and exactly-once
completion, including Closed-before-ShowAsync-completion ordering. No additional need for ordinary
dispatcher delivery was established. No lifecycle implementation changes or new experiment results
are recorded here.

**Delayed-sync reassessment:** the ContentDialog inline conflict likewise disappears after Close
returns. The [reassessment below](#delayed-sync-reassessment-2026-11-24) identifies the next concrete
compatibility work: post-layout unload can invalidate a reattached ItemsRepeater's scroller hookups,
zero a reattached ScrollPresenter's viewport, or cancel ColorSpectrum's newly started bitmap work.
Ordering notifications Unloaded-before-Loaded does not by itself order them before layout/property
initialization for the new attachment. These are control/checkpoint issues to resolve, not reasons to
retain ordinary dispatcher delivery or the old stale-event guards wholesale.

Track the known control-side changes in the [control adaptation checklist](#control-adaptation-checklist).
ItemsRepeater's reset already runs in today's Loaded recovery path; moving reset to ordered Unloaded
and requesting reconnection on Loaded does not inherently add another reset/measure cycle.

Buckets 4-6 and 8 describe actions driven by entry/exit, with initialization prerequisites where necessary.
For now, bucket 7 does not establish an independent need for Loaded: initial calculation, ongoing change
handling, and dependency-readiness recovery can cover it. This is a classification/design conclusion,
not an experimentally verified claim that every current Loaded handler can simply be deleted.

A "correct control initialization signal" here is a hypothetical contract, not an existing API or a proposal to
rename Loaded. It must cover the control's required dependency chain recursively, including completion of
dependency initialization handlers and relevant binding/template work. Delivery must finish before the first
dependent presentation/input, but need not be an inline callback from each `ApplyTemplate` call. Its contract
must define collapsed/deferred dependencies, externally supplied dependencies, late subscribers, and renewed
initialization after retemplating or dependency replacement. It cannot simply declare every descendant ready
when the parent's template objects have been created.

The remaining observed responsibilities are not all reducible to that signal:

| Observed Loaded responsibility | Examples | What a correct initialization signal would cover |
| --- | --- | --- |
| Dependency-complete control initialization | AutoSuggestBox query-button hookup; nested parts in CommandBar, MediaTransportControls, TabView, TwoPaneView; ParallaxView's external Source; child-state setup in NumberBox and BreadcrumbBarItem; refresh adapter's template-generated parent | The initialization work, if the necessary dependencies are included. These are nine identified control-level scenarios, not nine removable handlers or an exhaustive count. Initial binding, selection, and VSM setup can also fit here. |
| Root/host attachment and migration | [WebView2][webview-lifecycle], InkCanvas, SystemBackdropElement; host subscriptions in NavigationView and TabView | Only initial setup that happens to share this timing. A once-only signal does not report attachment to a different root/host. |
| Structural ownership/ancestor rebinding | [NavigationViewItemBase][navigation-item-base-loaded] finding its NavigationView; [ItemsPresenter][items-presenter-loaded] finding its ScrollViewer; Pivot slide-in registration | Initial discovery can fit. Moving between owners within the same root needs a relationship-change mechanism, not just initialization or root migration. |
| Per-presentation actions | Initial focus in Page, MenuFlyoutPresenter, ContentDialog and FlyoutBase; [FlyoutBase focus and Opened notification][flyout-presentation] | First-presentation readiness can fit, but focusing or reporting each subsequent presentation is a separate responsibility. |
| Interaction-state reset on reuse | [SwipeControl][swipe-loaded] initializes once but closes on every load, including recycling; menu handlers clear interaction flags | Initial state can fit. An already initialized, reused instance may still need a reset. |
| Restart deferred work that needs the control to be in the tree | [ListViewBase][listview-loaded] re-registers pending phased realization work; the worker gates phase advancement on live-tree membership rather than waiting for Unloaded to pause it | Tree presence is the execution policy for this work, not necessarily a fundamental inability to execute it off-tree. Restarting after a later detach/re-entry is not one-time initialization. Work deferred solely for template or layout readiness belongs under initialization or layout output instead. |
| Obtaining usable layout output | AutoSuggestBox suggestion placement, [Popup positioning][popup-child-loaded], ScrollViewer DirectManipulation setup, Pivot parallax; deferred scrolling in ListViewBase and Hub also overlaps initialization/layout readiness | Recursive template readiness alone does not provide measured/arranged sizes, positions, viewport/extents, or bounds in the needed coordinate space. However, initial calculation plus complete change notifications can maintain derived layout output; the audit below distinguishes missing invalidation from setup-order retries. These consumers do not establish that a separate Loaded-like layout signal is inherently necessary. |
| Restoring resources released while detached | [AnimatedVisualPlayer][animated-player-lifecycle] restores content; composition-backed controls reconnect resources | Initial creation can fit. Restoration depends on the separate release policy and attachment lifetime. Unloaded policy is not being redesigned here. |

These are responsibilities, not a prescription for eight new events. One handler can serve several rows, and
some rows may share a future signal. ItemsRepeater's Loaded/Unloaded counters are compatibility machinery,
not an additional user-facing scenario.

#### Proposed initialization API: scope, dependencies, and naming

The candidate public shape is **FrameworkElement.IsInitialized** plus an **InitializedChanged** event.
These are hypothetical APIs, not existing WinUI members or a finalized design.
The bool would be publicly read-only; individual controls would report their own readiness only after
their required initialization work and dependencies are complete. How controls report that state is not
specified here.

**FrameworkElement is the natural API owner, rather than Control.** Core template application already
lives at FrameworkElement. The public ApplyTemplate/Template/GetTemplateChild surface belongs to Control,
but ContentPresenter and ItemsPresenter derive directly from FrameworkElement and have specialized
template-application implementations. Non-templated elements can also have initialization work.
[Core application machinery][apply-template-order], [FrameworkElement hook declaration][initialization-api-base],
[Control public ApplyTemplate][control-public-apply-template],
[ContentPresenter inheritance][content-presenter-inheritance], [ItemsPresenter inheritance][items-presenter-inheritance].

Candidate state semantics:

* True means required local/subtree initialization is complete, including relevant setup performed by
  dependency initialization handlers. Merely constructing template objects does not establish that.
* Template replacement or replacement/invalidation of a required dependency can make readiness false
  again. Detachment/re-entry alone does not reset it; attachment is a separate mechanism.
* Change notifications are synchronous so dependent setup can finish before presentation/input.
  Consumers subscribe, then examine current readiness, accounting for dependencies already initialized.
* Each control is responsible for waiting for the descendants it actually needs. Readiness does not
  automatically require every visual descendant, including unrelated or unrealized collapsed content.

**Candidate cycle-prevention rule:** hard IsInitialized prerequisites are restricted to strict visual
descendants. Every hard edge then points downward in an acyclic tree; self, ancestor, and external hard
prerequisites are excluded. Reparenting/template changes require reevaluating those relationships.
The scope of owned popup/detached content remains to be defined: a strict visual-subtree rule excludes it.
With only independently managed bools and event subscriptions this is a convention; framework-enforced
cycle prevention would require explicit dependency registration/validation.

External references can instead be **soft readiness notifications**: attempt setup immediately and
subscribe to the external element's initialization changes to retry, without withholding the consumer's
own IsInitialized. This is the proposed treatment of ParallaxView.Source.

Today, [ScrollInputHelper first searches the Source's visual subtree][parallax-source-discovery] for a
ScrollViewer/ScrollPresenter. If discovery fails, it subscribes to Source.Loaded and retries on delivery;
it does not follow Source references recursively. Two ParallaxViews referencing each other therefore
do not prevent each other's Loaded notifications. Each can discover the other's actual scroll subtree,
if one exists, without waiting for the other's parallax hookup to finish.
Requiring both to wait for the other's complete IsInitialized would introduce a readiness deadlock.

While discovery is pending, ParallaxView still measures/arranges its child and does not disable, hide,
or suppress child input. If the scroll source is already discoverable, setup can run before Source.Loaded.
Otherwise the parallax effect waits: the source property set remains null, and prior parallax animations,
if any, are stopped with translation reset.
[Source availability][parallax-source-discovery], [animation fallback][parallax-no-source].
Thus the proposed initialization scope is not "every externally configured feature is operational."

**Naming conclusion: prefer IsInitialized over IsInteractable.** Dependency-complete setup covers input
behavior, appearance, layout, and accessibility. An initialized element can still be collapsed, disabled,
detached, or noninteractive by design. Conversely, some ordinary interaction remains possible before a
particular setup step finishes. "Interactable" would imply a different capability/state contract.

The other audited initialization consumers illustrate that breadth:

| Consumer and dependency | Setup once the dependency is available |
| --- | --- |
| [AutoSuggestBox -> TextBox][autosuggest-query-initialization] | Find QueryButton; attach its submission Click handler; configure icon and accessibility name. |
| [CommandBar -> overflow presenter][commandbar-part-loaded] | Find ItemsPresenter and intercept arrow keys to move focus instead of scrolling. The same handler also performs pending presentation-specific focus work. |
| [MediaTransportControls -> CommandBar][media-commandbar-loaded] | Find/configure internal buttons, hide inappropriate ones, and remeasure the command bar. |
| [TabView -> ListView -> ScrollViewer][tabview-child-initialization] | Initialize items/selection; discover presenters and scroll buttons; attach scrolling handlers and localized tooltips. |
| [TwoPaneView -> ScrollViewer][two-pane-loaded] | Find ScrollContentPresenter and configure its content-sizing behavior. |
| [NumberBox -> TextBox][numberbox-loaded] | Apply the child's visual state corresponding to spin-button placement. |
| [BreadcrumbBarItem -> Button][breadcrumb-item-loaded] | Attach Click behavior and configure ellipsis/last-item appearance. |
| [Refresh adapter -> ScrollViewer][refresh-adapter-loaded] | Obtain the template-generated content parent and create the composition interaction source used for pull-to-refresh. |

These are current Loaded consumers, not proof that their handlers all have identical hard prerequisites
or should be moved wholesale. For example, missing ASB setup breaks query-button submission, whereas
missing TwoPaneView setup affects sizing and missing NumberBox setup affects visual states.

Bucket 8 is principally resource-lifetime optimization driven by leave/entry, not another readiness
mechanism. AnimatedVisualPlayer's [unload path][animated-player-release] stops playback, removes
composition children, closes/releases the animated visual, and clears exposed state. Re-entry recreates
content from Source. This is destroy/recreate, not necessarily reconnecting the same objects; moving its
timing must deliberately preserve playback-completion, property-change, and source-callback semantics.

#### AutoSuggestBox: confirmed false-positive readiness

AutoSuggestBox's own template application obtains its TextBox, then waits for **TextBox.Loaded** to discover
the QueryButton inside the TextBox template and attach its Click handler. If that TextBox is collapsed before
its first measure, it can receive Loaded with no template visuals. The lookup finds no QueryButton. Revealing
the TextBox later creates the button but does not raise Loaded again, so the hookup remains missing.

The focused [native reproduction][autosuggest-readiness-test] confirmed this sequence on the local VM:

* Initially: TextBox Loading count 0, Loaded count 1, no visual children.
* After reveal: Loading count 1, Loaded still 1; QueryButton exists, but its query icon is missing.
* An injected mouse click reaches QueryButton.Click, but AutoSuggestBox.QuerySubmitted remains at 0.
* Removing/reinserting the same AutoSuggestBox raises another Loaded, configures the icon, and restores submission.

The test ran against the local amd64chk build on `ge-dev1-261003-Desktop`. It fails the intended
correct-behavior assertion (expected one submission after reveal, actual zero); this is a confirmed defect
reproduction, not a passing regression test or a product fix.

Collapsing only QueryButton is different: its object can already exist inside the applied TextBox template,
and attaching Click does not require the button's own template. `Loading` is not a replacement for the
TextBox dependency because it runs before template application. A post-template notification on TextBox
could support this particular lookup, but would not itself guarantee recursively initialized descendants.

#### Layout-output consumers: initial calculations and change coverage

This source inspection follows the four continuing-calculation examples in the layout-output bucket:
AutoSuggestBox, Popup, ScrollViewer, and Pivot. It does not remove Loaded handlers or claim experimentally
verified equivalence of a replacement.

**Timing correction:** SizeChanged and LayoutUpdated are not ordinary dispatcher-queued callbacks.
They can be deferred relative to a property assignment that invalidates layout, but the layout manager
[drains SizeChanged and invokes LayoutUpdated synchronously inside layout processing][layout-event-delivery],
before the frame is rendered. If those handlers dirty layout, the layout loop continues.
[LayoutUpdated dispatch visits all registered event sources][layout-updated-dispatch], rather than notifying
only the element whose bounds changed. There is therefore no inherent extra dispatcher-turn delay that
requires Loaded for these calculations.

| Consumer | Initial work and ongoing update path | Coverage and remaining Loaded responsibility |
| --- | --- | --- |
| AutoSuggestBox | SuggestionsContainer.Loaded updates suggestion position and size; Popup.Opened updates position. Item changes schedule a batched position/size update; candidate-window and input-pane paths also update placement. Its own SizeChanged updates suggestion width only. | **Incomplete invalidation.** The calculations consume root/monitor bounds, anchor position and height, and suggestion height/margins, but there is no general position/size refresh for changes to those inputs. Loaded is an initial calculation, not coverage for subsequent changes. |
| Popup using PlacementTarget and non-Auto DesiredPlacement | Opening subscribes to target.LayoutUpdated and child events, then immediately calculates placement. Target/placement/child replacement also recalculates. LayoutUpdated, child.SizeChanged on the open-time hookup, and child.Loaded call the same placement method. | **Ordinary layout-driven changes have a continuing update path**, including same-size target movement and child-size changes, because LayoutUpdated is global. Child.Loaded is another calculation, not the sole post-layout trigger. This is not a guarantee of tracking compositor-only motion or external display changes that do not cause layout. |
| ScrollViewer | IScrollInfo invalidation pushes offset/extent/viewport changes into DirectManipulation synchronously. Setting a manipulation handler immediately catches up its transform; live-tree entry also refreshes it. | **Ongoing numeric changes are handled, but setup ordering has an explicit retry.** Loaded forces another full transform synchronization because an earlier attempt may encounter a zero pixel viewport. Finding the numeric change paths does not establish that this retry can be deleted. |
| Pivot parallax | Header measurement/arrangement and the state machine update curve inputs and secondary-content relationships. Header/item changes invalidate layout; the input manager queues relationships when a manipulatable element or viewport is not ready and retries after viewport configuration. | **There is a continuing layout path and native readiness recovery.** ScrollViewer.Loaded requests another header arrange as an additional synchronization workaround. It does not itself calculate parallax immediately. No missing ordinary layout-input notification was established; necessity of this additional retry was not proven. |

**AutoSuggestBox's gaps are narrower and more concrete than "it needs layout."**
[UpdateSuggestionListPosition][autosuggest-position] reads the TextBox/internal ScrollViewer's location and
height, suggestion margin, and (for above placement) suggestion-container height.
[MaximizeSuggestionAreaWithoutInputPane][autosuggest-available-space] also reads available root/monitor
bounds and chooses above/below placement and available height.
[UpdateSuggestionListSize][autosuggest-size] copies the resulting height limit and the control's width.

However, [its SizeChanged handler][autosuggest-width-update] only copies width. The template-part hookups
do not subscribe to the TextBox/internal ScrollViewer's SizeChanged or the suggestion container's
SizeChanged. Its [LayoutUpdated handler][autosuggest-overlay-layout] exists only while the light-dismiss
overlay is active and only repositions layout-transition elements; it does not refresh these suggestion
calculations. Consequently, changes to the internal anchor's size/position or suggestion height without
a collection change have no general recalculation trigger. The same applies to available root height
when host dismissal policy leaves the popup open; root-size dismissal can instead end that presentation.
An unrelated later item/input-pane/candidate-window/opening notification can mask the missing
invalidation. These are source-identified coverage gaps, not additional reproduced visual defects.

The [default AutoSuggestBox template][autosuggest-popup-template] does not set Popup.PlacementTarget or
DesiredPlacement, so Popup's automatic-placement calculation is not an alternate implementation of
AutoSuggestBox's above/below and MaxHeight decisions.
[Item-change updates really are dispatcher-deferred][autosuggest-deferred-layout]: the source explicitly
avoids forcing layout before the inner ListView has processed the same collection notification.

**Popup's bounded result:** [opening follows subscribe-then-calculate][popup-placement-subscriptions].
The [placement calculation][popup-placement-calculation] consumes child size, target global bounds,
available window bounds, and flow direction; it skips placement while either child dimension is zero.
Its global layout subscription provides a catch-up after layout, including changes that do not raise
target.SizeChanged. The child-replacement branch currently attaches child.SizeChanged to inherited
FrameworkElement.OnSizeChanged rather than OnChildSizeChanged; that handler does not itself recalculate
placement. The global LayoutUpdated path still recalculates for the automatic-placement case, so this
wiring discrepancy alone does not prove a visible placement failure or a need for child.Loaded.

**ScrollViewer's ordering dependency:** ScrollContentPresenter
[VerifyScrollData][presenter-scroll-invalidation] directly calls its owner's InvalidateScrollInfoImpl when
viewport, extent, or coerced offsets differ. The owner
[updates viewport/configuration/alignment and transform state][scrollviewer-layout-invalidation] internally;
deferred public ViewChanged delivery is not the mechanism used to synchronize DirectManipulation.
[Installing the manipulation handler][scrollviewer-handler-catchup] also forces an immediate catch-up.
For logical-scrolling providers, [pixel viewport dimensions can come from provider ActualWidth/Height][scrollviewer-pixel-viewport],
not just IScrollInfo's advertised viewport. The [ZoomToRect path requires positive pixel dimensions][scrollviewer-zero-viewport].
[Loaded explicitly retries synchronization][scrollviewer-loaded] after an earlier zero-pixel-viewport
attempt. A replacement must handle that readiness boundary, rather than assuming an unchanged logical
viewport necessarily produces another notification.

**Pivot's ordering dependency:** [header Arrange calls the manager][pivot-header-layout], which reaches
the state machine and [sets parallax when no parent arrange is pending][pivot-header-state-machine].
[ApplyParallax explicitly requires measured header sizes before compositor submission][pivot-parallax-layout].
[SyncParallax only invalidates header arrange][pivot-parallax-invalidation]; that is what
[ScrollViewer.Loaded invokes][pivot-loaded]. There is also an
[explicit parent-arrange invalidation workaround][pivot-dm-readiness] for header layout preceding
DirectManipulation-container initialization.
[The input manager separately queues and retries relationships][secondary-relationship-readiness] when
native dependencies are absent, including a retry after adding a viewport configuration. These paths
establish ongoing layout updates and readiness recovery, not proof that every overlapping workaround is
necessary or removable.

Deferred scrolling in ListViewBase and Hub is a different shape of work: completing a pending command,
not maintaining a calculated value forever. [ListViewBase.Loaded executes the command selected for that defer point][listview-loaded];
[Hub.Panel.Loaded processes a deferred ScrollToSection request][hub-panel-loaded].
The [Hub StackPanel path sums section DesiredSize][hub-scroll-layout] when executing the command and
[clears the deferred request][hub-deferred-scroll] afterward. Repeating that scroll on every later size
change would impose a new persistent scroll-anchoring behavior, not merely keep a derived calculation current.

**Compatibility conclusion:** initial calculation plus complete dependency/change handling remains a
valid replacement model. AutoSuggestBox does not currently implement complete invalidation; ScrollViewer
and Pivot also need dependency-readiness/order handling, which can be distinct from numeric changes.
The source does not establish an inherent need for a one-time Loaded layout calculation, nor justify
wholesale removal of these existing Loaded callbacks.

#### Own-VSM refreshes: source-inspection results

Five audited MUX base/control types refresh their own visual states on Loaded. The investigation traced their
state inputs, earlier update paths, container preparation, template-binding refreshes, and VSM leave/re-entry
behavior. It did not remove handlers or experimentally test their removal.

| Type | Existing earlier state-update paths | What the inspection establishes |
| --- | --- | --- |
| ButtonBase | [OnApplyTemplate refreshes states][button-apply]; enabled, pressed, pointer and focus paths also refresh them. ChangeVisualState dispatches to the derived control. Command.CanExecute is explicitly refreshed on live entry after reconnecting its subscription. | Loaded is an additional transition-free replay, not the only initialization update. No ButtonBase-specific missed-update sequence was established that proves this replay necessary. Its inherited reach means inspecting Button alone cannot prove it redundant for all derived controls/templates. |
| ListBox | Enabled and focus changes refresh its CommonStates/FocusStates. Its inherited [Selector.OnApplyTemplate][selector-apply] and Control.OnApplyTemplate do not select those states. [Loaded performs that selection][listbox-loaded]. | There is a concrete initial-state responsibility: a state request before template creation can find no implementation root, and template application alone does not retry it. A correct initialization callback could supply that retry. The [current default ListBox template][listbox-default-template] has no own VSM groups, so this is not evidence of a visible defect in that template; custom/legacy templates with those groups are relevant. |
| ListBoxItem | [OnApplyTemplate selects states][listboxitem-apply]. [Selector container preparation][selector-prepare] sets its parent, reconciles selection, then explicitly refreshes states. Enabled, focus, pointer, visibility and parent selection-active paths also refresh them. | The early return when no parent Selector exists is not by itself proof that Loaded is required: both template-after-preparation and preparation-after-template have refresh paths. No unique remaining missed-update sequence was established. |
| ComboBoxItem | [OnApplyTemplate][comboboxitem-apply] explicitly applies its ContentPresenter's template before selecting its own states. Selector preparation also refreshes it; selected-state override methods refresh immediately. | A descendant-template ordering hazard is explicitly documented and mitigated here: later ContentPresenter initialization can reset VSM-written template-bound properties and change input-mode sizing. That proves a recursive initialization dependency, not that this additional Loaded replay is necessary. Its states also read the parent's current dropdown/input mode, so a replacement must not silently discard repeated-presentation behavior. |
| ListViewBaseItem | [OnApplyTemplate][listviewitem-apply] connects chrome and selects states. [Container preparation][listview-prepare] clears interaction state and batches a final refresh. Parent selection-mode/checkbox changes explicitly update visible and cached items; local enabled/focus/input paths also update states. | Ordinary preparation/recycling is not a demonstrated missing-refresh gap. Loaded supplies another replay of local and parent-dependent state; no unique remaining sequence was established that proves that replay necessary. |

Shared mechanisms explain why merely finding an earlier `UpdateVisualState` is insufficient proof of redundancy:

* [InvokeApplyTemplate][apply-template-order] refreshes the parent's template bindings, invokes OnApplyTemplate,
  then refreshes newly realized bindings. It has no unconditional final call to the control's ChangeVisualState.
  State triggers are initialized there, but do not replace control code selecting named states.
* A [ContentPresenter can later add implicit bindings][presenter-refresh] and call its templated parent's
  `RefreshTemplateBindings(All)`. An [ItemsPresenter has a related refresh path][items-presenter-refresh].
  The [ComboBoxItem comment and workaround][comboboxitem-apply] document the VSM overwrite consequence.
  This is concrete initialization-ordering evidence, but does not establish that every default template of
  the other controls experiences that overwrite or relies on Loaded to repair it.
* [GoToState with no implementation root does nothing][vsm-no-root]. Conversely,
  [requesting the same state with transitions disabled][vsm-same-state] is not necessarily a no-op:
  it snaps storyboards to their end and reevaluates applied setters, including binding expressions.
  Inspecting only current state names would miss these effects.
* [StateChangeSuspender resumes with a refresh][state-change-suspender]. Suspension alone is therefore not
  evidence of an update that only Loaded can replay.
* The [VSM group-collection leave path][vsm-leave] snaps active groups to their end rather than unconditionally
  resetting their current state to null. This does not prove that every effective property survives every
  detach/re-entry path, but it does not establish a blanket "all states must be selected again on entry" rule.

**Classification:** ListBox has a demonstrated initial-state-selection role (template-dependent);
ComboBoxItem has a demonstrated descendant-initialization ordering hazard already handled by OnApplyTemplate.
The other three remain additional state-replay consumers whose necessity was not proven by this inspection.
None of these findings establishes a new, independent Loaded scenario beyond initialization and
state/presentation resynchronization, or proves that any of the five Loaded handlers can simply be deleted.
Do not count all five as additional scenarios definitively eliminated by the proposed initialization signal.

### Unloaded scenario breakdown (2026-11-23)

This groups the audited MUX, MUXC, and adjacent Tabular consumers by the work they perform, rather than
by control type. A handler can serve several buckets. These are observed responsibilities, not a
prescription for eight separate APIs or proof that every current subscription is necessary.

Unloaded is not simply the inverse of initialized. The element can remain alive and retain its template
and local initialization after detachment. The [core schedules Unloaded on live Leave][unloaded-raise];
delivery is deferred as described at the beginning of this note. Calling a cancellation or completion
function synchronously inside the handler does not make the notification synchronous with tree removal.
The current core also skips scheduling this event when shutting down, so it is not a guaranteed
object-destruction/process-exit notification.

| Bucket | Observed responsibility | Examples and work performed | Important distinction |
| --- | --- | --- | --- |
| 1 | Release/detach visual and host resources | [AnimatedVisualPlayer][animated-player-release] stops playback, closes/releases its animated visual, and clears exposed state. [SystemBackdropElement][backdrop-resource-release] disconnects its backdrop target and drops composition resources. [WebView2][webview-lifecycle] disconnects its root visual target; [InkCanvas][ink-unloaded] detaches its visual link. | Resource/attachment lifetime can end without the control being destroyed or becoming uninitialized. Teardown can invoke external callbacks and change observable state, not just save memory. |
| 2 | End external subscriptions, registrations, and ownership | [ItemsPresenter][items-presenter-detach] removes its ScrollViewer subscription; [NavigationView][navigation-lifecycle] revokes title-bar subscriptions; [RadioMenuFlyoutItem][radio-menu-lifecycle] removes checked-group membership. [ModernCollectionBasePanel][panel-detach] drops scroller/presenter relationships and sticky headers. [TableView][table-unloaded] releases owning-table access to its source. | This ends participation in a relationship. It can prevent off-tree callbacks or allow another consumer to acquire ownership; it is not exclusively a performance optimization. |
| 3 | Stop pending asynchronous/deferred work and callbacks | [ColorPicker][colorpicker-unloaded], [ColorSpectrum][spectrum-unloaded], and [PersonPicture][person-picture-unloaded] request cancellation of image work. [ScrollPresenter][scroll-presenter-unload] completes pending view-change operations as Interrupted and removes its rendering callback. [ItemsView][itemsview-unload] clears queued keyboard navigation/bring-into-view work; [ScrollView][scrollview-unloaded] clears work and resets its timer. | Pending requests are canceled, interrupted, or discarded; they are not necessarily paused for automatic continuation on entry. Completing a request can notify callers. |
| 4 | End active interactions and reset transient interaction/animation state | [ResizeGripper][resize-unloaded] cancels an active drag, releases capture, and raises DragCompleted with cancellation. [ScrollViewer][scrollviewer-unload-state] clears pointer-pressed and indicator state. [Pivot][pivot-unloaded] completes animations so later callbacks do not modify detached-control state. TableView cancels column resizing and pending focus restoration. | This prevents an interaction from remaining stuck when its usual completion/input event will no longer arrive. It is distinct from merely releasing an animation or visual resource. |
| 5 | Dismiss transient UI when its owner/target leaves | [ComboBox][combobox-unloaded] closes its dropdown; [CommandBar][commandbar-unloaded] closes overflow and restores compact mode where appropriate; [TeachingTip][teaching-tip-unloaded] closes when it or its target unloads; [FrameworkElement][tooltip-unloaded] cancels an automatic tooltip. | The departure initiates dismissal of another presentation. Open-state changes and closure behavior are observable, even if the original owner will be reused. |
| 6 | Observe presentation teardown, finish a close operation, and advance lifecycle state | [ContentDialog popup-child Unloaded][dialog-unloaded] reaches close completion, Closed, and completion of ShowAsync. [FlyoutBase presenter Unloaded][flyout-unloaded] processes closure and can open a staged flyout. [ApplicationBarService][appbar-service-unload] removes an unloading appbar from its pending set and reevaluates whether its containing popup can close. | This is a completion/orchestration boundary, not just cleanup. The listened-to child/host leaving can complete an operation on another object. |
| 7 | Undo temporary property/policy overrides | ContentDialog's [close-completion path][dialog-close-restoration] restores temporarily disabled hit testing and window dragging. [TableViewRow][table-row-unload-restore] restores authored cell-content tab stops if unload precedes the scheduled restore turn. [TableViewCellAutomationPeer][table-peer-unloaded] drops its temporarily pinned pre-edit accessibility name. | Do not strand reused objects or callers with a temporary interaction/accessibility override after its normal restoration path stops running. This is restoration of state, not disposal of the object. |
| 8 | Record/flush session diagnostics and telemetry | [InkToolbar][ink-toolbar-unloaded] and [InkCanvas][ink-unloaded] use Unloaded to report session summaries, with destructor backstops/deduplication. [TableView][table-unloaded] samples telemetry and ends a session. | This treats departure as a session boundary. It neither requires template readiness nor establishes that Unloaded is guaranteed during process shutdown. |

#### Two distinct kinds of closure work

Buckets 5 and 6 deliberately separate **initiating dismissal** from **observing teardown and finishing
an operation**. ComboBox's OnUnloaded sets IsDropDownOpen false. In contrast, a popup-hosted ContentDialog
attaches to its popup child's Unloaded during closing; the handler either finishes an in-progress hide
or routes unexpected popup removal through HideInternal.

The [ContentDialog completion path][dialog-close-restoration] raises Closed, disconnects the old button
deferral manager, restores temporary policies, clears the current async-operation pointer, and fires
completion. Clearing that pointer allows a completion handler to show the dialog again.
The in-place dialog path uses other state notifications; the dependency described here is specifically
the popup-hosted path.

FlyoutBase's presenter handler calls OnClosed, clears open-flyout metadata, and opens a staged flyout
when its root context still exists. ApplicationBarService deliberately attaches an Unloaded listener
when removing an appbar; its [popup-state decision][appbar-popup-lifetime] keeps the popup open while
any appbars remain in the unloading set. The source uses Unloaded as a presentation-lifetime boundary;
this does not make Unloaded a general-purpose animation-completed event.

#### Cancellation and interaction termination are observable

For ScrollPresenter, pending view-change requests receive an Interrupted result rather than waiting
indefinitely or being silently resumed after re-entry. ResizeGripper's
[EndDrag][resize-drag-completion] clears IsDragging, updates visual state, and invokes DragCompleted
with the cancellation flag. Its unload comment specifically explains that removing the control mid-gesture
can prevent the normal manipulation-completion event from arriving.

AnimatedVisualPlayer illustrates bucket overlap: its unload both releases resources (bucket 1) and
completes playback/changes properties (buckets 3-4). TableView's unload spans subscriptions, ownership,
work cancellation, interaction cleanup, temporary overrides, cache/resource release, and telemetry.
These side effects must not be treated as interchangeable "free some memory" actions.

#### Event accounting is compatibility machinery, not another scenario

[ItemsRepeater][repeater-lifecycle] increments Loaded/Unloaded counters and only resets scrollers/cache
when the counts indicate an in-sync unload. Hub's panel-loaded flag and ScrollViewer's loaded flag also
record lifecycle state for later behavior. Counters and flags explain how controls cope with delivery
anomalies; they do not themselves establish a ninth independent user-facing responsibility.

Current defenses differ:

* AnimatedVisualPlayer, WebView2, InkCanvas, SystemBackdropElement, ScrollPresenter, ItemsView,
  ColorSpectrum, ResizeGripper, and TableView check current loaded state before destructive unload work.
* ItemsRepeater uses event counts instead.
* ItemsPresenter reconciles/removes an old subscription on Loaded because Unloaded may be missing.
* Other handlers, including ComboBox, NavigationView, RadioMenuFlyoutItem, ColorPicker, PersonPicture,
  and TableViewRow's restore callback, do not use that same current-state guard.
* Some closure listeners are intentionally attached only when removal/closing starts, rather than
  paired with a preceding Loaded subscription.

#### Starting points for evaluating a leave mechanism

Most buckets describe work triggered by departure, with different release/cancellation/restoration
policies; they do not by themselves require different leave events. Bucket 6 warrants separate scrutiny
because consumers use a departing child's notification to finish an existing presentation/operation.

The key compatibility questions are:

1. Does the handler concern the element's **current attachment**, an **old attachment/presentation**,
   a pending **operation**, or a temporary **override**?
2. Must it still execute after rapid re-entry, or would executing it then tear down newly established state?
3. Does moving it earlier change access to the old host, relationships, visuals, or teardown state, or introduce
   app callbacks during the framework's Leave traversal?
4. Does suppression/coalescing lose a cancellation/completion signal or ownership handoff?

These questions remain open for individual consumers. The breakdown does not establish that invoking
all existing handlers inline inside Leave is safe, or that one blanket IsLoaded/pairing rule preserves
every responsibility.

#### Correspondence with the original eight Loaded buckets

The numbers in this table refer to the original Loaded scenario table, not the later consolidation
into initialized/entered. Correspondence is not a requirement for symmetric handlers.

| Unloaded bucket | Closest Loaded bucket(s) | Relationship |
| --- | --- | --- |
| U1: release visual/host resources | L8: restore released resources; L2: root/host attachment | Direct release/restore or disconnect/reconnect pair. |
| U2: end subscriptions, registrations, ownership | L2: root/host attachment; L3: structural ownership | End the relationship that entry establishes/reconciles. |
| U3: stop pending work | L6: restart deferred tree-dependent work | Related execution-lifetime policy, but cancellation/interruption is not necessarily a pause that Loaded reverses. |
| U4: terminate interactions/reset transient state | L5: reset interaction state on reuse | Cleanup at exit versus reset at entry. Neither requires that the other event was observed. |
| U5: dismiss transient UI | L4: per-presentation actions | End an owner's presentation-dependent UI; subsequent entry does not necessarily reopen it. |
| U6: finish closure/advance lifecycle | L4: per-presentation actions, including Opened | The closing/completion side of presentation orchestration. Can also start another presentation. |
| U7: restore temporary overrides | L4/L5: presentation/interaction policy; no exact inverse | Undo temporary state, sometimes set during closing rather than during Loaded. Not "uninitialize." |
| U8: flush diagnostics/telemetry | No distinct Loaded bucket | Session bookkeeping can start at entry or first use, but was not an independent Loaded scenario. |

L1 (dependency-complete initialization) has no general inverse in Unloaded: removal does not erase
templates and local readiness. L7 (layout-output calculation) has no mandatory cleanup inverse either;
invalidating scroller/layout caches is work associated with ending those relationships/resources.

#### Compatibility of inline versus tick delivery

This analysis distinguishes two changes that are sometimes both called "synchronous Unloaded":

* **A: inline during Leave.** The handler runs before the removal/close call stack has unwound.
  At the existing CFrameworkElement raise site it also runs before the element is deactivated.
  Moving it later within Leave fixes some state checks but not the surrounding mutation/caller ordering.
* **B: synchronous dispatch at a safe tick checkpoint, like Loaded.** The removal/close call has
  returned before delivery; only the notification scheduling moves from regular deferred dispatch to tick.
  This assumes final-removal/transition gates remain intact, queued notifications/recipients are retained,
  and a tick is requested. Before/after Loaded ordering and handling new events during that checkpoint
  are separate policy choices, not specified by the word "tick."

The findings below are static call-order consequences and identified risks, not executed reproductions
of a modified event implementation. A bucket can contain both a straightforward cleanup and a
timing-sensitive consumer.

| Unloaded bucket | A: inline compatibility | B: tick compatibility |
| --- | --- | --- |
| U1: resource release | At the current raise site, current-state guards skip genuine teardown. After fixing that, destruction/property/source callbacks can still reenter an active removal stack. | No intrinsic need for regular dispatcher delivery established for an isolated completed departure. Re-entry and old/new resource identity must be handled. |
| U2: relationships/ownership | Guarded consumers can skip cleanup; ownership release and callback revocation occur earlier. TableView source release synchronously runs other tables' waiters, so it is not an inert pointer clear. | Ordinary detach work fits; stale events must not remove a new subscription/registration. ItemsRepeater has an additional post-layout ordering trap described below. |
| U3: pending work | Guarded cancellation can be skipped. Completion/cancellation callbacks now occur inside removal; even after deactivation, ScrollPresenter's expected post-removal layout state may not exist yet. | Ordinary cancellation after actual removal fits. Results/completion ordering relative to Loaded, rendering, and other queued work changes. |
| U4: interaction termination | ResizeGripper's guard skips EndDrag at the current site; moving it later permits DragCompleted and other app callbacks during tree mutation. | No inherent regular-dispatcher dependency established. Pending re-entry must not cause cancellation/reset of a new interaction. |
| U5: initiate dismissal | Popup closures become nested inside owner removal; their internal U6 consumers inherit the concrete failures below. An independent failure of every dismissing control was not established. | Fits post-removal processing, provided closures/newly queued lifecycle events are processed consistently. |
| U6: finish presentation | Concrete source-level failures: ContentDialog can clear Popup.Child inside Popup.Close; flyout replacement can receive completion before it has staged its successor. | Those particular caller-stack failures do not follow from a later safe tick. Completion must still be delivered after final removal, not suppressed just because a prior Loaded was absent. |
| U7: restore overrides | TableViewRow's restoration has no demonstrated requirement for delay. ContentDialog's restoration lives in the failing close-completion path; property/accessibility callbacks can reenter removal. | No independent late-dispatch requirement established. Old restoration must not overwrite a newer presentation's temporary policy. |
| U8: telemetry | Unguarded flushes run earlier; guarded InkCanvas/TableView cleanup can be skipped at the current site. No intrinsic need for a dispatcher turn established. | Generally compatible as a session-boundary policy; session timestamps/order can change. Shutdown delivery is still not guaranteed. |

**A1. Current raise-site guards skip real cleanup.** CFrameworkElement queues Unloaded before calling
CUIElement::LeaveImpl; CDependencyObject deactivates the element later. An ordinary previously loaded
element therefore still reports IsLoaded true at that point.
[Raise/deactivation][unloaded-raise], [IsLoaded computation][isloaded-getter].
Changing only the dispatch flag causes guarded teardown in AnimatedVisualPlayer, WebView2, InkCanvas,
SystemBackdropElement, ScrollPresenter, ItemsView, ColorSpectrum, ResizeGripper, and TableView to skip.
That is a source-demonstrable behavior failure, not merely a potential reentrancy issue.

Nor is deactivation necessarily the final safe boundary:
[collection removal calls ChildLeave, clears the parent, then performs removal cleanup][collection-remove-order].
[UIElementCollection removal cleanup calls Shutdown][collection-removal-shutdown], which
[resets layout storage][element-shutdown-layout].
[ScrollPresenter's handler asserts RenderSize is zero][scroll-presenter-unload], whereas
[RenderSize reads existing layout storage][render-size-storage].
For an ordinarily measured collection child, dispatch just after deactivation but still inside Leave
can violate those assertions before Shutdown has run.

**A2. ContentDialog has a concrete Popup.Close reentrancy conflict.** In a popup-hosted hide with no
transition/keep-visible delay, the call chain would become:

```text
ContentDialog hide -> popup.IsOpen = false -> CPopup::Close
  -> PopupRoot.RemoveChild -> live Leave -> inline Unloaded
    -> ContentDialog.OnPopupChildUnloaded -> OnFinishedClosing
      -> popup.Child = null
```

But [CPopup::Close holds m_isClosing true across RemoveChild][popup-close-mutation], and
[CPopup::SetChild explicitly fail-fasts on m_isClosing && !pChild][popup-child-close-guard].
ContentDialog's [completion path clears Popup.Child][dialog-close-restoration] even without any
application Closed handler. Simply moving Unloaded after element deactivation inside the same call
stack does not remove this conflict. The safe-tick alternative lets Close unwind first.

**A3. Flyout staging assumes Hide returns before Unloaded completion.**
The [replacement path][flyout-stage-after-hide] does this:

```text
oldFlyout.Hide();
metadata.SetStagedFlyout(newFlyout, target);
```

With inline Unloaded and immediate removal, oldFlyout.OnPresenterUnloaded runs inside Hide, sees no
staged successor, and clears open-flyout metadata. When Hide returns, staging occurs too late:
[SetStagedFlyout asserts there is an open flyout][flyout-stage-invariant], and the setter does not open
anything by itself. There is no remaining old-presenter unload to consume that newly staged request.
This is a concrete ordering failure when closing does not defer Leave for animations/keep-alive.
A later tick retains the required return-before-completion order.

Staging before Hide repairs that missing-stage dependency, but same-instance reopening has a
separate inline-delivery conflict. The replacement path also handles reopening the same flyout at
another target/position. Its presenter Unloaded can then call Open on the same popup while the old
CPopup::Close is still removing the presenter. Close removes the popup from the open-popup list only
after RemoveChild returns, whereas Open calls
[StartAdditionToOpenPopupList, which asserts the popup is not already in that list][popup-open-list-invariant].
This establishes an additional checked-build invariant conflict for immediate inline handoff, even
after staging is reordered. It requires a later completion boundary or changes to popup lifecycle
ordering, not necessarily ordinary dispatcher delivery.

**Transition timing is not the same as notification-queue timing.**
[UIElementCollection unloading storage][unloading-storage-gate] retains elements for theme transitions,
connected animations, implicit hide animations, and keep-alive requirements. It defers actual removal/
Leave until the reasons are released; [RemoveUnloadedElement performs final cleanup][unloading-final-removal].
Therefore, neither inline-at-final-Leave nor tick dispatch inherently means firing at the initial
Remove request before the animation finishes. Moving the event ahead of this gate would be a different
change and could prematurely close ApplicationBarService's popup. Keeping the gate avoids that specific
prematurity, though inline final cleanup still has the reentrancy problems above.

**B1. Tick order and pending Loaded state still matter.**
For a completed departure with no re-entry, the ordinary U1-U5/U7-U8 actions do not show an intrinsic
requirement for regular deferred dispatch rather than a safe tick. Bucket U6 also need not inherently
use that queue if the close/removal stack and transition gate have finished. This is not a proof of
equivalence for every consumer or application handler.

A remove/re-add before tick is harder. If Unloaded runs before the new Loaded, IsLoaded is false even
though the element is active, because Loaded remains pending. Guarded handlers can tear down the new
attachment, and ScrollPresenter's zero-size assertion can fail after that tick's layout has sized it.
If Unloaded instead runs after the new Loaded, guarded handlers normally skip it, but unguarded handlers
such as NavigationView/RadioMenuFlyoutItem can revoke/remove the newly established state. These are
existing stale-event weaknesses; a fixed tick ordering can expose them predictably rather than
automatically curing them.

There is a specific post-layout counter trap in [ItemsRepeater][repeater-lifecycle]. Start with L=1,
U=0, remove/re-add, and run layout for the new attachment. If tick dispatches old Unloaded before new
Loaded, Unloaded makes U=1 and resets the newly established scrollers. Loaded then sees L==U, does not
invalidate measure, and advances L to 2. [ResetScrollers revokes viewport/scroller handlers][repeater-reset-scrollers];
[EnsureScroller normally runs during measure][repeater-ensure-scroller]. There is no repair requested
by that Loaded path, so reconnection waits for another measure trigger. This is an identified
subscription-loss path absent an additional measure invalidation; a visible scrolling failure has not
been reproduced. Delivering Unloaded before the new attachment's layout would avoid this particular
post-layout reset, which is why the precise tick checkpoint matters.

**B2. Preserve occurrence and process new lifecycle work before rendering.**
Suppressing/coalescing events changes more than their scheduling: ItemsRepeater's counts and U6
completion listeners must be considered separately. A queued Unloaded must retain its eligible
recipients despite Leave disabling live event registrations; blindly reusing Loaded's live-element
delivery rules is not sufficient.

Unloaded handlers can open a staged flyout or add/reinsert other content. If the tick checkpoint runs
after its only Loaded pass, those new Loaded events can otherwise be deferred until a later frame.
The [existing tick explicitly relayouts after Loaded][loaded-dispatch]; integrating Unloaded requires
an explicit policy for new events and dirty layout before rendering as well. This is a scheduling
requirement/risk, not an experimentally demonstrated defect of a particular tick implementation.

**Conclusion:** dispatch-flag-only inline Unloaded is incompatible with current consumers. Even
post-deactivation inline delivery retains concrete popup/flyout caller-order failures. A safe tick
checkpoint is a materially different, plausible approach; no bucket has been shown to fundamentally
require the regular dispatcher queue, but its event-ordering, recipient, final-removal, and relayout
policies must be specified. Those policies, rather than a bucket-wide "sync safe/unsafe" label, determine
the remaining compatibility consequences.

#### Delayed-sync reassessment (2026-11-24)

This supersedes the dispatch-flag-only/inline assumptions when evaluating the preferred first step.
For the current additive proposal, apply the findings to Unloaded2/Loaded2: preserve actual
Leave/transition gates, dispatch after the triggering removal stack unwinds, and preserve occurrence
order. Existing out-of-order workarounds need control-side adaptation. Stage-before-Hide is an
independent experiment, not a prerequisite.
No implementation or runtime reproduction was performed for this reassessment.

**What no longer counts as a blocker:** callback execution inside Leave, ContentDialog clearing Child
inside Popup.Close, same-popup reopening while old Close still owns its open-list registration, and
the current-site IsLoaded guards skipping teardown. Those were consequences of inline delivery or
today's stale-event workarounds, not intrinsic delayed-sync requirements. The staging reorder remains
a useful independent latest-request-wins improvement.

**Remaining control adaptations: ordered notifications are not ordered tree mutations.**
With remove/re-add before tick, the element is physically active again before old Unloaded delivery.
At a checkpoint immediately before today's Loaded pass, it can also already have been laid out:

```text
remove -> reinsert -> new attachment's layout/setup -> Unloaded(old) -> Loaded(new)
```

Three source-backed cases need attention even with that correct notification order:

| Consumer | Remaining issue with post-layout Unloaded, then Loaded | Scope of finding |
| --- | --- | --- |
| ItemsRepeater | Starting with L=1/U=0, Unloaded increments U to 1 and resets scrollers/subscriptions after layout. Loaded sees equal counts, does not invalidate measure, then increments L. Reconnection occurs during measure, not simply because Loaded fired. | The existing counter workaround also needs reconsideration, not just IsLoaded guards. No remeasure is requested by this event path; an incidental later invalidation may repair it. |
| ScrollPresenter | Without its IsLoaded guard, Unloaded still asserts RenderSize is zero and explicitly writes viewport width/height zero. A reattached, already arranged presenter can have nonzero RenderSize. Its Loaded handler configures tracker/interaction sources but does not restore the viewport dimensions written during arrange. | Direct checked-build assertion mismatch and state-reset path. A fresh arrange or changed cleanup policy is needed; deleting only the guard is insufficient. |
| ColorSpectrum | Bitmap generation starts from template/property/SizeChanged paths, not a Loaded handler. An old Unloaded can cancel the current action after new setup has started it; the new Loaded has no control-specific restart to repair that cancellation. | Conditional cancellation-without-restart path if work is still pending. This is not a reproduced visible regression, and is related to an existing lifetime weakness rather than unique to the proposed scheduler. |

Sources: [Repeater event counts][repeater-lifecycle], [scroller reset][repeater-reset-scrollers],
[measure hookup][repeater-ensure-scroller]; [ScrollPresenter unload][scroll-presenter-unload],
[Loaded][scroll-presenter-loaded], [arrange viewport update][scroll-presenter-arrange-viewport];
[ColorSpectrum template setup][spectrum-template-setup], [size-triggered generation][spectrum-size-rebuild],
[cancellation][spectrum-unloaded], and [non-successful bitmap completion][spectrum-bitmap-completion].

These are not arguments for preserving out-of-order Unloaded. They identify further migration work:
reconsider obsolete counter logic, ensure re-entry invalidates/reestablishes derived state, or drain
unloads before the relevant setup. A pre-layout checkpoint avoids the ordinary post-layout examples,
but is not a universal cure: explicit ApplyTemplate/UpdateLayout and property-driven work can run
before tick. In particular, "new work" cannot always be identified by whether new Loaded has fired.

**Covered occurrence contract; additional recipient implementation work.**
The handwritten proposal preserves ordered attachment/departure occurrences. Existing
[DisableEvents][event-disable-loaded] removes
pending Loaded on departure. An element can therefore leave before its first Loaded is delivered.
ContentDialog/ApplicationBarService can attach Unloaded specifically for close completion, without a
corresponding earlier Loaded handler. Do not silently drop such Unloaded notifications to force balanced
pairs, or coalesce them solely because the element is active again.

**Concern 4 resolution:** preserve pending Loaded2 across Leave and deliver ordered historical pairs.
This is a required [primary-feature TODO](#primary-feature-todos). Leave continues to cancel pending
legacy Loaded; preserve the new event's occurrences and eligible recipients separately.

The existing IsLoaded guards and ItemsRepeater counters defend against Loaded(new) preceding
Unloaded(old); they do not demonstrate a need for attachment-generation IDs once notifications are
ordered. No concrete consumer requiring generation IDs under the proposed ordered semantics has been
established. Do not add a generation ID now; evaluate captured root/parent context separately against
concrete consumer needs.

A tick implementation must retain eligible notification recipients across Leave. Existing
[handler filtering requires active registrations][event-handler-eligibility], whereas Leave disables
them; delaying the ordinary Raise call until tick and then filtering current handlers would lose events.
Preserve the intended queued notification semantics rather than blindly copying Loaded's filtering.
This is an implementation requirement, not evidence that the dispatcher queue itself is needed.

**Delivery must accommodate work created by handlers.** U5 dismissal can trigger more actual departures,
and U6 completion can open a successor or show another dialog. Process or explicitly schedule the
resulting lifecycle/layout work so a successor does not render before its intended readiness/Loaded
work. The [existing tick][loaded-dispatch] already relayouts after Loaded; adding an Unloaded pass
without defining its relationship to that layout and Loaded processing is incomplete.

**Concern 5 resolution:** drain Loaded2/Unloaded2 identically in one ordered queue, including newly
generated occurrences and post-delivery layout work. This is a required
[primary-feature TODO](#primary-feature-todos).

**Other buckets:** no additional independent failure was established for ordinary completed departure
in resource release (U1), detach/ownership (U2), cancellation (U3), interaction termination (U4),
dismissal (U5), override restoration (U7), or telemetry (U8). U6 flyout and ContentDialog completion fit
the delayed boundary, as does ApplicationBarService when final Leave gates remain intact. Removal of
the stale-event guards is not itself a compatibility failure; the specific rapid-reentry assumptions
above are what still need adjustment. Migrated handlers observe different timing relative to
DispatcherQueue work, input, and async continuations. Legacy Unloaded subscribers keep their existing
delivery path under the additive proposal.

**Bottom line:** no further intrinsic need for ordinary asynchronous Unloaded was found. There is
concrete work beyond removing IsLoaded checks, concentrated in
rapid-reentry state repair and the lifecycle checkpoint's occurrence/order/layout contract.

### Primary-feature TODOs

These are required work for delayed, tick-dispatched Loaded2/Unloaded2, not optional experiments.
All remain pending; this is tracking only, not authorization to implement or run builds/tests.

#### Contract already described; implementation pending

The handwritten proposal already specifies these behaviors. The source-level implementation and
validation details below remain work, including eligible-recipient retention and frame requests.

- [ ] **Add delayed-async Loaded2.** Add the new attachment notification without changing legacy
  Loaded semantics. Record attachment occurrences and deliver them at the safe pre-render tick
  checkpoint through the ordered lifecycle queue. Request a tick when necessary and preserve
  occurrences and eligible recipients across Leave, including historical delivery while detached.
- [ ] **Add delayed-async Unloaded2.** Add the new departure notification without changing legacy
  Unloaded semantics. Queue it at actual Leave, after transition/keep-alive retention permits final
  removal, and deliver at the safe pre-render tick checkpoint after removal stacks unwind.
  Request a tick when necessary and preserve eligible recipients across event disabling.
- [ ] **Preserve pending Loaded2 across Leave and deliver ordered historical pairs (concern 4).**
  Preserve the new event's eligible recipients despite inactive registrations; retaining a queue
  entry alone does not bypass the active-registration filter. Keep legacy Loaded cancellation intact.
  An enter/leave before the checkpoint must produce Loaded2(old) -> Unloaded2(old), and rapid re-entry
  must produce Loaded2(old) -> Unloaded2(old) -> Loaded2(new), without dropping occurrences.
  Loaded2 may arrive while detached; adapt current-state-dependent handlers accordingly.
  Cover same-parent/root re-entry, root migration, handler-driven mutations, and ContentDialog/
  ApplicationBarService completion, with and without transition-delayed Leave. Pairing concerns
  occurrences, not requiring every recipient to subscribe to both events. No generation ID.

#### Additional work not yet described in the handwritten proposal

- [ ] **Drain Loaded2 and Unloaded2 identically in one ordered lifecycle queue (concern 5).**
  Dispatch the next occurrence synchronously, append handler-generated occurrences to the tail,
  and continue until neither kind remains. Neither event gets special priority.
  Preserve post-delivery layout processing and drain its generated lifecycle occurrences before
  completing the pre-render checkpoint. Queue Unloaded2 only at actual Leave, preserving transition/
  keep-alive gates and eligible recipients. This generalizes today's Loaded drain; indefinitely
  generated work is not a new Unloaded-specific concern.
- [ ] **Make IsLoaded usable from Loaded2.** The latest proposal adds Loaded2/Unloaded2 while
  preserving legacy Loaded/Unloaded behavior. IsLoaded currently means
  `IsActive() && !IsLoadedEventPending()` and therefore depends on legacy Loaded delivery.
  For a currently attached element, ensure legacy Loaded fires before Loaded2, or update IsLoaded's
  behavior so pending legacy delivery does not incorrectly defer work from Loaded2 handlers.
  Assess compatibility if changing the property. Cover first attachment, historical Loaded2 while
  detached, and rapid re-entry, including legacy Loaded handlers that mutate the tree. Reactor's
  deferred presentation paths need a reliable current-state check, not an attachment-generation ID.
  Tracking only; no implementation or validation performed.
- [ ] **Migrate controls and adapt their lifecycle policies.** Follow the
  [control adaptation checklist](#control-adaptation-checklist), including ItemsRepeater's reset/
  reconnection policy, ScrollPresenter's viewport/cancellation policy, and ColorSpectrum's generation/
  cancellation policy. Preserve unrelated legitimate live-tree gating.

### Control adaptation checklist

Initial list for migration to ordered, tick-dispatched Loaded2/Unloaded2. All items are pending; this is
the set identified in the audit, not a claim that every lifecycle consumer has been exhaustively checked.
Remove out-of-order-event workarounds only once the framework guarantees the new ordering. Do not remove
unrelated IsLoaded checks that legitimately gate live-tree-dependent work.

| Status | Control | Adaptation to examine |
| --- | --- | --- |
| [ ] | [ItemsRepeater][repeater-lifecycle] | Replace counter-based out-of-order recovery with the ordered lifecycle policy: reset scroller/cache state on Unloaded and ensure Loaded requests the measure needed to reconnect. The reset already exists in today's Loaded recovery path, so this is not inherently extra reset/reconnect work. |
| [ ] | [AnimatedVisualPlayer][animated-player-lifecycle] | Remove the stale-Unloaded IsLoaded guard; retain resource/playback teardown and subsequent content restoration. |
| [ ] | [WebView2][webview-lifecycle] | Adapt the Loaded/Unloaded current-state guards used to ignore out-of-order notifications; preserve host-subscription and visual-target disconnect/reconnect work. |
| [ ] | [InkCanvas][ink-unloaded] | Adapt the historical Loaded/Unloaded timing workaround; preserve subscription/visual-link cleanup and telemetry semantics. |
| [ ] | [SystemBackdropElement][backdrop-unloaded-guard] | Remove the stale-Unloaded IsLoaded guard around composition-resource release; preserve connection restoration. |
| [ ] | [ScrollPresenter][scroll-presenter-unload] | Remove the stale-event guard and update assumptions about zero RenderSize/viewport during delayed unload after re-entry. Ensure entry/layout restores derived viewport state, not just interaction sources. |
| [ ] | [ItemsView][itemsview-unload] | Remove the stale-Unloaded IsLoaded guard while preserving rendering-hook and keyboard-navigation/bring-into-view cleanup. |
| [ ] | [ColorSpectrum][spectrum-unloaded] | Remove the stale-Unloaded IsLoaded guard and reconcile cancellation with bitmap generation started by template/property/size changes before new Loaded. |
| [ ] | [ResizeGripper][resize-unloaded] | Remove the stale-Unloaded IsLoaded guard while preserving drag cancellation, pointer-capture release, and interaction reset. |
| [ ] | [TableView][table-unloaded] | Remove the stale-Unloaded IsLoaded guard while preserving source ownership, repeater/scroller state, subscriptions, interactions, and session teardown/re-entry work. This control is in the adjacent Tabular DLL. |

FlyoutBase's stage-before-Hide reorder is an independent experiment for chronological latest-request-wins
behavior, not a prerequisite for delayed-sync Loaded2/Unloaded2. Delayed delivery after Close returns
allows the current Hide-then-stage order and avoids the inline Close conflict.

### Experiment backlog

These are pending experiments, not implemented changes or completed validation.
None is required by the handwritten delayed-sync event contract.

- [ ] **Stage the replacement flyout before hiding the current flyout.** In
  [CheckAndHandleOpenFlyout][flyout-stage-after-hide], move
  `SetStagedFlyout(this, m_tpPlacementTarget.Get())` before `openFlyout->Hide()`.
  Keep B staged if A's Hide fails or Closing is canceled. If A.Closing requests C, C replaces B,
  honoring chronological latest-request-wins rather than allowing older B to overwrite C.
  Cover immediate and transition-delayed removal, same-flyout retargeting, and reentrant replacement.
  This fixes an independent staging-policy issue; delayed-sync Unloaded2 works with the existing
  Hide-then-stage order. Staging first does not by itself make inline same-popup reopening safe.
- [ ] **Prototype the IsInitialized mechanism.** Explore the proposed
  FrameworkElement.IsInitialized/InitializedChanged contract for dependency-complete initialization,
  distinct from live-tree entry. Include required subtree dependencies, readiness invalidation after
  retemplating/dependency replacement, and external dependencies as retry notifications rather than
  hard readiness blockers.
- [ ] **Try firing Loaded on live enter, before layout.** Separate entry/context reconciliation
  from dependency-complete initialization and layout-output work, then investigate the consequences
  of delivering Loaded during live entry rather than at its current post-layout tick checkpoint.
  Include entry of a previously disconnected subtree, collapsed/unmeasured elements, re-entry,
  event ordering, and handler-driven tree mutation.
- [ ] **Replace redundant Loaded handlers with property-change handling.** Identify handlers
  that recalculate derived state better maintained by property-change notifications or the
  appropriate size/layout change signals. Ensure an initial calculation and complete invalidation
  coverage before removing the Loaded retry; do not assume every audited retry is already redundant.
- [ ] **Move synchronous Unloaded to the end of Leave and move popup child Leave late in Close.**
  Treat these as one coordinated reentrancy experiment: complete Leave's other work before invoking
  Unloaded, and reorganize Popup.Close so its child-removal/Leave-triggering work runs as late as
  possible. Close currently reaches child Leave through PopupRoot.RemoveChild, not a direct Leave
  call. Separate genuine removal-result/lifetime/animation dependencies from incidental bookkeeping
  order. Check that Enter/Leave processing flags and scope-guard cleanup are settled appropriately
  before the callback, and account for parent/collection cleanup outside Leave that could otherwise
  undo a handler's reattachment. Exercise same-flyout reopening and the ContentDialog close-completion
  path; the intended outcome is safer reentrancy, not an assumption that moving the callback alone
  makes every enclosing operation reentrant.
- [ ] **Move popup child Enter as late as possible in Popup.Open.** As the opening-side counterpart,
  prepare the popup before the child-addition/Enter-triggering work, so an inline Loaded handler sees
  as little unfinished opening work as possible. Open currently reaches child Enter through
  PopupRoot.AddChild. Preserve genuine dependencies: initial registration in the open-popup list
  already precedes AddChild so entering ScrollViewers can discover the correct input site. Determine
  which remaining setup can move earlier and which requires the entered child, and account for
  Enter processing flags and enclosing collection work. Exercise Loaded handlers that hide, reopen,
  or retarget the flyout, including the resulting Flyout.Opened notification.

### Compatibility consequences

This is supporting analysis of the legacy consumers and earlier alternatives. The new proposal does
not move legacy Loaded/Unloaded; these risks guide handler migration or the optional experiments.
The timing/ordering decisions themselves are already described in the handwritten note.

#### Preserve the post-layout, pre-render role of Loaded

The core deliberately raises Loaded after layout, before rendering, and performs layout again if handlers
changed the tree. [Dispatch point][loaded-dispatch].

Moving Loaded earlier can expose missing nested template parts, stale bindings, unavailable host objects,
incorrect geometry, unrealized selection containers, or visual states overwritten by later template binding.
Focus initialization and popup placement use this opportunity for readiness. It is not a guarantee:
collapsed/unmeasured elements and elements added during the drain may lack template/layout output.
The refresh adapter explicitly throws when the expected content parent is absent.

Moving Loaded later, past rendering, instead risks a wrong first frame or a visible correction.
Changing ordering also deserves separate review: NavigationViewItem's handler explicitly expects its base
class's Loaded handler to have established the NavigationView reference first.

#### Making Unloaded synchronous requires a safe delivery point

At the current raise site, Unloaded is queued before the base Leave implementation deactivates the element:
[raise site][unloaded-raise], [deactivation][leave-deactivation].

Simply making that call synchronous would let an ordinary previously loaded element still report
`IsLoaded == true` inside the handler. Existing guards could then skip genuine cleanup.

Handlers also close popups, change properties, complete operations, and invoke application code. Synchronous
delivery introduces those callouts during tree mutation, potentially while Enter/Leave processing is active.
A synchronous design needs a deliberately chosen safe delivery point, not just a dispatch-flag change.

ApplicationBarService additionally uses Unloaded as a signal around unloading transitions so that its popup
is not closed too soon. The unloading-storage gate described above can preserve final-removal timing
independently of the notification queue. Preserving that gate does not by itself make inline callouts safe.

#### Suppressing stale unloads can break counter-based workarounds

ItemsRepeater is a concrete dependency on event accounting. Its Loaded handler checks whether the loaded
count is ahead of the unloaded count; its Unloaded handler resets scrollers/cache only when the counts match.

For example, suppressing a stale Unloaded while retaining both Loaded notifications produces:

```text
Initial Loaded:             L=1 U=0
Remove/re-add; new Loaded:  L=2 U=0
Suppress old Unloaded:     L=2 U=0
Final genuine Unloaded:    L=2 U=1
```

The final equality check fails and that cleanup path is skipped. Suppressing Unloaded alone is therefore
not compatible with this implementation. Event coalescing and the counter workaround must be considered together.

Conversely, coalescing away reload notifications can bypass legitimate host rebinding, such as
NavigationViewItem moving between NavigationViews or WebView2 reconnecting to a new host.

#### Requiring a preceding Loaded can suppress required completion signals

ContentDialog attaches its popup-child Unloaded handler during close. That handler reaches
`OnFinishedClosing()`, raises `Closed`, and completes `ShowAsync`.
[Subscription][dialog-unload-subscription], [handler][dialog-unloaded], [async completion][dialog-completion].

FlyoutBase uses presenter Unloaded to process closure and open a staged flyout. ApplicationBarService
waits for Unloaded to manage popup lifetime. These are priority consumers for any rule that suppresses
Unloaded unless Loaded has already occurred, especially show/hide-before-first-layout scenarios.

The source establishes the completion dependencies; whether a particular suppression rule strands an
operation requires scenario verification. Pairing per element is also different from pairing per subscriber:
some internal listeners intentionally attach only when removal/close is about to happen.

#### IsLoaded semantics and event ordering are coupled

`IsLoaded` is computed as active and not in the pending Loaded list, rather than being an independent
lifecycle flag. [Getter][isloaded-getter].

Changing queueing, batching, or coalescing can therefore change both notifications and the guards controls
use to interpret them. Existing defenses fall into distinct patterns:

* State checks: WebView2, InkCanvas, AnimatedVisualPlayer, ColorSpectrum, ScrollPresenter, ItemsView,
  SystemBackdropElement, ResizeGripper, and TableView.
* Active/live-tree checks: AutoSuggestBox.
* Event accounting: ItemsRepeater.
* Defensive rebinding despite a missing Unloaded: ItemsPresenter.
* Unguarded unload actions: examples include NavigationView's title-bar revocation, ComboBox dropdown closing,
  RadioMenuFlyoutItem's group-map removal, and image-work cancellation in ColorPicker and PersonPicture.

Avoid assuming that all controls share the same workaround, or that removing one anomaly makes all of
these defenses redundant.

### Implications for evaluating a change

Evaluate these independently:

1. Delivery timing: measurement, post-layout/pre-render tick, deferred dispatch, or synchronous removal.
2. Ordering: Loading versus Loaded, Loaded versus old Unloaded, and handler/element delivery order.
3. Pairing/coalescing: suppressing stale unloads, suppressing reloads, or requiring prior Loaded.
4. State: what `IsLoaded`, active-tree membership, parent relationships, and host identity mean during callbacks.

The first compatibility hotspots are ItemsRepeater accounting, popup/dialog completion, host rebinding,
generated binding initialization, and existing `IsLoaded` guards. The handwritten proposal now selects
an additive event contract; this audit supplies migration evidence, not proof that all migrated
handlers or optional timing changes are safe.

### Related GitHub reports

Statuses below were observed during the 2026-11-18 issue search, not maintained automatically:

* [#1900: Loaded/Unloaded/IsLoaded APIs break down on quick swapping][issue-1900] (open).
* [#8342: Unexpected Unloaded for ContentDialog content][issue-8342] (open).
  The [detailed explanation][issue-8342-explanation] discusses synchronous tick delivery versus deferred
  Unloaded, reparenting, and compatibility concerns.
* [#9778: Collapsed UserControl receives Loaded without Loading][issue-9778]
  (closed as a duplicate of #8342). The [response][issue-9778-explanation] explains measurement-dependent
  Loading and the collapsed-element ordering.
* [#11800: FlipView can unload an element multiple times][issue-11800] (open).

### Source references

Links identify the audited implementation locations. Line anchors may drift as source evolves.

[cs-loading-subscription]: ../../src/XamlCompiler/BuildTasks/Microsoft/Xaml/XamlCompiler/CodeGenerators/CSharpPagePass2.tt#L316
[cs-loading-handler]: ../../src/XamlCompiler/BuildTasks/Microsoft/Xaml/XamlCompiler/CodeGenerators/CSharpPagePass2.tt#L1031
[cs-binding-initialization]: ../../src/XamlCompiler/BuildTasks/Microsoft/Xaml/XamlCompiler/CodeGenerators/CSharpPagePass2.tt#L895
[cpp-loading-handler]: ../../src/XamlCompiler/BuildTasks/Microsoft/Xaml/XamlCompiler/CodeGenerators/CppWinRT/CppWinRT_BindingInfoPass2.tt#L42
[measure-loading]: ../../dxaml/xcp/core/core/elements/framework.cpp#L1624
[button-loaded]: ../../dxaml/xcp/dxaml/lib/ButtonBase_Partial.cpp#L373
[page-loaded]: ../../dxaml/xcp/dxaml/lib/Page_Partial.cpp#L481
[menu-presenter-loaded]: ../../dxaml/xcp/dxaml/lib/MenuFlyoutPresenter_Partial.cpp#L990
[window-chrome-loaded]: ../../dxaml/xcp/dxaml/lib/WindowChrome_Partial.cpp#L43
[appbar-loaded]: ../../dxaml/xcp/dxaml/lib/AppBar_Partial.cpp#L106
[splitview-loaded]: ../../dxaml/xcp/dxaml/lib/SplitView_Partial.cpp#L201
[autosuggest-parts]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L695
[autosuggest-unloaded]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L439
[combobox-unloaded]: ../../dxaml/xcp/dxaml/lib/ComboBox_Partial.cpp#L2145
[combobox-part-loaded]: ../../dxaml/xcp/dxaml/lib/ComboBox_Partial.cpp#L3678
[commandbar-unloaded]: ../../dxaml/xcp/dxaml/lib/CommandBar_Partial.cpp#L1818
[commandbar-part-loaded]: ../../dxaml/xcp/dxaml/lib/CommandBar_Partial.cpp#L1933
[dialog-unloaded]: ../../dxaml/xcp/dxaml/lib/ContentDialog_Partial.cpp#L1381
[flyout-unloaded]: ../../dxaml/xcp/dxaml/lib/FlyoutBase_partial.cpp#L2323
[appbar-service-subscription]: ../../dxaml/xcp/dxaml/lib/ApplicationBarService_Partial.cpp#L1139
[items-presenter-loaded]: ../../dxaml/xcp/dxaml/lib/ItemsPresenter_Partial.cpp#L194
[listview-loaded]: ../../dxaml/xcp/dxaml/lib/ListViewBase_Partial.cpp#L2757
[panel-unloaded]: ../../dxaml/xcp/dxaml/lib/ModernCollectionBasePanel_Partial.cpp#L2625
[panel-detach]: ../../dxaml/xcp/dxaml/lib/ModernCollectionBasePanel_WindowManagement_Partial.cpp#L420
[scrollviewer-loaded]: ../../dxaml/xcp/dxaml/lib/ScrollViewer_Partial.cpp#L15095
[scroll-content-unloaded]: ../../dxaml/xcp/dxaml/lib/ScrollContentPresenter_Partial.cpp#L3477
[popup-child-loaded]: ../../dxaml/xcp/dxaml/lib/Popup_Partial.cpp#L1697
[media-root-loaded]: ../../dxaml/xcp/dxaml/lib/MediaTransportControls_partial.cpp#L1924
[media-commandbar-loaded]: ../../dxaml/xcp/dxaml/lib/MediaTransportControls_partial.cpp#L6055
[timepicker-loaded]: ../../dxaml/xcp/dxaml/lib/TimePicker_Partial.cpp#L110
[menu-item-loaded]: ../../dxaml/xcp/dxaml/lib/MenuFlyoutItem_Partial.cpp#L38
[cascading-menu-loaded]: ../../dxaml/xcp/dxaml/lib/CascadingMenuHelper.cpp#L67
[hub-panel-loaded]: ../../dxaml/xcp/dxaml/lib/Hub_Partial.cpp#L546
[hub-section-loaded]: ../../dxaml/xcp/dxaml/lib/HubSection_Partial.cpp#L33
[pivot-loaded]: ../../dxaml/phone/lib/Pivot_Partial.cpp#L2737
[pivot-unloaded]: ../../dxaml/phone/lib/Pivot_Partial.cpp#L3108
[pivot-slide-in]: ../../dxaml/phone/lib/Pivot_SlideInEffect_Partial.cpp#L304
[element-name-loaded]: ../../dxaml/xcp/dxaml/lib/BindingExpression_Partial.cpp#L1750
[tooltip-unloaded]: ../../dxaml/xcp/dxaml/lib/FrameworkElement_partial.cpp#L1594
[event-trigger]: ../../dxaml/xcp/dxaml/lib/EventTrigger_Partial.cpp#L11
[animated-icon-loaded]: ../../controls/dev/AnimatedIcon/AnimatedIcon.cpp#L72
[progress-ring-loaded]: ../../controls/dev/ProgressRing/ProgressRing.cpp#L69
[numberbox-loaded]: ../../controls/dev/NumberBox/NumberBox.cpp#L237
[tab-item-loaded]: ../../controls/dev/TabView/TabViewItem.cpp#L125
[breadcrumb-loaded]: ../../controls/dev/Breadcrumb/BreadcrumbBar.cpp#L86
[breadcrumb-item-loaded]: ../../controls/dev/Breadcrumb/BreadcrumbBarItem.cpp#L115
[autosuggest-helper-loaded]: ../../controls/dev/AutoSuggestBox/AutoSuggestBoxHelper.cpp#L71
[two-pane-loaded]: ../../controls/dev/TwoPaneView/TwoPaneView.cpp#L78
[radio-buttons-loaded]: ../../controls/dev/RadioButtons/RadioButtons.cpp#L112
[selectorbar-loaded]: ../../controls/dev/SelectorBar/SelectorBar.cpp#L129
[navigation-repeater-loaded]: ../../controls/dev/NavigationView/NavigationView.cpp#L4666
[navigation-lifecycle]: ../../controls/dev/NavigationView/NavigationView.cpp#L4682
[navigation-item-base-loaded]: ../../controls/dev/NavigationView/NavigationViewItemBase.cpp#L20
[navigation-item-loaded]: ../../controls/dev/NavigationView/NavigationViewItem.cpp#L184
[tabview-lifecycle]: ../../controls/dev/TabView/TabView.cpp#L533
[commandbar-flyout-loaded]: ../../controls/dev/CommandBarFlyout/CommandBarFlyoutCommandBar.cpp#L20
[proofing-loaded]: ../../controls/dev/CommandBarFlyout/TextCommandBarFlyout.cpp#L208
[animated-player-lifecycle]: ../../controls/dev/AnimatedVisualPlayer/AnimatedVisualPlayer.cpp#L374
[webview-lifecycle]: ../../controls/dev/WebView2/WebView2.cpp#L1946
[ink-loaded]: ../../controls/dev/InkCanvas/InkCanvas.cpp#L126
[ink-unloaded]: ../../controls/dev/InkCanvas/InkCanvas.cpp#L318
[backdrop-lifecycle]: ../../controls/dev/SystemBackdropElement/SystemBackdropElement.cpp#L13
[scroll-presenter-lifecycle]: ../../controls/dev/ScrollPresenter/ScrollPresenter.cpp#L4314
[itemsview-lifecycle]: ../../controls/dev/ItemsView/ItemsView.cpp#L1585
[repeater-lifecycle]: ../../controls/dev/Repeater/ItemsRepeater.cpp#L492
[parallax-loaded]: ../../controls/dev/ParallaxView/ParallaxView.cpp#L247
[scroll-input-loaded]: ../../controls/dev/ParallaxView/ScrollInputHelper.cpp#L1058
[refresh-adapter-loaded]: ../../controls/dev/PullToRefresh/ScrollViewerIRefreshInfoProviderAdapter/ScrollViewerIRefreshInfoProviderAdapter.cpp#L241
[swipe-loaded]: ../../controls/dev/SwipeControl/SwipeControl.cpp#L540
[spectrum-unloaded]: ../../controls/dev/ColorPicker/ColorSpectrum.cpp#L432
[colorpicker-unloaded]: ../../controls/dev/ColorPicker/ColorPicker.cpp#L666
[person-picture-unloaded]: ../../controls/dev/PersonPicture/PersonPicture.cpp#L619
[teaching-tip-unloaded]: ../../controls/dev/TeachingTip/TeachingTip.cpp#L1389
[teaching-tip-loaded]: ../../controls/dev/TeachingTip/TeachingTip.cpp#L1630
[radio-menu-lifecycle]: ../../controls/dev/RadioMenuFlyoutItem/RadioMenuFlyoutItem.cpp#L30
[radio-submenu-loaded]: ../../controls/dev/RadioMenuFlyoutItem/RadioMenuFlyoutItem.cpp#L114
[scrollview-unloaded]: ../../controls/dev/ScrollView/ScrollView.cpp#L553
[resize-unloaded]: ../../controls/dev/ResizeGripper/ResizeGripper.cpp#L44
[ink-toolbar-unloaded]: ../../controls/dev/InkToolbar/InkToolbar.cpp#L59
[annotated-loaded]: ../../controls/dev/AnnotatedScrollBar/AnnotatedScrollBar.cpp#L34
[table-loaded]: ../../controls/dev/TableView/TableView.cpp#L748
[table-unloaded]: ../../controls/dev/TableView/TableView.cpp#L4073
[table-repeater-loaded]: ../../controls/dev/TableView/TableView.cpp#L1195
[table-row-lifecycle]: ../../controls/dev/TableView/TableViewRow.cpp#L224
[table-header-loaded]: ../../controls/dev/TableView/TableViewColumnHeader.cpp#L40
[table-editor-loaded]: ../../controls/dev/TableView/TableViewTextColumn.cpp#L215
[table-peer-unloaded]: ../../controls/dev/TableView/TableViewCellAutomationPeer.cpp#L355
[loaded-dispatch]: ../../dxaml/xcp/core/dll/xcpcore.cpp#L6587
[unloaded-raise]: ../../dxaml/xcp/core/core/elements/framework.cpp#L2567
[leave-deactivation]: ../../dxaml/xcp/core/core/elements/depends.cpp#L1247
[dialog-unload-subscription]: ../../dxaml/xcp/dxaml/lib/ContentDialog_Partial.cpp#L604
[dialog-completion]: ../../dxaml/xcp/dxaml/lib/ContentDialog_Partial.cpp#L1485
[isloaded-getter]: ../../dxaml/xcp/dxaml/lib/FrameworkElement_partial.cpp#L1710
[issue-1900]: https://github.com/microsoft/microsoft-ui-xaml/issues/1900
[issue-8342]: https://github.com/microsoft/microsoft-ui-xaml/issues/8342
[issue-8342-explanation]: https://github.com/microsoft/microsoft-ui-xaml/issues/8342#issuecomment-2031017667
[issue-9778]: https://github.com/microsoft/microsoft-ui-xaml/issues/9778
[issue-9778-explanation]: https://github.com/microsoft/microsoft-ui-xaml/issues/9778#issuecomment-2240900253
[issue-11800]: https://github.com/microsoft/microsoft-ui-xaml/issues/11800
[reactor-wishlist]: https://gist.github.com/kennykerr/eab2e1a3650d79b7a6a475e9eb58bd46
[flyout-presentation]: ../../dxaml/xcp/dxaml/lib/FlyoutBase_partial.cpp#L2235
[autosuggest-readiness-test]: ../../dxaml/test/native/external/controls/autosuggestbox/AutoSuggestBoxIntegrationTests.cpp#L2819
[button-apply]: ../../dxaml/xcp/dxaml/lib/ButtonBase_Partial.cpp#L863
[selector-apply]: ../../dxaml/xcp/dxaml/lib/Selector_Partial.cpp#L113
[listbox-loaded]: ../../dxaml/xcp/dxaml/lib/ListBox_Partial.cpp#L890
[listbox-default-template]: ../../controls/dev/CommonStyles/ListBox_themeresources.xaml#L224
[listboxitem-apply]: ../../dxaml/xcp/dxaml/lib/ListBoxItem_Partial.cpp#L69
[selector-prepare]: ../../dxaml/xcp/dxaml/lib/Selector_Partial.cpp#L1427
[comboboxitem-apply]: ../../dxaml/xcp/dxaml/lib/ComboBoxItem_Partial.cpp#L131
[listviewitem-apply]: ../../dxaml/xcp/dxaml/lib/ListViewBaseItem_Partial.cpp#L260
[listview-prepare]: ../../dxaml/xcp/dxaml/lib/ListViewBase_Partial.cpp#L238
[apply-template-order]: ../../dxaml/xcp/core/core/elements/framework.cpp#L1275
[presenter-refresh]: ../../dxaml/xcp/core/core/elements/ContentPresenter.cpp#L580
[items-presenter-refresh]: ../../dxaml/xcp/core/core/elements/ItemsPresenter.cpp#L97
[vsm-no-root]: ../../dxaml/xcp/dxaml/lib/VisualStateManager_Partial.cpp#L382
[vsm-same-state]: ../../dxaml/xcp/components/vsm/CVisualStateManager2.cpp#L127
[state-change-suspender]: ../../dxaml/xcp/dxaml/lib/Control_Partial.cpp#L1187
[vsm-leave]: ../../dxaml/xcp/components/vsm/CVisualStateManager2.cpp#L441
[layout-event-delivery]: ../../dxaml/xcp/core/layout/LayoutManager.cpp#L393
[layout-updated-dispatch]: ../../dxaml/xcp/dxaml/lib/DXamlCore.cpp#L2278
[autosuggest-position]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L1329
[autosuggest-available-space]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L2021
[autosuggest-size]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L1252
[autosuggest-width-update]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L554
[autosuggest-overlay-layout]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L2856
[autosuggest-popup-template]: ../../controls/dev/AutoSuggestBox/AutoSuggestBox_themeresources.xaml#L281
[autosuggest-deferred-layout]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L1498
[popup-placement-subscriptions]: ../../dxaml/xcp/dxaml/lib/Popup_Partial.cpp#L110
[popup-placement-calculation]: ../../dxaml/xcp/dxaml/lib/Popup_Partial.cpp#L975
[presenter-scroll-invalidation]: ../../dxaml/xcp/dxaml/lib/ScrollContentPresenter_Partial.cpp#L3123
[scrollviewer-layout-invalidation]: ../../dxaml/xcp/dxaml/lib/ScrollViewer_Partial.cpp#L4784
[scrollviewer-handler-catchup]: ../../dxaml/xcp/dxaml/lib/ScrollViewer_Partial.cpp#L10981
[scrollviewer-pixel-viewport]: ../../dxaml/xcp/dxaml/lib/ScrollViewer_Partial.cpp#L5757
[scrollviewer-zero-viewport]: ../../dxaml/xcp/dxaml/lib/ScrollViewer_Partial.cpp#L4008
[pivot-header-layout]: ../../dxaml/phone/lib/PivotHeaderPanel_Partial.cpp#L75
[pivot-header-state-machine]: ../../dxaml/xcp/components/pivot/PivotStateMachine.cpp#L720
[pivot-parallax-layout]: ../../dxaml/phone/lib/PivotHeaderManager.cpp#L736
[pivot-parallax-invalidation]: ../../dxaml/phone/lib/PivotHeaderManager.cpp#L702
[pivot-dm-readiness]: ../../dxaml/phone/lib/Pivot_Partial.cpp#L3184
[secondary-relationship-readiness]: ../../dxaml/xcp/core/input/InputServices.cpp#L10825
[hub-scroll-layout]: ../../dxaml/xcp/dxaml/lib/Hub_Partial.cpp#L733
[hub-deferred-scroll]: ../../dxaml/xcp/dxaml/lib/Hub_Partial.cpp#L1004
[initialization-api-base]: ../../dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.cs#L1885
[control-public-apply-template]: ../../dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Controls.cs#L3254
[content-presenter-inheritance]: ../../dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Controls.cs#L5849
[items-presenter-inheritance]: ../../dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Controls.cs#L6146
[parallax-source-discovery]: ../../controls/dev/ParallaxView/ScrollInputHelper.cpp#L520
[parallax-no-source]: ../../controls/dev/ParallaxView/ParallaxView.cpp#L550
[autosuggest-query-initialization]: ../../dxaml/xcp/dxaml/lib/AutoSuggestBox_Partial.cpp#L711
[tabview-child-initialization]: ../../controls/dev/TabView/TabView.cpp#L546
[animated-player-release]: ../../controls/dev/AnimatedVisualPlayer/AnimatedVisualPlayer.cpp#L924
[backdrop-resource-release]: ../../controls/dev/SystemBackdropElement/SystemBackdropElement.cpp#L221
[items-presenter-detach]: ../../dxaml/xcp/dxaml/lib/ItemsPresenter_Partial.cpp#L245
[scroll-presenter-unload]: ../../controls/dev/ScrollPresenter/ScrollPresenter.cpp#L4362
[itemsview-unload]: ../../controls/dev/ItemsView/ItemsView.cpp#L1605
[scrollviewer-unload-state]: ../../dxaml/xcp/dxaml/lib/ScrollViewer_Partial.cpp#L15110
[appbar-service-unload]: ../../dxaml/xcp/dxaml/lib/ApplicationBarService_Partial.cpp#L1437
[appbar-popup-lifetime]: ../../dxaml/xcp/dxaml/lib/ApplicationBarService_Partial.cpp#L704
[dialog-close-restoration]: ../../dxaml/xcp/dxaml/lib/ContentDialog_Partial.cpp#L1427
[table-row-unload-restore]: ../../controls/dev/TableView/TableViewRow.cpp#L235
[resize-drag-completion]: ../../controls/dev/ResizeGripper/ResizeGripper.cpp#L332
[collection-remove-order]: ../../dxaml/xcp/components/Collection/DOCollection.cpp#L714
[collection-removal-shutdown]: ../../dxaml/xcp/components/Collection/UIElementCollection.cpp#L140
[element-shutdown-layout]: ../../dxaml/xcp/core/core/elements/uielement.cpp#L7722
[render-size-storage]: ../../dxaml/xcp/core/core/elements/uielement.cpp#L3911
[popup-close-mutation]: ../../dxaml/xcp/core/core/elements/Popup.cpp#L708
[popup-child-close-guard]: ../../dxaml/xcp/core/core/elements/Popup.cpp#L2673
[flyout-stage-after-hide]: ../../dxaml/xcp/dxaml/lib/FlyoutBase_partial.cpp#L1765
[flyout-stage-invariant]: ../../dxaml/xcp/dxaml/lib/FlyoutMetadata.cpp#L41
[unloading-storage-gate]: ../../dxaml/xcp/components/Collection/UIElementCollection.cpp#L955
[unloading-final-removal]: ../../dxaml/xcp/components/Collection/UIElementCollection.cpp#L685
[repeater-reset-scrollers]: ../../controls/dev/Repeater/ViewportManager.cpp#L722
[repeater-ensure-scroller]: ../../controls/dev/Repeater/ViewportManager.cpp#L455
[popup-open-list-invariant]: ../../dxaml/xcp/core/core/elements/Popup.cpp#L4369
[scroll-presenter-loaded]: ../../controls/dev/ScrollPresenter/ScrollPresenter.cpp#L4314
[scroll-presenter-arrange-viewport]: ../../controls/dev/ScrollPresenter/ScrollPresenter.cpp#L957
[spectrum-template-setup]: ../../controls/dev/ColorPicker/ColorSpectrum.cpp#L42
[spectrum-size-rebuild]: ../../controls/dev/ColorPicker/ColorSpectrum.cpp#L776
[spectrum-bitmap-completion]: ../../controls/dev/ColorPicker/ColorSpectrum.cpp#L1012
[event-disable-loaded]: ../../dxaml/xcp/core/dll/eventmgr.cpp#L237
[event-handler-eligibility]: ../../dxaml/xcp/core/dll/eventmgr.cpp#L704
[backdrop-unloaded-guard]: ../../controls/dev/SystemBackdropElement/SystemBackdropElement.cpp#L19
