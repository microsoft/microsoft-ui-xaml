# Top-Level Window Placeholder Visual Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create opted-in WinUI top-level windows without a GDI redirection bitmap while presenting a system-theme placeholder visual until the first real XAML composition root is committed.

**Architecture:** `DesktopWindowImpl` owns feature policy and initial-show sequencing. `DCompTreeHost` owns per-island placeholder composition state and replaces the placeholder when the real XAML root is connected. A custom-entry-point sample toggles the optional change before XAML initialization.

**Tech Stack:** C++/WinRT, Win32 HWND styles, Microsoft UI Composition, ContentIsland partner interfaces, XamlOptionalChanges, TAEF integration tests, MSBuild.

## Global Constraints

- Disabled behavior must remain unchanged.
- `XamlChangeId.SkipWindowRedirectionSurface` uses tracking ID `63530879`.
- The optional change gates both `WS_EX_NOREDIRECTIONBITMAP` and placeholder behavior.
- The change is not included in the general performance opt-in.
- Placeholder color follows effective application/system light or dark theme.
- The placeholder is removed immediately after the first real XAML root is connected.
- Preserve unrelated existing worktree changes.

---

### Task 1: Add the optional change and no-redirection HWND creation

**Files:**
- Modify: `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Settings.cs`
- Modify: `dxaml/xcp/components/runtimeEnabledFeatures/inc/OptionalChangeState.h`
- Modify: `dxaml/xcp/dxaml/lib/XamlOptionalChanges_Partial.cpp`
- Modify: `dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp`
- Modify: `dxaml/test/infra/client/lib/WindowHelper.cpp`
- Modify: `controls/test/MUXControlsTestApp/Utilities/APITestBase.cs`
- Modify generated files produced by the repository's XamlOM/code-generation build.

**Interfaces:**
- Produces: `OptionalChangeState::IsSkipWindowRedirectionSurfaceEnabled() -> bool`
- Produces: `XamlChangeId.SkipWindowRedirectionSurface = 63530879`

- [ ] **Step 1: Add the XamlChangeId model value**

```csharp
DeferContextFlyoutInit = 61098986,
SkipWindowRedirectionSurface = 63530879,
```

- [ ] **Step 2: Add the process-wide bit and query helper**

```cpp
constexpr int BitIndex_SkipWindowRedirectionSurface = 4;

inline bool IsSkipWindowRedirectionSurfaceEnabled()
{
    return IsOptionalChangeEnabled(BitIndex_SkipWindowRedirectionSurface);
}
```

- [ ] **Step 3: Map the public enum to the bit**

```cpp
case xaml_settings::XamlChangeId_SkipWindowRedirectionSurface:
    return OptionalChangeState::BitIndex_SkipWindowRedirectionSurface;
```

- [ ] **Step 4: Add the creation-time extended style**

Include `OptionalChangeState.h`, then change `DesktopWindowImpl::CreateDesktopWindow`:

```cpp
const DWORD extendedStyle =
    OptionalChangeState::IsSkipWindowRedirectionSurfaceEnabled()
        ? WS_EX_NOREDIRECTIONBITMAP
        : 0;

_CreateWindow(
    extendedStyle,
    s_windowClassName,
    s_defaultWindowTitle,
    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    CW_USEDEFAULT,
    SW_HIDE,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    nullptr,
    nullptr);
```

- [ ] **Step 5: Bypass ineffective GDI erase when opted in**

At the start of `WM_ERASEBKGND` handling:

```cpp
if (OptionalChangeState::IsSkipWindowRedirectionSurfaceEnabled())
{
    return 1;
}
```

- [ ] **Step 6: Add the optional-change name to native and managed test parsing**

Map the case-insensitive string `SkipWindowRedirectionSurface` to the new enum in
`WindowHelper.cpp` and `APITestBase.cs`. Do not enable it in each harness's
enable-all-default path.

- [ ] **Step 7: Regenerate the enum/IDL outputs**

Run the repository's normal Microsoft.UI.Xaml code-generation/build target and retain
only generated changes caused by the new XamlOM value.

- [ ] **Step 8: Build the XAML DLL**

Run the repository's Microsoft.UI.Xaml project build for `Debug|x64`.
Expected: build succeeds with zero new warnings or errors.

- [ ] **Step 9: Commit**

```powershell
git add -- dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Settings.cs `
  dxaml/xcp/components/runtimeEnabledFeatures/inc/OptionalChangeState.h `
  dxaml/xcp/dxaml/lib/XamlOptionalChanges_Partial.cpp `
  dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp `
  dxaml/test/infra/client/lib/WindowHelper.cpp `
  controls/test/MUXControlsTestApp/Utilities/APITestBase.cs
git commit -m "Add no-redirection window optional change" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

### Task 2: Add per-island placeholder composition

**Files:**
- Modify: `dxaml/xcp/components/comptree/inc/DCompTreeHost.h`
- Modify: `dxaml/xcp/components/comptree/DCompTreeHost.cpp`

**Interfaces:**
- Consumes: `CXamlIslandRoot*`, framework background color, and current island size.
- Produces:

```cpp
_Check_return_ HRESULT PrepareXamlIslandForWindowShow(
    _In_ CXamlIslandRoot* xamlIslandRoot,
    _In_ UINT32 backgroundColor);

bool HasXamlIslandWindowPlaceholder(_In_ CXamlIslandRoot* xamlIslandRoot) const;
```

- [ ] **Step 1: Add per-island state**

Extend `XamlIslandRenderData`:

```cpp
wrl::ComPtr<WUComp::IVisual> windowPlaceholderVisual;
```

Declare `PrepareXamlIslandForWindowShow` and a test-query helper.

- [ ] **Step 2: Write unit-level state tests where DCompTreeHost test infrastructure permits**

Verify that an island render-data entry starts without a placeholder, reports one
after preparation, and loses it after removal/root connection. If MockDComp cannot
exercise `IContentIslandExperimental::put_Root`, cover these assertions through the
DXaml test hook in Task 4 instead.

- [ ] **Step 3: Implement placeholder preparation**

```cpp
_Check_return_ HRESULT DCompTreeHost::PrepareXamlIslandForWindowShow(
    _In_ CXamlIslandRoot* xamlIslandRoot,
    _In_ UINT32 backgroundColor)
{
    auto islandData = m_islandRenderData.find(xamlIslandRoot);
    IFCEXPECT_RETURN(islandData != m_islandRenderData.end());

    auto& renderData = islandData->second;
    if (renderData.contentConnected || renderData.windowPlaceholderVisual)
    {
        return S_OK;
    }

    wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;
    IFC_RETURN(GetCompositor()->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
    IFC_RETURN(colorBrush->put_Color(ColorUtils::GetWUColor(backgroundColor)));

    wrl::ComPtr<WUComp::ICompositionBrush> brush;
    IFC_RETURN(colorBrush.As(&brush));

    wrl::ComPtr<WUComp::ISpriteVisual> spriteVisual;
    IFC_RETURN(GetCompositor()->CreateSpriteVisual(spriteVisual.ReleaseAndGetAddressOf()));
    IFC_RETURN(spriteVisual->put_Brush(brush.Get()));

    const auto size = xamlIslandRoot->GetSize();
    IFC_RETURN(spriteVisual->put_Size({ size.Width, size.Height }));
    IFC_RETURN(spriteVisual.As(&renderData.windowPlaceholderVisual));

    wrl::ComPtr<ixp::IContentIslandExperimental> contentIsland;
    IFC_RETURN(xamlIslandRoot->GetContentIsland()->QueryInterface(
        IID_PPV_ARGS(contentIsland.ReleaseAndGetAddressOf())));
    IFC_RETURN(contentIsland->put_Root(renderData.windowPlaceholderVisual.Get()));

    return CommitMainDevice();
}
```

Use the repository's accepted `Vector2` initialization style if aggregate
initialization does not compile.

- [ ] **Step 4: Keep the placeholder sized**

In `UpdateXamlIslandTargetSize`, after updating `WindowsPresentTarget`, apply the
same `CXamlIslandRoot::GetSize()` to `windowPlaceholderVisual`.

- [ ] **Step 5: Replace the placeholder with real content**

In `ConnectXamlIslandTargetRoots`, retain the existing `put_Root(wucVisual)` call.
After it succeeds:

```cpp
const bool replacedWindowPlaceholder =
    renderData.windowPlaceholderVisual != nullptr;
renderData.windowPlaceholderVisual.Reset();
```

Commit after root connection when `replacedWindowPlaceholder` is true, combining the
commit with the existing frame-rate-visual recommit condition:

```cpp
if (m_needsFrameRateVisual)
{
    ShowUIThreadCounters();
}

if (m_needsFrameRateVisual || replacedWindowPlaceholder)
{
    IFC_RETURN(CommitMainDevice());
}
```

- [ ] **Step 6: Clean up on island removal**

`m_islandRenderData.erase(islandData)` releases the stored visual. Before erasing,
clear the ContentIsland root if the placeholder is still active and the island is
not already closed; tolerate the repository-standard closed-island HRESULT.

- [ ] **Step 7: Build the XAML DLL**

Run the Microsoft.UI.Xaml project build for `Debug|x64`.
Expected: build succeeds.

- [ ] **Step 8: Commit**

```powershell
git add -- dxaml/xcp/components/comptree/inc/DCompTreeHost.h `
  dxaml/xcp/components/comptree/DCompTreeHost.cpp
git commit -m "Add top-level window placeholder visual" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

### Task 3: Prepare the placeholder before initial activation

**Files:**
- Modify: `dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp`
- Modify: `dxaml/xcp/dxaml/lib/DesktopWindowImpl.h` only if a helper declaration is needed.

**Interfaces:**
- Consumes: `DCompTreeHost::PrepareXamlIslandForWindowShow(CXamlIslandRoot*, UINT32)`
- Produces: initial activation ordering of placeholder commit, `ShowWindow`, then real root replacement.

- [ ] **Step 1: Extract effective HWND background selection**

Add a private helper or local reusable function that implements the existing
`WM_ERASEBKGND` theme lookup and returns the framework color:

```cpp
UINT32 DesktopWindowImpl::GetEffectiveWindowBackgroundColor()
{
    Theming::Theme appTheme = Theming::Theme::None;
    ctl::ComPtr<xaml::IUIElement> content;
    if (!m_bIsClosed && SUCCEEDED(get_ContentImpl(&content)) && content)
    {
        ctl::ComPtr<DirectUI::FrameworkElement> contentAsFE;
        if (SUCCEEDED(content.As(&contentAsFE)) && contentAsFE)
        {
            xaml::ElementTheme actualTheme{};
            if (SUCCEEDED(contentAsFE->get_ActualTheme(&actualTheme)) &&
                actualTheme != xaml::ElementTheme_Default)
            {
                appTheme = actualTheme == xaml::ElementTheme_Light
                    ? Theming::Theme::Light
                    : Theming::Theme::Dark;
            }
        }
    }

    return m_dxamlCoreNoRef->GetHandle()
        ->GetFrameworkTheming()
        ->GetHwndBackground(appTheme);
}
```

Preserve existing error-handling expectations in `WM_ERASEBKGND`; do not introduce a
silent fallback where the current code asserts or propagates.

- [ ] **Step 2: Reuse the helper in WM_ERASEBKGND**

Convert the returned color with `ColorUtils::GetWUColor` and retain the current GDI
fill behavior when the optional change is disabled.

- [ ] **Step 3: Prepare before first ShowWindow**

At the start of the first-activation block in `ActivateImpl`:

```cpp
if (m_bInitialWindowActivation &&
    OptionalChangeState::IsSkipWindowRedirectionSurfaceEnabled())
{
    ctl::ComPtr<DirectUI::XamlIslandRoot> xamlIslandRoot;
    ctl::ComPtr<xaml_hosting::IXamlIslandRoot> island =
        m_desktopWindowXamlSource->GetXamlIslandRootNoRef();
    IFC_RETURN(island.As(&xamlIslandRoot));

    auto coreIsland =
        static_cast<CXamlIslandRoot*>(xamlIslandRoot->GetHandle());
    IFC_RETURN(coreIsland->GetDCompTreeHost()->PrepareXamlIslandForWindowShow(
        coreIsland,
        GetEffectiveWindowBackgroundColor()));
}
```

This must execute after pending initial client size is applied and before
`::ShowWindow`.

- [ ] **Step 4: Build and smoke test**

Build the XAML DLL and launch an existing desktop sample with the optional change
disabled. Expected: behavior matches baseline.

- [ ] **Step 5: Commit**

```powershell
git add -- dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp dxaml/xcp/dxaml/lib/DesktopWindowImpl.h
git commit -m "Prepare window placeholder before first show" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

### Task 4: Add deterministic integration coverage

**Files:**
- Modify: `dxaml/xcp/dxaml/lib/DxamlCoreTestHooks_Partial.cpp`
- Modify the corresponding test-hook declaration/IDL files required by code generation.
- Modify: `dxaml/test/native/external/controls/window/WindowIntegrationTests.h`
- Modify: `dxaml/test/native/external/controls/window/WindowIntegrationTests.cpp`

**Interfaces:**
- Produces a test-only query that accepts a `DesktopWindowXamlSource` or `Window`
  associated with a top-level island and returns whether its DComp render data has
  an active window placeholder.

- [ ] **Step 1: Add a narrow placeholder-state test hook**

Resolve the target window's `CXamlIslandRoot` using the existing
`DesktopWindowXamlSource` test-hook pattern and call
`DCompTreeHost::HasXamlIslandWindowPlaceholder`.

- [ ] **Step 2: Add an opt-in initial activation test**

Declare the TAEF property:

```cpp
TEST_METHOD_PROPERTY(
    L"Data:XamlOptionalChanges",
    L"{SkipWindowRedirectionSurface:true}")
```

Create a secondary window, obtain its HWND before activation, verify
`GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_NOREDIRECTIONBITMAP`, disable rendering,
activate it, and assert that the placeholder is active.

- [ ] **Step 3: Verify first-frame replacement**

Re-enable rendering, wait for idle/first frame, then assert that the placeholder is
no longer active and the real content remains visible.

- [ ] **Step 4: Add disabled behavior coverage**

Run the equivalent test with:

```cpp
TEST_METHOD_PROPERTY(
    L"Data:XamlOptionalChanges",
    L"{SkipWindowRedirectionSurface:false}")
```

Verify the extended style bit is absent and no placeholder is installed.

- [ ] **Step 5: Add close-before-frame and multiple-window coverage**

With rendering disabled, activate and close one opted-in window and verify no crash.
Create two opted-in windows and verify each reports an independent placeholder before
rendering resumes.

- [ ] **Step 6: Run targeted tests**

Run the window integration test DLL with only the new test methods.
Expected: all new methods pass in both optional-change modes.

- [ ] **Step 7: Commit**

```powershell
git add -- dxaml/xcp/dxaml/lib/DxamlCoreTestHooks_Partial.* `
  dxaml/test/native/external/controls/window/WindowIntegrationTests.*
git commit -m "Test window placeholder activation behavior" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

### Task 5: Add the on/off validation sample

**Files:**
- Create: `Samples/WindowPlaceholderVisual/WindowPlaceholderVisual.vcxproj`
- Create: `Samples/WindowPlaceholderVisual/WindowPlaceholderVisual.vcxproj.filters`
- Create: `Samples/WindowPlaceholderVisual/pch.h`
- Create: `Samples/WindowPlaceholderVisual/pch.cpp`
- Create: `Samples/WindowPlaceholderVisual/program.cpp`
- Create: `Samples/WindowPlaceholderVisual/App.xaml`
- Create: `Samples/WindowPlaceholderVisual/App.xaml.h`
- Create: `Samples/WindowPlaceholderVisual/App.xaml.cpp`
- Create: `Samples/WindowPlaceholderVisual/MainWindow.xaml`
- Create: `Samples/WindowPlaceholderVisual/MainWindow.xaml.h`
- Create: `Samples/WindowPlaceholderVisual/MainWindow.xaml.cpp`
- Create: `Samples/WindowPlaceholderVisual/MainWindow.idl`
- Create supporting manifest/package files following the nearest C++ desktop sample.

**Interfaces:**
- Command line: `--placeholder=on` or `--placeholder=off`
- Default: off

- [ ] **Step 1: Scaffold from the smallest custom-entry-point C++ desktop sample**

Copy only the project structure needed for a WinUI desktop window. Use a new project
GUID and root namespace `WindowPlaceholderVisual`.

- [ ] **Step 2: Parse the launch mode before XAML initialization**

```cpp
enum class PlaceholderMode { Off, On };

PlaceholderMode ParsePlaceholderMode(std::wstring_view commandLine)
{
    return commandLine.find(L"--placeholder=on") != std::wstring_view::npos
        ? PlaceholderMode::On
        : PlaceholderMode::Off;
}
```

Reject simultaneous `--placeholder=on` and `--placeholder=off` with a message box and
nonzero exit code.

- [ ] **Step 3: Toggle XamlOptionalChanges before Application::Start**

```cpp
const auto mode = ParsePlaceholderMode(GetCommandLineW());
if (mode == PlaceholderMode::On)
{
    Microsoft::UI::Xaml::Settings::XamlOptionalChanges::EnableChange(
        Microsoft::UI::Xaml::Settings::XamlChangeId::SkipWindowRedirectionSurface);
}
else
{
    Microsoft::UI::Xaml::Settings::XamlOptionalChanges::DisableChange(
        Microsoft::UI::Xaml::Settings::XamlChangeId::SkipWindowRedirectionSurface);
}

Application::Start([](auto&&)
{
    make<WindowPlaceholderVisual::implementation::App>();
});
```

- [ ] **Step 4: Make startup flashing observable**

Create a window whose initial content is intentionally attached after a short
dispatcher-queue delay. Display the selected mode, current requested theme, and
instructions to launch the other mode. Use a strongly contrasting final content
color so an uninitialized white/black/transparent frame is visible during comparison.

- [ ] **Step 5: Build both modes**

Build `WindowPlaceholderVisual.vcxproj` for `Debug|x64`, then launch:

```powershell
WindowPlaceholderVisual.exe --placeholder=off
WindowPlaceholderVisual.exe --placeholder=on
```

Expected: both launch successfully; the on mode has no pre-content flash and reports
that the optional change is enabled.

- [ ] **Step 6: Commit**

```powershell
git add -- Samples/WindowPlaceholderVisual
git commit -m "Add window placeholder validation sample" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

### Task 6: Document and run final verification

**Files:**
- Modify: `docs/design-notes/xaml-window.md`

**Interfaces:**
- Documents `SkipWindowRedirectionSurface`, placeholder mitigation, and sample launch
  commands.

- [ ] **Step 1: Update the window design note**

Explain the redirection-surface cost, creation-time optional change, theme placeholder,
first-frame replacement, and GDI behavior change. Link to the validation sample.

- [ ] **Step 2: Run formatting and diff checks**

```powershell
git diff --check
git status --short
```

Expected: no whitespace errors; unrelated generated InkToolbar modifications remain
untouched.

- [ ] **Step 3: Run targeted validation**

Build the Microsoft.UI.Xaml DLL, build the sample, and run the new window integration
tests. Expected: all succeed.

- [ ] **Step 4: Review the complete change**

Use the WinUI code-review specialist on the complete diff and fix every confirmed
correctness issue caused by this work.

- [ ] **Step 5: Commit documentation/final fixes**

```powershell
git add -- docs/design-notes/xaml-window.md
git commit -m "Document no-flash window activation" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

