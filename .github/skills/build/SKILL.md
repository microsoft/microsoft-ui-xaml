---
name: build
description: Build the WinUI repository. Use when asked to build, compile, or rebuild the project after making code changes.
---

# Building WinUI

## AI Agent Quick Start

```powershell
# -EnsureInitialized performs first-time setup automatically when needed.
# Default flavor is amd64chk. Override with -Flavor.

.\initrun.ps1 -EnsureInitialized .\build.cmd /q                  # full repo build (product + tests) — USE THIS BY DEFAULT
.\initrun.ps1 -EnsureInitialized .\build.cmd /q product          # product code only (no tests)
.\initrun.ps1 -EnsureInitialized .\build.cmd /q mux              # MUX only (Microsoft.UI.Xaml.dll)
.\initrun.ps1 -EnsureInitialized msb /q "path\to\project.vcxproj" # build a single project
.\initrun.ps1 -EnsureInitialized -Flavor arm64fre .\build.cmd /q # different flavor
```

## Prefer bt for inner-loop builds (internal clones only)

If `.github/skills/bt-build/SKILL.md` is present and the only changes are to **source files**
(`.cpp`, `.h`, `.idl`, `.xaml`, `.appxmanifest`), use the **`bt-build` skill** instead of
MSBuild. bt skips MSBuild entirely, replaying only the dirty compile/link steps in seconds.

`bt` ships from an internal feed, so it is unavailable in public GitHub clones. When that skill
file does not exist, ignore this section and use MSBuild for every build.

**Use MSBuild (this skill) when any of these are true:**
- `.vcxproj` / `.vcxitems` files were added, removed, or edited
- `.props` / `.targets` files were changed
- NuGet package dependencies changed
- WinRT runtime classes were added or removed
- Packaging, signing, or AppX bundling is needed
- First build (no binlog exists yet)
- You are unsure whether bt covers the change

**Rules:**
- Always use `.\initrun.ps1 -EnsureInitialized` for agent-driven builds
- Always pass `/q` for quiet output (errors only)
- Set `initial_wait` to at least **300 seconds** — builds take 1-10+ minutes
- Do not ask the user to run setup or build commands; run them yourself
- **When the user asks to "build the repo" or just "build" without specifying a target, use `.\initrun.ps1 -EnsureInitialized .\build.cmd /q` (full build).**
Only use `mux` or a single project when the user asks for a specific component or when you know exactly which files changed.

## Autonomous Recovery Policy

When a build fails, diagnose the error and apply only these bounded recovery steps without
asking the user to run commands:

1. Missing tools, packages, or restore outputs: run `.\init.ps1 <flavor>` and retry once.
2. `C3859` or `C1076` PCH memory failures: retry once with `/b`.
3. `C1853` stale compiler/PCH failures: retry once with `/c /b`.
4. `MSB4217` task-host exit or its follow-on `MSB4027`: retry once with `/m:1`.
5. Otherwise, stop and report the failing project, error, exit code, and binlog path.

Never repeat the same failed command indefinitely, hide an error, or claim success based on
a smaller build. A generic "build the repo" request succeeds only when the complete default
build exits with code 0.

## First-Time Setup

A full initialization must run once per flavor to download tools and NuGet packages.
For agent-driven builds, `-EnsureInitialized` performs this automatically.

For a fresh public GitHub clone, build the branch you were asked to build. The default branch
(`main`) and the mirrored source branch (`winui3/main`) both contain a complete, buildable tree,
so no branch switch is required unless the user names one:

```powershell
.\initrun.ps1 -EnsureInitialized .\build.cmd /q
```

Initialization is successful only when the command exits with code 0 and prints
`Initialized environment`. Do not continue to a build after an initialization error.

For manual troubleshooting, initialization can still be run directly:

```powershell
.\init.ps1                # default: amd64chk
.\init.ps1 amd64fre       # specific flavor
```

Set `initial_wait` to at least **300 seconds** — the first init downloads tools and restores NuGet packages.

Flavors: `amd64chk`, `amd64fre`, `x86chk`, `x86fre`, `arm64chk`, `arm64fre` (`chk` = debug, `fre` = release)

If a build reports missing dependencies, run full initialization once more and retry
the original build. Do not repeatedly retry the same failure.

## Commands

| Command | What it builds | Time |
|---------|---------------|------|
| `.\initrun.ps1 -EnsureInitialized .\build.cmd /q` | Everything (product + tests) | 10+ min |
| `.\initrun.ps1 -EnsureInitialized .\build.cmd /q mux` | `Microsoft.UI.Xaml.dll` only | 1-6 min |
| `.\initrun.ps1 -EnsureInitialized .\build.cmd /q product` | Product code (no tests) | 5-10 min |
| `.\initrun.ps1 -EnsureInitialized .\build.cmd /q /c` | Clean + full rebuild | 15+ min |
| `.\initrun.ps1 -EnsureInitialized msb /q "<project>"` | Single `.vcxproj` | 5s - 5 min |

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
| `src/XamlCompiler/BuildTasks/**/*.tt` | `.\initrun.ps1 msb /q /t:TransformAll "src\XamlCompiler\Microsoft.UI.Xaml.Markup.Compiler.csproj"` followed by usual build of repository |
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
   .\initrun.ps1 -EnsureInitialized .\build.cmd /q /b
   ```
2. If `/b` still fails, close other memory-intensive applications (browsers, VS instances, etc.).
3. If it keeps failing, stale PCH files from a previous build with a different compiler version may be the cause. Do a clean build:
   ```powershell
   .\initrun.ps1 -EnsureInitialized .\build.cmd /q /c /b
   ```

### `error C1853: precompiled header file is from a different version of the compiler`
**Symptom:** Build fails saying the `.pch` file is from a different compiler version.

**Root Cause:** Stale precompiled header files remain from a previous build with a different compiler (e.g., after a VS update).

**Fix:** Do a clean build with `/c`:
```powershell
.\initrun.ps1 -EnsureInitialized .\build.cmd /q /c /b
```

### `MSB4217: Task host node exited prematurely`
**Symptom:** An isolated MSBuild custom task exits unexpectedly, sometimes followed by `MSB4027`.

**Fix:** Retry once with a single MSBuild process:
```powershell
.\initrun.ps1 -EnsureInitialized .\build.cmd /q /m:1
```

If the serial retry fails, stop and report both errors, the exit code, and the binlog. Do not loop.

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

**Root Cause:** The Azure Artifacts credential provider could not authenticate to a configured dependency feed.

**Fix:**
1. Run `.\initrun.ps1 -EnsureInitialized .\build.cmd /q` once more so initialization can reinstall the credential provider and retry restore.
2. If authentication still fails, report the feed URL and HTTP status from the restore output. Do not request or print credentials.
3. Confirm the clone is on a public branch (`main` or `winui3/main`) and uses the repository's public `NuGet.config`.

### dotnet-install fails to download SDK
**Symptom:** `init.ps1` fails while downloading the .NET SDK.

**Root Cause:** Network connectivity issue or the download URL has changed.

**Fix:**
1. Check your internet connection and VPN
2. Retry — transient network errors are common
3. If the URL has changed, check `Version.props` for the expected SDK version and install it manually from https://dotnet.microsoft.com/download
