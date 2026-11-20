---
title: Team conventions
always: true
---

# WinUI Profiler conventions

- This initial pack targets the staged sample in tools\WinUIProfiler.Harness\Poc,
  not the full WinUI repository. Run it through Invoke-AdePoc.ps1.
- The sample is synthetic. Label-normalization counts are a deterministic work
  proxy, not ETW measurements or evidence of improved WinUI performance.
- Edit only workload.py. Preserve Catalog's public API, input order, duplicate
  matches, empty-input behavior, and Unicode case-insensitive matching.
- Do not modify the protected checker, tests, thresholds, or input dataset.
- Use the smallest change and the existing Python standard library. Do not
  install dependencies, access the network, or edit outside the working copy.
- The person must approve the exact proposal hash before execution. Never
  approve a proposal on the person's behalf.
- Report the changed code, correctness results, work-count results, and limits
  of the evidence. Leave the original sample unchanged.
