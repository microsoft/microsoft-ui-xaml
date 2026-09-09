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
reproduced. This harness keeps that soak capability but front-loads the signal so most regressions are caught on a
normal test pass:

- **Aggressive per-iteration settling.** After every create/teardown cycle we drain the UI thread
  (`IdleSynchronizer.Wait`) and force `GC.Collect` + `WaitForPendingFinalizers` + `GC.Collect`. A dangling native
  peer is then far more likely to fault *immediately*, on the iteration that created it, instead of thousands of
  iterations later.
- **Targeted torture paths.** In addition to a broad create/load/unload sweep across many controls, we specifically
  exercise the paths that have historically produced lifetime crashes: ItemsRepeater realization/recycling,
  element reparenting (enter/leave), and window open/close.
- **Isolation.** The tests are tagged into their own TAEF test suite (`LifetimeStressTestSuite`). The Helix
  work-item generator emits a dedicated work item for that suite, so a lifetime crash does not cascade into
  unrelated tests and the soak can be scheduled independently.

Source: [`controls/test/MUXControlsTestApp/LifetimeStressTests.cs`](../../controls/test/MUXControlsTestApp/LifetimeStressTests.cs)

## How it runs in CI

Because each test carries `[TestProperty("TestSuite", "LifetimeStressTestSuite")]`, the existing Helix work-item
generation (`Helix/common/pipeline/GenerateHelixWorkItems.ps1`) automatically produces an isolated work item for
the suite on **every** DevTestSuite test pass (PR and Nightly). With the default (gate) iteration counts each
scenario runs quickly, so this is a real regression gate — no pipeline changes are required for that.

For the long **soak**, the scheduled pipeline
[`build/WinUI-LifetimeStress.yml`](../../build/WinUI-LifetimeStress.yml) builds WinUI and runs the test pass with
the soak environment variable set, so the lifetime scenarios loop on a wall-clock budget instead of a fixed
iteration count. That pipeline must be registered in Azure DevOps; tune its schedule and soak duration there.

## Configuration

Both knobs are read from the environment, so they work locally, on pipeline agents, and when injected into a Helix
work item. Defaults keep the fast gate behavior, so setting nothing is safe.

| Variable | Meaning | Default |
| --- | --- | --- |
| `WINUI_LIFETIME_STRESS_ITERATIONS` | Fixed number of create/destroy cycles per scenario. | `50` |
| `WINUI_LIFETIME_STRESS_MINUTES` | If > 0, each scenario loops until this many minutes elapse instead of using the iteration count. Use for the soak. | `0` (disabled) |

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
