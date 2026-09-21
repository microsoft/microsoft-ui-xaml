# AGENTS.md

Guidance for AI agents working in the WinUI repository.

## Build the repository

Run this from the repository root. It initializes the repository on first use, runs the
build, and returns a non-zero exit code if the build fails:

```powershell
.\.github\skills\build\Invoke-AgentBuild.ps1
```

Allow at least **300 seconds**, and treat over an hour as normal for a first full build.

Clone to a short path such as `C:\mx`. The repository contains paths long enough that
`git checkout` fails with `Filename too long` when the clone sits under a deep directory,
before any build starts.

**Do not call `build.cmd` directly and trust its exit code.** It can exit with code `0`
after a failed build, because the failing code is not preserved on the way out of the
script. An agent that trusts it will continue on a broken tree.

Targets, flavors, build flags, the failure recovery policy, and troubleshooting are in
[`.github/skills/build/SKILL.md`](.github/skills/build/SKILL.md). Read it before building
anything other than the default, and whenever a build fails.

## Terminology

**MUX** is `Microsoft.UI.Xaml.dll`, the core XAML runtime. It is not
`Microsoft.UI.Xaml.Controls.dll`.
