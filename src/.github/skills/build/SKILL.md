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
.\initrun.ps1 .\build.cmd /q product                    # product code only (no tests)
.\initrun.ps1 .\build.cmd /q mux                        # MUX only (Microsoft.UI.Xaml.dll)
.\initrun.ps1 msb /q "path\to\project.vcxproj"          # build a single project
.\initrun.ps1 -Flavor arm64fre .\build.cmd /q           # build for a different flavor
```

## Prefer bt for inner-loop builds

If the only changes are to **source files** (`.cpp`, `.h`, `.idl`, `.xaml`,
`.appxmanifest`), use the **`bt-build` skill** instead of MSBuild. bt skips
MSBuild entirely, replaying only the dirty compile/link steps in seconds.

**Use MSBuild (this skill) when any of these are true:**
- `.vcxproj` / `.vcxitems` files were added, removed, or edited
- `.props` / `.targets` files were changed
- NuGet package dependencies changed
- WinRT runtime classes were added or removed
- Packaging, signing, or AppX bundling is needed
- First build (no binlog exists yet)
- You are unsure whether bt covers the change

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
| `.\initrun.ps1 .\build.cmd /q mux` | `Microsoft.UI.Xaml.dll` only | 1-6 min |
| `.\initrun.ps1 .\build.cmd /q product` | Product code (no tests) | 5-10 min |
| `.\initrun.ps1 .\build.cmd /q /c` | Clean + full rebuild | 15+ min |
| `.\initrun.ps1 msb /q "<project>"` | Single `.vcxproj` | 5s - 5 min |

### Flags (for `.\build.cmd`)

| Flag | Effect |
|------|--------|
| `/q` | Quiet — errors only, plus elapsed time |
| `/b` | Reduced parallelism (`/m:2`) — prevents PCH virtual memory exhaustion on limited-memory machines |
| `/c` | Clean build — deletes BuildOutput first. Use on first build or when switching flavors |
| `/restore` | NuGet restore before building |
| `/nomock` | Skip mock package.  Use if you're only updating product and test code under`dxaml/` and don't need to run MUXControls or sample tests.) |
| `/fake` | Dry run — print commands without executing |

## What to Build After a Code Change

| Files changed in | Build command |
|---|---|
| `dxaml/xcp/**` (source only) | **bt:** `bt build` · MSBuild: `.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"` |
| `controls/dev/**` or `controls/idl/**` (source only) | **bt:** `bt build` · MSBuild: `.\initrun.ps1 msb /q "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"` |
| `dxaml/test/native/external/<area>/**` (source only) | **bt:** `bt build` · MSBuild: `.\initrun.ps1 msb /q "dxaml\test\native\external\<area>\Microsoft.UI.Xaml.Tests.External.<Area>.vcxproj"` |
| `.vcxproj`, `.vcxitems`, `.props`, `.targets`, NuGet deps | `.\initrun.ps1 .\build.cmd /q` (MSBuild only — do NOT use bt) |
| Multiple areas or unsure | `.\initrun.ps1 .\build.cmd /q` |

Test areas: `controls`, `foundation`, `framework`, `automation`

## Terminology

**MUX** = `Microsoft.UI.Xaml.dll` (core XAML runtime). This is NOT `Microsoft.UI.Xaml.Controls.dll`.

## Troubleshooting

### `error C3859: Failed to create virtual memory for PCH` / `error C1076: compiler limit: internal heap limit reached`
**Symptom:** Build fails with dozens of PCH (precompiled header) virtual memory errors across multiple .cpp files.
This typically happens when building with the default `/m:4` parallelism on machines with limited memory.

**Root Cause:** Multiple parallel cl.exe compiler instances each try to allocate large PCH memory regions, exhausting the process address space.

**Fix:**
1. Use the `/b` flag in `build.cmd` which sets `/m:2` (2 parallel processes):
   ```powershell
   .\initrun.ps1 .\build.cmd /q /b
   ```
2. If `/b` still fails, close other memory-intensive applications (browsers, VS instances, etc.).
3. If it keeps failing, stale PCH files from a previous build with a different compiler version may be the cause. Do a clean build:
   ```powershell
   .\initrun.ps1 .\build.cmd /q /c /b
   ```

### `error C1853: precompiled header file is from a different version of the compiler`
**Symptom:** Build fails saying the `.pch` file is from a different compiler version.

**Root Cause:** Stale precompiled header files remain from a previous build with a different compiler (e.g., after a VS update).

**Fix:** Do a clean build with `/c`:
```powershell
.\initrun.ps1 .\build.cmd /q /c /b
```

### Missing Spectre mitigation libs
**Symptom:** Build errors about missing Spectre mitigation libraries from Visual Studio.

**Fix:** Import the `.vsconfig` file from the repo root via Visual Studio Installer:
1. Open Visual Studio Installer
2. Click "More" → "Import configuration"
3. Select `<repo-root>\.vsconfig`
4. Install the missing components

### `DevEnvDir environment variable not set`
**Symptom:** This message appears at the start of every `initrun.ps1` command.

**Root Cause:** This is informational, not an error. `initrun.ps1` automatically runs `DevCmd.cmd` to set up the VS environment.

**Fix:** No fix needed — this is normal behavior.

### NuGet restore fails with authentication errors
**Symptom:** `init.ps1` fails during NuGet package restore with 401/403 errors.

**Root Cause:** Missing or expired Azure DevOps credentials for internal NuGet feeds.

**Fix:**
1. Ensure Azure Artifacts Credential Provider is installed (init.ps1 should do this automatically)
2. If it persists, manually authenticate:
   ```powershell
   dotnet nuget update source OSClient --username "your-alias" --password "your-PAT"
   ```
3. Or use `nuget.exe sources update` with a Personal Access Token from https://dev.azure.com/microsoft/_usersSettings/tokens

### dotnet-install fails to download SDK
**Symptom:** `init.ps1` fails while downloading the .NET SDK.

**Root Cause:** Network connectivity issue or the download URL has changed.

**Fix:**
1. Check your internet connection and VPN
2. Retry — transient network errors are common
3. If the URL has changed, check `Version.props` for the expected SDK version and install it manually from https://dotnet.microsoft.com/download
