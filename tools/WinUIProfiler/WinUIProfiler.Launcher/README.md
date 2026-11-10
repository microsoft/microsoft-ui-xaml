# Private WinUI launcher

`ProfilerLauncher.exe` starts a WinUI 3 app so that it loads private builds of
`Microsoft.UI.Xaml.dll` (MUX) and `Microsoft.UI.Xaml.Controls.dll` (MUXC)
instead of the runtime it ships with or depends on. Installed app and framework
files are never changed, no `.local` redirection is set up, and no restart is
needed. The profiler uses it when it is started with `--private-runtime`; see
"Profile an app with private WinUI DLLs" in the profiler's README.

The sources are copied from the `winui-perf-tools` native launcher in
winui-tools. They build with the Visual Studio C++ tools and
[Microsoft Detours](https://github.com/microsoft/Detours), which
`scripts\Build-Launcher.ps1` downloads at the commit pinned in
`dependencies.json`. Git holds only the sources; the binaries and Detours are
built under `artifacts\launcher`.

## How it works

- **Unpackaged app:** Detours creates the process suspended and adds
  `ProfilerRedirect.dll` to its imports, so the helper initializes before the
  app's own code runs.
- **Packaged app:** `IPackageDebugSettings` registers the launcher as the
  package's debugger for this activation, and `IApplicationActivationManager`
  activates the app normally. The launcher, started by Windows with the new
  process ID, adds the helper with Detours and resumes the process. A short,
  per-launch named file mapping carries the configuration, so long paths stay
  out of the debugger command line. Package debugging is turned off again after
  the helper reports ready.
- **Helper:** `ProfilerRedirect.dll` hooks `KernelBase!LoadLibraryExW`. A
  request for MUX by bare name or from the original runtime folder loads the
  private copy instead; so does MUXC when `--controls 1`. Other DLLs, and the
  MUI resources, still come from the original runtime. If MUX is already
  loaded when the helper starts, or a private DLL is missing, it fails instead
  of running the app with the wrong runtime.

The launcher waits up to 30 seconds for the helper to report ready, then
writes its result and exits. It never runs elevated: an elevated profiler
starts it through the signed-in Explorer desktop, so the app does not inherit
administrator rights.

## Command line

```text
ProfilerLauncher.exe --exe <app.exe> --source <original runtime folder>
    --private <private runtime folder> --controls 0|1
    --log <redirect.jsonl> --result <launch.json>
    [--arguments <app command line>]
    [--aumid <AUMID> --package <PackageFullName> --expected-aumid <AUMID>]
```

`--aumid` and `--package` select a packaged launch; the launcher activates the
app without arguments. Unknown or repeated options exit with code 87.

## Results

- `<result>.owner.json` is written first, with the launcher's process ID and
  creation time.
- `<result>` (`launch.json`) records the outcome:

  ```json
  {"schemaVersion":1,"pid":1234,"creationTimeFileTime":134000000000000000,"error":0,"stage":"ready","packageFullName":"","aumid":""}
  ```

  `pid` and `creationTimeFileTime` (the process's creation time as a UTC
  FILETIME) name the app's process instance, since Windows can give a later
  process the same ID. `pid` is 0 when no process started;
  `creationTimeFileTime` is 0 when the launcher could not open the process or
  read its creation time.

  `error` is a Win32 error code. When it is not 0, `stage` says which step
  failed: `configuration`, `create_process`, `insert_helper`,
  `package_debug_settings`, `package_enable_debugging`,
  `package_activation_manager`, `package_activate`, `package_debugger`,
  `helper_ready`, `package_cleanup`, `package_identity` or `process_times`. A
  helper that does not report ready within 30 seconds gives error 1460
  (`ERROR_TIMEOUT`); an app that exits first gives its exit code.
- `<log>` (`redirect.jsonl`) has one JSON line per helper event, for example
  `{"event":"redirected","pid":1234,"requested":"...","selected":"...","error":0}`.
  A packaged app may write it to its package's `LocalCache` folder instead,
  because Windows virtualizes new files a packaged app creates in `AppData`.

A ready result means only that the helper started. The profiler then checks
that the `Microsoft.UI.Xaml.dll` the app loaded is the private file.

## Build

From `tools\WinUIProfiler`:

```powershell
.\scripts\Build-Launcher.ps1 -Architecture x64
```

`Build-App.ps1` builds the launcher for x64 and ARM64 and deploys it next to
the profiler in `NativeLauncher\<architecture>`, with a `manifest.json` of
SHA-256 hashes that the profiler checks before each launch.

## Limits

- The private MUX and MUXC must be ABI- and resource-compatible with the
  runtime they replace: same Windows App SDK version and architecture.
- UWP (AppContainer) apps and apps started through a launcher stub are not
  supported.
- The helper and the package debugger add time to the launch. Compare only
  launches that use the same mode and DLLs.
- Don't debug or profile the same package in another session at the same
  time; package debugging is per package.
- Code integrity policies still apply; the launcher does not bypass them.
- The app's import table names `ProfilerRedirect.dll`, next to the launcher,
  by its path in the system's ANSI code page, with 8.3 short names where the
  path has them. Without short names (on a Dev Drive, for example), a path
  with characters outside that code page fails at `create_process` or
  `package_debugger` with error 1113 (`ERROR_NO_UNICODE_TRANSLATION`), and a
  non-ASCII path fails at `helper_ready` when the app's manifest sets another
  `activeCodePage`, such as UTF-8.
