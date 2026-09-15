# Inking telemetry proposal

**Usage, reliability and performance** are the starting point for the inking stack: which
configurations apps actually use, whether an `InkCanvas` initialises successfully, and how long that
takes.

There is no prior art to mirror. UWP/WinUI 2 shipped no ink telemetry — the UWP `InkPresenter` and
`InkToolbar` live in the OS repo and are not instrumented here — so these signals are defined fresh.

## Eight signals

| Question | Event | What it records |
|---|---|---|
| Did the canvas initialise? | `InkCanvas_Initialization` | When a loaded `InkCanvas` attaches: a start, then success or failure with the stage reached and the `HRESULT`. |
| How long did it take? | `InkCanvas_InitializationLatency` | Elapsed time from the attach attempt's start to a successful attach. First initialisation only, not re-attach after a window move. |
| How is the canvas configured? | `InkCanvas_UsageSummary` | Once after a successful attach: compositor engine, input device mask, high-contrast adjustment. |
| Did the app take over drying? | `InkPresenter_CustomDryingActivation` | Emitted from `ActivateCustomDrying` itself: success, or cancelled with the `HRESULT` when the presenter was not ready (a recoverable call made too early). |
| How is the toolbar configured? | `InkToolbar_UsageSummary` | Once after auto-population settles: initial controls, orientation, active tool kind, and which target is attached. |
| What went wrong? | `Ink_ControlError` | Any control-attributable failure: category, the operation that failed, whether it was recoverable, and the `HRESULT`. |
| Was the canvas actually used? | `InkCanvas_SessionSummary` | One roll-up as the canvas is destroyed: activation count, bucketed stroke-collected and stroke-erased counts, error count, whether it ever received ink, time to first ink, and active duration. |
| Was the toolbar actually used? | `InkToolbar_SessionSummary` | One roll-up as the toolbar is destroyed: tool switch count and the number of distinct tools used. |

These describe individual control instances. Strokes are **counted**, never emitted per stroke: the
handlers increment a counter and the totals are reported once in the session summary.

Custom drying is reported as an event at its call site rather than sampled as state on the usage
summary. `ActivateCustomDrying` must be called before the first stroke, and its ordering against
`Loaded` is not guaranteed, so a snapshot taken at attach time would under-report it.

## Common dimensions

Every event carries the dimensions the WinUI 3 controls telemetry spec requires, so inking rows join
the same schema as the other controls:

| Field | Value |
|---|---|
| `ControlType` | Always `Inking`. |
| `ControlVersion` | File version of the module the control ships in, read once per process. |
| `AppSessionId` | Ephemeral per-process id. Never persisted; it only groups one run's events together. |
| `SchemaVersion` | Bumped whenever a field is added, removed or redefined. Currently `2`. |

`AppId`, `DeviceId` and `OSVersion` are deliberately **not** emitted here. Those are attached by the
telemetry pipeline, and emitting our own device identifier would be a privacy problem rather than a
fix. The device-level metrics in the spec (Daily Active Control Devices, Active Apps) depend on the
pipeline supplying them — see [Questions for review](OpenQuestions.md).

## Why the compositor engine matters most

`InkCanvas` forks on the composition engine: a system-backed process splices the ink visual under a
lifted MUC visual, while a lifted process bridges it through `ContentExternalOutputLink`. The two
paths differ in rendering and input behaviour, and the system path depends on OS support that is not
yet broadly available.

`CompositorEngine` therefore appears on the initialisation, latency and usage events. It answers the
question we cannot currently answer: what proportion of real users are on each path.

## What is not collected

No stroke data, no coordinates, no pressure or tilt, no timestamps of user input, no bitmap content,
and no text of any kind. Every field is an enum, a flag, a count or a duration. Stroke counts are
*bucketed* (0, 1, 2–4, 5–16, 17+) and record roughly *how many*, never *what*. The instance ids are
process-local counters used only to match a start to its outcome; they are not stable across runs
and carry no user or device identity.

## Proposed measures

| Measure | Definition |
|---|---|
| Observed InkCanvas instances | Distinct canvases with a usage record. Not a count of apps or devices. |
| Compositor engine share | Share of usage records on each engine. |
| Initialisation reliability | Successes / completed attempts that have both a start and an outcome. Report missing outcomes separately rather than assuming failure. |
| Initialisation latency | P50, P90 and P99 elapsed time over successful attempts. |
| Control error rate | `Ink_ControlError` count over activations, split by category and operation. |
| Custom drying adoption | Share of canvases that activate custom drying, and the success rate of those activations. |
| Toolbar configuration share | Share of toolbar usage records by initial controls, orientation and active tool. |
| Real-use rate | Share of session summaries with `ReceivedInk = true`. Separates genuine use from incidental instantiation. |
| Engagement depth | Bucketed stroke-collected and stroke-erased counts per session summary; tool switches and distinct tools per toolbar. Counts are bucketed (0, 1, 2–4, 5–16, 17+) to match the Charts convention and bound cardinality. |
| Time to first ink | Elapsed time from successful activation to the first collected stroke. `-1` where no ink was ever received. |

## Reading the results

Success means the canvas attached to a composition target and sized its presenter. It does not
confirm that ink was drawn, or that it was visible.

An `InkCanvas` that is loaded and unloaded repeatedly re-attaches; only the first attempt is timed,
so latency reflects cold initialisation. A failed attempt that later succeeds on re-attach stays a
failure for reliability purposes.

Missing fields are unknown, not zero or disabled. A failed initialisation has no usage record, so
feature breakdowns are drawn only from canvases that got far enough to report. A canvas that never
activated successfully emits no session summary either, because there is no active period to describe.

Input-to-render latency is **not** reported. Wet ink is rendered by the OS ink stack on the ink
thread, so this layer cannot observe the pointer-to-pixel path and any number produced here would
measure something else. `TimeToFirstInkMs` is an engagement signal, not a rendering latency.

[Questions for review](OpenQuestions.md) covers the decisions still needed before production
collection.
