# XAML Compiler Unit Tests

Standalone test build & run instructions for `XamlCompilerUnitTests.csproj` and a record of the changes that make this test suite buildable and runnable on a dev box without invoking the full `build.cmd prodtest` every time.

Baseline result on a clean amd64chk build: **244 passed / 21 failed / 49 skipped (314 total)**.

---

## Contents

1. [Quick start](#quick-start)
2. [Prerequisites](#prerequisites)
3. [Build the test project](#build-the-test-project)
4. [Run the tests (`runtests.cmd`)](#run-the-tests-runtestscmd)
5. [Summary of changes that make this work](#summary-of-changes-that-make-this-work)
6. [Troubleshooting](#troubleshooting)

---

## Quick start

From a "Developer Command Prompt for VS 2022" opened at the repo root:

```cmd
REM First-time only: produce native artifacts (GenXbf.dll, Microsoft.UI*.winmd/.dll, Markup.Compiler.dll, etc.)
build.cmd prodtest

REM Build the test DLL
msbuild src\XamlCompiler\Tests\UnitTests\XamlCompilerUnitTests.csproj ^
    /p:Configuration=Debug /p:Platform=x64 /restore ^
    /p:RuntimeIdentifiers="win;win10-x64;win10-x86;win10-arm64"

REM Stage everything into UnitTestingBin\ and run vstest
cd src\XamlCompiler
runtests.cmd
```

You should see something like:

```
Total tests: 314
     Passed: 246
     Failed:  21
    Skipped:  49
```

---

## Prerequisites

| Requirement | How to satisfy it |
|---|---|
| Windows 10 / 11 | (any supported version) |
| Visual Studio 2022 (Enterprise, Professional, Community, or BuildTools) — for MSBuild + `vstest.console.exe` | Standard install. Or, on a machine without VS, run `init.cmd` from the repo root; it populates `.buildtools\` with everything `runtests.cmd` needs. |
| .NET Framework 4.7.2 reference assemblies | Installed by VS by default. Lives under `%ProgramFiles(x86)%\Reference Assemblies\Microsoft\Framework\.NETFramework\v4.7.2\Facades`. |
| The full repo build outputs (from `build.cmd prodtest`) | Run `build.cmd prodtest` once. Produces `GenXbf.dll`, `Microsoft.UI.Xaml.Markup.Compiler.dll`, `Microsoft.UI.Xaml.winmd`, `Microsoft.UI.winmd`, `Microsoft.UI.dll`, `Microsoft.ui.xaml.dll` under `BuildOutput\bin\amd64chk\…`. The test project's `CopyPrebuiltArtifacts` target picks them up from there. |

---

## Build the test project

The test project is **not** part of `build.cmd`'s target solutions. Build it explicitly:

```cmd
msbuild src\XamlCompiler\Tests\UnitTests\XamlCompilerUnitTests.csproj ^
    /p:Configuration=Debug /p:Platform=x64 /restore ^
    /p:RuntimeIdentifiers="win;win10-x64;win10-x86;win10-arm64"
```

Notes:

- **`Configuration=Debug` + `Platform=x64`** → maps to `amd64chk` in this repo's `BuildOutput\bin\…` layout (configured by `eng\configuration.props`).
- `/restore` runs the NuGet restore pass (needed for the SRM/Roslyn packages used by `XamlCompilerProxies`).
- `RuntimeIdentifiers` value **must have no spaces** between the semicolons, otherwise NuGet treats them as separate RIDs and fails.
- `DisableWarnForInvalidRestoreProjects=true` suppresses noisy "project doesn't support restore" warnings.

Build outputs:

```
BuildOutput\obj\amd64chk\src\XamlCompiler\Tests\UnitTests\XamlCompilerUnitTests\
    UnitTests.dll          ← the test assembly
    UnitTests.pdb
    Win8Xaml.CompilerProxies.dll / .pdb
    Microsoft.UI.Xaml.Markup.Compiler.dll (copied from build.cmd output)
    Microsoft.UI.Xaml.winmd / Microsoft.UI.winmd / Microsoft.UI.dll
    Microsoft.ui.xaml.dll
    GenXbf.dll
    TestMasters\           ← XAML codegen fixture files
    LibManagedDll.pdb / LibManagedDllSatellite.pdb / LibManagedWinmd.pdb
```

The three `LibManaged*.dll` files build into their own sub-folders:

```
BuildOutput\obj\amd64chk\src\XamlCompiler\Tests\UnitTests\LibManagedDll\LibManagedDll\LibManagedDll.dll
BuildOutput\obj\amd64chk\src\XamlCompiler\Tests\UnitTests\LibManagedDllSatellite\LibManagedDllSatellite\LibManagedDllSatellite.dll
BuildOutput\obj\amd64chk\src\XamlCompiler\Tests\UnitTests\LibManagedWinmd\LibManagedWinmd\LibManagedWinmd.winmd
```

`runtests.cmd` knows where to find each of these.

---

## Run the tests (`runtests.cmd`)

`runtests.cmd` lives one folder up at `src\XamlCompiler\runtests.cmd`. From the repo root:

```cmd
cd src\XamlCompiler
runtests.cmd
```

### CLI options

```
runtests.cmd [options] [extra-vstest-args]

OPTIONS
  /config:<name>      Build flavor folder under BuildOutput\obj\.
                      Examples: amd64chk (default), amd64fre, x86chk.
  /platform:<name>    Platform part only (amd64, x86, arm64).
                      Combined with /flavor (default flavor: chk).
  /flavor:<chk|fre>   Configuration part only.
                      Combined with /platform (default platform: amd64).
  /vstest:<path>      Absolute path to vstest.console.exe.
  /? or /help         Print help.

Any unrecognised arg is forwarded verbatim to vstest.console.exe, so
flags like /TestCaseFilter:..., /Logger:trx, /Blame, /Diag work.
```

### Examples

```cmd
runtests.cmd
runtests.cmd /config:amd64fre
runtests.cmd /platform:x86 /flavor:chk
runtests.cmd /TestCaseFilter:"FullyQualifiedName~Basic01"
runtests.cmd "/Logger:trx;LogFileName=full-run.trx"
runtests.cmd /vstest:"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\Extensions\TestPlatform\vstest.console.exe"
```

### Environment variable overrides

| Var | Same effect as |
|---|---|
| `XAML_TESTS_CONFIG` | `/config:` |
| `VSTEST_CONSOLE` | `/vstest:` |

### What `runtests.cmd` does, step by step

1. **Computes `REPO_ROOT`** from its own location (`%~dp0`) — location-independent.
2. **Parses CLI flags** and resolves the final build config (`amd64chk` by default).
3. **Locates `vstest.console.exe`** in this order:
   1. `/vstest:<path>` CLI arg
   2. `%VSTEST_CONSOLE%` env var
   3. `.buildtools\Common7\IDE\Extensions\TestPlatform\vstest.console.exe` (in-repo)
   4. `vswhere -latest` → any VS install
   5. Well-known VS 2022 install paths (Enterprise / Professional / Community / BuildTools)
4. **Stages every required file** into `src\XamlCompiler\Tests\UnitTests\UnitTestingBin\`:
   - `xcopy /Y /Q /E /I` of the entire XamlCompilerUnitTests obj output (so `TestMasters\` comes along recursively).
   - Individual `copy /Y` for `LibManagedDll.dll`, `LibManagedDllSatellite.dll`, `LibManagedWinmd.winmd`.
   - `XamlCompiler.exe` + its `Microsoft.Build.*` / `System.*` / `Microsoft.UI.Xaml.Markup.Compiler.{IO,MSBuildInterop}.dll` dependencies.
   - `test.runsettings` from the source folder.
5. **Reports every missing file in one pass** (not just the first), so you see the full list of what needs to be built.
6. **Runs vstest** from inside the staging folder:
   ```cmd
   vstest.console.exe UnitTests.dll /Settings:test.runsettings <forwarded args>
   ```
7. **Returns vstest's exit code** (0 = all pass, 1 = some failures, 2 = vstest not found, 3–7 = staging problems).

### `test.runsettings`

```xml
<RunSettings>
  <MSTest>
    <DeploymentEnabled>false</DeploymentEnabled>     <!-- the critical one -->
  </MSTest>
</RunSettings>
```

`DeploymentEnabled=false` is **load-bearing**. Without it MSTest's default `true` would create a per-run `TestResults\Deploy_…\Out\` folder and copy only `[DeploymentItem]`-tagged files there, leaving every non-tagged dep (`Win8Xaml.CompilerProxies.dll`, `Microsoft.UI.Xaml.Markup.Compiler.IO.dll`, the `System.*` DLLs, `XamlCompiler.exe`) outside the test working directory → ~160-test regression.

---
