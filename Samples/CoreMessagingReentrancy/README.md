# CoreMessaging reentrancy comparison

This Win32 sample builds two executables from the same source:

- `CoreMessagingReentrancy.System.exe` creates a session through
  `CoreUICreate` in inbox `CoreMessaging.dll`.
- `CoreMessagingReentrancy.Lifted.exe` creates a session through
  `CoreMsgCreateSession` in app-local `CoreMessagingXP.dll`.

Both variants query the shared private `IMessageSessionStable` interface and
use its `DeferInvoke` operation. The test dispatches callback A, starts a worker
that defers callback B, and runs a nested `PeekMessage`/`DispatchMessage` loop
inside A.

The lifted project stages `CoreMessagingXP.dll` from the repository's resolved
`LiftedIXPRuntimePath`. Older Interactive Experiences payloads may not
implement `IMessageSessionStable` and are not compatible with this harness.

Expected ordering:

```text
System CoreMessaging:
  A ENTER
  A EXIT
  B RUNS after A exited

CoreMessagingXP:
  A ENTER
  B RUNS while A is active
  A EXIT
```

The test records the loaded paths for `CoreMessaging.dll` and
`CoreMessagingXP.dll` so the observed behavior can be associated with the
actual runtime binaries.

## Build

From the repository root:

```powershell
.\initrun.ps1 msb /q /p:Configuration=Debug /p:Platform=x64 "Samples\CoreMessagingReentrancy\System\CoreMessagingReentrancy.System.vcxproj"
.\initrun.ps1 msb /q /p:Configuration=Debug /p:Platform=x64 "Samples\CoreMessagingReentrancy\Lifted\CoreMessagingReentrancy.Lifted.vcxproj"
```

Run each executable and select **Run nested-pump test**.

## Automated mode

Pass `--auto` to run once and exit. The process returns zero when the observed
behavior matches the expected provider behavior and writes
`<executable-name>.result.txt` beside the executable.

```powershell
CoreMessagingReentrancy.System.exe --auto
CoreMessagingReentrancy.Lifted.exe --auto
```

This test demonstrates DispatcherQueue/CoreMessaging callback reentrancy. It
does not claim that every arbitrary Win32 message is suppressed by system
CoreMessaging during a nested pump.
