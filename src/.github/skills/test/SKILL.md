---
name: test
description: Run tests in the WinUI repository. Use when asked to run, execute, or verify tests.
---

# Running WinUI Tests

WARNING: Running tests is currently only supported for Microsoft internal users.  Some of the below
scripts aren't yet available publicly.

## AI Agent Quick Start

```powershell
# Run a specific test on a remote VM (preferred)
.\initrun.ps1 run-tests-vm.ps1 -IPAddress <ip> <testname>

# Run tests matching a wildcard
.\initrun.ps1 run-tests-vm.ps1 *CommandBar*

# Default smoke test (if user doesn't specify a test)
.\initrun.ps1 run-tests-vm.ps1 CalendarViewIntegrationTests::ValidateUIElementTree
```

If the user hasn't provided a VM IP address and the script reports no active connection, ask the user for it.

## Build Before Testing

You must build before running tests. Build the product DLL if product code changed, and/or the test DLL if test code changed.

```powershell
# Start with a full build
.\initrun.ps1 .\build.cmd

# If you've already done a build above, you can just build the specific product or test DLL you need.
```

See the build skill for the full file-to-project mapping.

## Test Methods

### Remote VM (preferred, no admin required)

```powershell
.\initrun.ps1 run-tests-vm.ps1 -IPAddress <ip> <testname>
```

Connects via tshell. If you have an active connection, `-IPAddress` can be omitted.

### Local (requires admin)

```powershell
.\initrun.ps1 run-tests-local.ps1 <testname>
```

## Wildcards

Test names support wildcards:

```powershell
.\initrun.ps1 run-tests-vm.ps1 *CommandBar*        # all CommandBar tests
.\initrun.ps1 run-tests-vm.ps1 *TabView*Close*     # narrow down further
```

## Known Issues

**Tests require a visible desktop session.** UI tests will fail if the Remote Desktop window is minimized, the screen is locked, or another window has focus. Look for errors like `RpcSetForegroundWindow` or `SetForegroundWindow: Failed` — they mean the session isn't in the foreground.
