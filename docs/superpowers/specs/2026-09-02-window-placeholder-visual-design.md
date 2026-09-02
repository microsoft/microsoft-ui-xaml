# Top-Level Window Placeholder Visual Design

## Goal

When `XamlChangeId.SkipWindowRedirectionSurface` is enabled, create WinUI top-level
windows with `WS_EX_NOREDIRECTIONBITMAP` while still guaranteeing that DWM has
meaningful, theme-appropriate content before the window is shown.

The feature remains fully contained behind `XamlOptionalChanges`. Disabled behavior
is unchanged.

## Approaches Considered

### DesktopWindowImpl owns all composition

`DesktopWindowImpl` would create and attach the placeholder itself. This keeps the
activation policy local but duplicates ContentIsland root-management logic and makes
multi-window cleanup difficult.

### DesktopWindowXamlSource owns the placeholder

`DesktopWindowXamlSource` could install a placeholder while connecting its site
bridge. This is naturally per-window, but it does not own frame submission or the
point where the real XAML composition root becomes available.

### DesktopWindowImpl policy with DCompTreeHost composition ownership

This is the selected approach. `DesktopWindowImpl` decides whether the feature is
enabled and requests placeholder preparation before the initial `ShowWindow`.
`DCompTreeHost` owns the compositor, ContentIsland root connection, per-island render
state, resizing, commit, replacement, and cleanup.

## Optional Change

Add `XamlChangeId.SkipWindowRedirectionSurface`, using tracking ID `63530879`, to the
XamlOM model and generated/public enum surfaces. Add a corresponding bit to
`OptionalChangeState` and map it in `XamlOptionalChanges_Partial.cpp`.

The change is an explicit opt-in and is not enabled by the general performance
opt-in. It controls both parts of the safe behavior:

1. Create the top-level HWND with `WS_EX_NOREDIRECTIONBITMAP`.
2. Present a theme-colored placeholder visual before first activation.

Tests and the validation sample may enable or disable the change before XAML
initialization.

## Window Creation and Activation

`DesktopWindowImpl::CreateDesktopWindow` passes `WS_EX_NOREDIRECTIONBITMAP` when the
optional change is enabled. `WM_ERASEBKGND` returns handled without GDI painting in
that mode because the HWND has no redirection bitmap.

During the first `DesktopWindowImpl::ActivateImpl`, before `ShowWindow`:

1. Resolve the window's `CXamlIslandRoot` from its `DesktopWindowXamlSource`.
2. Read the current client size.
3. Resolve the background from the effective system/application theme using the same
   framework theming policy as the existing HWND background fill.
4. Ask `DCompTreeHost` to install and commit a placeholder for that island.
5. Call `ShowWindow`, `UpdateWindow`, and `SetActiveWindow` using the existing flow.

Later activation, restore, and show operations do not recreate the placeholder.

## Placeholder Composition

Extend `DCompTreeHost::XamlIslandRenderData` with per-island placeholder state. A
single `DCompTreeHost` can serve multiple top-level windows, so global state is not
correct.

Placeholder preparation:

1. Create a `CompositionColorBrush` from the supplied framework theme color.
2. Create a `SpriteVisual`, assign the brush, and size it to the island client area.
3. Set it as that island's `IContentIslandExperimental::Root`.
4. Commit the composition device before the HWND is shown.

`UpdateXamlIslandTargetSize` keeps an active placeholder sized with the island.
`RemoveXamlIslandTarget` releases all placeholder state.

When `ConnectXamlIslandTargetRoots` receives the first real WUC root for an island
with an active placeholder, it replaces the ContentIsland root with the real XAML
root and commits that root change. The initial implementation does not animate the
transition; immediate replacement minimizes lifetime and failure modes while still
eliminating the uninitialized frame. Opacity animation can be added later without
changing ownership.

## Theme Behavior

The placeholder uses `FrameworkTheming::GetHwndBackground` with the effective
application theme when available and the system/default theme otherwise. This keeps
the placeholder consistent with the existing `WM_ERASEBKGND` fallback in light,
dark, and system-default configurations.

The color is captured immediately before first activation. Theme changes after the
real XAML root is connected do not affect the placeholder because it no longer
exists.

## Validation Sample

Add a small C++/WinRT desktop sample under `Samples` with a custom entry point. It
accepts:

- `--placeholder=on`: enables `SkipWindowRedirectionSurface` before
  `Application::Start`.
- `--placeholder=off`: explicitly disables it before `Application::Start`.

The default is off so launching without arguments preserves current behavior. The
window displays the selected mode and intentionally delays creation of visibly
distinct content briefly after activation, making startup flashing easy to compare.
The placeholder color follows the current system theme.

## Testing

Add desktop window integration coverage for:

- Optional change disabled: existing extended style and activation behavior.
- Optional change enabled: `WS_EX_NOREDIRECTIONBITMAP` is present at creation.
- Placeholder preparation occurs only on initial activation.
- Placeholder is removed after the real XAML root is connected.
- Placeholder state is cleaned up if the window closes before first content.
- Multiple top-level windows maintain independent placeholder state.

Use test hooks for deterministic placeholder-state inspection if existing public and
Win32 state cannot prove the composition transition. Avoid screenshot-only or
timing-only assertions.

## Error Handling

Placeholder creation and the pre-show commit are part of initial activation. Failures
propagate through the existing HRESULT path rather than showing an uninitialized
no-redirection window. Teardown tolerates an island or bridge already being closed,
following existing ContentIsland cleanup behavior.

