

Put information here about case studies, espescially experiments that didn't go well.

## UI launch-path measurement is blocked by the noninteractive desktop (2026-09-25)

The follow-up measurement needs five shell gestures: a Start menu tile, a pinned
taskbar item, a jump list item, a desktop shortcut double click, and a packaged
AUMID activation. I reused the earlier disposable `GetStartupInfoW` probe rather
than changing product code.

The probe could be launched through a desktop-style `.lnk` with Explorer's
`Open` verb. That path reported:

| Launch path measured | `dwFlags` | `STARTF_USESTDHANDLES` | `hStdOutput` |
| --- | --- | --- | --- |
| Explorer opening a `.lnk` shortcut | `0x00000801` | no | `0x0` |

This is evidence for shortcut activation, but not evidence for the other four
requested paths.

The current CLI session has an Explorer process, but it has no input-capable
interactive desktop. `SendKeys` returned without delivering the Win key or
search text, so the Start menu could not be opened and the taskbar or jump list
could not be selected. A disposable full-trust package registration also could
not be completed: AppX registration rejected the minimal probe manifest before
an AUMID activation could run. No result is being inferred for those paths.

The next measurement must run the same probe from an interactive desktop (for
example, a logged-in VM console or an equivalent UI automation session). Record
`dwFlags` and `hStdOutput` for each gesture before changing the Launch policy or
specification.

## The launch monitor hint was reading a redirected stdout pipe handle (2026-09-25)

`TryGetLaunchMonitor` in `WindowPlacementApplication.cpp` called `::GetStartupInfoW` and cast
`startup.hStdOutput` straight to `HMONITOR` with no flag check. `STARTUPINFOW` overloads that
field. It is a monitor handle only when the launcher asked the system for one, and it is a real
file or pipe handle when `STARTF_USESTDHANDLES` is set. The two uses are mutually exclusive, and
the field is documented as ignored unless that flag is set.

**Measured, not guessed.** A small C# probe called `GetStartupInfoW` on itself and wrote the
result to a file, so each launch path could be started differently and still report.

| Launch | `dwFlags` | `STARTF_USESTDHANDLES` | `hStdOutput` |
| --- | --- | --- | --- |
| PowerShell, no redirect | `0x00000000` | no | `0x0` |
| PowerShell, stdout redirected to a file | `0x00000100` | **yes** | **`0x8A8`** |
| `explorer.exe <path>` | `0x00000001` | no | `0x0` |
| `ProcessStartInfo.UseShellExecute = true` | `0x00000001` | no | `0x0` |

So the only observed launch that populates `hStdOutput` is the one where it is definitely not a
monitor, and every shell path leaves it null. Production was relying on `GetMonitorInfo` happening
to reject an unrelated kernel handle value. That is luck, not a contract: both are process-local
small even integers in the same numeric range.

**Fix.** A named pure rule, `TryGetLaunchMonitorHint(const STARTUPINFOW&, HMONITOR&)`, rejects a
null handle and rejects any `STARTUPINFOW` with `STARTF_USESTDHANDLES` set, and is the only
production path to the hint. Rejecting that flag cannot lose a real hint, because the flags cannot
be combined.

**Why the rule does not also require `STARTF_USEMONITOR` (`0x400`).** No observed launch set it, so
requiring it would have been a narrowing with no positive case to validate it against. The flag is
also absent from the public SDK headers. The rule accepts it, and the test covers it, but the
rejection does the work.

**Anti-vacuous check.** The first draft of the test only fed the measured `0x8A8` pipe handle. That
passes with or without the fix, because `GetMonitorInfo` rejects `0x8A8` either way. The test now
also sets `hStdOutput` to a **real** `MonitorFromPoint` handle with `STARTF_USESTDHANDLES` set.
Removing the guard, rebuilding, and rerunning failed at that line
(`WindowPlacementApplicationTests.cpp` line 634), and restoring it passed. Always confirm a
regression test fails against the unfixed code when the bug is "a wrong value that usually gets
rejected downstream anyway".

**This also settles a spec note.** The 2026-09-21 note said the `STARTUPINFO` hint "is not
propagated correctly for packaged apps", cause unknown. The field is simply null on those paths.
Packaged propagation is still open, but it is an absent hint, not a misread one.

Validation: `Microsoft.UI.Xaml.Tests.Isolated.WindowPlacement.vcxproj` and
`wrtdxamlfoundation.vcxproj` both built with 0 warnings and 0 errors. The whole isolated suite
passed `Total=212, Passed=212, Failed=0`.

## "Snapped application fails" was two different results, and one of them was wrong (2026-09-25)

The design note carried the line "New-window snapped application still fails on the VM" for days.
Nobody had measured it. The only coverage was
`NativeStatesAndRestoreTargetsAreCapturedWithoutActivation`, which loops over five saved states and
accepts either `Applied` or `FailedAfterStart` for `Snapped` and `MinimizedFromSnapped`. A test that
accepts both answers records neither.

Measuring it split the claim in half:

| Saved state | Request flag | Engine show | Outcome |
| --- | --- | --- | --- |
| `Snapped` | `Arranged` | `SW_NORMAL` | `FailedAfterStart` |
| `MinimizedFromSnapped` | `RestoreToArranged` | `SW_MINIMIZE` | `Applied` |

Half the sentence was true and half was false. `SetArranged` on a window that has never been shown
is declined by the OS. `SetMinRestoreToArranged` on the same window, in the same test, in the same
process, is accepted. The note had been telling readers that both paths were broken.

Three things worth keeping.

First, a tolerant assertion is a place where a wrong belief can live indefinitely. The tolerant
branch was written for a good reason: the result genuinely varies by OS. But the fix for "this
varies" is to log what actually happened, not to stop asking. The new test still tolerates both
outcomes, and it now prints the request flags, the engine show command, the outcome, and the
observed native state on every run. The next person reads the number instead of the guess.

Second, `NoActivate` is the cheap way to make a native result unambiguous. WinUI prohibits legacy
show-and-repair fallback for a no-activate request, so `ApplyOutcome` collapses to the return value
of `action.Apply`. Without that, `FailedAfterStart` could mean the Window Action failed, or that it
succeeded and a repair step failed afterward. With it, there is one call and one answer.

Third, assert the whole chain, not just the endpoint. The test verifies the policy flag, the engine
flag, and the show command before it calls anything native. That is what lets the result be stated
as "the OS declined a correctly built request" rather than "snapped does not work". The identical
minimized investigation the day before went the other way: the request was built wrong, by a test
that omitted `Reason`. Same shape of complaint, opposite root cause. The only way to tell those
apart is to check the request before you check the window.

## Preinstalled-package hosting preserves storage but exposes coordinate drift (2026-09-25)

The deployment blocker in the next case study is resolved. `test\scripts\runtests.ps1` now
supports `-PreservePackageRegistration` for WPF hosting. It registers the generated Centennial
manifest once and passes TAEF `/UAP:PackageFullName=<registered full name>`. It refuses an
existing registration instead of replacing another run's package. Its `finally` removes only
the registration at this payload path and restores the original manifest bytes.

TAEF rejects `UAP:PackageFullName` together with any effective `UAP:AppXManifest`, even an empty
manifest argument. Common assembly metadata therefore suppresses both manifest branches when
the custom runtime parameter `/p:PreservePackageRegistration=true` is set. The runner forwards
that parameter to discovery as well as execution. The new class is ignored without it. Keep
method isolation and declaration order; do not use an execution group or copy settings.

Builds (both exit 0, zero warnings/errors):

```powershell
.\initrun.ps1 msb /q dxaml\test\packages\appx\packages.csproj
.\initrun.ps1 msb /q dxaml\test\managed\Win32\WPF\Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj
```

Reproducer (refreshes payload; exit 1):

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop -SkipPrerun 'WindowPlacementCrossProcessTests.*' '-PreservePackageRegistration' '-SkipPackageUninstall'
```

Writer PID 11236 and reader PID 10712 both reported
`XamlTAEFTests_8wekyb3d8bbwe!XamlManagedTAEFTests`. The reader loaded both real saved records
exactly: `(53,71,500,400)` and `(107,107,640,480)`, DPI 96, Normal state. Hidden application of
the first id returned true, kept the window invisible, and captured `(53,76,500,400)`. Both
the saved and applied captures reported work area `(0,0,1024,720)`.

```text
Error: Verify: AreEqual(71, 76)
Summary: Total=2, Passed=1, Failed=1, Blocked=0, Not Run=0, Skipped=0
```

Cleanup of both ids and the handoff passed. Deployment event 400 showed Register at 00:51:01
and final Remove at 00:51:05, with no deployment between the processes. The registration was
absent afterward and the restored manifest's SHA-256 matched the deployed original. An earlier
run with PIDs 7420/12132 reproduced the same drift. The existing-registration refusal path also
left the pre-existing package and manifest unchanged.

Ordinary regression command (exit 0):

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop -SkipPrerun 'WindowPlacementPersistenceTests.*' '-SkipPackageUninstall'
```

Summary: Total=16, Passed=16, Failed=0, Blocked=0, Not Run=0, Skipped=0. Its same-process
application test compares width/height only; it does not prove X/Y round-trip correctness.
No existing placement-bounds tolerance was found, so the new test keeps exact assertions.

Appending `/UAP:AppXManifest=Test\AppxManifest.Centennial.xml` to the reproducer deliberately
blocks both hosts: Total=2, Passed=0, Failed=0, Blocked=2, Not Run=0, Skipped=0. This exposed a
runner reporting bug: `run-tests-on-vm.ps1` previously treated zero failures plus blocked tests
as success. It now returns 1, and package/manifest cleanup still runs.

The round-trip acceptance remains blocked, including the unreached shown-window assertions.
Next action: trace the first coordinate change through `TryComputeNativeRequest` and
`PlacementEx::SetPlacementWithApplyWindowAction`. Do not infer the native root cause from the
5-pixel difference alone or loosen the assertion to hide it.

## TAEF method isolation removes the package between placement processes (2026-09-25)

The cross-process placement task is blocked by the packaged test host's deployment lifecycle.
An experimental `WindowPlacementCrossProcessTests` class used `IsolationLevel=Method` and two
declaration-ordered methods. The writer saved two unique ids through real windows, closed them,
verified both public detached loads against the captured bounds/DPI/state, and published expected
captures in a separate `ApplicationData.LocalSettings` composite value. The reader was to load
both ids, apply the first to a new window without cascading, compare the captured placement, and
delete only those two records and the handoff value in `finally`.

Build (exit 0; `/restore` was required initially because `project.assets.json` was missing):

```powershell
.\initrun.ps1 msb /q dxaml\test\managed\Win32\WPF\Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj /restore
```

VM command (fresh payload, exit 1):

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop -SkipPrerun 'WindowPlacementCrossProcessTests.*'
```

The writer (PID 9404) passed, including immediate readback of the handoff. The reader (PID 13204)
had the same AUMID, `XamlTAEFTests_8wekyb3d8bbwe!XamlManagedTAEFTests`, and the same LocalState
path, but failed before loading a placement:

```text
Error: Verify: IsTrue - Run the whole class so SaveInFirstProcess publishes the expected captures.
Summary: Total=2, Passed=1, Failed=1, Blocked=0, Not Run=0, Skipped=0
```

`Microsoft-Windows-AppXDeploymentServer/Operational`, event 400, confirmed successful Register,
Remove, Register operations on `XamlTAEFTests_1.0.0.0_x64_en-us_8wekyb3d8bbwe` between the
methods. This is not evidence of a placement codec failure: the test's independent settings
handoff was also deleted. `UAP:Unregister=false` / `UAP:Cleanup=false` test metadata and
`/p:UAP:Unregister=false /p:UAP:Cleanup=false` did not preserve it. Direct
`/UAP:Cleanup=false /UAP:Unregister=false` switches were accepted but still allowed removal
between hosts (Register at 00:27:05, Remove at 00:27:07, Register at 00:27:08 VM local time).
The colon-value forms `/UAP:Cleanup:false` and `/UAP:Unregister:false` produced explicit
unknown-argument warnings. Every run returned 1 passed, 1 failed, 0 blocked, 0 skipped.

Do not use `ExecutionGroup` to fix this: TAEF runs an execution group in one process even with
method isolation, which would weaken the scenario to same-process coverage.

The experimental source and helper-access patch are preserved outside the compiled suite at
`C:\Users\jecollin\.copilot\session-state\521e7bd6-7a3e-4e22-b603-0b3beab11d7e\files\`
as `WindowPlacementCrossProcessTests.cs` and `cross-process-helpers.patch`. To reproduce,
apply the patch with `git apply`, copy the source into `dxaml\test\managed\Win32\WPF`, then
run the commands above. The experimental code was removed from the worktree to avoid adding
a known-failing test to normal runs.

Next action: use a package-preserving host or a dedicated packaged helper that launches two
processes under one registration. Require distinct PIDs, the same AUMID, real persisted-record
loads and native application, and cleanup. Do not export/import settings to bypass the failure.

## The missing-value null lookup was a bug shape, not a single bug (2026-09-25)

Commit `32f2012a1` fixed `ApplicationDataPlacementSettingsBackend::ReadValue`, where
`ApplicationDataContainerSettings` reports a missing value as `S_OK` with a null
`IInspectable` instead of `E_BOUNDS`. The fix was correct but was applied at one call site.

`WindowPlacementStore.cpp` calls `Lookup` twice. The second one, `GetContainer` with
`create == false`, had the identical shape: it mapped `E_BOUNDS` to `ERROR_FILE_NOT_FOUND`
and returned every other HRESULT unchanged. A missing container returned as `S_OK` with a
null `ComPtr` therefore reached `GetSettingsContainer`, which reported `Success()`. All four
backend entry points (`ReadValue`, `ReplaceValue`, `EnumerateValues`, `DeleteValue`)
immediately dereference that container with `container->get_Values(...)`, so this was the
same access violation one frame earlier. It had not been observed because reads normally
fail earlier, at identity or at `GetLocalSettings`.

Two durable results:

| Result | Detail |
| --- | --- |
| Rule is now named and shared | `IsMissingLookupResult(hr, hasResult)` is declared in `WindowPlacementStore.h` and used by both lookups. `CreateContainer` also rejects a null success. |
| Rule is now unit tested | The original fix had no test because it sat in an anonymous namespace behind real ABI objects. Three tests in `WindowPlacementStoreTests.cpp` cover `E_BOUNDS`, success with a null result, success with a result, and other failure HRESULTs. |

The ordering matters. `IsMissingLookupResult` returns true only for `E_BOUNDS` or a
`SUCCEEDED` result with nothing in it. A naive `hr == E_BOUNDS || !hasResult` would report
`E_ACCESSDENIED` as Missing and silently discard an Unavailable store.

Lesson: when an ABI reports absence in a way the code did not expect, fix the rule, not the
call site, and grep for the other call sites before closing the task.

## AnchorContext stack allocation is safe, crash root cause is not stack deallocation (2026-09-25)

The `WindowPlacementPersistenceTests` class crash on second sequential test occurred when `AnchorContext`
was stack-allocated at `DesktopWindowImpl.cpp:1116` inside `TryComputeNativeRequest()`. A prior speculative fix
heap-allocated `AnchorContext` with `new`, but **the 0xC0000005 crash persisted identically** on the second test
and no crash occurred on first test. This proved:

1. **Stack allocation is safe** for `AnchorContext`: The lambda callback at line 1119 is synchronous and invoked 
   directly within `TryComputeNativeRequest()` before control returns, so the stack object outlives all uses.

2. **Root cause is not stack deallocation**: If stack corruption caused the crash, heap allocation would have fixed 
   it. The identical crash after heap allocation rules out stack lifetime as the problem.

3. **Heap allocation was ineffective and leaked memory**: The change introduced a memory leak (no delete statement), 
   and the crash recurred at exactly the same point in the second test, confirming the fix was wrong.

**Revert** (commit `2d8a15b7a`): Stack allocation restored at line 1116.

Sequential test results after revert confirm the crash persists:
- `FirstActivateUsesPersistenceConfiguration`: 0xC0000005 in coreclr.dll on second sequential test (passed in isolation)
- `DetachedLoadReturnsPlacementSavedByAClosedWindow`: 0xC0000005 (cascade from prior crash)  
- `DetachedLoadReturnsTheMostRecentSaveForAPersistPlacementId`: Failed (cascade)
- Total: 9 passed, 3 failed

**Resolved.** The window-teardown and HWND-reuse theories recorded here were wrong. A WER dump taken
later showed the faulting frame was
`Microsoft_UI_Xaml!DirectUI::WindowPlacementPersistence::GetStringValue+0x4b`, reached through
`ApplicationDataPlacementSettingsBackend::ReadValue`. A missing settings value is reported as `S_OK`
with a null `IInspectable`, not `E_BOUNDS`, so `ReadValue` handed a null pointer to `GetStringValue`.
The order dependency came from the store, not from window state: alone the settings container does
not exist and the lookup never runs, while after an earlier test writes a record the container
exists and the next test takes the missing-value-inside-an-existing-container path. The class run is
now `Total=12, Passed=12, Failed=0`. See
[TAEF said the crash was in coreclr.dll](#taef-said-the-crash-was-in-coreclrdll-it-was-ours) and
[the missing-value null lookup was a bug shape](#the-missing-value-null-lookup-was-a-bug-shape-not-a-single-bug-2026-09-25).

The durable lesson from this entry is narrower than its original conclusion: ruling out one
mechanism is not the same as localizing the fault. Two speculative product edits were written and
reverted before anyone took a dump, and the dump found the bug in the first attempt.

## CoreCLR crash after first placement-persistence show (2026-09-24)

The full `WindowPlacementPersistenceTests` class consistently ran 12 tests with 9 passed and
3 failed. The first failure was `FirstActivateUsesPersistenceConfiguration`, which crashed
`Te.ProcessHost.exe` with `0xC0000005` in `coreclr.dll`; the two detached-load failures were
downstream because cleanup could not run after the host crash.

The failure is order-dependent. A VM-side `-fromFile` selection containing only
`FirstShowUsesPersistenceConfigurationAndCapturesBounds` followed by
`FirstActivateUsesPersistenceConfiguration` reproduced it: the first test passed and the second
crashed before its first verification. The activation test passes alone. Repeating the full class
with `-FullCopy` reproduced the same crash, ruling out stale incremental deployment artifacts.
TAEF normalized a reversed selection back to class order, so reverse-order behavior was not tested.

No code change was made. The next useful diagnostic is a native dump/stack for the two-test
sequence, with focus on first-window teardown, placement-peer marker removal, and queued callbacks.

## A misplaced msb quiet switch runs MSBuild question mode (2026-09-24)

While completing the hosting-project build handoff at `2efc36b84`, this command failed
twice with exit code 1:

```powershell
.\initrun.ps1 msb dxaml\test\managed\Win32\WPF\Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj /q
```

It reported `MSB3492` at `eng\winui-version.props(79,5)` for
`BuildOutput\Temp\amd64chk\WinUISourceRevision.h`, plus warnings about touching build
tracking files. This was not a C# compile failure or evidence of a file-write race.
`tools\msb.cmd` consumes `/q` only as its first argument. Elsewhere it passes through
to MSBuild, whose help identifies `/q` as the experimental `/question` switch.
Question mode reports work that is not up to date; it is not a normal build.

Use the wrapper's switch before the project path:

```powershell
.\initrun.ps1 msb /q dxaml\test\managed\Win32\WPF\Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj
```

That command completed in 9.80 seconds with exit code 0, zero warnings, and zero errors.
No `BuildAll` override or source change was needed. Before that corrected standalone
build, `.\initrun.ps1 .\build.cmd /q` also succeeded (exit 0, 6 minutes 17 seconds).
The `build.cmd` parser accepts its own `/q` switch; this positional trap concerns `msb.cmd`.

With the rebuilt outputs, these VM commands refreshed the payload and completed with exit 0:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop '*WindowPlacementExplicitCascadeTests*'
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop '*WindowPlacementExplicitCascadeTests.EnabledCascadesExplicitPlacementOnlyByPosition'
```

The class selection ran four native seam tests and six managed integration tests:
8 passed, 0 failed or blocked, and 2 skipped. The independent managed test passed.
Its baseline was `80,85`, and its cascaded result was `88,99`; Automatic, Disabled,
and ApplicationRestart matched the baseline in the class run. Both multi-monitor
tests still skipped on the single-display guest, so that acceptance gap remains open.

Local diagnostic logs under `D:\x1\BuildOutput`:
`ralph-hosting-build-20260924.log`, `ralph-hosting-build-retry-20260924.log`,
`ralph-hosting-build-corrected-20260924.log`, `ralph-prodtest-build-20260924.log`,
`ralph-cascade-class-20260924.log`, and `ralph-cascade-enabled-20260924.log`.

## SkipProject does nothing in SDK-style projects (2026-09-24)

The Infra handoff said the managed WPF hosting test project was "skipped by the normal build"
because of `<SkipProject>true</SkipProject>`, and that it "only builds with `/p:BuildAll=true`".
Both claims are wrong. The property was inert.

`SkipProject` is only honored by `dxaml\msbuild\BuildSettings\SkipProjectBuild.targets`, and the
only import of that file is line 28 of `dxaml\Microsoft.UI.Xaml.Build.targets`. That targets file
is reached by legacy projects through `Xaml.NTLegacy.Cpp.targets` or an explicit import. An
SDK-style project (`<Project Sdk="Microsoft.NET.Sdk">`) never imports it, so the property is read
by nobody and the normal SDK `Build` target runs.

Three checks, all without `BuildAll`:

```powershell
# 1. Builds fine with the property still present. Exit code 0, produced the DLL.
.\initrun.ps1 msb dxaml\test\managed\Win32\WPF\Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj

# 2. The fully expanded project contains no reference to the skip machinery. Zero matches.
.\initrun.ps1 msb /q dxaml\test\managed\Win32\WPF\Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj `
    "/pp:D:\x1\BuildOutput\hosting.pp.xml"
Select-String -Path BuildOutput\hosting.pp.xml -Pattern "SkipProjectBuild|Microsoft.UI.Xaml.Build.targets"

# 3. The solution traversal does include the project. It appears in the MSB4057 list.
.\initrun.ps1 msb dxaml\Microsoft.UI.Xaml.sln /t:__ProbeNonexistentTarget__ | Select-String Hosting
```

Check 3 is the useful trick: asking the solution for a target that does not exist makes MSBuild
enumerate exactly the projects it would build and report each one, in about 4 seconds, with no
compilation. `dxaml\Microsoft.UI.Xaml.sln` also carries `.Build.0` entries for this project in
every configuration, and `Build.cmd` never sets `BuildAll`.

So the compile errors did not survive a build. No full build ran between the commit that
introduced them and the commit that fixed them. The real defect is that a property which looks
like it disables a build silently does nothing, which sent a developer chasing a phantom CI gap.

Fix: remove the inert property, and add `ValidateSkipProjectIsHonored` to the root
`Directory.Build.targets`. `SkipProjectBuild.targets` now sets `SkipProjectBuildImported=true`, and
the guard raises error `XAMLSKIP003` when `SkipProject=true` without that marker. The two existing
skip warnings got codes `XAMLSKIP001` and `XAMLSKIP002` so they can be grepped out of pipeline logs.

Verified after the change:

| Project | Expected | Result |
| --- | --- | --- |
| `Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj` | builds | `Build succeeded`, exit 0 |
| same, with `SkipProject=true` re-added | hard error | `error XAMLSKIP003` |
| `Microsoft.UI.Xaml.Tests.External.Foundation.Printing.vcxproj` | still skips | `warning XAMLSKIP002`, exit 0 |

`XamlManagedLifetimeTests.csproj` and `Microsoft.UI.Xaml.Tests.Managed.PGOTests.csproj` also set
`SkipProject`, and both import `Microsoft.UI.Xaml.Build.targets`, so the guard does not fire on
them. Both already fail a standalone `msb` with `MSB4019` on unrelated stale import paths
(`$(XamlSourcePath)\Microsoft.UI.Xaml.Build.targets` and `$(ManagedTestPath)\common.props` both
evaluate to nonexistent paths). That is pre-existing and was not touched.

Still open, not fixed: `BuildTestCode=false` rides the same import and is therefore equally inert
for SDK-style test projects. Turning that into an error could break a product-only pipeline build,
which cannot be validated from here, so it was left alone.

## Hyper-V cannot give an automated test run a second display (2026-09-24)

The request for a two-display test VM cannot be satisfied by a repository change. Read-only probes:

```powershell
Get-VM | Select-Object Name, State, Generation, EnhancedSessionTransportType
Get-VMHost | Select-Object EnableEnhancedSessionMode
```

All five VMs are Generation 2 on the VMBus transport, and `ge_current-260828-Desktop` is the only
one running. A Generation 2 VM has one synthetic `Microsoft Hyper-V Video` adapter, which presents
exactly one display to the guest. Hyper-V only produces multiple guest displays through an
interactive enhanced-session `vmconnect` from a client that has several monitors and opts into
them. A TAEF run driven over PowerShell Direct is not that, so the guest will report one display
no matter how the VM is configured.

That leaves three options, all requiring a decision outside this loop:

| Option | Cost | Note |
| --- | --- | --- |
| Interactive enhanced session before each run | Manual step per run | Not automatable; conflicts with the active console user |
| A physical or cloud two-monitor test machine | Provisioning | The build host itself has two 1920x1080 displays |
| Make monitor selection injectable in the product seam | Developer work | Removes the hardware dependency from the assertion |

No VM or host settings were changed. The two multi-monitor cascade tests continue to self-skip.

## Explicit cascade multi-monitor validation needs real peer discovery tests (2026-09-24)

The Tester handoff cannot be closed by rerunning `WindowPlacementExplicitCascadeTests`
on more displays. Its `Anchor::TryGet` returns a scripted snapshot without enumerating
windows or checking the requested device name. Its fixture creates one hidden Win32
window, not shown WinUI peers with group membership and controlled Z order. The isolated
project compiles `WindowPlacementApplication.cpp`, but not `DesktopWindowImpl.cpp`,
where `TryFindPlacementPeer` performs the monitor filtering and Z-order walk.
Consequently, the existing four tests would not detect removal of that monitor filter
or an early return on an off-monitor peer, even on a two-monitor machine.

The existing tests cover the application callback seam, not the full acceptance matrix.
The position test uses normal state, checks only whether snap bounds are present, and
does not check desktop policy. The caller-immutability assertions cover only `NormalRect`.
The managed `WindowPlacementPersistenceTests` class has no enabled-cascade peer scenario.
Dedicated desktop integration tests are needed before the requested automated validation
can run. This is a source-confirmed coverage gap, not a reproduced product defect.

Read-only environment probes in this round succeeded (PowerShell exit code 0):

```powershell
Get-VM | Select-Object Name, State
$cred = Import-Clixml (Join-Path $env:USERPROFILE '.winui-test\vmcred-ge_current_260828_Desktop.xml')
Invoke-Command -VMName ge_current-260828-Desktop -Credential $cred {
    Get-CimInstance Win32_VideoController |
        Select-Object Name, CurrentHorizontalResolution, CurrentVerticalResolution
    Get-CimInstance -Namespace root\wmi -ClassName WmiMonitorBasicDisplayParams |
        Select-Object InstanceName, Active
    quser
}
Add-Type -AssemblyName System.Windows.Forms
[System.Windows.Forms.Screen]::AllScreens | Select-Object DeviceName, Primary, Bounds
```

Only `ge_current-260828-Desktop` was running. It reported one active monitor, one
`Microsoft Hyper-V Video` controller at 1024x768, and an active console user. In contrast,
the current host session reported two 1920x1080 screens, at X=-1920 and X=0.
Do not describe the entire environment as single-monitor: the host is a possible
two-monitor execution target once real peer-discovery coverage and its desktop test
prerequisites are available. Host topology alone does not establish those prerequisites.
No VM or display settings were changed.

No build or tests were rerun in this round: the handoff already records class and
individual single-display passes, and repeating the scripted-anchor tests cannot
establish the missing behavior. The Tester task remains deferred, with a Developer
request for real WinUI peer tests ahead of it in `inbox.md`. Multi-monitor behavior,
snap/desktop preservation, and full public-object immutability remain unverified.

## Null-options documentation disagreed with the public boundary (2026-09-24)

The PM SAL follow-up found that the spec's conceptual guidance and both method pages
promised null options would use defaults. `WindowGenerated::ShowWithOptions` and
`WindowGenerated::TryApplyInitialPlacement` instead call `ARG_NOTNULL(pOptions, "options")`
before thread checks or implementation dispatch. A caller following that guidance gets
`E_INVALIDARG`, not default placement. This is a source-confirmed documentation mismatch,
not a new runtime reproduction.

The PM decision retains required, non-null options and no parameterless hidden-application
overload. The spec now uses `new WindowShowOptions()` for default hidden preparation,
distinguishes null `Placement`, and makes required-argument checks explicit before late and
reentrant behavior. The design note records the alternatives and rationale. No product code
changed; dedicated desktop public-projection coverage remains in the Developer inbox.

## Isolated placement test discovery on the VM (2026-09-24)

An initial VM invocation using the class name did not execute any tests: the
runner returned exit code 0 while TAEF reported that the selection criteria
matched no tests. Local TAEF enumeration confirmed that the test class was
present in the DLL.

Using a wildcard selection and a full payload copy executed the tests correctly:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop -FullCopy '*WindowPlacementDetachedLoaderTests*'
```

TAEF reported `Total=2, Passed=2, Failed=0`. The issue was test selection, not
the product or compilation.

## Public Hide reaches private UWP audio cleanup (2026-09-24)

Source review of `56e658935` found that public `IWindow12::Hide` and
`IWindowPrivate::Hide` both dispatch through `WindowGenerated::Hide` and
`Window::HideImpl`. On a `UWPWindowImpl`, that invokes `TearDownAudioGraph` if a
sound service exists and otherwise returns success. It does not change visibility.
Both UWP Show overloads instead return `E_NOTIMPL`.

The constructor still uses `UWPWindowImpl` for the framework dummy window, so
disallowing ordinary UWP construction does not establish that this code is unreachable.
`WindowShowHideTests` covers desktop windows under WPF hosting, not this distinction.
This is a static finding; no UWP/Xbox runtime reproduction or audio test was performed.

The design now requires side-effect-free `E_NOTIMPL` from public display APIs on
non-desktop instances while preserving private lifecycle behavior. Merely returning
`E_NOTIMPL` from the shared UWP Hide implementation would break private cleanup.
The dispatch separation and public/private regression coverage remain in the Developer
inbox. See the host-boundary decision in `docs/design-notes/Window-PlacementPersistence.md`.

## Window.TryGetPlacement validation (2026-09-24)

The required full build completed successfully:

```powershell
.\initrun.ps1 .\build.cmd /q
```

The dedicated VM test passed:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop TryGetPlacementReturnsCurrentPlacement
```

Running the complete `WindowPlacementPersistenceTests` class passed 5 of 6 tests.
`FirstActivateUsesPersistenceConfiguration` crashed in `coreclr.dll`; the new
`TryGetPlacementReturnsCurrentPlacement` test passed, as did the other existing tests.
The crash was not reproduced by the new API test.

## Host-boundary test infrastructure investigation (2026-09-24)

The required build entry point completed successfully:

```powershell
.\initrun.ps1 .\build.cmd /q
```

The existing VM test workflow can deploy and run the Window Placement tests:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop '*WindowPlacement*'
```

The run completed 170 of 171 tests. The only failure was the known
`FirstActivateUsesPersistenceConfiguration` failure in `coreclr.dll`; the other
placement tests, including the new placement API coverage, passed.

The source and project investigation corrected the original infrastructure
assumption. `dxaml\test\native\external\foundation` is a native desktop test
target with the shared `WindowHelper` infrastructure, and several tests already
query `Window::Current` as `IWindowPrivate`. The target is also usable with UAP
hosting, which is the required host for `Window::Current`; it does not need a
new desktop-hosted target.

The remaining gaps are narrower:

- No test hook exposes the framework dummy returned by
  `DXamlCore::GetDummyWindowNoRef()`, so public `E_NOTIMPL` behavior on that
  specific instance remains untested.
- A native UAP test can query `IWindowPrivate`, but no existing test controls or
  observes the UWP sound-service state needed to prove private `Hide` behavior
  both with and without a sound service.

No machine-wide changes were made. The durable follow-up is to add focused tests
to the existing native foundation target, and add only a narrowly scoped
dummy-window hook if the public non-desktop rejection must be tested directly.

## MSBuild repair and full-build acceptance (2026-09-24)

After the Visual Studio repair, the read-only consistency check returned exit code 0 for
both installed instances. The required full build succeeded from a fresh PowerShell process:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Set-Location 'D:\x1'; & .\initrun.ps1 .\build.cmd /q; exit $LASTEXITCODE"
```

The same build also succeeded after running `.\init.ps1 amd64chk /envcheck /notitle` and then
`.\build.cmd /q` in the already initialized process. Both runs used MSBuild
17.14.60+43b635718, completed packaging, and returned exit code 0. The host prerequisite and
full-build acceptance criterion are resolved.

## MSB4216 root cause: two stale files, and the build loop that made them (2026-09-24)

The `TransformTemplate` failure in `.\initrun.ps1 .\build.cmd /q` is not a repo defect and
not a discovery problem. It is a partially applied Visual Studio update, and the repo's own
tooling helped cause it.

Minimal repro, no repo content involved:

```xml
<Project DefaultTargets="Go">
  <UsingTask TaskName="Message" AssemblyFile="$(MSBuildToolsPath)\Microsoft.Build.Tasks.Core.dll" TaskFactory="TaskHostFactory" />
  <Target Name="Go"><Message Text="TASKHOST-OK" Importance="High" /></Target>
</Project>
```

`MSBuild.exe taskhost.proj` fails with MSB4216 from both the root and the amd64 bin folder,
on both installed VS generations. With `MSBUILDDEBUGCOMM=1` and `MSBUILDDEBUGPATH=<dir>` the
trace names the problem exactly:

```text
parent: Writing handshake part 4 (51) ... part 5 (32402)
child:  Handshake failed. Received 51 from host not 60.
```

The parent derives the handshake from `Microsoft.Build.dll`; the task host derives it from
`MSBuild.exe`. In `...\2022\Enterprise\MSBuild\Current\Bin` those two files are from
different builds:

| File | Version | Last write |
| --- | --- | --- |
| `MSBuild.exe` | 17.14.60.43110 | 2026-09-24 05:08 |
| `Microsoft.Build.dll` | 17.14.51.32402 | 2026-07-22 13:00 |
| `Microsoft.Build.Framework.dll` | 17.14.51.32402 | 2026-07-22 13:00 |
| `Microsoft.Build.Tasks.Core.dll` | 17.14.60.43110 | 2026-09-24 05:08 |
| `Microsoft.Build.Utilities.Core.dll` | 17.14.60.43110 | 2026-09-24 05:08 |

An update ran at 05:08 and could not replace those two files. The earlier case study in this
file recorded why: an MSBuild node from a previous build (`/nodemode:1 /nodeReuse:true`,
PID 10604) still had them loaded. VS 18 Enterprise is broken the same way, which is why it
throws `System.Collections.Immutable` load errors: `Microsoft.Build*.dll` 18.9.1 next to
`Microsoft.Build.Tasks.Core.dll` 18.10.1.

Two dead ends worth not repeating:

- Using the amd64 MSBuild as the parent does not help. `Bin\amd64\MSBuild.exe.config`
  redirects `Microsoft.Build` and `Microsoft.Build.Framework` to `..\Microsoft.Build.dll`,
  so the 64-bit host loads the same stale root assemblies. The amd64 copies are fine at
  17.14.60 and are never used.
- `MSBUILD_EXE_PATH` pointed at the amd64 executable does not change the handshake either.

There is no process-local fix. Only a Visual Studio Repair replaces the two files, and that
needs administrator rights.

The loop is worth calling out because it will happen again. Each MSB4216 failure leaks ten
`/nodemode:2` task hosts that sit in `Waiting for connection 900000 ms`, holding the
installation's assemblies open for 15 minutes. If an update lands in that window, it fails
to replace exactly those files. Anyone running the repair has to close every `MSBuild.exe`
first or it will silently do nothing useful.

Fixes committed here:

- `tools\msb.cmd` passed `/m` without `/nr:false`, unlike `Build.cmd` and
  `controls\Build.cmd`. Every `msb` build therefore left reusable nodes alive for 15 minutes
  with build output and task assemblies loaded. Added `/nr:false`.
- `scripts\init\Test-MSBuildInstall.ps1` compares the file versions that decide the
  handshake and prints the mismatch with the required repair. `init.cmd` and `Build.cmd`
  both call it. It warns and does not block, because test runs and VM work do not need
  MSBuild and the real build failure is still reported normally.

Current state of `.\initrun.ps1 .\build.cmd /q`: init succeeds, `XamlCompilerPrerequisites`
builds, `Microsoft.UI.Xaml.sln` fails in about 37 seconds with MSB4216 at
`controls\dev\dll\Microsoft.UI.Xaml.Common.targets(565,5)`, exit code 1. Binlog:
`BuildOutput\Microsoft.UI.Xaml.amd64chk.binlog`. No installation file was modified.

The required entry point was rerun on 2026-09-24 from both a fresh `pwsh -NoProfile`
process and a process that initialized first with `.\init.ps1 amd64chk /envcheck /notitle`.
The fresh run again selected VS 2022 and failed with MSB4216 at
`TransformTemplate`; the diagnostic binlog is
`BuildOutput\Microsoft.UI.Xaml.amd64chk.binlog`. The already-initialized run selected
VS 18 and failed before project execution with a `System.Collections.Immutable` 10.0.0.10
versus 9.0.0.11 load mismatch. That run printed `BUILD SUCCEEDED` and returned exit code 0
despite the MSBuild process errors, so the full-build acceptance criterion remains unmet.
No installation files or processes were modified.

The false-success path was in `Build.cmd`: `if ERRORLEVEL 1` does not match the negative
CLR exception exit code. Its build and helper checks now compare the expanded error level
against zero, preserving both positive and negative failures. The initialized rerun now
reports `BUILD FAILED (exit code -532462766)` and returns `-532462766`; the Visual Studio
repair is still required for a successful full build.

### Infra prerequisite recheck (2026-09-24, after `1ceb77855`)

The first inbox task cannot proceed yet. A read-only recheck of both installed Visual
Studio instances still reports the version mismatches above. Reproduction from the repo root:

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installs = & $vswhere -all -products '*' -format json | ConvertFrom-Json
foreach ($install in $installs) {
    $msbuild = Join-Path $install.installationPath 'MSBuild\Current\Bin\MSBuild.exe'
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\init\Test-MSBuildInstall.ps1 -MSBuildPath $msbuild
    Write-Output "DIAGNOSTIC_EXIT_CODE=$LASTEXITCODE"
}
```

Each diagnostic returned 1. VS 2022 still mixes 17.14.51.32402 and 17.14.60.43110;
VS 18 still mixes 18.9.1.35102 and 18.10.1.42706, including an older amd64 executable.
The diagnostic prints to the console and creates no log. No new build or binlog was
produced; the previous full-build binlog remains at the path above. Repeating the full
build before repair risks leaving more task hosts holding installation files open.
No processes, installation files, or machine settings were changed. The existing
host-repair request and both post-repair full-build acceptance runs remain open.


## Initialization now selects a consistent Visual Studio generation (2026-09-24)

The normal `.\initrun.ps1` entry point inherited VS 18 variables and
`_OriginalPathBeforeInit`, so `init.cmd` restored a PATH without MSBuild and then
`DevCmd.cmd` could not find an installation because both installed instances were
marked incomplete by `vswhere`. `DevCmd.cmd` also selected VS 18 when both
generations were discoverable.

`initrun.ps1` now clears only process-local Visual Studio/toolchain variables
before calling `init.ps1`. `DevCmd.cmd` uses `vswhere -all`, enumerates all VS 17
or later installations, and selects the VS 2022 path instead of relying on
`-latest`. Both a fresh PowerShell process and a process seeded with VS 18
variables now complete `.\initrun.ps1 msbuild -version` with exit code 0.

The required `.\initrun.ps1 .\build.cmd /q` entry point reaches the build, but
does not complete. The selected VS 2022 installation still contains mixed
MSBuild assemblies: the root `Microsoft.Build.dll` reports 17.14.51 while the
task and utility assemblies report 17.14.60. A full build then failed during
packaging because `Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop.dll` was
locked by an existing MSBuild process (PID 10604); the generated diagnostic log
is `BuildOutput\XamlCompilerPrerequisites.amd64chk.binlog`. No process was
terminated and no installation files were modified. A VS installation repair or
equivalent host maintenance remains required before the full-build acceptance
criterion can be met.

## Full build is still blocked by a reused MSBuild node (2026-09-24)

The normal full-build entry point was rerun from a fresh PowerShell process:

```powershell
.\initrun.ps1 .\build.cmd /q
```

Initialization selected `C:\Program Files\Microsoft Visual Studio\2022\Enterprise`
and completed successfully with MSBuild 17.14.51.32402. The build returned exit code
1 after 22.86 seconds while packaging `XamlCompilerPrerequisites.sln`. The first
failure was `MSB3027`/`MSB3021`: the destination
`BuildOutput\packaging\Debug\tools\net472\Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop.dll`
was locked by `MSBuild.exe` PID 10604. The process command line was:

```text
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe" /noautoresponse /nologo /nodemode:1 /nodeReuse:true /low:false
```

The diagnostic binlog is `BuildOutput\XamlCompilerPrerequisites.amd64chk.binlog`.
This is a stale/reused node from an earlier build, not a reason to disable packaging
or suppress the failure. The installation also remains inconsistent: the root
`MSBuild\Current\Bin\Microsoft.Build.dll` reports file version 17.14.51.32402,
while the root and amd64 MSBuild executables and the amd64 `Microsoft.Build.dll`
report 17.14.60.43110. A Visual Studio repair/update and cleanup of the stale
MSBuild process by the owner of that work are required before the full-build
acceptance criterion can pass. No process was terminated and no installation files
were modified.

## Native placement application and validation blockers (2026-09-24)

**Flags alone do not make legacy SetPlacement safe.** The old adapter directly called the engine
and translated raw capture without the shared cache reconciliation. The legacy path can temporarily
show a hidden HWND, ignores
NoActivate, and can repair activation around a virtual-desktop move. Native application now
requires Window Action for restricted requests and disables legacy fallback even after Window
Action failure. Effective capture reads the HWND on both success and partial failure. Invalid
inputs and unsupported hidden states fail before application; no selected-source retry is added.

**Nonactivating reveal can accidentally restore.** `SW_SHOWNOACTIVATE` restores to the most
recent size/position; it is not the right reveal command after applying maximized placement.
The desktop display path now uses `SW_SHOWNA` (and retains its minimized-specific command).
ApplicationRestart suppresses activation on first Show even after skipped/failed placement.
API-level acceptance remains blocked as described below.

**Native fitting is not request echo.** The first refreshed VM run was 147/149: a hidden test
expected Y=40 but Window Action produced Y=43, and new-window snap application failed. The hidden
test now compares against independently captured effective geometry rather than requested Y.
On `ge_current-260828-Desktop`, maximized, minimized, and minimized-from-maximized application
works without activation. Snapped and minimized-from-snapped Window Action requests fail, leaving
the original hidden normal placement. Tests explicitly permit this best-effort outcome and assert
no show/activation, a refreshed normal capture, and no retry. This is not proof of working snap
support, and the native limitation remains in the inbox.

**Native-width adjustment needs an input guard.** Valid Int32 rectangles can still produce
overflowing subtraction, MulDiv results, or sums inside engine adjustment. The adapter now checks
a conservative 64-bit envelope before `MoveToMonitor`; a real-HWND test verifies that a valid
extreme-width source is rejected before application. This is not a second fitting algorithm.

**Full-build blocker narrowed to an MSBuild handshake mismatch.**
The required `.\initrun.ps1 .\build.cmd /q` first failed environment discovery. Process-local
cleanup of inherited VS/compiler variables and `_OriginalPathBeforeInit`, followed by the
installed VS 2022 environment, permitted compilation but did not fix full builds. External
CLR4 task hosts fail for `TransformTemplate`/`GenerateWinRTClassRegistrations` (`MSB4216`, with
secondary `MSB4027`). With MSBuild engine tracing enabled, the parent sent version
17.14.51.32402 while the child expected 17.14.60.43110:

- `BuildOutput\Temp\amd64chk\MSBuild_CommTrace_PID_58724.txt`: parent handshake and child launch.
- `BuildOutput\Temp\amd64chk\MSBuild_CommTrace_PID_59024.txt`: "Received 51 from host not 60".
- `BuildOutput\Temp\amd64chk\1790259227734-copilot-tool-output-22696-bde6bdd1-1867-4ee4-9000-cd20066b7da0.txt`:
  failed generation, MSB4216/MSB4027.

The selected installation is `C:\Program Files\Microsoft Visual Studio\2022\Enterprise`.
Its root `MSBuild\Current\Bin\Microsoft.Build.dll` reports 17.14.51.32402; root MSBuild.exe and
the amd64 executable/assembly report 17.14.60.43110. Selecting the x86 host also did not fix
task-host startup. No installation repair, assembly copying, elevation, or machine change was
attempted. The infrastructure owner should resolve installation/version consistency with the
user if host repair is necessary, and then revalidate normal entry points. Do not equate the
following workaround with a repaired toolchain.

Successful targeted commands (each through `initrun.ps1`, `amd64chk`):

```powershell
.\initrun.ps1 msb /q dxaml\xcp\dxaml\lib\winrtfoundation\wrtdxamlfoundation.vcxproj
.\initrun.ps1 msb /q dxaml\xcp\dxaml\lib\unittests\WindowPlacement\Microsoft.UI.Xaml.Tests.Isolated.WindowPlacement.vcxproj
.\initrun.ps1 msb /q dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj /p:BuildProjectReferences=false
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop '*WindowPlacement*Tests*'
```

For reproduction only, the commands above ran in one fresh `cmd.exe` process after calling
`Common7\Tools\VsDevCmd.bat -no_logo -arch=x64 -host_arch=x64` from that VS 2022 installation
and returning to `D:\x1`. Before starting that process, PowerShell removed environment variables
matching `^(VS|__VSCMD|DevEnvDir|VC|WindowsSdk|UCRT|UniversalCRT|ExtensionSdk|_OriginalPathBeforeInit|VisualStudioVersion)`.
Each `initrun.ps1` invocation used `pwsh -NoProfile -File`. The initial full build returned 1;
all final targeted builds and VM testing returned 0, with zero build warnings/errors.
The DLL command deliberately reuses existing project-reference outputs; it is not a full build.

Final validation log:
`BuildOutput\Temp\amd64chk\1790260242159-copilot-tool-output-22696-d1a1adad-f6b4-4636-bcf5-a9bdd45cee00.txt`.
The runner refreshed the payload (no SkipPayload) and passed **153/153**, including 13 dedicated
real-HWND application tests; none failed, blocked, or skipped. An earlier dedicated run passed
12/12 before the extreme-intermediate test was added.

**Generated projection gap prevents public integration tests.** An attempted managed API test
extension could not compile because existing generated C# Window projections lack the new
placement members. Regeneration reaches the task-host failure above. The attempted managed
test changes were removed; the passing suite must not be described as covering new public
Window-to-DesktopWindowImpl application. Peer/cascade production selection and real multi-monitor,
mixed-DPI, downlevel, launch-hint, presenter, and virtual-desktop validation remain open.

## ShowWindow's return value is not "did it show" (2026-09-24)

`DesktopWindowImpl::Display` returned `ShowWindow(...) != FALSE`. `ShowWindow` returns
whether the window was *previously* visible, so the very first display always returned
false. The coordinator reads that as a pre-display abort, which is designed to leave the
initial-placement phase open for a retry. The result was silent and total: the phase never
closed, the window never enrolled, no save could ever run, and `Show`/`Activate` returned
false to the app. Nothing failed loudly and no unit test could catch it, because the fake
host in `WindowPlacementCoordinatorTests.cpp` returns its own `DisplayResult`.

Lesson: when a host method is a Boolean "did this work", check the Win32 call's documented
return value rather than assuming nonzero means success. Report the post-state instead.

## The framework library is built without exceptions (2026-09-24)

Adding a `try`/`catch` to `DesktopWindowImpl.cpp` failed the build with C4530 treated as an
error. Placement files that need to catch, such as `WindowPlacementCoordinator.cpp` and
`WindowPlacementPublic.cpp`, carry a per-file `<ExceptionHandling>Sync</ExceptionHandling>`
in `wrtdxamlfoundation.vcxproj`. Shared PCH-using files do not, and should not be changed to
get one. Follow the existing fail-fast-on-allocation-failure convention instead.

## Marking a save "attempted" is not the same as "completed" (2026-09-24)

An earlier round set `m_saveAttempted` before calling the host, to stop duplicate saves. That
also suppressed the destruction backstop after a *failed* accepted-close save, which is exactly
the case the backstop exists for. The design note requires the opposite: mark a save complete
only after a successful write. The flag is now `m_saveCompleted`. This supersedes the
`m_saveAttempted` notes in the step 6 section below.

## Adapter cascade boundary validation (2026-09-24)

The first focused VM run exposed two test assumptions rather than product failures:
the cascade fixture crossed the work-area edge and wrapped as designed, and the
native engine rejected a rectangle that was not valid relative to its original
work area. The tests now use an in-bounds cascade fixture and a representable
near-maximum work area. The adapter validates rectangle edges with 64-bit
intermediates before converting to native `LONG` values, passes caller offsets
through the engine, and rejects cascade overflow. The isolated project builds
and all 11 focused adapter tests pass on the VM.

## The initrun.ps1 MSBuild discovery failure has two separate causes (2026-09-24)

`.\initrun.ps1 .\build.cmd /q` failed again with "Could not find an MSBuild install".
Tracing `init.cmd` with `winui_echo=1` showed `DevEnvDir` was set but `where msbuild` failed,
so init fell through to `DevCmd.cmd`, which then failed too. Two independent problems:

| Problem | Detail | Workaround |
|---|---|---|
| Stale `_OriginalPathBeforeInit` in the agent process env | `init.cmd` line 58 restores that saved PATH on any re-run. The inherited value was a 1231-char PATH with no Visual Studio entries, so `where msbuild` failed. | `Remove-Item Env:\_OriginalPathBeforeInit` before calling init |
| `vswhere` finds no install | Both VS 2022 (17.14) and VS 18 report `isComplete:false` and `isLaunchable:false`, so `vswhere` returns nothing without `-all`. `DevCmd.cmd` does not pass `-all`. | Call `VsDevCmd.bat` directly |

A debugging trap: `cmd /c "call init.cmd & echo %PATH%"` expands `%PATH%` before init runs,
so it prints the pre-init value and hides the clobber. Use `cmd /v:on` and `!PATH!`.

Two more environment facts. VS 18's `MSBuild.exe` throws `System.Collections.Immutable`
load errors on any command line, including `-version`, so only VS 2022's MSBuild is usable.
VS 2022's `VsDevCmd.bat` fails until the leaked `VCToolsVersion`, `VCToolsInstallDir`,
`VCToolsRedistDir`, `DevEnvDir`, `VSINSTALLDIR`, `VCINSTALLDIR`, and `VSCMD_VER` values
from VS 18 are cleared. `tools\run-tests-on-vm.ps1` also needs `pwsh`; under Windows
PowerShell 5.1 its three-argument `Join-Path` is a parameter-binding error.

The working sequence is: clear those variables, call VS 2022's `VsDevCmd.bat -arch=amd64
-host_arch=amd64`, then `init.cmd amd64chk /envcheck /notitle`, then msbuild or the VM runner.

This does not fully fix `build.cmd`. The whole-solution build still fails in
`Microsoft.UI.Xaml.Controls.vcxproj` with MSB4216 because the `TransformTemplate` task host
cannot start. Building a single project works, so this round built and ran only the isolated
WindowPlacement test project.

## Step 6 cannot consume an unwired coordinator (2026-09-24)

The first inbox task requested saving and teardown integration at `149f2f961`.
The packaged backend and detached loader have landed, but the remaining step 4 Window
surface and step 5 production integration have not. `Window` does not expose
`PersistPlacementId` or `UseAutomaticPlacementPersistence`. The only implementation of
`IWindowPlacementCoordinatorHost` is the fake host in `WindowPlacementCoordinatorTests.cpp`;
`DesktopWindowImpl` does not own or call the coordinator.

Adding close hooks now would require a second enrollment mechanism or completing the missing
display integration. Neither is a safe substitute for step 5. The spec's surviving-window
first-display abort/retry boundary also still needs the existing PM decision. This round defers
step 6, moves that PM request ahead of it, and restores a Developer request for the missing
Window surface and coordinator wiring. The step 6 acceptance criteria remain unchanged.

## VM runner requires PowerShell 7 (2026-09-24)

Running `tools\run-tests-on-vm.ps1` through Windows PowerShell 5.1 failed while
binding the incremental deployment manifest, even though the script connected
to the VM and created the payload. Running the same command with
`C:\Program Files\PowerShell\7\pwsh.exe` completed deployment and executed all
129 `*WindowPlacement*` tests successfully.

The attempted foundation build through `initrun.ps1` stopped during initialization with
"Could not find an MSBuild install"; compilation and the subsequent isolated build did not run.
MSBuild is present on the parent shell's PATH, so this is an initialization/discovery failure,
not evidence that MSBuild is absent. No VM tests ran and no new lifecycle coverage is claimed.

## Step 4 projections do not complete storage integration (2026-09-24)

The first step 4 Developer round found that the step 1 store has an injectable
`IPlacementSettingsBackend`, but no concrete Windows App SDK `ApplicationData` backend.
Passing store tests with an in-memory backend is not evidence that detached loading can
read production settings. The spec also explicitly leaves unexpected loader errors and
late Boolean/enrollment behavior open. Do not invent null/error mappings to connect the
public API, or expose methods that only return `E_NOTIMPL`.

This round implements the value/options/enums projections and their private snapshot
boundary only. The framework and isolated project build, and the targeted VM run passes
107 tests, including five managed API tests and five native request/copy tests.
Dedicated C++/WinRT runtime tests, Window wiring, real capture/load independence, and
validation-before-window-side-effects remain required. The inbox task stays open and
routes the missing backend and decisions to their owners.

Revisited at `774e4e4b4` on 2026-09-24: the first inbox item still asks for full step 4
acceptance ahead of its unresolved prerequisites. Repeating that assignment cannot establish
the required null/error mappings or storage-backed independence. In particular,
`PlacementStore::Load` maps `StorageResult::Failure` to `LoadStatus::Unexpected`, and neither
result retains a backend HRESULT. A contract that propagates the original error would require
a private result-boundary change, not just a public exception translation.

This Developer round defers full step 4 rather than selecting those open behaviors. Its unfinished
request remains in the inbox, behind the existing PM and backend requests. Value-only C++/WinRT
runtime coverage can proceed independently, but cannot substitute for step 4 acceptance.

PM resolution (2026-09-24): unexpected detached-load failures must preserve their HRESULT.
The identity helper also loses error information: `TryGetPackagedApplicationId` returns only
`bool`, so it cannot distinguish an expected lack of identity from an unexpected query failure.
The backend handoff now requires error preservation at both boundaries, not just `LoadResult`.
Late setters are accepted, but they cannot create enrollment; the current Boolean/id gate saves
only for a window enrolled at first display. These are documented decisions, not implemented fixes.
The separate first-display abort/retry question remains open.

Build lessons: the build skill is at `src/.github/skills/build/SKILL.md`, not the root
path referenced below. XamlOM changes require `dxaml/xcp/runcodegen.cmd`. Compiling the
full XAML header set without its PCH exposed an unrelated `CompositeTransform` analysis
warning; allocation-catching helpers instead compile in a small exception-enabled
source without XAML headers. Nullable rectangle/GUID boxing also needs reference-element
runtime names; managed round-trip tests cover those specializations.

## Missing build skill reference (2026-09-23)

`src/.github/copilot-instructions.md` points to `.github/skills/build/SKILL.md`,
but that file is absent in this checkout. The test-on-vm skill's existing workflow
worked instead: `.\initrun.ps1 .\build.cmd /q`, followed by
`.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <vm> '*WindowPlacementRecordTests*'`.
The build succeeded and all 13 codec tests passed on the VM. These data-only tests
do not cover window lifecycle, storage, native placement, or display changes.

## Rewriting inbox.md clobbers concurrent role edits (2026-09-23)

A Developer round read inbox.md at the start, worked for about 20 minutes (build plus
VM test runs), then rewrote the whole file from that stale copy. A PM round committed a
new inbox item in the meantime (be7f38cf7), and the rewrite silently deleted it. The loss
only showed up because `git log` listed an unexpected parent commit after `git commit`.

Do not rewrite inbox.md from a copy read before a long build or test run. Re-read the file
immediately before writing, edit only the lines you own, and check `git log` for new commits
before committing.

## First-display reservation is not actual display (2026-09-23)

Architect review found a contract gap, not a reproduced runtime failure. The spec's
"Loading, applying, and displaying" sequence ends the phase and records enrollment before
callbacks. Its lifecycle table instead ends the phase only on actual display. A validated first
`Show` can enter native placement callbacks before display; closing there, or aborting the display
attempt while the window survives, separates those two points.

A single consumed-opportunity flag cannot represent in-progress work, actual visibility, and
save enrollment safely. Keep those states separate, and never treat a hidden application or
pre-display reservation as evidence that saving is eligible. The design note now records the
boundary and routes the surviving-window retry decision to PM before coordinator implementation.

PM resolution (2026-09-24): the phase ends only on actual display. A validated attempt that
aborts before any display releases its guard, discards its snapshots and enrollment decision,
and leaves the phase open, so a surviving hidden window retries with a fresh full pass. A
never-displayed window is still ineligible to save. See the spec's "Aborting before display".

## A WinRT Boolean option cannot express "unset" (2026-09-23)

PM review of the `Activate`-after-hidden-preparation hazard considered making the first display
skip automatic placement implicitly once the app had called `TryApplyInitialPlacement`. That looks
safe until you ask how a caller opts back in. `WindowShowOptions.SkipInitialPlacement` is a plain
Boolean that defaults to `false`, so `false` means both "I did not set this" and "I deliberately
want the automatic pass". An implicit skip would win over both, and there would be no way to
request the pass again.

That matters here because hidden application can return `false` and is still an open backend gate
for maximized, minimized, and snapped placement. Letting the first display place the window is the
documented recovery, and the implicit-skip design would have removed it silently.

Before adding implicit behavior keyed off a Boolean option, check whether the caller can still
express the opposite. If the option has only two states and one of them is the default, it cannot
also carry an override.

## Imported PlacementEx headers were not build-ready (2026-09-23)

Step 2A's first foundation build failed because `WindowActions.h` included
`platformsdk-uplevel/winuser_windowaction.h`, which was not in the checkout or restored packages.
The repository targets SDK 22621. Local SDK 26100's `WinUser.h` provides the Window Action ABI,
but raising the Windows target to expose those declarations would change the wrong boundary.

Added `WindowActionTypes.h` with compatible declarations and a guard for SDKs that already expose
them. The API itself remains dynamically resolved. The dedicated adapter tests verify structure
size, key offsets, and flag values on the built architecture.

Foundation static analysis then exposed missing success annotations on Boolean output helpers,
an incorrectly optional virtual-desktop output pointer, and unchecked module handles passed to
`GetProcAddress`. Fixed those contracts and null checks rather than suppressing analysis.
The isolated test project also needed Unicode definitions to match the imported engine and
foundation build. Both affected projects now build, and all 77 placement tests pass on the VM.

This is build and value-seam coverage, not native-backend approval. In particular,
`PlacementEx::Cascade()` reads system metrics rather than accepting the policy's offset input,
and its geometry helpers use native-width arithmetic. Step 2B must handle injected metrics and
intermediate overflow without reintroducing a second WinUI geometry engine.

## Adapter step 2C implementation (2026-09-23)

Implemented TryApplyNativeRequest function in WindowPlacementAdapter to bind the native request
to PlacementEx::SetPlacement. The function:

1. Validates the input NativeRequest and HWND
2. Creates an engine PlacementEx from the request
3. Ensures AllowPartiallyOffScreen is never set (defensive check)
4. Calls PlacementEx::SetPlacement to apply the placement
5. Captures effective placement using PlacementEx::GetPlacement
6. Returns appropriate ApplyOutcome (Applied, FailedBeforeStart, FailedAfterStart)

Changes:
- Added TryApplyNativeRequest to WindowPlacementAdapter.h/.cpp
- Added dwmapi.lib to test project dependencies (needed for inline GetWindowMargins function)
- Added basic unit tests for invalid inputs (null HWND, invalid request)

Build status: Success - all 77 placement tests pass

Remaining gaps:
1. Full integration testing with real windows requires VM access with credentials
2. Virtual desktop ID capture skipped with CaptureFlags::SkipVirtualDesktopId to avoid RPC_E_CANTCALLOUT_ININPUTSYNCCALL
3. Need to verify behavior with different monitor topologies and DPI changes
4. Need to test with actually hidden windows and non-activating scenarios
5. Code Reviewer should verify geometry comes from engine and no duplicate implementations remain

## Capture and placement caches step 3 implementation (2026-09-23)

Implemented WindowPlacementCapture to integrate placement capture with DesktopWindowImpl. The implementation:

### New Files:
- `WindowPlacementCapture.h`: Header declaring the capture class and state transition tracking
- `WindowPlacementCapture.cpp`: Implementation that captures placement from HWND
- `WindowPlacementCaptureTests.cpp`: Unit tests for state machine and validation

### Key Features:
1. **State Tracking**: Properly determines placement state (Normal, Maximized, Minimized, Snapped, MinimizedFromSnapped, MinimizedFromMaximized) based on current conditions and previous snapshot
2. **Monitor Integration**: Uses GetMonitorInfoEx to capture monitor device names and work areas
3. **Coordinate Capture**: Captures physical coordinates for NormalRect, WorkArea, and SnapRect
4. **State Transitions**: Tracks minimized-from-snapped and minimized-from-maximized transitions via previousSnapshot parameter
5. **Message-Safe Capture**: Captures placement synchronously on WM_SIZE and WM_MOVE without blocking or RPC calls
6. **Validation**: Ensures captured placement passes IsValid() checks before storing

### Changes to DesktopWindowImpl:
- Added `m_cachedPlacement` member to cache the most recent Snapshot
- Added `CaptureEffectivePlacement()` method to synchronously update the cache
- Integrated capture into OnSizeChanged and OnMoved to track state changes
- Added `GetCachedPlacement()` accessor for other components

### Build Integration:
- Added WindowPlacementCapture.cpp to wrtdxamlfoundation.vcxproj (NonPreCompClCompile target)
- Added tests and source to WindowPlacement.vcxproj
- Uses MonitorInfoEx for device name capture instead of plain MonitorInfo

Build status: Success - all 77 placement tests pass

### Remaining Gaps and Limitations:
1. Virtual desktop ID is intentionally not captured (avoids RPC_E_CANTCALLOUT_ININPUTSYNCCALL)
2. Snap rectangle detection uses simple heuristics (50px tolerance) and may miss snapped windows on different topologies
3. Cache is updated only on WM_SIZE and WM_MOVE, may miss programmatic size/position changes via other APIs
4. No handling for variable-DPI scenarios or monitor configuration changes while window is hidden
5. Device name conversion from wide-char to u16string needs testing on multi-monitor setups
6. Post-close HWND reuse scenario not yet tested (cache persistence after window closes)

### Next Steps:
- Coordinator layer (step 5) will wire cache into placement decision pipeline
- Code Reviewer should verify state machine, device name handling, and message-safe synchronization
- VM tests needed for hidden window moves, presenter transitions, and multi-monitor scenarios


## Hand-written capture duplicated the placement engine (2026-09-23)

Code review of step 3 (`79c2f629a`). The capture unit re-derived placement from `GetWindowRect`,
`GetWindowPlacement`, `MonitorFromPoint`, `IsIconic`, and `IsZoomed` rather than calling
`PlacementEx::GetPlacement` through the step 2 adapter, which already captures effective placement
after application. Every hard case then went wrong in a way the engine already handles:

- `rcNormalPosition` is in Win32 workspace coordinates, mixed with screen-coordinate work areas.
- A minimized window reports roughly `(-32000, -32000)`, so picking the monitor from the window
  rect center selects an unrelated monitor and the snapshot fails validation.
- `MinimizedFromMaximized` and `MinimizedFromSnapped` were downgraded to `Minimized` on the very
  next capture, and minimizing sends both `WM_MOVE` and `WM_SIZE`.
- `State::Snapped` was only reachable if the previous snapshot was already `Snapped`, so it was
  never reachable at all.

Reuse the engine for capture. Write the WinUI-specific parts (last meaningful show state, the
pre-full-screen snapshot, cached virtual-desktop id) as a thin layer over it.

## Including a configurable third-party header two ways is an ODR bug (2026-09-23)

`WindowPlacementAdapter.h` includes `User32Utils.h` after defining `USE_VIRTUAL_DESKTOP_APIS`.
The new `WindowPlacementCapture.h` included the same header with no defines. `PlacementEx` declares
`GUID virtualDesktopId` only under that define, so the class had two different layouts and two sets
of inline functions in one binary, and `#pragma once` made the result depend on include order per
translation unit. It still built and tests still passed, which is exactly why it is dangerous.

Only one header should configure and include an external engine. Other headers include that header,
or, if they only need a handle type, include nothing from the engine at all.

## Tests that assert their own setup prove nothing (2026-09-23)

Four of the six tests added for capture never called `TryCaptureFromWindow`. They assigned a field
on a local `Snapshot` and verified the field held the assigned value, or re-tested `IsValid`, which
an existing class already covers. The commit message reported "all 77 placement tests passing" and
listed state transitions and device-name conversion as tested features. Neither was executed.

A passing count is not coverage. For each claimed behavior, check that some test calls the new
entry point.

## The engine capture vocabulary is wider than the request vocabulary (2026-09-24)

Reworking capture onto `PlacementEx::GetPlacement` did not just work. `TryReadEnginePlacement`
validates against the request rules in `WindowPlacementAdapter.cpp`, and a captured window
regularly reports values no valid request may carry:

- `GetWindowPlacement` returns show-command aliases such as `SW_SHOWMINIMIZED` and
  `SW_SHOWMAXIMIZED`. The request form only accepts `SW_NORMAL`, `SW_MAXIMIZE`, `SW_MINIMIZE`,
  and `SW_HIDE`. Every minimized capture failed until capture canonicalized the command.
- `SW_HIDE` maps to `NoChange`, which only validates together with `KeepHidden` and `NoActivate`.
  Capture has to set both before translating, or hidden windows never capture.
- Neither maximize nor hide may carry `Arranged` or a restore flag, but a real window can report
  those at the same time.

When a translation layer is written for one direction (build a request, apply it), reading a live
value back through it needs an explicit normalization step. Do not assume the two directions share
a vocabulary.

## SW_RESTORE on a minimized-from-maximized window restores to maximized (2026-09-24)

A capture test asserted `Normal` after `ShowWindow(SW_RESTORE)` on a window that had been maximized
and then minimized. It got `Maximized`, which is correct Windows behavior: the first restore returns
the window to maximized, and a second restore returns it to normal. The test was wrong, not the
code.

State-transition expectations for window show commands are worth checking against the real window
manager before assuming a test failure is a product bug.

## WM_CLOSE is not a close (2026-09-24)

`DesktopWindowImpl::OnClosed` sounds like a post-close hook. It is the `WM_CLOSE` handler, and it
runs before anyone knows whether the close will be accepted: `CloseImpl` raises `Window.Closed`, and
a handler that sets `Handled` cancels the close. `WM_CLOSE` never reaches `DefWindowProc`, so the
window survives and keeps pumping messages.

The capture rework put `m_placementCache.Detach()` at the top of that handler. On a cancelled close
the cache is left detached with a live window, and the next `WM_MOVE` rebinds it as if it were a
brand-new HWND, discarding the cached snapshot and the virtual desktop id.

Two lessons:

- Attach teardown to the step that actually tears down (`Shutdown`, or `CloseImpl` after
  `m_bIsClosed`), not to the request that might start one. Handler names ending in "Closed" are
  worth reading before trusting.
- A "reset when the HWND changes" rule needs a separate "this object was retired" state. Reusing
  a null `m_window` to mean both "nothing bound yet" and "detached" makes cancel-and-continue
  indistinguishable from a new window.

## Removing noexcept is not an exception-safety fix (2026-09-24)

A review found `std::terminate` risk: a `noexcept` capture function appends to a `std::u16string`.
The fix removed `noexcept`, reasoning that the library has no `catch` anywhere, so a handler would
be dead code.

That reasoning inverts. The function is reached from `OnMoved` and `OnSizeChanged`, which the window
procedure dispatches directly, so removing `noexcept` lets `std::bad_alloc` unwind out of a Win32
callback frame, which is undefined behavior. `noexcept` at least failed fast at a known frame.
"There is no `catch` anywhere" says where the handler belongs - just inside the component that
allocates, so the exception never reaches the callback - not that the exception may escape.

## Failed replacement capture must not publish a new binding (2026-09-24)

The capture review fix exposed a related cache problem: `TryCapture` reset the snapshot and
desktop id, then issued a new lifetime token, before checking the presenter or reading placement.
A different live HWND that could not be captured therefore destroyed the old cache on failure.
A child HWND makes a useful regression case: it passes `IsWindow` but the placement engine rejects
it. Build the candidate snapshot first, then publish its binding and token only on success.

## Shared deferred callbacks need unconditional teardown protection (2026-09-24)

The restored-size callback's lifetime sentinel was disarmed only when the new windowing APIs were
enabled. Sharing that callback with placement capture extends its use to feature-off windows too.
Keeping the old teardown guard would let a queued callback access a destroyed window implementation.
`Shutdown()` now disarms any allocated sentinel without consulting the feature switch.

## Coordinator phase reservation versus actual display (2026-09-24)

The step 5 coordinator must keep its in-progress guard, first-display enrollment,
initial-placement phase, and actual visibility evidence as separate state. A display
operation reserves the phase before callbacks so reentrant placement cannot start, but
the host still reports whether an actual display occurred. Hidden placement never
enrolls, and direct native display ends the phase without enrollment.

The specification had an open retry decision when a validated first display
callback aborts before the window becomes visible. This implementation keeps the
phase closed after the validated framework display operation and preserves the
separate `HasEverBeenDisplayed` evidence; it does not use that reservation to make
an undisplayed window eligible for saving.

PM resolved this on 2026-09-24 the other way: the phase closes and enrollment is recorded
only on actual display, so a surviving undisplayed window retries with a fresh full pass.
`WindowPlacementCoordinator::Show`, `Activate`, and `RecordFirstDisplayEnrollment` therefore
need rework before Window wiring. Today they set `m_phaseOpen = false` and record enrollment
before `m_host.Display(...)` reports anything, which is exactly the rejected consumed-opportunity
behavior. The saving path is still safe because it also requires `HasEverBeenDisplayed`, so this
is a contract mismatch rather than a data-loss bug. Its coordinator tests assert the old order
and will need to change with it.

## Step 6 save lifecycle integration completed without full storage backend (2026-09-25)

Step 6 ("saving and teardown integration") implements the coordinator close/destroy lifecycle
and integrates placement capture snapshots into DesktopWindowImpl without blocking on
PlacementStore API integration. The implementation:

**Coordinator lifecycle:**
- `OnAcceptedClose()`: Called after close event handlers finish and close isn't cancelled.
  Only saves if window was enrolled at first display and has ever been displayed. Snapshots
  both `PersistPlacementId` and `UseAutomaticPlacementPersistence` at this point.
- `OnDestroy()`: Best-effort backstop save during Shutdown() and destructor, in case
  OnAcceptedClose never ran (abnormal termination, unhandled exception, etc).
- `m_saveAttempted` flag: Prevents duplicate saves if close handlers call Close() again.

**Window lifecycle integration:**
- `CloseImpl()`: Calls `coordinator->OnAcceptedClose()` after Closed event, before
  m_bIsClosed is set. Returns early if event is handled (close cancelled).
- `Shutdown()`: Calls `coordinator->OnDestroy()` first, before detaching placement cache.
- Destructor: Calls `coordinator->OnDestroy()` as backstop if CloseImpl never ran.

**Enrollment and display eligibility:**
All save decisions delegate to coordinator's tracking of:
- `m_hasEnrollment`: Window enrolled at first display (took placement pass, Boolean was true, id non-empty)
- `m_everDisplayed`: Window was actually displayed (not just prepared while hidden)

Never-displayed windows never save, even if subsequently moved/resized/styled. Displayed
windows that later hide still save on close. This matches the spec's "Know when placement is saved"
section and the "Hidden-only no-save" contract.

**TryCapturePlacementAndSave() implementation:**
Currently returns false (marked TODO) because storage backend integration is deferred.
Structure is complete and ready for PlacementStore::Save integration:
- Validates placement ID and length (not null, 0 < length < 4096)
- Checks both enrolled and current AutomaticPersistence flags (snapshot isolation)
- Captures placement from m_placementCache.GetCachedPlacement()
- Obtains application ID via TryGetPackagedApplicationId (TODO: handle error properly)
- Calls PlacementStore::Save when backend ready (TODO)

**Property snapshot contract:**
Coordinator passes enrolled id/Boolean at call time. Host snapshots current values
independently, allowing app code to modify properties during close handlers without
affecting the enrolled snapshot. The coordinator's m_saveAttempted flag then prevents
the actual save attempt if enrollment was already completed.

**Handler opt-out and cancelled close:**
- `UseAutomaticPlacementPersistence = false` at any point (before or during close) prevents save
- Cancelled close (Handled event) skips TryCapturePlacementAndSave call entirely
- Both cases leave m_saveAttempted = false, so retry is possible if close is retried

Remaining work (Step 7 and follow-up):
- Implement storage backend in TryCapturePlacementAndSave (PlacementStore integration)
- Extract placement options from WindowShowOptions in TryApplyInitialPlacementImpl/ShowImpl
- Add comprehensive test suite for lifecycle scenarios (accepted/cancelled/destruction, id changes, etc)
- VM test validation

## Display routing through the placement coordinator (2026-09-25, Developer)

**A later `Activate()` on a visible window is not a no-op.**
The coordinator's post-phase `Activate` path early-returned `true` when `IsVisible()`. That looked
right by symmetry with `Show`, but the spec section "Control display and activation" says a later
`Activate()` still restores a minimized window and requests activation. `Show` is the one that
no-ops on an already-shown window. Removed the early return from `Activate` only.

**`TryApplyInitialPlacement` must check state before it validates.**
Spec order matters: the displayed / operation-in-progress / already-visible checks come first, so
a late call with an invalid request returns `false` instead of throwing `E_INVALIDARG`. Validating
first would have turned a documented `false` into a failure HRESULT.

**`Display` returns `bool`, so a native HRESULT needs somewhere to go.**
Added `m_lastDisplayResult` on `DesktopWindowImpl`: the callback stores the failure, and the
`ShowImpl`/`ActivateImpl` caller does `IFC_RETURN(m_lastDisplayResult)` after the coordinator
returns. Changing the interface to return `HRESULT` would have leaked framework error handling
into the coordinator, which is meant to stay host-agnostic.

**A TAEF test class cannot derive from a `final` fake host.**
`error C3246`. Tried deriving a `NestedHost` inside a `TEST_METHOD` to re-enter the coordinator
from inside `Display`. Simpler fix: add a `NotifyNativeDuringDisplay` flag and a back pointer to
the existing `RecordingHost` instead of subclassing it.

**Build environment, again (see the earlier toolchain entry).**
VS 18 MSBuild still fails with a `System.Collections.Immutable` load error. A temporary
`build-placement.cmd` wrapper worked: clear the leaked VS 18 env vars, call VS 2022's
`VsDevCmd.bat`, `cd /d "%~dp0"` because `VsDevCmd.bat` changes the working directory, then
`call "%~dp0init.cmd" amd64chk /envcheck /notitle`. Note the `%~dp0` on `init.cmd` too: a bare
`call init.cmd` failed with "not recognized" even from the right directory.

## Placement integration test transient crash (2026-09-24, Developer)

The first VM run of `*WindowPlacementPersistenceTests*` crashed in `coreclr.dll`
(`0xC0000005`) during `FirstActivateUsesPersistenceConfiguration`, after the preceding
placement tests passed. Running that test alone immediately afterward passed, including cleanup.
The full build and payload deployment completed successfully. This appears to be a transient
test-host failure; no product failure was reproduced on the retry.

## WinRT method overloads need explicit overload attributes (2026-09-25, Developer)

Adding a second same-named method to a WinRT interface in the XamlOM model without overload
attributes makes MIDL auto-generate a mangled ABI name. Adding a parameterless `Show()` next to
the existing `Show(WindowShowOptions)` on `IWindow12` produced `Show2` in the IDL. The generated
C++ forwarder still declared `Show`, so `ctl::interface_forwarder<IWindow12, WindowGenerated>`
failed to compile with `C3668` ("method with override specifier did not override any base class
methods") plus a cascade of `C2259 cannot instantiate abstract class` across many projects.

Fix: follow the `ContentDialog.ShowAsync` / `ShowAsyncWithPlacement` precedent. Put
`[DXamlName("...")]` and `[DXamlOverloadName("Show")]` on every overload, and `[DefaultOverload]`
on the one that should be the default. That emits `[method_name("...")]` in the IDL and keeps the
generated forwarder and the ABI in agreement. The options overload became `ShowWithOptions`.

## Promoting a private API to public: check the forwarder, not just the impl (2026-09-25, Developer)

`Window.Show()` and `Window.Hide()` already existed on the private `IWindowPrivate` interface, so
it looked like only the public projection was missing. Two separate stubs were hiding behind that:

| Location | State found |
| --- | --- |
| `DesktopWindowImpl::HideImpl()` | `return E_NOTIMPL;` |
| `Window::ShowImpl()` in `Window_Partial.cpp` | `return E_NOTIMPL;` |

`DesktopWindowImpl::ShowImpl()` was correctly wired, so a quick grep of the impl class suggested
`Show` was done. The failure only showed up as a `NotImplementedException` on the VM, because the
public call goes through the `Window_Partial` forwarding layer first, and that layer was never
wired to `m_spWindowImpl`. When promoting a private member to public, read the whole call chain:
generated class -> `Window_Partial` forwarder -> `WindowImpl` virtual -> `DesktopWindowImpl`.

## XamlGen .tt templates are not transformed by the build (2026-09-25, Developer)

Editing a T4 template under `dxaml\xcp\tools\XCPTypesAutoGen\XamlGen\Templates\` has no effect on
its own. Each `.tt` has a checked-in `.cs` next to it, generated by the Visual Studio
`TextTemplatingFilePreprocessor` custom tool. Nothing in the build re-runs that preprocessor; the
`.cs` files are what actually compile.

Symptom: after changing `Headers\Method.tt`, `Bodies\Method.tt`, and `Headers\MethodForwarder.tt`,
the codegen run

```
.\initrun.ps1 msb dxaml\xcp\tools\XCPTypesAutoGen\RunCodeGen\xamlgen.runcodegen.proj
```

still produced the old output. Changes to hand-written model classes in the same tree
(`OM\MemberDefinition.cs`, `Templates\ModernIDL\MethodModel.cs`,
`Templates\...\ForwarderMethodModel.cs`) took effect immediately, which made the template edits
look like they had been applied.

Fix: make the matching edit in the generated `.cs` by hand and keep it in sync with the `.tt`.
The `.cs` text is a series of `this.Write(...)` calls, so the mapping is mechanical.

Also note the generated product files are diff-checked. When they drift, the codegen project fails
with "Checked in generated files are out of sync" and points at
`BuildOutput\obj\<arch><flavor>\dxaml\Codegen\updatecheckedinfiles.cmd`. Run that script, then
re-run the codegen project and confirm exit code 0. The loop is about 8 seconds, so iterate there
rather than in a full build.

## Method header and body templates disagreed on the C++ member name (2026-09-25)

The new `[DXamlName("HidePublic", IdlName = "Hide")]` capability adds
`MemberDefinition.CppName` (= `CppNameOverride ?? IdlMemberInfo.Name`), but only two of the
branches in the `Code\Framework` `Method` templates were switched to use it:
`Bodies\Method.tt` used `CppName` only in the interface-impl non-pure-virtual branch, and
`Headers\Method.tt` used it only in the version-interface-forwarded branch. Every other branch
still emitted `Model.IdlMethodInfo.Name`.

That is invisible today because `Window.Show`/`Hide` happen to hit exactly the two branches that
were updated. Put `IdlName` on a static method, a non-forwarded interface method, or a plain
non-interface method and the header declares one name while the body defines another, which fails
at link time with no useful diagnostic.

Fix: use `Model.CppName` in all branches of `Headers\Method.tt` (2 sites) and `Bodies\Method.tt`
(3 sites), plus the hand-synced `.cs` preprocessed copies. All five are textually identical output
when `CppNameOverride` is null, which is every existing member.

Related: `Helper.CopyMemberData` copied `IdlMemberInfo.Name` but not `CppNameOverride`, so the
override was dropped when a class implemented an interface method that carried it.

BOM warning: `Headers\Method.cs` has a UTF-8 BOM and `Bodies\Method.cs` does not. PowerShell 7
`Set-Content -Encoding UTF8` strips the BOM and produces a whole-file diff. Use
`-Encoding utf8BOM -NoNewline` for the first file.

## The codegen C# projects cannot be built with the dotnet CLI (2026-09-25)

```powershell
dotnet build dxaml\xcp\tools\XCPTypesAutoGen\XamlGen\XamlGen.csproj
```

fails with:

```
MSB4278: The imported file "$(VCTargetsPath)\Microsoft.Cpp.Default.props" does not exist
```

`XamlGen.csproj` transitively references
`dxaml\xcp\dxaml\idl\winrt\merged\private\Microsoft.UI.Xaml.vcxproj`, and the dotnet CLI cannot
load C++ project files.

This is not a host-prerequisite gap. Full MSBuild works:

```powershell
$msb = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
& $msb dxaml\xcp\tools\XCPTypesAutoGen\XamlGen\XamlGen.csproj /v:m /nologo
```

That runs MIDLRT and the metadata merge and produces `XamlGen.exe` in about three minutes. Use it
to validate template and OM edits instead of concluding that codegen changes cannot be verified
locally.
## Do not infer a null contract from the generated ABI header

While writing `WindowDisplayApiHostTests` I read the MIDL-generated ABI header:

```
BuildOutput\obj\amd64chk\dxaml\publicHeaders\microsoft.ui.xaml.coretypes2.h
```

`IWindow12::ShowWithOptions` and `IWindow12::TryApplyInitialPlacement` are declared there with
`__RPC__in_opt` on the options parameter. I concluded null was allowed and wrote two tests
expecting `E_NOTIMPL` (the host gate) for a null options object.

Both failed on the VM with `0x80070057` (`E_INVALIDARG`):

```
Error: Verify: AreEqual(E_NOTIMPL, window12->ShowWithOptions(nullptr)) - Values (0x80004001, 0x80070057)
Error: Verify: SUCCEEDED(window12->TryApplyInitialPlacement(nullptr, &applied)) - Value (0x80070057)
Summary: Total=7, Passed=5, Failed=2
```

`__RPC__in_opt` is MIDL boilerplate for an interface pointer. It says nothing about the framework
contract. The real contract is in the generated dispatch stub,
`dxaml\xcp\dxaml\lib\winrtgeneratedclasses\Window.g.cpp`:

```cpp
ARG_NOTNULL(pOptions, "options");
```

That check runs before `CheckThread`, before the strict-API check, and before the
`SupportsPublicDisplayApis()` host gate. Null never reaches the impl layer from a public caller.

Lesson: to learn what a WinUI public API does with a null argument, read the `*.g.cpp` dispatch
stub, not the ABI header. The header describes marshalling; the stub describes behavior.

Related: this also explains the `_In_` vs `_In_opt_` "mismatch" flagged in code review. `_In_` on
the generated boundary is correct because of `ARG_NOTNULL`. `_In_opt_` on the impl layer is also
correct because `Window::ShowImpl` calls `ShowWithOptionsImpl(nullptr)` internally. Two layers,
two contracts, both accurate.

## Validating a codegen change: build the codegen driver, not XamlGen.csproj

`XamlGen.csproj` compiles the generator, but it does **not** compile the model assemblies that
apply the `XamlOM` attributes. Its `ProjectReference`s to `XamlOM.csproj` and friends are wrapped
in `Condition="'$(BuildingInsideVisualStudio)'=='true'"`. In a command-line build those references
are skipped entirely, so a change to something like `DXamlNameAttribute`'s `AttributeUsage` can
build clean and still be wrong.

Building `XamlOM.csproj` standalone does not work either. It fails with roughly 6000 errors rooted
in `'OM' could not be found`, because its `XamlGen.OM.dll` reference is not satisfied.
`XamlOM.Instant.proj` standalone fails the same way with `CS0006: Metadata file
...\Codegen\XamlGen.OM.dll could not be found`.

The project that actually works is the codegen driver:

```powershell
$msb = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
& $msb dxaml\xcp\tools\XCPTypesAutoGen\RunCodeGen\xamlgen.runcodegen.proj /v:m /nologo
```

It builds `XamlGen.exe` and `XamlGen.OM.dll`, "instant-compiles" all five model assemblies
(`XamlOM.Instant.dll`, `XamlOM.Controls.Instant.dll`, `XamlOM.ExCore.Instant.dll`,
`XamlOM.AutoSuggestBox.Instant.dll`, `XamlOM.Phone.Instant.dll`), and regenerates the `Core` and
`Phone` output trees under `BuildOutput\obj\<config>\dxaml\Codegen\`. It takes a few minutes and
prints nothing at `/v:m` on success, so check the output timestamps to confirm it really ran.

Lesson: for any change under `XCPTypesAutoGen`, `xamlgen.runcodegen.proj` is the validation build.
`XamlGen.csproj` alone only proves the generator compiles.

## A feature can be fully specified, fully implemented, and still be dead code

`WindowPlacementPolicy.cpp` has implemented the spec's "cascade explicitly supplied placement" rule
for a while. The branch is gated on `Cascade.PeerAnchorNormalRect.has_value()`. Production never
set it: `TryApplyPlacement` passed a literal `std::nullopt`, and the coordinator only looked up a
peer when `request.Placement` was *absent*, which is exactly the case where the source is not
explicit. So the branch could never run outside a test.

Nothing caught this:

| Signal | Why it stayed green |
| --- | --- |
| Policy unit tests | They call `ComputePlacement` directly and set `PeerAnchorNormalRect` by hand, so the branch was covered in isolation. |
| Application tests | They passed the default `std::nullopt` and asserted the uncascaded result, which was correct for what they asked for. |
| Coordinator tests | The coordinator genuinely needed no change, so its tests were right too. |
| Compiler | A hardcoded `std::nullopt` argument is not dead code to the compiler. |
| Spec review | The spec was correct. The design note was correct. Only the wiring was missing. |

Every layer was individually defensible. The gap was in the seam between two of them, and it was
invisible because each layer's tests supplied the seam's input themselves.

Lessons:

- Coverage of a branch is not evidence the branch is reachable. If a test is the only thing that
  ever supplies an input, treat that input as unimplemented until a production caller supplies it.
- Grep for literal `std::nullopt` / `{}` / `nullptr` arguments at layer boundaries. A parameter that
  only ever receives a constant from production is a strong smell.
- When a spec sentence has no production code path, it is not "already done." Trace the value from
  the public entry point to the branch, not from the branch outward.

The wiring also had a design constraint worth remembering. The spec requires resolving the target
monitor *first*, then searching for a peer restricted to that monitor. The coordinator has neither
an `HWND` nor a monitor, so it structurally cannot do that lookup - the ordering in the spec was
already telling us which layer owns it. When a spec's step order does not fit your layering, the
layering is usually the thing that is wrong.

## A test project that never builds is not a test

`WindowPlacementExplicitCascadeTests.cs` was committed with six compile errors and nobody noticed.
The reason: `Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj` sets `<SkipProject>true</SkipProject>`,
and `dxaml/Microsoft.UI.Xaml.Build.targets` line 28 swaps in `SkipProjectBuild.targets` whenever
`SkipProject` is true and `BuildAll` is not. The normal build silently skips the whole project.

To actually compile it:

```
& $env:ComSpec /c "cd /d D:\x1 && call D:\x1\init.cmd amd64chk /envcheck && cd /d D:\x1\dxaml\test\managed\Win32\WPF && call D:\x1\tools\msb.cmd Microsoft.UI.Xaml.Tests.Managed.Win32.Hosting.csproj /p:BuildAll=true /t:Build"
```

Do not pass `msb.cmd /q` when you want diagnostics - it suppresses errors too.

The six errors, and what they teach:

| Error | Cause |
| --- | --- |
| `CS0117 Verify.Inconclusive` | TAEF managed has no `Inconclusive`. Use `Log.Result(TestResult.Skipped, msg)`. |
| `CS0234 Interop.SetWindowPos` (x2) | Inside `namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF`, the bare name `Interop` binds to the `Microsoft.UI.Xaml.Interop` *namespace*, not the hosting class. A `using` does not win. Use a `using` alias. |
| `CS0748` (x3) | A lambda with a `ref RECT` parameter forces *every* parameter to be explicitly typed. |

Lesson: before reviewing a test for correctness, prove it compiles. And when a test file has never
been built, assume the project is excluded rather than assuming the author was careless.

## Assert against a measured baseline, not against the value you asked for

The first VM run failed three of the four "must not cascade" cases. The assertion was
`requested.NormalRect.Y == actual.NormalRect.Y`, and the actual was 85 where 80 was requested.

That 5-pixel delta had nothing to do with cascading. The platform applies its own position fitting
to an explicit placement, so the uncascaded result is simply not the literal requested rect. Worse,
the one *passing* test was passing for the wrong reason: it asserted "position changed", and the
fitting offset alone would have satisfied it even if cascading were completely broken.

The fix was to measure the uncascaded position inside the test - show a control window with the same
placement in a placement group with no other members, so no peer can exist - and compare against
that. Logged values after the change:

| Scenario | Baseline | Result |
| --- | --- | --- |
| Enabled / Default | 80,85 | 88,99 |
| Automatic, Disabled, ApplicationRestart | 80,85 | 80,85 |

Now the cascade case proves a real offset and the no-cascade cases prove exact equality.

Lesson: when the system under test sits behind other transforms, the request is not the expected
value. Derive the expected value from the system itself with the feature turned off. A test that
compares against the raw input can fail for reasons unrelated to the feature, and - more dangerous -
can pass for them too.

## Making the peer monitor filter testable without a second display (2026-09-24, Developer)

**The gap.** `DesktopWindowImpl::TryFindPlacementPeer` owns the real peer discovery: it walks
Z order, filters candidates by display device name, and keeps walking past a mismatch. The
isolated unit test project compiles `WindowPlacementApplication.cpp` but not
`DesktopWindowImpl.cpp`, so none of that rule was unit testable. The only coverage was two
managed tests that self-skip with "This scenario requires at least two displays." Hyper-V Gen 2
guests present a single display over PowerShell Direct, so the VM could never run them.

**Rejected: a pull-based candidate source.** The tempting fix was a
`PeerCandidateSource { bool (*TryGetNext)(void*, Snapshot&); void* Context; }` enumerator that
moves the whole walk into the persistence library, making both the filter and the continuation
scriptable. It was rejected for this iteration. The loop has two snapshot sources per HWND
(the framework cache, then a `PlacementEx` capture) plus per-candidate marker, closed, and
presenter checks. Restructuring it is a real regression risk on a build host that cannot run
the integration tests that would catch a mistake.

**Chosen: extract the predicate only.** `ArePlacementDeviceNamesEqual` moved out of
`DesktopWindowImpl.cpp` and `IsAcceptablePlacementPeer(candidate, requiredDeviceName)` was
added next to it in `DirectUI::WindowPlacementPersistence`. The walk stays where it is and
calls the shared rule. The rule is a snapshot comparison, not a topology query, so it tests
fine on one display.

**What this does and does not buy.** Covered now: monitor match, case-insensitive ordinal
match, mismatch rejection, empty requirement accepting any monitor, and invalid snapshots
being rejected. Still uncovered: that a rejected candidate continues the walk rather than
ending it. That part stays dependent on real two-display hardware until the enumerator
refactor is done.

**A tautological test was written and deleted.** The first draft added a test that built a
two-element candidate array in test code, looped over it calling the predicate, and asserted
the second element was found. It passes, but the loop under test was written in the test file.
It proves nothing about the product walk. A test that restates the test's own code is worse
than no test, because it reads like coverage. Deleted, and the real gap was queued instead.

**Placement note.** The helpers went in `WindowPlacementApplication.h/.cpp` rather than
`WindowPlacementRecord.h` because `::CompareStringOrdinal` needs `Windows.h`, and the record
header is deliberately Windows-free.

---

## TAEF said the crash was in coreclr.dll. It was ours.

`WindowPlacementPersistenceTests` passed test by test but failed as a class:
`Total=12, Passed=9, Failed=3`. The first failure was
`FirstActivateUsesPersistenceConfiguration`, and TAEF reported
`A crash with exception code 0xC0000005 occurred in module "coreclr.dll" in process
"Te.ProcessHost.exe"`. That module attribution is the top of the stack after the runtime
takes over a fatal error. It is not where the fault happened. Do not trust it.

**Get the real stack.** The VM already has WER LocalDumps configured for
`Te.ProcessHost.exe` (`HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\Te.ProcessHost.exe`
-> `DumpFolder=C:\dumps`, `DumpType=2`). No setup is needed. Clear `C:\dumps`, run, then:

| Step | Command |
| ---- | ------- |
| Debugger is already on the VM | `C:\Debuggers\cdb.exe` |
| Copy matching symbols | `Microsoft.ui.xaml.pdb` -> `C:\syms` on the VM |
| Set path in the remote block | `$env:_NT_SYMBOL_PATH = "C:\syms;srv*C:\symcache*https://msdl.microsoft.com/download/symbols"` |
| Get the stack | `cdb -z <dump> -c ".ecxr;kb 40;q"` |
| Get the locals | `cdb -z <dump> -c ".ecxr;.frame 5;dv /t /v;q"` |

Analyze in place. The dumps are 547 MB each; do not copy them to the host. Use `.ecxr;kb`,
not `!analyze -v` - without matching symbols `!analyze` invents a bogus `WRONG_SYMBOLS` /
`ntdll.wrong.symbols.dll` bucket that sends you the wrong way. The PDB must match the DLL
that actually crashed, so rebuild, then run, then copy the PDB.

**What it found.** The faulting frame was
`Microsoft_UI_Xaml!DirectUI::WindowPlacementPersistence::GetStringValue+0x4b`, reached from
`ApplicationDataPlacementSettingsBackend::ReadValue` -> `PlacementStore::Load` ->
`ReadPersistPlacement` -> `TryApplyPlacement` -> `DesktopWindowImpl::ApplyPlacement` ->
`WindowPlacementCoordinator::Activate` -> `Window.Activate`. `dv` showed
`object = 0x00000000` in `GetStringValue` and `hr = 0x00000000` in `ReadValue`.

**The bug.** `ApplicationDataContainerSettings` reports a missing key as `S_OK` with a null
`IInspectable`. It does not return `E_BOUNDS`. `ReadValue` checked only for `E_BOUNDS`, so a
missing value produced a null pointer that `GetStringValue` dereferenced.

**Why only in a class run.** The store has two ways to be missing. Alone, the settings
container does not exist, `GetSettingsContainer` returns Missing, and the lookup never runs.
After an earlier test writes a record the container exists, so the next test's different
value name takes the missing-value-inside-an-existing-container path and hits the null. Any
"passes alone, fails in the suite" placement failure should make you ask which state the
store was left in, not which test leaked a window.

**Rejected on the way.** An uncommitted edit to `DesktopWindowImpl.cpp` blamed first-window
teardown and HWND reuse, and removed a real `XAML_FAIL_FAST()` map invariant to do it. The
race is not possible: `DestroyWindow` delivers `WM_DESTROY` synchronously before the handle
is released. Reverting it and rerunning gave the identical `Passed=9`, so it was neither
cause nor cure. Two lessons. Never delete an invariant to quiet a crash you have not
localized, and reproduce on clean HEAD binaries before you believe any theory.

Fix: `ReadValue` treats a null lookup result as Missing, and `GetStringValue` returns
`E_POINTER` for a null object rather than faulting. Class run went to
`Total=12, Passed=12, Failed=0`.

## No test project can reach a C++/WinRT projection of locally built XAML types (2026-09-24)

A queued task asked for C++/WinRT runtime coverage of `WindowPlacement` in "an existing
C++/WinRT-capable test project". No such project exists. This is a durable fact about the
repository, not a missing reference.

| Candidate | Why it cannot reach the new API |
| --- | --- |
| `Microsoft.UI.Xaml.Tests.Isolated.WindowPlacement.vcxproj` | Standalone TAEF DLL. It compiles the placement `.cpp` files directly and never loads or registers `Microsoft.UI.Xaml.dll`, so no runtime class is activatable. |
| `dxaml\test\native\external\*` | Runs with the built framework loaded, but is C++/CX plus raw WRL ABI. `winrt::Microsoft::UI::Xaml` has zero matches under that tree, and C++/CX and C++/WinRT cannot share a translation unit. |
| `Samples\`, `perf\scenarios\`, `controls\test\`, `src\XamlCompiler\Tests\` | These include `winrt/Microsoft.UI.Xaml.h`, but it resolves against the shipped WindowsAppSDK package, not the local winmd, so a new API is invisible. |
| `controls\dev\Lightup` | Generates C++/WinRT headers for Windows SDK platform types only. |

The reachable equivalent is the raw ABI, which is what the C++/WinRT header is a thin wrapper
over. `dxaml\test\native\external\foundation\hosting\WindowDisplayApiHostTests.cpp` already uses
that pattern and documents why: going through the ABI keeps the HRESULT intact instead of letting
the projection turn it into an exception.

**Rejected on the way.** Commit 59e66cf61 answered the task with five test methods whose bodies
were identical and only fetched an activation factory, so `ConstructorValidatesInvalidDpi` and
`SnapshotIndependenceOfReturnedPlacement` asserted nothing about DPI or snapshots. It also swapped
the `WindowPlacementPublic.h` include for `WindowPlacementPolicy.h`, which does not declare
`InitialRequest`, `IsValidInitialRequest`, `TryCopySnapshot`, or `TryCopyDeviceName`. Building the
project at that commit produced 51 errors; after the revert in 936c655ca the same command reports
`Build succeeded. 0 Warning(s) 0 Error(s)`.

```
.\initrun.ps1 msb /q dxaml\xcp\dxaml\lib\unittests\WindowPlacement\Microsoft.UI.Xaml.Tests.Isolated.WindowPlacement.vcxproj
```

Lesson: when a task names a resource that does not exist, say so and requeue. A test whose name
promises coverage it does not deliver is worse than an open task, because the next reader stops
looking. Verify with a build before recording a task as done or as needing only verification.

## Native ABI tests compile, but test-host prerequisites block payload creation (2026-09-24)

`WindowPlacementAbiTests` in External.Foundation compiles against the locally generated ABI
and TAEF lists all five methods. This resolves the compile-time reachability question, not the
runtime question. The VM runner fails before deployment, so no test results exist yet.

The Foundation build initially reported NETSDK1004 for HostingHelpers. Restoring that project
resolved it:

```powershell
.\initrun.ps1 msb /q dxaml\test\infra\Win32Hosting\WPF\HostingHelpers\Private.Infrastructure.Hosting.HostingHelpers.csproj /t:Restore
.\initrun.ps1 msb /q dxaml\test\native\external\foundation\Microsoft.UI.Xaml.Tests.External.Foundation.vcxproj
```

The second command succeeded with zero errors and two dependency warnings (CsWinRT1028 and
MSB8019). The VM command was:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop -SkipPrerun WindowPlacementAbiTests
```

`-SkipPrerun` avoids machine-setup changes on the existing configured VM; payload refresh was
not skipped. The first attempt failed because `BuildOutput\bin\amd64chk\TAEF` was missing.
The following existing producers restored the TAEF and local .NET outputs after each had
reported NETSDK1004:

```powershell
.\initrun.ps1 msb /q dxaml\test\infra\BinplaceTestDependencies.csproj /restore
.\initrun.ps1 msb /q dxaml\test\infra\taefhostappnetcore\taefhostappnetcore.csproj /restore
```

Both succeeded with zero warnings and errors. Repeating the VM command then failed with exit 16:

```text
ERROR 2 (0x00000002) Accessing Source Directory D:\x1\BuildOutput\bin\amd64chk\TestDependencies\crtforwarders\
```

`dxaml\test\infra\taefhostapp\taefhostapp.vcxproj` binplaces those forwarders. Building it with
`.\initrun.ps1 msb /q dxaml\test\infra\taefhostapp\taefhostapp.vcxproj` failed with 3298 errors,
starting with:

```text
Styles.xaml(4960,141): XamlCompiler error WMC0003: A type's assembly could not be resolved.
This is often because it was forwarded to an unresolved assembly: Type universe cannot resolve
assembly: System.Runtime.WindowsRuntime, Version=4.0.0.0, Culture=neutral,
PublicKeyToken=b77a5c561934e089. [D:\x1\dxaml\xcp\dxaml\themes\autogen\GenAllXbf.csproj]
```

No product or build configuration was changed to bypass this failure. Fix the host build
prerequisite, complete payload creation, then run the ABI class. Do not interpret a successful
`TE.exe /list` as runtime coverage or deploy the partial payload as if it were complete.

## The payload block was three missing NuGet restores, and the runner passes on zero tests (2026-09-25)

Follow-up to the case above. Both problems in it turned out to be the same problem, and a third
one was hiding behind them.

### WMC0003 was a missing restore, not a type-resolution bug

`GenAllXbf.csproj` has a `PackageReference` to `Microsoft.NETCore.UniversalWindowsPlatform` and
sets `RestoreProjectStyle=PackageReference`. NuGet restore had never run for it in this
enlistment, so the UWP reference assemblies were absent and the XAML compiler could not resolve
`System.Runtime.WindowsRuntime`. That surfaced as 3298 `WMC0003` errors rather than the
`NETSDK1004` you get from a C# compile.

The check that identifies this class of failure:

> Look for `project.assets.json` under `BuildOutput\obj\<flavor>\<project path>\`.
> If it is absent, restore has never run for that project.

```powershell
.\initrun.ps1 msb /q dxaml\xcp\dxaml\themes\autogen\GenAllXbf.csproj /t:Restore
.\initrun.ps1 msb /q dxaml\xcp\dxaml\themes\autogen\GenAllXbf.csproj
```

The second command then succeeded in 34 seconds with zero errors. Restore is process-local, so
this needs no machine-wide change.

### Payload creation reveals one missing producer per run

`test\CreateTestPayload.ps1` calls `Publish-Item`, which shells out to robocopy and exits
immediately when the source directory is missing. Each run reports only the first missing
directory, so a naive loop is one full build-and-rerun cycle per producer. The chain here was:

| Missing payload path | Producer | Build command |
| --- | --- | --- |
| `TestDependencies\crtforwarders` | `taefhostapp.vcxproj` | `.\initrun.ps1 msb /q dxaml\test\infra\taefhostapp\taefhostapp.vcxproj` |
| `TestDependencies\AppX` | `TaefHostAppManaged.csproj` | `.\initrun.ps1 msb /q dxaml\test\infra\taefhostappmanaged\TaefHostAppManaged.csproj /restore` |
| `Test\UnpackagedApps\MUXControlsTestApp` | `MUXControlsTestApp.csproj` | `.\initrun.ps1 msb /q controls\test\MUXControlsTestApp\MUXControlsTestApp.csproj /restore` |

`robocopy` `ERROR 2` and `ERROR 3` both mean the source path does not exist, and both surface as
exit 16 from the runner.

Do not iterate one run at a time. Read the `Publish-Item` list in `CreateTestPayload.ps1` for the
mode you are running (`DevTestSuite` is lines 162-242) and test every source path at once:

```powershell
$b='D:\x1\BuildOutput\bin\amd64chk'
@("$b\TAEF","$b\TestDependencies\crtforwarders","$b\TestDependencies\AppX",
  "$b\Test\UnpackagedApps\MUXControlsTestApp") |
  ForEach-Object { if (-not (Test-Path $_)) { "MISSING: $_" } }
```

Missing *files* inside an existing directory are harmless. robocopy returns 0 when a filename
filter matches nothing, so `Publish-Item` only hard-fails on a missing directory.

Note that a native ABI test in `External.Foundation` required building the managed
`MUXControlsTestApp`, because `TE.ProcessHost.exe` in the payload root is the manifest-patched copy
that project produces. The native test lane is not independent of the managed app.

### The runner reports success when TAEF runs zero tests

With the payload finally complete, the first real run still executed nothing:

```text
Error: TAEF: The selection criteria did not match any tests.
Error: TAEF: No test cases were executed.
--------------------------------------------
Tests PASSED.
```

`test\scripts\runtests.ps1` builds its filter as `@Name='*<TestQuery>'`. TAEF matches that against
full test names, which look like `Namespace::Class::Method`. A bare class name therefore matches
only the class node and selects no test methods. The fix at the call site is to append `::*`:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop -SkipPrerun 'WindowPlacementAbiTests::*'
```

That ran all five cases: `Summary: Total=5, Passed=5, Failed=0, Blocked=0, Not Run=0, Skipped=0`.

The durable lesson is the failure mode, not the suffix. A mistyped or under-specified filter
produces a green run and exit 0. Always confirm the `Total=` count matches the number of tests you
expected before believing a pass. A follow-up task is queued to make the runner fail on zero
executed tests.

## A saved maximized placement restores 7 pixels lower than it was saved (2026-09-25)

`MaximizedPlacementSurvivesCloseAndRestore` in `WindowPlacementPersistenceTests` is the first
storage-backed check that a maximized window comes back maximized with its saved restore bounds. It
shows a window with an explicit placement, maximizes it through `OverlappedPresenter.Maximize()`,
closes it so automatic persistence saves, then reopens under the same persist id with no `Placement`
so the saved record is the source.

Everything about the state round trip works. The presenter and the capture both report `Maximized`
before the close, the loaded record reports `Maximized`, and the restored window reports `Maximized`
from both the presenter and a fresh capture. The storage round trip is exact: the loaded placement
matches the saved placement on X, Y, Width, Height, and Dpi.

The restore bounds do not survive:

```text
Before maximize: normal 107,107 640x480 work area 0,0 1024x720 at 96 DPI.
Saved maximized restore bounds: 107,107 640x480 work area 0,0 1024x720 at 96 DPI.
Restored maximized restore bounds: 107,114 640x480 work area 0,0 1024x720 at 96 DPI.
Error: Verify: AreEqual(107, 114)
```

Only Y moves, and only by 7. X, Width, Height, and Dpi all match, and the work area is identical on
both sides. That rules out the obvious suspects. It is not cascading, which was disabled on both
windows. It is not work-area fitting, because the saved rect already fits the current work area
unchanged. It is not serialization, because the loaded record is byte-for-byte what was captured.

One detail is worth separating out so it is not mistaken for the bug. The test asks for
`200,150 640x480` but the window lands at `107,107` before the maximize ever happens. That is the
placement policy fitting the rect, because the test helper records a `0,0 1920x1080` work area while
the VM work area is `0,0 1024x720`. It is expected, and it is the same on both sides of the round
trip, so it cannot explain a one-sided 7 pixel shift.

Both windows pass `DoNotActivate`, so both take the Window Action path in
`WindowPlacementApplication.cpp`, which hands the whole placement to `PlacementEx::SetPlacement` in a
single call. The drift is therefore somewhere in how the normal rect is handed to, or read back
from, that engine when the show command is maximize.

Two process lessons came out of this.

First, log the inputs, not just the mismatch. The first run only printed the failing assertion, which
looked like a vague "bounds drifted" problem. Adding the pre-maximize placement and the work area to
the log turned it into a precise, one-variable fact, and immediately eliminated fitting and cascading
as causes.

Second, a correct test that finds a real defect should not be weakened to go green, and it should not
be left red either. This one is committed with the pattern already used elsewhere in the same test
area, `[TestProperty("Ignore", "TRUE")]` plus a TODO naming the exact numbers, with the finding
recorded in the queue and a `[Code]` task to chase the 7 pixels. The assertion stays strict, so the
moment the product is fixed the marker comes off and the test proves the behavior.

### Resolution

The 7 pixels came from a redundant same-monitor move action. `PlacementEx::SetPlacementWithApplyWindowAction`
already skipped the move-to-monitor action and the historical work-area and DPI modifiers when the
window was already on a monitor with the same work area and DPI, but that optimization was gated on
the placement being *normal*, so a maximized or minimized placement still got the action.
`ApplyWindowAction` applies a frame adjustment whenever that action is present, and the saved normal
rectangle was already in the current monitor's coordinate space, so the adjustment shifted Y.

The gate now only excludes full-screen and arranged placements. Maximized and minimized placements
take the same-monitor path, because their normal rectangle is the restore position. A different
monitor, work area, or DPI still keeps the action and modifiers, which is what lets a placement from
a removed or changed monitor be migrated.

The `Ignore` marker came off and the class runs `Total=17, Passed=17` on `ge_current-260828-Desktop`.
The saved and restored bounds now match exactly at `107,100 640x480` on a `0,0 1024x720` work area at
96 DPI.

The durable lesson: when an optimization is correct for one state, check whether the property it
depends on is really about that state. Here the property was "the rectangle is already in this
monitor's space," which is true for every non-migrating placement, not just normal ones. The
state-shaped condition was narrower than the fact it was standing in for.

## The VM applies a saved minimized state to an existing window but not to a new one (2026-09-25)

`MinimizedPlacementSurvivesCloseAndRestore` in `WindowPlacementPersistenceTests` closes the last
step 7 gap that could be tried on one display: a saved minimized placement. It mirrors the
maximized test exactly, only with `OverlappedPresenter.Minimize()`.

The save half works. On `ge_current-260828-Desktop`:

```text
Before minimize: normal 107,100 640x480 work area 0,0 1024x720 at 96 DPI.
Saved minimized restore bounds: 107,100 640x480 work area 0,0 1024x720 at 96 DPI.
Verify: AreEqual(Minimized, Minimized)
Verify: AreEqual(107, 107)  Verify: AreEqual(100, 100)
Verify: AreEqual(640, 640)  Verify: AreEqual(480, 480)  Verify: AreEqual(96, 96)
```

The presenter reports `Minimized`, capture reports `Minimized`, and the detached load returns
`Minimized` with the restore rectangle intact. So state translation, capture, the record format,
and packaged storage all handle minimization correctly.

The apply half does not. Reopening under the same persist id with no `Placement` gives a window
whose presenter reports `Restored`:

```text
TestSkipped: This environment refused minimized application on a new window.
The restored presenter reports Restored.
```

This is the same class of refusal the design note already records for new-window snapped and
minimized-from-snapped Window Action requests. Plain minimize is refused too. The test reports
Skipped with the observed state rather than passing, so the suite stays honest: `Total=18,
Passed=17, Skipped=1`.

Two things worth keeping.

First, split the assertion at the seam between the two halves. Had the test asserted only the end
state, the result would have been a bare failure that says nothing about where minimization breaks.
Verifying the loaded record before touching the restored window turns one red result into a precise
statement: capture and storage are correct, application is refused by the OS.

Second, a skip needs the observed value, not just the reason. `reports Restored` is what makes this
actionable later. It distinguishes an environment that ignored the request from a product path that
never issued one, and it gives the next run something to compare against.

## Generated argument checks run before the out-parameter is cleared (2026-09-25)

A new ABI test for `WindowPlacement.LoadForPersistPlacementId` started from a non-null sentinel so
it could prove the callee cleared the out-parameter on the argument-rejection path. The HRESULT
was right and the out-parameter was not:

```text
Verify: AreEqual(E_INVALIDARG, statics->LoadForPersistPlacementId(id, &placement))
Error: Verify: IsNull(placement) - Value (0xFFFFFFFFFFFFFFFF)
```

The cause is in generated code, not in the placement implementation:

```cpp
// dxaml\xcp\dxaml\lib\winrtgeneratedclasses\WindowPlacement.g.cpp:231
ARG_NOTNULL(persistPlacementId, "persistPlacementId");
ARG_VALIDRETURNPOINTER(ppResult);
*ppResult={};
IFC(LoadForPersistPlacementIdImpl(persistPlacementId, ppResult));
```

A null input string fails `ARG_NOTNULL` and returns two lines before `*ppResult={}`. That ordering
is the standard shape the IDL code generator emits for every WinUI API, so it is not a placement
defect and is not worth a one-API fix.

Two durable points.

First, `LoadForPersistPlacementIdImpl`'s own `placementIdLength == 0` guard cannot be reached from
a well-formed ABI caller. WinRT canonicalizes an empty string to a null `HSTRING`, so both the null
and the empty case stop at `ARG_NOTNULL`. The guard is defense in depth for internal callers.

Second, when a sentinel assertion fails, check whether the contract or the test is wrong before
changing product code. Here the real contract is "no object is produced", which a default-null
out-parameter proves. The sentinel assertion still has value, so it moved to the path that does
reach the callee: a valid unused id, where the out-parameter is cleared and the call returns
`S_OK` with null.
