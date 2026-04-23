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

`bt` reads an MSBuild binary log, builds a file-level dependency graph, and
replays only the dirty compile/link/MIDL/XAML/makepri steps directly — no
MSBuild overhead.

**Use bt for the edit-build-debug cycle on source files only.**
For anything else, use the `build` skill (MSBuild).

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

Run `bt --version`.  If bt is not found, run the `install-bt.ps1` script from
this skill's directory to download and install it from GitHub Releases.

### 2. Pick the right binlog

bt requires a binary log from a prior full build.  This repo produces several
binlogs at the repo root, named by component and flavor:

| Binlog | Component |
|--------|-----------|
| `msbuild.binlog` | Last MSBuild invocation (generic) |
| `Microsoft.ui.xaml.<flavor>.binlog` | MUX (`Microsoft.UI.Xaml.dll`) |
| `Microsoft.UI.Xaml.Controls.<flavor>.binlog` | Controls DLL |
| `Microsoft.UI.Xaml-Product.<flavor>.binlog` | Full product build |
| `MUXControls.<flavor>.binlog` | MUX Controls (product + tests) |

Where `<flavor>` is e.g. `amd64chk`, `arm64fre`, `x64fre`.

**Default:** bt looks for `msbuild.binlog` in the current directory.
Use `--binlog <path>` to point at a specific one.

**Rule of thumb:** use the most specific binlog that covers your change.
For example, if you only edited files under `dxaml/xcp/`, use
`Microsoft.ui.xaml.amd64chk.binlog`.

If no binlog exists, use the `build` skill.  It produces the binlogs bt needs.

## Common workflows

### Build what's stale (most common)

```
bt build                      # mtime-based — build only dirty files
```

### Build a specific target

```
bt build MyFile.cpp            # forward walk — compile + relink affected targets
bt build MyApp.dll             # backward walk — only what this target needs
bt build MyLib.lib             # build only this library
```

### Compile only (skip link)

```
bt build -c                   # compile dirty sources, don't link
bt build -c MyApp.dll         # compile only sources feeding this target
```

### Dry run — see what would build

```
bt build -n                   # print commands without executing
```

### Watch and build on save

```
bt watch                      # build on file change
bt watch --run .\deploy.ps1   # build + run a command after
```

### Query the dependency graph

```
bt dirty                      # what needs rebuilding right now?
bt dirty MyApp.dll            # build plan scoped to a target
bt bins MyHeader.h            # what rebuilds when this header changes?
bt srcs MyApp.dll             # what sources feed into this binary?
bt srcs --headers MyFile.cpp  # upstream sources + all #include headers
```

## When to use bt vs MSBuild

| Scenario | Use |
|----------|-----|
| Edited `.cpp`/`.h`/`.idl`/`.xaml`/`.appxmanifest` | `bt build` |
| Edited resources tracked by makepri | `bt build` |
| First build of a repo | MSBuild (`build` skill) |
| Added/removed WinRT runtime classes | MSBuild (`build` skill) |
| Changed `.vcxproj`/`.vcxitems` (including adding sources) | MSBuild (`build` skill) |
| Changed `.props`/`.targets` imports | MSBuild (`build` skill) |
| Changed NuGet dependencies | MSBuild (`build` skill) |
| Need AppX packaging, signing, etc. | MSBuild (`build` skill) |
| Per-file metadata (optimization overrides) | MSBuild, then `bt build` |

## Key options

| Option | Description |
|--------|-------------|
| `--binlog <path>` | Path to binary log (default: `msbuild.binlog`) |
| `-j <N>` | Max parallel commands (default: CPU cores) |
| `-n, --dry-run` | Print commands without executing |
| `-c, --compile-only` | Compile only — skip link/lib |
| `--debounce <ms>` | Debounce delay for `watch` (default: 300) |
| `--run <cmd>` | Run command after each successful `watch` rebuild |

## Important notes

- bt replays `cl.exe`/`link.exe` directly — it does NOT invoke MSBuild
  for those tools.  CompileXaml is the exception: it invokes
  `msbuild /t:MarkupCompilePass1;SelectClCompile;MarkupCompilePass2`
  since the standalone XAML compiler is broken (~2s per project).
- The first run after a binlog change parses and caches the graph (~700ms).
  Subsequent runs use the cache (~20ms).
- Headers not yet in the tlog (newly added `#include`s) won't trigger
  rebuilds until the next `msbuild -bl`.
