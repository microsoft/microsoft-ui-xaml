---
name: test-suite-setup
description: Complete end-to-end setup and execution of the WinUI3 test suite locally. Delegates building to the build skill, then handles test payload creation, machine setup, and running all tests.
---

# WinUI3 Full Test Suite Setup & Execution

This skill performs a complete end-to-end test run of the WinUI3 repository on the local machine.
It covers every step from environment initialization through running all ~8000 tests.

## AI Agent Instructions

> **When something goes wrong, read these first.** This skill has accumulated reference material later in this document that will save hours of debugging:
> - **HRESULT Decoder Table** — every crash code we've seen (`0xC0000602`, `0x8007007E`, `0x80000003`, etc.) with cause + first fix.
> - **OSS Package Coherence** — symptom: cascading combase crashes / >10% External failures. Root cause is almost always an **incoherent OSS-pinned package set**, not your code.
> - **CI Test Invocation Map** — what hosting mode each CI work item uses. Critical: Managed tests run with `/p:HostingMode=WPF`, **not** AppX. Missing this flag will look like "all 700+ Managed tests blocked" when nothing is actually broken.
> - **Baseline Failure Catalog** — methodology for "is this my bug or pre-existing?" before treating any failure as a regression.
> - **ADO ↔ GitHub Mirror Reference** — which clone to edit when the file exists in both.

When this skill is invoked:

1. **First, ensure the environment is ready** — check if TestPayload exists and is populated. If not, invoke the **`build` skill** to initialize and build the repo (product + tests), then run Steps 3-5 below (create payload, machine setup, install runtimes).

   > **Platform/configuration:** The default flavor is `x64 Debug (amd64chk)`. If the user requests a different architecture (e.g., `arm64`) or configuration (`fre`/release), pass the appropriate flags to the build skill and adjust `-Platform`/`-Configuration` in all subsequent steps. Supported platforms: `x86`, `x64`, `arm64`, `arm64ec`. Supported configurations: `chk` (debug), `fre` (release).

2. **Determine the comparison branch**, then analyze changed files to suggest relevant tests.

   **Branch detection logic (in order):**
   1. If the user explicitly specified a branch (e.g., "compare against `release/2.8`"), use that.
   2. Try to auto-detect the PR target branch:
      ```powershell
      git --no-pager config --get "branch.$(git branch --show-current).merge" 2>$null
      ```
   3. If the current branch name suggests a release/servicing target (matches `release/*`, `servicing/*`, `hotfix/*`), ask the user via `ask_user`:
      > "Your branch appears to target a release branch. Which branch should I compare against?"
      > Choices: detected release branch, `main`, "Let me specify"
   4. Default to `main`.

   **Get changed files:**
   ```powershell
   git --no-pager diff --name-only <comparison-branch>...HEAD
   ```
   If the comparison branch is not available locally, fetch it first:
   ```powershell
   git fetch origin <comparison-branch>
   git --no-pager diff --name-only origin/<comparison-branch>...HEAD
   ```
   If that also fails, fall back to:
   ```powershell
   git --no-pager diff --name-only HEAD~5..HEAD
   ```

3. **Map changed files to test suites** using a hybrid approach: known mappings first, then dynamic discovery for anything new, and a user fallback for unmatched files.

   ### Step 3a: Known mappings (fast path)

   These curated rules cover the most common source → test relationships:

   | Changed files match | Recommended test DLLs | Hosting Mode |
   |---|---|---|
   | `dxaml/xcp/core/**`, `dxaml/xcp/dxaml/**` | `External.Foundation`, `External.Framework`, `External.Controls` | WPF |
   | `dxaml/xcp/components/imaging/**` | `Isolated.Foundation.Imaging` | None |
   | `dxaml/xcp/components/text/**` | `External.Foundation` (text tests), `Isolated.Text` | WPF / None |
   | `dxaml/xcp/components/focus/**` | `Isolated.Xaml.Focus.*` | None |
   | `dxaml/xcp/components/accesskeys/**` | `Isolated.AccessKeys.*` | None |
   | `dxaml/test/native/external/foundation/**` | `External.Foundation` | WPF |
   | `dxaml/test/native/external/framework/**` | `External.Framework` | WPF |
   | `dxaml/test/native/external/controls/**` | `External.Controls` | WPF |
   | `dxaml/test/native/external/automation/**` | `External.Automation` | WPF |
   | `dxaml/test/native/external/enterprise/**` | `External.Enterprise` | WPF |
   | `dxaml/test/native/external/win32/**` | `External.Win32` | WPF |
   | `dxaml/test/native/external/activation/**` | `External.Activation` | WPF |
   | `dxaml/test/native/external/tools/**` | `External.Tools` | WPF |
   | `dxaml/test/resources/**` (images, GIFs, masters) | `Isolated.Foundation.Imaging`, `External.Foundation` | None / WPF |
   | `dxaml/test/managed/**` | `Managed.*` | WPF |
   | `controls/dev/**` (WinUI controls source) | `MUXControls.Test.dll`, `External.Controls` | None / WPF |
   | `controls/test/**` | `MUXControls.Test.dll` | None |
   | `controls/idl/**` | `MUXControls.Test.dll`, `External.Controls` | None / WPF |
   | `src/XamlCompiler/**` | `Isolated.Tools.XbfGenerator` | None |
   | `dxaml/xcp/dxaml/themes/**` | `External.Controls`, `External.Framework` | WPF |
   | `Samples/**` | ScenarioTestSuite (sample app tests, separate payload) | N/A |
   | `build/**`, `eng/**`, `packaging/**` | No functional tests needed (build/infra only) | N/A |
   | Only `.md`, `.txt`, config files | No tests needed | N/A |

   ### Step 3b: Dynamic discovery (for new/unknown test areas)

   For any changed files that **don't match** the known mappings above, dynamically discover matching tests:

   1. **Scan available test DLLs** in the TestPayload:
      ```powershell
      Get-ChildItem "<repo-root>\TestPayload\x64chk\Test\Microsoft.UI.Xaml.Tests.*.dll" | Select-Object -ExpandProperty Name
      ```

   2. **Match by naming convention** — extract keywords from the changed file path and look for test DLLs with matching names:
      - Changed `dxaml/xcp/components/<area>/**` → look for `*Isolated.<Area>*` or `*External.<Area>*`
      - Changed `dxaml/test/native/external/<area>/**` → match `*External.<Area>*`
      - Changed `dxaml/test/native/isolated/<area>/**` → match `*Isolated.<Area>*`
      - Changed `controls/dev/<ControlName>/**` → match `*MUXControls*` and `*<ControlName>*`

   3. **Use TAEF metadata** for fine-grained matching when the DLL is found but you want to narrow to specific tests:
      ```powershell
      .\te.exe Test\<matched-dll> /list /select:"@Name='*<keyword>*'"
      ```
      This confirms the DLL actually contains tests related to the changed area.

   ### Step 3c: User fallback (for unresolvable changes)

   If any changed files still can't be mapped after Steps 3a and 3b, ask the user via `ask_user`:
   > "I found changes in `<path>` but couldn't auto-detect the relevant tests. Which test suite covers this area?"
   > Choices: list of all discovered test DLLs + "Skip — no tests needed" + "Let me specify"

   **If no source code files changed** (only docs, config, build scripts), tell the user no functional tests are needed.

   **If changes span multiple areas**, union all the recommended test DLLs from Steps 3a, 3b, and 3c.

4. **Present the recommendation** using `ask_user`. Show:
   - The files that changed (summarized)
   - The recommended test suites based on the mapping
   - Choices: `Run recommended tests (Recommended)`, `Choose different tests`, `Run all tests`

   If user picks "Choose different tests", **dynamically list all available test DLLs** from the TestPayload:
   ```powershell
   Get-ChildItem "<repo-root>\TestPayload\x64chk\Test\Microsoft.UI.Xaml.Tests.*.dll" |
     ForEach-Object { $_.BaseName -replace '^Microsoft\.UI\.Xaml\.Tests\.', '' }
   ```
   Present each discovered suite as a choice, plus:
   - `All Tests (all hosting modes)` — runs everything
   - `Custom query` — let user specify a wildcard pattern like `*CommandBar*`

   **Hosting mode inference** for dynamically discovered DLLs:
   - DLL name contains `External.` → WPF
   - DLL name contains `Isolated.` → None
   - DLL name contains `Managed.` → WPF
   - `MUXControls.Test.dll`, `UnitTests.dll` → None
   - Unknown → ask user or try without hosting mode

5. **Run the selected tests** using te.exe directly for specific DLLs (more precise than runtests.cmd):

   ```powershell
   cd <repo-root>\TestPayload\x64chk

   # For a specific test DLL (preferred for targeted runs):
   .\te.exe Test\Microsoft.UI.Xaml.Tests.Isolated.Foundation.Imaging.dll /p:SkipConsoleWindowMinimize

   # For External.* with WPF hosting:
   .\te.exe Test\Microsoft.UI.Xaml.Tests.External.Controls.dll /p:SkipConsoleWindowMinimize /p:HostingMode=WPF

   # For Managed.* with WPF hosting:
   .\te.exe Test\Microsoft.UI.Xaml.Tests.Managed.Controls.dll /p:SkipConsoleWindowMinimize /p:HostingMode=WPF

   # For multiple DLLs at once:
   .\te.exe Test\Microsoft.UI.Xaml.Tests.Isolated.Foundation.Imaging.dll Test\Microsoft.UI.Xaml.Tests.External.Foundation.dll /p:SkipConsoleWindowMinimize

   # Using runtests.cmd with wildcard (runs across all DLLs):
   .\runtests.cmd *AnimatedImage* -HostingMode:WPF

   # For ALL tests — run each hosting mode separately:
   .\runtests.cmd * -HostingMode:UAP
   .\runtests.cmd * -HostingMode:WPF
   .\runtests.cmd * -HostingMode:Win32Explicit
   .\runtests.cmd * -HostingMode:None
   ```

6. **After tests complete**, provide a detailed results summary. Parse the TAEF output and present:

   **Required summary format:**
   - A results table per test suite with columns: Total, Passed, Failed, Blocked, Skipped
   - For each **failed** test: the full test name, the failure reason (e.g., assertion message, HRESULT, CRC mismatch), and the source file/line if available
   - For each **blocked** test: the full test name and the blocking reason (e.g., missing privileges, missing Te.Service, test marked Ignore)
   - An overall verdict: whether the changes are safe (no new failures) or need attention
   - Actionable next steps for any failures or blocked tests (e.g., "run from admin prompt", "update master image", "pre-existing failure unrelated to your changes")

   **How to extract failure details:** Search the TAEF output for lines containing `[Failed]`, `[Blocked]`, `Error:`, `Verify:` (failed assertions), and `Summary:`. Use `Select-String` or `Select-Object -Last` to find relevant lines. For each failed test, look for the lines between `StartGroup: <testname>` and `EndGroup: <testname> [Failed]` to capture the full failure context.

   **Example summary:**

   ```
   ### Test Results: Isolated.Foundation.Imaging
   | Total | Passed | Failed | Blocked | Skipped |
   |-------|--------|--------|---------|---------|
   | 39    | 38     | 1      | 0       | 0       |

   #### Failed Tests
   1. **DecodingUnitTests::Jpeg**
      - Reason: CRC mismatch (Expected 0x5fd3ea9b, Actual 0xda5a7f30)
      - Source: DecodingUnitTests.cpp, Line 111
      - Action: Pre-existing environment-specific failure, unrelated to your changes. Can be ignored.

   #### Blocked Tests
   (None)

   ### Overall Verdict
   ✅ Your changes did not introduce any new test failures.
   ```

   **Classify failures as:**
   - **New failure** (likely caused by the current changes) — needs investigation
   - **Pre-existing failure** (exists on main branch too) — note it but don't block
   - **Environment-specific** (CRC mismatch, timing, machine config) — suggest rerun or admin prompt

   If any tests are blocked due to missing admin privileges or Te.Service, provide the exact command to rerun them from an elevated prompt.

**Hosting mode rules:**
- `External.*` tests → use `/p:HostingMode=WPF` for local runs (UAP requires full UWP container)
- `Isolated.*` tests → no hosting mode needed (unit tests, no UI)
- `Managed.*` tests → use `/p:HostingMode=WPF` (C# tests only work in WPF mode)
- `MUXControls.Test.dll` / `UnitTests.dll` → no hosting mode needed
- When user selects "All Tests", run all 4 hosting modes sequentially

## Prerequisites

- Windows 10/11 (20H2 or later recommended)
- Visual Studio 2022 with C++ workloads and Spectre mitigation libs
  - If missing Spectre libs, import the `.vsconfig` file from the repo root via VS Installer
- Admin privileges (required for test machine setup and running tests)
- A visible desktop session (tests launch UI apps and send input — do NOT run over minimized RDP or locked screen)

## Step 1–2: Build the Repository (Delegate to Build Skill)

Before creating the test payload, the repo must be initialized and built (product + tests).

**Invoke the `build` skill** to handle this. When delegating, request:
- A **full repo build** (product + tests): `.\initrun.ps1 .\build.cmd /q /b`
- The `/b` flag is recommended for test-suite prep to avoid PCH memory exhaustion on limited-memory machines.
- For a clean first-time build, use `/c` as well: `.\initrun.ps1 .\build.cmd /q /c /b`

The build skill handles init, flavor selection, troubleshooting (PCH errors, stale PCH, missing Spectre libs, etc.).

> **Note:** `amd64` and `x64` are used interchangeably. `init.ps1` uses `amd64chk` while `CreateTestPayload.ps1` uses `-Platform x64 -Configuration chk`, but they produce output in the same `TestPayload\x64chk\` directory. Ensure the build flavor matches the payload platform/configuration.

## Step 3: Create the Test Payload

The test payload aggregates all built test binaries, TAEF infrastructure, and dependencies into a single directory.

```powershell
.\initrun.ps1 powershell -ExecutionPolicy Bypass -File test\CreateTestPayload.ps1 -Platform x64 -Configuration chk
```

**Parameters:**
- `-Platform`: `x86`, `x64`, `arm64`, `arm64ec` (must match your build flavor)
- `-Configuration`: `chk` or `fre` (must match your build flavor)
- `-Mode`: `DevTestSuite` (default, for functional tests) or `ScenarioTestSuite` (for sample app tests)
- `-Clean`: Wipe the TestPayload directory before creating

The payload is created at `<repo-root>\TestPayload\<platform><config>\` (e.g., `TestPayload\x64chk\`).

## Step 4: One-Time Machine Setup (Admin Required)

This sets up crash dump collection, installs test certificates, installs required AppX framework packages,
and kills processes known to interfere with tests.

```powershell
# Open an ADMIN PowerShell prompt, then:
cd <repo-root>\TestPayload\x64chk
.\testmachine-prerun.cmd
```

**What it does:**
- Enables developer mode (sideloading without dev license)
- Disables network flyout
- Adds Windows Defender exclusion for test directory
- Kills interfering processes (Photos, Skype, YourPhone, etc.)
- Installs VCLibs and .NET runtime AppX packages
- Configures crash dump collection for `te.exe` and `te.processhost.exe`

This only needs to be run once per machine (or after OS reinstall).

## Step 5: Install .NET Desktop Runtime (if needed)

If managed (C#) tests fail to launch, install the .NET Desktop Runtime:

```powershell
cd <repo-root>\TestPayload\x64chk
.\dotnet-windowsdesktop-runtime-installer.exe /install /quiet /norestart
```

## Step 6: Install VC++ Redistributable (if needed)

```powershell
cd <repo-root>\TestPayload\x64chk
.\vc_redist.x64.exe /install /quiet /norestart
```

## Step 7: Run All Tests

From the TestPayload directory, use `runtests.cmd` to run tests. Tests use the TAEF (te.exe) framework.

### Run ALL tests (all hosting modes):

WinUI tests run in different hosting modes (UAP, WPF, Win32Explicit). You need to run each mode separately.

```powershell
cd <repo-root>\TestPayload\x64chk

# Run all UAP-hosted tests (native C++ tests):
.\runtests.cmd * -HostingMode:UAP

# Run all WPF-hosted tests (native + managed C# tests):
.\runtests.cmd * -HostingMode:WPF

# Run all Win32Explicit-hosted tests:
.\runtests.cmd * -HostingMode:Win32Explicit

# Run tests with no hosting mode (unit tests, app tests):
.\runtests.cmd * -HostingMode:None
```

### Run a specific test or subset:

```powershell
# Run tests matching a name pattern:
.\runtests.cmd *CommandBar*

# Run a specific test class:
.\runtests.cmd *CommandBarAutomationIntegrationTests*

# Run with WPF hosting:
.\runtests.cmd *MenuFlyout* -HostingMode:WPF
```

### Useful flags:

```powershell
# See test statistics without running:
.\runtests.cmd * -Stat

# List tests without running them:
.\runtests.cmd *CommandBar* /list

# Wait for debugger attachment:
.\runtests.cmd *MyTest* -WaitForDebugger

# Run including disabled/ignored tests:
.\runtests.cmd *MyTest* -RunIgnoredTests

# Run tests in a loop (for reliability testing):
.\runtests.cmd *MyTest* -RunTestsInALoop

# Stop on first failure:
.\runtests.cmd *MyTest* -TerminateOnFirstFailure

# Slow down test execution to watch it:
.\runtests.cmd *MyTest* /p:GoSlow
```

### Using te.exe directly (advanced):

```powershell
cd <repo-root>\TestPayload\x64chk

# Run a specific test DLL with WPF hosting:
.\te.exe Test\Microsoft.UI.Xaml.Tests.External.Controls.dll /p:SkipConsoleWindowMinimize /p:HostingMode=WPF

# Run with a TAEF select query:
.\te.exe Test\Microsoft.UI.Xaml.Tests.External.Controls.dll /select:"@Name='*CommandBar*'" /p:SkipConsoleWindowMinimize

# Run managed tests in WPF mode:
.\te.exe Test\Microsoft.UI.Xaml.Tests.Managed.Controls.dll /p:HostingMode=WPF /p:SkipConsoleWindowMinimize
```

## Full Automated Script (All Steps)

Below is a complete PowerShell script that runs all steps end-to-end.
**Must be run from an admin PowerShell prompt.**

Adjust `$platform` and `$config` if targeting a different architecture (e.g., `arm64` + `chk` for ARM64 debug).

```powershell
# === WinUI3 Full Test Suite — End-to-End ===
# Run from admin PowerShell at repo root

$ErrorActionPreference = "Stop"
$repoRoot = Get-Location

# --- Configuration ---
# Change these to target a different architecture/configuration
$platform = "x64"       # x86, x64, arm64, arm64ec
$config   = "chk"       # chk (debug) or fre (release)
$flavor   = "${platform}${config}"   # e.g., x64chk, arm64chk

# --- Step 1: Init ---
Write-Host "=== Step 1: Initializing build environment ===" -ForegroundColor Cyan
.\init.ps1

# --- Step 2: Build (clean, reduced parallelism) ---
Write-Host "=== Step 2: Building repository (product + tests) ===" -ForegroundColor Cyan
.\initrun.ps1 .\build.cmd /q /c /b

# --- Step 3: Create Test Payload ---
Write-Host "=== Step 3: Creating test payload ===" -ForegroundColor Cyan
.\initrun.ps1 powershell -ExecutionPolicy Bypass -File test\CreateTestPayload.ps1 -Platform $platform -Configuration $config -Clean

# --- Step 4: Machine Setup ---
Write-Host "=== Step 4: One-time machine setup ===" -ForegroundColor Cyan
Push-Location "$repoRoot\TestPayload\$flavor"
.\testmachine-prerun.cmd

# --- Step 5: Install runtimes if present ---
Write-Host "=== Step 5: Installing runtimes ===" -ForegroundColor Cyan
if (Test-Path ".\dotnet-windowsdesktop-runtime-installer.exe") {
    Start-Process -Wait -FilePath ".\dotnet-windowsdesktop-runtime-installer.exe" -ArgumentList "/install /quiet /norestart"
}
if (Test-Path ".\vc_redist.x64.exe") {
    Start-Process -Wait -FilePath ".\vc_redist.x64.exe" -ArgumentList "/install /quiet /norestart"
}

# --- Step 6: Run Tests ---
Write-Host "=== Step 6: Running all tests ===" -ForegroundColor Cyan

# Run UAP tests
Write-Host "--- Running UAP tests ---" -ForegroundColor Yellow
.\runtests.cmd * -HostingMode:UAP

# Run WPF tests
Write-Host "--- Running WPF tests ---" -ForegroundColor Yellow
.\runtests.cmd * -HostingMode:WPF

# Run Win32Explicit tests
Write-Host "--- Running Win32Explicit tests ---" -ForegroundColor Yellow
.\runtests.cmd * -HostingMode:Win32Explicit

# Run no-hosting-mode tests (unit tests, app tests)
Write-Host "--- Running unit/app tests ---" -ForegroundColor Yellow
.\runtests.cmd * -HostingMode:None

Pop-Location
Write-Host "=== All tests complete ===" -ForegroundColor Green
```

## Notes

- **~8000 tests total**, taking ~8 hours to run serially across all hosting modes.
- **Tests require a visible desktop session.** UI tests will fail if the Remote Desktop window is minimized, the screen is locked, or another window has focus. Look for errors like `RpcSetForegroundWindow` or `SetForegroundWindow: Failed`.
- **Managed tests (C#)** can only run in WPF hosting mode (not UAP).
- **Native tests (C++)** can run in both UAP and WPF modes.
- Use `-TerminateOnFirstFailure` for quick validation.
- Use `-Stat` to see test statistics before running.
- Test output is in the `TestPayload\<platform><config>\Test\WexLogFileOutput\` folder.

## Test Suite Reference

All test DLLs are in `TestPayload\<platform><config>\Test\`. Here is the complete listing:

### External Tests (E2E UI integration — C++ native, require UAP or WPF hosting)

| DLL | Area |
|---|---|
| `External.Activation` | App activation |
| `External.Adaptability` | DPI/scaling/adaptability |
| `External.Automation` | UI Automation / accessibility |
| `External.Controls` | Built-in XAML controls (Button, ComboBox, CommandBar, etc.) |
| `External.Convergence` | API convergence |
| `External.Enterprise` | Enterprise scenarios (ListView, GridView, etc.) |
| `External.Foundation` | Core rendering, images, text, media (includes AnimatedImageTests) |
| `External.Framework` | XAML framework (binding, templates, resources) |
| `External.Quality` | Visual quality tests |
| `External.Test` | Test infrastructure validation |
| `External.Tools` | App analysis tools |
| `External.Win32` | Win32/Desktop hosting |

### Isolated Tests (Unit tests — C++ native, no hosting mode needed)

| DLL | Area |
|---|---|
| `Isolated.AccessKeys.*` (6 DLLs) | Keyboard access keys |
| `Isolated.Adaptability.Qualifiers` | Qualifier logic |
| `Isolated.Associative/Base/Collection/Colors/Com` | Core data structures |
| `Isolated.Controls.Moco/Pivot/RelativePanel/Theming` | Control-specific unit tests |
| `Isolated.Core.Gestures/Input` | Touch/pointer input |
| `Isolated.Enterprise.*` (2 DLLs) | ItemIndexRange, VisualStates |
| `Isolated.Foundation.Animation/Brushes/Elements/Math/Transforms` | Foundation unit tests |
| `Isolated.Foundation.Imaging` | Image decoding (includes AnimatedGifUnitTests) |
| `Isolated.Foundation.OfferableHeap/ThemeAnimationsHelper/Threading` | Memory, animations, threading |
| `Isolated.Framework.*` (9 DLLs) | Framework internals (CValue, Parser, DependencyObject, etc.) |
| `Isolated.Graphics/Legacy/Lifetime/Strings/Text` | Various |
| `Isolated.Tools.AppAnalysis.*/XbfGenerator` | Tooling |
| `Isolated.Xaml.AccessKeys.*/Focus.*` | Focus/AccessKey subsystems |
| `Isolated.XamlDiagnostics` | XAML diagnostics |

### Managed Tests (C# — WPF hosting mode only)

| DLL | Area |
|---|---|
| `Managed.AccessKeys` | Access keys (C#) |
| `Managed.Animation.EasingFunctionBaseTests` | Animation easing |
| `Managed.Common` | Common framework tests |
| `Managed.Controls` | Controls (C#) |
| `Managed.Enterprise.Moco` | Enterprise/Moco |
| `Managed.Foundation` | Foundation (C#) |
| `Managed.Framework` | Framework (C#) |
| `Managed.Media` | Media |
| `Managed.Win32.Common/Hosting` | Win32 hosting |

### Other Test DLLs

| DLL | Area |
|---|---|
| `MUXControls.Test.dll` | Modern WinUI controls (TreeView, NavigationView, InfoBar, etc.) |
| `UnitTests.dll` | Core XAML unit tests |
| `DxamlCoreTipUnitTests.dll` | Core tooltip unit tests |

### Mapping: Source Code → Test DLL

| Source files changed in | Test DLL to run |
|---|---|
| `dxaml/xcp/**` (core runtime) | `External.Foundation`, `External.Framework`, `External.Controls` |
| `controls/dev/**` (WinUI controls) | `MUXControls.Test.dll`, `External.Controls` |
| `dxaml/xcp/components/imaging/**` | `Isolated.Foundation.Imaging` |
| `dxaml/test/native/external/<area>/**` | `External.<Area>` |
| `dxaml/test/resources/` (GIF/image assets) | `Isolated.Foundation.Imaging`, `External.Foundation` |
| `controls/test/**` | `MUXControls.Test.dll` |

## OSS Package Coherence

The OSS build of WinUI pins a small set of dependency packages directly in `eng/Versions.props` (the internal build ignores these and uses Maestro/Darc-managed versions instead). When those pins are not from the **same internal build run**, the symptoms are dramatic but the diagnosis is subtle.

### Symptom recognition

If you see **any** of these, suspect package incoherence before suspecting your code:

- Mass `0xC0000602` (`RoFailFastWithErrorContext`) crashes on test startup — combase reports WinRT activation metadata mismatch.
- External category pass rate drops below ~80% (healthy is ~91%).
- Adaptability / Automation / Tools / Enterprise DLLs show cascading "Blocked" or crashes.
- `Microsoft.UI.Xaml.dll` loads but a controls type fails to activate at runtime.
- Same test passes in the internal CI but crashes locally on an OSS-restored build.

### The `IsInternalWinUIBuild` gating pattern

`eng/Versions.props` (around line 21) defines:

```xml
<IsInternalWinUIBuild Condition="'$(IsInternalWinUIBuild)' == '' AND Exists('..\.azuredevops\')">true</IsInternalWinUIBuild>
```

The flag is `true` whenever `..\.azuredevops\` exists (i.e., when the repo is cloned from the ADO mirror, not from GitHub). Lines under `Condition="'$(IsInternalWinUIBuild)' != 'true'"` are the **OSS-only pins** — typically 5–7 packages:

- `Microsoft.Internal.FrameworkUdk`
- `Microsoft.WindowsAppSDK.InteractiveExperiences` (IXP) and its transport package
- `Microsoft.WindowsAppSDK.Foundation` and its transport package
- `Microsoft.WindowsAppSDK.Base`
- `Microsoft.WindowsAppSDK.WinUIDetails`

Internal builds read versions from `eng/Version.Details.xml` (Maestro dep-flow). OSS builds read the literal pins.

### What "coherent" means

Two packages are coherent when their version strings prove they were produced by the **same internal build run** (byte-identical date stamp / build label, e.g. version suffixes ending in the same `YYMMDD-HHMM.B` token). Examples of incoherence that look fine to a human reviewer but break at runtime:

- `FrameworkUdk` from build A combined with the `FrameworkUdk` DLL embedded inside the IXP package from build B — both packages claim the same nominal version, but the embedded DLL differs by KB.
- MUX compiled against `WinUIDetails` headers from build A, then linked at deployment time against the runtime IXP from build B.
- `Foundation` and `Base` snapped from different days.

### `WinUIDetails` is the secret killer

`WinUIDetails` is a headers-only NuGet package, but the headers ARE the compile-time ABI surface (vtable layouts, IIDs, struct sizes). If `WinUIDetails` is from a different build run than the runtime IXP / Foundation packages, MUX compiles successfully against one set of vtable layouts and then encounters a different set at activation time → `0xC0000602`.

### How to find a coherent snapshot

Two paths:

1. **Dep-flow / BAR (preferred)** — query the BAR (Build Asset Registry) or use Darc to find a recent build that produced all the packages together. The build label suffix is the coherence proof.
2. **Cache inspection** — check what's already on the machine: `packages\`, `packages_temp\`, `~\.nuget\packages\` for matching date stamps across the OSS-pinned package set.

### Validating the snapshot is published to the OSS feed

OSS builds restore from the `WinUI-Dependencies` feed on `dev.azure.com/shine-oss`. To verify a specific version is published before pinning:

```powershell
$tok = (az account get-access-token --resource '499b84ac-1321-427f-aa17-267ca6975798' --query accessToken -o tsv)
$pkg = 'microsoft.internal.frameworkudk'  # lowercase
Invoke-RestMethod -Uri "https://pkgs.dev.azure.com/shine-oss/microsoft-ui-xaml/_packaging/WinUI-Dependencies/nuget/v3-flatcontainer/$pkg/index.json" -Headers @{ Authorization = "Bearer $tok" }
```

A 200 with the version listed = ready to pin. A 404 = package needs to be promoted/published first.

### The two-tier package consumption model

- **Product DLL** (`Microsoft.UI.Xaml.dll`) consumes **transport** packages (`*TransportPackage`), gated by `IsInternalWinUIBuild`.
- **Test/sample apps** consume the **flat** packages (`LiftedIXPPackageName`, `FoundationPackageName`) — and these versions are typically set **unconditionally**, not gated.

Both tiers must be coherent or runtime DLLs in `TestPayload\` will conflict.

### PR pattern when bumping OSS pins

A PR that updates the OSS-pinned versions:

- Touches only `eng/Versions.props` (and `eng/Version.Details.xml` if needed for OSS-feed mirroring).
- All changes gated by `Condition="'$(IsInternalWinUIBuild)' != 'true'"` so internal CI is unaffected.
- Must include a pass-rate validation table (see "PR + Validation Template" below).
- Acceptance bar: zero new failures vs unchanged main.

## CI Test Invocation Map

When you need to know **what CI runs and how it runs it**, these two files are the source of truth:

- **`build/AzurePipelinesTemplates/WinUI-CreateTestPayload-Job.yml`** — defines each test work item: `testFilePathPattern` (which DLL or AppX) + `hostingMode`.
- **`Helix/GenerateHelixWorkItems.ps1`** — look for the line that generates the TAEF command:

  ```powershell
  $taefExtraParameters = "/p:HostingMode=$HostingMode"
  ```

  Every CI test invocation passes a `HostingMode` value to TAEF via `/p:`.

### CI ↔ local mapping

| CI work item | Test pattern | Hosting mode | Local equivalent |
|---|---|---|---|
| MUXControlsInteractionTests | `MUXControls.Test.dll` | Default | `runtests.cmd MUXControls.Test.dll` |
| MUXControlsApiTests | `UnpackagedApps\MUXControlsTestApp\MUXControlsTestApp.dll` | Default | run from `UnpackagedApps\MUXControlsTestApp\` |
| IXMPTests | `IXMPTestApp.appx` | Default | run from registered loose layout |
| MUXCoreTests | `Microsoft.UI.Xaml.Tests.External.*.dll` | UAP | `runtests.cmd ... -HostingMode:UAP` |
| MUXCoreTests-WPF | same | WPF | `-HostingMode:WPF` |
| **MUXCoreManagedTests-WPF** | `Microsoft.UI.Xaml.Tests.Managed.*.dll` | **WPF** | **`te.exe ... /p:HostingMode=WPF`** |
| MUXCoreTests-Win32Explicit | `Microsoft.UI.Xaml.Tests.External.*.dll` | Win32Explicit | `-HostingMode:Win32Explicit` |
| UnitTests (static / Isolated) | `Microsoft.UI.Xaml.Tests.Isolated.*.dll` | None | `-HostingMode:None` |

### The Managed-tests gotcha (READ THIS BEFORE DEBUGGING "BLOCKED" MANAGED TESTS)

**Managed tests are not AppX-hosted in CI.** They run via `te.processhost.exe` with WPF XamlIsland hosting — i.e., they need `/p:HostingMode=WPF`.

A local runner that omits this flag will appear to "block all 700+ Managed tests" with `0x80070002` / `0x8007007E` / `0x80000003` and lead the investigator down a `.rd.xml` / `.NET Native` rabbit hole that does not exist. The fix is one flag: `/p:HostingMode=WPF`.

This was a multi-week false wall. Don't repeat it.

## ADO ↔ GitHub Mirror Reference

The WinUI3 source code lives in two clones, kept in sync by a one-way mirror:

- **ADO** `microsoft.visualstudio.com/WinUI/_git/microsoft-ui-xaml-lift` (branch `main`) — source of truth for all mirrored content.
- **GitHub** `github.com/microsoft/microsoft-ui-xaml` (branch `winui3/main`) — read-only mirror **for mirrored files**. A small set of GitHub-only files (typically OSS-only docs and samples not present in ADO) can and should be edited directly on GitHub.

The mirror pipeline is `build/WinUI-MirrorMainSourceToExternalRepo-Official.yml`.

### Layout difference

The mirror uses `targetRepositorySubdirectory: "src"`. A file at ADO `eng/Versions.props` lands at GitHub `src/eng/Versions.props`. The GitHub repo's `main` branch is a legacy OSS-only branch — **do not edit it**; the mirror writes to `winui3/main`.

### Exclusions

`build/PipelineScripts/WinUISourceMirroringExclusions.txt` controls what does NOT mirror.

| Syntax | Meaning |
|---|---|
| `:/path/` | Exclude this path from the mirror |
| `:!/path/` | **Re-include** (negate) this path even if a parent is excluded |
| `:(glob,top)*.md` | Top-level glob exclusion |

Useful when re-enabling a small subset of an excluded tree (e.g., excluding `:/test/` but re-including `:!/test/scripts/` for the local runner scripts).

### "Which clone do I edit?" decision rule

- **File exists in ADO under any mirrored path** (most product/test source, this skill, etc.) → edit in **ADO**, create an ADO PR. Editing the GitHub copy will be silently overwritten on the next mirror sync.
- **File exists only on GitHub** (rare — typically OSS-only docs or samples added directly to the GitHub repo) → edit on GitHub.
- **Submodules** — `Samples/WinUIGallery` is a submodule pointer to a separate repo. The submodule contents are **not** mirrored as plain files; only the pointer commit is.

## Cross-Category Pass-Rate Reference

Expected pass-rate bands and approximate wall times for a healthy local validation run. Use these to calibrate "is my run healthy" before chasing failures.

| Category | Expected pass rate | Approx. test count | Approx. wall time | Notes |
|---|---|---|---|---|
| Isolated (unit, hosting `None`) | ≥99% | ~720 | ~2 min | Fastest signal — run first |
| IXMPTestApp | 100% | 1 | <1 min | Smoke test for AppX deployment |
| MUXControlsTestApp | ~95% | ~600 | ~40 min | UI assertion drift expected |
| MUXControls.Test (sample of ~50) | ~92% | ~50 | ~20 min | Full suite ~825 tests, multi-hour |
| External (UAP + WPF + Win32Explicit) | ~91% | ~1,200 | ~3 h | Use 5s keepalive (see Reliability) |
| Managed (`HostingMode=WPF`) | ~79–95% | ~720 | ~60–90 min | Requires `/p:HostingMode=WPF` |
| **Combined OSS validation target** | **≥94%** | **~4,100** | **~4–5 h** | A healthy OSS validation hits this band |

A pass rate that falls dramatically below the band for a category is almost always either:
1. A package coherence issue (see OSS Package Coherence), or
2. The runner being killed by idle-detection (see Reliability Tweaks).

It is **rarely** "your code change broke 30% of tests."

## Troubleshooting & Known Issues

### Build Errors

For build-related issues (PCH virtual memory exhaustion, stale precompiled headers, missing Spectre libs, `Run a full init first`, etc.), refer to the **`build` skill** which covers all build troubleshooting.

### Test Payload Errors

#### `CreateTestPayload.ps1` — robocopy exit codes
**Symptom:** Script appears to "fail" but robocopy exit codes below 8 are actually success.

**Root Cause:** Robocopy uses non-standard exit codes where 0-7 indicate various forms of success. The script handles this internally.

**Fix:** Only exit codes 8+ are actual failures. Check `CreateTestPayload.log` in the TestPayload directory for details.

#### TestPayload is empty or missing files
**Symptom:** Test payload directory exists but is missing test DLLs or TAEF binaries.

**Root Cause:** Build didn't produce all outputs (partial build, or wrong build target).

**Fix:** Ensure you ran a full build (not just `product` or `mux`):
```powershell
.\initrun.ps1 .\build.cmd /q /b   # builds product + tests
```
Then recreate the payload:
```powershell
.\initrun.ps1 powershell -ExecutionPolicy Bypass -File test\CreateTestPayload.ps1 -Platform x64 -Configuration chk -Clean
```

### Machine Setup Errors

#### `testmachine-prerun.cmd` fails with access denied
**Symptom:** Registry operations or AppX installs fail with permission errors.

**Fix:** Must run from an **elevated (admin)** PowerShell or cmd prompt.

#### AppX package install error `0x80073D06` or `0x80073CFB`
**Symptom:** `Add-AppxPackage` reports an error during machine setup.

**Root Cause:** A same or newer version of the package is already installed.

**Fix:** Safe to ignore — the setup script already handles this and prints "The same or higher version of this package is already installed."

### Test Execution Errors

#### `SetForegroundWindow: Failed` / `RpcSetForegroundWindow` errors
**Symptom:** Tests fail because the test app window cannot be brought to the foreground.

**Root Cause:** UI tests require an active, visible desktop session. The session is either minimized, locked, or another window has focus.

**Fix:**
- If using Hyper-V: Use basic "Virtual Machine Connection" (vmconnect.exe), NOT Enhanced Session
- If using Remote Desktop: Keep the RDP window visible and focused, do not minimize it
- Do not lock the screen while tests are running
- Check View menu in VM connect to ensure you're NOT using Enhanced Session

#### Tests show "blocked" status instead of running
**Symptom:** Managed C# tests report "blocked" and don't execute.

**Root Cause:** Managed (.NET) tests cannot run in UAP hosting mode. They must use WPF mode.

**Fix:** Run managed tests with WPF hosting:
```powershell
.\runtests.cmd *ManagedTestName* -HostingMode:WPF
```

#### `COM_END WindowHelper::VerifyTestCleanup: Error: Caught Platform::Exception^: Unspecified error`
**Symptom:** Tests fail with COM/platform errors during cleanup.

**Root Cause:** Usually caused by running tests over Remote Desktop or Hyper-V Enhanced Session.

**Fix:** Switch to basic VM connection (not Enhanced Session, not RDP). See the `SetForegroundWindow` fix above.

#### Tests fail to find test DLLs
**Symptom:** `te.exe` reports it cannot find test binaries.

**Root Cause:** Running runtests.cmd from the wrong directory.

**Fix:** You must `cd` into the TestPayload directory first:
```powershell
cd <repo-root>\TestPayload\x64chk
.\runtests.cmd *MyTest*
```

#### Query matched tests requiring conflicting HostingModes
**Symptom:** runtests.cmd auto-detection fails because the query matches tests in multiple hosting modes.

**Root Cause:** A broad wildcard (e.g., `*`) matches tests that require different hosting modes, and the script can only run one mode at a time.

**Fix:** Explicitly specify the hosting mode:
```powershell
.\runtests.cmd * -HostingMode:UAP
.\runtests.cmd * -HostingMode:WPF
.\runtests.cmd * -HostingMode:Win32Explicit
.\runtests.cmd * -HostingMode:None
```

### Init / Build Errors

For init and build-related issues (PCH virtual memory exhaustion, stale precompiled headers, missing Spectre libs, NuGet authentication, dotnet-install failures, etc.), refer to the **`build` skill** (`/.github/skills/build/SKILL.md`) which covers all build and init troubleshooting.

### HRESULT Decoder Table

Every crash code we've encountered, what it actually means, and the first thing to try.

| HRESULT | Symbolic name | Common cause | First fix |
|---|---|---|---|
| `0x80073CF6` / `0x80073CFB` / `0x80073D06` | `APPX_E_*` install conflicts | Same or newer version of an AppX is already registered | Benign in dev mode — loose layout still works. Uninstall the conflicting registration only if it actively prevents activation. |
| `0x80070002` | `ERROR_FILE_NOT_FOUND` | TAEF `DeploymentItem` source missing in deployment root | Stage the missing file from `BuildOutput\bin\<flavor>\Test\` to the AppX deployment root (parent of the AppX folder). |
| `0x8007007E` | `ERROR_MOD_NOT_FOUND` | Transitive DLL dependency missing at load time | Bulk-copy from `Test\UnpackagedApps\MUXControlsTestApp\` (or the equivalent staging dir) into the deployment root. |
| `0x80080204` | AppX activation failure | Manifest/identity mismatch, or TAEF launching the wrong host EXE | Verify the registered AppX matches what TAEF is launching; confirm the AppX manifest entry point. |
| `0x80000003` | `Wex.Common.dll` `DebugBreak` | Test framework hit a native breakpoint — usually because a test DLL is missing from inside the registered AppX | Re-stage test DLLs into the AppX install location (not just the deployment root parent). |
| `0x8027025B` | AppX activation failed | `IApplicationActivationManager` couldn't activate the identity | Confirm AppX is registered AND the test cert is in `TrustedPeople`. |
| `0xC0000602` | `RoFailFastWithErrorContext` | WinRT activation metadata mismatch — **almost always package incoherence** | See **OSS Package Coherence** section above. Don't debug your code first. |
| `0x80004005` | `E_FAIL` from TAEF | TAEF received a target file type it doesn't accept (e.g. `.msix` instead of `.appx`, or a DLL outside its registered AppX install location) | Pass the test DLL **inside** the registered AppX install location, not the loose `BuildOutput` path. |

### Baseline Failure Catalog

Before treating any failure as "caused by my change", reproduce it on **unchanged main** with the same package pins and the same TestPayload state. If it reproduces, it's pre-existing baseline drift, not your bug.

#### Categories of baseline failures we've consistently observed

- **External (component) DLLs** — typically 80–110 pre-existing failures on a healthy ~91% pass rate. Distribution roughly:
  - `Test`, `Win32`, `Convergence` — 3–4 failures each.
  - `Adaptability` — ~6 failures.
  - `Enterprise` — ~58–74 failures (largest contributor).
  - `Tools` — ~4–11 failures.
  - These persist across package version changes and across many weeks.
- **MUXControlsTestApp** (~95% expected) — UI-assertion drift is expected. Examples we've reproduced on unchanged main: `RatingControlTests` tap-and-return offsets, asynchronous-timing assertions in animation tests.
- **Isolated** — 1–2 single flakes (often clean on retry). Expected ≥99% pass.
- **Managed** (`HostingMode=WPF`) — some `Win32.Common` deterministic failures show up in the ~79% pass-rate sample.

#### What is NOT a baseline failure

- Any cascading `0xC0000602` activation crash — that's package incoherence.
- A sudden surge of "Blocked" results across multiple DLLs — that's the runner being killed by idle-detection (see Reliability Tweaks), not a real failure.
- A whole category dropping below its expected band — investigate the package set, not the individual tests.

### Test Runner Reliability Tweaks

Patterns that prevent the most common run-time waste during long local validation runs.

#### 5-second keepalive for long External runs

Without keepalive, idle-detection kills External DLL runs after a period of no input — and produces hundreds of "Blocked" results instead of real pass/fail. Two options:

1. Pass a TAEF foreground / idle-keepalive option appropriate for your TAEF version.
2. Run a parallel mouse-wiggle script: move the cursor by 1 pixel every ~5–15 seconds. This keeps `SetForegroundWindow` permission alive on the active session, which the UI tests need.

#### Background-shell pattern for long-running DLLs

Anything that runs for ≥5 minutes (Controls = 7+ min, Enterprise / Tools / Framework, full External run) should be launched detached so it survives Copilot CLI / shell restart:

```powershell
Start-Process pwsh -WindowStyle Normal -ArgumentList '-NoExit','-Command',"<run command> *> '<per-DLL log>'; Add-Content '<csv>' '<dll>,<status>'"
```

Tee output to a per-DLL log file and append a one-line CSV summary at completion — that gives you an audit trail across the full multi-hour run.

#### Crash dumps via WER LocalDumps

Configure HKCU once so any test-host crash produces a dump:

```
HKCU\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\te.processhost.exe
HKCU\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\TE.exe
   DumpFolder = <writable path>
   DumpType   = 2  (full dump)
```

Critical when chasing combase fast-fail crashes.

#### TAEF `/select` filter is unreliable

`TE.exe <dll> /select:"@Name='X' OR @Name='Y'"` sometimes runs the **full** suite anyway. When sampling for diagnostic signal:

1. Enumerate first: `TE.exe <dll> /list` → capture the full test name list.
2. Run the first N test names individually, not via `/select`.

A 10-tests-per-DLL sample across all DLLs in a category gives ~80% of the diagnostic signal in 10–15% of the wall time. **Sample by running fewer DLLs, not by trying to filter inside a DLL.**

#### TestPayload directory naming

Build outputs land at `BuildOutput\bin\amd64chk\` but TestPayload lands at `TestPayload\x64chk\` — same architecture, different naming convention. This is normal; don't try to "fix" it.

#### AppX-hosted tests need the right deployment root

For AppX-hosted tests, target the test DLL **inside the registered loose layout**, not under the raw `TestPayload\` directory. The deployment root for `DeploymentItem` resolution is the **parent** of the AppX folder.

### PR + Validation Template

For changes that affect the OSS test pipeline (Versions.props pins, mirror exclusions, test infra scripts):

#### Always include a pass-rate validation table

```
Category                    Total  Run    Pass   Fail   Pass%    Time
Isolated (unit)               718   718    717      1   99.86%   ~2m
IXMPTestApp                     1     1      1      0  100.00%   ~30s
MUXControls.Test (sample)     825    54     50      4   92.59%   ~19m
MUXControlsTestApp            601   543    517     26   95.21%   ~38m
External (component)        1,225 1,225  1,121    104   91.51%   ~3h
Managed (.NET, WPF host)      724   ...    ...    ...    ~79%   ~60-90m
TOTAL                       4,094 ...    ...    ...    ≥94%    ~4.5h
```

#### Failure analysis paragraph

State explicitly whether failures reproduce on unchanged main:

- **Reproduces on unchanged main** → pre-existing baseline drift, not introduced by this PR.
- **Does not reproduce on unchanged main** → real regression, must be investigated before merge.

The acceptance bar is **"PR introduces zero new failures."**

#### Gating note

When the change is gated by `Condition="'$(IsInternalWinUIBuild)' != 'true'"`, say so explicitly so reviewers know the internal CI build is unaffected and only the GitHub OSS build picks up the change.

#### ADO PR description size

ADO PR descriptions have a **4000-character hard limit**. If your validation report is larger:

- Truncate aggressively in the PR body.
- Keep the validation table.
- Drop per-test failure breakdowns; link instead.

#### Which repo to PR to

Per the **ADO ↔ GitHub Mirror Reference** above:
- File exists in ADO under any mirrored path → ADO PR.
- File exists only on GitHub → GitHub PR.
- When in doubt, ADO PR — the mirror will propagate it.
