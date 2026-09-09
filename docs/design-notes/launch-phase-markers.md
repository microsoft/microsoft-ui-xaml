# XAML launch boundary observations

For the underlying startup paths, see [Startup for Xaml](startup-overview.md).

`Microsoft-Windows-XAML` (`531a35ab-63ce-4bcf-aa98-f88c7a89e455`) emits the
TraceLogging event `LaunchPhaseTransition` at six native boundaries. The event
uses level 0 and keyword 0. The provider must still be enabled; filtering, late
capture, truncation and event loss can leave incomplete observations.

## Boundaries and canonical intervals

| Ordinal | Boundary |
| --- | --- |
| 1 | Entry to `FrameworkApplicationFactory::StartImpl`. |
| 2 | Immediately before the core's first native `OnLaunchedProtected` callback. |
| 3 | Return from that callback, before unhandled-error reporting. |
| 4 | Before `UpdateLayout` in the first qualifying launch draw attempt. |
| 5 | The selected post-layout boundary in `NWDrawTree`, before the window-destroyed check. |
| 6 | At draw-attempt cleanup, after frame-end telemetry, when `pFrameDrawn` is true. |

| Phase label | Boundaries | Meaning |
| --- | --- | --- |
| FrameworkInitialization | 1 to 2 | Elapsed startup entry to callback entry, including application construction and other app work on that path. Not pure framework CPU time. |
| ApplicationInitialization | 2 to 3 | Synchronous native callback span, including any nested message pumping. Not all asynchronous initialization. |
| FirstFrameScheduling | 3 to 4 | Elapsed callback return to the selected initial-layout entry. Not solely scheduler work. |
| InitialLayout | 4 to 5 | Elapsed initial-layout entry to the post-layout boundary, including intervening callbacks and possibly retries. Not just `UpdateLayout` duration. |
| FirstFrameProduction | 5 to 6 | Elapsed post-layout boundary to qualifying draw cleanup, including callbacks, waits and possibly later draw attempts. Not pure rendering time. |

Phase values are `None=0`, the five labels above in order (`1..5`),
`Complete=6`, and `Aborted=7`. `PreviousPhase` and `NextPhase` label the canonical
sides of a boundary; they do **not** assert a transition in an ordered runtime
state machine.

`Ordinal` is a boundary identifier, **not** a chronological sequence number.
Application code can pump messages during `OnLaunched`, so a valid observation
order is `1,2,4,5,6,3`. The producer does not delay or suppress real frame
boundaries to manufacture ordered intervals. Retain actual event timestamps.
Derive the five-phase partition only when all six boundaries are captured once,
correlated to the same initialization, in chronological ordinal order, and the
callback returned successfully. Otherwise report the observed partial/nonlinear
sequence, not negative durations, guessed boundaries or a loss diagnosis.

Marker 3 uses `NextPhase=Aborted` for a failed HRESULT. This describes callback
failure only; it neither terminates nor resets the independent frame observations.
Marker 6's `Complete` means the draw observation was reached, not successful app
launch. `Result` is the callback HRESULT at boundary 3 and the current draw HRESULT
at boundary 6 (which can fail after submission); it is zero elsewhere.

Some asynchronous overrides return at an incomplete await, while completed awaits
may not yield. Continuations can run in later intervals, across retries or after
boundary 6. No marker establishes async completion, presentation, window-specific
first pixels, app readiness or input responsiveness. In particular, synchronous
`CompositionTarget.Rendered` handlers run before boundary 6.

## Identity, scope and repeated work

`StartupId` and `CoreId` are nonzero numeric identities unique within a loaded
instance of this XAML producer. Scope them to the trace, process lifetime and
producer-module lifetime; do not join across processes or module reloads.

At boundary 1, `CoreId=0`: the core does not yet exist. A thread-local startup
scope lets the first core constructed on that thread claim the `StartupId`
once. Its later events carry that token and its independent `CoreId`.
`EntryKind=1` (`ApplicationStart`) identifies this association. Normal first
desktop `Application.Start` initialization is the supported full six-boundary
scenario; a start that fails before core/callback construction stays partial.

`EntryKind=2` (`CoreInitialization`) and `NoApplicationStart` identify a core
without that association. Standalone islands, additional cores, reinitialization
under an already-claimed startup scope and startup on another thread produce
independent partial sequences. In particular, the UWP cross-thread path does not
inherit the startup token. Never infer an equivalent marker 1 for these paths or
attach their observations to the most recent process-wide `Application.Start`.

Only the first `OnLaunched` invocation and first qualifying layout/production/draw
boundaries are recorded per core lifetime. Warm activations and
`StartApplication` content replacement do not reset these latches. Multiple
windows/islands can share a core; these are core-scoped, not per-window phases.
The callback's return retains its original identity even if app code destroys or
replaces the core, without extending the core's lifetime.

`DrawAttemptId` counts `NWDrawTree` attempts in this core until boundary 6.
Boundaries 4/5/6 include the attempt ID; other boundaries use zero.
`FrameNumber` is the submission counter at attempt entry, not an attempt count
or proof of presentation, and is zero for boundaries 1/2/3. A retry can have the
same `FrameNumber` but a different `DrawAttemptId`. Boundaries 4 and 5 are not
repeated on retries; the intervals can therefore include work from later attempts.

`Flags` is a bitmask of producer observations, accumulated while the core exists:

| Bit | Meaning |
| --- | --- |
| `0x1` NoApplicationStart | No startup scope was associated with this core. This does not mean ETW lost marker 1. |
| `0x2` NonCanonicalOrder | The producer reached a boundary other than the next canonical ordinal. |
| `0x4` CallbackFailed | The first callback returned a failed HRESULT. |
| `0x8` MultipleDrawAttempts | A frame-side boundary occurred in an attempt other than boundary 4's attempt. |
| `0x10` CoreUnavailableAtCallbackReturn | The original core was absent or replaced at callback return; its return event uses a saved identity. |

Flags are not predictions: a callback can fail after an already-observed frame
completion. Missing captured events cannot be diagnosed solely from these flags.
