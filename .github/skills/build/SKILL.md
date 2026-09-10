---
name: build
description: Build the WinUI repository. Use when asked to build, compile, or rebuild the project after making code changes.
---

# Building WinUI

## AI Agent Quick Start

Run build commands from the repository root through `initrun.ps1`.
**For "build the repo" without a target, use the default product-and-tests build,
not a single project.** Samples are a separate target.

```powershell
.\initrun.ps1 .\Build.cmd /q
.\initrun.ps1 -Flavor arm64fre .\Build.cmd /q  # alternative flavor
```

Set `initial_wait` to at least **300 seconds** for initialization and builds.
Full builds can take tens of minutes; wait for the existing command rather than
starting another build when a tool call times out.

## Initialization and Missing Dependencies

`initrun.ps1` runs `init.ps1 <flavor> /envcheck /notitle`. This sets up the
environment but skips package restore and tool downloads.

Run a **full init** for the required flavor only on first use or after
dependencies or tools are updated. Full init updates submodules, restores NuGet
packages, and downloads required tools and .NET SDKs:

```powershell
.\init.ps1 amd64chk
# For an ARM64 release build, use this instead:
.\init.ps1 arm64fre
```

Once full init succeeds for the current commit, the required packages and tools
should be available. If the build then fails, treat it as a likely build issue
and investigate the error instead of rerunning init. Source-only changes do not
require another full init.

Build with the **same flavor**. `initrun.ps1` defaults to
`amd64chk` on each invocation; it does not inherit a flavor from a previous init.
Pass `-Flavor` for non-default builds.

Supported architectures are `x86`, `amd64` (also accepted as `x64`), `arm64`,
and `arm64ec`. Append `chk` for Debug or `fre` for Release. Examples:
`amd64chk`, `x86fre`, `arm64chk`, `arm64ecfre`.

Visual Studio 2022 build tooling is required; `init.cmd` rejects VS 2019.
Use the root `.vsconfig` to identify required components. Full init uses
`scripts\init\Initialize-Restore.ps1` and `scripts\PostInit.ps1`; rerunning only
`initrun.ps1` is not a substitute for that restore.
If full init fails during restore or download, diagnose that error
rather than repeatedly initializing or changing package versions.

## Build Targets

| Command | Scope |
|---------|-------|
| `.\initrun.ps1 .\Build.cmd /q` | Default `prodtest`: product and tests, without the separate sample-build stage |
| `.\initrun.ps1 .\Build.cmd /q product` | Product target and local WinUI component package, without test/sample targets |
| `.\initrun.ps1 .\Build.cmd /q mux` | Core `Microsoft.UI.Xaml.dll` project and its dependencies |
| `.\initrun.ps1 .\Build.cmd /q test` | `controls\MUXControls.sln`; requires current product outputs for the local package |
| `.\initrun.ps1 .\Build.cmd /q samples` | Sample applications selected by `buildsamples.cmd`; requires current product outputs |
| `.\initrun.ps1 .\Build.cmd /q all` | Default product-and-tests build plus the sample-build stage |

`Build.cmd` builds `XamlCompilerPrerequisites.sln` first, including the managed
XAML compiler and the GenXbf prerequisite. The default build then runs
`dxaml\Microsoft.UI.Xaml.sln`, packs the local component, and builds
`controls\MUXControls.sln`. Building tests is not the same as running them.

### Flags for `Build.cmd`

| Flag | Effect |
|------|--------|
| `/q` | Quiet MSBuild output; called scripts may still print messages |
| `/b` | Use two MSBuild workers instead of the default four to reduce memory pressure |
| `/m` | Let MSBuild use all available processors; can increase memory pressure |
| `/restore` | Add MSBuild restore; does not replace full init's tool downloads |
| `/fake` | Print the build plan without running its build, packaging, or clean commands; the `initrun.ps1` wrapper still initializes the environment |
| `/c` | Clean all build flavors before restoring/building; see Clean-Build Safety below |

Do not combine `/b` and `/m`.

## Targeted Builds After Source Changes

Use targeted builds after the prerequisite build has succeeded and the owning
project is known. For shared build files, NuGet changes, added/removed runtime
classes, or uncertain scope, use the default build instead.

| Changed area | Command |
|--------------|---------|
| Core runtime implementation under `dxaml\xcp` | `.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"` |
| Regular Controls implementation, excluding Tabular and tests | `.\initrun.ps1 msb /q "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"` |
| TableView and other Tabular implementation | `.\initrun.ps1 msb /q "controls\dev\dll-tabular\Microsoft.UI.Xaml.Controls.Tabular.vcxproj"` |
| Controls test sources | `.\initrun.ps1 .\Build.cmd /q test` |
| Native external tests | `.\initrun.ps1 msb /q "dxaml\test\native\external\<area>\Microsoft.UI.Xaml.Tests.External.<Area>.vcxproj"` |
| XAML compiler or multiple areas | `.\initrun.ps1 .\Build.cmd /q` |

Find the actual external-test project rather than guessing its name. Controls
source ownership is defined by `controls\ProjectImports.targets` and
`controls\Tabular.ProjectImports.targets`; shared sources may affect both DLLs.
Tabular is included in `controls\MUXControls.sln`, but building just
`Microsoft.UI.Xaml.Controls.vcxproj` does not build the Tabular DLL.

`msb` is `tools\msb.cmd`. It accepts projects and solutions, but sets
`SkipSigning=true`; use `Build.cmd` when packaging or signing is needed.
Keep `/q` as the first argument to `msb`.

For XAML compiler `.tt` template changes, regenerate their companion `.cs` files
before the normal build. The imported TextTemplating targets do not transform
templates during a normal build by default:

```powershell
.\initrun.ps1 msb /q /t:TransformAll "src\XamlCompiler\Microsoft.UI.Xaml.Markup.Compiler.csproj"
```

## Optional bt Inner-Loop Builds

For supported source-only changes, prefer `bt` when the tool and a suitable
MSBuild binlog are available. Follow the `bt-build` skill, currently at
`src\.github\skills\bt-build\SKILL.md`, including its requirement to pass
`--binlog` explicitly. Do not use a bare `bt build`.

The bundled bt installer uses an internal Microsoft feed. If bt or feed access
is unavailable, use MSBuild rather than making public builds depend on it.
Also use MSBuild for project/dependency changes, newly included headers absent
from the previous build's tracking logs, runtime-class changes, compiler
templates, packaging/signing, or any uncertainty about bt's coverage.

## Results and Diagnostics

`Build.cmd`, `buildsamples.cmd`, and `msb` write logs under
`BuildOutput\<project-or-solution>.<arch><configuration>.binlog`, for example
`BuildOutput\MUXControls.amd64chk.binlog`. Core-only and default-build logs
named `Microsoft.ui.xaml` and `Microsoft.UI.Xaml` collide on case-insensitive
filesystems; check which command produced the latest log.

Inspect command output and the relevant binlogs before reporting success.
The current batch wrappers can mask a child command's failure, so an exit code
of zero, an elapsed-time footer, or "Initialized environment" alone is not proof
that the requested build completed. Report the failing step, error, and binlog
path; do not silently substitute a narrower target.

## Troubleshooting

| Symptom | Action |
|---------|--------|
| Missing packages or downloaded tools | Run full init only for first use or updated dependencies/tools. If it already succeeded for the current commit, investigate the build failure |
| `C3859` / `C1076` (PCH memory pressure) | Retry with `.\initrun.ps1 .\Build.cmd /q /b`, preserving any explicit flavor or target |
| `C1853` (PCH from a different compiler) | Clean and rebuild using the precautions in Clean-Build Safety |
| Missing Spectre mitigation libraries | Have the user import the root `.vsconfig` through Visual Studio Installer and install the missing components |
| `DevEnvDir environment variable not set or msbuild unavailable` | This starts `DevCmd.cmd` setup; only treat it as informational if that setup succeeds |
| A called `.cmd` script is "not recognized" even though it exists | Check command lookup. With `NoDefaultCurrentDirectoryInExePath` set, the repository root must be explicitly available on `PATH` in the build process |
| NuGet authentication errors | Check the failing feed and `NuGet.config`; full init installs the Azure Artifacts Credential Provider. Have the user complete any required sign-in |
| .NET SDK download failure during init | Diagnose the download error before retrying the failed init. `build\DownloadDotNetCoreSdk.ps1` reads SDK channels from `eng\Versions.props` and installs into `.dotnet` and `.dotnet\x86`, not the system SDK directory |

The public repo's `NuGet.config` uses `WinUI.Dependencies` and the local
`packagestore` source. Do not add unrelated internal feeds or put access tokens
in commands to work around restore errors.

## Clean-Build Safety

`Build.cmd /c` invokes `tools\clean.cmd /all`. It kills all `MSBuild.exe` and
`VBCSCompiler.exe` processes and removes this worktree's outputs for every
architecture/flavor, including `TestPayload`. This can interrupt other sessions'
builds.

After a substantial pull or rebase since the last successful build, prefer a
clean build, especially when code generation, project structure, or compiler
settings changed. Several days of accumulated development is a useful signal,
not a fixed time cutoff. Base the decision on the scope and type of changes.

The agent may also clean and rebuild when unexpected errors are consistent with
stale generated files, PCH files, or other build outputs. In either case, the
agent may perform the clean itself, preserving the requested flavor and target.

Before cleaning, check for other active builds or compiler work on the machine,
including other sessions and worktrees. If any are active, wait or coordinate
before cleaning; do not interrupt them.

A large sync requires another full init only if dependencies or tools changed.
Do not use `/c` as routine first-build, missing-package, or flavor-switch
recovery. If the same error persists after a clean rebuild, investigate it
rather than repeating the clean.
