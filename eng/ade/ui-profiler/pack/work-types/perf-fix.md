---
name: perf-fix
description: Demonstrate a governed fix to the synthetic catalog work-count regression.
done: fail-to-pass
---
# Performance engine POC

1. Read the protected check's baseline result and the selected knowledge.
2. Diagnose why repeated Catalog queries exceed the label-normalization budget.
3. Edit only workload.py, preserving all behavior covered by the protected tests.
4. Run the existing correctness tests and python -B verify.py.
5. Summarize the cause, patch, correctness results, and work-count change.
6. State explicitly that this is a synthetic work-proxy demonstration, not a
   WinUI performance validation. Do not change the original staged source or
   apply the resulting patch outside the ADE working copy.
