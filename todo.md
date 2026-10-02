# WinUI top-border TODO and machine handoff

Updated: 2026-10-02. This is the single repository handoff file. It includes the complete former `pr-feedback.md` below.

**Read this current handoff first.** The imported review history describes several older snapshots, superseded tests and earlier limitations. Its old line numbers and eight-test totals are not the current suite. The decisions below supersede older recommendations where they differ.

## Remaining work

- [ ] Transfer the local branch history, all six modified tracked files, and this untracked `todo.md` to the new machine. Fetching the remote PR branch alone is not enough.
- [ ] Mitigate concern 5: replace the display-compatible bitmap in `EraseWindowTopRows` with an explicit 32-bit RGB DIB.
- [ ] Build the transferred source, including the DIB change, using the repository build environment.
- [ ] Mitigate concern 1: run all six remaining top-border variations on a Windows 10 VM and a Windows 11 VM with freshly generated and deployed payloads.
- [ ] Verify each VM's capability diagnostic, six-case outcome, cleanup and binary provenance; record both results in this file.
- [ ] Review the final source/documentation diff and prepare the existing draft PR for Jesse's approval. Do not commit or push without his instruction.

### Jesse's latest decisions

| Previous concern | Decision and scope |
| --- | --- |
| 1: Platform coverage and DWM probe | Mitigate with the two-VM validation. The current six-case suite has not run on Windows 11. Prefer a newer Windows 10 build on the new machine if available, since RS5/build 17763 already has passing evidence. |
| 2: No composed-pixel regression test | Accepted. Keep `WindowTopBorderMatchesSideBorders` removed. Do not restore its backdrop, activation window, desktop screenshots or settling/pixel-comparison loop. Jesse does not want to test all the layers underneath WinUI. |
| 3: App-supplied DWM margins | Accept with documentation. Preserve the explanation of complete margin-set ownership, possible changes to all four border colors and inability to restore unknown prior app margins. |
| 4: No-redirection interaction | Jesse reports that he already tested this and is not concerned. Do not treat the older agent reports of missing combined-opt-in validation as a current blocker or add another integration workstream. |
| 5: Erase-test display color format | Mitigate with the explicit 32-bit DIB. Keep the synchronous in-memory erase checks; removing the desktop-pixel test does not mean removing these painting assertions. |

## Exact work to continue

### A. Make the erase test independent of display color depth

Location: `dxaml\test\native\external\controls\window\WindowIntegrationTests.cpp`, anonymous-namespace helper `EraseWindowTopRows`.

The helper currently creates a memory DC and a `CreateCompatibleBitmap(screenDC, 1, 2)`. It seeds both pixels magenta, synchronously sends `WM_ERASEBKGND` to the HWND with that memory DC, and reads rows 0 and 1 with `GetPixel`.

Replace only the bitmap-format dependency:

- Use an explicit 1-by-2, 32-bit `BI_RGB` DIB section. A top-down DIB makes row orientation explicit.
- Retain the two sentinel pixels, synchronous erase, return-value assertion and exact RGB assertions.
- Preserve the existing RAII and restore the previously selected bitmap before deleting the DIB.
- Verify allocation and selection failures using the existing test assertions. Do not hide errors or introduce success-shaped fallbacks.
- Keep the test about WinUI's RGB painting. Do not add alpha, DWM-margin or final desktop-composition assertions.
- Keep `Microsoft::UI::Colors`, not `Windows::UI::Colors`; the latter previously loaded Windows.UI.Xaml.dll and caused cleanup failures.

Acceptance: no display-compatible bitmap remains in this helper, the suite still has exactly six variations, and all six pass on both VMs. Do not add new libraries or general-purpose test infrastructure for this small change.

### B. Validate Windows 10 and Windows 11 expectations

The remaining painting tests probe `DwmGetWindowAttribute(... DWMWA_VISIBLE_FRAME_BORDER_THICKNESS ...)`, matching the product's capability approach, not a literal OS-version/build comparison.

| Environment | Expected capability result | Expected treatment |
| --- | --- | --- |
| Windows 10 | `E_INVALIDARG`; diagnostic `DWM top-border workaround required: 1` | With the fix enabled, a restored Window-entry-point window with a native frame gets the special top-row paint. Opt-out, fullscreen/borderless and closing cases use normal erase. |
| Windows 11 | Successful getter; diagnostic `DWM top-border workaround required: 0` | Normal background erase. The tests should execute and pass, not skip the OS solely because the workaround is unnecessary. |

The diagnostic describes DWM capability independently of whether a particular test has disabled the optional change. A Windows 10 opt-out variation can report capability value 1 and still correctly use normal erase.

If a modern-OS expectation fails, investigate the real behavior and compare with the existing behavior. Do not change geometry, skip Windows 11 or loosen assertions merely to obtain a pass.

## Repository and transfer snapshot

| Item | State at handoff |
| --- | --- |
| Repository | `microsoft/microsoft-ui-xaml`; old checkout `D:\x2` |
| Workspace | User-owned in-place checkout, not an isolated worktree |
| Local branch | `user/jessecol/top-border-bug` |
| Local HEAD | `e85b0b8456109be5996633c44c82870cf72ef5cf` |
| Rebased main | `7b68d3e0b771a57d80098799406234efee479517` |
| Existing PR | Draft [#11698](https://github.com/microsoft/microsoft-ui-xaml/pull/11698), targeting `main` |
| Remote PR head | `8ce077c2e37752402da3f76fd2ae77a2cc14aa25` |
| Local/remote relationship | Ahead 23 / behind 9 at handoff, reflecting the rebase and local changes; nothing pushed |
| Index | No staged changes at handoff |
| Handoff notes | `todo.md` is intentionally untracked, replacing the intentionally untracked `pr-feedback.md` |
| Unrelated file | `old-pr-description.md` remains untracked and must be preserved, not folded in or deleted |

The topic contains ten local commits above the rebased main. Some older commit titles mention fullscreen geometry; the current implementation deliberately preserves existing geometry and does not fix the fullscreen gap.

These six tracked files have additional uncommitted changes and must travel with the branch:

| File | Pending changes |
| --- | --- |
| `docs\design-notes\customtitlebar.md` | Restored original content/order; topic diff against the recorded main is entirely additive, 59 added lines and zero deleted lines. Added the fullscreen issue link and current behavior notes. |
| `dxaml\test\native\external\controls\window\WindowIntegrationTests.cpp` | Removed the composed-pixel test; retained erase/transition/close checks and added capability-probe explanation/diagnostic. The 32-bit DIB mitigation is still pending. |
| `dxaml\test\native\external\controls\window\WindowIntegrationTests.h` | Removed composed-test registration and clarified normal-background-erase wording. |
| `dxaml\xcp\components\WindowChrome\CWindowChrome.cpp` | Clarified existing spacing and added the workaround-precondition assertion inside `ShouldPaintTopRowOfClientArea`. |
| `dxaml\xcp\components\WindowChrome\WindowHelpers.cpp` | Added the enabled-change plus Windows 10 DWM-probe header comment. No gating behavior changed. |
| `dxaml\xcp\dxaml\lib\DesktopWindowImpl.cpp` | Renamed `borderRect` to `topRowRect` and replaced misleading "legacy" offset wording. |

**A normal clone of the remote branch will miss both the rebased local history and these working-tree changes.** Transfer the checkout or use a Git bundle plus a binary working-tree patch and this untracked file. A patch alone does not contain the ten topic commits; a bundle alone does not contain dirty files or `todo.md`. Verify branch, HEAD and `git status` on the destination before continuing. Do not recover the work from the older `user/jessecol/top-border-bug-minimal` branch.

Do not rely on copied `BuildOutput`, `packages` or `TestPayload` as proof of a current build. Generate fresh outputs on the new machine.

## Implementation map and invariants

| Surface | Current behavior to preserve |
| --- | --- |
| `WindowHelpers::ShouldApplyDwmTopBorderWorkaround` | Requires `XamlChangeId::FixWindowTopBorder` enabled and a cached DWM probe result of `E_INVALIDARG` for `DWMWA_VISIBLE_FRAME_BORDER_THICKNESS`. Success or another failure returns false. The header explicitly explains the intended Windows 10 selection. |
| `CWindowChrome::ShouldPaintTopRowOfClientArea` | Boolean native-treatment predicate. Starts with `ASSERT(WindowHelpers::ShouldApplyDwmTopBorderWorkaround(m_topLevelWindow));`; both current call paths already require that condition. |
| Native eligibility | Requires existing top spacing, `WS_BORDER` or `WS_THICKFRAME`, an attached DesktopWindow and eligible presenter. Excludes FullScreen and OverlappedPresenter with `HasBorder` false. A resize style alone is not enough. |
| Closing/query failure | Detachment is checked before querying AppWindow. Failed queries log and return false; callers use normal erase and request clearing owned margins. Null presenter logs `E_UNEXPECTED`. Keep the nonfatal accessor overload; other existing AppWindow callers retain their original accessor. |
| `GetTopBorderHeight` / `IsTitlebarVisible` | Keep these names. They describe cached Window chrome state and existing spacing, not actual native border measurements or visual title-bar visibility. |
| `TryEraseBackgroundForWindowTopBorder` | Paints `topRowRect`, then the ordinary background below. Standard themes use black GDI pixels to expose the extended frame; High Contrast uses `COLOR_WINDOWFRAME`. `WS_EX_NOREDIRECTIONBITMAP` skips special GDI painting. |
| `UpdateDwmFrameMargins` | Extends the caption/resize-frame height computed by `AdjustWindowRectExForDpi`, not only one pixel. Tracks owned/last-successful margins; style changes and settled presenter changes refresh them. DWM failures retain the last successful cached value. |
| Optional change | `FixWindowTopBorder = 8948`, bit index 5. Disabled by default in production; initialize before XAML startup. Tests must close created windows before test-only flag resets. |

The current `Switcher` is capped at UniversalApiContract 10 and cannot distinguish Windows 10 from Windows 11 as implemented. `IsOSBuildAtLeast` exists in native test infrastructure, not product code. The investigation recommended keeping the DWM probe for now. Build 22000 support for the attribute is documented; the specific unsupported-attribute `E_INVALIDARG` convention is observed rather than documented. Do not repeat an unverified claim that UniversalApiContract versions necessarily alias across the Windows 10/11 boundary.

The HWND style mask is a conservative eligibility/transition guard, not a universal guarantee about DWM rendering. Forced `DWMNCRP_ENABLED` was not tested. Jesse chose to retain the mask.

## Scope and user preferences

- Do not restore the removed desktop-pixel/composed-border test, or add an equivalent under a new name.
- Do not fix the existing fullscreen layout gap in this change. It is tracked separately in [#12138](https://github.com/microsoft/microsoft-ui-xaml/issues/12138). The retained tests intentionally preserve the existing offset through fullscreen/borderless transitions.
- Preserve the distinction between `Window.ExtendsContentIntoTitleBar` and direct `AppWindow.TitleBar.ExtendsContentIntoTitleBar`; the AppWindow entry point does not activate the same cached Window chrome state.
- Keep `GetTopBorderHeight`, `IsTitlebarVisible`, `ShouldPaintTopRowOfClientArea`, `RefreshContainerSizeAndPosition` and `topRowRect`. Do not rename helpers as part of the remaining mitigations.
- Keep "one-pixel", "existing" and "normal" for current geometry/erase behavior, not "legacy". Historical review quotes below are preserved records, not current code wording.
- Jesse rejected goto-style `IFC` and a `succeeded` lambda for the new predicate. Preserve the explicit nonfatal WIL error handling; do not add custom tracing without a concrete need.
- Keep `customtitlebar.md` changes additive to the original main text. Restored original lines contain pre-existing trailing spaces; do not rewrite that old text just to make a whole-HEAD whitespace check clean.
- No speculative opacity change for the unproven Dark startup/live-resize hypothesis. No demonstrated product defect was established from that hypothesis.
- Do not commit, push, reset, stash, rebase or create branches automatically. Jesse has not authorized publishing the current local changes.
- Use plain ASCII and clear, direct wording. Do not include private research, internal-only links or credentials in public source/docs.
- If asked to update the PR, inspect and preserve the repository PR template. Every PR body/comment starts with `*This content was largely generated by AI.  AI makes mistakes.*` and a newline.
- If Jesse explicitly asks for a commit, include `Co-authored-by: Copilot App <223556219+Copilot@users.noreply.github.com>` unless he asks otherwise. Do not stage this handoff document automatically.

## Build and VM procedure on the new machine

Load the repository `build` and `test-on-vm` skills first. Their files are `.github\skills\build\SKILL.md` and `.github\skills\test-on-vm\SKILL.md`. Jesse asked for subagent build/test validation; keep source edits and validation ownership coordinated so nobody changes compiled inputs during a build.

The default flavor is `amd64chk`; the VM payload flavor is `x64chk`. Use x64 VMs for that build. Run from the new checkout root, not the old `D:\x2` path if it differs.

On a newly initialized machine, build the full repository after transferring the source and applying the DIB change:

```powershell
.\initrun.ps1 .\build.cmd /q
```

Use a tool wait of at least 300 seconds. If `initrun.ps1` explicitly reports that a full init is required, follow the build skill for `amd64chk`; do not install/restore dependencies preemptively on an already initialized host.

For a later source-only Controls test change with current outputs, the targeted command is:

```powershell
.\initrun.ps1 msb /q "dxaml\test\native\external\controls\Microsoft.UI.Xaml.Tests.External.Controls.vcxproj"
```

The corresponding runtime command, if runtime source changes, is:

```powershell
.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"
```

Use the repository `msb /q` wrapper, not raw `msbuild /q` (which has different semantics). Do not start another full build merely to revalidate an already-current, single-project source edit.

### VM prerequisites

Record the two actual VM names and guest OS builds. Do not assume the old host's `rs5` name exists on the new host.

- Both VMs must be running, logged in, unlocked and have an interactive Explorer desktop. Check for a lock-screen `LogonUI` process.
- Confirm Hyper-V access. If permissions are missing, stop and have Jesse follow the manual permission procedure in the skill. Never grant membership or open an elevated shell on his behalf.
- Credentials are encrypted per host/user under `$env:USERPROFILE\.winui-test`. Do not copy the old credential XML as a portable credential or print its contents. Use the script's credential prompt/cache on the new host.
- Run the tests on the VMs, not on the host. Keep bounded commands attached and preserve complete output.

Run sequentially, replacing the placeholders with actual VM names. This avoids simultaneous local payload generation or manifest changes:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <Win10VM> '*WindowTopBorder*' -HostingMode WPF -SkipPackageUninstall
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <Win11VM> '*WindowTopBorder*' -HostingMode WPF -SkipPackageUninstall
```

Use a tool wait of at least 180 seconds. **Do not use `-SkipPayload`.** Each run must refresh the local payload from the current outputs and deploy it. `-FullCopy` is available if incremental deployment is suspect.

### Required outcomes on each VM

Exactly these three methods, with two variations each, must execute:

| Method | Variations |
| --- | --- |
| `WindowTopBorderPainting` | `ECITBBeforeActivation=false/true` |
| `WindowTopBorderPaintingPreservesCompatBehavior` | `ECITBBeforeActivation=false/true`, fix disabled |
| `WindowTopBorderEraseDuringClose` | `FixWindowTopBorder=true/false` |

Require `Total=6, Passed=6, Failed=0, Blocked=0, Not Run=0, Skipped=0`, not just process exit 0. Eight variations mean stale test registration/binaries. Zero tests, skipped modern-OS bodies or cleanup failures are not success.

Inspect shutdown, host reset, RPC stop and manifest restoration; reject dirty-pool/cleanup failures. Existing infrastructure skips leak detection, so do not claim leak validation.

Compare fresh SHA256 values across these build outputs, the local `TestPayload\x64chk\Test` files, and each VM's deployed `Test` files:

| Build output | Payload filename |
| --- | --- |
| `BuildOutput\bin\amd64chk\Product\Microsoft.ui.xaml.dll` | `Microsoft.ui.xaml.dll` |
| `BuildOutput\bin\amd64chk\Product\Microsoft.UI.Xaml.Controls.dll` | `Microsoft.UI.Xaml.Controls.dll` |
| `BuildOutput\bin\amd64chk\Test\Microsoft.UI.Xaml.Tests.External.Controls.dll` | `Microsoft.UI.Xaml.Tests.External.Controls.dll` |

Discover the new remote payload root from the script banner. The old host used `C:\TestPayload\JESSE3-x2`; it is not a portable constant. `BuildOutput` and `TestPayload` are gitignored, so normal file-search tools can miss them; use exact-path `Get-FileHash` for provenance.

Capture complete logs outside the repository and summarize results in this file. An external PowerShell child piped through `Tee-Object` captured inherited console output reliably; `Start-Transcript` alone previously omitted it.

### Known build traps

- A prior full build resolved an AMD64 managed projection from an AnyCPU consumer because generated projection output/cache paths are shared and the incremental hash omitted `PlatformTarget`. It was recovered locally without tracked infrastructure changes. Do not suppress the warning, change the project's intended architecture or assume full init fixes the cache collision.
- The first post-rebase full build failed late because `Microsoft.Windows.CppWinRT.2.0.250303.1` was missing for XamlCompiler regression projects. Restoring `src\XamlCompiler\Tests\packages.config` with the repository `NuGet.config` fixed that concrete failure. The full retry passed in 31 minutes 16 seconds.
- The native Controls build has an existing `MSB8019` warning for `Private.Infrastructure.Server.lib` packaging. Do not turn this handoff into an unrelated packaging fix.
- Package/mock outputs can become stale. Refresh payloads from current binaries rather than relying on timestamps, old DLLs or a successful previous test count.

## Latest verified results before the remaining mitigations

The latest runtime build, including the new predicate assertion and OS-gate header comment, passed in 26.66 seconds with zero warnings/errors:

```powershell
.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"
```

The native Controls test project had passed after the test removal and wording cleanup in 40.34 seconds, with one existing `MSB8019` warning and zero errors. It did not need rebuilding for the later runtime-only assertion.

The final fresh-payload RS5 run on Windows 10 build 17763 passed all six variations, with no failed/blocked/skipped cases and no cleanup failures. This validates the current assertion but **does not validate the still-pending DIB change or the Windows 11 matrix**.

| DLL | Last verified old-host SHA256 |
| --- | --- |
| Runtime | `575EF889D6F7FA4D9ADFCF01470A1CFDAFD632121BB513EED97CC611E1ACDFC8` |
| Controls | `E298F56EABE5E43D4AC60BAFC1CA4668C96A81EA3A2669FCBBB0380A56622561` |
| Native Controls tests | `929E327AEA2E5C5D35299CFD459963687CDFCAA42AE44243FDD18123C6BAD1F8` |

These hashes are historical provenance, not expected values for a fresh build on a different machine. Compute new hashes and compare build/local/remote values to each other.

### Results to fill in on the new machine

| Item | Windows 10 | Windows 11 |
| --- | --- | --- |
| VM name and guest build | Pending | Pending |
| DIB change present in rebuilt test assembly | Pending | Pending |
| DWM capability diagnostic | Expect 1; pending | Expect 0; pending |
| Six-case outcome and cleanup | Pending | Pending |
| Fresh build/local/remote DLL hash match | Pending | Pending |
| Complete log location | Pending | Pending |

## Optional old-host artifacts

This handoff is self-contained for continuing the work. If preserving detailed old logs/images, copy selected artifacts separately from `$env:USERPROFILE\.copilot\session-state\9014df20-7017-4b8f-8453-ff0095fa837a\files`; they are not part of the Git checkout.

- `simplified-top-border-assertion-vm-20261002.log`: complete final six-case assertion-validation output.
- `simplified-top-border-agent-final-vm-20261002.log`: six-case output before the later assertion.
- `rebase-main-full-build-retry-20261002.log`: successful full post-rebase build.
- `borderless-visual\window-pixel-comparison.csv`: 15-case stock/fix-off/fix-on comparison data.
- `borderless-visual\captures\fully-frameless\stock`, `current-disabled` and `current-enabled`: retained desktop PNG/BMP captures and observations for bordered, borderless-resizable, borderless-fixed, native-frameless and fullscreen cases.

Those desktop captures are historical/manual evidence only. They are not a request to restore an automated composed-pixel test.

---

The complete imported feedback follows. Its historical recommendations, coverage gaps and wording are preserved for context. Jesse's latest decisions and the active checklist above control the next work.

# Collected branch review feedback

Originally reviewed commit: `2f0299ad5783634de56dd0ff308e2225b1d28fac`.

Review base: `50e8f5013136eaf64c181663d975bcd635c113ff`.

All four independent reviews have returned: the original three deep multi-modal passes and the additional user-requested grumpy tester. Repeated findings are consolidated without hiding disagreements or turning static predictions into observed failures.

## Individual feedback

- Pass 1: Native correctness and lifecycle.
- Pass 2: Visual rendering and compatibility.
- Pass 3: Tests, API wiring and documentation.
- Pass 4: Grumpy adversarial tester.

The individual reports, review scope and screenshot provenance are preserved separately in the review session. This document collects their substantive findings.

The original three reports preserve all substantive feedback from each agent's initial review and screenshot follow-up, with duplicate explanations combined and ASCII punctuation. The fourth report preserves both complete returned verdicts and the limitation that no rationale was supplied. Confidence and evidence limitations are retained.

## Review outcomes

| Pass | Outcome |
| --- | --- |
| 1: Native correctness | One P2 product finding, one P2 coverage gap, and a conditional P2 integration risk; static behavior qualified by screenshot observations. |
| 2: Visual compatibility | One P2 potential product finding supported by a synthetic pixel-alpha probe, not a reproduced composed artifact; no combined-opt-in visual failure established. |
| 3: Tests and API | Same fullscreen/borderless P2 as pass 1, additional coverage/isolation concerns, successful enum/bit consistency checks, and a qualified integration test requirement. |
| 4: Grumpy tester | "No significant issues found in the reviewed changes." The agent repeated this verdict when asked for its rationale and supplied no supporting breakdown. |

The grumpy verdict does not resolve the earlier findings. There is no unanimous clean bill of health and no confirmed runtime reproduction of the predicted visual failures.

## Deduplicated findings from the original three passes

| ID | Classification | Reported by | Location | Finding and evidence boundary |
| --- | --- | --- | --- | --- |
| F1 | P2 product finding | Passes 1 and 3 | `dxaml\xcp\dxaml\lib\DesktopWindowImpl.cpp:1252-1257`; `dxaml\xcp\components\WindowChrome\CWindowChrome.cpp:202-220` | New painting/margin paths use the legacy one-pixel XAML offset as native-border eligibility. Fullscreen/borderless windows can retain that offset, so the opt-in can change their row treatment despite the documented compatibility promise. Control flow is high confidence; final composed fullscreen output was not reproduced. |
| F2 | P2 potential product finding | Pass 2 | `dxaml\xcp\dxaml\lib\DesktopWindowImpl.cpp:1281-1284` | Dark-theme background erasure below the reserved row may leave zero-alpha black pixels within the enlarged DWM margin, exposing the native caption during startup/live resize. A synthetic DIB probe establishes the pixel-alpha mechanism, not an actual HWND/DWM-composed artifact. |
| C1 | Regression coverage gap | Passes 1, 2 and 3 | `dxaml\test\native\external\controls\window\WindowIntegrationTests.cpp:372-458`, especially `417-425` | Tests sample RGB in a memory DC and bridge geometry. They cannot detect a missing or wrong margin extension, its cleanup, alpha errors or incorrect final composed border colors. |
| C2 | Transition/matrix coverage gap | Pass 3 | `WindowIntegrationTests.cpp:379-455`; `DesktopWindowImpl.cpp:1255-1263`; `CWindowChrome.cpp:223-244` | No targeted coverage for preactivation `ExtendsContentIntoTitleBar`, fullscreen/borderless transitions, DPI changes, deterministic/live High Contrast, no-redirection behavior, failure/retry, or equal-margin/idempotence/cleanup behavior. |
| C3 | Test-isolation concern | Pass 3 | `controls\test\MUXControlsTestApp\Utilities\APITestBase.cs:85-86`; `dxaml\test\infra\client\lib\WindowHelper.cpp:2201-2209` | Clearing process optional-change bits does not undo margins on surviving HWNDs. New native tests close their windows, so no demonstrated isolation failure or production opt-out defect is claimed. |
| I1 | Conditional integration risk | Passes 1, 2 and 3, with differing assessments | `DesktopWindowImpl.cpp:1260-1263`; `CWindowChrome.cpp:282-285`; open [#11587](https://github.com/microsoft/microsoft-ui-xaml/pull/11587) | Combined opt-ins can remove the GDI surface and bypass the High Contrast paint path. Pass 1 flags a compatibility-policy requirement. Passes 2 and 3 do not establish a visual incompatibility; pass 3 notes that removing opaque erasure might itself expose the frame correctly. Treat this as an integration test requirement, not a confirmed standalone branch defect. |

## Recommended narrow responses

- F1: Separate native-frame eligibility from legacy geometry, skip new treatment and clear owned margins in fullscreen/borderless states, and preserve existing XAML offset behavior.
- F2: Reproduce startup/live resize on Windows 10 in Dark theme. If the actual surface exhibits the predicted exposure, make the normal background explicitly opaque while leaving only the reserved border row transparent.
- C1: Add composed-output checks for active/inactive top-versus-side border matching. Ensure removing the margin call or reducing its extent to one pixel causes the regression to fail.
- C2: Exercise the listed transitions and failure paths with the opt-in enabled and disabled.
- C3: Verify live-window reset isolation or make teardown requirements explicit.
- I1: Test both opt-ins together in normal and contrast themes before convergence. Do not select a compatibility policy on the assumption of a reproduced failure.

## Evidence that supports the intended fix

All three agents inspected author-provided screenshots from related [PR #11698](https://github.com/microsoft/microsoft-ui-xaml/pull/11698). The restored-window after comparisons show matching top and side colors in active and inactive examples without a larger XAML offset. The High Contrast after examples show an added white row over the main XAML span. Side-border color also changes in the active case, consistent with the disclosed opt-in compatibility effect.

The caption-button gap is explicitly out of scope and was not reported as a new defect. Zero-margin and one-pixel-margin crops support the larger-margin rationale, but do not prove a minimum sufficient value.

These screenshots are not proof that the exact reviewed tip was executed. They do not validate fullscreen, startup/live resize, DPI changes, opt-out or combined optional changes.

## API and state checks

Pass 3 found no enum/model/IDL mismatch. Existing numeric values are stable, `FixWindowTopBorder = 8948` agrees across surfaces, bit index 5 is unique/in range, and production initialization remains disabled. The legacy executable `GetTopBorderHeight()` body is unchanged.

## Related work

The original fix's open [PR #11698](https://github.com/microsoft/microsoft-ui-xaml/pull/11698) overlaps all 18 files. Open [#11587](https://github.com/microsoft/microsoft-ui-xaml/pull/11587) overlaps eight files and has the rendering integration boundary above. Related open [#11804](https://github.com/microsoft/microsoft-ui-xaml/pull/11804) and [#11967](https://github.com/microsoft/microsoft-ui-xaml/pull/11967) share no files. [#11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) and [#11950](https://github.com/microsoft/microsoft-ui-xaml/pull/11950) are merged, not competing open work.

## Original review limitations

The reviews are read-only. No product fix was applied. Final Windows 10 rendering at the exact tip, presenter transitions and combined opt-ins were not reproduced. Review findings must retain these distinctions rather than promoting predicted visual consequences to observed failures.

The fourth reviewer was explicitly instructed to perform a full adversarial review and inspect available visual evidence, but returned no account of that work. Its clean verdict is weaker evidence than a documented examination of a specific failure scenario.

## Implementation and review loop

The three requested developer/reviewer cycles completed in the working tree without commits or branch changes. The original findings above remain a record of the earlier snapshot; line numbers can shift as fixes are added.

| Cycle | Developer work | Independent review and corrections |
| --- | --- | --- |
| 1 | Separated native-frame eligibility from legacy XAML geometry. Excluded fullscreen/borderless presentation from the new painting/margin treatment, retained border-without-titlebar behavior, refreshed margins on style changes and settled presenter notifications, and added transition/composed-output coverage. | Found a P2 close-time null dereference: the new query accessed AppWindow after DesktopWindow detachment during a closing visibility callback. A developer added a shared detachment guard and synchronous closing erase/margin regression coverage. Runtime and Controls builds passed. |
| 2 | Retained the native and close-time fixes. Strengthened composed-border tests with Light/Dark variations, DPI-aware caption insets, multiple top/side samples, immediate-next-row content assertions, and foreground/occlusion/state checks. Controls build passed. | No significant issues found. The reviewer explicitly retained Windows 10 execution, margin negative-control, F2, cleanup and combined-opt-in limitations. No speculative correction was applied. |
| 3 | Independently checked the current fixes, lifecycle, gating, metadata, cleanup and pixel arithmetic. No further source change was warranted. | No significant issues found. No additional correction was required. |

### Current dispositions

- F1: Implemented, with default/opt-out, Windows 11 and legacy bridge geometry preserved. The final Windows 10 VM run passed the targeted native transition tests.
- F2: Still an unproven Dark startup/live-resize hypothesis. No speculative opacity patch was added.
- C1: Added and strengthened real composed-output coverage. Both Light/Dark variations passed. Removing the margin or reducing it to one physical pixel made both variations fail as expected; the correct production source was restored before final validation.
- C2: Added and executed preactivation, entry-point, repeated `ExtendsContentIntoTitleBar` changes, maximize/restore, fullscreen, borderless and bordered-without-titlebar coverage. Actual DPI transitions, live High Contrast, DWM failures/retry and direct margin observation remain limited.
- C3: Documented that test-created custom-titlebar windows must close before test-only optional-change resets. No production live-reset policy was introduced.
- I1: Remains a conditional integration requirement for #11587. No code from that PR was merged.
- Cycle-1 close-time P2: Corrected in the shared native eligibility query before AppWindow access. Both erase and margin-update callers use the guard. Both opt-in/opt-out close-time regression variations passed on the Windows 10 VM.

During the three review cycles, the regression inventory contained eight WPF variations across `WindowTopBorderPainting`, `WindowTopBorderPaintingPreservesCompatBehavior`, `WindowTopBorderMatchesSideBorders` and `WindowTopBorderEraseDuringClose`.

Previous test attempts blocked before their bodies executed and are not passes. A narrow existing-app Windows 11 startup/ordinary-window geometry smoke check succeeded with the current runtime, but it does not validate the Windows 10 opt-in or F2.

The user confirmed the rs5 VM desktop is unlocked and active. Final build and fresh-payload VM validation completed as recorded below.

## Full-build issue investigation

The first full build failed with MSB3270 because the AnyCPU XamlHost project resolved an AMD64 private WinUI projection assembly. The solution correctly selected AnyCPU for both managed projects; the mismatch came from stale incremental output.

Direct x64 and solution AnyCPU projection builds share output and intermediate/cache paths. The managed incremental compile hash omits `PlatformTarget`, so the full solution can skip compilation and reuse a prior AMD64 assembly. Binlog replay, evaluated properties, PE architecture inspection and a controlled hash comparison verified this mechanism.

Regenerating the intended AnyCPU output fixed the failure. No tracked build configuration or source change was needed, no architecture warning was suppressed, and no full init was run. Full init is not a reliable fix for this output/cache collision. This is a local generated-output recovery, not a permanent prevention fix; mixing future direct-project and solution contexts can recreate the issue.

## Actual VM failures and corrections

After the desktop was unlocked, the first fresh-payload run executed all eight variations: two passed and six failed. The close-time regressions passed. The other failures exposed test defects that had been hidden by the earlier setup blockers:

| Observed failure | Correction |
| --- | --- |
| Fullscreen and borderless checks with `ExtendsContentIntoTitleBar` disabled expected bridge offset zero, but both opt-in and opt-out retained the pre-existing one-pixel offset. | Corrected only those frameless test expectations. Production geometry remained unchanged, and native-border/erase checks remain separate from bridge positioning. |
| Top/side RGB comparisons sampled activation fades, sometimes across different composed frames or backdrops. | Added a controlled test-owned backdrop, atomic BitBlt frame snapshots, and bounded consecutive-frame settling. Retained exact RGB equality, multiple top/side samples, next-row content assertions and both activation/theme states. No broad tolerance or affected-platform skip was added. |
| Composed-test cleanup detected Windows.UI.Xaml.dll loading. | Traced the cause to `Windows::UI::Colors`, not native screenshot capture. Switched the test to `Microsoft::UI::Colors` and retained screenshot logging. Final cleanup was clean. |

These validation-follow-through corrections changed tests and related documentation only. Temporary production mutations used for negative controls were restored exactly before final builds.

## Final verification

Final full build command:

```powershell
.\initrun.ps1 .\build.cmd /q
```

**Passed**, exit 0, elapsed `00:06:42.07`. Product, tests, mock package and MUXControls stages completed. Existing packaging warnings remain; the build is not claimed to be warning-free. This full build preceded the final error-logging simplification.

The final native-runtime build after that simplification:

```powershell
.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"
```

**Passed**, exit 0, elapsed `00:00:38.66`, with zero warnings and errors.

Final Windows 10 build 17763 VM command:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName rs5 '*WindowTopBorder*' -HostingMode WPF -SkipPackageUninstall
```

**Passed**, exit 0, with a newly refreshed and deployed payload:

```text
Summary: Total=8, Passed=8, Failed=0, Blocked=0, Not Run=0, Skipped=0
```

All individual variations passed, including both Light/Dark composed checks and both close-time optional-change variations. The final run contained no error lines or dirty-pool/cleanup failures. Product and Controls DLL SHA256 values matched across final build outputs, local payload and remote deployed payload.

Both composed-output negative controls failed as intended:

- Missing DWM margin: Light/Dark active top row stayed black while side borders were colored; two failures.
- One-physical-pixel margin: active checks passed, but Light/Dark inactive top row was white instead of matching the gray side borders; two failures.

No temporary mutation, owned test process or scheduled test task remained after validation. At that point all review-follow-through changes were uncommitted, and branch/HEAD and the unrelated user description were preserved.

### Manual borderless visual check

Used a standalone ReproStudio probe on Windows 10 build 17763, with real desktop captures rather than XAML-only screenshots. Captured a stock baseline and the current native runtime with `FixWindowTopBorder` disabled and enabled. Verified the loaded native DLL hash and logged HWND styles, DWM non-client rendering state, foreground state and client coordinates.

All cases used `Window.ExtendsContentIntoTitleBar = true`. Comparing the entire window area with the fix disabled and enabled gave:

| Window state | `WS_BORDER` / `WS_THICKFRAME` | DWM non-client rendering | Different window pixels |
| --- | --- | --- | --- |
| Bordered control | Both set | Enabled | 464, confined to the top client row |
| Borderless, resizable | `WS_THICKFRAME` set | Enabled | 0 |
| Borderless, fixed size | Neither set | Disabled | 0 |
| All three native frame styles explicitly cleared | Neither set | Disabled | 0 |
| Fullscreen | Neither set | Disabled | 0 |

The fixed-size borderless presenter retained `WS_DLGFRAME` and a three-pixel client offset. The explicitly frameless case cleared `WS_BORDER`, `WS_DLGFRAME` and `WS_THICKFRAME`, removing that offset. Thus, absence of the two bits checked by the predicate does not mean every native frame style is absent.

Visual inspection confirmed that the existing white top client row remained in the frameless/fullscreen cases with the fix both disabled and enabled. Only the bordered control changed, from white to the native frame color.

These results cover the tested presenter states and configured DWM policy on this VM. They do not establish a universal style-bit rule: forced `DWMNCRP_ENABLED` was not tested. No production or integration-test source was changed for this probe, and no permanent bitmap test was added.

### Final local review

Reviewed the pending product, test and documentation changes before the local PR-branch handoff. No new blocking findings were identified. The current helper is `bool ShouldPaintTopRowOfClientArea()`: AppWindow/presenter query failures log the HRESULT and return false, without a fail-fast lookup. Legacy geometry and existing helper names remain unchanged.

The composed-output test remains dependent on foreground state, occlusion, physical sample positions and exact RGB equality. Controlled capture and bounded settling reduce those risks, but do not make it independent of the desktop environment. No test was removed or disabled during this review.

Rechecked related open work. [#11587](https://github.com/microsoft/microsoft-ui-xaml/pull/11587) still overlaps the background-erase path; its combined-opt-in validation remains a limitation, and none of its code was brought into this branch. [#11804](https://github.com/microsoft/microsoft-ui-xaml/pull/11804) has no shared files. The existing fullscreen layout gap belongs to [#12138](https://github.com/microsoft/microsoft-ui-xaml/issues/12138), not this border-color fix.

### Remaining limitations

F2 remains unproven: settled Dark pixels and activation-fade screenshots do not reproduce an exposed native caption during Dark startup/live resize. No speculative opacity patch was applied.

Combined no-redirection opt-ins, actual DPI transitions, live High Contrast, fault-injected DWM retry/cleanup and direct margin-value observation remain outside these runs. The managed build-output collision was recovered locally, not permanently prevented in tracked infrastructure.

## Current suite simplification

At Jesse's request, `WindowTopBorderMatchesSideBorders` and its test metadata were removed. This also removes the test-owned backdrop, activation window, desktop capture, pixel-settling loop and exact composed-border comparisons.

The suite now contains six WPF variations across `WindowTopBorderPainting`, `WindowTopBorderPaintingPreservesCompatBehavior` and `WindowTopBorderEraseDuringClose`. These retain synchronous `WM_ERASEBKGND` checks in an in-memory bitmap, title-bar entry-point and presenter transitions, and close-time background erasure. The production rectangle was renamed from `borderRect` to `topRowRect` without changing its behavior.

The remaining painting tests probe `DWMWA_VISIBLE_FRAME_BORDER_THICKNESS`, like the product. `E_INVALIDARG` selects the older-DWM workaround expectations; a successful query selects normal background erasure. They do not require an RS5 build number or skip Windows 11 solely because it has modern DWM.

The in-memory checks verify RGB painting, not transparency or final DWM composition. They do not replace the removed test's coverage of missing or insufficient frame margins. The eight-case passes and margin negative controls above remain historical evidence, not validation of the simplified suite.

The wording sweep replaced "legacy spacing" with "existing spacing", "legacy XAML offset" with "one-pixel XAML offset", and "legacy background erase" with "normal background erase". These were comment/description changes only.

A separate build/test agent validated the final wording-cleaned sources:

| Command | Result |
| --- | --- |
| `.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"` | Passed in 37.92 seconds, zero warnings and errors. |
| `.\initrun.ps1 msb /q "dxaml\test\native\external\controls\Microsoft.UI.Xaml.Tests.External.Controls.vcxproj"` | Passed in 40.34 seconds, one existing `MSB8019` packaging warning and zero errors. |
| `.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName rs5 '*WindowTopBorder*' -HostingMode WPF -SkipPackageUninstall` | Passed after refreshing and deploying the payload: Total=6, Passed=6, Failed=0, Blocked=0, Not Run=0, Skipped=0. |

Runtime, Controls and native Controls test DLL SHA256 values matched across the build outputs, local payload and RS5 deployment. Host reset, shutdown and RPC cleanup completed without dirty-pool or cleanup failures. Existing leak detection remained skipped.

Windows 11 execution has not been performed in this investigation.

## DWM platform-gating investigation

The read-only investigation found no existing product helper that directly identifies Windows 10 rather than Windows 11. The current `Switcher::OSVersion` stops at UniversalApiContract 10 and cannot distinguish them as implemented. A build-number helper, `IsOSBuildAtLeast`, exists in native test infrastructure, not product code.

The recommendation is to retain the current DWM capability probe for now. Microsoft documents `DWMWA_VISIBLE_FRAME_BORDER_THICKNESS` as supported starting with Windows 11 build 22000. However, the `DwmGetWindowAttribute` documentation does not promise that an unsupported attribute specifically returns `E_INVALIDARG`; that convention was observed on RS5. A direct build-number check would avoid that HRESULT assumption but require new product-level version detection. Neither approach proves that attribute support alone determines the visual defect.

Sources: [DWMWINDOWATTRIBUTE](https://learn.microsoft.com/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute) and [DwmGetWindowAttribute](https://learn.microsoft.com/windows/win32/api/dwmapi/nf-dwmapi-dwmgetwindowattribute). No OS-gating behavior changed. At Jesse's request, the function header now states that the optional change must be enabled and the DWM probe must indicate Windows 10; success or other probe failures return false.

## Top five remaining PR concerns

These are remaining validation and compatibility risks, not five demonstrated defects.

| Rank | Concern | Assessment |
| --- | --- | --- |
| 1 | Platform coverage and the DWM probe | Only RS5 has been executed. Windows 11's normal-erase branch and a newer Windows 10 build remain unverified; the unsupported-attribute HRESULT convention is not a documented API guarantee. |
| 2 | Final composed appearance is no longer regression-tested | Earlier composed runs and negative controls provide evidence, but the six retained memory-bitmap tests cannot detect wrong DWM margins or alpha/composition regressions. This is the accepted tradeoff from removing the desktop-pixel test. |
| 3 | Ownership of app-supplied DWM margins | The opt-in can change all four border colors and takes ownership of the complete margin set. Earlier app margins cannot be restored automatically. This compatibility change is documented and remains opt-in. |
| 4 | No-redirection interaction | Open [#11587](https://github.com/microsoft/microsoft-ui-xaml/pull/11587) changes the same background-erase path. Combined behavior has not been validated; special GDI painting is skipped for `WS_EX_NOREDIRECTIONBITMAP`. |
| 5 | The erase helper still depends on display color format | It uses `CreateCompatibleBitmap`, not an explicit 32-bit DIB. A different display format can quantize custom High Contrast colors and make exact RGB assertions fail even when painting is correct. |

## Eligibility assertion validation

Added `ASSERT(WindowHelpers::ShouldApplyDwmTopBorderWorkaround(m_topLevelWindow));` at the start of `ShouldPaintTopRowOfClientArea`. Both current call paths already require that condition.

The validation subagent reran `.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"`: passed in 26.66 seconds with zero warnings or errors. The Controls test sources had not changed since their successful build and were not rebuilt.

It then reran `.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName rs5 '*WindowTopBorder*' -HostingMode WPF -SkipPackageUninstall` with a refreshed payload: Total=6, Passed=6, Failed=0, Blocked=0, Not Run=0, Skipped=0. Runtime, Controls and native Controls test DLL hashes matched the deployed payload. Cleanup completed without dirty-pool or cleanup failures; existing leak detection remained skipped.
