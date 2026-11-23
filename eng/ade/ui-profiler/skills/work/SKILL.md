---
name: work
description: Profile an independent WinUI app scenario with MUX WinUIProfiler, ask the actual WinUI expert for source-grounded recommendations, then use ADE for one approved isolated fix and one candidate comparison. Use for an end-to-end UI performance investigation, not quick code questions.
user-invocable: true
---

# WinUI performance investigation

The installed ADE runtime owns proposal approval, isolated coding, verification and provenance.
The team helpers own application build/deployment, external UI Automation and MUX WinUIProfiler capture.
SQLite is immutable measured evidence; WinUI source and the expert's existing KB are knowledge.
Never implement a replacement ADE loop, use a test executor as AI, or add harness hooks to the app.

## Locate prerequisites and collect inputs

Resolve this plugin from the launcher environment or
`$env:LOCALAPPDATA\ade\plugins\ui-profiler.txt`; do not recursively search repositories.
Run `scripts\ade.ps1 doctor`. Collect a clean application source repo, external scenario JSON,
current MUX knowledge repo, complete MUX profiler CLI deployment, .NET SDK, and actual
`WinUI_Expert_Agent` bundle. Select an existing licensed GitHub account for the isolated expert client.
Do not refresh the expert's collectors or install/schedule it globally.

For Gallery, use `scenarios\gallery-treeview.json`: launch, type TreeView, select the search result,
verify the page, and perform real collapse/expand/collapse transitions. The preparatory collapse is
necessary because its basic tree starts expanded. Other apps need independently verified UI selectors
and a build/deployment configuration, not references to this plugin in their source.

## Prepare

Call `scripts\Start-Investigation.ps1 -Phase Prepare` with those inputs and an external run directory.
The helper copies the app, builds a Release unpackaged deployment, captures once, checks recording
health, analyzes every retained profiler evidence family and calls the actual read-only WinUI expert.
It supplies the capability/coverage matrix, local RAG excerpts and current application/MUX sources;
the expert recommends, not implements. Native samples, queries and snapshots are hash-bound evidence.

The helper then creates a per-run team pack/workstream and an ADE `change` proposal. Baseline checks
reuse the hash-verified recording. Show the baseline metrics, recommended change, risks, protected
paths, proposal plan, run ID and exact SHA-256.

## Human approval

Use the host's user-question tool to ask for approval of that exact proposal hash.
A request to investigate is not approval. Never call `approve` without the human's answer.
If nobody can answer, leave the run awaiting approval.

After approval, call `scripts\ade.ps1 approve --run <id> --hash <hash> --runs-dir <run-directory>\ade-runs`.
Do not change proposal inputs afterward.

## Execute and report

Call `scripts\Start-Investigation.ps1 -Phase Run -Directory <run-directory>`.
The real ADE Copilot executor makes one small source fix in its working copy. It has no shell or URL
permission. ADE's verification builds/deploys the candidate and executes the same scenario once.
Never call profiler `live` or UI actions outside that check, reset capture budgets, or take repair runs.

Read ADE's report/diff and `multidimensional\comparison.md`/`comparison.json`/`comparison.csv`.
The full JSON/CSV compare all supported dimensions; per-side analysis/manifests expose drilldowns and
missing/partial data. Verify the event chain with Phase Verify.
Explain the actual change, functional assertions, recording quality, absolute/percentage differences,
and regressions. Tick duration is elapsed time, not CPU time; wall time includes automation waits.
One pair demonstrates observed differences, not significance. Missing data is not a zero.
Passing verifies functionality and evidence comparability, not necessarily improvement.
Do not apply the ADE diff to the original app unless separately requested.

To extend an old report, use `pack\tools\Compare-Evidence.ps1` with existing measurement paths,
the exact recorded profiler payload and a new output directory. It is offline, does not consume
captures or invoke agents, and never retrofits the approved pack, proposal or original evidence.
