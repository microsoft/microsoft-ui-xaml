Enable Gamepad Key Routing at XAML Initialization
===

# Background

WinUI's built-in keyboard and focus navigation (arrow-key XY focus, `Enter`/`Space`
invocation, and so on) responds to keyboard virtual-key input. On devices that use a
gamepad as a primary input method, the platform can deliver gamepad input to an app as
keyboard virtual keys so that this existing navigation "just works" without the app writing
any gamepad-specific code.

In System XAML (UWP XAML), this gamepad-driven navigation was on by default — an app got it
for free without any opt-in. WinUI 3 XAML does not turn it on automatically, so today the
same app loses gamepad navigation unless it (or its host) explicitly enables it. We consider
that gap a regression from System XAML, and this spec describes closing it by having the
WinUI framework enable gamepad key routing on by default, the way System XAML did.

Whether gamepad input is delivered to an app as virtual keys is controlled by an existing
operating-system API, `Windows.UI.Input.GamepadKeyRoutingConfiguration`. This is a
process-wide setting: when key routing is enabled, gamepad input arrives as virtual keys
for the whole process; when it is disabled, it does not.

Today an app (or host) has to turn this on itself. This spec describes enabling it inside
the WinUI framework, so that a full WinUI app gets gamepad-driven navigation on by default,
while still allowing the app to opt out.

The framework enables key routing only for a **full WinUI app** — one that starts XAML by
calling `Application.Start()`. A process that merely embeds WinUI as a **XAML Island** is
deliberately left alone: the framework does not flip the process-wide switch for it, so the
host's non-XAML input code is unaffected. A host that wants routing can still enable it
itself.

This spec does not introduce a new API. `GamepadKeyRoutingConfiguration` already ships
in the OS (`Windows.Foundation.UniversalApiContract`, version 19). The work here is a
framework-internal call to that existing API at the right point in XAML startup, plus
guidance for how an app opts out. The API surface is reproduced in
[API Details](#api-details) for reference only.

# Conceptual pages (How To)

## Gamepad navigation is on by default

When your full WinUI app starts XAML — which it does by calling `Application.Start()` from
its generated entry point — the framework enables gamepad key routing for the process.
From that point on, gamepad input (D-pad, thumbstick, and buttons that map to navigation
keys) is delivered to your app as keyboard virtual keys. This means WinUI's normal keyboard
and focus behaviors — XY focus navigation, `Enter`/`Space` to invoke, `Esc` to dismiss, and
similar — respond to the gamepad automatically, with no additional code in your app.

If your app does nothing special, this is the recommended experience: you get gamepad
navigation for free, and if your app has no interest in gamepad input it can simply ignore
those virtual keys.

## Turning gamepad key routing off

Some apps handle raw gamepad input themselves (for example, a game or an app that uses
`Windows.Gaming.Input`) and do not want gamepad input surfaced as keyboard virtual keys. If
that describes your app, you can opt out by calling the OS API directly and passing `false`.

Because this is a single process-wide setting, the last write wins. The framework enables
key routing **once per process**, when your app calls `Application.Start()`, and never
re-runs the enable afterward (see [Where the framework makes the call](#where-the-framework-makes-the-call)).
So the rule for opting out is simply: **call the setter with `false` any time after
`Application.Start()`.** Because the framework does not re-enable on later threads or
re-inits, a single `false` sticks for the lifetime of the process.

The timing is not otherwise strict: there is no upper bound and no requirement to opt out
before any window is created. Because the setting is live and process-wide, calling the
setter with `false` at any later point — even after windows exist and have been receiving
gamepad input as virtual keys — takes effect immediately from that point on. Opting out
early is simply the tidiest choice, since it means no window ever observes gamepad-as-keys
in the first place.

Because the framework's enable runs inside `Application.Start()`, before your `App` object
is constructed, a good place to opt out is your `App` constructor or the start of your
`OnLaunched` handler.

If instead you are **hosting a XAML Island** in a non-WinUI process, there is nothing to opt
out of: the framework does not auto-enable routing for island hosts (they never call
`Application.Start()`), so the process-wide switch stays at its OS default of off unless you
turn it on yourself.

## Scope: full WinUI apps, not XAML Island hosts

Gamepad key routing is a **process-wide** OS setting, not a per-window or per-XAML-content
setting. So the framework has to be deliberate about *which processes* it turns it on for.

It enables routing only from the **full-app entry point**, `Application.Start()`. A normal
WinUI app reaches that method from its generated entry point, owns its process, and gets
gamepad navigation on by default — process-wide is exactly what it wants.
A process that embeds WinUI only as a **XAML Island** never calls `Application.Start()`, so
the framework does not auto-enable routing for it.

One residual point worth stating plainly: within a full WinUI app the setting is still
process-wide, so if that app *also* hosts non-XAML UI in the same process, that UI's input
paths (anything that ultimately relies on `WM_KEYDOWN`/`WM_KEYUP`) may also begin seeing
gamepad input delivered as virtual keys. For a full app that owns its process this is
expected; an app that does not want it can opt out as described in
[Turning gamepad key routing off](#turning-gamepad-key-routing-off).

# Examples

## Framework enables key routing at app startup (no app code required)

The framework makes the call once per process, from the full-app entry point
(`Application.Start()`). Conceptually:

```cpp
// Inside Application.Start() (FrameworkApplicationFactory::StartImpl), on the branch that
// starts a full WinUI app. Island hosts don't reach here, so their process is left alone.
// IsApiContractPresent checks the OS actually has the API (it ships in a newer contract than
// WinUI's minimum OS); IsSupported checks the feature is usable on this device.
if (Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
        L"Windows.Foundation.UniversalApiContract", 19) &&
    Windows::UI::Input::GamepadKeyRoutingConfiguration::IsSupported())
{
    Windows::UI::Input::GamepadKeyRoutingConfiguration::TrySetKeyRoutingEnabled(true);
}
```

Your app does not need to do anything to get this behavior.

## Opting out from your app

If you do not want gamepad input delivered as virtual keys, disable it any time after XAML
init — the examples below do so early (before the first window) simply because it is the
tidiest place, but a later opt-out works just as well. In C#:

```c#
public partial class App : Application
{
    public App()
    {
        this.InitializeComponent();

        // Opt out of gamepad-to-key routing; the framework enabled it during XAML init.
        // IsApiContractPresent guards OS versions without the API (the type ships in a newer
        // contract than WinUI's minimum OS); IsSupported guards devices where it's unusable.
        if (Windows.Foundation.Metadata.ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 19) &&
            Windows.UI.Input.GamepadKeyRoutingConfiguration.IsSupported())
        {
            Windows.UI.Input.GamepadKeyRoutingConfiguration.TrySetKeyRoutingEnabled(false);
        }
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        // Alternatively, opt out here. Doing it before creating/activating the first window
        // is only a recommendation.  A later opt-out works just as well.
        m_window = new MainWindow();
        m_window.Activate();
    }
}
```

The equivalent in C++/WinRT:

```cpp
App::App()
{
    // Opt out of gamepad-to-key routing; the framework enabled it during XAML init.
    // IsApiContractPresent guards OS versions without the API (the type ships in a newer
    // contract than WinUI's minimum OS); IsSupported guards devices where it's unusable.
    if (winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
            L"Windows.Foundation.UniversalApiContract", 19) &&
        winrt::Windows::UI::Input::GamepadKeyRoutingConfiguration::IsSupported())
    {
        winrt::Windows::UI::Input::GamepadKeyRoutingConfiguration::TrySetKeyRoutingEnabled(false);
    }
}
```

# API Pages

_Spec note: This feature introduces no new WinUI API. It calls the existing OS API
[`Windows.UI.Input.GamepadKeyRoutingConfiguration`](https://learn.microsoft.com/uwp/api/windows.ui.input.gamepadkeyroutingconfiguration).
The existing surface is listed in [API Details](#api-details) for reviewer convenience._

# API Details

```c# (but really MIDL3)
namespace Windows.UI.Input
{
    // Existing OS API (Windows.Foundation.UniversalApiContract, version 19).
    // Reproduced here for reference; not introduced by this work.
    [contract(Windows.Foundation.UniversalApiContract, 19)]
    runtimeclass GamepadKeyRoutingConfiguration
    {
        // Returns whether gamepad key routing is supported on the current device/OS.
        static Boolean IsSupported();

        // Gets whether gamepad input is currently being routed to the process as virtual keys.
        static Boolean IsKeyRoutingEnabled { get; };

        // Attempts to enable (true) or disable (false) routing gamepad input as virtual keys
        // for the process. Returns whether the setting was applied.
        static Boolean TrySetKeyRoutingEnabled(Boolean enabled);
    }
}
```

# Appendix

## Where the framework makes the call

The enable is placed at the **full-app entry point**: `Application.Start()`, whose
implementation is `FrameworkApplicationFactory::StartImpl` in
`/dxaml/xcp/dxaml/lib/FrameworkApplication_Partial.cpp`. `StartImpl` reads the app's
windowing model and dispatches to `StartDesktop()` (WinUI Desktop) or `StartUWP()` (WinUI
UWP). A full WinUI app reaches this method from its generated entry point; a process that
only embeds WinUI as a XAML Island never does. Because the enable lives here, only full
WinUI apps turn routing on — anything that starts XAML without going through
`Application.Start()` leaves the process-wide setting untouched.

`Application.Start()` is a full app's one-time startup call on its main thread, so the enable
runs exactly once per process on its own — no latch or per-thread guard needed. And because
it runs before `StartImpl` dispatches into the rest of startup and app code, it is already in
place before the `App` constructor, so an app's later opt-out is the last write and wins:

```cpp
_Check_return_ HRESULT FrameworkApplicationFactory::StartImpl(_In_opt_ xaml::IApplicationInitializationCallback* pCallback)
{
    g_spApplicationInitializationCallback = pCallback;

    AppPolicyWindowingModel policy = AppPolicyWindowingModel_None;
    LONG status = AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &policy);
    if (status != ERROR_SUCCESS)
    {
        IFC_RETURN(E_FAIL);
    }

    if (policy == AppPolicyWindowingModel_ClassicDesktop ||
        policy == AppPolicyWindowingModel_Universal)
    {
        // Full WinUI app: enable gamepad-to-key routing for the process. Island hosts never
        // reach here (they start via WindowsXamlManager.InitializeForCurrentThread()), so the
        // host process is left untouched.
        // IsApiContractPresent: the OS has the type (it ships in UniversalApiContract 19,
        // newer than WinUI's minimum OS, so on downlevel OS the type is absent and even
        // calling IsSupported() would throw). IsSupported: usable on this device.
        if (winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
                L"Windows.Foundation.UniversalApiContract", 19) &&
            winrt::Windows::UI::Input::GamepadKeyRoutingConfiguration::IsSupported())
        {
            winrt::Windows::UI::Input::GamepadKeyRoutingConfiguration::TrySetKeyRoutingEnabled(true);
        }
    }

    if (policy == AppPolicyWindowingModel_ClassicDesktop)
    {
        return FrameworkApplication::StartDesktop();
    }
    else if (policy == AppPolicyWindowingModel_Universal)
    {
        return FrameworkApplication::StartUWP(pCallback);
    }

    return E_FAIL;
}
```

`ApiInformation::IsApiContractPresent(..., 19)` guards OS versions that predate the API
(where the type isn't registered and calling any member — including `IsSupported()` — would
throw), and `IsSupported()` guards devices where the feature exists but is unusable.

`FrameworkApplication_Partial.cpp` will need `#include <winrt/Windows.UI.Input.h>` and
`#include <winrt/Windows.Foundation.Metadata.h>`.

## Startup ordering (why app opt-out works)

On the desktop path the sequence is:

1. `Application.Start()` → `StartImpl` — the framework's `TrySetKeyRoutingEnabled(true)` runs
   here, before dispatching to `StartDesktop()`.
2. `StartDesktop()` brings up the framework.
3. Construct the app's `Application` object (`App` ctor).
4. `OnLaunched` → app creates its first window.
5. Run the message loop.

The framework's enable (step 1) runs before any app code (step 3 onward) and never runs
again. So to turn gamepad navigation off, an app just calls the setter with `false` from its
`App` constructor or `OnLaunched` — that later write is the last one and sticks for the
process.
