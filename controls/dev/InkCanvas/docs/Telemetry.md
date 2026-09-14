# Inking telemetry proposal

**Usage, reliability and performance** are the starting point for the inking stack: which
configurations apps actually use, whether an `InkCanvas` initialises successfully, and how long that
takes.

There is no prior art to mirror. UWP/WinUI 2 shipped no ink telemetry — the UWP `InkPresenter` and
`InkToolbar` live in the OS repo and are not instrumented here — so these signals are defined fresh.

## Five initial signals

| Question | Event | What it records |
|---|---|---|
| Did the canvas initialise? | `InkCanvas_Initialization` | When a loaded `InkCanvas` attaches: a start, then success or failure with the stage reached and the `HRESULT`. |
| How long did it take? | `InkCanvas_InitializationLatency` | Elapsed time from the attach attempt's start to a successful attach. First initialisation only, not re-attach after a window move. |
| How is the canvas configured? | `InkCanvas_UsageSummary` | Once after a successful attach: compositor engine, input device mask, high-contrast adjustment. |
| Did the app take over drying? | `InkPresenter_CustomDryingActivation` | Emitted from `ActivateCustomDrying` itself: success, or failure with the `HRESULT` when the presenter was not ready. |
| How is the toolbar configured? | `InkToolbar_UsageSummary` | Once after auto-population settles: initial controls, orientation, active tool kind, and which target is attached. |

These describe individual control instances, not every stroke or redraw.

Custom drying is reported as an event at its call site rather than sampled as state on the usage
summary. `ActivateCustomDrying` must be called before the first stroke, and its ordering against
`Loaded` is not guaranteed, so a snapshot taken at attach time would under-report it.

## Why the compositor engine matters most

`InkCanvas` forks on the composition engine: a system-backed process splices the ink visual under a
lifted MUC visual, while a lifted process bridges it through `ContentExternalOutputLink`. The two
paths differ in rendering and input behaviour, and the system path depends on OS support that is not
yet broadly available.

`CompositorEngine` therefore appears on the initialisation, latency and usage events. It answers the
question we cannot currently answer: what proportion of real users are on each path.

## What is not collected

No stroke data, no coordinates, no pressure or tilt, no timestamps of user input, no bitmap content,
and no text of any kind. Every field is an enum, a flag, or a duration. The instance ids are
process-local counters used only to match a start to its outcome; they are not stable across runs
and carry no user or device identity.

## Proposed measures

| Measure | Definition |
|---|---|
| Observed InkCanvas instances | Distinct canvases with a usage record. Not a count of apps or devices. |
| Compositor engine share | Share of usage records on each engine. |
| Initialisation reliability | Successes / completed attempts that have both a start and an outcome. Report missing outcomes separately rather than assuming failure. |
| Initialisation latency | P50, P90 and P99 elapsed time over successful attempts. |
| Custom drying adoption | Share of canvases that activate custom drying, and the success rate of those activations. |
| Toolbar configuration share | Share of toolbar usage records by initial controls, orientation and active tool. |

## Reading the results

Success means the canvas attached to a composition target and sized its presenter. It does not
confirm that ink was drawn, or that it was visible.

An `InkCanvas` that is loaded and unloaded repeatedly re-attaches; only the first attempt is timed,
so latency reflects cold initialisation. A failed attempt that later succeeds on re-attach stays a
failure for reliability purposes.

Missing fields are unknown, not zero or disabled. A failed initialisation has no usage record, so
feature breakdowns are drawn only from canvases that got far enough to report.

[Questions for review](OpenQuestions.md) covers the decisions still needed before production
collection.
