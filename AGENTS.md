# AGENTS.md

Guidance for AI agents working in the WinUI repository. This file is the canonical
source; `.github/copilot-instructions.md` and `.github/skills/build/SKILL.md` point here.

## Build the repository

Run this from the repository root. It initializes the repository on first use, runs the
build, and returns a non-zero exit code if the build fails:

```powershell
.\.github\skills\build\Invoke-AgentBuild.ps1
```

Useful variations:

```powershell
.\.github\skills\build\Invoke-AgentBuild.ps1 -Target mux       # Microsoft.UI.Xaml.dll only
.\.github\skills\build\Invoke-AgentBuild.ps1 -Target product   # product code, no tests
.\.github\skills\build\Invoke-AgentBuild.ps1 -Flavor arm64fre  # different flavor
.\.github\skills\build\Invoke-AgentBuild.ps1 -Detailed         # full build output
```

Allow at least **300 seconds**, and treat over an hour as normal for a first full build.

### Why not call `build.cmd` directly

The underlying command is:

```powershell
.\initrun.ps1 .\build.cmd /q
```

That works, and it is the right command to give a person. It is a poor fit for an
unattended agent for two reasons:

- It requires a full `init.cmd` to have been run at least once, and fails in a way that
  looks like a code error when it has not.
- **`build.cmd` can exit with code `0` after a failed build**, because the failing exit
  code is not preserved on the way out of the script. An agent that trusts the exit code
  will continue on a broken tree.

`Invoke-AgentBuild.ps1` wraps those scripts without modifying them. It initializes on
first use and derives the real result from the build output and the binary log, so the
exit code can be trusted. It prints the binary log path on success and on failure.

## Recovery policy

When a build fails, apply only these bounded steps. Do not ask the user to run commands,
and never repeat the same failing command indefinitely:

1. Missing tools, packages, or restore outputs — run `.\init.cmd <flavor>` once, then retry.
2. `C3859` or `C1076` (precompiled header memory) — retry once with `-BuildArguments /b`.
3. `C1853` (stale precompiled header) — retry once with `-BuildArguments '/c','/b'`.
4. `MSB4217` task host exit, or a following `MSB4027` — retry once with `-BuildArguments /m:1`.
5. Anything else — stop, and report the failing project, the error, and the binary log path.

Never report success from a smaller build than the one you were asked for. A plain
"build the repo" request succeeds only when the full default build succeeds.

## What to build after a change

| Files changed in | Build |
|---|---|
| `dxaml/xcp/**` | `.\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"` |
| `controls/dev/**`, `controls/idl/**` | `.\initrun.ps1 msb /q "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"` |
| `dxaml/test/native/external/<area>/**` | `.\initrun.ps1 msb /q "dxaml\test\native\external\<area>\Microsoft.UI.Xaml.Tests.External.<Area>.vcxproj"` |
| `.vcxproj`, `.vcxitems`, `.props`, `.targets`, NuGet dependencies | Full build |
| `src/XamlCompiler/BuildTasks/**/*.tt` | `.\initrun.ps1 msb /q /t:TransformAll "src\XamlCompiler\Microsoft.UI.Xaml.Markup.Compiler.csproj"`, then a full build |
| Several areas, or unsure | Full build |

## Flavors

`amd64chk` (default), `amd64fre`, `x86chk`, `x86fre`, `arm64chk`, `arm64fre`.
`chk` is debug, `fre` is release.

## Terminology

**MUX** is `Microsoft.UI.Xaml.dll`, the core XAML runtime. It is not
`Microsoft.UI.Xaml.Controls.dll`.

## More detail

- Build targets, flags, and troubleshooting: [`.github/skills/build/SKILL.md`](.github/skills/build/SKILL.md)
- Running tests on a VM: `.github/skills/test-on-vm/SKILL.md`, when present.

Some skills referenced by internal documentation, such as fast incremental builds, depend
on tooling that is only available in internal clones. When the matching skill file is not
present in the repository, ignore it and use the commands above.
