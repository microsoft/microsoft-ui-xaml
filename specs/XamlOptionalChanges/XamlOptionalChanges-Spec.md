XamlOptionalChanges API
===

# Background

WinUI occasionally needs to ship breaking changes that you can explicitly
choose to include/exclude.  For example, significant performance improvements
that subtly alter layout trees, or long-standing bug fixes that correct visual
behavior some apps may have inadvertently relied on.  Shipping these changes as
always-on by default risks breaking existing apps on update.

WinUI needs a general-purpose mechanism for you to say "I've tested my app with
change X, please enable it."  An API that lets apps explicitly enable breaking
changes early in its life.

`XamlOptionalChanges` follows a similar early-lock-down pattern but generalizes
it: you opt in to (or explicitly opt out of) individually identified changes
before XAML is initialized, after which the selections are locked and
cannot be modified.  This avoids the need for separate Boolean properties or
per-feature knobs on `Application`, keeps the API surface evergreen (enum
values can be deprecated and eventually removed when a change becomes
unconditionally enabled), and provides a single place for documentation and
tooling to enumerate available optional changes.

### Why an enum + methods instead of individual properties?

Using a `XamlChangeId` enum with `EnableChange` / `DisableChange` methods
rather than per-change properties allows us to:

* Add new opt-in changes without growing the `Application` or `Window` API
  surface.
* Deprecate and ultimately *remove* an enum value in the future when a change
  is promoted to always-on -- far simpler than removing a property.
* Keep a single audit point (`IsChangeEnabled`) that both app code and
  platform internals can query.


# Conceptual pages (How To)

## Opting in to optional XAML changes

Some WinUI releases include optional changes that are **off by default** to
preserve backward compatibility.  These changes might improve performance, fix
a long-standing bug, or align behavior with an updated specification.  Each
change is identified by a [XamlChangeId](#xamlchangeid-enum) value.

You enable (or disable) changes through the static
[XamlOptionalChanges](#xamloptionalchanges-class) methods **before** starting
XAML.  This means before calling `Application.Start()` (if WinUI 3 app) or
`WindowsXamlManager.InitializeForCurrentThread()` (apps just using XAML
islands).

WinUI apps depending on the XAML-generated main function should define
`DISABLE_XAML_GENERATED_MAIN` project-wide.  You can then write your own main
function where these API functions can be called _before_ calling
`Microsoft.UI.Xaml.Application.Start` or
`Microsoft.UI.Xaml.Hosting.WindowsXamlManager.InitializeForCurrentThread`.

### The one rule

These are process-wide settings.

> **Configure all optional changes on your main thread, before any
> thread calls `Application.Start()` or
> `WindowsXamlManager.InitializeForCurrentThread()`.**

Do it once, up front, before you start any thread / function using WinUI 3.

Following this rule means:

* No locks are needed.
* No races are possible.
* `Lock()` is not required (the platform auto-locks at XAML initialization).

### Typical workflow

1. Read the release notes for the WinUI version you are targeting.
   Each optional change will list its `XamlChangeId` and describe the
   behavioral difference.
2. Test your app with required change(s) by enabling before XAML
   initialization.
3. If everything works, ship with the change(s) enabled.

### When are changes locked?

The optional-changes state is automatically **locked** when XAML is
initialized; specifically, at the entry point of `Application.Start()`
or `WindowsXamlManager.InitializeForCurrentThread()`, whichever is
called first in the process.  After locking, any call to
`EnableChange` or `DisableChange` will throw an `InvalidOperationException`.

You can also call `XamlOptionalChanges.Lock()` yourself if you want
to freeze the state even earlier (for example, library authors who
want to guarantee no downstream code mutates state after their
initialization logic).

### Example: WinUI 3 packaged / unpackaged app

``` csharp
// Program.cs
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Settings;

static class Program
{
    // Ensure DISABLE_XAML_GENERATED_MAIN is defined
    static void Main(string[] args)
    {
        // Configure optional changes BEFORE starting XAML.
        XamlOptionalChanges.EnableChange(XamlChangeId.Perf2026);

        // Internally calls WindowsXamlManager.InitializeForCurrentThread
        Application.Start((p) =>
        {
            // App constructor and OnLaunched run inside here.
            var app = new App();
        });
    }
}
```

### Example: XAML Island (C++ Win32) app

``` cpp
// App.xaml.h
namespace winrt::MyIslandApp::implementation
{
struct App : AppT<App>
{
    App() : m_windowsXamlManager{
      winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread()
    } { }
    
    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
    
private:
    winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager m_windowsXamlManager{ nullptr };
};
}

// ----------------------------------------------------------------------------

// main.cpp
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Interop.h>

#include <winrt/Microsoft.UI.Xaml.Settings.h>

namespace settings = winrt::Microsoft::UI::Xaml::Settings;
namespace hosting = winrt::Microsoft::UI::Xaml::Hosting;
namespace xaml = winrt::Microsoft::UI::Xaml;
namespace dispatching = winrt::Microsoft::UI::Dispatching;

// Ensure DISABLE_XAML_GENERATED_MAIN is defined.
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    MddBootstrapInitialize(0x00010008, nullptr, {});
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    auto dispatcherQueueController{
      dispatching::DispatcherQueueController::CreateOnCurrentThread() };

    // Configure optional changes BEFORE XAML initialization.
    settings::XamlOptionalChanges::EnableChange(
      settings::XamlChangeId::Perf2026);
    settings::XamlOptionalChanges::DisableChange(
      settings::XamlChangeId::SomeOtherFix);

    // Lock explicitly (optional; platform auto-locks on first XAML init).
    settings::XamlOptionalChanges::Lock();

    // Create App object initializing XAML (needed for proper styling of controls).
    auto myIslandApp{ winrt::make<winrt::MyIslandApp::implementation::App>() };

    // Setup UI
    HWND hwnd = MyWindow();
    hosting::DesktopWindowXamlSource host;
    host.Initialize(GetWindowIdFromWindow(hwnd));
    xaml::UIElement myXamlRoot = MakeMyXamlRoot();
    host.Content(myXamlRoot);

    // app life cycle

    MddBootstrapShutdown();
    return 0;
}
```

### Example: querying state at runtime

``` csharp
if (XamlOptionalChanges.IsChangeEnabled(XamlChangeId.Perf2026))
{
    // App has this change active.
}
```

### Multi-threading

`XamlOptionalChanges` state is **process-wide**; shared across all threads.
The recommended pattern is straightforward:

1. **Configure on your main thread**: call `EnableChange` /
   `DisableChange` sequentially, before initializing XAML or spawning any XAML
   threads.
2. **Then start XAML**: each thread calls
   `WindowsXamlManager.InitializeForCurrentThread()` as needed directly or
   indirectly (`Application.Start()`, `DesktopWindowXamlSource`, etc.)

```
main() or WinMain()
│
├─ EnableChange(Perf2026)        ← single-threaded, no races
├─ EnableChange(SomeOtherFix)
│
├─ spawn Thread A → InitializeForCurrentThread() → create Window / Island
├─ spawn Thread B → InitializeForCurrentThread() → create Window / Island
│   ...
```

Because all configuration happens before any thread touches XAML, no
synchronization is needed in your code and no `Lock()` call is required.

**Do not** scatter `EnableChange` / `DisableChange` calls across multiple
threads that concurrently initialize XAML.  The first thread to call
`Application.Start()` or `InitializeForCurrentThread()` will auto-lock the
state, causing `EnableChange` on any other thread to throw
`InvalidOperationException`.

If your app spawns a thread that might *optionally* initialize XAML (e.g. a
plug-in host), call `Lock()` explicitly on your main thread after configuration
to make the locked state deterministic regardless of thread scheduling.


# API Pages

## XamlChangeId enum

Identifies an individual optional change that can be enabled or disabled
through [XamlOptionalChanges](#xamloptionalchanges-class).

Each value corresponds to a specific feature, fix, or behavioral change
documented in the WinUI release notes.  The numeric value is the internal
tracking ID for the change, which keeps `enum` names stable even if friendly
descriptions evolve.

| Value      | Numeric  | Description                            | Default  |
|------------|----------|----------------------------------------|----------|
| `Perf2026` | 60952725 | Breaking perf changes shipped in 2026. | Disabled |

_Spec note: The team will add new values here as future optional changes
are introduced.  Values that have been promoted to always-on across all
supported SDK versions will be marked `[Deprecated]` and eventually removed._

### Remarks

* New `XamlChangeId` values are introduced alongside the WinUI release that
  contains the corresponding change.
* You do **not** need to reference a specific WinUI NuGet version to enable a
  change -- you only need a version that recognizes the enum value.  If you pass
  an unrecognized value to `EnableChange`, the call is silently ignored (the
  change simply does not exist in that build); querying it returns `false`.
* Over time a change may be promoted to **always-on**, at which point its
  `XamlChangeId` will be deprecated.  Your existing `EnableChange` call will
  continue to compile and run without error, but the change will be active
  regardless.
* The typical progression is: introduced as default-off, promoted to
  default-on in a later major version (apps can still opt out via
  `DisableChange`), then made permanent (old code path removed, enum
  value becomes a no-op).

## XamlOptionalChanges class

Provides static methods to opt in to or out of individual breaking or
behavioral changes identified by [XamlChangeId](#xamlchangeid-enum).

All methods are static; you do not instantiate this class.

### Remarks

* Call `EnableChange` / `DisableChange` **before** calling
  `Application.Start()` or `WindowsXamlManager.InitializeForCurrentThread()`.
  The state is automatically locked at the entry point of whichever of those
  methods is called first in the process.
* The optional-changes state is **process-wide**.  It is shared across all
  threads and all windows within the same process.
* **Thread safety.** All methods on this class are safe to call from any
  thread.  Internally, `EnableChange`, `DisableChange`, and `Lock` are
  serialized; `IsChangeEnabled` and `IsLocked` are lock-free reads once the
  state has been locked.
* After locking, `EnableChange` and `DisableChange` throw
  `InvalidOperationException`, even for unrecognized values.
  `IsChangeEnabled`, `Lock`, and `IsLocked` never throw.


## XamlOptionalChanges.EnableChange method

```csharp
public static void EnableChange(XamlChangeId changeId);
```

Enables the optional change identified by `changeId`.

Call this before XAML is initialized (see
[Remarks on XamlOptionalChanges](#xamloptionalchanges-class)).

### Exceptions

| Exception                   | Condition                                                                |
|-----------------------------|--------------------------------------------------------------------------|
| `InvalidOperationException` | Valid `XamlChangeId` and optional-changes state has already been locked. |

### Example

```csharp
// Before Application.Start() or WindowsXamlManager.InitializeForCurrentThread()
XamlOptionalChanges.EnableChange(XamlChangeId.Perf2026);
```


## XamlOptionalChanges.DisableChange method

```csharp
public static void DisableChange(XamlChangeId changeId);
```

Explicitly disables the optional change identified by `changeId`.
 
`DisableChange` is useful in the following situations:

* A future WinUI release changes a `XamlChangeId` from
  *default-off* to *default-on*.  If you are not yet ready to adopt
  the change, you can call `DisableChange` to preserve existing
  behavior.
* Your app needs to explicitly document that a change is intentionally
  **not** enabled, for diagnostic or auditing purposes.

### Exceptions

| Exception                   | Condition                                                                |
|-----------------------------|--------------------------------------------------------------------------|
| `InvalidOperationException` | Valid `XamlChangeId` and optional-changes state has already been locked. |


## XamlOptionalChanges.IsChangeEnabled method

```csharp
public static bool IsChangeEnabled(XamlChangeId changeId);
```

Returns `true` if the specified change is currently enabled, `false`
otherwise.

This method can be called at any time; before or after locking.  It never
throws.

### Remarks

You can use this method in your own code to branch on whether a change
is active, for example to adjust layout expectations in a unit test.

_Spec note: Platform-internal callers of `IsChangeEnabled` should assert
that the state has been locked by the time they query it, to avoid
reading a value that might still change._


## XamlOptionalChanges.Lock method

```csharp
public static bool Lock();
```

Freezes the optional-changes state so that subsequent calls to
`EnableChange` or `DisableChange` will throw.

Returns `true` if this call actually performed the lock, or `false` if
the state was already locked (either from a previous `Lock()` call or
from an automatic platform lock).  This method never throws.

### Remarks

You do not normally need to call `Lock()`.  The platform auto-locks
when `Application.Start()` or
`WindowsXamlManager.InitializeForCurrentThread()` is called.

`Lock()` is primarily useful for **Apps with non-deterministic thread startup**
(e.g. a plug-in host where a thread might optionally initialize XAML) that want
to make the locked state explicit and independent of thread scheduling.


## XamlOptionalChanges.IsLocked method

```csharp
public static bool IsLocked();
```

Returns `true` if the optional-changes state has been locked, `false`
otherwise.  This method never throws.


## Summary of XamlOptionalChanges members

| Name                            | Description                                                                            |
|---------------------------------|----------------------------------------------------------------------------------------|
| `EnableChange(XamlChangeId)`    | Enables the specified optional change. Throws `InvalidOperationException` after lock.  |
| `DisableChange(XamlChangeId)`   | Disables the specified optional change. Throws `InvalidOperationException` after lock. |
| `IsChangeEnabled(XamlChangeId)` | Returns whether the specified change is enabled. Safe to call at any time.             |
| `Lock()`                        | Freezes the state. Returns `true` if this call performed the lock.                     |
| `IsLocked()`                    | Returns whether the state is currently locked.                                         |


# API Details

``` csharp
// MIDL3
namespace Microsoft.UI.Xaml.Settings
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 9)]
    [v1_enum] enum XamlChangeId
    {
        Perf2026 = 60952725,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 9)]
    [webhosthidden]
    [static_name("Microsoft.UI.Xaml.Settings.IXamlOptionalChangesStatics")]
    static runtimeclass XamlOptionalChanges
    {
        /// Enables the specified optional change. Must be called before XAML
        /// initialization; otherwise throws InvalidOperationException.
        static void EnableChange(XamlChangeId changeId);

        /// Explicitly disables the specified optional change. Must be called
        /// before XAML initialization; otherwise throws InvalidOperationException.
        static void DisableChange(XamlChangeId changeId);

        /// Returns true if the specified change is currently enabled.
        /// Safe to call at any time (before or after locking).
        static Boolean IsChangeEnabled(XamlChangeId changeId);

        /// Freezes the optional-changes state. Returns true if this call
        /// performed the lock, false if already locked.
        static Boolean Lock();

        /// Returns true if the optional-changes state has been locked.
        static Boolean IsLocked();
    };
}
```


# Appendix

## Design alternatives considered

### Per-change Boolean properties on Application

Adding `Application.EnablePerf2026` (and a new property for every future
change) was rejected because:

* It does not scale -- each change bloats the `Application` API surface.
* Removing a property when a change is promoted to always-on is a
  breaking API removal.
* It scatters related opt-in logic across many properties.

### Per-thread state

Making the enabled-change set per-thread (stored in TLS) was considered because
it would eliminate cross-thread synchronization entirely.  It was rejected
because:

* It violates developer expectations: most apps have one XAML thread, and
  developers think of these as app-level choices.
* The `Application` singleton is process-wide; some changes may affect shared
  state that cannot be cleanly partitioned per-thread.
* It creates an explosion of configurations (N threads × M change IDs) that is
  difficult to test, reproduce, and support.
* Platform-internal code running on threadpool or COM callbacks would have
  ambiguous context for which thread's state to query.

### Allowing configuration in the App constructor

Keeping the auto-lock timing late (just before markup parsing, like
`RequestedTheme`) so that `EnableChange` works inside the `App` constructor was
considered.  It was rejected in favor of locking at the `Application.Start()` /
`InitializeForCurrentThread()` entry point because:

* The earlier lock makes the rule absolute: configure before starting XAML.  No
  "well, actually the App constructor works too" footnote.
* It eliminates a race window between `Start()` and the markup-parse lock point
  where a second thread could call `InitializeForCurrentThread()` and auto-lock
  while the first thread's `App` constructor is still running.
* The cost is that `EnableChange` must move from the `App` constructor to
  before `Application.Start()`; a minor structural change that makes intent
  clearer.

### Auto-lock timing

The auto-lock fires at the very beginning of:

* `Application.Start()` before calling `ApplicationInitializationCallback`
* `WindowsXamlManager.InitializeForCurrentThread()`

Whichever is called first in the process triggers the lock.  Since
`Application.Start()` itself calls `InitializeForCurrentThread()` internally, a
single lock check at the `WindowsXamlManager::Initialize()` entry point is
sufficient to cover both paths.

The `InvalidOperationException` message should be educative and conclusive:

> `XamlOptionalChanges cannot be modified after XAML has been`
> `initialized. Call EnableChange/DisableChange before`
> `Application.Start() or`
> `WindowsXamlManager.InitializeForCurrentThread().`

## Related APIs

Windows App SDK’s [RuntimeCompatibilityOptions][] is a similar but that’s for bug fixes,
not explicitly introduced breaking changes to move WinUI forward.

.NET’s [AppContext][] switches -- process-global, set-early opt-in/-out for
behavioural breaking changes -- is for a similar purpose for .NET apps.

[runtimecompatibilityoptions]: https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.windows.applicationmodel.windowsappruntime.runtimecompatibilityoptions?view=windows-app-sdk-2.0
[appcontext]: https://learn.microsoft.com/en-us/dotnet/fundamentals/runtime-libraries/system-appcontext

## Future considerations

* **Telemetry / diagnostics:** The locked snapshot of enabled changes
  is a natural payload for diagnostic telemetry, making it easy to
  correlate crashes or performance data with a specific set of opt-ins.
