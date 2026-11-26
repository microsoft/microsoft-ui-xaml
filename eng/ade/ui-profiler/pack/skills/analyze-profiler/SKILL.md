---
name: analyze-profiler
description: Interpret profiler-backed workstream evidence and actual WinUI expert recommendations before making the one approved application performance fix.
---

1. Read the pack's mandatory `harness-contracts.md`, then the workstream's measured observations,
   source/KB citations and expert hypotheses. A pivot may not relax those contracts.
2. Read the full multidimensional evidence and availability matrix, not only ticks: action/launch
   latency, layout/render spans, CPU/symbol/drop quality, every process-memory and GPU gauge,
   graphics/texture lifetimes/known bytes, object/tree activity and runtime resource diagnostics.
   Treat missing/unsupported evidence as unknown, not zero. Do not pair IDs/handles across recordings.
   Tick cadence is not present timing; nested spans are not additive CPU time.
3. Query the declared `winui-profiler` and `ui-profiler-evidence` read-only MCPs for scoped
   evidence/drilldowns when needed. Read the report's application/control/framework coverage,
   runtime/source parity caveats, source-read provenance and per-item validation requirements.
   Native `recommend` items require a `ui-profiler/native-trial/v1` context and the separate native
   check, never the app check. Inspect current source for the declared target. Preserve its behavior,
   accessibility and build settings. Do not remove work just to improve a number.
4. Implement only this recommendation's behavior-preserving fix in its fresh original-source working
   copy. App trials permit only approved existing app C#/XAML. Native trials permit only approved
   existing source in the declared `controls\dev` or `dxaml\xcp` layer. Never change app usage in
   a native trial. Use its exact approved scope, not an arbitrary two-file cap.
   Do not import other trials' fixes or add profiler/harness references.
5. Do not run shell commands, build, UI actions or captures. ADE's verification owns the only
   candidate capture for this recommendation. App trials reuse the original baseline. Native checks
   build a source-matched private native baseline/candidate pair under the separately approved
   two-capture budget; the old discovery runtime is not that baseline.
6. Treat a single-pair comparison as observed evidence. Call out regressions, missing attribution,
   incomplete data, and remaining uncertainty rather than asserting a proven root cause or speedup.
   Deferring work can move its cost to a later interaction; a faster tick is not a universal win.
