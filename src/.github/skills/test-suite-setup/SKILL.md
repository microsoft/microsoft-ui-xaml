---
name: test-suite-setup
description: Complete end-to-end setup and execution of the WinUI3 test suite locally. Delegates building to the build skill, then handles test payload creation, machine setup, and running all tests.
---

# WinUI3 Full Test Suite Setup & Execution

This skill performs a complete end-to-end test run of the WinUI3 repository on the local machine.
It covers every step from environment initialization through running all ~8000 tests.

## AI Agent Instructions

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
