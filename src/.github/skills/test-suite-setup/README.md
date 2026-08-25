# WinUI3 Test Suite Skill

## What is this?

This is a **Copilot CLI skill** that automates running the WinUI3 test suite locally. Instead of manually figuring out which tests to run, how to build, and how to set up your machine — just invoke the skill and it handles everything.

## What does it do?

1. **Detects your changes** — Compares your branch against `origin/main` using `git diff --name-only main...HEAD` to find changed files, then maps them to the relevant test suites (out of ~8000 total across 81 test DLLs). If `main` is not available locally, it falls back to comparing recent commits (`HEAD~5..HEAD`).

2. **Sets up everything from scratch** — If it's your first time, it automatically:
   - Delegates to the **`build` skill** to initialize and build the repo (product + tests)
   - Creates the TestPayload with all test binaries
   - Runs one-time machine setup (dev license, certificates, crash dumps)
   - Installs required runtimes (.NET Desktop, VC++ Redist)

3. **Runs targeted tests** — Executes only the test suites relevant to your changes using the correct hosting mode (WPF, UAP, or None), and reports pass/fail results.

4. **Lets you choose** — You can accept the recommended tests, pick from the full list of 23+ test suites, or enter a custom wildcard/regex pattern (e.g., `*CommandBar*`, `*ListView*`).

## Prerequisites

- **Windows 10/11** (20H2 or later)
- **Visual Studio 2022** with C++ workloads
- **Admin PowerShell** (required for test machine setup and running UI tests)
- **Visible desktop** — don't minimize RDP or lock the screen while tests run

## How to use

### Run tests for your current changes (most common)

```
run the test-suite-setup skill
```

Or ask naturally:

```
run tests for my changes
```

The skill will:
1. Check if your environment is ready (build + TestPayload)
2. Detect the comparison branch (defaults to `main`, auto-detects PR targets, asks if on a release branch)
3. Diff your branch to find changed files
4. Map changed files to the relevant test suites
5. Present the recommendation and ask you to confirm
6. Run the tests and report pass/fail results

### Compare against a specific branch

```
run tests comparing against release/2.8
run tests for my changes vs origin/servicing/2.9
```

By default the skill compares against `main`. If your branch targets a release or servicing branch, it will auto-detect and ask you to confirm. You can also specify any branch explicitly.

### Run a specific test suite or wildcard pattern

```
run CommandBar tests
run tests matching *ListView*
run External.Controls tests in WPF mode
```

The skill understands wildcard patterns and TAEF select queries, so you can target exactly the tests you need.

### Run tests for a different architecture or configuration

The default build/test flavor is `x64 Debug (amd64chk)`. To target a different platform:

```
run tests for my changes using arm64 debug
run tests with x86 release configuration
```

Supported platforms: `x86`, `x64`, `arm64`, `arm64ec`
Supported configurations: `chk` (debug), `fre` (release)

> **Note:** The build and test payload must match — if you build `arm64chk`, create the payload with `-Platform arm64 -Configuration chk` and run from `TestPayload\arm64chk\`.

### First-time full setup

If you've never run tests on this machine before:

```
set up and run the full test suite from scratch
```

This runs the complete pipeline: init → build → create payload → machine setup → install runtimes → run tests.

### Run all tests

```
run all tests across all hosting modes
```

This runs ~8000 tests across UAP, WPF, Win32Explicit, and None hosting modes (~8 hours total).

## How test discovery works

The skill uses a **hybrid approach** to map your code changes to the right tests — combining curated knowledge with dynamic discovery so it works for existing and future test areas.

### 1. Known mappings (instant)

A curated table in `SKILL.md` maps common source paths to their test DLLs. For example:
- `dxaml/xcp/components/imaging/**` → `Isolated.Foundation.Imaging`
- `controls/dev/**` → `MUXControls.Test.dll`
- `dxaml/test/native/external/controls/**` → `External.Controls`

This covers the most common areas and gives fast, accurate results.

### 2. Dynamic discovery (for new/unknown areas)

For any changed files that don't match the known table, the skill:
1. **Scans the TestPayload** to discover all available test DLLs at runtime
2. **Matches by naming convention** — extracts keywords from the changed file path and looks for test DLLs with matching names (e.g., changes in `components/foo/` → look for `*Foo*` test DLLs)
3. **Validates with TAEF metadata** — runs `te.exe /list` to confirm the DLL contains tests related to the area

This means new test areas are automatically picked up without needing to update the skill.

### 3. User fallback (last resort)

If the skill can't auto-detect tests for a changed file, it asks you:
> "I found changes in `<path>` but couldn't detect the relevant tests. Which test suite covers this?"

You pick from the dynamically discovered list, specify a custom pattern, or skip if no tests are needed.

### Hosting mode inference

The skill automatically picks the right hosting mode based on test DLL type:
| DLL pattern | Hosting Mode |
|---|---|
| `External.*` | WPF |
| `Isolated.*` | None (unit tests) |
| `Managed.*` | WPF |
| `MUXControls.Test`, `UnitTests` | None |

## Example

```
You: run the test-suite-setup skill

Copilot: Based on your branch `user/you/my-feature`, I found changes in:
  - dxaml/xcp/components/imaging/** (3 files)
  - dxaml/test/resources/** (12 files)

Recommended tests:
  1. Isolated.Foundation.Imaging (39 tests, ~1 min)
  2. External.Foundation (1621 tests, ~30 min)

> Run recommended tests (Recommended)
> Choose different tests
> Run all tests
```

## Files

| File | Purpose |
|---|---|
| `SKILL.md` | The skill definition — contains AI agent instructions, setup steps, test suite mappings, and troubleshooting |
| `README.md` | This file — quick-start guide |
