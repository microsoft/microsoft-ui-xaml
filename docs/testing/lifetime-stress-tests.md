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
  unrelated tests and the soak can be scheduled independently.

Source: [`controls/test/MUXControlsTestApp/LifetimeStressTests.cs`](../../controls/test/MUXControlsTestApp/LifetimeStressTests.cs)

## How it runs in CI

Each test carries `[TestProperty("TestSuite", "LifetimeStressTestSuite")]` and the class is
`[TestProperty("Classification", "Integration")]`, so the existing Helix work-item generation
(`Helix/common/pipeline/GenerateHelixWorkItems.ps1`) produces an **isolated** work item for the suite on every
DevTestSuite test pass. Isolation means a lifetime crash cannot cascade into unrelated tests.

**Reports in the PR run, but never gates it.** The suite runs its create/teardown/GC workload on **every** test
pass — including the per-PR gate and Nightly — so a lifetime **report** is produced right there in the pipeline run.
It is engineered so it can never fail the pipeline: every catchable failure is downgraded to a non-gating warning,
so the suite never records a *Failed* test result (and so never trips the Run Tests stage's *Publish Test Results*
step, which fails the task on any failed test). Two layers do this:

- **UI-thread exceptions** are caught *inside* the UI-thread callback (`SafeUI`), before `RunOnUIThread.Execute` can
  turn an escaping exception into a `Verify.Fail` — which would record a Failed verdict that a later catch could not
  undo.
- **Test-thread exceptions and leaks** are caught in the outer per-iteration wrapper (`RunIterationReporting`) and by
  `VerifyCollected(failOnLeak: false)` respectively, and logged as `Log.Warning`.

The **one** thing no managed catch can intercept is a genuine *native* crash / fail-fast (for example a stowed
exception in `combase.dll`) that terminates the TAEF test host outright — and that is exactly the lifetime signal we
want. A known deterministic crasher is quarantined per-scenario with `[TestProperty("Ignore", "True")]` (see the
note below) so it does not gate while its underlying product bug is pending; if a *new* scenario is found to crash
the host deterministically, quarantine it the same way.

Run modes (all optional; the default needs no configuration):

- **PR gate + Nightly (default)** — neither environment variable set: each scenario runs a small non-gating
  **report pass** (`DefaultReportIterations` cycles). Fast, and it produces the report in the PR run.
- **Scheduled soak** — [`build/WinUI-LifetimeStress.yml`](../../build/WinUI-LifetimeStress.yml) sets
  `WINUI_LIFETIME_STRESS_MINUTES > 0`, so each scenario loops on a wall-clock budget. That pipeline must be
  registered in Azure DevOps and its schedule/soak duration tuned there.
- **Explicit local/manual run** — set `WINUI_LIFETIME_STRESS_ITERATIONS > 0` to run a heavier fixed cycle count.

To make leak detection fail locally while iterating, flip a scenario's `failOnLeak` argument to `true`.

> **Note:** the `StressItemsRepeaterRealizationAndRecycling` scenario is currently **quarantined**
> (`[TestProperty("Ignore", "True")]`) because it reproduces a deterministic native crash. Re-enable it once that
> underlying ItemsRepeater lifetime bug is understood/fixed.

## Configuration

All knobs are read from the environment, so they work locally, on pipeline agents, and when injected into a Helix
work item. With nothing set — the default, including the PR gate and Nightly — each scenario runs the small
non-gating report pass, so the suite is safe everywhere by default.

| Variable | Meaning | Default |
| --- | --- | --- |
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
