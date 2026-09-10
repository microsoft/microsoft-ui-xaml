WinUI Window Placement Persistence
---

> **AI-assisted document:** This document was written and revised with AI assistance. AI makes
> mistakes; technical claims require human review.

- [WinUI Window Placement Persistence](#winui-window-placement-persistence)
- [1. Background](#1-background)
  - [1.1. What other frameworks provide](#11-what-other-frameworks-provide)
  - [1.2. Design goals and constraints](#12-design-goals-and-constraints)
    - [Goals](#goals)
    - [Constraints](#constraints)
- [2. Dev Experience (what users need to know)](#2-dev-experience-what-users-need-to-know)
  - [2.1. API Shape](#21-api-shape)
  - [2.2. Window state that WinUI applies](#22-window-state-that-winui-applies)
  - [2.3. Window state that WinUI does not apply](#23-window-state-that-winui-does-not-apply)
  - [2.4. Packaged apps](#24-packaged-apps)
  - [2.5. Unpackaged apps](#25-unpackaged-apps)
  - [2.6. Other kinds of apps](#26-other-kinds-of-apps)
  - [2.7. Kinds of launches](#27-kinds-of-launches)
- [3. Design](#3-design)
  - [3.1. Loading the persistence data](#31-loading-the-persistence-data)
  - [3.2. Applying the persistence data: PlacementEx](#32-applying-the-persistence-data-placementex)
    - [3.2.1. PlacementEx fields used by v1](#321-placementex-fields-used-by-v1)
    - [3.2.2. Policy before apply](#322-policy-before-apply)
    - [3.2.3. PlacementEx state not persisted by v1](#323-placementex-state-not-persisted-by-v1)
  - [3.3. Saving the persistence data](#33-saving-the-persistence-data)
  - [3.4. Persistence data format](#34-persistence-data-format)
    - [3.4.1. v1 tags](#341-v1-tags)
    - [3.4.2. Durable flag bits](#342-durable-flag-bits)
    - [3.4.3. Compatibility](#343-compatibility)
- [4. Appendix](#4-appendix)
  - [4.1. Decision rationale](#41-decision-rationale)
    - [4.1.1. Why one string property](#411-why-one-string-property)
    - [4.1.2. Why the app supplies the id](#412-why-the-app-supplies-the-id)
    - [4.1.3. Why `PersistPlacementId`](#413-why-persistplacementid)
    - [4.1.4. Why `ApplicationData`](#414-why-applicationdata)
    - [4.1.5. Why WinUI owns the serialized format](#415-why-winui-owns-the-serialized-format)
    - [4.1.6. Why presenter state is absent from v1](#416-why-presenter-state-is-absent-from-v1)
    - [4.1.7. Why launch reason is explicit](#417-why-launch-reason-is-explicit)
    - [4.1.8. Why `Window.Show()` and `Window.Hide()`](#418-why-windowshow-and-windowhide)
    - [4.1.9. Why `InitialShowOptions`](#419-why-initialshowoptions)
  - [4.2. Storage measurements and constraints](#42-storage-measurements-and-constraints)
    - [4.2.1. Unpackaged `LocalSettings`](#421-unpackaged-localsettings)
    - [4.2.2. Deployment caveat](#422-deployment-caveat)
    - [4.2.3. Prototype and test requirements](#423-prototype-and-test-requirements)
  - [4.3. Prior art](#43-prior-art)
    - [4.3.1. WPF](#431-wpf)
    - [4.3.2. WinUIEx](#432-winuiex)
    - [4.3.3. Experimental AppWindow placement APIs](#433-experimental-appwindow-placement-apis)
  - [4.4. Unpackaged default-store dependency](#44-unpackaged-default-store-dependency)
  - [4.5. Rejected alternatives](#45-rejected-alternatives)
    - [4.5.1. App-owned persistence engine](#451-app-owned-persistence-engine)
    - [4.5.2. AUMID-based storage identity](#452-aumid-based-storage-identity)
    - [4.5.3. Default-on persistence](#453-default-on-persistence)
  - [4.6. Proposed future work](#46-proposed-future-work)
    - [4.6.1. Restore options](#461-restore-options)
    - [4.6.2. Cascade duplicate instances](#462-cascade-duplicate-instances)
    - [4.6.3. Save on settle](#463-save-on-settle)
  - [4.7. Glossary](#47-glossary)


## 1. Background

WinUI has no built-in way to reopen a desktop window where the user left it. An app can close with
its main window maximized on a particular monitor and reopen at its default size and position. Classic
UWP remembers window placement automatically, so this is a visible regression for apps moving to
WinUI.

The demand is established:

- [WinUI issue #2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680) and
  [#9503](https://github.com/microsoft/microsoft-ui-xaml/issues/9503) request built-in placement
  persistence.
- [WinUI Gallery issue #1606](https://github.com/microsoft/WinUI-Gallery/issues/1606) was the
  Gallery's most-requested backlog item in an early poll.
- WinUIEx offers `WindowManager.PersistenceId`, demonstrating that WinUI apps will adopt a
  one-property solution.

Implementing this correctly requires more than saving `GetWindowRect`. A complete placement includes
the normal rectangle, show state, snapped rectangle, monitor identity, work area, and DPI. Restore
must handle a missing monitor, a changed work area or scale, and a saved window that would otherwise
open off-screen.

### 1.1. What other frameworks provide

Existing frameworks cover different subsets. The [prior art appendix](#43-prior-art) has the detailed
analysis.

| Capability | UWP | WPF | WinUIEx | AppWindow APIs | Our v1 |
|---|:---:|:---:|:---:|:---:|:---:|
| Restore position + size | automatic | app-owned | yes | yes | yes |
| Restore saved maximized state | yes | app-owned | yes | yes | yes |
| Restore snapped state | no | no | no | yes | **yes** |
| Validate saved monitor layout | ? | no | exact topology match | yes | yes |
| Migrate saved size across DPI changes | ? | no | no saved DPI; suppresses WinUI resize during restore | yes | yes |
| Monitor gone -> relocate rather than give up | ? | no | no | yes | **yes** |
| Ordinary launch restores to a visible window | ? | n/a | yes | ? | yes |
| Application restart preserves minimized state | ? | app-owned | no | ? | **yes** |
| Save virtual-desktop identity | ? | app-owned | no | no | **best effort** |
| Restore virtual desktop during non-activating app restart | ? | app-owned | no | no | **yes** |
| Save on logoff / shutdown | ? | no | no | ? | **yes** |
| Preserve overlapped placement through presenter changes | ? | app-owned | yes, including maximized state | ? | position + size only |
| Restore presenter (full screen / compact) | ? | no | no | **yes** | no |
| Honor an external launch command (`start /max`) | n/a | no | no | **yes** | **yes** |
| Versioned, evolvable format | ? | n/a | no | ? | **yes** |
| Framework owns storage for packaged apps | yes | no | yes | yes | **yes** |
| Unpackaged apps covered | n/a | app-owned | dictionary escape hatch | ? | **requires new WASDK API** |
| Simple placement opt-in | automatic | no | **yes** | no (uses a GUID) | **yes** |

The UWP column records its automatic size and position persistence; other UWP details shown as `?`
have not been verified. The AppWindow column is based on the experimental API surface and the shared
use of PlacementEx. Other `?` entries have not been verified.

The selected WinUI design adds placement identity and initial-show APIs on `Window`, uses the app's
default `Microsoft.Windows.Storage.ApplicationData` store, and delegates native placement behavior
to PlacementEx. WinUI owns the public policy, storage layout, serialization format, and save timing.

### 1.2. Design goals and constraints

#### Goals

- **Close a UWP migration gap.** UWP remembers window placement automatically. WinUI should provide a
  supported path that restores the position, size, and state users expect when an app migrates.
- **Nail the common 80% case.** An existing single-window app should enable ordinary placement
  persistence by setting one stable `PersistPlacementId` and keeping its existing
  `window.Activate()` call.
- **Create a pit of success.** The simplest path should handle monitor removal, work-area changes,
  DPI migration, maximized state, snap state, and corrupt data correctly. Apps should not need to
  reproduce native window-placement policy.
- **Restore before becoming user-visible.** A participating window should first appear at its
  restored placement rather than visibly moving between geometry, monitor, or virtual desktop after
  it is shown.
- **Keep advanced scenarios possible.** Multi-window launch, application restart, and an optional
  pre-display placement workflow can add explicit controls without making the common path more
  complex.
- **Remain evolvable.** Stable app-owned ids and a versioned format should allow future placement
  fields and implementation changes without invalidating existing saved state.

#### Constraints

- **Preserve compatibility for non-participating apps.** A window with no `PersistPlacementId` should
  retain existing observable behavior. Any change to HWND creation or first-`ShowWindow` timing must
  meet this bar.
- **Preserve the `Activate()` adoption path.** Advanced placement or `Show()` APIs must remain optional.
  An app must not need to replace `Activate()` merely to obtain ordinary persistence.
- **Do not impose a window hierarchy.** WinUI should not infer or enforce relationships among an
  app's windows. If the app supplies primary/secondary placement information, use it only to select
  startup placement behavior; it does not define ownership, lifetime, activation, or shutdown
  semantics.
- **Account for non-WinUI HWNDs.** A process can show a splash screen or another HWND before its WinUI
  windows. Native `STARTUPINFO.wShowWindow` remains tied to the process's first `ShowWindow` call.
- **Use the process default store.** WinUI resolves placement only through
  `ApplicationData.GetDefault()`. Unpackaged support depends on Windows App SDK default-store setup;
  this design adds no XAML storage API.
- **Fail safely.** Missing storage, corrupt data, monitor changes, or placement failures must not
  prevent the app window from opening.
- **Choose the v1 boundary based on usability.** A smaller implementation is not a goal by itself.
  Defer a scenario only when its absence leaves placement behavior predictable and does not force
  apps back to custom persistence for otherwise common use.

## 2. Dev Experience (what users need to know)

### 2.1. API Shape

`Window.PersistPlacementId` opts a window into persistence. For an existing single-window app, that
property is the only new API the app needs. Existing `Activate()` code remains valid.

`Window.InitialShowOptions` configures the window's one initial placement attempt. The first
`Show()` or `Activate()` reached while that opportunity is open snapshots, honors, and consumes
every option. After that initial operation, `Activate()` retains its existing behavior and requests
activation.

The MIDL below shows the intended stable API shape. The first implementation remains experimental
and feature-gated while the prototype and compatibility work is completed. API graduation requires
the ship gates in Section 4.2.3 to pass.

```midl
namespace Microsoft.UI.Xaml
{
    // Specifies why a window is being shown for the first time.
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowShowReason
    {
        // Ordinary display.
        Default = 0,

        // This window is being shown as part of app launch.
        Launch = 1,

        // The app is reconstructing the window after application restart.
        ApplicationRestart = 2,
    };

    // Specifies activation policy for the initial Show or Activate operation.
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowActivationBehavior
    {
        // Use activating placement policy and request activation unless KeepHidden.
        Activate = 0,

        // Use non-activating placement policy and do not request activation.
        DoNotActivate = 1,
    };

    // Provides one-time options for a window's initial Show or Activate operation.
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    runtimeclass WindowInitialShowOptions
    {
        WindowInitialShowOptions();

        // Selects the initial placement policy.
        WindowShowReason Reason;

        // Selects activation policy for the initial Show or Activate operation.
        WindowActivationBehavior ActivationBehavior;

        // When the initial Show or Activate consumes the options, applies placement while
        // leaving the window hidden.
        Boolean KeepHidden;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    unsealed runtimeclass Window
    {
        // ... existing APIs ...

        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        {
            // Enables placement persistence and identifies the saved placement.
            String PersistPlacementId;

            // Gets or sets options consumed by the first Show or Activate call.
            WindowInitialShowOptions InitialShowOptions;

            // Runs the initial-display pipeline or displays a hidden window.
            [method_name("ShowDefault")]
            void Show();

            // Hides the window without closing it.
            [method_name("HideDefault")]
            void Hide();
        }
    };

}
```

The new APIs are desktop-only. Their UWP-mode implementations return `E_NOTIMPL`; the existing
`Activate()` method retains its current UWP-mode behavior.

A non-empty value both enables persistence and names the saved placement. Null or empty is the
default and disables persistence. There is no separate Boolean enable property; the
[one-property rationale](#411-why-one-string-property) explains why.

Set `PersistPlacementId` before the first `Show()` or `Activate()` so WinUI can restore before the
window appears. An ordinary single-window app only sets the id:

```csharp
public MainWindow()
{
    InitializeComponent();
    PersistPlacementId = "MainWindow";
}
```

It can also be set in XAML:

```xml
<Window
    x:Class="Contoso.MainWindow"
    PersistPlacementId="MainWindow">
    ...
</Window>
```

The app is responsible for keeping each window's `PersistPlacementId` value stable across app
updates for as long as it wants to reuse the saved placement. Changing the value selects a new
placement slot; WinUI does not migrate or delete the old entry. The appendix explains
[why the app supplies this identity](#412-why-the-app-supplies-the-id). Multi-window apps assign a
distinct id to each independently restored window:

The id is persistent metadata, not a secret. Its storage name includes a short readable prefix.
Apps should use a stable, non-sensitive id and should not include file paths, document titles,
account names, or other sensitive or user-identifying text.

```csharp
public DocumentWindow(string documentPersistenceId)
{
    InitializeComponent();
    PersistPlacementId = $"DocumentWindow:{documentPersistenceId}";
}
```

WinUI reads the property for the initial placement attempt and for each save:

- Setting it after the initial placement attempt enables future saves but does not move or
  retroactively restore the live window.
- Changing it after restore sends the next save to the new id. The old entry remains.
- Clearing it stops future saves.
- An app that never sets it follows the existing windowing path and never resolves placement storage.

`InitialShowOptions` defaults to null, which uses all default values. It can be set before the
window's first `Show()` or `Activate()`:

```csharp
window.PersistPlacementId = "MainWindow";
window.InitialShowOptions = new WindowInitialShowOptions
{
    Reason = WindowShowReason.Launch,
};
window.Show();
```

While the placement opportunity is `Not attempted`, the first `Show()` or `Activate()` snapshots
and validates all option values before changing window or placement-attempt state. An unknown enum
fails with `E_INVALIDARG` without consuming the attempt, so the app can correct the options and
retry. After validation, placement behavior never reads the property again. The getter and setter
remain usable as normal property storage: the getter returns the currently assigned object, and a
later assignment or mutation is accepted but has no effect on that window. Sharing one options object
across windows is allowed; each window snapshots it independently.

The first `Show()` or `Activate()` honors all initial options:

- `Reason` selects placement policy.
- `ActivationBehavior` selects minimized-state and virtual-desktop policy and, when `KeepHidden` is
  false, whether the initial operation requests activation.
- `KeepHidden` applies placement without making an initially hidden window visible. It suppresses
  this display and activation but does not change the placement-policy row selected by
  `ActivationBehavior`. It raises no `VisibilityChanged` or `Activated` event and does not move
  focus.

After option validation, the first operation consumes the placement attempt even when the id is
empty, no saved value exists, saved data is invalid, or placement fails. A `KeepHidden` operation
remains hidden at its current fallback placement. Its later `Show()` or `Activate()` makes the
window visible without loading or applying persisted placement again and preserves any app
adjustment.

After initial placement, `Show()` is a no-op while the HWND is visible, including while it is
minimized. Use `Activate()` to request activation. After `AppWindow.Hide()` or a `KeepHidden`
placement, `Show()` makes the window visible and requests activation through the normal show path;
the consumed `InitialShowOptions` no longer apply. A later `Activate()` displays a hidden window,
restores a minimized window, and calls `SetActiveWindow()`.

`Window.Hide()` hides a currently visible HWND, including one displayed directly through
`AppWindow.Show()`, without closing it. It does not save placement, reset `InitialShowOptions`, or
reset placement state. It is a no-op while hidden and does not consume an open placement
opportunity. A later `Show()` or `Activate()` reuses the same HWND and XAML content.

Hiding a visible window raises `VisibilityChanged`. If the window was active, existing activation
event behavior reports its deactivation. An already-hidden no-op raises no visibility event.

If automatic persistence captures a hidden window during close, destruction, or session end, it
uses the last non-hidden placement rather than saving `SW_HIDE`.
After a `KeepHidden` placement, an app that needs to reveal without activation calls
`window.AppWindow.Show(false)`. That path preserves an applied minimized state. Because the placement
attempt is already complete, it does not trigger another restore or count as an initial-display
bypass.

If `ApplicationRestart + DoNotActivate` moved the window to a saved virtual desktop, a later
`Activate()` or activating reveal may switch to that desktop. The app should request that activation
only when the switch is intended; a hidden window can instead use `AppWindow.Show(false)`.

`WindowInitialShowOptions` defaults to `Reason = Default`, `ActivationBehavior = Activate`, and
`KeepHidden = false`. Calls and property access remain UI-thread-bound, and calls after `Close()`
follow the existing closed-window error.

A first visible display with `DoNotActivate` raises `VisibilityChanged` without raising `Activated`
or moving content focus. `KeepHidden` raises neither event. The public `Show()` path performs the
same HWND-lifetime peg that first `Activate()` performs today.

The new single-window project template uses the form above to identify its launch window explicitly.

Existing apps can add only `PersistPlacementId` and keep calling `Activate()`. That path restores
ordinary placement and requests activation, but it does not infer `Launch` or apply the shell
monitor hint.

Every window whose consumed `InitialShowOptions` has `Reason = Launch` receives the shell monitor
hint, whether the consuming operation is `Show()` or `Activate()`. Null options, `Default`, and
`ApplicationRestart` do not apply it.

WinUI does not infer which windows are part of app launch from construction, initial operation,
placement opt-in, or activation order. The app supplies `Launch` for each applicable window.

Setting `Reason = ApplicationRestart` tells WinUI that the app is reconstructing that window from
its previous process instance:

The rules above produce this placement-policy table. The activation column is the assigned value
for either initial entry method; `KeepHidden` does not change the selected row.

| Reason | Activation behavior | Saved minimized state | Saved virtual desktop | Shell monitor hint |
|---|---|---|---|---|
| `Default` | `Activate` | Normalize | Ignore | Ignore |
| `Default` | `DoNotActivate` | Normalize | Ignore | Ignore |
| `Launch` | `Activate` | Normalize | Ignore | Apply |
| `Launch` | `DoNotActivate` | Normalize | Ignore | Apply |
| `ApplicationRestart` | `DoNotActivate` | Preserve | Restore best effort | Ignore |
| `ApplicationRestart` | `Activate` | Normalize | Ignore | Ignore |

An activation request in the initial options takes precedence over a saved minimized state and
virtual-desktop identity. Initial `ApplicationRestart + Activate` placement ignores the saved
desktop so showing the window does not unexpectedly switch the user's active desktop.

The app owns application restart registration, detection, and window inventory. It calls the Win32
`RegisterApplicationRestart` API according to its own policy and includes an app-defined command-line
marker. When launched with that marker, the app recreates each window that belonged to the previous
application instance, records which windows were minimized and which window had focus, assigns each
stable placement id, and supplies `ApplicationRestart` without requesting activation:

```csharp
foreach (SavedWindow savedWindow in restartState.Windows)
{
    Window window = CreateWindow(savedWindow);
    window.PersistPlacementId = savedWindow.PlacementId;
    window.InitialShowOptions = new WindowInitialShowOptions
    {
        Reason = WindowShowReason.ApplicationRestart,
        ActivationBehavior = WindowActivationBehavior.DoNotActivate,
    };
    window.Show();
}
```

The app can then call `Activate()` on the window that should receive focus. If every reconstructed
window was minimized, it does not activate one. An initial `Activate()` with these assigned options
has the same non-activating restart behavior as the example's initial `Show()`. A later
`Activate()` requests activation without reapplying placement policy. A window opened later by a
user action uses ordinary `Show()` or `Activate()`.

For non-activating restart, WinUI also attempts to return each window to its saved virtual desktop.
If that desktop no longer exists or the move fails, the window remains on the current desktop and
restart continues.

### 2.2. Window state that WinUI applies

WinUI restores these parameters together:

| State | v1 behavior |
|---|---|
| Normal position and size | Restored from the saved normal rectangle |
| Maximized state | A window closed maximized reopens maximized |
| Snapped state | The saved snap zone and frame bounds are restored |
| Monitor | Matched by GDI device name |
| Missing monitor | Placement relocates to the nearest surviving monitor |
| Work area | The window is kept on-screen within the current work area |
| DPI | Size is migrated to the target monitor's scale |
| Saved minimized state | Normalized for ordinary display; preserved by non-activating `ApplicationRestart` |
| Virtual desktop | Restored best effort only by non-activating `ApplicationRestart` |

An ordinary initial operation never restores a saved minimized state. The only v1 exception is an
explicit `ApplicationRestart` operation with `ActivationBehavior = DoNotActivate`. This prevents
an ordinary launch from appearing to do nothing while still allowing an app to reconstruct an
all-minimized restart inventory. The same non-activating restart path is the only one that restores
saved virtual-desktop identity.

The startup show command and shell monitor hint follow different rules. Win32 applies
`STARTUPINFO.wShowWindow` mechanically to the process's first `ShowWindow` call. WinUI does not track,
reinterpret, or reapply that command. Therefore `start /max` and `start /min` affect whichever HWND
the app shows first, including an in-process splash window. A later WinUI window does not receive the
command again.

This is a known limitation when a splash or non-WinUI HWND appears first: the user's startup show
command can affect that HWND rather than the window explicitly marked `Launch`. WinUI cannot
reliably determine whether another HWND has already consumed the native instruction because
`GetStartupInfo` continues to return the original value. Retargeting or reapplying the show command
requires a separate compatibility decision and prototype.

Normal packaged activation does not document a `STARTUPINFO` show-state contract. The design does
not depend on packaged activation supplying one; when `STARTF_USESHOWWINDOW` is absent, this rule has
no effect.

The shell monitor hint is not applied automatically by Win32 to an explicit restored rectangle.
WinUI applies it to every window explicitly shown with `Reason = Launch`. PlacementEx validates the
`HMONITOR` carried in `STARTUPINFO.hStdOutput` and migrates each window's saved or fallback placement
to the hinted monitor, including work-area and DPI adjustment.

Restore is fail-safe. Missing storage, a missing value, invalid data, an unsupported major format
version, or a PlacementEx apply failure all fall back to the normal first-show placement. No
placement failure is thrown to the app.

### 2.3. Window state that WinUI does not apply

v1 does not apply:

- FullScreen or CompactOverlay presenter state; see [Restore options](#461-restore-options).
- A saved minimized state during an ordinary show or activation.
- A saved virtual-desktop identity during ordinary display or any activating display.
- A cascade offset for a second process or window using the same id; see
  [Cascade duplicate instances](#462-cascade-duplicate-instances).
- The most recent move after a crash or task termination if no normal close or session-end save
  occurred; see [Save on settle](#463-save-on-settle).

FullScreen and CompactOverlay are intentionally app-controlled. A presenter can be temporary, such as
full-screen video, and the framework cannot infer that the user wants it on the next launch. When a
window closes in one of those presenters, WinUI saves the full overlapped rectangle cached by v1
rather than the presenter bounds. WinUI already tracks the restored size (client size plus chrome) for
presenter transitions, but not the restored X/Y position. v1 extends that tracking to retain the
position as well, producing the full last overlapped rectangle: position plus size. It does not save
the presenter kind or a pre-presenter show state.

This also defines the maximized-then-full-screen case. A full-screen window is not in the native
maximized state, so its current placement contains no maximized state to save. WinUI saves the full
overlapped rectangle described above, but it does not add separate tracking for the show state that
existed before the presenter change. The next launch therefore opens Overlapped at the tracked
windowed position and size.

### 2.4. Packaged apps

Packaged apps require no storage setup beyond `PersistPlacementId`. WinUI calls
`Microsoft.Windows.Storage.ApplicationData.GetDefault()` and writes one opaque string per persisted
window into the app's store. The appendix explains
[why WinUI uses `ApplicationData`](#414-why-applicationdata).

The storage layout is:

```text
LocalSettings
  Microsoft.UI.Xaml.WindowPlacement
    wp1_<readable-slug>_<hash> = <base64 placement blob>
```

`LocalSettings` is backed by the package settings hive. Windows removes that store when the package
is uninstalled.

Apps can reset all saved placements by deleting the `Microsoft.UI.Xaml.WindowPlacement` container
through `ApplicationData`. They should not hardcode the underlying registry path. WinUI does not
expose a delete method and does not delete an old entry when an app changes an id. A participating
window can recreate its entry on a later save; clear its `PersistPlacementId` before deleting the
container when the reset must survive the current session.

### 2.5. Unpackaged apps

Unpackaged apps use the same `PersistPlacementId` property, but they must first establish the
process-wide default `ApplicationData` identity. This requires a Windows App SDK storage API, shown
here with a placeholder name:

```csharp
protected override void OnLaunched(LaunchActivatedEventArgs args)
{
    // Proposed Windows App SDK API. The final name and signature are not committed.
    ApplicationData.SetDefaultForUnpackaged("Contoso", "CoolApp");

    var window = new MainWindow();
    window.InitialShowOptions = new WindowInitialShowOptions
    {
        Reason = WindowShowReason.Launch,
    };
    window.Show();
}
```

The identity must be established before any participating window resolves placement storage. WinUI
calls only `ApplicationData.GetDefault()`; it does not derive identity from the executable name,
AUMID, window class, or assembly. This proposal adds no XAML storage API.

If no usable store exists, restore and save are quiet no-ops. WinUI emits a debug breadcrumb and
does not throw. If the Windows App SDK capability does not ship in time, v1 is packaged-only.

The unpackaged app owns the identity and its lifecycle. Its installer or uninstaller must remove the
corresponding settings data when appropriate. Cleanup should use `ApplicationData`, not a hardcoded
registry path. The minimum Windows App SDK version is the first version that ships the required
default-store setup API. Storage validation uses 2.3.1 or later because 2.3.1 is the first public
version that writes unpackaged `LocalSettings` to the corrected registry root. The
[storage measurements](#421-unpackaged-localsettings) record the supporting experiments.

### 2.6. Other kinds of apps

| App type | Guidance |
|---|---|
| New single-window app | Set one stable id, assign `WindowInitialShowOptions` with `Reason = Launch`, then call `Show()` |
| Existing single-window app | Add `PersistPlacementId`; retain `Activate()` for ordinary restore |
| Multi-window app | Give each window a stable id and assign `WindowInitialShowOptions` with `Reason = Launch` to every window shown as part of app launch |
| Application restart | Recreate the previous inventory; configure each window for `ApplicationRestart` and `DoNotActivate`, then call `Show()` |
| App that adjusts placement before display | Assign `WindowInitialShowOptions` with `KeepHidden = true`, call `Show()`, adjust `AppWindow`, then reveal with `Show()`, `Activate()`, or `AppWindow.Show(false)` |
| Multi-instance app using one id | Every instance restores to the same placement; v1 does not [cascade duplicates](#462-cascade-duplicate-instances) |
| Launcher, splash screen, or fixed-position utility | Leave `PersistPlacementId` empty |
| Kiosk or app-controlled layout | Do not opt in; continue applying layout explicitly |
| App using WinUIEx persistence | Remove one of the persistence mechanisms to avoid competing restore and save behavior |
| Classic UWP app | Continue using UWP's existing window persistence; this API applies to WinUI `Window` |
| XAML Island or plugin host | Each WinUI `Window` follows the same property and default-store rules; the host must initialize the Windows App SDK runtime correctly (see the [deployment caveat](#422-deployment-caveat)) |

Two windows that intentionally represent different content should not share an id. The app already
needs a content identity for those windows and can use a stable, non-sensitive form of that identity
as part of `PersistPlacementId`.

Processes that use the same app store and id share one placement slot. v1 does not coordinate their
writes; the last successful save wins.

### 2.7. Kinds of launches

Window topology and launch kind are independent. The app supplies only the information WinUI cannot
infer:

| Launch kind | App action | WinUI policy |
|---|---|---|
| New app, one main window | Set a stable id, assign `WindowInitialShowOptions` with `Reason = Launch`, then call `Show()` | Apply ordinary placement, normalize minimized state, apply the shell monitor hint, request activation by default |
| New app, main window plus satellite | Mark both windows `Launch` when both are shown as part of app launch | Apply the monitor hint to both windows while preserving each saved relative placement |
| New app, N peer windows | Mark every initial app-launch window `Launch` | Apply the monitor hint independently to every marked window |
| Existing app adopting one property | Set `PersistPlacementId` and retain `Activate()` | Restore ordinary placement without inferring launch reason |
| Registered application restart | Recreate the saved window inventory; show each with `ApplicationRestart` and normally `DoNotActivate` | Skip launch information; preserve saved minimization; restore virtual desktop best effort; let the app activate one window afterward |
| Window opened later by user action | Use parameterless `Show()` or `Activate()` | Apply ordinary policy; do not reuse launch or restart context |

For a single-window application restart, the same explicit restart path applies; the app reconstructs
one inventory entry instead of several. A main-plus-satellite or N-window app reconstructs whichever
windows belong to its saved inventory. WinUI does not decide which app windows should exist.

Regular launch policy never restores saved minimized or virtual-desktop state. A native startup show
command can still request minimization through Win32's first-`ShowWindow` behavior described above.

## 3. Design

### 3.1. Loading the persistence data

WinUI attempts restore once, during the first `Show()` or `Activate()` reached from `Not attempted`. At
the start of that operation it snapshots and validates `InitialShowOptions`, or uses defaults when
the property is null. An unknown enum fails without changing state. An empty `PersistPlacementId`
skips persistence without resolving storage. The initial options still control visibility,
activation, and launch placement when the id is empty.

Each window tracks placement independently from display:

- **Not attempted:** the first `Show()` or `Activate()` still restores placement.
- **In progress:** a valid options snapshot exists and the outer operation owns the attempt.
  Reentrant `Show()`, `Hide()`, and `Activate()` calls are ignored. Reentrant property updates are
  accepted for inspection but cannot replace that snapshot.
- **Applied:** saved placement was applied; later display does not reapply it.
- **Attempted without placement:** the first operation found an empty id, no usable placement, or
  failed to apply it, or the HWND was displayed directly through AppWindow; later display uses the
  current fallback or app-adjusted placement without retrying.

`Hide()` does not transition this state machine. It hides whenever the HWND is currently visible,
including after a direct `AppWindow.Show()` bypass, and is otherwise a no-op.

After option validation succeeds, the first `Show()` or `Activate()` always ends the placement
opportunity, including when `KeepHidden` leaves the HWND invisible or the id is empty. Before any
window state changes, events, or native placement, WinUI enters `In progress` so reentrant calls
cannot start a second attempt or alter the outer verb. Every fail-safe exit from `In progress`
transitions exactly once to `Applied` or `Attempted without placement`; it never returns to
`Not attempted`.

An app can display the HWND directly through `Window.AppWindow.Show()`. That bypasses the WinUI
initial-placement path. WinUI records AppWindow visibility changes, observes native show messages,
and synchronously checks `IsWindowVisible` from the `InitialShowOptions` setter and before starting
`Show()` or `Activate()`. This covers an immediate setter call without relying on message-pump
timing. The detector can transition only `Not attempted` to `Attempted without placement`;
observations during `In progress` cannot replace the outer operation's state. WinUI-owned native
show calls are tagged and suppressed from bypass classification. A direct display from
`Not attempted` closes `InitialShowOptions` for behavior, so a later assignment or mutation has no
effect and a later `Show()` or `Activate()` uses normal post-attempt behavior. WinUI does not later
move that live window.

Each window resolves its store at most once, during its first enabled restore or save operation. The
window caches that result, including "no usable store," for its lifetime. This prevents restore from
using one store and save from another. An id set after the initial placement attempt resolves the
store at the first save and does not trigger a restore.

WinUI resolves storage only through `ApplicationData.GetDefault()`. An unpackaged app must establish
that process default through the Windows App SDK before any participating window resolves storage. A
new process resolves the current default again. Regular app updates retain placement when the app
identity is unchanged. If the package identity or unpackaged publisher/product identity changes, the
app starts fresh in the new store; WinUI does not search or migrate the old store.

The restore load path is:

1. Snapshot `InitialShowOptions`; if null, use default values.
2. Validate the snapshot. Invalid app-supplied values fail without consuming the attempt or changing
   window state.
3. Enter `In progress`.
4. Apply pending `Width` and `Height` normally. They remain the fallback if no placement is applied.
5. Read the current `PersistPlacementId`. An empty id skips steps 6-10 and continues with initial
   display policy.
6. Resolve and cache `ApplicationData.GetDefault()`. Failure or no usable store means no saved
   placement.
7. Convert the raw id into the deterministic `LocalSettings` value name.
8. Read the value from the `Microsoft.UI.Xaml.WindowPlacement` container.
9. Base64-decode and parse the TLV blob.
10. Treat a missing value, read failure, invalid base64, corrupt field, or unknown major version as no
   saved placement.
11. If no saved placement exists and `Reason = Launch`, seed a fallback placement by calling
   `PlacementEx::GetPlacement` after pending size is applied, using the capture option that skips the
   virtual-desktop COM query. Normalize `SW_HIDE` or any unsupported captured show command to
   `SW_NORMAL` and clear structural `FullScreen` plus call-time-only flags. This gives the shell
   monitor hint valid geometry to migrate consistently on native and downlevel paths.
12. Apply reason policy to the saved or captured fallback placement. If neither exists, continue
    through the normal initial-operation path.
13. When valid saved placement supersedes pending `Width` and `Height`, clear those pending values
   before placement or presenter callbacks can observe them.
14. Apply current size constraints.
15. Apply placement before first paint.

For the first `Show()` or `Activate()` with `KeepHidden = true`, WinUI applies placement while
keeping the window hidden and stops before visibility and activation. Later calls continue through
the normal visibility and activation path. The appendix explains
[why KeepHidden is part of the initial show options](#419-why-initialshowoptions).

Within the resolved `LocalSettings` store, the value is under this registry key:

```text
<LocalSettings root>\Microsoft.UI.Xaml.WindowPlacement
  wp1_<readable-slug>_<hash> = <base64 placement blob>
```

For an unpackaged app using Windows App SDK 2.3.1 or later, the full key is:

```text
HKCU\SOFTWARE\Classes\Local Settings\SOFTWARE\<publisher>\<product>\Microsoft.UI.Xaml.WindowPlacement
```

The [storage measurements](#421-unpackaged-localsettings) record the Windows App SDK version and
registry-path behavior behind this location.

For a packaged app, the same container is a subkey in the package-private registry hive at
`%LOCALAPPDATA%\Packages\<PackageFamilyName>\Settings\settings.dat`; it does not have a stable public
HKCU path. Code and cleanup use `ApplicationData`, not either backing path.

The raw id is never used directly as a settings value name. WinUI maps it as follows:

```text
valueName = "wp1_" + slug(rawId) + "_" + Base32(SHA-256(UTF-16LE(rawId)))
```

`slug` scans left to right, keeps only ASCII `[A-Za-z0-9]`, and stops after 16 accepted characters.
If no characters are accepted, it uses the string `"id"`. SHA-256 is computed over the raw `HSTRING`'s UTF-16
code units, interpreted as little-endian bytes with no trailing NUL. `microsoft.ui.xaml.dll` calls
`CryptHashCertificate2` with `BCRYPT_SHA256_ALGORITHM`. This hashes an arbitrary byte block through
CNG using `Crypt32.dll`, which the binary already imports. It avoids both a new `bcrypt.lib` link
dependency and a private SHA-256 implementation. The hash provides a stable, negligibly
collision-prone value-name component; it is not used for authentication, integrity, or
confidentiality. A collision would select the same placement slot within the app's settings store,
not cross an application security boundary.
Base32 uses the uppercase RFC 4648 alphabet with no padding. Standard Base64
and Base64url use both uppercase and lowercase letters; because `LocalSettings` value names are
case-insensitive, distinct encodings could compare as the same name. Hexadecimal would also be safe
but would use 64 characters for the digest instead of Base32's 52. The input is normally a short id
and hashing occurs only when resolving a persistence value name, so its cost is negligible relative
to the `LocalSettings` operation that follows.

For example:

| Piece | Value |
|---|---|
| Raw id | `DocumentWindow:doc42` |
| Slug | `DocumentWindowdo` |
| Value name | `wp1_DocumentWindowdo_<base32-hash>` |

The readable slug is diagnostic only and is not confidential. The hash supplies uniqueness and
makes arbitrary app strings syntactically safe as value names; it does not hide the slug. Fail-safe
paths emit an `OutputDebugString` breadcrumb and an ETW event containing the operation and error
code, not the raw id, placement blob, coordinates, or virtual-desktop id.

### 3.2. Applying the persistence data: PlacementEx

PlacementEx is a header-only C++ library that packages the Windows team's current guidance for
capturing and restoring window placement. It handles details that are easy to get wrong in
hand-written Win32 code, including monitor matching and relocation, keep-on-screen behavior, DPI
migration, and snapped-window restoration. WinUI uses PlacementEx for those native placement
mechanics while retaining ownership of public API policy, storage, and the durable format.

The approved PlacementEx source is checked into the
[windowplacement component](../../dxaml/xcp/components/windowplacement/README.md). It was copied from
the Microsoft OS repository at commit `f0d35ebfccaae9bdec48e2eb29e08f6224579209`.

The copied files use the WinUI MIT license. WinUI owns their review, servicing, and maintenance like
other source in this repository.

The imported guidance provides additional background:

- [Remembering Window Positions](../../dxaml/xcp/components/windowplacement/inc/PlacementEx/RememberingWindowPositions.md)
- [Win32 Windowing Concepts](../../dxaml/xcp/components/windowplacement/inc/PlacementEx/Win32Concepts.md)

WinUI reconstructs a PlacementEx placement from the parsed fields and applies WinUI policy before
calling PlacementEx.

Virtual-desktop support requires WinUI to define `USE_VIRTUAL_DESKTOP_APIS` before including
PlacementEx, link `onecoreuapuuid.lib` and `OneCore_Forwarder_ole32.lib`, and make COM calls from an
appropriately initialized apartment. The public virtual-desktop APIs can return
`TYPE_E_ELEMENTNOTFOUND` before the taskbar registers a new window and
`RPC_E_CANTCALLOUT_ININPUTSYNCCALL` from an input-synchronous message. The prototype must also
determine whether a dedicated COM helper thread is required to avoid UI-thread reentrancy or
deadlock.

WinUI therefore caches the last successfully queried virtual-desktop id with a deferred query after
the first visible display and refreshes it at safe UI-thread opportunities, including activation
and normal close when the thread is not inside an input-synchronous message.
WinUI adds a PlacementEx call-time capture option that skips the virtual-desktop COM query. It uses
that option from `WM_ENDSESSION` and overlays the cached id onto the captured geometry. If no valid
cached id is available, WinUI omits `TAG_VIRTUAL_DESKTOP_ID`. Capture and restore remain best effort.

#### 3.2.1. PlacementEx fields used by v1

| Stored field | PlacementEx field | Use |
|---|---|---|
| `TAG_NORMAL_RECT` | `normalRect` | Restored screen-coordinate rectangle |
| `TAG_WORK_AREA` | `workArea` | Saved monitor work area used for relocation and fitting |
| `TAG_ARRANGE_RECT` | `arrangeRect` | Actual frame bounds of a snapped window |
| `TAG_DPI` | `dpi` | Saved DPI used for scale migration |
| `TAG_SHOW_CMD` | `showCmd` | Normal, maximized, or minimized native show state |
| `TAG_FLAGS` | selected `PlacementFlags` | Durable maximize, snap, sizing, and off-screen intent |
| `TAG_DEVICE_NAME` | `deviceName` | Saved monitor identity |
| `TAG_VIRTUAL_DESKTOP_ID` | `virtualDesktopId` | Restored best effort only by non-activating application restart |

WinUI gets these values from `PlacementEx::GetPlacement`, not from similar-looking Win32 APIs:

- `normalRect` is in screen coordinates. `GetWindowPlacement().rcNormalPosition` can be in workspace
  coordinates.
- `arrangeRect` uses extended frame bounds rather than `GetWindowRect`, avoiding invisible resize
  borders that would misalign a restored snap.
- `dpi` follows PlacementEx's DPI-virtualization rules.

#### 3.2.2. Policy before apply

1. Pending `Width` and `Height` remain the fallback. A valid saved placement overrides them and
   clears the pending values so a later presenter transition cannot reapply stale size. Current
   `MinWidth`, `MaxWidth`, `MinHeight`, and `MaxHeight` constrain the restored size when those APIs
   are available.
2. `Default` calls
   `PlacementEx::AdjustForMainWindow(nullptr, StartupInfoFlags::None)`. This normalizes a saved
   minimized state and removes virtual-desktop identity from the placement being applied without
   changing the saved blob. Before that call, WinUI converts minimized `RestoreToArranged` state to
   normal `Arranged` state so a window minimized from snap reopens snapped. PlacementEx already
   converts `RestoreToMaximized` to maximized.
3. Every initial placement attempt whose snapshot has `Reason = Launch` and has valid saved or
   captured fallback placement calls
   `PlacementEx::AdjustForMainWindow(nullptr, StartupInfoFlags::MonitorHint)`. Passing `nullptr`
   tells PlacementEx to read process `STARTUPINFO`. PlacementEx validates `hStdOutput` as an
   `HMONITOR` and migrates that window's saved or fallback placement to the hinted monitor. The
   monitor hint is reusable; every window explicitly marked `Launch` receives the same adjustment.
   WinUI performs the same minimized-from-arranged conversion before this call.
4. Null options and `Default` ignore the monitor hint. Both `Show()` and `Activate()` honor a
   snapshotted `Reason = Launch`. WinUI does not infer launch reason from construction, display,
   persistence, or activation order.
5. `KeepHidden` does not change the policy selected by the assigned `ActivationBehavior`.
   `ApplicationRestart` with `DoNotActivate` does not call `AdjustForMainWindow`, preserving a saved
   minimized state and virtual-desktop identity.
   PlacementEx applies the geometry, moves the window to the saved desktop best effort, and does not
   activate it. PlacementEx cloaks the HWND while applying multi-step placement so the move should
   not become user-visible. `ApplicationRestart` with effective `Activate` uses the `Default`
   adjustment because an activation request takes precedence over saved minimization and desktop
   identity. Neither form applies the monitor hint.
6. WinUI strips virtual-desktop identity from every ordinary or activating apply. If the saved
   desktop no longer exists or the move fails during non-activating restart, PlacementEx leaves the
   window on the current desktop and WinUI continues.
7. WinUI intentionally does not pass `StartupInfoFlags::ShowCommand`. Win32 applies
   `STARTUPINFO.wShowWindow` to the process's first underlying `ShowWindow` call. WinUI does not
   reinterpret or reapply that command.
8. Current HWND construction uses `WS_VISIBLE` with a special `SW_HIDE` create parameter and can
   consume the first-`ShowWindow` policy before public `Show()` or `Activate()`. The implementation
   must prototype hidden construction without `WS_VISIBLE`. Because this changes startup timing for
   existing apps, API ratification treats it as a compatibility decision. If non-participating apps
   observe a difference, the implementation must preserve their old path or gate the new behavior
   behind `XamlOptionalChanges`; a runtime feature gate alone is not a compatibility contract.
9. The existing `m_bInitialWindowActivation` combines display, placement, visibility, sizing, and
   activation state. Split those concepts so a first non-activating display can apply placement,
   raise visibility, retain HWND lifetime, and avoid moving focus.
10. On the native `ApplyWindowAction` path, WinUI passes PlacementEx `KeepHidden` and `NoActivate` as
    separate call-time instructions. Public `KeepHidden = true` passes both; visible
    `DoNotActivate` passes `NoActivate` only. `KeepHidden` suppresses visibility and `NoActivate`
    suppresses activation. Neither replaces the assigned `ActivationBehavior` for placement policy.
    `VirtualDesktopId` also implies `NoActivate`.
11. PlacementEx's downlevel path does not implement `NoActivate`. For public `KeepHidden` or visible
    `DoNotActivate`, WinUI therefore applies while cloaked and internally hidden. If public
    `KeepHidden` is false, WinUI then reveals the already placed HWND with
    `SetWindowPos(SWP_SHOWWINDOW | SWP_NOACTIVATE)` without changing its normal, maximized, minimized,
    or snapped state. The prototype must prove that this sequence does not expose activation, focus
    transfer, or flashing.
12. Null or default option values make either initial entry method request visibility and
    activation. `DoNotActivate` and `KeepHidden` are honored identically by the first `Show()` or
    `Activate()`. WinUI invokes the full PlacementEx apply path before the window becomes
    user-visible.

PlacementEx then performs:

- monitor matching by device name;
- relocation to a surviving monitor;
- keep-on-screen fitting;
- DPI migration;
- snap restoration;
- native `ApplyWindowAction` on supported systems; and
- its downlevel fallback on older supported systems.

If snapping is disabled in Windows settings, a saved snapped window returns to its normal rectangle.
If PlacementEx cannot apply the placement, WinUI continues with the normal first-show placement.

#### 3.2.3. PlacementEx state not persisted by v1

| State or facility | v1 treatment |
|---|---|
| PlacementEx string serialization | Not used; see [why WinUI owns the serialized format](#415-why-winui-owns-the-serialized-format) |
| `KeepHidden`, `NoActivate`, `NoApplyWindowAction` | Call-time instructions, never persisted |
| `FullScreen` | Not persisted or restored; see [Why presenter state is absent from v1](#416-why-presenter-state-is-absent-from-v1) |
| Structural full-screen detection | Disabled; WinUI substitutes the full overlapped rectangle cached by v1 |
| Presenter kind | Not persisted or restored; see [Why presenter state is absent from v1](#416-why-presenter-state-is-absent-from-v1) |

WinUI owns the serialized meaning of durable flags and translates them at the PlacementEx boundary.
It does not persist PlacementEx's internal flags value verbatim.

### 3.3. Saving the persistence data

WinUI saves during normal close and session end:

| Trigger | v1 |
|---|---|
| Accepted normal close | Save after `Closed` declines to cancel and before framework teardown |
| Other native destruction (`WM_DESTROY`) | Best-effort backstop before the last-window early return |
| `WM_ENDSESSION` with `wParam == TRUE` | Save for logoff and shutdown |
| `WM_ENDSESSION` with `wParam == FALSE` | Do not save; shutdown was cancelled |
| Move/resize settle | Do not save |
| Crash or task termination | No save point is available |

A caption close, Alt+F4, or another system close request first arrives as `WM_CLOSE`;
`Window.Close()` enters the same framework close path directly. Both raise the app's `Closed` event.
If the app handles that event, WinUI cancels the close and does not save. Otherwise WinUI captures
after the handler returns and before `PrepareToClose`, content removal, WindowChrome disconnection,
or `Shutdown()`. Changes the handler makes to live placement or `PersistPlacementId` intentionally
win when close proceeds.

`AppWindow.Destroy()`, external `DestroyWindow`, or destructor teardown can bypass this close path.
WinUI therefore also saves at the start of `WM_DESTROY` as a best-effort backstop, before the
existing last-window `PostQuitMessage` early return. A per-window close-path latch prevents
`WM_DESTROY` from repeating a save already performed by normal close. It does not suppress the
intentional `WM_ENDSESSION` plus close sequence described below.

The save path is:

1. If placement is still `Not attempted` and the HWND was never displayed directly, do not save.
   This prevents a constructed-but-unused window from replacing good data with default HWND geometry.
2. Read the current `PersistPlacementId`. Empty means no save.
3. Use the window's cached store result. For late opt-in, resolve and cache the store now.
4. Capture PlacementEx geometry while the top-level HWND and cached placement state are valid. Skip
   the virtual-desktop COM query for input-synchronous messages.
5. If the window is FullScreen or CompactOverlay, replace the current bounds with WinUI's full
   overlapped rectangle, including the restored X/Y position added by v1. Do not save presenter kind
   or pre-presenter show state.
6. Canonicalize show state to `SW_NORMAL`, `SW_SHOWMAXIMIZED`, or `SW_SHOWMINIMIZED`. Never persist
   `SW_HIDE`. For a hidden HWND, use its last cached non-hidden show state. A `KeepHidden` apply
   seeds that cache from the applied placement. If neither a visible nor applied placement has
   supplied a show state, use `SW_NORMAL`.
7. Add the last safely cached virtual-desktop id when one is available. v1 applies it only during
   non-activating application restart.
8. Translate only the durable PlacementEx flags into WinUI wire bits.
9. Serialize the v1 TLV blob and base64-encode it.
10. Derive the value name from the current id and write the string to `LocalSettings`.

The id is read live. If an app restored from `MainWindow` and changes the property to
`MainWindow-v2`, the next save writes the new slot and leaves the old slot unchanged.

Close and session end can both save during shutdown. The writes are intentionally not deduplicated:
the process may be terminated after `WM_ENDSESSION` without reaching normal teardown. One redundant
small settings write is safer than depending on the later message.

A crash or task kill can lose the most recent in-session move because v1 does not save on every
move/resize settle. The next launch uses the last placement saved by close or session end.

Capture, serialization, and storage failures never throw to the app.

### 3.4. Persistence data format

WinUI stores one base64 string containing a versioned binary TLV envelope. The app does not inspect or
modify this value. The appendix explains [why WinUI owns this format](#415-why-winui-owns-the-serialized-format)
rather than persisting PlacementEx serialization.

| Layer | Representation | Purpose |
|---|---|---|
| Outer | Base64 text | Safe `LocalSettings` string and an obvious opaque implementation detail |
| Inner | Header followed by TLV fields | Versioning and field-by-field evolution |

The binary layout is:

| Part | Layout |
|---|---|
| Header | ASCII `WPL1` (4 bytes), major `u16`, minor `u16`, total blob length `u32` |
| Field | tag `u16`, length `u16`, then `length` value bytes |

The v1 header is 12 bytes. Its total length includes the header and must exactly match the decoded
byte count. No field count is stored: the reader advances through each bounds-checked TLV until it
reaches that exact boundary. v1 writes major `1`, minor `0`, little-endian. Rect coordinates are
signed `i32`; other scalar values are unsigned `u32`. Device names are raw UTF-16LE without a trailing
NUL. A virtual-desktop id uses the Windows `GUID` field layout: little-endian `u32 Data1`,
little-endian `u16 Data2`, little-endian `u16 Data3`, and the eight `Data4` bytes.

#### 3.4.1. v1 tags

| Tag | Value | Wire value | Length |
|---|---:|---|---:|
| `TAG_NORMAL_RECT` | `0x0001` | four `i32`: left, top, right, bottom | 16 |
| `TAG_WORK_AREA` | `0x0002` | four `i32` | 16 |
| `TAG_ARRANGE_RECT` | `0x0003` | four `i32` | 16 |
| `TAG_DPI` | `0x0010` | `u32` | 4 |
| `TAG_SHOW_CMD` | `0x0011` | `u32` Win32 `SW_*` value | 4 |
| `TAG_FLAGS` | `0x0012` | `u32` WinUI-owned bitfield | 4 |
| `TAG_DEVICE_NAME` | `0x0020` | UTF-16LE code units, no NUL | 0-62 |
| `TAG_VIRTUAL_DESKTOP_ID` | `0x0021` | Windows `GUID` field layout | 16 |

`TAG_NORMAL_RECT`, `TAG_WORK_AREA`, and `TAG_DPI` are required because PlacementEx validity depends
on all three. The writer emits `TAG_ARRANGE_RECT` only when `Arranged` or `RestoreToArranged` is set,
emits `TAG_DEVICE_NAME` only when non-empty, and emits `TAG_VIRTUAL_DESKTOP_ID` only when a valid
cached id exists. Other tags are optional.

#### 3.4.2. Durable flag bits

| Bit | Meaning |
|---:|---|
| `0x0001` | `RestoreToMaximized` |
| `0x0002` | `Arranged` |
| `0x0004` | `AllowPartiallyOffScreen` |
| `0x0008` | `AllowSizing` |
| `0x0020` | `RestoreToArranged` |

These are WinUI wire meanings. Their initial values match the current PlacementEx values to simplify
translation, but the storage contract belongs to WinUI. Bit `0x0010` is intentionally unused because
the analogous PlacementEx bit is the call-time-only `KeepHidden` instruction. PlacementEx
`FullScreen` (`0x0040`) is not persisted. Virtual-desktop identity uses its dedicated tag rather than
a durable `0x0080` flag, and `NoActivate` (`0x0100`) is also call-time only. During non-activating
application restart, WinUI sets the PlacementEx `virtualDesktopId` field and synthesizes
`PlacementFlags::VirtualDesktopId` from a valid tag; every other path clears both.

The writer emits applicable tags once, in ascending order, producing a canonical blob suitable for
a checked-in golden test. The parser does not require that order.

#### 3.4.3. Compatibility

The reader follows fail-safe rules:

- Missing `TAG_NORMAL_RECT`, `TAG_WORK_AREA`, or `TAG_DPI` means no saved placement.
- A missing optional tag uses its default: normal show state, zero flags, or PlacementEx's default
  matching behavior.
- Unknown tags are skipped using their encoded length.
- A higher minor version with major version `1` is accepted; unknown additive tags are skipped.
- Recognized normal, maximized, and minimized aliases are canonicalized. `SW_HIDE`, another
  non-displaying command, or an unknown `TAG_SHOW_CMD` value is treated as an absent tag and defaults
  to `SW_NORMAL`.
- Unknown `TAG_FLAGS` bits are ignored and never passed through to PlacementEx.
- `Arranged` or `RestoreToArranged` requires a valid `TAG_ARRANGE_RECT`.
- Duplicate tags are accepted with last-one-wins semantics, although the WinUI writer never emits
  duplicates.
- A known fixed-width tag with the wrong length rejects the whole blob.
- Every present rectangle tag must have positive width and height, and every coordinate must be
  within +/-1,000,000 pixels.
- `TAG_DPI` must be at least 96; a checked DPI-scaling overflow rejects the blob.
- Device-name length must be even and no greater than 62 bytes. Zero length is treated as absent;
  embedded NUL characters reject the blob.
- Virtual-desktop ids must be exactly 16 bytes. `GUID_NULL` is treated as an absent tag.
- Every field is bounds-checked before it is read.
- A total length that does not equal the decoded byte count rejects the whole blob.
- The final field must end exactly at the total-length boundary.
- The decoded blob size is capped at 4 KiB before allocation.
- The reconstructed PlacementEx value must pass `PlacementEx::IsValid()` before policy or apply.

Format evolution uses both tags and the header version:

- **Additive field:** add a new optional tag and increment the minor version. Old readers skip it;
  new readers default it when reading an old blob.
- **New enum value:** define how old readers default the known tag and increment the minor version.
- **Must-understand semantic change:** increment the major version. Old readers reject the blob and
  use normal placement rather than confidently applying incomplete state.
- **Removed field:** stop emitting its tag. Readers continue accepting older blobs that contain it.

For example, the proposed [future restore options](#461-restore-options) can add presenter state as an
optional tag. v1 blobs contain no presenter state and restore Overlapped. After a newer release saves
once, the blob contains the new state. No migration of the v1 layout is required.

An unreadable blob costs one default-position launch. The next successful close rewrites a valid blob
for the running WinUI version.

## 4. Appendix

### 4.1. Decision rationale

#### 4.1.1. Why one string property

The id is the opt-in. A separate `PersistPlacement` Boolean creates two invalid states:

- `PersistPlacement = true` with no id silently cannot save.
- A non-empty id with `PersistPlacement = false` is dead configuration.

Using one id property as the opt-in makes both states unrepresentable. This matches WinUIEx and the
experimental AppWindow surface, which also use identity as the enablement signal.

#### 4.1.2. Why the app supplies the id

An automatically derived class name, `x:Name`, or type name is a code identifier that developers
rename freely. Attaching persistent user data to it would turn an ordinary refactor into an
unannounced data migration and strand the old entry. Native peer-type experiments also showed that
the obvious class-name lookup does not consistently return a C# subclass name.

The app knows which windows represent stable user concepts. It supplies a stable, non-sensitive id
and can combine a readable window-category prefix with an opaque document or workspace identity in
multi-window apps.

#### 4.1.3. Why `PersistPlacementId`

The original proposal used `PersistPlacementAs`. The selected name keeps the opt-in action visible
while making clear that the value is an app-owned id:

```xml
PersistPlacementId="MainWindow"
```

`PersistenceId` is shorter and matches WinUIEx, but it is less explicit about what is persisted.
This design selects `PersistPlacementId`; final naming remains subject to API review.

#### 4.1.4. Why `ApplicationData`

WinUI should not invent a storage location or identity. `ApplicationData` already defines storage,
scope, and cleanup. WinUI calls only `ApplicationData.GetDefault()` for packaged and unpackaged apps.
An unpackaged app establishes the process default through a Windows App SDK storage API before
placement is first used.

Keeping setup in the storage layer avoids a public
`Microsoft.UI.Xaml -> Microsoft.Windows.Storage` metadata dependency and avoids exposing a
placement-specific storage override in XAML.

#### 4.1.5. Why WinUI owns the serialized format

PlacementEx's serialization was provisional and is not a durable user-storage contract. WinUI needs
saved data written by one release to remain readable by later releases. A versioned TLV format also
lets new fields be added without fixing C++ struct layout, padding, or enum values in storage.

#### 4.1.6. Why presenter state is absent from v1

Closing in FullScreen or CompactOverlay does not prove that the app or user wants that presenter next
launch. It may be a temporary video, slideshow, or game mode. v1 therefore saves only the tracked
overlapped placement.

Saving an unused presenter tag now would add code and tests solely to activate old transient state in
a later release. The TLV format already permits a future feature to add presenter state. Old blobs
will default to Overlapped and gain the new tag on their next save.

#### 4.1.7. Why launch reason is explicit

WinUI has no semantic main-window concept. The first WinUI window created, the first top-level HWND
created, and the first placement-enabled window can all be different. A splash or helper window can
also appear first. Any order-based rule can misclassify which windows are part of app launch.

New project templates assign `WindowInitialShowOptions` with `Reason = Launch` before calling
`Show()`. A multi-window app supplies the same reason for every initial window that should receive
app-launch placement behavior. Existing apps can add only `PersistPlacementId` and keep
`Activate()` for ordinary restore,
but they do not receive the shell monitor hint until they configure the explicit launch reason. The
reason affects placement only; it creates no ownership, lifetime, shutdown, or general main-window
semantics.

Application restart is also explicit because several windows may be reconstructed and each needs
per-window minimized-state policy.

#### 4.1.8. Why `Window.Show()` and `Window.Hide()`

Existing `Window.Activate()` serves as WinUI's initial display entry point and requests activation.
Parameterless `Window.Show()` adds the familiar WPF display verb. The first call to either method
honors `InitialShowOptions`; with default options, both display and request activation. On an
already displayed window, `Show()` is a no-op while `Activate()` retains current behavior and calls
`SetActiveWindow()`. This includes a minimized window: WPF defines `Show()` as setting
`Visibility.Visible`, and its show helper returns when the window is already visible rather than
restoring it.

`InitialShowOptions` is required because application reconstruction may need to display several
windows without activating each one, including a previous instance where every window was minimized.
`Reason` carries launch or restart placement policy; `ActivationBehavior` independently carries
activation policy for the initial `Show()` or `Activate()`. The app reconstructs with
`DoNotActivate`, then calls `Activate()` on one window when appropriate.

`Window.Hide()` completes the WPF-style visibility pair. It hides without closing or destroying
the HWND, and a later `Show()` reuses the same window and content. WinUI already exposes equivalent
visibility through `AppWindow.Hide()`, but adding `Window.Show()` without `Window.Hide()` would
leave the `Window` surface asymmetric and require apps to switch abstractions for the reverse
operation.

Adding `Show()` or `Hide()` to unsealed `Window` can hide an app-defined member and can take
precedence over an extension method with the same name. API review must accept that
source-compatibility cost. MIDL `method_name("ShowDefault")` and `method_name("HideDefault")` avoid
the existing private `ShowImpl` and `HideImpl` implementation names.

#### 4.1.9. Why `InitialShowOptions`

Some apps need to inspect or adjust restored placement before the HWND becomes visible.
`Window.InitialShowOptions.KeepHidden` exposes that advanced step without weakening the common path:

```csharp
window.PersistPlacementId = "MainWindow";
window.InitialShowOptions = new WindowInitialShowOptions
{
    KeepHidden = true,
};
window.Show();

// Optionally adjust the applied placement.
window.AppWindow.Move(...);
window.Activate(); // Or use window.AppWindow.Show(false) to reveal without activation.
```

An explicit apply method and a `Show(options)` overload were considered. A separate apply method
would duplicate placement options such as reason and future collision policy.

**Why not `Show(options)`?** The overload keeps configuration local to the call, but an overload by
itself leaves the existing `Activate()` adoption path unable to consume the same placement policy.
Keeping both an overload and a property would create two configuration channels and require
precedence rules when both are supplied. Adding `Activate(options)` would expand the API again.
`InitialShowOptions` instead gives both entry verbs one snapshot and lets existing parameterless
`Activate()` calls consume the same placement, visibility, and activation policy as `Show()`.

Late assignment is accepted rather than throwing because mutation through an already assigned or
shared options object cannot be rejected per window. Both forms remain valid property operations and
are behaviorally inert after that window snapshots its values.

On systems with `ApplyWindowAction`, WinUI passes both `KeepHidden` and `NoActivate`; `KeepHidden`
alone suppresses only visibility. The downlevel fallback can internally call
`ShowWindow(SW_SHOWNOACTIVATE)`, cloak the HWND while applying multi-step placement, and hide it
again. For visible `DoNotActivate`, WinUI uses the hidden apply plus non-activating reveal described
in Section 3.2.2. PlacementEx tests verify that restored, maximized, and arranged windows remain
invisible on both paths. WinUI must additionally prove that the downlevel path does not expose XAML
visibility or activation events, move focus, flash, or incorrectly consume startup placement. WinUI
suppresses direct-display detection around its own native show calls so the fallback cannot
transition its in-progress attempt to `Attempted without placement`.

### 4.2. Storage measurements and constraints

#### 4.2.1. Unpackaged `LocalSettings`

Tests with `ApplicationData.GetForUnpackaged(publisher, product)` established:

- `LocalSettings` creates its backing registry key on first write.
- No pre-provisioned directory or registry key is required for settings.
- `LocalFolder` has different provisioning requirements, but placement does not use it.
- The unpackaged root moved between earlier Windows App SDK versions as a bug fix.
- Windows App SDK 2.3.1 is the first public version using the corrected location.

The registry path is an implementation detail and has already changed. Documentation and cleanup
must use `ApplicationData`, never a literal registry path.

#### 4.2.2. Deployment caveat

A framework-dependent unpackaged app can resolve a newer projection than the runtime selected by the
bootstrapper and receive `E_NOINTERFACE` for `IApplicationDataStatics2`. Self-contained deployment
with undocked registration-free WinRT avoids that mismatch. The error resembles a missing API, so the
sample and test coverage must make the runtime requirement explicit.

#### 4.2.3. Prototype and test requirements

A prototype proved that `microsoft.ui.xaml.dll` can read and write Windows App SDK
`ApplicationData.LocalSettings` across the ABI. The remaining ship bar includes:

**Storage and format**

- packaged save/restore self-test;
- platform-configured unpackaged default storage in framework-dependent and self-contained deployment;
- no-store quiet no-op;
- unpackaged default established before first placement use and configured too late;
- late-opt-in save;
- id change and empty-id behavior;
- missing required tags, invalid rectangles and DPI, arrange-flag dependencies, unknown flags and
  tags, canonical show commands including hidden HWNDs, and unknown-major blobs; and
- golden value-name and v1 blob compatibility, including diagnostic redaction.

**Placement**

- normal, maximized, minimized, and snapped restore, including minimized-from-maximized and
  minimized-from-snapped normalization;
- pending Width/Height and current min/max constraints against saved placement;
- monitor removal and DPI migration;
- native and forced downlevel PlacementEx apply paths, including apply failure;
- virtual-desktop build/link prerequisites and COM apartment or helper-thread behavior without
  deadlock; and
- FullScreen/CompactOverlay capture using the full overlapped rectangle cached by v1.

**Launch and compatibility**

- hidden HWND construction without `WS_VISIBLE`;
- native launcher max/min behavior for each first-shown-HWND ordering, including an in-process splash;
- explicit `Launch` monitor migration with and without saved placement;
- no-saved-placement `Launch` fallback capture with virtual-desktop query suppressed and `SW_HIDE`
  normalized on native and downlevel paths;
- multiple `Launch` windows each applying the same monitor hint while preserving relative placement;
- no launch-reason inference from a persisted activation, including a non-persisted splash and
  multiple persisted windows; and
- non-participating apps retaining first-`ShowWindow` behavior or using the
  `XamlOptionalChanges` fallback.

**Initial display**

- `WindowInitialShowOptions` null/default behavior, snapshot timing, shared objects, and
  post-consumption assignment and mutation;
- desktop behavior plus UWP-mode `E_NOTIMPL` for new APIs without changing existing `Activate()`;
- unknown enum validation leaving the attempt unconsumed and allowing correction and retry;
- wrong-thread and closed-window property errors;
- reentrant `Show()` and `Activate()` being ignored while the outer attempt is `In progress`;
- both initial entry methods honoring every `InitialShowOptions` value;
- parameterless `Show()` and `Activate()` as the initial operation, while already visible, after
  `AppWindow.Hide()`, and while already displayed but minimized;
- `Hide()` before the initial operation, while visible, while minimized, while already hidden, and
  after close;
- direct `AppWindow.Show()` followed by `Window.Hide()`;
- reentrant `Hide()` while initial `Show()` or `Activate()` is `In progress`;
- `Hide()` followed by `Show()`, `Activate()`, and `AppWindow.Show(false)`;
- visibility and deactivation event behavior for active, inactive, and already-hidden windows;
- saving while hidden, preserving the last non-hidden normal, maximized, minimized, or snapped
  placement;
- non-activating initial-operation visibility, focus, event ordering, and lifetime on native and forced
  downlevel hidden-apply/reveal paths;
- direct `AppWindow.Show()` bypass, including an immediate setter, a show/hide sequence, WinUI-owned
  downlevel show calls, and observations during `In progress`; and
- generated public `ShowDefaultImpl` and `HideDefaultImpl` alongside private `ShowImpl` and
  `HideImpl`, including a derived `Window` with app-defined methods and existing
  `Show(this Window)` and `Hide(this Window)` extension methods.

**Hidden apply**

- `KeepHidden` with every reason, activation behavior, and entry verb, verifying that the assigned
  activation behavior still selects placement policy;
- restored, maximized, minimized, and snapped placement on native and forced downlevel paths;
- empty id, missing store or value, corrupt data, and apply failure;
- no `VisibilityChanged`, `Activated`, focus transfer, taskbar flash, or user-visible HWND;
- app move, resize, and presenter changes before reveal; and
- activating and non-activating reveal without a second restore, including their minimized-state
  behavior.

**Application restart**

- ordinary and `ApplicationRestart` minimized-state behavior;
- multiple application-restart windows, including an all-minimized set and an ordinary window opened
  later in the restarted process;
- non-activating application restart across multiple virtual desktops;
- virtual-desktop query before taskbar registration, safe cache refresh, and `WM_ENDSESSION` use of
  the cache without an input-synchronous COM call;
- missing saved virtual desktop falling back to the current desktop;
- no focus stealing, visible flashing, or active-desktop switch while restoring background windows;
- virtual-desktop capture and apply on native and downlevel paths; and
- activating and non-activating reveal after a window was restored to a saved virtual desktop.

**Save timing and failure handling**

- constructed-but-never-displayed windows not overwriting saved placement;
- accepted and cancelled close;
- capture and storage-write failures remaining fail-safe without throwing;
- normal close plus the one-shot `WM_DESTROY` backstop;
- `AppWindow.Destroy()`, external `DestroyWindow`, and destructor teardown;
- successful and cancelled `WM_ENDSESSION`; and
- intentional `WM_ENDSESSION` plus close double-write behavior.

`PlacementFlags::NoApplyWindowAction` can force PlacementEx's downlevel path on current Windows for
direct comparison with native `ApplyWindowAction`.

### 4.3. Prior art

#### 4.3.1. WPF

WPF exposes `Window.RestoreBounds` and `WindowState`, but the app must save them. A rectangle alone
does not preserve monitor identity, snap state, or DPI migration. WPF commonly saves from `Closing`,
which does not cover every logoff and shutdown path.

#### 4.3.2. WinUIEx

The [WinUIEx implementation](https://github.com/dotMorten/WinUIEx/blob/main/src/WinUIEx/WindowManager.cs)
provides the closest developer experience:

- set `WindowManager.PersistenceId` before first activation, or set the equivalent `WindowEx`
  property in XAML;
- restore on the first `WM_SHOWWINDOW`;
- use packaged `Windows.Storage.ApplicationData.Current` automatically or supply a static
  `IDictionary<string, object>` for an unpackaged app;
- save a raw `WINDOWPLACEMENT` plus an exact monitor-layout snapshot when `Window.Closed` fires;
- normalize a saved minimized state to visible, while preserving restore-to-maximized state;
- suppress WinUI's `WM_DPICHANGED` resize while applying saved placement; and
- cache the full overlapped `WINDOWPLACEMENT` in memory as position, size, state, or presenter
  changes.

If a window enters FullScreen and closes, WinUIEx reopens it using the previous overlapped position,
size, and show state. This includes preserving a maximized state that preceded FullScreen. It does
not restore the FullScreen presenter itself.

WinUIEx deliberately declines restore when the monitor count or any monitor rectangle changes. It
stores monitor names but does not use them to match a monitor. It stores no saved DPI for logical-size
migration; its restore instead suppresses WinUI's DPI adjustment while `SetWindowPlacement` runs.
Its move/size timer refreshes only the in-memory overlapped cache; persistence storage is written on
close. The blob has no explicit version header, does not restore snap zones or virtual desktops, and
does not save on session end.

#### 4.3.3. Experimental AppWindow placement APIs

The Windows App SDK currently contains experimental `AppWindow` placement APIs:

- `PersistedStateId`;
- `PlacementRestorationBehavior`;
- `GetCurrentPlacement`;
- `SetCurrentPlacement`;
- `SaveCurrentPlacement`;
- `SaveCurrentPlacementForAllPersistedStateIds`; and
- `AppWindowPlacementDetails`.

Both designs use PlacementEx for geometry. WinUI v1 does not depend on the experimental surface
because WinUI needs a supported API commitment, its own string id, its storage policy,
session-end saving, and the visible-window/launcher precedence defined above.

It is a design goal to move WinUI's implementation to the AppWindow placement APIs if they become
stable and meet these requirements. `AppWindowPlacementDetails` carries every durable v1 field except
the saved virtual-desktop id. WinUI can map its string id to a stable name-based GUID, translate flags
by meaning rather than numeric value, and migrate old blobs by reading the WinUI value once and
writing through AppWindow. Until AppWindow can carry virtual-desktop identity, WinUI retains that
field in a sidecar. The public WinUI contract and saved-placement identity must remain stable across
that implementation change.

### 4.4. Unpackaged default-store dependency

The [unpackaged app guidance](#25-unpackaged-apps) shows the app model. The unresolved dependency is
a Windows App SDK API that establishes the process-wide default store before WinUI calls
`ApplicationData.GetDefault()`. The app owns that identity and its cleanup. If the Windows App SDK
capability is unavailable, placement persistence is packaged-only.

### 4.5. Rejected alternatives

#### 4.5.1. App-owned persistence engine

One proposal exposed capture and apply methods plus a save-requested event so the app could store the
opaque bytes anywhere. It works for every app and gives maximum control, but creates the largest API
surface and requires every app to wire storage and lifecycle behavior. The selected design retains
WinUI ownership of the difficult and repetitive parts.

#### 4.5.2. AUMID-based storage identity

Another proposal asked unpackaged apps to provide an AUMID and had WinUI derive storage from it. It
was rejected because:

- an AUMID is one opaque shell identifier, while `GetForUnpackaged` requires separate publisher and
  product values;
- no defensible general mapping exists;
- shell identity is not a storage key;
- cleanup ownership is unclear; and
- the feasibility path was never proven.

WinUI does not guess by splitting an AUMID or by hashing it into publisher/product values.

#### 4.5.3. Default-on persistence

Default-on requires a stable id for windows that never opted in. Class name, `x:Name`, and type name
all bind user data to refactorable code identifiers. No stable framework-owned identity source has
been found, so default-on is not planned.

### 4.6. Proposed future work

Nothing in this section is part of v1.

#### 4.6.1. Restore options

Presenter fidelity remains future work:

```midl
[flags]
enum WindowPlacementRestoreOptions
{
    None           = 0x0000,
    FullScreen     = 0x0001,
    CompactOverlay = 0x0002,
};

[webhosthidden]
runtimeclass Window
{
    WindowPlacementRestoreOptions PlacementRestoreOptions;
}
```

`None` preserves v1 behavior. FullScreen can use the target monitor bounds. CompactOverlay requires
an additional saved rectangle.

#### 4.6.2. Cascade duplicate instances

Two processes using the same id read the same placement and stack exactly. A possible follow-on:

```midl
enum WindowPlacementCollisionPolicy
{
    Exact = 0,
    Cascade,
};

[webhosthidden]
runtimeclass WindowInitialShowOptions
{
    WindowPlacementCollisionPolicy PlacementCollisionPolicy;
}
```

`Exact = 0` preserves v1 behavior when the property is added to a default-constructed options object.
The collision policy is an orthogonal initial-placement option, not a show reason. Keeping it in
`WindowInitialShowOptions` gives `Show()` and `Activate()` one evolvable placement-options surface. The
proposed implementation identifies same-app windows with a private HWND property, serializes
cross-process placement with a session-local named mutex, and offsets by a per-DPI caption/frame step
until it finds an unoccupied slot in the target monitor's work area. Maximized and FullScreen windows
are not cascaded.

#### 4.6.3. Save on settle

A debounced save after move or resize would preserve placement across crashes and task termination.
It should capture and write only. Monitor enumeration and validation remain restore-time work.

### 4.7. Glossary

| Term | Meaning |
|---|---|
| Arrange / snapped | A window docked to a system-defined screen zone |
| Arrange rectangle | The actual frame bounds of a snapped window, distinct from its normal rectangle |
| Normal / overlapped rectangle | The position and size used when the window is not minimized, maximized, or in another presenter |
| PlacementEx | Internal Windows helper for capture, monitor matching, DPI migration, snap restore, and safe apply |
| Presenter | AppWindow display mode such as Overlapped, FullScreen, or CompactOverlay |
| TLV | Tag-Length-Value, the field encoding inside WinUI's versioned blob |
| Initial show options | One-time placement, activation, and visibility policy consumed by the first `Show()` or `Activate()` |
| `ApplicationData` | Windows App SDK `Microsoft.Windows.Storage.ApplicationData`, not the UWP type with the same short name |