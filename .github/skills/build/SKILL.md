---
name: build
description: Build the WinUI repository. Use when asked to build, compile, or rebuild the project after making code changes.
---

# Building WinUI

## Quick start

```powershell
# Initializes on first use, and returns a non-zero exit code when the build fails.
.\.github\skills\build\Invoke-AgentBuild.ps1                     # full build (product + tests)
.\.github\skills\build\Invoke-AgentBuild.ps1 -Target product     # product code only
.\.github\skills\build\Invoke-AgentBuild.ps1 -Target mux         # Microsoft.UI.Xaml.dll only
.\.github\skills\build\Invoke-AgentBuild.ps1 -Flavor arm64fre    # different flavor
.\.github\skills\build\Invoke-AgentBuild.ps1 -BuildArguments /c  # clean rebuild
```

Set `initial_wait` to at least **300 seconds**. A first full build takes over an hour.

Use `-Detailed` to see full build output; by default the build runs quietly and prints
errors only.

## Use the wrapper for unattended builds

A person normally runs `.\init.cmd` once and then `.\build.cmd /q` repeatedly in that same
shell, because `init` sets environment variables and `PATH` for the session.

That does not work for an agent, which runs each command in a fresh process, so the
environment from `init` is gone by the next command. `initrun.ps1` exists for this: it
re-establishes the environment in-process and then runs the command, so each invocation is
self-contained:

```powershell
.\initrun.ps1 .\build.cmd /q
```

For an unattended agent, prefer `Invoke-AgentBuild.ps1`. It calls the same scripts and
changes none of them, but it adds three things an agent needs:

- **First-time initialization.** A full `init.cmd` must have run at least once, or the
  build fails in a way that looks like a code error. The wrapper initializes first when it
  finds the repository uninitialized. That check is deliberately simple: it confirms that
  `packages\` and `.tools\` exist, the same signal `init.cmd /envcheck` uses. It cannot
  tell that a restore was interrupted or has gone out of date, so a partial restore passes
  the check and the build then fails for missing packages. See
  [missing packages or tools](#missing-packages-or-tools) when that happens.
- **A trustworthy exit code.** `build.cmd` can exit `0` after a failed build, because the
  failing exit code is not preserved on the way out of the script. The wrapper derives the
  real result from the build output and the binary log.
- **The binary log path**, reported on success as well as failure.

If you do call `build.cmd` directly, do not treat exit code `0` as proof of success.
Check the output for `ERROR:` lines and confirm a binary log was written under
`BuildOutput\`.

## Recovery policy

When a build fails, diagnose the error and apply only these bounded recovery steps.
Run them yourself; do not ask the user to.

1. Missing tools, packages, or restore outputs — run `.\init.cmd <flavor>` once and retry.
2. `C3859` or `C1076` precompiled header memory failures — retry once with `-BuildArguments /b`.
3. `C1853` stale precompiled header failures — retry once with `-BuildArguments '/c','/b'`.
4. `MSB4217` task host exit, or a following `MSB4027` — retry once with `-BuildArguments /b`.
5. Anything else — stop and report the failing project, the error, the exit code, and the
   binary log path.

Never repeat the same failed command indefinitely, hide an error, or claim success from a
smaller build. A generic "build the repo" request succeeds only when the full default
build exits with code 0.

## Machine setup

On a machine that has never built WinUI, `Invoke-AgentBuild.ps1` sets the machine up
before it initializes the repository:

- **Visual Studio.** If none is installed, the installer is downloaded and Visual Studio
  is installed with every component in `.vsconfig` at the repository root.
- **Missing components.** If Visual Studio is installed but is missing components
  `.vsconfig` asks for, they are added to the existing installation. `init.cmd` does not
  do this: it only probes for ATL/ARM64 and repairs against `.vsconfig_buildtools`.
- **Long path support.** Enabled if it is off. The build uses paths that exceed the
  default limit.

Nothing is installed when the machine already has it, so this costs one `vswhere` probe on
a machine that is ready.

Installing Visual Studio requires administrator rights. When the session is not elevated,
the setup step is relaunched elevated on its own, which raises one consent prompt. Where no
consent can be given, the setup that is still required is reported and the build does not
start, because a build missing those components fails later with errors that read as broken
source code.

```powershell
# Set the machine up without building. Run from an elevated prompt to avoid the prompt.
.\.github\skills\build\Invoke-AgentBuild.ps1 -SetupMachineOnly

# Build with the machine as it is, installing nothing.
.\.github\skills\build\Invoke-AgentBuild.ps1 -SkipMachineSetup
```

A Microsoft account is not needed. The repository restores from a public feed.

## First-time setup

A full initialization runs once per flavor to download tools and restore NuGet packages.
`Invoke-AgentBuild.ps1` does this automatically. To run it directly:

```powershell
.\init.cmd amd64chk
```

Allow at least 300 seconds. Initialization has succeeded when the command exits with code
0 and the `packages` and `.tools` directories exist at the repository root. Do not start a
build after an initialization error.

Flavors: `amd64chk`, `amd64fre`, `x86chk`, `x86fre`, `arm64chk`, `arm64fre`
(`chk` is debug, `fre` is release).

## Targets

| Target | What it builds | Time |
|---|---|---|
| `prodtest` (default) | Product code and tests | 1 hour or more |
| `product` | Product code, no tests | 5-10 min incremental |
| `mux` | `Microsoft.UI.Xaml.dll` only | 1-6 min incremental |
| `test` | Tests only | varies |
| `samples` | Sample applications | varies |

Single project, when you know exactly what changed:

```powershell
.\initrun.ps1 msb /q "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"
```

## Flags

Pass these through `-BuildArguments`.

| Flag | Effect |
|---|---|
| `/c` | Clean build. Deletes `BuildOutput` first. Use when switching flavors |
| `/b` | Reduced parallelism (`/m:2`). Avoids precompiled header memory exhaustion, and helps after `MSB4217` |
| `/restore` | NuGet restore before building |
| `/nomock` | Skip the mock package. Only when changing `dxaml/` product and test code |
| `/fake` | Print the commands without running them |

## What to build after a code change

| Files changed in | Build |
|---|---|
| `dxaml/xcp/**` | `.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"` |
| `controls/dev/**`, `controls/idl/**` | `.\initrun.ps1 msb /q "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"` |
| `dxaml/test/native/external/<area>/**` | `.\initrun.ps1 msb /q "dxaml\test\native\external\<area>\Microsoft.UI.Xaml.Tests.External.<Area>.vcxproj"` |
| `.vcxproj`, `.vcxitems`, `.props`, `.targets`, NuGet dependencies | Full build |
| `src/XamlCompiler/BuildTasks/**/*.tt` | `.\initrun.ps1 msb /q /t:TransformAll "src\XamlCompiler\Microsoft.UI.Xaml.Markup.Compiler.csproj"`, then a full build |
| Several areas, or unsure | Full build |

Test areas: `controls`, `foundation`, `framework`, `automation`.

## Terminology

**MUX** is `Microsoft.UI.Xaml.dll`, the core XAML runtime. It is not
`Microsoft.UI.Xaml.Controls.dll`.

## Faster incremental builds

Internal clones provide a tool that replays only the dirty compile and link steps,
skipping MSBuild. It is described by a separate skill under `.github/skills/`, and it
depends on tooling from an internal feed, so it is unavailable in public clones. When that
skill file is not present, use MSBuild for every build.

Use MSBuild rather than an incremental tool whenever project files, `.props`/`.targets`,
or NuGet dependencies changed, when WinRT runtime classes were added or removed, when
packaging or signing is needed, on a first build, or when unsure.

## Troubleshooting

### Missing packages or tools

Errors such as `NU1101`, `MSB3644`, "references NuGet package(s) that are missing", or a
tool that is "not recognized as an internal or external command" mean the repository is not
fully initialized. They are not code errors, so do not try to fix them by changing source.

This can happen even when the wrapper skipped initialization, because its check only
confirms that `packages\` and `.tools\` exist. A restore that was interrupted, or that
predates a change to the dependencies, leaves those directories in place but incomplete.
The wrapper prints this advice when it detects the case.

Initialize directly with the flavor you are building, then build again:

```powershell
.\init.cmd amd64chk
.\.github\skills\build\Invoke-AgentBuild.ps1
```

### `error C3859` or `error C1076`, precompiled header memory

Parallel compiler instances exhaust the process address space.

```powershell
.\.github\skills\build\Invoke-AgentBuild.ps1 -BuildArguments /b
```

If that still fails, close other memory-intensive applications, then try a clean build
with `-BuildArguments '/c','/b'`.

### `error C1853`, precompiled header from a different compiler version

Stale precompiled headers, typically after a Visual Studio update.

```powershell
.\.github\skills\build\Invoke-AgentBuild.ps1 -BuildArguments '/c','/b'
```

### `MSB4217: Task host node exited prematurely`

Retry once with reduced parallelism:

```powershell
.\.github\skills\build\Invoke-AgentBuild.ps1 -BuildArguments /b
```

`Build.cmd` chooses the process count itself and accepts only `/b` (`/m:2`) and `/m`
(one process per core). It rejects `/m:1` as an unrecognized option, so a fully serial
build is not available through the supported entry point.

If the reduced-parallelism retry fails, stop and report both errors, the exit code, and
the binary log.

### `git rev-parse` or another git command exits 128

Output such as `fatal: missing config value` or a project failing on `git rev-parse HEAD`
usually means the environment the agent runs in declared a git setting without a value.
Git reads `GIT_CONFIG_COUNT` with matching `GIT_CONFIG_KEY_<n>` and `GIT_CONFIG_VALUE_<n>`
pairs; an empty value is dropped when the environment reaches the build's `cmd.exe`, so
git refuses its own configuration. It is not a repository problem.

`Invoke-AgentBuild.ps1` detects an incomplete set and clears those variables for the
build, reporting which key was at fault. If you are running `build.cmd` yourself, clear
them in that shell first:

```powershell
Get-ChildItem Env: | Where-Object { $_.Name -like 'GIT_CONFIG*' } | Remove-Item
```

Do not do this in a shell you will push from: the same variables can carry credential
settings.

### Missing Spectre mitigation libraries

Import `.vsconfig` from the repository root through the Visual Studio Installer:
More, then Import configuration, then select `<repo-root>\.vsconfig` and install the
missing components.

### `DevEnvDir environment variable not set`

Informational, not an error. The initialization scripts run `DevCmd.cmd` to set up the
Visual Studio environment. No action needed.

### NuGet restore fails to authenticate

1. Run the build once more so initialization can reinstall the credential provider and
   retry the restore.
2. If it still fails, report the feed URL and the HTTP status from the restore output.
   Do not request, print, or store credentials.
3. Confirm the clone uses the repository's `NuGet.config`.

### The .NET SDK fails to download

Usually a transient network or VPN problem; retry. If the download URL has changed, check
`Version.props` for the expected SDK version.

## Verifying the build result

The wrapper reports the outcome and the binary log path. When checking a build by hand:

- A failed solution prints `ERROR: buildSolution for <solution> FAILED.` followed by the
  binary log path.
- Compiler and MSBuild diagnostics appear as `error C####`, `error MSB####`, and similar.
- A binary log is written to `BuildOutput\<name>.<flavor>.binlog`. A missing log after a
  build usually means the build crashed before it started.
