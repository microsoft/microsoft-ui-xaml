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
2. Run the profiler_query.py overview and record the actual observations and
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
   Run python -B profiler_query.py verify.
8. Leave only ade-output deliverables. Do not change any source or apply a fix.
