<!-- Explains how to run the standalone coverage regression suite and native smoke test. -->

# Coverage script regression tests

Run in a fresh process from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Helix\common\pipeline\coverage\tests\Run-Tests.ps1
pwsh.exe -NoProfile -File .\Helix\common\pipeline\coverage\tests\Run-Tests.ps1
```

Requirements: Windows, .NET Framework's C# compiler, and Pester 3.4.0 (included
with Windows PowerShell). No downloads, Visual Studio installation, or WinUI
test payload are required.

The tests execute the production scripts against unique fixture directories
inside this directory. A small executable replaces the coverage tool and
vswhere. It records arguments, returns native exit codes, and creates fixture
binaries, runtimes, and reports. Each run removes only its own fixture root.

Collector startup and pipe readiness are mocked in most tests. Shutdown clients
run as real processes to exercise Windows PowerShell exit-code handling and log
capture. Three shutdown tests also run a collector fixture with a unique pipe that
withholds its reply, verifying wall-clock timeout, cleanup, and test-result
preservation. Account discovery is mocked. Collection settings tests cover
distinct collector/console accounts, deduplication, no logged-in console user,
discovery and SID-resolution failures, and stale-settings cleanup. They verify
that the generated `AllowedUsers` configuration preserves the checked-in
instrumentation settings. Always use a fresh process, not an existing collector
session.

Collector-exit tests check successful and failed real collectors that exit before
the shutdown client returns. Nonzero or unavailable collector exit codes must
produce warnings without replacing passing or failing test results.

An empty-slice regression calls the production test runner through the coverage
callback with no matching work items. It verifies that the runner's `exit 0`
returns to the callback, clears a prior nonzero exit code, and still allows the
real fixture collector to shut down and write its report. Only the runner's
unrelated `C:\dumps` existence check is mocked to keep it inside the fixture.

An existing-session regression uses a real fixture pipe and verifies that the
wrapper fails without starting tests, changing permissions, or sending shutdown.
Native smoke lifecycle regressions invoke its collection helper with fixture
executables to cover successful shutdown, stalled clients and collectors, and
failure cleanup. Process-exit assertions allow up to five seconds for termination.

These are script contract tests, not a validation of native binary rewriting,
PDB identity, the coverage report schema, real VS collector readiness, or
Windows pipe permissions. They also check the merge job's success condition,
test dependency wiring, and successful-slice artifact filter. They do not run
WinUI tests, expand pipeline YAML, or simulate Azure's retry scheduling.

## Optional native smoke test

With Visual Studio x64 C++ tools and native code coverage installed:

```powershell
pwsh.exe -NoProfile -File .\Helix\common\pipeline\coverage\tests\Run-NativeSmoke.ps1
```

This builds two tiny native DLLs and a runner, executes production instrumentation
and merge scripts, and collects two slices using the bundled real coverage tool.
It checks runtime distribution, symbol cleanup, both output formats, and executed
lines for both fixture DLLs. It also rejects garbage and truncated coverage files
alongside valid slices using the real VS merge tool. All generated files stay
under a unique test directory and are removed after the run.

The fixture DLLs use `/PROFILE` to emit the linker metadata needed for native
instrumentation. The smoke test does not verify the real WinUI build's linker settings.

Each collector has a unique fixture-only session. The smoke test does not call the
production collector wrapper, change any pipe permissions, or stop
unrelated processes. It is not end-to-end WinUI or low-integrity TAEF validation.

Shutdown gives the client and collector a shared 60-second budget, configurable
with `-ShutdownTimeoutSeconds`. Failure cleanup terminates only the owned
processes and waits up to five seconds for each to exit; it does not retry the
shutdown command.
