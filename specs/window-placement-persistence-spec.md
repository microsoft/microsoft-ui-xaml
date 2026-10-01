Window placement persistence
===

# Background

In WinUI, `Microsoft.UI.Xaml.Window` hosts an app's XAML user interface in a desktop window.
Apps often need to remember that window's position and size between runs. They also need to
handle changes: a monitor might be disconnected, its scale might change, or the user might
close the window while it is maximized.

Today, apps must combine window-management APIs with their own storage and restore logic.
This proposal adds automatic placement persistence, editable placement data, and control over
how a window first appears. You can enable automatic persistence without replacing your
existing `Window.Activate()` call.

_Spec note: These APIs are proposed, not a release commitment. Review status, unresolved
questions, and links to the detailed design are in the [appendix](#appendix)._

Start with [concepts](#conceptual-pages-how-to) and [examples](#examples).
Use [API pages](#api-pages) for member descriptions, defaults, and important restrictions.
The complete declarations are in [API details](#api-details).

# Conceptual pages (How To)

## Understand window placement

A **placement** describes a window's position, size, and display. It records whether the window
is normal, maximized, or snapped. It also records whether the window is minimized and which
placement it should return to when restored. A snapped window occupies a region selected through
Windows snapping, such as one half of a display.

These placement and minimization choices make up the **placement state** represented by
`WindowPlacementState`. They do not describe every aspect of a window's state. Visibility,
activation, and display modes such as full-screen and compact-overlay are separate choices.

Placement also records the display's work area and scale. The **work area** is the part of
the display available to windows, excluding areas reserved for items such as the taskbar.
WinUI uses this information to adjust saved placement when the display environment changes.
Restoration does not guarantee the same physical pixels on a different display.

Showing and hiding control visibility, which is separate from minimization. **Activation**
requests that the window receive user input. You can show a window without requesting activation;
requesting activation does not guarantee that Windows will give it foreground focus.

<a id="contract-at-a-glance"></a>

## Choose where a new window appears

Set `PersistPlacementId` before the window first appears to give it a saved placement name.
For example, `"MainWindow"` identifies the position you want to remember for your main window.
Keep the name stable between runs; do not derive it from a localized window title.

When you first call `Show`, WinUI chooses placement in this order:

1. Use the `WindowPlacement` you supplied in the request, if any.
2. Otherwise, if cascading is enabled, use an eligible open window in the same group.
3. Otherwise, load saved placement if automatic persistence is enabled.
4. If no usable source is available, continue displaying the window without restoring a placement.

**Cascading** offsets a new window from an existing window so that their title bars are not
directly on top of each other. Windows with the same placement id form a cascade group.
For packaged apps, the windows must also belong to the same application. They can be in
different processes. Cascading does not guarantee that windows never overlap.

Use different ids for independently remembered windows. Reuse an id when windows should share
both a saved position and a cascade group. The feature does not provide separate ids for these
two purposes.

## Remember placement when a window closes

Automatic persistence requires both a non-empty `PersistPlacementId` and
`UseAutomaticPlacementPersistence = true` when the window first appears through `Show` or
`Activate`. WinUI then saves its placement on an accepted close. A canceled close does not save.
WinUI also attempts to save during window destruction and confirmed Windows session shutdown.

Setting the properties after first display does not enable saving for a window that missed
this initial setup. For a window that did enable saving, setting the Boolean to `false` or
clearing the id suspends future saves. Setting them back resumes saving under the current id.

Hiding, moving, resizing, or capturing a window does not save it. A window that has never
appeared does not save. A previously shown window can still save after it has been hidden.
Do not rely on saving during a crash or forced termination.

Saved placement is a cache, not permanent application data. Loading can return no data, and
these APIs do not report whether an automatic save succeeded.

## Packaged and unpackaged apps

Automatic storage requires a packaged desktop app with a usable registered application
identity. A package, commonly installed through MSIX, gives Windows an identity for the app.
Apps in the same package have separate placement stores. The data is local to the current
user and computer, not synchronized between devices.

In an unpackaged desktop app, you can still capture and supply placement, cascade windows,
and use the display options. You must provide your own storage to remember placement between
runs. Use an app-specific placement id for cascading: unrelated unpackaged apps using the
same id can join the same group.

This feature does not restore full-screen or compact-overlay modes, recreate snap groups, or
keep a separate placement history for every monitor arrangement. It does not expose its storage
provider or serialization format, or methods to migrate or delete individual saved records.

<a id="errors-and-threading"></a>

## Call the APIs from the appropriate thread

Call the `Window` members on the UI thread that owns the window, before it closes. Calls on
the wrong thread or after close report the corresponding Window API error. A method whose
name begins with `Try` can still report these programming errors.

You can use `WindowPlacement` and `WindowShowOptions` from any thread. Individual property
access is thread-safe, but a series of assignments is not one transaction. Finish related edits
before passing an object to a method. Each operation takes a consistent snapshot of each object;
later edits do not change that operation. The options and referenced placement are not one
combined transaction.

Null options are invalid for both options-taking methods, even after first display or during
a nested call. WinUI checks for null before checking the thread, closed state, or window type.
Invalid arguments report `E_INVALIDARG`, which becomes `ArgumentException` in .NET.
Other errors retain their HRESULT, the error code exposed by Windows Runtime APIs.

<a id="primary-workflows"></a>

# Examples

These C# snippets run in an initialized desktop WinUI app and use the `Microsoft.UI.Xaml`
namespace. `MainWindow` is your app's `Window` subclass; `_mainWindow` is a field that retains
it. Each example starts with a new window. They are alternatives, not consecutive steps.

## Remember the main window automatically

Set a stable id before your existing `Activate()` call:

```csharp
_mainWindow = new MainWindow { PersistPlacementId = "MainWindow" };
_mainWindow.Activate();
```

`UseAutomaticPlacementPersistence` is already `true` by default. On a later run, WinUI can
restore the saved placement. If another related window is open, the new window can cascade
from it instead. You do not need to handle the close event to save placement.

For new code, you can use `_mainWindow.Show()` instead. Use a `WindowShowOptions` object when
you need to select launch or restart behavior, supply placement, or avoid requesting activation.

## Open a saved maximized or minimized window in its normal state

Load the saved value and set `State` to `Normal` to request a normal, non-minimized window:

```csharp
_mainWindow = new MainWindow { PersistPlacementId = "MainWindow" };
var placement = WindowPlacement.LoadForPersistPlacementId(_mainWindow.PersistPlacementId);
if (placement is not null)
{
    placement.State = WindowPlacementState.Normal;
    placement.SnapRect = null;
}

_mainWindow.Show(new WindowShowOptions { Placement = placement });
```

The loaded value is independent of the window and the stored record. Editing it changes
neither one. A non-null `Placement` replaces automatic source selection for this request;
it does not disable future automatic saves.

If loading returns null, this example permits normal automatic source selection. Passing
null does not mean "disable restore." To manage storage yourself, set
`UseAutomaticPlacementPersistence = false` and supply your own placement value.

## Prepare placement before showing a window

Use `TryApplyInitialPlacement` if you need to prepare placement while a window remains hidden.
Then skip the placement step when you reveal it:

```csharp
_mainWindow = new MainWindow { PersistPlacementId = "MainWindow" };
bool applied = _mainWindow.TryApplyInitialPlacement(new WindowShowOptions());

_mainWindow.Show(new WindowShowOptions
{
    SkipInitialPlacement = true,
    DoNotActivate = true,
});
```

This example accepts the window's resulting placement even if `applied` is `false`.
The hidden operation never shows or activates the window. The later `Show` reveals it
without requesting activation and enables future saves using its persistence properties.

An ordinary first `Show()` or `Activate()` would run placement selection again and could
replace the preparation. `Activate()` has no skip option.

_Spec note: Preparing minimized or maximized placement while hidden, then entering full screen,
has an unresolved behavior requirement. See S37 in the [scenario catalog][scenarios]._

<a id="defaults-and-value-model"></a>

# API Pages

The pages below cover the `Window` members, [placement data](#windowplacement-class),
[request options](#windowshowoptions-class), and the three enums.

## Window.PersistPlacementId property

Gets or sets the name used to store this window's placement and group it with related windows.

The default is an empty string, and null is treated as empty. Empty disables automatic
persistence and cascade grouping. Ids are case-sensitive and are not normalized for Unicode.
Windows sharing an id in the same application's store share one saved record; the last
successful save wins.

Setting this property does not move the window, access storage, or change
`UseAutomaticPlacementPersistence`. You can assign the two properties in either order.

## Window.UseAutomaticPlacementPersistence property

Gets or sets whether this window uses automatic placement persistence.

The default is `true`. A non-empty `PersistPlacementId` is also required. Setting this property
to `false` leaves the id available for cascading but disables automatic loading and saving.
Setting it does not move the window, access storage, or change the id.

Configure both properties before first display. See
[remembering placement](#remember-placement-when-a-window-closes) for later changes.
Showing the window first through `AppWindow` or a native Windows API bypasses this setup;
calling `Window.Show` or `Activate` afterward does not enable automatic saving.

## Window.TryGetPlacement method

Captures the window's placement in an independent, editable `WindowPlacement` value.

Returns `true` with a valid value, or `false` with a null output when capture is unavailable.
Capture does not read or write saved data, move the window, or enable automatic saving.

For a standard desktop window, capture uses its current geometry and last meaningful placement
state, including while hidden. In full-screen or compact-overlay mode, it uses the last complete
valid standard-window snapshot, if one exists. It returns `false` if no such snapshot exists.
The snapshot does not record those display modes or whether the window is hidden or active.

## Window.TryApplyInitialPlacement method

Applies placement to a window that has not yet appeared, without showing or activating it.

Pass a non-null `WindowShowOptions`. You can repeat this operation before first display.
It does not enable automatic saving or complete the initial-placement step.

Returns `true` only when an explicit, related-window, or stored placement was selected and
successfully applied. Returns `false` after first display, during a nested placement operation,
when no usable source exists, when the window's display mode is unsupported, or when application
fails. Adjusting fallback geometry alone does not return `true`.

Application is not all-or-nothing: a failure can leave partial placement changes. The method
does not undo them. To reveal the resulting placement without selecting it again, use
`Show` with `SkipInitialPlacement = true`, as in the
[example](#prepare-placement-before-showing-a-window).

## Window.Show methods

Shows the window, using initial placement options if it has not appeared before.

`Show()` uses default options. `Show(options)` requires a non-null `WindowShowOptions`.
Before first display, these methods select placement as described in
[choosing where a new window appears](#choose-where-a-new-window-appears).
If placement cannot be applied, display continues subject to the activation restrictions.
WinUI does not try another placement source after application begins.

After first display, only `DoNotActivate` is read; other option fields are ignored, even if
invalid. Showing a visible window has no effect. Showing a hidden window preserves its current
placement, including minimization, and requests activation only if it is not minimized and
`DoNotActivate` is `false`.

Unlike `Show`, `Activate()` can restore a minimized window and request activation. On first
display, `Activate()` also performs automatic placement when opted in, using `Default` reason,
`Automatic` cascading, and no explicit placement.

Hiding a window does not make it eligible for initial placement again. Nested display calls
during an initial operation are ignored after the required API-use checks; closing is not ignored.
Neither overload reports whether placement was applied or saved successfully.

## Window.Hide method

Hides the window without closing it or saving its placement.

The window and its content remain alive. Hiding does not reopen initial placement.
Use `Show` to reveal its current placement, or `Activate` to restore it if minimized and request
activation.

## WindowPlacement class

Represents an editable copy of a window's placement and saved display environment.

The object has no connection to a live window. Editing it does not move a window or write
stored data. To use the changed value, pass it in `WindowShowOptions.Placement`.

Rectangles use physical pixels in the coordinate space spanning all displays. A display to
the left of the primary display can have negative coordinates. These are not XAML logical
units, client-area bounds, or Win32 workspace coordinates. `NormalRect` includes the title bar
and borders, not just your content area. Keep the rectangles, work area, and DPI (dots per inch)
together when storing or constructing a value.

## WindowPlacement constructor

Initializes placement from normal window bounds, a display work area, and DPI.

The constructor sets `State` to `Normal`, `SnapRect` and `VirtualDesktopId` to null, and
`DisplayDeviceName` to empty. Invalid required geometry or DPI reports `E_INVALIDARG`.
The detailed [validity rules][validity] specify the accepted ranges and rectangle relationships.

## WindowPlacement.LoadForPersistPlacementId method

Loads saved placement for the specified id without creating or changing a window.

This synchronous method can run on any thread. It returns an independent value, not adjusted
for the current displays. It does not select a related window, apply placement, or write data.
Null or empty ids report `E_INVALIDARG`, unlike the valid empty `Window.PersistPlacementId`.

Returns null when storage is unavailable or inaccessible, no record exists, or stored data
is invalid or uses an unsupported version. Other failures are reported through their original
HRESULT, including `E_OUTOFMEMORY` for allocation failure, rather than being converted to null.
.NET and C++/WinRT expose these failures as exceptions.

Automatic restoration handles storage failures without requiring your app to catch them.
If you call this loader directly, account for its exceptions as well as its null result.

## Other WindowPlacement members

| Member | Description |
|---|---|
| `NormalRect` | Outer window bounds to use in the normal, non-snapped state. |
| `WorkArea` | Saved display's usable rectangle, associated with the saved geometry. |
| `Dpi` | Saved display scale expressed as dots per inch. |
| `State` | Placement and minimization, including the placement to use when restored. |
| `SnapRect` | Optional snapped visible-frame bounds, excluding invisible resize borders. |
| `DisplayDeviceName` | Optional GDI display name; not a permanent physical-device identity. |
| `VirtualDesktopId` | Optional identifier for a Windows virtual desktop. |

Property setters permit temporarily invalid combinations while you edit a value. An operation
that uses the placement validates the complete value. Null `DisplayDeviceName` means empty;
`Guid.Empty` means no virtual-desktop identity when placement is accepted.

## WindowShowOptions class

Specifies placement and display choices for a window operation.

Create an options object with the parameterless constructor, then set the properties you need.
Options apply to one call; the window does not retain them for later calls.

| Member | Default | Description |
|---|---|---|
| `Placement` | null | Explicit placement instead of automatic source selection. |
| `Reason` | `Default` | Policy for ordinary display, launch, or application restart. |
| `CascadeBehavior` | `Automatic` | Whether this operation can cascade from a related window. |
| `DoNotActivate` | `false` | Whether this `Show` avoids requesting activation. |
| `SkipInitialPlacement` | `false` | Whether first `Show` reveals current placement without selecting it. |

Skipping placement does not disable future automatic saves. On an eligible initial call,
combining skip with non-null `Placement`, or using skip with `TryApplyInitialPlacement`,
reports `E_INVALIDARG`. Invalid placement and undefined reason/cascade values also report
`E_INVALIDARG` before side effects. Fields ignored by late or nested calls are not validated.

## WindowPlacementState enum

Specifies a window's placement and minimization, including the placement to use when restored.

Each value combines normal, maximized, or snapped placement with whether the window is minimized.
Choose one value; this is not a flags enum, and its values must not be combined.

| Value | Minimized? | Placement when not minimized |
|---|---|---|
| `Normal` | No | Normal |
| `Maximized` | No | Maximized |
| `Minimized` | Yes | Normal |
| `Snapped` | No | Snapped to `SnapRect` |
| `MinimizedFromMaximized` | Yes | Maximized |
| `MinimizedFromSnapped` | Yes | Snapped to `SnapRect` |

Both snapped values require `SnapRect`. For a value you construct or edit, the enum describes
the requested placement and restore target, not a sequence of previous window states.

Visibility, activation, full-screen, and compact-overlay are not represented by this enum.
Control visibility and activation through display operations and their options; select full-screen
or compact-overlay mode separately.

## WindowShowReason enum

Specifies how an initial placement operation treats saved placement and launch information.

For a saved minimized window, `Default` uses the recorded normal, maximized, or snapped restore
target instead of keeping it minimized. It ignores the saved virtual desktop. `Launch` does
the same and also considers a valid monitor hint from the Windows shell: information about
which display the user launched the app from.

`ApplicationRestart` preserves saved minimization and its restore target, attempts to restore
the saved virtual desktop, and disables cascading and activation, even if `DoNotActivate`
is `false`.
When placement is skipped, only its activation restriction applies. Hidden preparation does
not carry this policy into a later reveal; specify nonactivation on that `Show` separately.

_Spec note: Availability of launch-monitor hints on actual activation paths is still under review._

## WindowCascadeBehavior enum

Specifies whether an initial placement operation can offset the window from a related window.

`Automatic` allows cascading when automatic persistence is enabled with a non-empty id and
no explicit placement is supplied. It does not require automatic storage to be available.
`Enabled` also allows cascading without automatic persistence or with explicit placement.
For explicit placement, it adjusts only position using a related window on the selected monitor.

`Disabled` prevents this operation from cascading. It does not prevent the window from being
a source for another window. Cascading always requires a non-empty group id and an eligible
related window. Restart policy and skipping placement suppress it.

<a id="public-api-surface"></a>

# API Details

The declarations use MIDL 3, the Windows Runtime interface-definition language. Existing
`Window` members are omitted. In C# and C++/WinRT you call `Show()`, `Show(options)`, and
`Hide()`; the `method_name` attributes distinguish their binary-interface names.

_Spec note: `WinUIContract` version 12 is a placeholder. The shipping version is not assigned._

```midl
namespace Microsoft.UI.Xaml
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowPlacementState
    {
        Normal = 0,
        Maximized = 1,
        Minimized = 2,
        Snapped = 3,
        MinimizedFromMaximized = 4,
        MinimizedFromSnapped = 5,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowShowReason
    {
        Default = 0,
        Launch = 1,
        ApplicationRestart = 2,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowCascadeBehavior
    {
        Automatic = 0,
        Enabled = 1,
        Disabled = 2,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    [threading(both)]
    [marshaling_behavior(agile)]
    runtimeclass WindowPlacement
    {
        WindowPlacement(
            Windows.Graphics.RectInt32 normalRect,
            Windows.Graphics.RectInt32 workArea,
            Int32 dpi);

        static WindowPlacement LoadForPersistPlacementId(String persistPlacementId);

        Windows.Graphics.RectInt32 NormalRect;
        Windows.Graphics.RectInt32 WorkArea;
        Int32 Dpi;
        WindowPlacementState State;
        Windows.Foundation.IReference<Windows.Graphics.RectInt32> SnapRect;
        String DisplayDeviceName;
        Windows.Foundation.IReference<Guid> VirtualDesktopId;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    [threading(both)]
    [marshaling_behavior(agile)]
    runtimeclass WindowShowOptions
    {
        WindowShowOptions();

        WindowPlacement Placement;
        WindowShowReason Reason;
        WindowCascadeBehavior CascadeBehavior;
        Boolean DoNotActivate;
        Boolean SkipInitialPlacement;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    unsealed runtimeclass Window
    {
        // ... existing members ...

        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        {
            String PersistPlacementId;
            Boolean UseAutomaticPlacementPersistence;

            Boolean TryGetPlacement(out WindowPlacement placement);
            Boolean TryApplyInitialPlacement(WindowShowOptions options);

            [method_name("ShowDefault")]
            void Show();

            [method_name("ShowWithOptions")]
            void Show(WindowShowOptions options);

            [method_name("HideDefault")]
            void Hide();
        }
    };
}
```

# Appendix

_This section supports proposal review and is not intended for product documentation._

## Document ownership and review status

This proposal follows the [repository's API-spec template](api_spec_template.md).
It owns the public declarations, defaults, member descriptions, API-use requirements, and scope.
The [feature specification][feature] owns detailed behavior, architecture, storage, the stable
S01-S37 scenario catalog, and acceptance requirements. The [decision log][decisions] records
rationale, alternatives, and dated evidence; historical entries are not additional guarantees.

Publication follows the [public API review process](public-api-review-process.md).
Public feedback start and end dates are not set. The feedback period has not started, and
the shipping contract version, release, and final API approval remain unassigned.

The proposal addresses [#2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680)
and [#9503](https://github.com/microsoft/microsoft-ui-xaml/issues/9503). Related requests include
[WinUI Gallery #1606](https://github.com/microsoft/WinUI-Gallery/issues/1606) and
[Windows App SDK #5896](https://github.com/microsoft/WindowsAppSDK/issues/5896).

<a id="compatibility-and-scope"></a>

## Compatibility and exclusions

See [packaged and unpackaged apps](#packaged-and-unpackaged-apps) for application support
and storage scope.

Apps that do not use the new APIs retain existing runtime behavior. On recompilation, new
instance `Show` and `Hide` methods can take precedence over extension methods or conflict with
similarly named derived-class members. This source-compatibility cost requires API review.

Public `Show()`, `Show(options)`, and `Hide()` reject otherwise valid calls on non-desktop or
framework dummy windows with `E_NOTIMPL`, without display, placement, persistence, or audio
side effects. Private lifecycle operations remain unchanged. No APIs are added to classic
`Windows.UI.Xaml.Window`.

This proposal does not add cross-process position reservations, general focus-management
APIs, or launcher show-command merging such as `start /min` and `start /max`.
Experimental AppWindow placement APIs are related, but are not a dependency or a
field-for-field equivalent of this contract.

## Known limitations and review questions

Implementation progress is not evidence that every requirement is resolved. In particular:

- Hidden minimized, maximized, and snapped application needs supported-version review.
  Failure must not show or activate a window to complete a hidden operation.
- S37, hidden preparation followed by full screen, still has an unresolved maximized-return
  requirement.
- Availability of real launch-monitor hints, including packaged activation paths, is not
  established.
- Synthetic and single-display evidence does not establish multi-monitor, mixed-DPI, or
  older-Windows behavior.

The [coverage and open gates][coverage] distinguish requirements from demonstrated behavior.
Reviewers should focus on whether the defaults are understandable, one id is sufficient for
storage and grouping, and `Show`, `Activate`, and hidden preparation are clearly distinct.
Also consider whether the synchronous loader's error behavior and the `Show`/`Hide`
source-compatibility cost are acceptable.

## Detailed behavior

Use the feature specification for rules intentionally kept out of the short API pages:

- [Source selection, nested calls, and first-display handling][loading].
- [Capture, editing, and supplied placement][capture]; [data validity][validity].
- [Cascading][cascading] and [launch or restart policy][policy].
- [Save eligibility][saving], [storage retention][retention], and
  [load-error classification][reader].
- [Scenario outcomes][scenarios], [safe fallback][fallback], and
  [acceptance requirements][acceptance].

[feature]: ../docs/design-notes/Window-PlacementPersistence.md
[decisions]: ../docs/design-notes/Window-PlacementPersistence-decisions.md
[loading]: ../docs/design-notes/Window-PlacementPersistence.md#loading-applying-and-displaying
[capture]: ../docs/design-notes/Window-PlacementPersistence.md#capture-change-and-supply-placement
[validity]: ../docs/design-notes/Window-PlacementPersistence.md#data-validity
[cascading]: ../docs/design-notes/Window-PlacementPersistence.md#cascade-related-windows
[policy]: ../docs/design-notes/Window-PlacementPersistence.md#choose-launch-or-restart-policy
[saving]: ../docs/design-notes/Window-PlacementPersistence.md#know-when-placement-is-saved
[retention]: ../docs/design-notes/Window-PlacementPersistence.md#retaining-the-32-most-recently-saved-placements
[reader]: ../docs/design-notes/Window-PlacementPersistence.md#one-reader-for-automatic-restore-and-detached-loading
[scenarios]: ../docs/design-notes/Window-PlacementPersistence.md#placement-scenarios-and-winui-decisions
[coverage]: ../docs/design-notes/Window-PlacementPersistence.md#coverage-and-open-gates
[fallback]: ../docs/design-notes/Window-PlacementPersistence.md#safe-fallback-and-review-gates
[acceptance]: ../docs/design-notes/Window-PlacementPersistence.md#implementation-acceptance-criteria
