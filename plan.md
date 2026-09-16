# Project facts

Facts established while working on WPF-hosted core-test leak detection. Items that
describe this topic branch are labeled accordingly.

- **WPF host resets change the UI thread between tests.** For tests that call
  `ShutdownXaml` and then `InitializeXaml`, host replacement creates a new
  `WPFHost`, which starts a new STA thread with its own dispatcher. Subsequent
  `RunOnUIThread` calls use that new thread. This behavior predates this branch.
  It concerns the host's UI thread, not the TAEF test-method thread. Tests that
  keep their host do not switch UI threads merely because another test starts.

- **Normal UAP test setup reuses the existing helper, host, and UI thread.**
  `DxamlCoreTestHooks::ShutdownXaml` shuts XAML down using
  `DeinitializationType::ToIdle`; `InitializeXaml` restarts it using
  `InitializationType::FromIdle`. The old helper remains valid because its host
  and UI thread have not changed.

- **Host initialization and XAML initialization are different operations.**
  `TestServices::InitializeHost` establishes the hosting infrastructure and its
  helpers. A normal UAP `TestSetup` calls `WindowHelper::InitializeXaml`, not
  `TestServices::InitializeHost`. The `WindowHelper` property getter returns the
  existing helper; it does not construct a new one.

- **Leak verification must happen before XAML initialization resets tracking.**
  The initialization test hook marks outstanding native allocations as
  ignorable before initializing XAML. Running it before checking the previous
  test can hide that test's leaks.

- **This branch delays WPF host replacement until after cleanup verification.**
  Previously, WPF recreated the host at the end of `ShutdownXaml`. The current
  sequence is shutdown to idle, `VerifyTestCleanup` against the retiring core,
  then host replacement at the next `InitializeXaml`. Host replacement rejects
  a pending leak check.

- **Stable `WindowHelper` identity is an infrastructure choice, not a WPF
  requirement.** This branch preserves the WPF helper and uses `UnbindFromHost`
  and `RebindToHost` to change its host-bound state. Other modes retain the
  existing behavior of constructing a new helper when `InitializeHost` runs.
  Replacing the WPF helper inside its own `InitializeXaml` call would instead
  require forwarding the operation to the replacement. `KeyboardHelper` is
  still replaced when the host changes and must be reacquired.

- **Helper event handles depend on the core and UI thread.** `IdleSynchronizer`
  and `KeyboardHelper` open named core events using the UI thread ID. Old
  handles cannot be carried over to a replacement host. A host created without
  initializing XAML does not have those core events yet, so this branch opens
  them lazily when needed rather than during helper construction.

- **WPF leak detection is opt-in and checks native allocations.** Tests opt in
  with `TEST_METHOD_PROPERTY(L"Data:WpfLeakDetection", L"{true}")`. The check runs
  after successful shutdown and honors the existing ignore-leaks behavior. It
  is not a check for all managed objects or retention of the entire WPF host.

- **Cleanup ordering matters for metadata and callbacks.** The current WPF
  helper closes custom metadata registration and disconnects core callbacks
  before core shutdown. It retains callback delegate references through leak
  verification, then releases them on the retiring UI thread during unbind.

- **Test-pool filters are static objects.** Constructing a new `WindowHelper`
  does not create fresh filter objects. The duplicate adapter accumulation in
  `DPITestPoolFilter` predates this work; fixing it is not required for helper
  reuse or leak detection and is outside this patch.

- **A successful test body or zero TAEF exit code does not prove cleanup passed.**
  Test-result handling must also inspect logged errors and complete summaries,
  including failed, blocked, and not-run counts. The VM runner includes these
  checks so cleanup failures cannot appear as successful validation.
