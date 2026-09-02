# Independent Window Placeholder Changes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Separate no-redirection HWND creation from placeholder composition and expose all four state combinations in the validation sample.

**Architecture:** Two independent `XamlChangeId` values map to separate process-wide bits. `DesktopWindowImpl` reads the redirection bit only during HWND creation/erase handling and reads the placeholder bit only during initial activation. The sample configures both bits before `Application::Start` and displays the resulting matrix state.

**Tech Stack:** C++/WinRT, Win32 HWND styles, Microsoft UI Composition, XamlOptionalChanges, TAEF integration tests, MSBuild.

## Global Constraints

- Both changes default off at the optional-change layer.
- `SkipWindowRedirectionSurface = 63530879` controls only `WS_EX_NOREDIRECTIONBITMAP` and `WM_ERASEBKGND`.
- `WindowPlaceholderVisual = 63530880` controls only placeholder composition.
- Neither change participates in the general performance opt-in.
- The sample defaults to redirection on and placeholder off.
- Preserve unrelated generated InkToolbar modifications.

---

### Task 1: Add the independent placeholder optional change

**Files:**
- Modify: `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Settings.cs`
- Modify: `dxaml/xcp/components/runtimeEnabledFeatures/inc/OptionalChangeState.h`
- Modify: `dxaml/xcp/dxaml/lib/XamlOptionalChanges_Partial.cpp`
- Modify: `dxaml/test/infra/client/lib/WindowHelper.cpp`
- Modify: `controls/test/MUXControlsTestApp/Utilities/APITestBase.cs`

**Interfaces:**
- Produces: `XamlChangeId.WindowPlaceholderVisual = 63530880`
- Produces: `OptionalChangeState::IsWindowPlaceholderVisualEnabled() -> bool`

- [ ] **Step 1: Add a failing optional-change mapping assertion**

Add `WindowPlaceholderVisual` to test metadata in the integration tests before adding
the mapping. Building the test infrastructure must fail because the enum/mapping is
not yet available.

- [ ] **Step 2: Add the model value and process bit**

```csharp
SkipWindowRedirectionSurface = 63530879,
WindowPlaceholderVisual = 63530880,
```

```cpp
constexpr int BitIndex_SkipWindowRedirectionSurface = 4;
constexpr int BitIndex_WindowPlaceholderVisual = 5;

inline bool IsWindowPlaceholderVisualEnabled()
{
    return IsOptionalChangeEnabled(BitIndex_WindowPlaceholderVisual);
}
```

- [ ] **Step 3: Map the new public enum**

```cpp
case xaml_settings::XamlChangeId_WindowPlaceholderVisual:
    return OptionalChangeState::BitIndex_WindowPlaceholderVisual;
```

Add case-insensitive `WindowPlaceholderVisual` mappings beside
`SkipWindowRedirectionSurface` in both test harnesses.

- [ ] **Step 4: Build the affected product and test projects**

Run the existing Debug x64 product/test build used for the placeholder feature.
Expected: the new enum and both harness mappings compile.

- [ ] **Step 5: Commit**

```powershell
git add -- dxaml\xcp\tools\XCPTypesAutoGen\XamlOM\Model\Microsoft.UI.Xaml.Settings.cs `
  dxaml\xcp\components\runtimeEnabledFeatures\inc\OptionalChangeState.h `
  dxaml\xcp\dxaml\lib\XamlOptionalChanges_Partial.cpp `
  dxaml\test\infra\client\lib\WindowHelper.cpp `
  controls\test\MUXControlsTestApp\Utilities\APITestBase.cs
git commit -m "Add window placeholder optional change" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

### Task 2: Separate product behavior and cover the state matrix

**Files:**
- Modify: `dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp`
- Modify: `dxaml/test/native/external/controls/window/WindowIntegrationTests.h`
- Modify: `dxaml/test/native/external/controls/window/WindowIntegrationTests.cpp`

**Interfaces:**
- Consumes: `OptionalChangeState::IsSkipWindowRedirectionSurfaceEnabled()`
- Consumes: `OptionalChangeState::IsWindowPlaceholderVisualEnabled()`

- [ ] **Step 1: Add failing matrix tests**

Declare two additional methods:

```cpp
BEGIN_TEST_METHOD(WindowNoRedirectionWithoutPlaceholder)
    TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    TEST_METHOD_PROPERTY(L"Data:XamlOptionalChanges",
        L"{SkipWindowRedirectionSurface:true,WindowPlaceholderVisual:false}")
END_TEST_METHOD()

BEGIN_TEST_METHOD(WindowPlaceholderWithRedirection)
    TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    TEST_METHOD_PROPERTY(L"Data:XamlOptionalChanges",
        L"{SkipWindowRedirectionSurface:false,WindowPlaceholderVisual:true}")
END_TEST_METHOD()
```

Update the existing both-enabled and both-disabled tests to specify both values.
Before the product gate changes, the placeholder-with-redirection case must fail.

- [ ] **Step 2: Gate initial placeholder preparation independently**

```cpp
if (OptionalChangeState::IsWindowPlaceholderVisualEnabled())
{
    // Resolve the island and call PrepareXamlIslandForWindowShow.
}
```

Do not change the existing `IsSkipWindowRedirectionSurfaceEnabled()` checks in
`CreateDesktopWindow` or `WM_ERASEBKGND`.

- [ ] **Step 3: Implement the two matrix test bodies**

For no-redirection without placeholder, disable rendering, create and activate the
window, assert `HasNoRedirectionBitmapExtendedStyle(windowHandle)` is true and
`HasWindowPlaceholder(window)` is false.

For placeholder with redirection, disable rendering, create and activate the window,
assert the extended style is absent and the placeholder is present; resume rendering
and assert the placeholder is removed.

- [ ] **Step 4: Run the targeted integration tests**

Run all placeholder/redirection methods together. Expected: all four combinations
report the independently selected style and placeholder state.

- [ ] **Step 5: Commit**

```powershell
git add -- dxaml\xcp\dxaml\lib\DesktopWindowImpl.cpp `
  dxaml\test\native\external\controls\window\WindowIntegrationTests.h `
  dxaml\test\native\external\controls\window\WindowIntegrationTests.cpp
git commit -m "Separate window redirection and placeholder behavior" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

### Task 3: Expose all four combinations in the sample

**Files:**
- Modify: `Samples/WindowPlaceholderVisual/pch.h`
- Modify: `Samples/WindowPlaceholderVisual/program.cpp`
- Modify: `Samples/WindowPlaceholderVisual/MainWindow.xaml.cpp`

**Interfaces:**
- Produces: `g_redirectionMode`
- Produces: `g_placeholderMode`
- Accepts: `--redirection=on|off`
- Accepts: `--placeholder=on|off`

- [ ] **Step 1: Define independent sample state**

```cpp
enum class FeatureMode
{
    Off,
    On,
};

extern FeatureMode g_redirectionMode;
extern FeatureMode g_placeholderMode;
```

- [ ] **Step 2: Parse both switches and reject per-switch conflicts**

Default `g_redirectionMode` to `On` and `g_placeholderMode` to `Off`. Parse each
switch independently. If both `on` and `off` occur for either switch, show:

```text
Specify only one value for each of --redirection=on|off and --placeholder=on|off.
```

- [ ] **Step 3: Configure both optional changes before XAML initialization**

Use named numeric constants so the sample can build against the pre-existing local
dev package metadata:

```cpp
constexpr auto SkipWindowRedirectionSurfaceChange =
    static_cast<XamlChangeId>(63530879);
constexpr auto WindowPlaceholderVisualChange =
    static_cast<XamlChangeId>(63530880);
```

Enable `SkipWindowRedirectionSurfaceChange` when redirection is off and disable it
when redirection is on. Enable or disable `WindowPlaceholderVisualChange` directly
from the placeholder switch.

- [ ] **Step 4: Display the selected matrix**

Show separate lines for `Redirection bitmap: ON|OFF` and
`Window placeholder: ON|OFF`. Show one matrix label: `Baseline`,
`No redirection only`, `Placeholder only`, or `Combined mitigation`. Display a
comparison command that toggles one dimension while preserving the other.

- [ ] **Step 5: Build and exercise the sample**

Build Debug x64, copy the locally built `Microsoft.UI.Xaml.dll` into the sample
output, and launch:

```text
--redirection=on  --placeholder=off
--redirection=off --placeholder=off
--redirection=on  --placeholder=on
--redirection=off --placeholder=on
```

Also launch one conflicting redirection command and one conflicting placeholder
command. Expected: four windows launch with the correct labels; conflicts exit with
code 2 after displaying the validation message.

- [ ] **Step 6: Commit**

```powershell
git add -- Samples\WindowPlaceholderVisual\pch.h `
  Samples\WindowPlaceholderVisual\program.cpp `
  Samples\WindowPlaceholderVisual\MainWindow.xaml.cpp
git commit -m "Add window placeholder state matrix sample" `
  -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
