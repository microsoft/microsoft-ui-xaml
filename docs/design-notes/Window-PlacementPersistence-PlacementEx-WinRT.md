# Public WinRT placement data for WinUI Window

> **AI-assisted document:** This exploratory design was written with AI assistance. AI makes
> mistakes; technical claims require human review.

## 1. Status

This document extends
[WinUI Window Placement Persistence](Window-PlacementPersistence.md) with an optional public
placement value. It describes the current local proposal in
[`specs/window-placement-persistence-spec.md`](../../specs/window-placement-persistence-spec.md).

The common packaged-app experience remains one property:

```xml
<Window PersistPlacementId="MainWindow" />
```

```csharp
window.Activate();
```

WinUI loads, applies, and saves placement automatically. An app does not need to use the advanced
API.

The advanced layer adds:

- a mutable, detached `WindowPlacement` value containing every durable placement field WinUI
  understands;
- `Window.TryGetPlacement` to capture current placement; and
- `Window.TrySetInitialPlacement` to supply placement before the initial operation.

This proposal does not add a builder, manager, public serialization contract, or custom-storage
event.

The broader `Window` proposal also adds `Hide()` as the counterpart to `Show()`. It hides without
closing and does not participate in initial placement selection.

## 2. Goals

1. Keep automatic persistence as the primary experience.
2. Let an app inspect and change all durable placement data.
3. Support one-time migration from legacy placement data.
4. Support copying placement between WinUI windows.
5. Use one native placement policy for automatic and explicit placement.
6. Keep transient display instructions separate from durable placement data.
7. Avoid exposing PlacementEx's native layout or implementation-only behavior.

## 3. Non-goals

- A general `AppWindow` placement API.
- Public access to WinUI's serialized placement format.
- An app-supplied persistence store in v1.
- Continuous persistence while a window moves or resizes.
- Full-screen or compact-overlay state persistence.
- Moving a live window by mutating a `WindowPlacement` object.

## 4. Proposed API

The following MIDL is conceptual. Names and contracts require API review.

```midl
namespace Microsoft.UI.Xaml
{
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
    enum WindowActivationBehavior
    {
        Activate = 0,
        DoNotActivate = 1,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    runtimeclass WindowInitialShowOptions
    {
        WindowInitialShowOptions();

        WindowShowReason Reason;
        WindowActivationBehavior ActivationBehavior;
        Boolean KeepHidden;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowPlacementShowState
    {
        Normal = 0,
        Maximized = 1,
        Minimized = 2,
    };

    [flags]
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowPlacementFlags
    {
        None = 0x0000,
        RestoreToMaximized = 0x0001,
        Arranged = 0x0002,
        AllowPartiallyOffScreen = 0x0004,
        Resizable = 0x0008,
        RestoreToArranged = 0x0020,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    runtimeclass WindowPlacement
    {
        WindowPlacement(
            Windows.Graphics.RectInt32 normalRect,
            Windows.Graphics.RectInt32 workArea,
            Int32 dpi);

        WindowPlacement(WindowPlacement source);

        Windows.Graphics.RectInt32 NormalRect;
        Windows.Graphics.RectInt32 WorkArea;
        Int32 Dpi;
        WindowPlacementShowState ShowState;
        WindowPlacementFlags Flags;
        String DisplayDeviceName;
        Windows.Foundation.IReference<
            Windows.Graphics.RectInt32> ArrangeRect;
        Windows.Foundation.IReference<Guid> VirtualDesktopId;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    unsealed runtimeclass Window
    {
        // ... existing APIs ...

        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        {
            String PersistPlacementId;
            WindowInitialShowOptions InitialShowOptions;

            Boolean TryGetPlacement(out WindowPlacement placement);
            Boolean TrySetInitialPlacement(WindowPlacement placement);

            [method_name("ShowDefault")]
            void Show();

            [method_name("HideDefault")]
            void Hide();
        }
    };
}
```

`WindowPlacement` is a mutable, detached value. Changing it does not affect a live window.
`Window` remains UI-thread-bound and owns the internal state required to capture and apply
placement.

## 5. Easy path: automatic persistence

A non-empty `PersistPlacementId`:

- enables automatic placement persistence;
- selects the saved placement in the app's default `ApplicationData` store;
- restores before first paint; and
- saves after accepted close, during window destruction as a backstop, and at session end.

```csharp
var window = new Window
{
    PersistPlacementId = "MainWindow",
};
window.Activate();
```

Null or empty disables automatic persistence. It does not disable explicit placement.

Automatic persistence currently requires package identity because WinUI uses the app's default
`ApplicationData` store. If that store is unavailable, automatic load and save are no-ops.

## 6. Advanced path: capture and explicit placement

### 6.1. Capture

`Window.TryGetPlacement` captures the window's current durable placement:

```csharp
if (window.TryGetPlacement(out WindowPlacement placement))
{
    // placement is detached from window
}
```

The method returns `false` and sets `placement` to null when the window has no valid native window
or WinUI cannot produce a valid placement.

Capture combines current native state with internal per-window history:

- normal rectangle;
- work area and DPI;
- maximized or minimized state;
- snapped rectangle;
- minimized-from-maximized state;
- minimized-from-snapped state;
- most recent overlapped geometry for full-screen and compact-overlay windows; and
- last safely queried virtual-desktop id.

The returned object is a snapshot. Mutating it does not move the source window.

### 6.2. Transform and reuse

An app can change the snapshot directly:

```csharp
if (sourceWindow.TryGetPlacement(out WindowPlacement placement))
{
    placement.ShowState = WindowPlacementShowState.Normal;
    placement.VirtualDesktopId = null;

    targetWindow.TrySetInitialPlacement(placement);
    targetWindow.Activate();
}
```

`TrySetInitialPlacement` copies the supplied data. Later mutation of the object does not alter the
staged placement.

### 6.3. Construct from app data

An app can construct a placement for migration:

```csharp
var placement = new WindowPlacement(
    legacyNormalRect,
    currentWorkArea,
    currentDpi)
{
    ShowState = legacyWasMaximized
        ? WindowPlacementShowState.Maximized
        : WindowPlacementShowState.Normal,
    DisplayDeviceName = legacyMonitorName,
};

window.TrySetInitialPlacement(placement);
window.Activate();
```

The three-argument constructor creates a normal placement with no flags, arrange rectangle, monitor
identity, or virtual-desktop identity.

The copy constructor creates an independent snapshot containing all placement data from the source:

```csharp
var copy = new WindowPlacement(original);
```

## 7. Placement opportunity and precedence

Each `Window` has one initial placement opportunity.

Internal state:

- **Not attempted**
  - explicit or automatic placement can still be selected;
- **In progress**
  - options and placement are snapshotted;
  - reentrant `Show()` and `Activate()` are ignored;
- **Applied**
  - placement was applied; or
- **Attempted without placement**
  - no usable placement existed, apply failed, or direct display bypassed the pipeline.

`TrySetInitialPlacement` returns `true` only in **Not attempted**. It stages placement; it does not
move or show the native window immediately.

The first placement-aware `Show()` or `Activate()` consumes the opportunity. Direct
`Window.AppWindow.Show()` consumes it without applying placement.

Selection order:

1. Explicit placement supplied through `TrySetInitialPlacement`.
2. Automatic placement loaded through `PersistPlacementId`.
3. Existing fallback placement.

Explicit placement wins only for the current attempt. If `PersistPlacementId` is non-empty,
automatic save still captures the window's final placement under that id.

After selection, explicit and automatic placement enter the same initial-show policy pipeline:

- `ActivationBehavior = Activate` normalizes minimized state;
- launch can apply the shell monitor hint;
- virtual-desktop identity is normally ignored; and
- non-activating application restart can preserve minimized state and restore virtual desktop.

Changing `PersistPlacementId` after the opportunity does not move the live window. The new id is
used for the next automatic save, and the old entry remains unchanged.

## 8. WindowPlacement data contract

`WindowPlacement` projects PlacementEx's durable meaning, not its native layout.

| WinRT property | PlacementEx source | Treatment |
|---|---|---|
| `NormalRect` | `normalRect` | Required restored position and size |
| `WorkArea` | `workArea` | Required original monitor work area |
| `Dpi` | `dpi` | Required; minimum 96 |
| `ShowState` | `showCmd` | Canonical Normal, Maximized, or Minimized |
| `Flags` | selected `PlacementFlags` | Durable bits translated by meaning |
| `DisplayDeviceName` | `deviceName` | Optional monitor identity |
| `ArrangeRect` | `arrangeRect` | Optional; required by arranged flags |
| `VirtualDesktopId` | `virtualDesktopId` | Optional and best effort |

The rectangles use physical screen coordinates. `Dpi` records the coordinate scale at capture so
PlacementEx can preserve logical window size when applying the placement to a monitor with a
different DPI.

### 8.1. Durable flags

`WindowPlacementFlags` exposes:

- `RestoreToMaximized`;
- `Arranged`;
- `AllowPartiallyOffScreen`;
- `Resizable`; and
- `RestoreToArranged`.

It excludes PlacementEx state that is not durable placement data:

- `KeepHidden` and `NoActivate` are initial-display instructions;
- `NoApplyWindowAction` is an implementation test control;
- `FullScreen` is a live presentation state; and
- native `VirtualDesktopId` presence is represented by the nullable property.

### 8.2. Validation

Properties may temporarily form an invalid combination while an app edits a placement.
`TrySetInitialPlacement` validates and copies the complete object atomically.

Validation includes:

- `NormalRect` and `WorkArea` have positive dimensions;
- coordinates are within supported bounds;
- `Dpi` is at least 96 and within supported bounds;
- `ShowState` is defined;
- `Flags` contains only known bits;
- `Arranged` or `RestoreToArranged` requires a valid `ArrangeRect`;
- `DisplayDeviceName` is within the supported monitor-name limit; and
- `Guid.Empty` is canonicalized to no virtual-desktop identity.

Null or invalid input fails with `E_INVALIDARG` (`ArgumentException` in .NET). A `false` return from
`TrySetInitialPlacement` means only that the placement opportunity has passed.

`TryGetPlacement` never returns invalid placement. It returns `false` if capture cannot meet the
same invariants.

## 9. Show and Activate policy

The advanced API does not change the selected design's
[reason and activation policy](Window-PlacementPersistence.md#21-api-shape).

The first `Show()` or `Activate()` uses the same `InitialShowOptions` snapshot. Both methods honor
`Reason`, `ActivationBehavior`, and `KeepHidden`. `KeepHidden` suppresses activation because the
window remains hidden.

After that initial operation, the options are consumed. A later `Show()` displays and requests
activation for a hidden window without reapplying placement, and is a no-op while the window is
visible, including while minimized. A later `Activate()` displays a hidden window, restores a
minimized window, and uses the existing WinUI activation path, including `SetActiveWindow()`,
without reapplying placement.

`Hide()` hides a currently visible window, including one displayed directly through `AppWindow`,
without closing it, saving placement, or resetting options. It is a no-op while hidden and does not
consume an open placement opportunity. A reentrant call during an initial operation is ignored.

Hiding raises the normal visibility event and, when applicable, existing deactivation event
behavior. If placement is saved while the window is hidden, capture uses the last non-hidden
placement. A `KeepHidden` apply seeds that value from its applied placement.

Preserving minimized state or virtual-desktop identity during application restart requires
`ApplicationRestart` with `DoNotActivate`, regardless of whether `Show()` or `Activate()` consumes
the initial opportunity.

## 10. Threading and lifetime

`Window.TryGetPlacement` and `Window.TrySetInitialPlacement` have the same UI-thread affinity as
the `Window` instance.

`WindowPlacement` is detached from the native window. It contains no HWND, event source, or
reference back to `Window`. Its projected threading model still requires API review.

The `Window` implementation owns:

- placement-opportunity state;
- copied explicit placement;
- arranged and minimized transition history;
- cached overlapped geometry;
- cached virtual-desktop id;
- automatic storage resolution;
- close and session-end integration; and
- direct-display detection.

## 11. Automatic storage remains opaque

The public value does not expose `Serialize` or `TryDeserialize`. Automatic storage continues to
use WinUI's internal versioned format.

This separation:

- lets WinUI add internal fields without making a public wire-format promise;
- avoids requiring old app code to preserve unknown serialized tags;
- avoids exposing untrusted-input parsing as a new public surface; and
- keeps custom storage out of the initial API.

Public serialization and a save callback can be evaluated separately if custom storage becomes a
required scenario.

## 12. Relationship to experimental AppWindow placement APIs

Windows App SDK experimental builds contain:

- `PersistedStateId`;
- `PlacementRestorationBehavior`;
- `GetCurrentPlacement`;
- `SetCurrentPlacement`;
- `SaveCurrentPlacement`;
- `SaveCurrentPlacementForAllPersistedStateIds`; and
- `AppWindowPlacementDetails`.

Both designs use PlacementEx for geometry. This proposal additionally defines:

- one-property XAML opt-in on `Window`;
- behavior of the first `Window.Show()` or `Window.Activate()`;
- application string identity;
- WinUI storage and session-end policy; and
- virtual-desktop state.

`AppWindowPlacementDetails` carries every durable v1 field except virtual-desktop identity. If the
AppWindow APIs stabilize and satisfy these requirements, WinUI can use them underneath this public
contract. WinUI can translate flags by meaning and retain virtual-desktop identity as sidecar
state if necessary.

## 13. Implementation delta

The existing prototype already contains:

- automatic `PersistPlacementId` storage;
- initial-operation state and policy;
- PlacementEx integration;
- internal versioned serialization;
- close and session-end saving; and
- internal placement capture and application.

This API design additionally requires:

1. A projected mutable `WindowPlacement`.
2. Conversion between `WindowPlacement` and the internal durable representation.
3. Public capture through `Window.TryGetPlacement`.
4. Copying, validation, and staging through `Window.TrySetInitialPlacement`.
5. Explicit-placement precedence over automatic load.
6. A public `Window.Hide()` path over the existing native-window visibility mechanism.

No public manager object or builder is required. Existing per-window state remains internal to
`Window`.

## 14. Test requirements

In addition to the selected design's tests:

- construct every valid placement shape;
- reject each invalid field and invalid field combination;
- capture every durable state;
- capture returns a detached object;
- mutation does not move the source window;
- accepted placement is copied rather than retained by reference;
- capture from one window and stage on another;
- explicit placement overrides automatic load;
- automatic save follows explicit restore;
- `TrySetInitialPlacement` returns false after each path that consumes the opportunity;
- full-screen and compact-overlay capture returns overlapped placement;
- virtual-desktop presence and absence round-trip through the public value;
- all public flag meanings map correctly to PlacementEx;
- call both methods from the wrong thread; and
- hide before and after the initial operation, hide twice, hide while minimized, and show again;
- direct `AppWindow.Show()` followed by `Window.Hide()`;
- reentrant `Hide()` during each initial operation;
- visibility and deactivation events for active, inactive, and already-hidden windows;
- save while hidden using the last non-hidden placement;
- existing one-property automatic persistence remains unchanged.

## 15. Alternatives considered

### 15.1. Immutable placement plus builder

An immutable value guarantees that every observable object is valid, but transforming one field
requires a second public type and a `Build()` step. The proposed mutable snapshot is simpler.
Validation occurs atomically when a window accepts the placement.

### 15.2. WindowPlacementManager

A separate manager was useful in the broader design that included save events, custom storage,
serialization, and capability state. With only capture and initial staging, it is a two-method
facade over one `Window`. Putting those methods on `Window` is more direct and avoids another
public lifetime and threading contract.

### 15.3. Operations on WindowPlacement

Capture and application need a live window, its native HWND, UI thread, and initial-placement
lifecycle. `WindowPlacement` is data and should not own operations on a target window.

### 15.4. Direct PlacementEx projection

PlacementEx contains transient instructions, test controls, full-screen operations, monitor
selection, and native policy methods. The WinRT type projects durable data by meaning so the native
implementation remains replaceable.

### 15.5. Public serialization and custom storage

Public serialization makes the wire format a long-term compatibility contract and requires rules
for preserving future unknown fields. A save event must also define synchronous work during close
and session end. These scenarios are deferred from v1.

## 16. Assessment

The proposal keeps the primary requirement simple:

> A packaged WinUI app gets automatic placement persistence by setting one property.

The public placement value is an optional advanced layer for migration, transformation, testing,
and cross-window reuse. It exposes one additional runtime class, two advanced placement methods,
and `Hide()` on `Window`, while leaving storage, serialization, and native placement policy
internal.
