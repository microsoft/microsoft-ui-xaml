---
name: bt-build
description: >-
  Fast incremental C++/MSBuild builds using bt. Use when the user wants to
  compile, build, check what's dirty, or query build dependencies in a repo
  that has .vcxproj files. Prefer bt over invoking MSBuild directly for
  inner-loop source changes (.cpp, .h, .idl, .xaml, .appxmanifest).
allowed-tools: shell
---

# bt — Fast Inner-Loop Builds

`bt` reads an MSBuild binary log and replays only the dirty
compile/link/MIDL/XAML/makepri steps directly — no MSBuild overhead.

**Use bt for the edit-build-debug cycle on source files only.**
For anything else, use the `build` skill (MSBuild).

Headers not yet in the tlog (newly added `#include`s) bt won't trigger rebuilds
until the next `msbuild -bl`.

## Do NOT use bt when

Any of these are true → use the `build` skill (MSBuild) instead:

- `.vcxproj` or `.vcxitems` files were added, removed, or edited
- `.props` or `.targets` files were changed
- NuGet package dependencies changed
- WinRT runtime classes were added or removed
- Packaging, signing, or AppX bundling is needed
- First build of the repo (no binlog exists yet)
- You are unsure whether bt covers the change

When in doubt, use MSBuild.

## Before you start

### 1. Check if bt is installed

Run `bt --version`.  If bt is missing, run the `install-bt.ps1` script from
this skill's directory to download and install it.  Periodically run it to
update bt as well.

### 2. Pick the right binlog

bt needs a binlog from a prior MSBuild build.  The repo's build scripts
all write to `BuildOutput\` with names like:

| Binlog                                       | Scope                                                            |
|----------------------------------------------|------------------------------------------------------------------|
| `Microsoft.UI.Xaml.<flavor>.binlog`          | **full product** (incl. Controls DLL) — prefer for product code  |
| `MUXControls.<flavor>.binlog`                | test apps + infra — prefer for test code                         |
| `Microsoft.ui.xaml.<flavor>.binlog`          | `Microsoft.UI.Xaml.dll` only — **NOT** Controls (mux subset)     |
| `Microsoft.UI.Xaml-Product.<flavor>.binlog`  | full product (alternate) — fallback for product code             |
| `Microsoft.UI.Xaml.Controls.<flavor>.binlog` | Controls DLL only — narrow fallback                              |
| `XamlCompilerPrerequisites.<flavor>.binlog`  | compiler tools — not used by bt                                  |

`<flavor>` is `amd64chk`, `arm64fre`, etc.  Single-project builds via
`msb`/`build-clang` produce `<project>.<flavor>.binlog` next to these.

To pick:

1. **Classify the change** by edited paths — test (table below) / mux
   (`dxaml/xcp/` only) / **product** (everything else under `controls/dev`,
   `controls/idl`, `dxaml/`, `src/`).
2. **Pick the freshest binlog whose scope covers that classification**,
   preferring broader scopes (product covers Controls, etc.).
3. **Infer `<flavor>`** from the chosen filename.
4. **Pass `--binlog BuildOutput\<chosen>`** to every bt invocation.
   bt has no useful default to fall back to.

Caveats:

- On NTFS, `Microsoft.UI.Xaml.<flavor>.binlog` and
  `Microsoft.ui.xaml.<flavor>.binlog` collide — content is whoever wrote
  last; trust mtime + size, not casing.
- If product and test binlogs exist with mtimes >1h apart, warn that the
  set may be inconsistent.
- If no binlog exists, run the `build` skill.

#### Test path classification

Anything under these directories is **test** scope:

- `controls/test/MUXControls.Test/`
- `controls/test/MUXControlsTestApp/`
- `controls/test/TabViewTearOutApp/`
- `controls/test/TestAppCX/`
- `controls/test/IXMPTestApp/`
- `controls/test/MUXTestInfra/` *(if present)*
- `Samples/AppTestAutomationHelpers/`

## Common workflows

| Command | What it does |
|---------|--------------|
| `bt build` | Build everything dirty (always pass `--binlog BuildOutput\<x>.binlog`) |
| `bt build MyFile.cpp` | Forward walk: compile + relink anything affected by this source |
| `bt build MyApp.dll` | Backward walk: only what this target needs |
| `bt build -c [target]` | Compile only — skip link/lib |
| `bt build -n` | Dry run — print commands without executing |
| `bt watch [--run <cmd>]` | Build on file change; optionally run a command after |
| `bt [--binlog some.binlog] dirty [target]` | What needs building now? |
| `bt bins MyHeader.h` | What rebuilds when this header changes? |
| `bt srcs [--headers] MyApp.dll` | Sources (and optionally `#include`d headers) feeding this target |

Run `bt --help` for command options (`--binlog <path>`),
`bt <subcommand> --help` for subcommand options like `-j <N>`,
`--debounce <ms>`, etc.
