---
name: work
description: Profile a natural-language WinUI scenario, retain all actual expert recommendations, then use independent approved ADE coding subagents and compare each variant with one shared baseline. Use for performance investigations, not quick code questions.
user-invocable: true
---

# WinUI performance investigation

Use the registered `ui-profiler-harness` MCP tools for investigation operations, not a shell
pipeline. The real controller chooses the application project and UI plan from the user's
**natural language**, source and observed UI. Deterministic adapters own build/deployment,
external UI Automation, validation, state and capture limits. The installed ADE runtime owns
exact-hash approval, isolated coding, verification and provenance.

Read `harness_contracts` before planning or pivoting. After `begin`, supply the run directory to
read the frozen, hash-bound contract copy. Its rules are mandatory guidance; executable gates,
not a model interpretation, decide whether an action is permitted.

## Collect the user's inputs

Collect a clean independent application Git repository, the user's scenario verbatim, current
MUX source, complete MUX profiler CLI deployment, .NET SDK and the actual `WinUI_Expert_Agent`
profile/RAG bundle. Use a short, fresh run directory outside every input root, and an existing
licensed GitHub credential. Require explicit preview and measurement authorization: one shared baseline
and one candidate per separately approved recommendation in the finite frozen report.
Without an authorization, ask the human through the host's user-question tool, or stop.
Do not grant it yourself or alter a recorded request.

The adapter supports unpackaged, self-contained .NET WinUI apps. Do not invent C++/packaged
support, SDK changes or app integration. Checked-in scenario JSON is an optional regression
fixture, **never required runtime input or a substitute for this user's request**.
Do not refresh expert collectors, install agents globally or replace the actual expert.

## Plan through tools

In a request-bound standalone session, call `begin` without arguments. In Agency, pass the
collected paths, verbatim scenario and explicit permissions to `begin`. Then:

1. Use `inspect_application`; read relevant app/project/scoped instructions using read tools.
   Select its actual tracked `.csproj`, release configuration and executable name.
2. Use `preview_application` with that build and a bounded window size. This is one separately
   authorized **unrecorded** preview, not a baseline. Inspect with `inspect_ui`.
3. Choose observed selectors and safe `act_on_ui` actions to resolve the requested scenario.
   Inspect changed UI as needed. Plan preparations that work from a fresh app launch, including
   meaningful transitions/assertions. Ambiguous matches need an observed index, not a guess.
4. Submit the generated `{build, window, steps}` object to `freeze_scenario`. An invalid plan
   may be revised before freezing, within the same user intent and preview limits. The tool
   preserves the request, verifies observations and closes only the owned preview.

No application-specific action sequence is prescribed. Allowed actions are `type-text`, bounded
navigation `keys`/Ctrl+A, `invoke`, `assert`, `expand` and `collapse`; never arbitrary scripts.
At most 32 plan steps, one ten-minute preview, 64 actions and 64 inspections are permitted.
Use `investigation_status` to read actual state, hashes, spent budgets and failures.
Call harness operations sequentially; their exclusive operation lock is not a parallel task queue.

## Evidence, expert and proposal

With a frozen plan and measurement consent, call `measure_baseline` once, then
`analyze_baseline`. Analysis covers every supported retained family, not just ticks.
Call `recommend_fix`: it invokes the actual WinUI expert with current source/KB and both
read-only servers: unchanged `winui-profiler` for original recording queries, and the plugin's
`ui-profiler-evidence` for bounded, hash-verified multidimensional drilldowns.
The expert must make successful real calls to both and return a validated ranked recommendation report.
Keep every item, including insufficient-evidence items with no file scope; those cannot become proposals.
Each actionable item is an independently testable variant of the ORIGINAL source, not a cumulative fix.

Call `propose_fix(recommendation_id)` for each actionable id. Every trial has a fresh ADE `change`
workstream/pack and later coding subagent/session, original-source working copy and shared baseline.
There is no arbitrary two-file cap: every changed existing C#/XAML file must belong to that
recommendation's explicitly approved scope. All other files are protected.
Use `recommendation_report` to show the full queue, reasons/risks, scopes, plans, run ids and exact
SHA-256 values. Stop for the human. No harness tool approves, runs or applies.

## Legal pivots and failure handling

Before freezing, resolve invalid selectors/plans through the authorized preview. After freezing,
never rewrite the plan/request to pass a check. Reserved/failed recordings remain spent.
No retry, second candidate for the same recommendation, repair loop, substituted expert or lock/budget
reset. A failed independent trial stays in the report; another separately approved recommendation
may run, but original-source drift stops everything. Candidate profiling is strictly serialized.
If new inputs/permissions are needed, use a separately authorized investigation.

## Human approval

Use the host's user-question tool to ask for approval of each exact proposal hash.
A request to investigate is not approval. Never call `approve` without the human's answer.
If nobody can answer, leave the run awaiting approval.

After approval, call `scripts\ade.ps1 approve --run <id> --hash <hash>
--runs-dir <run-directory>\trials\<recommendation-id>\ade-runs`.
Do not change proposal inputs afterward.

## Execute and report

Call `scripts\Start-Investigation.ps1 -Phase Run -Directory <run-directory>
-RecommendationId <approved-id>`; multiple approved ids are a sequential list.
Every real ADE Copilot coding subagent starts from original source in its own fresh working copy/state.
It has no shell/URL permission. ADE's check builds/deploys that candidate and replays the scenario once.
Never call profiler `live` or UI actions outside that check, reset capture budgets, or take repair runs.

Read `recommendations-report.md`/`.json` for all recommendations and outcomes, including failures and
unmeasured items. Read each trial's ADE report/diff and `trials\<id>\multidimensional\comparison.md`/
`comparison.json`/`comparison.csv` against the shared baseline. Phase Report reads the aggregate.
The full JSON/CSV compare all supported dimensions; per-side analysis/manifests expose drilldowns and
missing/partial data. Verify the event chain with Phase Verify.
Explain the actual change, functional assertions, recording quality, absolute/percentage differences,
and regressions. Tick duration is elapsed time, not CPU time; wall time includes automation waits.
One pair demonstrates observed differences, not significance. Missing data is not a zero.
Passing verifies functionality and evidence comparability, not necessarily improvement.
Do not combine or apply independent ADE diffs unless separately requested and verified.

To extend an old report, use `pack\tools\Compare-Evidence.ps1` with existing measurement paths,
the exact recorded profiler payload and a new output directory. It is offline, does not consume
captures or invoke agents, and never retrofits the approved pack, proposal or original evidence.
