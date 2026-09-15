# Copilot Instructions for WinUI

Full guidance for agents is in [`AGENTS.md`](../AGENTS.md) at the repository root. The
essentials are repeated here so this file stands on its own.

## Building

```powershell
.\.github\skills\build\Invoke-AgentBuild.ps1
```

This initializes the repository on first use and returns a non-zero exit code when the
build fails. Do not trust `build.cmd`'s exit code directly: it can report `0` for a failed
build. Allow at least 300 seconds; a first full build takes over an hour.

Add `-Target mux` or `-Target product` for a smaller build, `-Flavor <flavor>` for a
different flavor, and `-Detailed` for full output.

## Rules

- Run build and setup commands yourself. Do not ask the user to run them.
- When asked to build the repository without a named target, perform the complete default
  build. Never report success based on a smaller component build.
- On failure, follow the recovery policy in [`AGENTS.md`](../AGENTS.md): apply the bounded
  retries it lists, then stop and report the error and the binary log path.

## Skills

- `.github/skills/build/SKILL.md` — build targets, flags, and troubleshooting.
- `.github/skills/test-on-vm/SKILL.md` — running interaction tests on a VM, when present.
