---
name: analyze-profiler
description: Interpret profiler-backed workstream evidence and actual WinUI expert recommendations before making the one approved application performance fix.
---

1. Read the workstream's measured observations, source/KB citations and expert hypotheses.
2. Read the full multidimensional evidence and availability matrix, not only ticks: action/launch
   latency, layout/render spans, CPU/symbol/drop quality, every process-memory and GPU gauge,
   graphics/texture lifetimes/known bytes, object/tree activity and runtime resource diagnostics.
   Treat missing/unsupported evidence as unknown, not zero. Do not pair IDs/handles across recordings.
   Tick cadence is not present timing; nested spans are not additive CPU time.
3. Check the cited current application code. Preserve all samples, tree behavior, search semantics,
   accessibility and build settings. Do not remove work just to improve a number.
4. Implement at most one small behavior-preserving fix. Do not add profiler or harness references.
5. Do not run shell commands, build, UI actions or captures. ADE's verification owns the only
   candidate capture. It reuses evidence only for exactly matching source and inputs.
6. Treat a single-pair comparison as observed evidence. Call out regressions, missing attribution,
   incomplete data, and remaining uncertainty rather than asserting a proven root cause or speedup.
   Deferring work can move its cost to a later interaction; a faster tick is not a universal win.
