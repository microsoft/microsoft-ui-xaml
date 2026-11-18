---
title: WinUI control measurement constraints
always: true
appliesTo: [perf-fix]
---
# Real WinUI control measurement POC

Only ScenarioWorkload.cs is editable. Do not edit any other file, the original
staged sample, the mux repository, external tools, or the measurements.
Do not add files or dependencies or weaken the protected measurement contract.

The app constructs 40 Buttons in a StackPanel. Its protected caller performs a
final UpdateLayout, verifies button count/order/content, writes process and
loaded-framework identity, and closes its own window.

The target command is python -B -m helpers.verify --root . The shared helpers
consume the protected scenario.json; the app, its build script, marker payloads,
correctness expectations, and layout budget remain scenario-specific.
The verifier builds the app once, runs ten
independent local processes under uniquely owned ETW sessions, imports every
trace through the real WinUIProfiler CLI, and queries its SQLite event store.
It returns 0 only when all runs preserve correctness and have at most four
complete XAML layout passes inside the scenario's UI-thread markers.
It returns 1 for a measured budget failure and 2 for an environment/capture error.
An environment error is not evidence that the recommendation was ineffective.

The reported scenario duration is informational. This POC validates reduced
framework work from fixing a deliberately constructed app inefficiency. It does
not demonstrate a newly discovered framework bug, a statistically proven
latency improvement, a complete MAF agent graph, WPR integration, or VM support.

Do not control unrelated apps or captures. Do not upload ETLs or SQLite files:
the raw provider-wide ETL can include activity from other local applications.
ADE is not a security sandbox. All commands use the engineer's account.
