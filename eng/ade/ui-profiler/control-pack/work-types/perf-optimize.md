---
name: perf-optimize
description: Validate an evidence-backed app fix against a sealed identical-scenario baseline.
done: fail-to-pass
---
# Baseline-relative app optimization

This work type overrides seed-specific counts and budgets in control-demo knowledge.
The protected scenario.json and fix-validation.json define this run's actions,
correctness, markers, editable scope, and per-run reduction. Never substitute
40 buttons or a four-layout absolute budget for those contracts.

1. Read the verified diagnosis, structured recommendation and validation contract.
   The proposed cause remains a hypothesis until validated.
2. Change only the configured ScenarioWorkload.cs. Preserve every action and its
   ordering, final tree, setup, markers, and all protected inputs.
3. Run the extractor tests and python -B -m helpers.validate_fix --root .
   Before a change, this check fails without recapturing the unchanged baseline.
   After a change it builds and captures at least ten fresh processes, independently
   recounts both SQLite sets, and requires the specified reduction in every run.
4. Report the actual validation JSON path, measurement paths, diff and build result.
   Durations are observations, not statistical acceptance criteria.
5. Leave changes only in the ADE working copy. Do not commit, push or edit a PR.
   Passing the gate triggers a separate approved analysis of the validation evidence;
   it does not by itself establish the root cause.
