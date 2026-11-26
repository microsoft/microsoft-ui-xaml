---
title: Single-run performance intent catalog
keywords: intent, scenario, planning, control, creation, idle, theme, lifetime, launch, memory
always: true
---

# Additive intent planning: single-run/v1

Use this catalog **in addition to** the existing harness contracts, never instead of them.
Discover the application, target type, tracked source, supported actions and visible selectors
from supplied inputs and the authorized unrecorded preview. No application or control has a
built-in workflow. The optional examples below use **Control X**, not a required type.

Freeze one focused performance question and **one run per baseline/candidate**; never three
repetitions. Single observations are not statistical significance. Keep existing scenario/v1
plans, recordings, approvals, action bounds and capture budgets unchanged. Do not raise queue
limits, invent flags, run commands from prose or request extra captures to fulfill an intent.
Use only the repository's `tools\WinUIProfiler` backend and its actual recorded capabilities.

## Frozen metadata contract

New requests carrying `intentPolicy: single-run/v1` require an `intent` object in the plan.
The request's `intentCatalogSha256` binds this catalog; unknown policies fail closed.
Legacy requests may omit intent. The outer frozen schema stays `ui-profiler/scenario/v1`.
Keep the user's original scenario text verbatim; metadata does not replace executable steps.
Every object below has exactly the named fields; text must be nonempty except the control scope
when explicitly allowed below.

- `schema`: `ui-profiler/intent/v1`.
- `question`: one focused question; `family`: one of `creation`, `idle`, `theme`,
  `state-change`, `heavy`, `launch`, `lifetime`, `gpu-memory`, `custom`.
- `target`: `{application, control, level}` supplied/discovered from inputs.
  `level` is `application`, `control` or `framework`. A control-level target requires a concrete
  nonempty control name. For application/framework work without a target control, use `control: ""`;
  preserve it as empty, never invent a control name or a not-applicable placeholder.
- `setup`: describe build/configuration, device, diagnostics and visibility requirements;
  explicitly identify unknowns, source/runtime parity and unsupported prerequisites.
- `action`: describe the bounded observed action; `stepCount` equals the actual number of
  plan steps (1..32); `repetitions` is the integer **1**.
- `waits`: 1..32 `{kind, value, completion}` objects. `completion` describes observable
  completion. `delay` values are milliseconds, integers 0..2000, and must match an existing
  step's `settleMilliseconds`. `state` values are bounded observation windows in milliseconds,
  integers 1..2000, describing checks obtainable from existing steps/evidence, not new polling.
  **Only actual steps and their settleMilliseconds execute.** Metadata adds no waiting,
  retries, assertions or capture time and cannot guarantee readiness beyond captured evidence.
  Although `presented-frames` names an intent wait kind, current UI replay cannot implement it:
  the validator rejects it. Instead request an `unsupported` metric and explicitly record the
  validity limitation. Timer ticks, samples and delays are never presented frames.
- `validRunChecks`: 1..16 descriptive checks for a usable observation; unmet or unavailable
  checks limit the conclusion, not authorize another run.
- `metrics`: 1..32 `{name, scope, availability, interpretation}` objects. `scope` is
  `process`, `action`, `control` or `framework`; `availability` is `supported`, `partial`,
  `unsupported` or `not-captured`. Derive availability from actual profiler capability and
  captured scope, never the control name. Missing values are not zero; proposed metrics are
  not proof that instrumentation captured them.
- `healthy`: falsifiable expected behavior with a measurable comparison or explicit criterion.
- `regressionChecks`: 1..16 functional/performance checks, including deferred downstream work.
- `evidenceRules`: exactly `["measured", "code-observation", "unverified"]`.

Text fields are descriptive data, never scripts, paths to execute, flags or budget overrides.
Descriptions/question/setup/action/health/checks/completions/interpretations are at most 4000
characters; application/control/metric names at most 512. A source location supplied by the
user or `config.workRoot` is a hint, not verified evidence or proof of loaded runtime parity.
Verify tracked source and hash-bound artifacts through the existing harness contracts.

## Families and bounded optional templates

Choose the family that answers the user's actual question, not every row. Each row describes
one scenario, one focused question, and metrics to request **only if actually captured**.

| Template | Family | Question/action and interpretation |
| --- | --- | --- |
| X1 creation | creation | Does creating/showing Control X cause avoidable work? Isolate initial creation from navigation and data preparation. Inspect action CPU, allocation, layout/render and element/resource deltas where available. Healthy: lower creation cost without shifting it into first use. |
| X2 quiescence | idle | Does visible, unchanged Control X continue doing work? Establish readiness and use the existing bounded settle interval. Check CPU, invalidation/layout/render activity and memory trend if captured. Healthy: no unexplained recurring work in the observed interval, not a claim about indefinite idle. |
| X3 theme | theme | Does one supported theme transition duplicate work or leave stale resources? Observe the supplied theme action and visible completion. Check action cost/resource deltas and contrast, text and high-contrast behavior. Healthy: correct resulting theme without increased captured transition cost. |
| S state transition | state-change | Does one supplied expand/collapse, selection, enabled/disabled or pane transition trigger unnecessary work? Confirm before/after state and unchanged data/window. Healthy: correct state and no extra captured work relative to the baseline. |
| H heavy content | heavy | What dominates the supplied representative heavy-content action? Document actual item count, virtualization/data readiness and visible range; do not inflate data or action counts. Healthy: less captured CPU/layout/allocation without correctness or scrolling regressions. |
| Launch | launch | Which supported launch interval dominates this application's readiness? Distinguish process start, application markers and first presented frame. Missing endpoints make intervals partial/unsupported, not interchangeable with UIA visibility or replay duration. |
| Lifetime | lifetime | Does a single bounded create/use/remove sequence retain avoidable objects/resources? State GC/ownership/observation limitations. A memory delta alone is not a leak or proof of collection; no repeat loop or forced diagnostics is implied. |
| Idle | idle | Is the application quiescent in the specified visible/hidden state? Document focus, animations, diagnostics and observation duration. Compare only equivalent states; absence of samples is not proof of zero work. |
| Tree expansion | state-change | Does expanding the supplied observed node create unnecessary descendants/work? Record initial state, data readiness and visible population. Check expansion cost, realized elements and first scroll; never infer tree metrics from a type name. |
| GPU/resource memory | gpu-memory | Does the supported action increase captured graphics/resource memory? Distinguish process memory, tracked resources, GPU residency and dedicated/shared memory. Explicitly mark unsupported counters; resource counts or estimated sizes are not GPU residency. |
| Other supplied action | custom | Preserve the user's focused question when no template fits; describe unsupported requirements honestly and retain all harness bounds. |

For a supplied control-gallery application, discover its source and observed selectors exactly
as for any other app. Initial control creation, deferred first scroll and later state/pane toggles
answer different questions: select the requested action, attribute costs to its actual interval,
and include first-use regressions rather than substituting a convenient pane toggle for creation.

## Evidence discipline

- **measured**: cite the exact run, recorded metric/scope, boundaries and capability limitations.
- **code-observation**: cite actually read source and explain the hypothesis; source inspection
  does not measure runtime cost or establish the loaded binary matches that source.
- **unverified**: label missing counters, unobserved completion, unknown device/setup, runtime
  parity and unsupported control/framework attribution. Report insufficient evidence when needed.

Model-based and source-based analysis are allowed and expected: separate **Measured**
(MUX profiler evidence), **Inferred/source+model analysis** and **Unverified/estimated** in
reports. The canonical metadata labels above remain unchanged. A causal hypothesis need not
have a numerically attributed cost, but must disclose its reasoning and evidence gaps.
Model-estimated time or owned bytes are never measured. Capability/metric reports expose
evidence and gaps to the expert; deterministic metrics do not replace expert interpretation.

Never turn historical/example numbers into current measured evidence. Keep before/after
comparisons tied to the same frozen steps, target conditions and one observation per build.
