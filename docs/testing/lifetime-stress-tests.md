# Lifetime stress tests

## Why these exist

Object-lifetime crashes — use-after-free, premature native peer destruction, and ref-counting mistakes in the
3-layer peer model — have been a recurring, hard-to-pin-down source of WinUI crashes. They are rarely fixed in a
single place: each control can reintroduce the problem in its own realize/recycle or enter/leave path.

XAML used to ship dedicated **lifetime tests** (back to the Win8 era) that repeatedly created, parented, unparented,
and destroyed elements while forcing garbage collection, watching for *eventual* crashes. Those were lost when the
old test-team tests were dropped. This harness reintroduces that coverage.

## What we do differently

The old lifetime tests mostly relied on running for a long time (often hours) and hoping a crash eventually
reproduced. This harness keeps that soak capability but front-loads the signal — whenever it runs, every iteration
aggressively settles and collects so a dangling peer faults promptly rather than eventually:

- **Aggressive per-iteration settling.** After every create/teardown cycle we drain the UI thread
  (`IdleSynchronizer.Wait`) and force `GC.Collect` + `WaitForPendingFinalizers` + `GC.Collect`. A dangling native
  peer is then far more likely to fault *immediately*, on the iteration that created it, instead of thousands of
  iterations later.
- **Targeted torture paths.** In addition to a broad create/load/unload sweep across many controls, we exercise the
  paths that have historically produced lifetime crashes: ItemsRepeater realization/recycling (currently
  quarantined — see below), element reparenting (enter/leave), window open/close, ListView container recycling,
  Popup open/close, NavigationView menu churn, and TabView add/remove.
- **Isolation.** The tests are tagged into their own TAEF test suite (`LifetimeStressTestSuite`). The Helix
  work-item generator emits a dedicated work item for that suite, so a lifetime crash does not cascade into
  unrelated tests and the soak can be scheduled independently. That work item is only emitted on the dedicated
  lifetime-stress pipeline (see below), so the suite does not run on the shared PR gate or Nightly.

Source: [`controls/test/MUXControlsTestApp/LifetimeStressTests.cs`](../../controls/test/MUXControlsTestApp/LifetimeStressTests.cs)

## How it runs in CI

Each test carries `[TestProperty("TestSuite", "LifetimeStressTestSuite")]` and the class is
`[TestProperty("Classification", "Integration")]`, so the Helix work-item generation
(`Helix/common/pipeline/GenerateHelixWorkItems.ps1`) can produce an **isolated** work item for the suite. That
work item is **only** emitted when `WINUI_LIFETIME_STRESS_ENABLED=1`, which is set exclusively by the dedicated
lifetime-stress pipeline ([`build/WinUI-LifetimeStress.yml`](../../build/WinUI-LifetimeStress.yml)). On every other
test pass — the shared per-PR gate, Nightly, etc. — the generator skips the suite, so it never runs there.
Isolation means a lifetime crash cannot cascade into unrelated tests.

**Gating leak detection on the dedicated pipeline.** When the suite runs, a residual reference after forced
collection is reported through `VerifyCollected(failOnLeak: true)` as a `Verify.Fail`, which records a *Failed*
test result and fails the Run Tests stage's *Publish Test Results* step. Because the suite runs only on the
dedicated pipeline, that gating affects that pipeline alone and never the shared PR gate. Other catchable
failures are still handled so a stray exception cannot be mistaken for a leak:

- **UI-thread exceptions** are caught *inside* the UI-thread callback (`SafeUI`), before `RunOnUIThread.Execute` can
  turn an escaping exception into a `Verify.Fail`, and are logged as `Log.Warning`.
- **Test-thread exceptions** are caught in the outer per-iteration wrapper (`RunIterationReporting`) and logged as
  `Log.Warning`; **leaks** are reported as gating `Verify.Fail` by `VerifyCollected(failOnLeak: true)`.

The **one** thing no managed catch can intercept is a genuine *native* crash / fail-fast (for example a stowed
exception in `combase.dll`) that terminates the TAEF test host outright — and that is exactly the lifetime signal we
want. A known deterministic crasher is quarantined per-scenario with `[TestProperty("Ignore", "True")]` (see the
note below) so it does not gate while its underlying product bug is pending; if a *new* scenario is found to crash
the host deterministically, quarantine it the same way.

**Native crash/warning totals (PostTestRun).** Each native host crash and each non-gating native scenario warning
(a thrown-exception/COMException report) is recorded per work item in `LifetimeNativeCrashReport.json` by
[`RunHelixWorkItem.ps1`](../../Helix/common/test/RunHelixWorkItem.ps1) (`Report-LifetimeNativeCrash`). After the
test run, the **PostTestRun** step in
[`WinUI-RunTestPassOnPipeline-Job.yml`](../../build/AzurePipelinesTemplates/WinUI-RunTestPassOnPipeline-Job.yml)
runs [`Report-LifetimeNativeCrashTotals.ps1`](../../Helix/common/pipeline/Report-LifetimeNativeCrashTotals.ps1),
which totals those records across every lifetime work item on the shard and surfaces a single count — printed to
the log, emitted as a non-gating warning, published as the `LifetimeNativeCrashTotal` pipeline variable, and
written to `LifetimeNativeCrashSummary.json`. The suite runs as one isolated work item in the **checked (chk)
build flavor** (lifetime/TrackerHandle leak detection needs the reference-tracker instrumentation that free builds
lack), so its `LifetimeNativeCrashSummary.json` in that job carries the leg's full total. Test-pass jobs that ran
no lifetime work item (e.g. the fre flavor) total zero and **do not write a summary file**, so the only
`LifetimeNativeCrashSummary.json` in the artifacts is the populated one. Like everything else here, the step is
non-gating.

Run modes (all optional; the default needs no configuration):

- **Dedicated pipeline only** — the suite runs only when `WINUI_LIFETIME_STRESS_ENABLED=1`, which is set by
  [`build/WinUI-LifetimeStress.yml`](../../build/WinUI-LifetimeStress.yml). It does **not** run on the shared PR
  gate or Nightly. With no soak/iteration variable set, each scenario runs a small **report pass**
  (`DefaultReportIterations` cycles); leaks are gating (`failOnLeak: true`).
- **Scheduled soak** — the same dedicated pipeline sets `WINUI_LIFETIME_STRESS_MINUTES > 0`, so each scenario loops
  on a wall-clock budget. That pipeline must be registered in Azure DevOps and its schedule/soak duration tuned there.
- **Explicit local/manual run** — set `WINUI_LIFETIME_STRESS_ITERATIONS > 0` to run a heavier fixed cycle count.

Leak detection is gating everywhere the suite runs (`failOnLeak: true`); a residual reference records a *Failed*
test result.

> **Note:** the `StressItemsRepeaterRealizationAndRecycling` scenario is currently **quarantined**
> (`[TestProperty("Ignore", "True")]`) because it reproduces a deterministic native crash. Re-enable it once that
> underlying ItemsRepeater lifetime bug is understood/fixed.

## Configuration

All knobs are read from the environment, so they work locally, on pipeline agents, and when injected into a Helix
work item. The suite only runs where `WINUI_LIFETIME_STRESS_ENABLED=1` (the dedicated lifetime-stress pipeline);
with no soak/iteration variable set it runs the small report pass, and leak detection is gating there.

| Variable | Meaning | Default |
| --- | --- | --- |
| `WINUI_LIFETIME_STRESS_ENABLED` | If `1`, the Helix work-item generator emits the isolated `LifetimeStressTestSuite` work item. Set only by the dedicated lifetime-stress pipeline, so the suite runs there and not on the PR gate or Nightly. | unset (suite not scheduled) |
| `WINUI_LIFETIME_STRESS_MINUTES` | If > 0, each scenario soaks for this many minutes (wall-clock). The scheduled soak pipeline sets this. | `0` (disabled) |
| `WINUI_LIFETIME_STRESS_ITERATIONS` | If > 0 **and** soak mode is off, run this many create/destroy cycles per scenario — a heavier local/manual run. | `0` (use the default report pass) |

### Run a soak locally

```
:: From a developer command prompt after building the test app
set WINUI_LIFETIME_STRESS_MINUTES=120
te.exe MUXControlsTestApp.dll /select:"@TestSuite='LifetimeStressTestSuite'"
```

## Adding coverage

When you fix a lifetime crash, add the offending create/teardown sequence as a new scenario here (the ItemsRepeater
realization/recycling scenario is the template to copy). That converts a one-off fix into permanent
regression protection, which is exactly where these bugs keep coming back.
