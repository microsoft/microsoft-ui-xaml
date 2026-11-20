---
name: perf-fix
description: Validate a governed app-level layout fix using real WinUIProfiler telemetry.
done: fail-to-pass
---
# WinUI control POC

1. Read the selected knowledge, protected measurement contract, and baseline.
2. Check any supplied investigation against the measured evidence and source.
   Treat its proposed cause as a hypothesis to validate, not an established fact.
3. Change only ScenarioWorkload.cs while preserving the complete button tree.
4. Run the existing extractor tests and python -B -m helpers.verify --root .
5. Report the actual before/after layout counts, timing observations, and
   retained measurement paths. Distinguish workload reduction from a statistically
   supported latency claim.
6. Leave the patch in the ADE working copy for review. Do not apply it to mux.
