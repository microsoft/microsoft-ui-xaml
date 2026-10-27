# Issue #12158: activation DLL search and WebView2 recovery

[Issue #12158](https://github.com/microsoft/microsoft-ui-xaml/issues/12158)

## Changes

- Extend the DLL-name fallback in `MuxcActivationHandler`. Keep the existing
  `LoadLibraryW` search first. If it fails, try C++/WinRT's
  `LoadLibraryExW(..., LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)` policy, which includes
  directories registered with `AddDllDirectory`.
- Include environment-options activation and language setup in WebView2's existing
  environment-creation error handling. These failures now raise
  `CoreWebView2Initialized`, just like other environment-creation failures.
- Complete the creation wait and clear creation flags in `CreateCoreObjects` on
  both success and failure. Detach the completed wait before signaling it.

The original cleanup ran only after controller creation. An earlier failure left
the creation marker set, so subsequent `EnsureCoreWebView2Async` calls waited on an
operation that would never signal completion.

## Regression app

`controls\test\WebView2ActivationTestApp` is a standalone, unpackaged native app.
It calls the real WebView2 control and the real activation handler in the locally
built controls DLL. It does not substitute a test-only loader implementation.

Each case starts a fresh process with a separate browser-data directory. The native
Core DLL exists only in a `plugin` directory. The app's SDK activation manifest
deliberately excludes Core: otherwise manifest redirection overrides the DLL search
and does not reproduce the unpackaged fallback scenario.

| Case | Original product | Fixed product |
|---|---|---|
| `added`: only `AddDllDirectory`, then default Ensure | Exit 1; activation fails; no initialization event | Exit 0; one successful event; Core exists |
| `preload`: full-path DLL preload control | Exit 0 | Exit 0 |
| `recovery`: Source first without a DLL directory, then explicit-environment Ensure after adding it | Hangs; runner stops it after 45 seconds | Exit 0; failure event, pending caller completes, then successful initialization |
| `path`: only PATH contains the plugin directory | Exit 0 with the original DLL search | Exit 0 |

All successful cases verify that the loaded native Core DLL comes from the plugin
directory. The recovery case also registers an Ensure caller inside the failure
event, before the failing creation operation finishes.

The standalone baseline returns `0x80040154`, rather than the report's
`0x8007007E`, because Core has no activation-manifest registration in this app.
Both failures reach the same namespace-based DLL fallback. An additional
loader-only reversal, retaining the WebView2 cleanup fix, failed `added` and
passed `preload`, `recovery`, and `path`. This isolates the loader regression and
checks the pre-existing PATH behavior separately.

## Build and run

The regression app currently uses the existing full amd64chk build outputs,
including the native compiler-test projection and generated SDK manifest.

```powershell
.\initrun.ps1 msb /q "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"
.\initrun.ps1 .\controls\test\WebView2ActivationTestApp\Build.ps1
```

Both commands passed. Copy the complete resulting directory
`BuildOutput\bin\amd64chk\Test\UnpackagedApps\WebView2ActivationTestApp` to a
Windows x64 VM with an installed WebView2 Runtime. Run on its interactive desktop:

```powershell
.\Run.ps1
```

Executed through an interactive scheduled task on the local Hyper-V VM.
Before: one passing preload control, one failing lookup, and one timed-out recovery.
After: all four cases passed, with exit code 0 and no timeouts. Core SDK:
1.0.3719.77, from the repository's restored package.

Existing activation integration coverage also passed, 2/2:

```powershell
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <local-vm> "*ActivationIntegrationTests*" "-HostingMode:WPF"
```

The earlier native-host probe could reproduce the missing event and hang, but its
Core activation-manifest binding prevented a valid AddDllDirectory-only test.
The committed standalone app avoids that binding and does not rename shared test
payload DLLs.

## Compatibility and scope

- Existing successful DLL lookup keeps its precedence and behavior, including
  current-directory/PATH lookup. Only failed searches try the additional
  C++/WinRT directory policy. The PATH-only control passed both before and after.
- Existing `RoGetActivationFactory` handling, the XAML fast path, interface
  queries, and successful-module lifetime behavior remain unchanged.
- Initialization uses the existing failure event and completion semantics.
  No new public API or global `SetDefaultDllDirectories` workaround is added.
- Related open PRs [#12324](https://github.com/microsoft/microsoft-ui-xaml/pull/12324)
  and [#11165](https://github.com/microsoft/microsoft-ui-xaml/pull/11165) concern tab
  accelerator handling and transparent backgrounds, not this activation or
  failed-creation path.

All work stays local on `user/jessecol/issue-12158`. No push or PR was created.
