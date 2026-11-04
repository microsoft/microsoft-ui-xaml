# Diagnostics injector helper

Invoke the architecture-matching executable with exactly five positional
arguments, preserving the original helper contract:

```text
WinUIProfiler.Injector.exe "<Microsoft.UI.Xaml.dll absolute path>" "<Microsoft.Internal.FrameworkUdk.dll absolute path>" "<WinUIProfiler.Tap.dll absolute path>" <endpoint-number> --trust-local-binaries
```

Write the WUP2 (or legacy WUP1) bootstrap to standard input as a single line.
There are no `--pid` or `--creation-time` arguments: target/controller PIDs,
creation times, instance GUIDs, session GUID, pipe name, and 32-byte nonce all
come from that bootstrap. Never log the bootstrap.

`endpoint-number` is decimal `1..65535`; it selects
`WinUIVisualDiagConnection<number>`. This helper calls FrameworkUdk's exported
`InitializeXamlDiagnosticsEx` for the selected target PID and TAP class ID.

Before loading trusted binaries, it verifies the target creation time, same
user SID, matching x64/ARM64 machine type, PE image architectures, and that
the exact specified XAML and FrameworkUdk DLL paths are already loaded in
the target. Paths must be local, drive-qualified absolute paths. The TAP
path must be shorter than `MAX_PATH`.

The explicit trust switch authorizes loading those caller-supplied native
binaries; the helper does not verify their signatures. It does not terminate,
suspend, or modify the target outside the diagnostics initialization call.

Standard output is one JSON result:

```json
{"schemaVersion":1,"hresult":0,"injectionAccepted":true}
```

Failure diagnostics go to standard error; the exit code is the HRESULT.
`injectionAccepted` means the initializer accepted the request, not that the
TAP is authenticated or that hello succeeded. The controller must connect,
verify pipe-server ownership, authenticate, and exchange hello before
declaring the profiler connection ready.
