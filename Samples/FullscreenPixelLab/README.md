# Fullscreen Pixel Lab

A packaged C# diagnostic sample for [issue #12138](https://github.com/microsoft/microsoft-ui-xaml/issues/12138).
It measures the physical top offset of the XAML `DesktopChildSiteBridge` HWND relative to the window's client origin.
The bright green strip is an edge-test surface, not a simulated gap.

## Build and deploy

From an initialized repository root, build the local product and sample:

```powershell
.\initrun.ps1 .\build.cmd /q product
.\initrun.ps1 msb /q "Samples\FullscreenPixelLab\FullscreenPixelLab.csproj" /restore /p:Platform=x64 /p:RuntimeIdentifier=win-x64
.\Samples\FullscreenPixelLab\Export-Layout.ps1 -OutputDirectory C:\Temp\FullscreenPixelLab-AppX
```

Choose a new or empty output directory. Register its `AppxManifest.xml` on a development-mode machine
or deploy the directory to a test VM. The installed name is **Issue 12138 - Fullscreen Pixel Lab**.
The layout is self-contained, including .NET.

The export script uses the build's package recipe, the full local native runtime payload, and generated
activation registrations. Exporting only the app executable or swapping a single DLL into a shipping SDK
layout can fail before the sample opens.

For ARM64, build with `-Flavor arm64chk`, `/p:Platform=ARM64`, and `/p:RuntimeIdentifier=win-arm64`;
pass `-Flavor arm64chk` to the export script too.

## Exercise the fix

1. Start with **Window.ExtendsContentIntoTitleBar** enabled in a restored window.
2. Select **Enter fullscreen**, or press **F11**.
3. Check that the actual top offset is **0 physical px** and the green strip touches the top row.
4. Toggle title-bar extension off and on while fullscreen. The offset should remain 0.
5. Press **Esc**. With title-bar extension enabled, the restored window should report 1 physical px.
6. Repeat fullscreen entry and exit. Toggle title-bar extension in the restored window too.

| State | Expected offset with the fix |
|---|---:|
| Restored, title-bar extension on | 1 physical px |
| Restored, title-bar extension off | 0 physical px |
| Fullscreen, extension on or off | 0 physical px |
| Maximized | 0 physical px |

Entering fullscreen on an unfixed runtime with title-bar extension already enabled reports **1 physical px**.
The bridge is also one pixel shorter than the client area. Toggling title-bar extension while fullscreen
can leave the previous bridge offset in place until the next presenter transition.
The diagnostic reads native rectangles and styles every 250 ms, so allow a transition to settle.
Measurement failures appear as errors, not passes.

## Compare genuine runtimes

Build `Microsoft.ui.xaml.dll` at the pre-fix commit and at the fixed commit, preserving each output.
Export the same sample twice using `-Variant Before` or `-Variant After`,
`-RuntimeDll <preserved-dll>`, and an accurate
`-RuntimeLabel "BEFORE - pre-fix WinUI"` or `-RuntimeLabel "AFTER - fixed WinUI"`.
Only substitute compatible runtimes from the same source baseline and build flavor.
The export records the app and native DLL SHA-256 hashes in `runtime-provenance.json`.

The variants have separate package identities and **BEFORE** / **AFTER** suffixes in Start, so both
can stay installed. Close the app before updating its layout. If its registered installation path
changes, uninstall that sample variant before registering the new location.
Capture the full-resolution screen
in fullscreen and inspect the first row away from the mouse pointer or overlays.
Use real before/after runtimes; changing the label does not change behavior.

## Validation scope

The sample was tested on Windows 11 x64 at 96 DPI with genuine pre-fix and fixed local runtimes.
Each passed 45 checked assertions across fullscreen entry/exit, title-bar toggles, maximize/restore,
keyboard shortcuts, and repeated transitions, with the baseline checks expecting the known defect.
Full-resolution PNGs confirmed a white first row before the fix and a green first row after it.
Windows 10, ARM64, and other DPI settings have not been verified.
