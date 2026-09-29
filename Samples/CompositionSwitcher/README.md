# Composition switcher samples

Both projects are unpackaged native desktop applications that:

- use the lifted `Microsoft.UI.Dispatching.DispatcherQueue` that WinUI desktop
  creates on the current thread during `Application::Start`;
- host the same WinUI 3 window;
- animate a `Microsoft.UI.Composition.SpriteVisual` obtained from the WinUI 3
  XAML visual tree.

See [SwitcherOsDependencyDiagram.html](SwitcherOsDependencyDiagram.html) for a
diagram of the lifted scheduling session and system callback-session bridge.

See [LiftedDispatcherQueueApiReport.html](LiftedDispatcherQueueApiReport.html)
for a Microsoft-documentation-based inventory of public APIs that directly
consume or return `Microsoft.UI.Dispatching.DispatcherQueue`.

They use the public `Microsoft.WindowsAppSDK` NuGet package version `2.5.1`.
The framework-dependent executables load WinUI from the installed
`Microsoft.WindowsAppRuntime.2` package rather than from this repository's
`packages` or `BuildOutput` directories.

`Lifted\CompositionSwitcherLifted.vcxproj` makes no engine opt-in call, so it uses
the default lifted compositor.

`System\CompositionSwitcherSystem.vcxproj` calls
`CompositionEngine::TrySetProcessEngine(CompositionEngineType::System)` before
creating the DispatcherQueue or any XAML/composition object. It exits with an
error dialog when the OS switcher feature or LAF is unavailable.

The system project invokes the WinRT static ABI directly because the repository's
C++ projection filters this contract-v2 API from the generated header. The ABI
definition matches `ICompositionEngineStatics` in the Interactive Experiences
package metadata.

Build from the repository root:

```powershell
.\initrun.ps1 msb /q "Samples\CompositionSwitcher\Lifted\CompositionSwitcherLifted.vcxproj"
.\initrun.ps1 msb /q "Samples\CompositionSwitcher\System\CompositionSwitcherSystem.vcxproj"
```
