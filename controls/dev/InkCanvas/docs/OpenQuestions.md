# Inking telemetry — open questions

Decisions needed before this is relied on for reporting.

## Scope

1. Are four events the right starting set, or should the first drop be narrower — for example
   reliability and compositor engine only, with usage added once those are trusted?
2. Should stroke-level activity be measured at all (for example "did this canvas ever receive
   ink?"), or is instance configuration enough? A "canvas created but never inked" signal would
   distinguish real usage from incidental instantiation, at the cost of a per-stroke code path.
3. `InkToolbar` currently reports configuration only. Should tool switches be counted, and if so as
   a total or as a distinct-tools-used set?

## Boundaries

4. Initialisation is timed from the start of the attach sequence to a successful presenter size.
   Should presenter creation on the ink thread be included? It completes asynchronously, so the
   current boundary deliberately excludes it.
5. Re-attach after a window move is not timed and not counted as a new attempt. Is that the right
   call, or should re-attach reliability be tracked separately?
6. `HighContrastAdjustment` and `InputDeviceTypes` are sampled once, at first successful attach.
   Apps may change both later. Is a first-value snapshot sufficient, or is a last-value or
   changed-at-least-once signal more useful?
7. `InkPresenter_CustomDryingActivation` carries no instance id, because activation can precede the
   canvas attach that assigns one. Should the presenter own its own id so activations can be
   correlated to a specific canvas?

## Population and privacy

7. Instance ids are process-local counters. Confirm that is sufficient to match start/outcome pairs
   without any cross-session correlation.
8. `InputDeviceTypes` is recorded as the raw `CoreInputDeviceTypes` flags mask. Confirm a device
   capability mask is acceptable, or whether it should be reduced to booleans.
9. Confirm the privacy tags: performance events use `PDT_ProductAndServicePerformance`, usage events
   use `PDT_ProductAndServiceUsage`.

## Alignment with the WinUI 3 controls telemetry spec

13. The spec's north-star metrics — Daily Active Control Devices, Daily Active App-Device and Active
    Apps — need `DeviceId` and `AppId`. Neither is emitted here, on the assumption the telemetry
    pipeline attaches them. Confirm that, because if it does not, none of the P0 usage metrics can be
    computed from these events.
14. The spec proposes one common event set (`ControlActivated`, `ControlError`, `ControlPerformance`,
    `ControlSessionSummary`, `ControlInteraction`) shared by Inking, Charting and TableView, with a
    `ControlType` dimension. These events keep ink-specific names and add `ControlType` instead.
    Renaming affects all three controls, so it needs a cross-team decision rather than a unilateral
    one here. Existing WinUI events are per-control named (`ItemsViewPerf`, `ItemsRepeaterInfo`).
15. The spec lists `InputToRender` as the P0 performance granularity for inking. It is not
    implemented: wet ink is rendered by the OS ink stack on the ink thread, so this layer cannot
    observe the pointer-to-pixel path. Should this be measured in the OS ink stack instead, or
    dropped for inking?
16. `ControlVersion` is read from the file version of the hosting module. Confirm that is the version
    reporting should key on, rather than a package or Windows App SDK version.
17. `AppSessionId` is a process-local value derived from the process id and a performance counter. It
    is ephemeral and never persisted, but confirm the derivation is acceptable, or whether an
    existing session identifier should be reused.
18. The `Result` enum mirrors the spec's Outcome vocabulary (`Started`, `Success`, `Failure`,
    `Cancelled`). A `Degraded` outcome is intentionally *not* emitted: the lifted-vs-system
    compositor split is already carried explicitly on every event by the `CompositorEngine`
    dimension, so analysts can compute a degraded-engine rate without it. If the team wants a
    single Outcome-level "degraded" signal, wire it to the lifted-compositor init path — but that
    encodes a judgement that lifted is inferior, which needs sign-off.

## Operational

10. `KEYWORD_INKCANVAS` (`0x10000`) is newly allocated; `KEYWORD_INKTOOLBAR` (`0x4000`) already
    existed and was unused. Confirm both are correct for this stack.
11. Sampling and volume: an `InkCanvas` in a list or a repeatedly loaded page produces one
    initialisation record per attach. Is any client-side throttling required?
12. What is the success criterion for this instrumentation — what decision will the compositor
    engine share or the reliability rate actually drive?
