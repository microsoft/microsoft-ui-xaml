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

Collector process operations, time, and pipe readiness are mocked. An inert
`WinUI.Coverage.PipeAcl` type records ACL requests without loading Win32 code or
changing any permissions. Always use a fresh process, not an existing collector
session.

These are script contract tests, not a validation of native binary rewriting,
PDB identity, the coverage report schema, real collector readiness/shutdown, or
Windows pipe permissions. They do not run WinUI tests or validate pipeline YAML.

## Optional native smoke test

With Visual Studio x64 C++ tools and native code coverage installed:

```powershell
pwsh.exe -NoProfile -File .\Helix\common\pipeline\coverage\tests\Run-NativeSmoke.ps1
```

This builds two tiny native DLLs and a runner, executes production instrumentation
and merge scripts, and collects two slices using the bundled real coverage tool.
It checks runtime distribution, symbol cleanup, both output formats, and executed
lines for both fixture DLLs. All generated files stay under a unique test directory
and are removed after the run.

The fixture DLLs use `/PROFILE` to emit the linker metadata needed for native
instrumentation. The smoke test does not verify the real WinUI build's linker settings.

Each collector has a unique fixture-only session. The smoke test does not call the
production collector wrapper or ACL script, change any pipe permissions, or stop
unrelated processes. It is not end-to-end WinUI or low-integrity TAEF validation.
