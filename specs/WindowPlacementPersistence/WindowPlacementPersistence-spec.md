Window placement persistence
===

# Background

WinUI has no built-in way to reopen a desktop window where the user left it. An app can close with
its main window maximized on a particular monitor and reopen at its default size and position on the
primary monitor. Classic UWP restored window placement automatically, so this is a visible
regression for apps moving to WinUI. The request is tracked by
[WinUI issue #2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680).

This spec adds an opt-in API that lets a `Microsoft.UI.Xaml.Window` save its placement when it
changes and restore that placement the next time the app creates a window with the same identity.
Placement means the window's restored (normal) rectangle, its maximized or normal show state, the
monitor it was on, per-monitor DPI, and the flags needed to restore those safely.

The design this spec implements is in
`docs/design-notes/Window-PlacementPersistence.md`. This spec covers the public API surface only.

## What "placement" includes

WinUI saves and restores:

- The restored (normal) window rectangle.
- Whether the window was maximized or normal.
- The monitor the window was on, and the work area and DPI used to interpret the rectangle.
- The flags required to restore the above onto a currently connected monitor.

WinUI does not save or restore window content, presenter kind (for example overlapped versus
full screen), z-order, per-control state, or minimized state except in the application-restart case
described below.

# API Pages

## Window.PersistPlacementId property

Gets or sets the identifier under which this window's placement is saved and restored. Setting a
non-empty value opts the window into placement persistence.

```csharp
public string PersistPlacementId { get; set; }
```

A non-empty value both enables persistence and names the saved placement slot. The default is null,
which disables persistence. There is no separate Boolean enable property.

Set `PersistPlacementId` before the first `Show()` or `Activate()` call so WinUI can restore the
placement before the window appears.

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

The app is responsible for keeping each window's `PersistPlacementId` stable across app updates for
as long as it wants to reuse the saved placement. Changing the value selects a new slot. WinUI does
not migrate or delete the old entry.

A multi-window app gives each independently restored window a distinct id:

```csharp
public DocumentWindow(string documentId)
{
    InitializeComponent();
    PersistPlacementId = $"DocumentWindow:{documentId}";
}
```

WinUI reads the property at each operation:

- Setting it after first display enables future saves. It does not move or retroactively restore the
  live window.
- Changing it after restore sends the next save to the new id. The old entry remains.
- Clearing it stops future saves.
- A window that never sets it follows the existing windowing path and never resolves placement
  storage.

The value is an arbitrary app string. WinUI does not use it directly as a storage key; it derives a
stable, case-insensitive storage value name from it. The id is not a secret and is not a security
boundary.

## Window.Show method

Displays the window. The parameterless overload requests activation. The overload that takes a
`WindowShowOptions` supplies one-time options for the window's initial display.

```csharp
public void Show();
public void Show(WindowShowOptions options);
```

`Show()` uses `WindowShowReason.Default` and requests activation. On an already visible window,
`Show()` is a no-op; use `Activate()` to request activation. If an app hid the window through
`AppWindow.Hide()`, a later `Show()` makes it visible and requests activation.

The first `Show(options)` call is the window's one placement attempt. It consumes that attempt even
when the id is empty, no saved value exists, the saved data is invalid, or placement otherwise fails.
`Reason` and `KeepHidden` apply only to this first attempt. A later `Show(options)` call ignores both
`Reason` and `KeepHidden`; it is a no-op while the window is visible, and after `AppWindow.Hide()` it
follows only `ActivationBehavior`.

A null `options` object or an unknown enum value fails with `E_INVALIDARG`. Both overloads are bound
to the UI thread. Calling either overload after `Close()` follows the existing closed-window error.

A first non-minimized display with `WindowActivationBehavior.DoNotActivate` raises
`VisibilityChanged` without raising `Activated` and without moving content focus.

### Show for app launch

An app identifies its launch window by passing `WindowShowReason.Launch`:

```csharp
window.PersistPlacementId = "MainWindow";
window.Show(new WindowShowOptions
{
    Reason = WindowShowReason.Launch,
});
```

Every window first shown with `Reason = Launch` receives the shell monitor hint (the monitor the
shell requests for launch). `Default`, the parameterless `Show()`, `Activate()`, and
`ApplicationRestart` do not apply the hint. WinUI does not infer which windows are part of launch;
the app supplies `Launch` for each applicable window.

Existing apps can add only `PersistPlacementId` and keep calling `Activate()`. That path restores
ordinary placement and requests activation. It does not infer `Launch` and does not apply the shell
monitor hint.

### Show for application restart

`WindowShowReason.ApplicationRestart` tells WinUI that the app is reconstructing a window from its
previous process instance. The app owns restart registration (the Win32
`RegisterApplicationRestart` API), detection, and the window inventory. For each window that belonged
to the previous instance, the app recreates the window, assigns its stable placement id, and shows it
without requesting activation:

```csharp
foreach (SavedWindow savedWindow in restartState.Windows)
{
    Window window = CreateWindow(savedWindow);
    window.PersistPlacementId = savedWindow.PlacementId;
    window.Show(new WindowShowOptions
    {
        Reason = WindowShowReason.ApplicationRestart,
        ActivationBehavior = WindowActivationBehavior.DoNotActivate,
    });
}
```

The app then calls `Activate()` on the window that should receive focus. Calling `Activate()`
normalizes a minimized window to visible.

The following table defines how `Reason` and `ActivationBehavior` combine. An activation request
takes precedence over a saved minimized state and over saved virtual-desktop identity.

| Reason | Activation behavior | Saved minimized state | Saved virtual desktop | Shell monitor hint |
|---|---|---|---|---|
| `Default` | `Activate` | Normalize to visible | Ignore | Ignore |
| `Default` | `DoNotActivate` | Normalize to visible | Ignore | Ignore |
| `Launch` | `Activate` | Normalize to visible | Ignore | Apply |
| `Launch` | `DoNotActivate` | Normalize to visible | Ignore | Apply |
| `ApplicationRestart` | `Activate` | Normalize to visible | Ignore | Ignore |
| `ApplicationRestart` | `DoNotActivate` | Preserve | Restore best effort | Ignore |

For non-activating restart, WinUI also attempts to return each window to its saved virtual desktop.
If that desktop no longer exists or the move fails, the window stays on the current desktop and
restart continues.

## WindowShowOptions class

Provides one-time options for a window's initial display, passed to `Window.Show(WindowShowOptions)`.

```csharp
public sealed class WindowShowOptions
{
    public WindowShowOptions();

    public WindowShowReason Reason { get; set; }
    public WindowActivationBehavior ActivationBehavior { get; set; }
    public bool KeepHidden { get; set; }
}
```

Defaults are `Reason = WindowShowReason.Default`, `ActivationBehavior =
WindowActivationBehavior.Activate`, and `KeepHidden = false`.

### WindowShowOptions.Reason property

Selects the initial placement policy. See the `Show` method tables above.

### WindowShowOptions.ActivationBehavior property

Selects whether showing the window requests activation. This is independent of `Reason`.

### WindowShowOptions.KeepHidden property

Applies the selected placement policy while leaving an initially hidden window hidden. `KeepHidden`
lets an app inspect or adjust `Window.AppWindow` before the window becomes visible:

```csharp
window.PersistPlacementId = "MainWindow";
window.Show(new WindowShowOptions
{
    Reason = WindowShowReason.Launch,
    KeepHidden = true,
});

// Optionally adjust the applied placement before it becomes visible.
window.AppWindow.Move(...);
window.Show();
```

When `KeepHidden` is `true`, WinUI applies the selected placement policy but does not make the window
visible, request activation, move focus, or raise `VisibilityChanged` or `Activated`. `KeepHidden`
implies no activation regardless of `ActivationBehavior`. It applies only to the initial
`Show(options)` call and never hides a window that is already visible. A later `Show()` or
`Activate()` makes the window visible without loading or applying persisted placement again, which
preserves any adjustment the app made.

## WindowShowReason enum

Specifies why a window is being shown for the first time.

| Member | Value | Meaning |
|---|---|---|
| `Default` | 0 | Ordinary display. |
| `Launch` | 1 | The window is being shown as part of app launch. |
| `ApplicationRestart` | 2 | The app is reconstructing the window after application restart. |

## WindowActivationBehavior enum

Specifies whether showing a window requests activation.

| Member | Value | Meaning |
|---|---|---|
| `Activate` | 0 | Show the window and request activation. |
| `DoNotActivate` | 1 | Show the window without requesting activation. |

# API Details

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
    runtimeclass WindowShowOptions
    {
        WindowShowOptions();

        WindowShowReason Reason;
        WindowActivationBehavior ActivationBehavior;
        Boolean KeepHidden;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    unsealed runtimeclass Window
    {
        // ... existing APIs ...

        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        {
            String PersistPlacementId;

            [method_name("ShowDefault")]
            void Show();

            [method_name("ShowWithOptions")]
            void Show(WindowShowOptions options);
        }
    };
}
```

# Storage

WinUI stores a saved placement in the app's `ApplicationData.LocalSettings`, in a container named
`Microsoft.UI.Xaml.WindowPlacement`. The value name is derived from the `PersistPlacementId` and the
value is a Base64-encoded binary blob.

## Value name

The `PersistPlacementId` is an arbitrary app string, so WinUI does not use it directly as a
case-insensitive value name. The value name is:

```
valueName = "wp1_" + slug(PersistPlacementId) + "_" + Base32(SHA-256(UTF-16LE(PersistPlacementId)))
```

- `slug` keeps up to 16 ASCII alphanumeric characters from the id, or `id` if none qualify. It is
  diagnostic only.
- The uppercase, unpadded RFC 4648 Base32 of the SHA-256 digest (32 bytes, 52 characters) supplies
  uniqueness and makes any app string safe as a value name.

For example, `PersistPlacementId = "DocumentWindow:doc42"` produces a value name that begins with
`wp1_DocumentWindowdo_` followed by the 52-character Base32 hash.

## Value format

The value is a versioned tag-length-value binary blob, Base64-encoded. WinUI owns this format.

```
Header (12 bytes)
  char   magic[4]     'W' 'P' 'L' '1'
  uint16 major        must-understand version; a reader rejects an unknown major
  uint16 minor        additive version; a reader tolerates an unknown minor
  uint32 totalLength  whole blob length in bytes, including this header

Field (repeated until totalLength)
  uint16 tag
  uint16 length       length of value in bytes
  byte   value[length]
```

All integers are little-endian. Rectangle coordinates are signed `int32`; other scalar values are
unsigned `uint32`. A device name is raw UTF-16LE code units with no trailing NUL. A virtual-desktop
id uses the Windows `GUID` field layout.

The writer emits each present field once, in ascending tag order, so a given placement produces one
canonical byte sequence. The reader is fail-safe: any malformed, truncated, unknown-major, or
oversized blob is treated as "no saved placement" rather than a partial restore. The reader skips
unknown tags by their length and ignores unknown flag bits and unknown show commands, so a newer
minor version stays readable by an older reader. A blob is only usable if it contains the normal
rectangle.

The v1 tags are:

| Tag | Id | Value | Notes |
|---|---|---|---|
| Normal rect | 0x0001 | 4 x int32 | Required. left, top, right, bottom. |
| Work area | 0x0002 | 4 x int32 | Work area used when the placement was saved. |
| Arrange rect | 0x0003 | 4 x int32 | Rectangle for an arranged (snapped) window. |
| DPI | 0x0010 | uint32 | Per-monitor DPI at save time. |
| Show command | 0x0011 | uint32 | Win32 `SW_*` show state. |
| Flags | 0x0012 | uint32 | Durable placement flags (see below). |
| Device name | 0x0020 | UTF-16LE | GDI device name of the saved monitor. |
| Virtual desktop id | 0x0021 | GUID | Saved virtual-desktop identity. |

The durable flag bits are:

| Flag | Bit |
|---|---|
| Restore to maximized | 0x0001 |
| Arranged | 0x0002 |
| Allow partially off screen | 0x0004 |
| Allow sizing | 0x0008 |
| Restore to arranged | 0x0020 |

Bits outside this set are never written and are ignored on read.

# Remarks

## Packaged and unpackaged apps

Placement persistence uses `ApplicationData` and is available to packaged apps. Unpackaged support
depends on a Windows App SDK unpackaged `ApplicationData` store; where that store is unavailable,
setting `PersistPlacementId` resolves no storage and the window follows the ordinary windowing path.

## Threading and lifetime

`PersistPlacementId` and both `Show` overloads are bound to the window's UI thread. The public `Show`
path performs the same HWND-lifetime peg that the first `Activate()` performs today.

# Implementation status

- The public API surface (the `Window` members, `WindowShowOptions`, `WindowShowReason`, and
  `WindowActivationBehavior`) is defined here. Its full native wiring, code generation, and
  end-to-end behavior land incrementally.
- The WinUI-owned serialized format and value-name derivation are implemented and unit tested in
  `dxaml/xcp/components/windowplacement`, including a byte-exact golden-blob test.
- PlacementEx, the shared placement math, is owned by the Windows OS repository and is ported into
  `dxaml/xcp/components/windowplacement/inc/PlacementEx`.
- Unpackaged storage depends on a Windows App SDK unpackaged `ApplicationData` store.
