# TableView telemetry

The proposal covers **observed usage, initial readiness and update performance**.
It is a starting point for deciding whether TableView is being adopted and
providing a reliable, responsive experience.

## Signals

| Event | When and why |
|---|---|
| `TableView_Initialization` | Records the first hosted initialization attempt and its outcome. A successful outcome requires usable layout, not just construction or `Loaded`. |
| `TableView_UsageSummary` | Records one configuration snapshot after an observed usable layout. A later recovery or late collection can establish usage without changing the initial outcome. |
| `TableView_Performance` | Records successful initial usable-layout latency and sampled source-replacement/control-initiated-sort-to-layout durations. |
| `TableView_Error` | Records an observed operation failure or recoverable header-refresh failure, once per operation category per instance. |

An empty table is a valid result. Usage distinguishes empty content, visible
data rows and group-header-only content, and includes the configured column-count
range and whether the source is grouped. When `ConfigurationAvailable` is false,
ignore the column bucket and grouped flag; they may contain default values.
Populated usage requires a visible portion of a cell in the viewport. A
transparent table is rechecked when it becomes visible, without forcing layout;
initial latency can include time the app keeps it transparent.

## Measures

| Question | Proposed measure |
|---|---|
| How widely is TableView used? | Distinct active devices, app-device pairs and apps with a successful usage observation, using approved collection identities. Instance IDs are not device or app identities. |
| Does initial readiness succeed? | Matched successes / (successes + failures). Exclude cancellations; report unmatched attempts separately. A successful retry does not rewrite the first result. |
| How quickly is it usable? | P50/P95 successful initial latency, separating template initialization from attachment of an already-templated control. |
| How responsive are updates? | P50/P95 successful source-replacement and accepted control-sort to usable-layout samples, reported separately. |

Initial outcomes and usage are not locally sampled. Recurring operation timings
are sampled at 1 in 16, with the sampling denominator recorded. Superseded,
cancelled and failed operations are excluded from successful latency views.

These timings end at usable XAML layout, not physical display. They do not cover
every incremental data update or end-to-end scroll/input latency. Error records
can include failures from app-supplied templates or delegates; attribution is
unknown, so they are not a control-only error rate or an exact error count.

## Collection decisions

The implementation uses the existing Tabular provider and collects no cell
values, labels, headers, binding paths or other app content. Production routing,
privacy approval, common app/device identities and reporting remain owner
decisions. A once-per-instance snapshot also needs an agreed reporting rule for
controls that remain active across reporting days.

Session summaries, engagement counters and additional interaction timings are
outside this initial change.
