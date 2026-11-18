---
title: Evidence-driven WinUI investigation
always: true
appliesTo: [perf-investigate]
---
# Profiler investigation constraints

Start with profiler evidence, not an assumed code defect. The supplied scenario
is a controlled POC, not a blind benchmark of diagnosis quality. A hypothesis
is not a confirmed root cause until a controlled change and remeasurement.

Use `python -B profiler_query.py overview --iteration 1` before reading app code.
It calls the existing WinUIProfiler CLI: summary, frames, and interesting.
Choose a further query based on those results:

- `events --name <event-name-fragment>` dumps at most 300 matching events.
- `scenario [--name <event-name-fragment>]` groups events by provider/name/opcode
  and reads at most 300 EventDetails rows strictly inside the scenario/thread.
- `tick --id <tick-id> [--name <filter>]` reads one tick's events.
- `region --id <region-id>` reads derived region rows.
- `objects [--name <type>]` lists elements; `timeline --id <object-id>` follows one.

Each command also accepts `--iteration <1-10>`. Run these through
`python -B profiler_query.py`; do not edit the tool or fabricate receipts.
Use the full receipt file when stdoutTruncated is true; do not JSON-parse a
truncated stdout preview. Keep any scratch output under ade-output with unique
names, never in the shared TEMP directory.
Use the returned process, UI thread and scenario Seq interval for attribution.
CLI overviews include startup and teardown; interesting regions may not cover
the scenario, and a dispatcher callback can run outside an instrumented tick.
Do not substitute startup cost for scenario cost. Use the `scenario` query when
CLI tick/region attribution does not cover the scenario. It reads the profiler's
SQLite EventDetails view, without interpreting a particular event as the cause.

Ticks are not presented frames. "Interesting" is not synonymous with "slow."
Missing telemetry is not measured zero. Event patterns alone do not prove a
specific call stack. Do not invent stack samples, symbols or GPU measurements.
Recommend extra instrumentation if the supplied data cannot distinguish causes.

Inspect only the supplied app source snapshot for code correlation after
recording the profiler observations. Its hashes must match the measured source.
Do not read another run's fix, reports or candidate source, or use the known
POC outcome as evidence. Measurements and raw traces remain local.

Write only ade-output deliverables; do not change app source or outside files.
ADE is not an OS sandbox. Query receipts detect accidental artifact changes;
they are not a signature or proof that the diagnosis is correct.
