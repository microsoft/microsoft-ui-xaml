# PlacementEx (fork)

This folder is a **fork** of the shared Win32 window-placement headers owned by the Windows OS
repository (os.2020). WinUI keeps a near-verbatim copy here so that WinUI and the OS apply the
same placement rules.

## Provenance

| Field | Value |
|---|---|
| Source repo | os.2020 (Azure DevOps: microsoft/OS) |
| Source path | `clientcore/windows/Core/ntuser/test/Common/` (umbrella `User32Utils.h`) and its `inc/` subfolder |
| Source branch | `official/ge_current` |
| Source commit | `6964a8f5c224` (branch head when copied, 2026-08-29) |
| License | MIT (same as this repository). Preserve original copyright headers when porting updates. |

## This is a fork - please do not refactor

Keep these files as close to the upstream copy as possible. The same headers are forked into
several other places (for example OS `isoenvbroker` samples, OSClient `StreamingIslandTestApp`,
and the C# `SampleCSharpApp` port). We may want to pull fixes from those forks - or push ours
back - in the future. Every gratuitous rename, reformat, or restructure makes that merge harder.

Guidance:

- Modifying is fine when we need it. Refactoring for style is not.
- If you must change a file, keep the diff minimal and local so upstream diffs still apply.
- Prefer adding a thin WinUI adapter **outside** this folder over editing the headers.

## Layout note

In the OS repo, the umbrella `User32Utils.h` sits one directory above the other headers (in
`Common/`, with the rest in `Common/inc/`). Here we flattened `User32Utils.h` into this same
folder. That is the only structural change from upstream; the header contents are unmodified.

## Files

| File | Role |
|---|---|
| `User32Utils.h` | Umbrella include. Pulls in the Windows SDK headers plus the files below. |
| `PlacementEx.h` | The placement math: capture a window position, and restore it safely onto a currently connected monitor. |
| `MonitorData.h` | Per-monitor info (rect, work area, DPI, device name) and monitor enumeration. |
| `CurrentMonitorTopology.h` | Caches the connected-monitor set and reacts to display changes. |
| `MiscUser32.h` | DWM / user32 helpers (cloaking, frame bounds, DPI awareness, snapping). |
| `WindowActions.h` | Wraps the user32 `ApplyWindowAction` API (loaded dynamically). |
| `RegistryHelpers.h` | Read/write helpers for persisting values (registry). WinUI may not use these. |
| `VirtualDesktopIds.h` | Virtual-desktop GUID helpers. Gated by `USE_VIRTUAL_DESKTOP_APIS`. |
| `RememberingWindowPositions.md` | Upstream doc: how a window position is captured and restored. |
| `Win32Concepts.md` | Upstream doc: the Win32 window-placement concepts the math relies on. |

## How WinUI uses this

WinUI translates between its own `WindowPlacementData` and PlacementEx at the native boundary.
PlacementEx never sees the `WPL1` serialized bytes; serialization and value-name derivation are
WinUI-owned and live one directory up.

## Dependencies

The headers only need the public Windows SDK (`windows.h`, `shellscalingapi.h`, `dwmapi.h`,
`shobjidl*.h`) plus standard C++ and WIL (`wil/stl.h`, `wrl.h`). There are no internal OS-only
header dependencies, which is what makes this a clean fork.

The `sources` / `sources.inc` NTBUILD files from the upstream folder are intentionally **not**
copied - WinUI uses its own build system.
