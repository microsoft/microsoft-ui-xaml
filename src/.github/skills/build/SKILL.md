---
name: build
description: Build the WinUI repository. Use when asked to build, compile, or rebuild the project after making code changes.
---

# Building WinUI

## AI Agent Quick Start

```powershell
# Always wrap commands with .\initrun.ps1 — it sets up the build environment automatically.
# Default flavor is amd64chk. Override with -Flavor.

.\initrun.ps1 .\build.cmd /q                            # full repo build (product + tests) — USE THIS BY DEFAULT
.\initrun.ps1 .\build.cmd product /q                    # product code only (no tests)
.\initrun.ps1 .\build.cmd mux /q                        # MUX only (Microsoft.UI.Xaml.dll)
.\initrun.ps1 msb "path\to\project.vcxproj" /q          # build a single project
.\initrun.ps1 -Flavor arm64fre .\build.cmd /q           # build for a different flavor
```

**Rules:**
- Always prefix with `.\initrun.ps1`
- Always pass `/q` for quiet output (errors only)
- Set `initial_wait` to at least **300 seconds** — builds take 1-10+ minutes
- **When the user asks to "build the repo" or just "build" without specifying a target, use `.\initrun.ps1 .\build.cmd /q` (full build).** 
Only use `mux` or a single project when the user asks for a specific component or when you know exactly which files changed.

## First-Time Setup

A full `init` must be run once per flavor to download tools and NuGet packages.
`initrun.ps1` will fail with **"Run a full init first"** if this hasn't been done.

When you see that error, run a full init for the needed flavor:

```powershell
.\init.ps1                # default: amd64chk
.\init.ps1 amd64fre       # specific flavor
```

Set `initial_wait` to at least **300 seconds** — the first init downloads tools and restores NuGet packages.

Flavors: `amd64chk`, `amd64fre`, `x86chk`, `x86fre`, `arm64chk`, `arm64fre` (`chk` = debug, `fre` = release)

After init completes, retry the original `initrun.ps1` build command.

If you get build errors that seem to indicate missing dependencies, try running init again.

## Commands

| Command | What it builds | Time |
|---------|---------------|------|
| `.\initrun.ps1 .\build.cmd /q` | Everything (product + tests) | 10+ min |
| `.\initrun.ps1 .\build.cmd mux /q` | `Microsoft.UI.Xaml.dll` only | 1-6 min |
| `.\initrun.ps1 .\build.cmd product /q` | Product code (no tests) | 5-10 min |
| `.\initrun.ps1 .\build.cmd /c /q` | Clean + full rebuild | 15+ min |
| `.\initrun.ps1 msb "<project>" /q` | Single `.vcxproj` | 5s - 5 min |

### Flags (for `.\build.cmd` and `msb`)

| Flag | Effect |
|------|--------|
| `/q` | Quiet — errors only, plus elapsed time |
| `/i <flavor>` | Init environment inline (alternative to `.\initrun.ps1`) |
| `/restore` | NuGet restore before building |
| `/nomock` | Skip mock package.  Use if you're only updating product and test code under`dxaml/` and don't need to run MUXControls or sample tests.) |
| `/fake` | Dry run — print commands without executing |

## What to Build After a Code Change

| Files changed in | Build command |
|---|---|
| `dxaml/xcp/**` | `.\initrun.ps1 msb "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"` |
| `controls/dev/**` or `controls/idl/**` | `.\initrun.ps1 msb "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"` |
| `dxaml/test/native/external/<area>/**` | `.\initrun.ps1 msb "dxaml\test\native\external\<area>\Microsoft.UI.Xaml.Tests.External.<Area>.vcxproj"` |
| Multiple areas or unsure | `.\initrun.ps1 .\build.cmd /q` |

Test areas: `controls`, `foundation`, `framework`, `automation`

## Terminology

**MUX** = `Microsoft.UI.Xaml.dll` (core XAML runtime). This is NOT `Microsoft.UI.Xaml.Controls.dll`.
