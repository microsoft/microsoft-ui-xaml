---
name: perf-investigate
description: Investigate measured WinUI performance using profiler queries before inspecting code.
done: deliverable
deliverable: investigation.md
sections: Problem, Profiler observations, Hypotheses, Source correlation, Recommendation, Validation, Uncertainty
---
# Profiler-driven investigation

During proposal planning, plan the queries without reading app source or
presupposing a cause. The approved execution performs the investigation below.

1. Read investigation.json and the selected knowledge. Do not inspect app source yet.
2. Run python -B -m helpers.profiler_query overview --root . and record the actual observations and
   receipt path in ade-output/investigation.md before opening app source.
3. Choose and run at least one drilldown in response to the observed event names,
   ticks, regions or objects. Check other iterations as needed. Cite receipt
   paths, process/thread and scenario interval; distinguish scenario from launch.
4. Now inspect the matching source snapshot. Compare competing explanations.
   Correlate observed work with source paths and lines; do not claim unobserved stacks.
5. Prioritize a recommendation, expected effect and the evidence against alternatives.
   If inconclusive, request concrete additional measurements instead of guessing a fix.
6. Specify the unchanged correctness checks, scenario markers and metrics for
   validating the hypothesis with a later, separately approved fix run.
7. Complete the required report sections, with explicit uncertainty and a
   suspected-versus-confirmed status. Also write ade-output/decision.json:
   {"status":"RecommendationReady","reason":"evidence-backed rationale"}
   or {"status":"NeedsMeasurement","reason":"why inconclusive",
   "additionalMeasurements":["specific measurement to collect"]}.
   RecommendationReady means ready to test, not a confirmed root cause.
   NotDemonstrated or Rejected with a reason are valid when the measured problem
   is absent or the hypothesis is contradicted. Do not invent a fix to continue.
   If investigation.json contains flow, also follow the structured flow contract below.
   Run python -B -m helpers.profiler_query verify --root .
8. Leave only ade-output deliverables. Do not change any source or apply a fix.

## Integrated flow contract

Read flow.phase and flow.history. Each cycle must address the latest refined
question and prior findings rather than restarting the original investigation.
Only same-scenario repeat measurements are currently executable.

For diagnosis, RecommendationReady must additionally contain:

```json
{
  "recommendation": {
    "change": "Specific source change and causal rationale",
    "expectedImpact": "Expected work reduction, without invented latency claims",
    "editableFiles": ["ScenarioWorkload.cs"],
    "validationScenario": "Explain why the exact frozen scenario tests this cause",
    "markers": "The unchanged configured provider and start/stop markers",
    "validation": {"metric": "layoutPassCount", "minimumReduction": 1}
  }
}
```

Choose a meaningful positive integer reduction supported by evidence, not an
arbitrary threshold to make the run pass. If this metric or editable scope
cannot test the hypothesis, request unsupported measurements instead of a fix.
NeedsMeasurement requires additionalMeasurements objects of this shape:
`{"kind":"repeat","question":"A narrower question requiring another ten runs"}`.
Use kind unsupported for another provider, metric, scenario, app, or framework
change; explain exactly what is missing. Do not silently substitute a repeat.

For phase validation, inspect the candidate profiler evidence and the sealed
flow comparison/diagnosis inputs. Use Accepted only when the independent gate
passed AND the controlled change supports the causal explanation. Rejected
means the proposed causal explanation is not supported. NeedsMeasurement is
also valid. Do not emit RecommendationReady or introduce a second code change
in the validation phase. State the scope of any acceptance: this app, action
plan, metric and runtime, not a general framework or latency conclusion.
