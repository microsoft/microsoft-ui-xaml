# WinUI Profiler app shell

Minimal extraction of the app layout from microsoft/microsoft-ui-xaml#12393,
revision `2edb8164bb56e217dfbc23d4bc3309284fe07f2c`.

Includes the process graph panels, saved-session workspace, and a separate
snapshot viewer with XAML/composition panes. These are empty views: there is no
connection, collection, session loading, snapshot capture, or tree construction.
Data-dependent controls stay disabled. Reopening the snapshot viewer reuses its
window; closing the app closes the viewer.

## Build and run

Requires Windows 11, the .NET 10.0.4xx SDK, and Visual Studio with the WinUI/C#
build tools. The app uses the prototype's pinned WinUI NuGet package, not a local
WinUI product build. Both .NET and Windows App SDK runtimes are deployed beside
the executable; no MSIX registration is needed.

From the repository root, build a clean x64 Release:

```powershell
.\initrun.ps1 -Flavor amd64fre .\tools\WinUIProfiler\scripts\Build.ps1 -Clean
```

Run `tools\WinUIProfiler\WinUIProfiler.App\bin\x64\Release\net10.0-windows10.0.26100.0\win-x64\WinUIProfiler.App.exe`.
Keep the complete output directory together when copying the app.
`Build.ps1` also accepts `-Configuration Debug` and `-Architecture ARM64`.

To exercise the actual app's navigation and window lifetime:

```powershell
powershell.exe -NoProfile -File .\tools\WinUIProfiler\scripts\Test-App.ps1 -Executable .\tools\WinUIProfiler\WinUIProfiler.App\bin\x64\Release\net10.0-windows10.0.26100.0\win-x64\WinUIProfiler.App.exe
```
