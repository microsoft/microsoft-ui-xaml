# SDLC Agent

Standalone C#/.NET 10 local SDLC orchestrator using Microsoft Agent Framework (MAF) agents and GitHub Copilot for every model call. Application-owned SQLite checkpoints drive scheduling and recovery; this does not depend on retaining a live chat or serializing an arbitrary running process.

## Requirements and trust boundary

- Windows, Git with worktree support, and the .NET 10 SDK/runtime.
- An authenticated GitHub Copilot account with model access. Build downloads the SDK-matched runtime from official GitHub releases and verifies its checksum. NuGet packages use the tool-local official NuGet v2 feed.
- A **trusted** local repository and a dedicated, existing integration branch that is **not checked out in any worktree**. Workers are native, non-elevated processes, **not an OS sandbox**. Approved repository scripts retain the user's ambient filesystem, credential, and network access. Inspect scripts and dependency sources before trusting them.

Run the host from a normal, non-administrator terminal. Elevated/root hosts and real workers are refused. `doctor` performs runtime/authentication diagnostics only and does not dispatch a repository worker.

## Build and test

From the WinUI repository root, using the system .NET 10 installation rather than the repository's .NET 9 installation:

```powershell
$dotnet = 'C:\Program Files\dotnet\dotnet.exe'
& $dotnet restore tools\SDLCAgent\Tests\SDLCAgent.Tests.csproj --configfile tools\SDLCAgent\NuGet.config --locked-mode --verbosity quiet
.\initrun.ps1 $dotnet build tools\SDLCAgent\Tests\SDLCAgent.Tests.csproj --no-restore --verbosity quiet /p:Platform=AnyCPU /nologo
$env:DOTNET_ROOT = 'C:\Program Files\dotnet'
& .\tools\SDLCAgent\Tests\bin\Debug\net10.0\SDLCAgent.Tests.exe
```

The test executable creates and removes its own disposable Git repositories. It tests durable orchestration with a deterministic fake model, including two concurrent pairs, saved phased plans, TDD, autonomous repair, exact-version approval, pause/restart, integration receipts, stale assignments/refs, flaky-gate rejection, and the live authenticated HTTP/viewer routes. It never uses the WinUI integration branch as a fixture.

Optional tests requiring a **normal terminal** and, for the second command, real Copilot access:

```powershell
& .\tools\SDLCAgent\Tests\bin\Debug\net10.0\SDLCAgent.Tests.exe --host-smoke
& .\tools\SDLCAgent\Tests\bin\Debug\net10.0\SDLCAgent.Tests.exe --copilot-smoke
```

The first tests a separate background host's startup, independent lifetime, authenticated API, viewer, and shutdown. The second runs a real Copilot-backed MAF agent against a read-only fixture and verifies that it invokes the scoped list/read tools. Both passed from a non-elevated terminal. Application tools use an `sdlc_` prefix to avoid collisions with runtime-native tool names; the smoke verifies execution reached the application handlers, not merely that the model returned the expected text.

Worker protocol regressions also cover the recorded commentary-before-JSON failure, selection of only the terminal assistant message, retained progress evidence, result-only repairs without reexecuting implementation, and preservation of plans/step cursors when protocol repair is exhausted or transport completion is uncertain.

`SDLCAgent.Tests.exe --viewer-smoke` separately uses an installed Microsoft Edge with an isolated temporary profile to exercise real browser JavaScript, bearer authentication, specification drill-down, inline answers/feedback, draft preservation during refresh, specification revision, and explicit exact-version approval. It uses disposable repositories and a deterministic fake model; it does not dispatch real Copilot workers or approve a real run.

## Start a run

1. In the **target repository**, create a dedicated delivery branch while leaving the checkout on another branch:

   ```powershell
   git branch user/neeverma/sdlc-delivery
   ```

2. Copy `examples\run-request.json` outside the source checkout. Replace its repository, problem statement, branch, and check commands with your actual repository configuration. The sample assumes a .NET solution; native WinUI needs its own explicitly authorized scripts. Checks are executable/argument arrays, not shell strings. Use non-interactive checks that fail on unmet conditions and leave tracked/untracked files unchanged; generated ignored build outputs are fine. Include restore/build/test/format checks appropriate to the repository, and do not put credentials in arguments.

3. From a normal terminal, start the host and submit the request:

   ```powershell
   $agent = 'C:\microsoft-ui-xaml\tools\SDLCAgent\bin\Debug\net10.0\SDLCAgent.exe'
   & $agent doctor
   & $agent start
   & $agent create --request C:\work\run-request.json
   & $agent list
   & $agent status RUN_ID
   & $agent viewer
   ```

   `start` returns only after the independently running host responds. Closing the terminal or browser does not stop it. `serve` is the foreground alternative. Copy the URL printed by `viewer` into a browser; its fragment carries a local bearer credential. Do not share that URL or the data directory's `host.json`.

4. SpecCreator inspects a pinned repository revision and proposes a versioned specification. In the web UI, select the run. At `AwaitingApproval`, the **Review specification** panel shows the full specification, success criteria/measurements, execution policy, and a text box for each question. Enter answers or additional feedback and click **Send feedback / answers**. SpecCreator revises the specification and requests review again. Draft text survives live refresh and switching runs in the current page; unsent drafts are not persisted across browser reload/closure.

   When there are no outstanding questions or unsent edits, check the review confirmation and click **Approve and start implementation**. Approval is bound to the displayed specification version; stale feedback/approval is rejected instead of being applied to a newer version. Paused/blocked runs also allow feedback once active work has drained. The overview provides **Pause run** / **Resume run** controls.

   The CLI remains available as an alternative:

   ```powershell
   & $agent artifact SPEC_ARTIFACT_ID
   & $agent feedback RUN_ID --file C:\work\feedback.txt
   & $agent status RUN_ID
   & $agent approve RUN_ID --version CURRENT_SPEC_ARTIFACT_ID
   ```

   Approval requires the current artifact ID, a verified content hash, and no outstanding questions. It records the run's policy hash. No implementation starts before this approval. Feedback creates a new specification version; previous approvals cannot authorize it.

At **Blocked**, the viewer shows an **Unblock this run** instructions field, even when no specification exists yet. Once all active work drains, **Send unblock instructions** queues a durable request for the Orchestrator's read-only assessment. It either explains why the run must stay blocked or resumes the existing phase with scoped worker guidance. Original plans, cursors, approval, recovery guards and gates are preserved; no uncertain execution is authorized by this assessment. Requirement changes still use specification feedback and fresh approval. Requests are bound to the displayed blocked-run revision. Drafts survive live refresh and switching runs within the page; submitted requests and decisions survive host restart. Pause/resume can suspend the assessment without reopening workers prematurely.

Reported token usage is displayed in **millions (M)** and remains explicitly partial/unknown when provider usage is unavailable. The **Execution** section lists commits recorded during this run, including not-yet-integrated worker changes and newly created integration candidates, with task, role, time and full hash. A listed candidate is not evidence that its gates passed. The list excludes inherited commits and paginates older records; separate expandable branch histories still show the full managed branch or task range. Historical candidates not recorded as logical commits remain visible in branch history. `SDLCAgent.Tests.exe --dashboard-controls` validates unblock controls/assessment, persistence and commit projection; `--viewer-smoke` exercises the input, token units, live commit list and authenticated submission in Edge. `--copilot-unblock-smoke` validates a real read-only Copilot Orchestrator assessment without dispatching implementation.

After approval, the orchestrator runs autonomously: decomposition, paired planning, implementer-owned API scaffolding, test preparation, successful build prerequisites, meaningful-red assessment, implementation, independent saved verification steps, combined-candidate gates, integration, and final goal verification. Overlapping file scopes are serialized through dependencies. A final failing goal triggers bounded replanning rather than success based on worker claims. A genuine specification gap blocks for exceptional human clarification rather than silently changing the approved goal or repeatedly executing the same impossible step.

## Worker and validation contracts

### Task obligations and criterion coverage

Every new work package declares non-empty `obligations`: each has a unique task-local `id`, a parent `criterionId`, bounded `outcome`, measurable `measurement`, owned `paths` and authorized `checks`. `criterionIds` are traceability links, **not** a requirement to finish every referenced parent criterion in every task. Shared criteria must be split into independently achievable slices: an upstream fixture task can verify the sample portion of C16 while a downstream README task verifies C16's documentation portion. Neither upstream task plans nor verification may require the dependent README work to be complete first.

Before scheduling, SpecCreator independently reviews the union of obligations against every full approved outcome and measurement, including negative checks and UI/documentation evidence. Duties must have capable owners within the declared paths and completed prerequisites; omitted requirements and downstream-dependent duties are rejected with bounded read-only replanning. This is model-assisted coverage review, not a formal proof of semantic completeness. Structural validation rejects missing obligations, unmapped criteria, duplicate obligation IDs and out-of-scope obligation paths.

Task verification uses `TaskVerificationResult`, separate from final-goal `Verification`. A persisted message binds the assignment, task, approved spec/hash and exact source revision. The Tester assesses **each assigned obligation ID**, citing supplied fresh command evidence or current-message read observations, with `satisfied`, `unsatisfied` or `notVerified`. The task passes only when all its assigned obligations are satisfied; unfinished obligations of other tasks are not its defects. Missing/fabricated assessment entries get bounded read-only correction; unverified evidence blocks at the verification boundary without replaying implementation or regenerating worker plans. An explicitly evidenced unmet obligation retains the existing bounded behavioral-repair path.

Normal task checks, exact-revision integration gates and **full parent-criterion final verification remain mandatory**. Partial obligations never waive any approved criterion or measurement. Historical run definitions lacking obligations remain readable but require explicit replanning before execution; deployment does not silently reinterpret, resume or modify blocked runs. Worker plan contract version remains 3 because its role/step schema is unchanged.

`SDLCAgent.Tests.exe --obligations` covers the fixture/README shared-criterion dependency, rejected coverage/replanning, missing and foreign evidence, unchanged implementation cursors, stale identities and rejection by the complete final gate. `--copilot-obligations-smoke` uses real read-only Copilot to verify only an upstream C16 fixture obligation while downstream documentation remains pending.

Worker plans use **`contractVersion: 3`** and separate typed role contracts. An `ImplementerPlan` has required `scaffoldingSteps` and `implementationSteps`; a `TesterPlan` has required `preparationSteps` and `verificationSteps`. Workers cannot invent phase labels or transfer lifecycle ownership. Writable-capable steps declare explicit `writePaths` within the role's assigned paths and authorized successful `checks`; `[]` is a read-only inspection/self-check. Independent verification uses a separate read-only step type with `passChecks` and no write-path field. Implementer self-checks are allowed inside implementation steps, not a competing independent-verification phase. JSON schemas come from these types and accompany initial calls and result repairs.

The orchestrator alone owns scaffold -> test preparation -> prerequisite build -> meaningful red -> implementation -> independent green verification. Setup steps can declare/run only restore/build prerequisites, never later behavior tests. Every declared check is a success condition executed independently before step completion; red assessment is an orchestrator-owned operation, not a plan step that could accidentally execute after implementation. Scaffolding supplies compiling API shapes/project plumbing, not completed behavior. A tester never creates production stubs. The saved tester plan is supplied to the implementer as reference-only interface context when scaffolding begins.

Execution requests contain **one current step**, not the whole executable plan: assignment ID, saved plan ID, role/stage, step, actual write/check capabilities, and completion conditions. Results identify that exact assignment/plan/step and report `completed`, `blocked`, or `failed`, a summary, current-step `evidenceIds`, and a structured blocker when incomplete. Repository/check tools return auditable evidence IDs; completion rejects fabricated, other-step/assignment, failed, or incomplete check evidence. Tool calls and assignments must still match the persisted step/cursor. Lacking tools for a later writing step cannot invalidate a completed read-only inspection. Contradictory results receive tool-free current-step clarification, never automatic permission expansion, fabricated completion, or execution replay.

Malformed result formatting is repaired without tools or task replay. A structurally valid but unsafe plan instead gets bounded **read-only replanning** with a machine-readable violation (code, section, step, field, permitted values). Repeated identical plan violations stop rather than looping. Exhaustion produces `WorkerContractBlocked` before execution, preserving saved state rather than consuming eight whole-task attempts. Legacy v1/v2 plans remain readable as audit artifacts but cannot execute under v3; they need explicit regeneration. Deploying the new host does not rewrite, approve, resume, or repair blocked runs.

Structured blockers route missing production APIs to implementer scaffolding, defective test setup to tester preparation, and behavioral failures to implementer implementation. Only the owning role's plan is regenerated; the counterpart's saved plan is retained. Suspected cross-role defects and specification gaps use the review protocol below; only SpecCreator-confirmed product decisions require human specification feedback. External infrastructure, missing capabilities, or dependencies outside the assigned role scope block without expanding permissions. An identical blocker on an unchanged revision stops with a no-progress diagnosis. Failed-worker edits remain preserved and subject to the existing clean-handoff guard.

### Cross-role review and disputes

An evidence-reported production failure with an explicit file outside the implementer's assignment is not retried by that same incapable owner. The orchestrator queues a bounded **dependency assignment repair** after active pairs drain, preserves the clean committed worktree, and proposes a revised assignment adding exactly that production file. Independent SpecCreator review must confirm all existing approved outcomes and measurements remain covered. Identity, dependencies, criteria, checks, test scopes and TDD policy cannot change; overlapping unfinished owners require explicit replanning instead. Accepted revisions archive the old assignment/plans/cursor, regenerate only the implementer repair plan, retain the tester plan and all gates, and revoke stale worker assignments. Two scope repairs per item are allowed; unknown effects, dirty handoffs and pending diagnosis/review cannot authorize scope changes. `SDLCAgent.Tests.exe --dependency-repairs` covers successful gated completion, rejected reviews and broader-scope proposals.

The approved specification is authoritative; tests, implementation, and worker conclusions are claims against it. A worker can return `status: reviewRequested`, `blocker: null`, current-step evidence IDs, and a `challenge` containing `target` (`test`, `implementation`, or `specificationInterpretation`), `claim`, assigned `criterionIds`, and repository-relative subject `paths`. Suspected test defects are not automatically product-specification gaps. Older-style `specificationGap` results also receive SpecCreator review before human escalation; existing blocked runs are not automatically reopened.

The orchestrator durably saves the challenge envelope (source assignment, step/plan, approved spec/hash, source revision, evidence) and pending review message before dispatch. The counterparty inspects read-only and returns a decision with exact approved requirement quotations and observations from the current message. Admitted owner defects receive an owner-scoped repair; disagreement or accusations against another role go to read-only SpecCreator adjudication. Only a genuine requirement ambiguity/change reaches a saved `SpecificationChangeRequest` and human feedback/fresh approval. The orchestrator never rewrites the approved specification, grants another role's paths, or waives checks.

Repair plans are separate saved artifacts, visible alongside normal plans in the viewer. They preserve original plans, completed steps, and the original return cursor. Before/after checks, scoped commits, and independent repair verification are retained: SpecCreator independently reviews tester corrections against the requirements and original diff; Tester reviews production corrections. Test repairs must preserve assertions and coverage rather than skip tests, relax expectations without requirement support, change the runner, or delete failures. A real source correction is required. Review acceptance resolves only the narrow dispute; original step completion, full item verification, combined-candidate integration gates, and final goal verification still run on fresh source.

Review stage, outbox message, decisions, repair cursor, revision, and original cursor survive pause/host restart. New assignments invalidate old replies; stale spec/revision/message identities are rejected. A clean handoff is required, and uncertain write completion or dirty files block without replay or cleanup. Repeated unchanged disputes go directly to SpecCreator, with at most three review rounds per item. Rejected repairs, exhausted reviews, infrastructure/scope problems, and genuine product decisions block explicitly. Independent review remains model-assisted, not a formal proof that a test or implementation is correct.

`SDLCAgent.Tests.exe --reviews` exercises accepted and contested tester defects, independent repair rejection, unchanged-dispute limits, stale revisions/messages/assignments, scoped permissions, and pause/restart without replaying completed decisions or repairs.

`SDLCAgent.Tests.exe --copilot-review-smoke` independently reassesses an isolated BFS test contradiction using real Copilot, the actual review prompt/schema, scoped read-only tools, exact approved-requirement citations, and current-message evidence. It does not dispatch workers against a real product run.

Validation batches are ordered restore, build, other checks, then tests, and stop on the first failure. Tests cannot use an old executable after compilation fails: each configured build must have successful, durable evidence for the same workspace, approved command definition, and current source fingerprint (HEAD, tracked diff, and untracked file contents). New restores invalidate build evidence; a new build attempt invalidates its previous evidence before execution, including on failure/interruption. Test-only check configurations without a separate build command are treated as self-contained runners and must build/validate their own inputs correctly. Decompositions must include configured restore/build prerequisites with their test checks.

Source-changing validation results are rejected, and checks within one workspace are serialized. Ignored build outputs are not fingerprinted: authorized scripts remain trusted to actually rebuild and test what they claim; this is not binary attestation or an OS sandbox. Captured command output and source identities remain auditable. Compilation/setup failures return to role-scoped planning/scaffolding with their evidence and a separate bounded preparation-repair count; only actual test failures reach meaningful-red assessment. Scaffolding phase and step cursors survive pause/restart.

Focused regression validation: `SDLCAgent.Tests.exe --contracts`. This includes the recorded read-only-inspection/later-write contradiction, step identity/evidence rejection, role-specific schemas, implementer self-checks versus read-only independent verification, bounded blocker routing, scaffolding pause/restart, and a real disposable .NET project that first fails with `CS0246`, then compiles with stubs and fails behaviorally, and finally passes after implementation. It also checks stale-build rejection. The optional `--copilot-smoke` validates actual role-specific plans and current-step execution through Copilot in addition to the provider tools.

Task verification supplies a current-message, evidence-bound provenance observation: verified task worktree/branch ownership, the exact source revision, commit ancestry with changed paths, and staged execution/TDD audit records. This lets a read-only tester measure process obligations such as scaffold-before-test ordering without native Git or shell permissions. Historical records remain historical; the observation is freshly tied to the current verification message and cannot prove source behavior or omitted truncated history. `--obligations` covers this process obligation alongside missing/foreign evidence rejection; `--workspace-gates` also rejects stale or dirty provenance.

Check prerequisite failures are recorded before command dispatch and therefore have no command side effects. If a current step already committed its changes before a stale-build rejection, diagnosis can reconcile only that exact clean revision and complete it **read-only**, with all write/delete/commit capabilities removed. The scoped commit, complete intervening audit interval and fresh source snapshot must prove no subsequent command/write was dispatched; other uncertain outcomes and policy denials still block. Original failure records, plans, cursor, recovery limits and every independent gate remain mandatory. `--diagnosis` covers this committed-step continuation and unsafe recovery rejection.

## Diagnosis and bounded execution recovery

Application tool failures return `success=false`, a durable failure evidence ID, redacted arguments (write content is represented only by length/hash), exception type/message, and side effects (`none`, `confirmed`, or `unknown`). Pre-write validation failures are distinguished from failures after a write/delete/commit/check may have started. Permission denials, uncertain effects, or three rejected calls disable further mutations/checks in that worker session. Cancellation is not converted into a normal tool result.

Every project/dependency manifest edit still requires `dependencyJustification`, including fixture-copy configuration that leaves dependencies unchanged. Missing justification returns a specific correction before any repository write. Known input errors can be corrected within the worker session; if reported as a blocker, the orchestrator supplies the deterministic correction without another LLM investigation.

Other execution/tool blockers are checkpointed for a **read-only Diagnosis worker**, using GitHub Copilot through the same MAF adapter. Its tools are limited to repository list/read/diff: no writes, commands/checks, commits, permission edits or worker dispatch. The message includes the original step, actual failure evidence, policy snapshot and source identity. The reply must match the pending assignment/message/specification/source and cite current-message observations. Diagnosis proposes a cause (or explicitly unknown), a targeted instruction and verification; it cannot change the approved requirements, worker plan, role, paths, checks or policy.

The orchestrator authorizes continuation only when all recorded failures are confirmed pre-mutation, no policy denial occurred, and the original step's starting HEAD and clean tracked/untracked tree are unchanged. Advice alone never proves safety. Unknown/confirmed effects, dirty or changed source, and uncertain transport/protocol completion receive diagnosis but require operator reconciliation before any further execution; there is no automatic replay or cleanup. Requirement disputes continue through cross-role review/SpecCreator, not diagnosis.

Requests, outbox messages, decisions, recovery instructions and dispatch receipts persist with the original plan/cursor. Pause/restart after a saved decision reuses it; a dispatched recovery without a completion checkpoint blocks rather than replaying. The owner executes only the original step with the saved correction, after which normal scope validation, step checks, commit, item, integration and final gates still apply. Failed recovery gates preserve the diagnosis and cursor. Identical failure on unchanged source blocks immediately; at most two diagnosis rounds per work item are allowed. Existing blocked runs are not automatically resumed or migrated into recovery.

`SDLCAgent.Tests.exe --diagnosis` covers deterministic corrections, unfamiliar-error diagnosis, uncertain writes, policy denial, stale replies/source, retry limits, interrupted dispatch, failed gates and pause/restart. `--copilot-diagnosis-smoke` diagnoses a real isolated file-not-found tool failure through the production read-only Copilot contract without touching a product run.

Missing measurements scoped entirely to existing tester-owned paths can trigger bounded postimplementation verification preparation. Only the tester plan is renewed; completed implementation is not replayed. Each probe must edit, build, observe failure, restore exact original assertions and rebuild/retest green within one writable step. Worker commits and orchestrator auto-commits are disabled for that phase, and any remaining source/HEAD change blocks without cleanup. `sdlc_read_diff` with `workingTreeOnly=true` distinguishes uncommitted edits from earlier committed task work. Result-only corrections receive a scoped observed-evidence index, distinguishing negative receipts from restored passing completion evidence. A fully observed, restored probe with a lost result can be reconciled only to read-only completion, preserving failure history and recovery limits; uncertain transport, missing command proof or source changes cannot authorize replay. Unknown, foreign, unscoped, or repeatedly missing evidence remains blocked. Fresh provenance includes integrity-checked historical failing command receipts for process requirements; those never replace passing final-revision gates.

## Controls and audit viewer

```powershell
& $agent pause RUN_ID
& $agent status RUN_ID
& $agent resume RUN_ID
& $agent events RUN_ID
& $agent events RUN_ID --task TASK_ID --before EVENT_ID
& $agent artifact ARTIFACT_ID
& $agent stop
```

Pause is graceful: an in-flight operation may finish and checkpoint before the run becomes `Paused`. Stop ends the host; restart with the same data directory reconstructs unfinished assignments and reconciles integration receipts. Recovery does not resume a process/model call mid-instruction; the last incomplete saved step can be retried. Helpers should therefore be repeatable. A run becomes `Completed` only after every approved criterion is verified on the final integrated revision.

The localhost-only HTML UI is ordered as a live status dashboard, collapsible structured logs, implementation/test plans with branch history, and the exact approved specification. The dashboard shows blockers with links to their audit records, integrated/total items, running pairs and assignment start times, unscheduled backlog, test evidence, final-goal status, and the orchestrator's branch/commit counts. The sidebar status updates along with the dashboard. Review/feedback/approval and pause/resume controls remain available.

Test counts describe the **latest recognized test suite**, not a sum of repeated executions or parallel branch results. The implemented count is that suite's discovered total. Output with no recognized summary, inconsistent counts, or truncation is explicitly unknown; a successful command alone does not establish a test count. The planned total is shown only when the approved specification explicitly records `plannedTestCount`. Existing approved specifications are not rewritten to invent a target. Goal status remains unmet until final integrated-revision verification completes.

Expand the log panel for the latest 25 events, grouped by work item, assignment/role, and plan step. **Expand complete structured history** loads the remaining records in 200-event pages; loading can be stopped, and **Latest events** returns to live updates. Blocker links can open records older than the recent page. All saved worker plans are available (not limited to the generic artifact listing), with active plans first and the rest ordered by completion/last-activity time. Plan content loads on expansion. Branch history shows actual Git commits with audited worker roles when available, paginated in groups of 50; use **Refresh commits** to refresh an expanded history. Logs/specification/code text are rendered as text, never executable HTML. All API access, including interaction controls, requires the per-host bearer token. No desktop notification or automatic browser popup is provided; keep the run page open for live review prompts.

After draining, `cancel RUN_ID` can abandon a paused, blocked, or awaiting-approval run and release its branch for a replacement. It **does not undo already integrated commits** or delete audit evidence/worktrees. Blocked runs retain branch ownership until resumed or canceled.

## Configuration and permissions

`appsettings.json` controls the host and execution limits. Pass `--config FILE`, `--policy FILE`, and/or `--data DIR` to `start`/`serve`; client commands use the same `--data DIR`. Paths may be absolute. Settings take effect on host startup. The policy is snapshotted when a run is created; editing a policy file does not silently expand an existing run's authority.

| Setting | Default / behavior |
| --- | --- |
| Storage | `%LOCALAPPDATA%\SDLCAgent`: SQLite ledger, immutable hashed JSON artifacts, worktrees, host descriptor/log |
| Host | `127.0.0.1:47831`, one host per data directory |
| Parallelism | Two implementer-tester pairs globally; one serialized integration lane |
| Retries/repair | Three transient/read-only plan or result-format retries; up to three preparation replans; eight actual implementation/goal repair attempts before explicit blocking |
| Timeouts | 600 seconds per command; 900 seconds per agent operation |
| Run budgets | No default active-hour/token cap; configurable soft limits pause at durable boundaries |
| Token accounting | Provider-reported tokens only; a configured token budget pauses if usage is unavailable |
| Artifacts | 1 GiB configurable quota; exhaustion pauses instead of dropping evidence |
| Retention | Raw host diagnostic logs rotate daily and expire after 30 days when logging; essential audit history is retained |
| Copilot | SDK-matched runtime explicitly selected; optional `Copilot.CliPath` must be an absolute compatible runtime path; model configurable globally or per request |

`execution-policy.json` defaults to deny unclassified operations, elevation, machine changes, global installs, remote pushes/force-pushes, other-branch merges, deployment, and worker policy edits. Only GitHub Copilot is an authorized model service. Workers receive application-defined read/list/diff/write/delete/commit/named-check tools; built-in shell, network, subagent, skill, host-Git, and repository configuration discovery are disabled. Direct file tools reject traversal, Git metadata access, and symlinks/junctions. Deletion is limited to individual assigned files, never recursive directories. Implementation/test write scopes are disjoint, and committed as well as uncommitted changes are checked against the current role's scope. Project dependency manifest edits/deletions require content-bound audited justification.

The supported policy schema is deliberately closed. Safety invariants cannot be disabled, and turning an unused allow flag on does not synthesize a corresponding tool. Native checks are trusted operator-authorized executables, not a way to enforce operating-system file/network restrictions. Redaction is best-effort defense in depth, not permission to supply secrets or a substitute for OS isolation. Protect the data directory with normal user-profile filesystem permissions.

## Integration and failure semantics

- Each work item owns one `sdlc/RUN_ID/TASK_ID` branch and one worktree at `workspaces\RUN_ID\TASK_ID` under the data directory. Its implementer/tester pair shares that worktree sequentially; retries and host recovery reuse it, while other items use separate worktrees. Reuse verifies the saved path, branch, and owning Git repository. Workers commit coherent plan-step changes; the orchestrator preserves those commits in candidate merges. No push or deployment happens automatically.
- Candidate and final verification worktrees use compact names directly under `workspaces` so gate builds do not add run/task nesting that breaks legacy XAML compiler path limits. Candidates remain fresh and isolated; final names are pinned to the run and full source revision. Existing worker worktrees and prior evidence are retained unchanged. `SDLCAgent.Tests.exe --workspace-gates` validates compact allocation, revision isolation, gated rejection and atomic branch advancement.
- Flaky-gate rejection compares the source tree, exact authorized command and normalized workspace layout. Fresh random candidate IDs do not change that context. A real layout correction requires fresh passing gates, retains and audits the earlier failure, and does not imply source behavior passed. Legacy failures are lazily bound to the command/layout in their integrity-checked receipts; they are not discarded on upgrade.
- A worker handoff requires a clean tracked/untracked working tree. Dirty retries or handoffs produce an explicit `WorkspaceBlocked` event **before the next worker starts**, preserving plans, step cursors, and all files. The orchestrator does not automatically discard, stash, or commit a failed worker's edits under another role. Reconcile those edits under their original permissions before resuming. Worktree isolation does not expand assigned file scopes or fix a plan that requests unauthorized paths.
- All configured gates run against the **combined candidate**, not just the task tip. The target ref advances atomically only from the tested baseline; moved heads invalidate/retest the candidate. A persisted integration receipt recovers a branch update that happened just before a host crash.
- Gate failures are never waived. Failure followed by success on the identical source tree is explicitly classified as unstable, not accepted as a healthy retry. Repair the flaky check/environment; cancel the blocked run and create a replacement after remediation. TDD's expected pre-implementation failures are separate from immutable integration/baseline/final gates.
- Normal task/test/integration failures feed back into saved repair plans. Policy failures, unresolved baseline problems, exhausted budgets/repairs, and conflicts requiring changes outside assigned scopes block or pause with evidence. Stop the host before offline database backup/maintenance; retain artifacts with the database.
- The worker boundary returns validated typed results to the orchestrator, not concatenated model transcripts. The Copilot adapter keeps preceding assistant progress separate from the terminal assistant result; it never substitutes tool output or an earlier JSON-looking message. Both progress and rejected results remain auditable. If the final result needs correction, a schema-guided, **tool-free result-only call** repairs its representation without replaying implementation/checks. Exhausted protocol repairs block the current task while preserving its saved plans, stage, and cursor, rather than consuming the implementation-repair loop. Transport failure after writes/checks may have started likewise blocks without an automatic replay. Manual resume can retry the preserved incomplete step after operator remediation; it does not undo prior side effects.

## First-version boundaries

Local Windows workers only; no multi-machine dispatch, service installation, boot/login auto-start, remote publishing, deployment, plugin auto-installation, or hostile-code sandbox. Worktrees and task branches are retained for inspection/recovery; automatic workspace cleanup is deferred. The artifact quota does not include SQLite files, worktrees, build outputs, or package/runtime caches: monitor total disk usage. Merge conflicts involving externally changed/shared files can require operator remediation. Automated criteria still depend on the quality of the approved tests and independent model review; a green command is not proof of unspecified behavior.

Core components: `Orchestrator.cs` (durable scheduler), `CrossRoleReviews.cs` (persisted disputes, adjudication, scoped repairs and independent review), `FailureDiagnosis.cs` (read-only diagnosis and bounded owner-directed recovery), `Agents.cs` (MAF/Copilot adapter and scoped tools), `Store.cs` (ledger/artifacts), `GitWorkspace.cs` (branch ownership/candidates), `Processes.cs` (checks/evidence/process lifetime), `RunControls.cs` (approval/lifecycle), and `Program.cs`/`wwwroot` (CLI/host/viewer).
