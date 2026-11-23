---
title: Team conventions
always: true
---

# WinUI Performance Profiling conventions

- The application is an independent source input. Do not reference this harness, add profiler hooks,
  or change its SDK, dependencies, build settings, public behavior, or accessibility to improve a metric.
- Use only the MUX `tools\WinUIProfiler` CLI and its complete native payload. SQLite recordings are
  immutable evidence, not orchestration state or team knowledge.
- The WinUI expert's measured observations and source-grounded recommendations arrive in workstream
  notes. Apply one small, behavior-preserving source fix in ADE's working copy.
- The coding session has no shell or URL permission. Do not run builds, UI actions, or captures yourself.
  ADE's verification check owns the candidate build and the only candidate capture.
- Baseline and candidate each have one capture. Cached verification must match the exact source,
  scenario, profiler closure, framework, and evidence hashes. Never bypass or reset the capture budget.
- UI tick duration is elapsed time, not CPU time or proof of a dropped frame. CPU stacks may be absent.
  Resource counters do not establish XAML-element-to-texture ownership.
- Compare every retained evidence family over matching action/QPC windows, including memory, GPU,
  graphics/texture allocation aggregates, layout, objects/trees/resources and launch. Keep separate
  CPU stack-drop and symbol quality; do not confuse it with ETW event loss. Keep exact integer bytes.
- Cross-recording handles, texture IDs, node IDs and thread IDs are not stable identities. Use the
  profiler's canonical texture/history/diff implementations and comparable aggregates. Snapshot
  diffs require the same process instance. Unknown sizes/links are not zero and do not disprove leaks.
- Keep original recordings and approved ADE inputs immutable. Derived manifests bind the native
  stream, queries, snapshots, analyzer implementation and reports. Capture budgets are unchanged;
  an end snapshot uses the existing connection after the timed actions, not a new recording.
- Missing, lost, dropped, trimmed, incomplete, or different-scenario evidence is not a valid improvement.
  Report regressions and null metrics honestly. One pair establishes observed differences, not significance.
