# Synthetic ADE engine sample

This intentionally inefficient catalog demonstrates ADE's proposal, approval,
edit, verification, repair, and evidence workflow. It does not host a WinUI
control, collect ETW, or prove user-visible performance improvement.

`Catalog.find` returns every matching input position, in input order, using
Unicode case-insensitive exact matching. Duplicate labels must remain separate
matches. Input labels are fixed when the catalog is constructed.

Run the correctness checks:

```powershell
python -B -m unittest discover -s tests -t .
```

Run the protected work-count check:

```powershell
python -B verify.py
```

The original sample passes correctness and fails the work-count check. The
checker includes catalog construction and 50 queries in each of 10 independent
iterations. It permits at most 1,000 label normalizations per iteration. Elapsed
time is informational only, not an acceptance criterion.

During the ADE demonstration, only `workload.py` may change. The tests, checker,
dataset, and budget are protected. Keep this seed intentionally inefficient so
that a fresh POC can reproduce the baseline failure.
