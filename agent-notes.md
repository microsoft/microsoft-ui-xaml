# Agent handoff: Window Width/Height after close

Updated 2026-09-10.

## Status and scope

Implementation and regression tests are complete. A full amd64chk build succeeded.
All 33 selected window integration tests passed on the VM, including all four new
tests. There were no failed, blocked, skipped, or unrun tests.

- Repository: `microsoft/microsoft-ui-xaml`.
- Branch: `jessecol-window-closed-sizing`, based on `main` at `d0c6705ec`.
- Spec reference: commit `e2fba765690ed8009824fbf36ce06fa7bcc5d314`
  ("Updates from PR feedback"), read from the separate
  `user/jessecol/window-width-height` branch.
- Spec path on that branch: `specs\Window\window-width-height-spec.md`, especially
  section 2.4, "Sizing properties after the window closes".
- This implementation branch does not include the spec branch's commits.
- Scope is Width/Height after close. Other spec edits, including title-bar
  behavior, are not part of this change. No pull request has been created.

## Implemented contract

Closing freezes each property's getter value after all `Closed` handlers finish
without canceling, before teardown changes the client area. This is not
necessarily the last value assigned by the app. It includes native/user resizing,
restored sizes while minimized or maximized, and unapplied requests before first
activation or in a non-default presenter.

After close, getters return those preserved values. Valid assignments succeed
without changing either value, scheduling a resize, raising `SizeChanged`, or
reopening the window. Invalid values still fail with `E_INVALIDARG`. Access on
another thread still fails with `RPC_E_WRONG_THREAD`.

Sizing retains normal open-window behavior during `Closed` handlers. A canceled
close does not freeze values. Other members, including `Bounds`, retain their
existing closed-state behavior.

## Code changes

- `dxaml\xcp\dxaml\lib\DesktopWindowImpl.cpp`: `CloseImpl` captures the existing
  getters after the cancellation check and before `PrepareToClose`, gated by
  `AreNewWindowingApisEnabled`. Closed getters return the snapshots. Setters
  validate first, then return success without applying or deferring a size when
  `m_bIsClosed` is true.
- `dxaml\xcp\dxaml\lib\DesktopWindowImpl.h`: two dedicated double-precision
  snapshot fields preserve pending requests without rounding them to `wf::Size`.
- `dxaml\test\native\external\controls\window\WindowIntegrationTests.cpp` and
  `.h`: four new prerelease, WPF-hosted test methods:
  `WindowWidthHeightAfterClose`,
  `WindowWidthHeightAfterCloseValidatesArguments`,
  `WindowWidthHeightAfterCloseChecksThread`, and
  `WindowWidthHeightDuringClose`.

Coverage includes six window states with and without explicit sizes, both
title-bar configurations, exact pending double values, mixed pending/native
dimensions, invalid arguments, thread affinity, repeated/reentrant close,
cancellation, multiple `Closed` handlers, and unchanged post-close `Bounds` errors.

## Build and test on the next machine

Use a short checkout path (the developer guide recommends fewer than 10
characters), adequate disk space, the existing Visual Studio prerequisites, and
a configured Hyper-V VM with an interactive desktop. From the checkout root:

```powershell
# Needed once for a fresh checkout/flavor.
.\init.ps1 amd64chk

# Full product and test build, including the local SDK package.
.\initrun.ps1 .\build.cmd /q

# Replace the VM name with the target machine's configured test VM.
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName 'ge_current-260820-Desktop' 'Controls::Window::WindowIntegrationTests::*' -Platform x64 -Configuration chk -HostingMode WPF -SkipPackageUninstall
```

The test script creates and deploys the payload and handles VM credential
prompting/caching. Credentials and build outputs are not included in Git.

### Environment issues encountered

- `initrun.ps1` initially failed because the fresh worktree had not been
  initialized. A full `init.ps1 amd64chk` resolved this.
- The inherited Git environment had `GIT_CONFIG_KEY_2=core.fsmonitor` with an
  empty value. Environment initialization dropped that empty value, causing
  MSBuild's `git rev-parse HEAD` commands to fail. Before each build/test command,
  this process-local workaround retained the intended disabled setting:

  ```powershell
  if ($env:GIT_CONFIG_KEY_2 -eq 'core.fsmonitor' -and
      [string]::IsNullOrEmpty($env:GIT_CONFIG_VALUE_2)) {
      $env:GIT_CONFIG_VALUE_2 = 'false'
  }
  ```

- Do not use `/nomock` for a fresh full build. It built the native solution but
  skipped the local `Microsoft.WindowsAppSDK.WinUI` package needed by MUX test
  apps, causing `NU1102`. Repeating the full build without `/nomock` succeeded.
- `build.cmd` returned process exit code 0 on earlier failures. Inspect its
  output for `ERROR: buildSolution ... FAILED`; do not rely on exit code alone.
- VM prerun printed an existing `Cannot overwrite variable Error` issue while
  handling VCLibs installation. Setup continued and the test run passed.
  No unrelated script changes were made.

## Original-machine artifacts

These paths describe the original machine only. Do not assume they exist or
reuse them on the next machine.

| Purpose | Location |
| --- | --- |
| Worktree | `D:\gh-cp-repos\copilot-worktrees\x1\jessecol-friendly-tribble` |
| Short junction to that worktree | `C:\w455` |
| Build output | Worktree `BuildOutput` junction to `C:\wb455` |
| Test payload | Worktree `TestPayload` junction to `C:\wp455` |
| VM | `ge_current-260820-Desktop`, Windows build 26678 |
| VM payload | `C:\TestPayload\JESSE3-w455` |
| Raw test log | `%USERPROFILE%\.copilot\session-state\45510588-8bad-42ba-a142-b00dca9a9ca3\files\window-integration-tests.log` |

The output/payload junctions were necessary because D: was low on space. Build
logs remain under `BuildOutput`, including the `amd64chk` solution binlogs.
The raw test log is not committed; the portable result is 33 passed out of 33.

## Related work

A separate session, "Window size constraints", is handling post-close
`MinWidth`, `MinHeight`, `MaxWidth`, and `MaxHeight` behavior on branch
`jessecol-window-size-constraints`. Those changes are not included here. Both
branches touch `DesktopWindowImpl.cpp` and `WindowIntegrationTests.cpp/.h`, so
preserve both sets of changes when combining them.

That session was notified when this build/test run finished and shared resources
were free. Its later build/test status has not been checked here.
