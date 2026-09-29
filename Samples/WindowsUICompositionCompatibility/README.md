# Windows.UI.Composition compatibility harness

This standalone x64 Win32/C++/WinRT application compares system
`Windows.UI.Composition` behavior on Windows 11 build 22000 and a current
Windows 11 build. It does not use WinUI or lifted `Microsoft.UI.Composition`.

The native window contains an indexed scenario list. Selecting a scenario opens
its detail page, and each scenario can be run independently. **Run all** also
writes deterministic JSON results next to the executable.

## Build

Build with the Windows 11 SDK 10.0.26100.0 while preserving build 22000 as the
minimum runtime:

```powershell
.\initrun.ps1 msb /q "Samples\WindowsUICompositionCompatibility\WindowsUICompositionCompatibility.vcxproj"
```

The output is under the normal repository `BuildOutput` tree. Copy the EXE and
its adjacent runtime files to each test machine. The harness uses API
information checks before calling UniversalApiContract v15 APIs, so those tests
report **NOT AVAILABLE** rather than failing on build 22000.

For unattended collection, run:

```powershell
WindowsUICompositionCompatibility.exe --run-all
```

This runs every scenario without showing the window and writes
`WindowsUIComposition-results-<OS version>.json` beside the executable.

## Result interpretation

- `PASS`: the API was present and the scenario completed.
- `FAIL`: the API was present but its behavior or result did not satisfy the
  scenario.
- `NOT AVAILABLE`: expected for v15 additions on build 22000, or when optional
  hardware/diagnostic support is unavailable.

The detailed analysis and expected cross-build results are in
`WindowsUIComposition-22000-vs-26100.html`. The exact additive API list is also
available as `WindowsUIComposition-api-diff.csv`. Checked-in result captures
are under `Results`.
