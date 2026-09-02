# Top-Level Window Placeholder Visual Design

## Goal

Allow WinUI top-level windows to independently opt into
`WS_EX_NOREDIRECTIONBITMAP` and a theme-appropriate placeholder visual. This permits
validation of each mitigation independently as well as the intended combined mode.

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

Use two independent `XamlChangeId` values and `OptionalChangeState` bits:

- `SkipWindowRedirectionSurface`, tracking ID `63530879`, controls creation-time
  `WS_EX_NOREDIRECTIONBITMAP` and the corresponding `WM_ERASEBKGND` behavior.
- `WindowPlaceholderVisual`, prototype ID `63530880`, controls placeholder creation,
  commit, replacement, sizing, and cleanup.

Both changes are explicit opt-ins, default off, and are not enabled by the general
performance opt-in. They may be enabled or disabled in any combination before XAML
initialization.

Tests and the validation sample may enable or disable either change before XAML
initialization.

## Window Creation and Activation

`DesktopWindowImpl::CreateDesktopWindow` passes `WS_EX_NOREDIRECTIONBITMAP` only when
`SkipWindowRedirectionSurface` is enabled. `WM_ERASEBKGND` returns handled without
GDI painting only in that mode because the HWND has no redirection bitmap.

When `WindowPlaceholderVisual` is enabled, the first
`DesktopWindowImpl::ActivateImpl` performs the following before `ShowWindow`,
regardless of the redirection-surface state:

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

The C++/WinRT desktop sample accepts two independent switches:

- `--no-redirection=on|off`: directly enables or disables
  `SkipWindowRedirectionSurface`.
- `--placeholder=on|off`: independently enables or disables
  `WindowPlaceholderVisual`.

Both defaults preserve current behavior: no-redirection is off and the placeholder
is off. Conflicting values for either switch are rejected before XAML initialization.
The window displays both selected states, identifies the active matrix combination,
and intentionally delays visibly distinct content so all four combinations can be
compared:

1. No-redirection off, placeholder off: baseline.
2. No-redirection on, placeholder off: isolated no-redirection behavior.
3. No-redirection off, placeholder on: isolated placeholder behavior.
4. No-redirection on, placeholder on: intended combined mitigation.

## Testing

Add desktop window integration coverage for all four optional-change combinations:

- No-redirection off, placeholder off: no extended style and no placeholder.
- No-redirection on, placeholder off: extended style and no placeholder.
- No-redirection off, placeholder on: no extended style and a placeholder until the
  first frame.
- No-redirection on, placeholder on: extended style and a placeholder until the first
  frame.
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
