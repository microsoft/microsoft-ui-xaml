# Subtrial A results

Subtrial A keeps WinUI XAML and Controls lifted while moving their Composition,
DispatcherQueue, DirectManipulation, and supporting runtime paths to Windows
system components. Public API compatibility with normal WinUI 3 is intentionally
not preserved.

## Result

The experiment is viable on the validated Windows 11 system, with explicit
feature exclusions. The Release x64 XAML and Controls products build, packaged
C++/WinRT and C# apps activate and render, and WinUI Gallery completes a
three-minute responsive soak plus Color, CreateMultipleWindows,
AppWindowTitleBar, NavigationView, ScrollView, ItemsView, XamlCompInterop,
ContentIsland, TitleBar, SystemBackdrops, and SystemBackdropElement validation. The validated
processes use system `dcomp.dll`, `CoreMessaging.dll`, and
`DirectManipulation.dll`.

No forbidden module was loaded by the validated Gallery process:

- `Microsoft.UI.Composition.dll`
- `Microsoft.UI.Dispatching.dll`
- `Microsoft.UI.Content.dll`
- `CoreMessagingXP.dll`
- `dcompi.dll`
- `Microsoft.DirectManipulation.dll`
- `Microsoft.UI.Composition.OSSupport.dll`
- `DwmSceneI.dll`
- `dwmcorei.dll`
- `marshal.dll`
- `wuceffectsi.dll`

The rebuilt `Microsoft.UI.Xaml.Controls.dll` also has no direct import of a
lifted Composition, Dispatching, Content, or DirectManipulation DLL.

## Component findings

| Component | Windows 11 result | Windows 10 result |
| --- | --- | --- |
| Composition public API | Uses `Windows.UI.Composition`; activation, visuals, brushes, animations, surfaces, and Controls compilation pass. | The API is OS-versioned. Only the Composition surface present on the installed Windows 10 release can be used; lifted additions are unavailable. |
| Composition private interop | System partner interfaces replace unsupported lifted `InternalStable` interfaces on the validated build. | Not validated. These private interfaces are not contractual and are the primary Windows 10 execution risk. |
| Composition property notifications | System Composition does not expose the lifted experimental property-change interface. Listener registration is skipped, so hit-test synchronization for animated handoff/facade transforms is an accepted functional gap. | Same gap; behavior must be evaluated for any scenario that depends on animated hit-test geometry. |
| DispatcherQueue | Uses `Windows.System.DispatcherQueue`; native and managed startup and enqueue pass. | Available from Windows 10 version 1809, but requires execution validation on each supported build. |
| DirectComposition / DirectManipulation | Uses system `dcomp.dll` and system DirectManipulation paths. | System APIs exist, but the exact XAML-private integration has not been executed on Windows 10. |
| Input pointer handoff | InteractionTracker paths consume `Windows.UI.Input.PointerPoint`. | Limited to the system Input API surface on the installed release. |
| Non-client pointer input | Restored the private `PartnerPointerPoint` activation factory required by XAML. `SetWindowRgn` and `SetWindowPos` now leave the WRL2 session around synchronous window-message callouts. The rebuilt Input DLL passed the Gallery TitleBar soak. | Private factory and re-entry behavior are not contractual; execution validation is required. |
| Content external links | No compatible system projection exists for lifted `ContentExternalOutputLink` and `ContentExternalBackdropLink`. | Same gap. |
| System backdrops | No projected `Windows.UI.Composition.SystemBackdrops` controller counterpart exists. | Same gap, plus OS feature availability varies by release. |
| Reveal lighting effect | No usable system projection for the lifted `SceneLightingEffect` path in this build. | Same gap. |

## Accepted exclusions

The following features fail explicitly with `E_NOTIMPL` instead of silently
loading or mixing lifted Composition objects:

- `SystemBackdropElement`
- CommandBarFlyout custom `SystemBackdrop`
- Mica and Desktop Acrylic backdrop controllers
- WebView2 composition hosting
- Reveal hover and border effects

InkCanvas uses the direct system-compositor splice and no longer has a lifted
ContentExternalOutputLink fallback.

## Validation boundary

Windows 11 validation was performed on build 26200. A Windows 10 machine or VM
was not available in this environment, so the Windows 10 column is an API and
architecture gap analysis, not an execution claim. Before productization, run
the same packaged harness on the minimum supported Windows 10 build and record
the availability of every private partner interface used by WinUIDetails.
