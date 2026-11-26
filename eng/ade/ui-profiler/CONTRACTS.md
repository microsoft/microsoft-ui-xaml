---
title: UI-profiler harness contracts
always: true
---

# UI-profiler harness contracts

These are the acceptance contracts for the **agent harness**, not assertions that an
agent-generated app fix is correct or faster. The harness must enforce them and its
contract suites must pass before the implementation is reported complete.

The Agency/Copilot controller plans through bounded tools. The team plugin owns
build/deployment, UI Automation, evidence and comparisons. **Only MUX WinUIProfiler
performs capture; its MCP is read-only.** GUI capture uses the profiler's native SQLite
metric persistence, not chart scraping or an external sampler. ADE owns exact-hash proposal approval,
isolated coding, protected-file enforcement and verification.

## How the harness uses this file

This is **active agent guidance**, not an English-language substitute for executable
checks. `ui-profiler-harness/harness_contracts` exposes it read-only for planning
and pivots; the standalone controller also receives it in its instructions. Each investigation
copies it into `pack\knowledge\harness-contracts.md`, records its SHA-256 in context,
supplies it to the actual WinUI expert and names it in ADE's workstream instructions.
The per-run copy is part of ADE's hash-bound approved knowledge. Changed contract
snapshots fail input verification instead of silently altering an approved run.
Tools/analysis execute from the per-run frozen pack, not mutable installation files;
the adapter/module are verified before loading. Upgrading the installed plugin does not
silently change a run's approved implementation or evidence provenance.
Before spending a baseline capture, the harness privately publishes the existing
WinUIProfiler MCP from unchanged tracked MUX source and checks its real MCP handshake/catalog,
including read-only annotations for every exposed evidence tool.
Only the private build's dependency state is restored; the original profiler sources/locks
are not edited. Its deployment is included in the approved pack.

Use contracts to choose a legal pivot:

| Situation | Legal pivot |
|---|---|
| A selector/action/plan is invalid **before freezing** | Inspect the authorized preview, revise the generated plan within the same user intent and bounded actions, and validate again. |
| Required input or authorization is missing | Ask the human for it; do not rewrite the recorded request or grant permission yourself. |
| A read-only evidence query has invalid filters/paging/pointers | Discover reported capabilities, correct the bounded query and reuse the same immutable evidence. No new capture or invented data. |
| A plan is already frozen | Do not edit it to make a check pass. Stop or start a separately authorized investigation. |
| A recording is failed, reserved, lost or incomplete | Stop that trial and retain diagnostics. No replay or budget reset for that recommendation. Another independently approved recommendation may run from original source; it cannot reuse the failed trial's slot. |
| Evidence or an expert answer is insufficient | Report insufficient evidence; do not invent observations, substitute an agent or propose an unsupported fix. |
| A proposal/input changed after approval | Refuse execution; require human review of the current exact hash. |
| A protected edit or required-check failure occurs | Mark that trial failed, never relax the check or label it successful. Keep it in the report. Original-source drift stops the entire investigation. |

The **deterministic tool/state/hash/approval checks** decide whether a pivot is
allowed. A model's interpretation of this document cannot override those gates.

## Inputs and planning

| ID | Guarantee | Refusal/failure behavior |
|---|---|---|
| INPUT-01 | Accept an independent, clean application Git repository and a nonempty **natural-language scenario**, plus explicit MUX source, profiler payload, actual WinUI expert bundle and SDK paths. | Missing/invalid inputs or dirty source stop preparation. Nothing is discarded. Checked-in scenario JSON is an optional regression fixture, not a required user input. |
| INPUT-02 | Preserve the user's scenario verbatim in a hash-bound request. Read contracts before `begin`; successful initialization binds the server to that request/run. | Different paths/scenario/permissions or changed contracts are rejected; use a new investigation rather than rewriting the old one. |
| INPUT-03 | Investigation output must be outside all application/framework source, expert, SDK and profiler input roots. | Overlapping paths, drive roots, output filesystem links, UNC/device paths and DOS short-name aliases are rejected. |
| INPUT-04 | New preparations default to GUI capture and bind its complete payload, UIA driver and SQLite reader. Explicit CLI mode and bound legacy CLI requests retain their recorded backend. | Missing/changed GUI inputs stop the run; no silent CLI capture fallback. |
| PLAN-01 | The real planning agent selects the tracked application project, build configuration, UI actions and selectors. Its output is a validated, run-local action plan. | Reject unknown fields, absolute/escaping project paths, untracked projects, unsupported actions, command-like configurations and executable paths that are not leaf `.exe` names. |
| PLAN-02 | Plans contain 1–32 uniquely named steps, a bounded window size and bounded waits/input. Selectors must have resolved in the authorized preview. | Reject missing/offscreen selectors and ambiguous matches without an observed index. A truncated inspection cannot prove a unique match. Invalid plans are not frozen or measured. |
| PLAN-03 | The same frozen scenario file, hash, build configuration and window conditions govern the shared baseline and every independent candidate. | A plan cannot be overwritten after freezing. Changed plan/request/runner hashes, different actions or transitions, DPI/window conditions or recorded runtime inputs invalidate verification/comparison. |
| TRIAL-01 | Keep the expert's complete ranked application/control/framework report. Each application `propose` item gets its own ADE proposal, fresh coding subagent/session and isolated working copy rooted in the **original** app source. Bind its id, directory, context, ADE run and proposal to shared inputs. | Reject repeated ids, swapped references and native recommendations entering the app runner. No cumulative fixes. Control/framework `recommend` items remain `target-workflow-required`, not mislabeled as missing evidence or authorized edits. |
| TRIAL-02 | Reuse one immutable original baseline recording for all independent trials, including ADE's source-matched pre-fix checks. | A recommendation does not require another investigation or baseline capture. Different source/scenario/runtime/evidence inputs invalidate reuse. Ambient machine drift still limits one-pair conclusions. |

## Allowed actions and resource limits

| ID | Guarantee | Refusal/failure behavior |
|---|---|---|
| ACTION-01 | Preview and measurements require explicit, separate authorization. An investigation request is **not** approval of a source-code proposal. | Without the corresponding authorization, no preview or baseline capture is started. Without exact-hash approval, no coding round starts. |
| ACTION-02 | UI Automation targets only the task-owned executable, PID and process creation time. Allowed actions are `type-text`, bounded navigation `keys`, `invoke`, `select`, `assert`, `expand` and `collapse`. `select` uses the resolved element's SelectionItemPattern and verifies a non-no-op transition. | Reject other actions and OS shortcuts. Reject reused PIDs/different executables. Keyboard input requires foreground and keyboard focus in the owned process. Selection never falls back to keyboard input or invocation. |
| ACTION-03 | At most one unrecorded preview launch, 64 preview actions, 64 inspections and ten minutes of preview use. Inspections return at most 512 visible elements. | Expired/spent preview use is refused. Freezing closes only the verified owned preview. Normal server shutdown attempts owned-preview cleanup; it never terminates unrelated apps. |
| CAPTURE-01 | At most one shared baseline capture and one candidate capture **per independently approved recommendation** in the finite frozen report. Source-matched completed evidence may be reused without another recording. | A reserved or failed trial budget stays spent. No retry, reset or second candidate for that id. Old sealed investigations keep their old budgets; they are not expanded. |
| CAPTURE-02 | Candidate builds/profiling are serialized under the investigation operation lock on the shared desktop. | No simultaneous candidate recordings or app interaction; interrupted locks are not silently reset. Coding subagents have separate state/source, not separate measurement desktops. |
| CAPTURE-03 | Replay deadlines derive from the frozen step count and waits, including bounded selector/state completion and startup/snapshot allowances. Short scenarios retain a 90-second minimum. | No fixed short-scenario deadline truncates an otherwise legal 32-step plan. Timeout and profiler termination are distinguished; retain diagnostics and the spent capture budget, never retry it. |
| ACTION-04 | Controller tools expose no `approve`, coding `run`, `apply` or arbitrary shell/write operation. Each coding subagent receives only declared read-only evidence MCPs; no harness capture tools. | Model requests cannot bypass per-fix approval/capture boundaries through the team tool API. Shell and URL access remain disabled in ADE's coding sessions. |

## Expert and evidence outputs

| ID | Guarantee | Refusal/failure behavior |
|---|---|---|
| EXPERT-01 | Use the actual supplied `winui-expert` profile and local KB, with current app/MUX source, not a substitute controller persona. Do not refresh collectors or install it globally. | Missing profile/RAG files stop recommendation. Record original/rendered profile and retrieval provenance; no cached recommendation from different evidence. |
| EXPERT-02 | The expert must successfully query the unchanged WinUIProfiler MCP against the **exact measured database**, and the plugin's separate read-only derived-evidence MCP. Probe native MCP readiness/read-only catalog before capturing; preserve the structured tool transcript and arguments. | Missing/broken/non-read-only native MCP prevents spending the baseline budget. Text claiming “I queried it” is insufficient. Missing/failed/wrong-server/wrong-database queries cannot satisfy the contract. |
| EXPERT-03 | New v3 reports require application/control/framework coverage and identical question/time/CPU/memory/model-analysis/unverified/validation sections at every level, successful source reads at all three levels, 1–64 uniquely identified recommendations and repository-relative scope. App `propose` scopes remain existing app C#/XAML; native `recommend` scopes are existing source under MUX `controls\dev` or `dxaml\xcp`. Total output is bounded to one MiB; legacy v1/v2 remain readable. | Reject missing levels/reads/analysis sections, malformed fields, duplicate/escaping ids, wrong-repository/layer paths and protected files. `insufficient-evidence` has no file scope. Native recommendations cannot become app trials. |
| EXPERT-04 | State whether the measured runtime is proven to match inspected MUX source. Preserve stack-drop/symbol limitations. Existing valid captures may receive a new read-only three-level investigation/report without a build or capture. | No runtime/source equivalence inferred from checkout location. Never rewrite a sealed pack/context, reset a budget, or call a read-only report an approved/verified candidate. |
| NATIVE-01 | A separate native trial has a fresh ADE coding workstream/session and immutable current-source snapshot. Its exact-hash proposal declares one source-built baseline plus one candidate capture. The application stays unchanged. | No old runtime/source parity inference, reset discovery budget, imported sibling patch, or native edit through the app runner. No build/capture during proposal preparation; execution requires exact-hash approval. |
| NATIVE-02 | Build only the declared native target through `initrun.ps1`; stage and hash its private runtime/resource closure. Verify the task-owned app actually loads that private runtime before and after actions. | Missing prerequisites, ambiguous/incompatible output, changed source, partial closure or fallback to installed runtime fail explicitly. Never globally install/register native candidates. |
| NATIVE-03 | Native comparisons intentionally permit changed framework hashes only with source/runtime provenance for both recordings, the same native trial, and unchanged application source/executable. Serialize captures and retain every outcome. | Legacy app comparisons still require identical framework hashes. Missing native provenance, cross-trial pairs, scope escape, missing candidate comparison or repeated/spent captures cannot become success. |
| INTENT-01 | New planning uses the hash-bound additive intent catalog and a frozen `ui-profiler/intent/v1` specification: one question, generic app/control target, setup, action, actual step count/waits, valid-run checks, metrics, falsifiable healthy result and regressions. Repetitions are exactly one per baseline/candidate side. | Do not hardcode a Gallery/control flow, retrofit old plans, execute descriptive intent text, increase budgets, or substitute UI ticks for unsupported presented-frame waits. |
| ATTRIBUTION-01 | Measurements come only from MUX WinUIProfiler; model/source/KB reasoning may explain mechanisms and propose hypotheses. Control reports retain process-local instance identities, proven nested spans, capture-local subtree counts and explicit missing CPU/owned-memory attribution. | No alternate profiler, inferred CPU/bytes presented as measurements, process growth assigned to one control, overlapping subtree totals summed, cross-process pointer joins, or activity counts treated as elapsed cost. |
| DRAFT-PR-01 | Opt-in draft publication requires exact approval, verified ADE checks/event chain, immutable originals, a hash-verified measured comparison and an observed primary improvement. Export only approved scoped fixes into original Git history; preserve repository PR templates and exact validation commands/results. | No PR from baseline-only/no-improvement/failed trials, invented links, imported harness/sibling changes, force push, global credential switch or universal/statistical speedup claim. Regressions and unavailable evidence remain visible. |
| PUBLICATION-01 | External work-item creation and draft PR creation are separate explicit opt-ins, both off by default. Local task/action-item and applied-fix/evidence reporting is mandatory regardless of either opt-in. | Planning, analysis, preparation and report generation never imply permission to create Azure DevOps/GitHub items. A task row is not a created external work item, and missing opt-in never suppresses a local report. |
| EVIDENCE-01 | Measurements must contain passing action assertions, complete ticks and finalized, untrimmed event recordings with zero ETW/profiler event loss. | Missing/non-finite metrics, failed/no-op transitions, lost/dropped/trimmed/unfinished events or changed evidence fail verification. CPU **stack** drops remain separately reported as sampling quality. |
| EVIDENCE-02 | Bind SQLite, action audits, native samples, query exports, snapshots and derived reports to explicit file hashes/manifests. Legacy evidence is labeled honestly. | Reject tampered/missing/unbound reports, wrong schemas/references and cyclic manifests. Recursively interpret only references explicitly marked `evidence-manifest`, not every file named `manifest.json`. Integrity checking is not a cryptographic signature. |
| EVIDENCE-03 | Evidence tools are read-only, report-scoped and bounded. Preserve units, nulls, availability, exact integer bytes, scope and query/computation provenance. | No arbitrary SQL, launch, capture or filesystem write. Page scopes/metrics/drilldowns; reject non-finite numbers and invalid views/limits/pointers. Large integers include exact decimal strings for clients with limited numeric precision. |
| COMPARE-01 | Deterministically compare every supported retained family: actions/ticks/cadence, layout/render/events/regions, CPU/stacks, process memory, GPU utilization/memory, graphics/textures, objects/trees/resources and launch. | Never silently limit comparison to tick metrics. Preserve unavailable/unsupported/partial states. Missing evidence is not zero; IDs/handles from independent processes are not ownership matches. |

## Approval, source isolation and completion

| ID | Guarantee | Refusal/failure behavior |
|---|---|---|
| APPROVAL-01 | **Every trial** requires human approval of its exact current ADE proposal SHA-256, binding its pack/workstream/runtime/source and explicit file scope. | Missing/wrong/stale approval or changed approved inputs prevents that execution. Unapproved trials stay unspent; the controller does not approve for the human. |
| POLICY-01 | Each subagent's single coding round may change only the recommendation's explicitly approved existing C#/XAML file set, however many files the fix needs. All other tracked files, tests, instructions, projects, manifests, dependencies and build settings remain protected. | Detect/reject added, deleted, unrelated or out-of-scope changes. A three-file fix is legal when all three are approved; a fourth unapproved file is not. No app/profiler/harness coupling. |
| ISOLATION-01 | The original repository remains unchanged throughout **all** trials. Every fix stays in its own ADE working copy/diff. | Original-source drift stops the investigation. Sibling fixes never become another trial's baseline. No automatic merging or application; combining variants requires a separately authorized verification. |
| OUTCOME-01 | Mark an individual trial `verified-comparison` only after required ADE checks, event-chain/evidence verification and original-source checks pass. Keep a report of **every** recommendation, proposal, outcome and full baseline comparison. | Failed/missing checks, malformed output, interruptions or incomplete evidence cannot become success. Failed, pending and insufficient-evidence entries are not omitted. No hidden fallback, synthetic recording or repair loop. |
| REPORT-01 | Persist `recommendations-report.md`/`.json`, with a local task/action list, recommendation ids, reasons, scopes, risks, exact approvals/ADE runs, honest statuses, actual applied-fix/evidence references and per-trial multidimensional Markdown/JSON/CSV comparisons. Reporting does not depend on external publication opt-in. Bind outcomes/comparisons to hashes. | Changed report inputs/outcomes/comparisons are refused. Proposed changes are never labeled applied; unavailable fix evidence is explicit. An aggregate report is not a combined-patch measurement or scalar score; a failed trial prevents an overall success claim. |
| OUTCOME-02 | Functional/evidence success is distinct from performance improvement. Publish absolute/percentage differences, regressions, workload context and missing capabilities. | One pair is observational, not statistical significance or a universal speedup. UI ticks are elapsed UI-thread time, not CPU work or displayed frames; automation waits remain included. No scalar score hides regressions. |
| ADE-BUG-01 | For ADE's **bug** work type, the target check must fail before the fix, then pass afterward while baseline-passing regression checks stay passing. | An already-passing target cannot become an approvable bug proposal. This performance harness uses **change**: its functional scenario passes at baseline and must keep passing; it does not fabricate a failing performance threshold. |

## Required artifacts

| Artifact | Contract/schema |
|---|---|
| `request.json` | `ui-profiler/request/v2`: language, input paths, permissions, independent strategy/candidate policy and contracts hash |
| `scenario.json` | `ui-profiler/scenario/v1`: generated, validated, frozen internal replay plan |
| `context.json` | `ui-profiler/investigation/v3`: request/contracts/source/plan/shared evidence, recommendation/trial/report bindings |
| Per-trial `pack\knowledge\trial-context.json` | `ui-profiler/trial/v1`: original-source baseline, recommendation id, approved file scope and frozen implementation bindings |
| Measurement and analysis | `ui-profiler/measurement/v2` for new captures; `ui-profiler/analysis/v1` and `ui-profiler/evidence-manifest/v1` for derived evidence |
| Expert output | `ui-profiler/recommendations/v3` (legacy v1/v2 readable), identical deep three-level `recommendation.md`, actual MCP/source-read transcript and `provenance.json` |
| Recommendation report and outcomes | `ui-profiler/recommendation-report/v3` (legacy v1/v2 readable) and per-trial `ui-profiler/trial-outcome/v1`; all levels, execution eligibility and statuses retained |
| Read-only investigation | `ui-profiler/read-only-investigation/v1`; existing valid baseline/analysis in a new directory, zero captures and no edit authorization |
| Independent trial set | `ui-profiler/recommendation-trials/v1`, per-target app/native contexts, exact-hash ADE proposals, `ui-profiler/recommendation-trial-report/v1` retaining all levels |
| Native trial/runtime | `ui-profiler/native-trial/v1`, `ui-profiler/native-runtime/v1`; approved bounded capture pair, native source/closure hashes and actual private-module load checks |
| Control-scoped evidence | `ui-profiler/control-analysis/v1`, Markdown and immutable manifest; a supplied control type, native object timeline evidence and explicit attribution/availability |
| MCP results | `ui-profiler/tool-result/v1` and `ui-profiler/evidence-query/v1` |
| ADE proposal/run | ADE-owned proposal, approval, event chain, diff, check logs and provenance |
| `completion.json` | `ui-profiler/completion/v2`: `verified-comparisons`, `partial` or `failed-trials`; not a blanket improvement or combined fix |

## Contract tests

Run the combined suite with the documented prerequisites and an activated local
Python environment:

```powershell
.\tests\Test-Harness.ps1
```

The suite contains:

| Suite | What it verifies |
|---|---|
| `tests\Test-Harness.ps1` | Shared/per-trial capture budgets, unique selections/bound trial references, finite/complete metrics, identical actions/conditions, primary report tables/missing-data states, retention/tamper/false-success checks, deployment and same-connection snapshots |
| `tests\Test-Analysis.py` | Multidimensional arithmetic, CPU drop/symbol quality, memory/GPU coverage, unsigned bytes, ownership ambiguity, unsupported states and manifest integrity |
| `tests\Test-AgentContracts.py` | Model-plan/output validation, selector/path/action bounds, frozen contract/input integrity, real stdio MCP schemas/error replies, bounded evidence queries and false-success rejection |
| `tests\Test-Contracts.ps1` | **Actual ADE runtime**: baseline/approval/input/check contracts, multi-file approved scopes, independent recommendation working copies, source isolation and no repeated completed run |

ADE boundary tests use explicitly labeled, inert **script test adapters**, not AI
executors. They operate only on synthetic temporary repositories. They never build
or launch an app, call a model, consume a real capture, change approved evidence or
apply a patch to the user's repository.

These contracts are permission/state/integrity policies, **not an OS sandbox** or
an assurance that arbitrary generated code is correct. Real UI/profiler end-to-end
validation is separate and requires explicit authorization.
