using System.Collections.Concurrent;
using System.Diagnostics;

namespace SDLCAgent;

public sealed class PauseBoundaryException : Exception;

public sealed class Orchestrator : BackgroundService
{
    private readonly Store store;
    private readonly GitWorkspace git;
    private readonly ProcessRunner processes;
    private readonly CheckRunner checks;
    private readonly AgentRunner agents;
    private readonly AgentOptions options;
    private readonly ILogger<Orchestrator> logger;
    private readonly SemaphoreSlim pairs;
    private readonly SemaphoreSlim integration = new(1);
    private readonly ConcurrentDictionary<string, Task> running = new();

    public Orchestrator(Store store, GitWorkspace git, ProcessRunner processes, CheckRunner checks,
        AgentRunner agents, AgentOptions options, ILogger<Orchestrator> logger)
    {
        this.store = store; this.git = git; this.processes = processes;
        this.checks = checks; this.agents = agents; this.options = options; this.logger = logger;
        pairs = new SemaphoreSlim(options.Execution.MaxPairs);
    }

    public void Recover()
    {
        foreach (var id in store.ActiveRunIds())
            store.Change(id, "HostRecovered", "Previous host stopped. Preserve plan cursors, revoke old assignments, and reconcile Git receipts.", run =>
            {
                foreach (var task in run.Tasks.Where(t => t.State is "Running" or "Integrating"))
                {
                    task.State = task.CandidatePassed ? "Ready" : "Pending";
                    task.AssignmentId = null;
                }
                if (run.State == "PauseRequested")
                {
                    run.State = "Paused";
                    run.PreviousState ??= "Running";
                }
            });
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        Recover();
        try
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                foreach (var pair in running.Where(p => p.Value.IsCompleted).ToArray())
                {
                    await pair.Value;
                    running.TryRemove(pair.Key, out _);
                }
                foreach (var id in store.ActiveRunIds())
                    if (!running.ContainsKey(id)) running[id] = Run(id, stoppingToken);
                await Task.Delay(500, stoppingToken);
            }
        }
        catch (OperationCanceledException) when (stoppingToken.IsCancellationRequested) { }
        finally
        {
            await Task.WhenAll(running.Values);
        }
    }

    public async Task Run(string id, CancellationToken token)
    {
        var clock = Stopwatch.StartNew();
        var active = new Dictionary<string, Task>();
        var initialSeconds = store.Get(id).ActiveSeconds;
        using var checkpointCancellation = CancellationTokenSource.CreateLinkedTokenSource(token);
        var clockCheckpoint = CheckpointClock();
        try
        {
            store.Change(id, "RunStarted", "Orchestrator executing persisted phase.", r =>
            {
                if (r.State == "Queued") r.State = "Running";
            });
            while (true)
            {
                token.ThrowIfCancellationRequested();
                var run = store.Get(id);
                foreach (var task in active.Where(p => p.Value.IsCompleted).ToArray())
                {
                    await task.Value;
                    active.Remove(task.Key);
                }
                if (run.State == "Running" && ((options.Execution.MaxRunHours is { } hours
                    && initialSeconds + clock.Elapsed.TotalSeconds >= hours * 3600)
                    || (options.Execution.MaxReportedTokens is { } cap && run.ReportedTokens >= cap)))
                    store.Change(id, "RunLimitReached", "Configured time/token limit reached; draining to a durable pause.", r =>
                    {
                        r.State = "PauseRequested"; r.PreviousState = "Running";
                        r.Failure = "Time/token limit reached. Increase the configured limit before resuming.";
                    });
                run = store.Get(id);
                if (run.State == "PauseRequested")
                {
                    if (active.Count == 0)
                    {
                        store.Change(id, "RunPaused", "No active operations remain; run is durably paused.", r =>
                        {
                            r.State = "Paused"; r.PreviousState ??= "Running";
                        });
                        return;
                    }
                }
                else if (run.State != "Running")
                {
                    if (active.Count == 0) return;
                }
                else if (run.PendingUnblockId is not null)
                {
                    await AssessUnblock(run, token);
                }
                else if (run.Phase == "Specification")
                {
                    await DevelopSpecification(run, token);
                }
                else if (run.Phase == "Planning")
                {
                    await Decompose(run, token);
                }
                else if (run.Phase == "Execution")
                {
                    var dependencyRepair = run.Tasks.FirstOrDefault(t => t.DependencyRepairId is not null && t.State is "Pending" or "Blocked");
                    if (active.Count == 0 && dependencyRepair is not null)
                    {
                        RequireApproval(run);
                        await new DependencyRepairs(store, git, processes, checks, agents).Run(run, dependencyRepair, token);
                        continue;
                    }
                    foreach (var task in run.Tasks.Where(t => t.State == "Pending"))
                    {
                        if (!task.Definition.Dependencies.All(dep => run.Tasks.Any(t => t.Definition.Id == dep && t.State == "Integrated"))) continue;
                        if (!await pairs.WaitAsync(0, token)) break;
                        var taskId = task.Definition.Id;
                        active[taskId] = ExecutePair(id, taskId, token);
                    }
                    foreach (var task in run.Tasks.Where(t => t.State == "Ready"))
                        if (!active.ContainsKey(task.Definition.Id)) active[task.Definition.Id] = Integrate(id, task.Definition.Id, token);
                    if (active.Count == 0 && run.Tasks.Count > 0)
                    {
                        if (run.Tasks.All(t => t.State == "Integrated")) await VerifyGoal(run, token);
                        else if (run.Tasks.Any(t => t.State == "Blocked"))
                            throw new WorkflowException("Blocked work packages: " + string.Join("\n", run.Tasks.Where(t => t.State == "Blocked").Select(t => t.Definition.Id + ": " + t.Failure)));
                        else if (!run.Tasks.Any(t => t.State == "Pending" && t.Definition.Dependencies.All(d => run.Tasks.Any(x => x.Definition.Id == d && x.State == "Integrated"))))
                            throw new WorkflowException("No schedulable work remains, but the approved goal has not been verified.");
                    }
                }
                await Task.Delay(200, token);
            }
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
            store.Event(id, "HostStopping", "Host stopping; unfinished assignments will be reconciled on restart.");
        }
        catch (BudgetException ex)
        {
            store.Change(id, "RunLimitReached", ex.Message, r =>
            {
                r.State = active.Count > 0 ? "PauseRequested" : "Paused";
                r.PreviousState = "Running"; r.Failure = ex.Message;
            });
        }
        catch (Exception ex) when (ex is not OutOfMemoryException)
        {
            logger.LogError(ex, "Run {RunId} blocked", id);
            store.Change(id, "RunBlocked", ex.Message, r =>
            {
                r.Failure = Redaction.Apply(r.PendingUnblockId is null ? ex.Message : r.Failure + "\nUnblock assessment failed: " + ex.Message);
                r.PendingUnblockId = null; r.State = "Blocked";
            });
        }
        finally
        {
            await Task.WhenAll(active.Values);
            await checkpointCancellation.CancelAsync();
            await clockCheckpoint;
            store.Change(id, "RunCheckpoint", "Persisted orchestration checkpoint.", r => r.ActiveSeconds = initialSeconds + clock.Elapsed.TotalSeconds);
            var run = store.Get(id);
            if (run.State == "PauseRequested")
                store.Change(id, "RunPaused", "Active work drained at checkpoint.", r => { r.State = "Paused"; r.PreviousState ??= "Running"; });
        }

        async Task CheckpointClock()
        {
            using var timer = new PeriodicTimer(TimeSpan.FromSeconds(5));
            try
            {
                while (await timer.WaitForNextTickAsync(checkpointCancellation.Token))
                    store.Change(id, "RunClockCheckpoint", "Checkpointed active execution time for live status and crash recovery.",
                        r => r.ActiveSeconds = initialSeconds + clock.Elapsed.TotalSeconds);
            }
            catch (OperationCanceledException) when (checkpointCancellation.IsCancellationRequested) { }
        }
    }

    private void Boundary(string id)
    {
        var state = store.Get(id).State;
        if (state == "PauseRequested") throw new PauseBoundaryException();
        if (state != "Running") throw new PolicyException("Run is no longer executing.");
    }

    internal async Task AssessUnblock(RunData run, CancellationToken token)
    {
        var requestId = run.PendingUnblockId!;
        var request = Json.Read<UnblockRequest>(store.ReadArtifact(requestId));
        var workspace = run.Tasks.FirstOrDefault(t => t.State == "Blocked" && t.Workspace is not null)?.Workspace
            ?? run.InspectionWorkspace ?? run.Request.Repository;
        var before = await checks.Snapshot(workspace, token);
        var decision = await agents.Invoke<UnblockDecision>(new(run.Id, null, null, "Orchestrator", "assess-unblock", workspace,
            $$"""
            Assess human instructions for this blocked run READ-ONLY. Do not execute a repair here.
            Request ID: {{requestId}}
            Saved blocker, instructions, approval and original task state: {{Json.Write(request)}}
            Existing execution policy (authority): {{run.PolicyJson}}
            Current phase: {{run.Phase}}. Source snapshot: {{Json.Write(new { before.Revision, before.Fingerprint })}}
            Inspect relevant repository evidence using supplied read/list/diff tools. Distinguish proposed repairs from already completed remediation.
            Return requestId exactly, resume, explanation, workerInstruction. Resume means the existing durable phase and blocked tasks
            can continue using their ORIGINAL plans, scopes, gates and recovery guards with the specific workerInstruction as guidance.
            It does not certify success. Never grant permissions, waive gates, approve/change requirements, reset diagnosis/review guards,
            discard dirty edits, or replay uncertain dispatched execution. A proposal requiring any of those, a new decomposition,
            or missing capabilities must remain blocked with an actionable explanation. Specification changes require normal feedback and fresh approval.
            No approval exists unless recorded above; resuming Specification cannot authorize implementation.
            If safe, translate human instructions into concrete scoped worker guidance. Otherwise resume=false and explain required operator action.
            """, [], UnblockRequestId: requestId), token);
        var after = await checks.Snapshot(workspace, token);
        if (before != after) throw new WorkerContractException("Source changed during unblock assessment; stale guidance cannot resume execution.");
        var artifact = store.Save(run.Id, null, "OrchestratorUnblockDecision", decision);
        store.Change(run.Id, decision.Resume ? "OperatorUnblockAccepted" : "OperatorUnblockDeclined", decision.Explanation, r =>
        {
            if (r.PendingUnblockId != requestId || r.Phase != request.Phase || r.SpecificationId != request.SpecificationId
                || r.ApprovedSpecificationId != request.ApprovedSpecificationId || r.ApprovedSpecificationHash != request.ApprovedSpecificationHash
                || Json.Write(r.Tasks) != Json.Write(request.Tasks))
                throw new WorkerContractException("Unblock request no longer matches the current specification, task plans or cursors.");
            if (r.State == "PauseRequested") return;
            if (r.State != "Running") throw new WorkerContractException("Run stopped during unblock assessment.");
            r.PendingUnblockId = null;
            if (!decision.Resume)
            {
                r.State = "Blocked"; r.Failure = request.Failure + "\nOrchestrator: " + decision.Explanation;
                return;
            }
            r.OperatorInstructionId = artifact.Id;
            r.State = r.PreviousState == "AwaitingApproval" ? "AwaitingApproval" : "Running";
            r.PreviousState = null; r.Failure = null;
            foreach (var task in r.Tasks.Where(t => t.State == "Blocked")) { task.State = "Pending"; task.Attempts = 0; }
        }, documentId: artifact.Id);
    }

    private async Task DevelopSpecification(RunData run, CancellationToken token)
    {
        var workspace = await git.Inspection(run, token);
        if (!run.BaselinePassed)
        {
            if (!await checks.All(run, null, workspace, run.Request.Checks.Select(c => c.Name), token))
                throw new WorkflowException("Baseline gates failed on the initial revision. Repair the baseline/check configuration before proceeding; no failing gate was waived.");
            if ((await GitWorkspace.Git(processes, workspace, token, "status", "--porcelain")).Length != 0)
                throw new WorkflowException("Baseline commands changed non-ignored files. Supply non-mutating gates or commit required generated artifacts first.");
            store.Change(run.Id, "BaselinePassed", "All configured initial gates passed.", r => r.BaselinePassed = true);
        }
        var previous = run.SpecificationId is null ? "None" : store.ReadArtifact(run.SpecificationId);
        var spec = await agents.Invoke<Specification>(new(run.Id, null, null, "SpecCreator", "specification", workspace,
            $$"""
            Inspect relevant repository code and existing tests before drafting. Evolve the problem into a fully implementable specification.
            Problem: {{run.Request.Problem}}
            Previous draft: {{previous}}
            Human feedback / unresolved execution finding: {{run.Feedback}}
            Each criterion must identify authorized check names, concrete pass/fail conditions, and measurement procedure.
            Ask necessary product questions in questions; no implementation may begin until those questions are resolved and a human approves.
            Return this JSON structure:
            {"title":"...","problem":"...","scope":"...","nonGoals":["..."],"repositoryFindings":"files and observations",
             "approach":"implementation approach, interfaces, edge cases and constraints",
             "criteria":[{"id":"C1","outcome":"observable outcome","measurement":"conditions, threshold and evidence","checks":["check-name"]}],"questions":[],
             "plannedTestCount":null}
            Set plannedTestCount only when a total test target is explicitly part of the specification; otherwise leave null.
            """, []), token);
        Contracts.ValidateSpec(spec, run.Request.Checks, forApproval: false);
        var artifact = store.Save(run.Id, null, "Specification", spec);
        store.Change(run.Id, "SpecificationReady", "Human feedback or approval of this exact specification version is required.", r =>
        {
            r.SpecificationId = artifact.Id; r.Failure = null;
            if (r.State == "PauseRequested") r.PreviousState = "AwaitingApproval";
            else r.State = "AwaitingApproval";
        }, documentId: artifact.Id);
    }

    private async Task Decompose(RunData run, CancellationToken token)
    {
        RequireApproval(run);
        var spec = Json.Read<Specification>(store.ReadArtifact(run.ApprovedSpecificationId!));
        var planningWorkspace = await git.FinalWorkspace(run, await git.BranchHead(run.Request.Repository, run.Request.Branch, token), token);
        var feedback = run.Feedback;
        for (var attempt = 0; ; attempt++)
        {
        var result = await agents.Invoke<Decomposition>(new(run.Id, null, null, "Orchestrator", "decomposition",
            planningWorkspace, $$"""
            Break this approved specification into bounded, independently implementable work packages with dependencies.
            Approved specification: {{Json.Write(spec)}}
            Replanning evidence: {{feedback}}
            Every criterion must be covered collectively. Each task must declare explicit obligations: id, criterionId, outcome, measurement, paths and checks.
            criterionIds are traceability links, NOT a demand that this task satisfy the entire parent criterion.
            Split shared criteria into independently measurable per-task duties. Assign only duties achievable within this task's owned paths and completed prerequisites.
            Never require a later dependent task's deliverable to finish an earlier task. Example: a fixture task's C16 obligation verifies the fixture;
            a later README task's C16 obligation verifies complete documentation. The fixture task must not require README content or a README-headings test yet.
            Together all obligations must cover the FULL approved outcomes AND measurement requirements (including negative checks); do not omit or weaken any part.
            Final integrated-goal verification still requires every complete parent criterion. Use exact repository-relative paths or directory scopes, not globs.
            A task's codePaths and testPaths must be disjoint. Changes to shared files require dependencies.
            Include every approved restore/build prerequisite in any task that runs test checks.
            Make production API/project scaffolding an implementer-owned part of the task's codePaths, or an explicit dependency.
            Do not assign a test-first item an impossible setup that requires its tester to create production types or modify shared configuration.
            Each task is owned by an implementer-tester pair. Use testFirst=true wherever feasible; otherwise supply nonTddReason.
            Return {"tasks":[{"id":"short-kebab-id","title":"...","objective":"...","criterionIds":["C1"],"dependencies":[],
            "codePaths":["src"],"testPaths":["tests"],"checks":["check-name"],"testFirst":true,"nonTddReason":null,
            "obligations":[{"id":"bounded-duty","criterionId":"C1","outcome":"this task's deliverable only","measurement":"exact evidence of this duty",
            "paths":["src"],"checks":["check-name"]}]}]}
            """, []), token);
        Contracts.ValidateTasks(result.Tasks, spec, run.Request.Checks);
        var ordered = new List<WorkDefinition>();
        while (ordered.Count != result.Tasks.Length)
        {
            var next = result.Tasks.First(t => !ordered.Any(x => x.Id == t.Id)
                && t.Dependencies.All(dep => ordered.Any(x => x.Id == dep)));
            if (SafePaths.Overlap(next.CodePaths, next.TestPaths))
                throw new InvalidDataException("Implementation and test write scopes must be disjoint.");
            var overlaps = ordered.Where(t => SafePaths.Overlap(t.CodePaths.Concat(t.TestPaths), next.CodePaths.Concat(next.TestPaths))).Select(t => t.Id);
            ordered.Add(next with { Dependencies = next.Dependencies.Concat(overlaps).Distinct().ToArray() });
        }
        var prefix = $"p{run.Replans}-";
        var definitions = ordered.Select(t => t with
        {
            Id = prefix + t.Id, Dependencies = t.Dependencies.Select(d => prefix + d).ToArray()
        }).ToArray();
        var proposal = store.Save(run.Id, null, "DecompositionProposal", definitions);
        var coverage = await agents.Invoke<CoverageAssessment>(new(run.Id, null, null, "SpecCreator", "verify-decomposition", planningWorkspace,
            "Independently inspect this proposed decomposition read-only against the approved specification. Return passed and explanation. "
            + "Passing requires the union of explicit obligations to cover every full criterion outcome AND measurement (including negative checks, UI evidence and documentation), without weakening or omitting any requirement. "
            + "Each obligation must be achievable within its task's owned paths and completed prerequisites. Reject implicit whole-criterion completion duties, downstream-dependent deliverables in upstream tasks, and duties with no capable owner. "
            + "Shared criterion IDs are traceability only: different tasks may satisfy different explicit slices. Fixture existence can belong upstream while README coverage belongs downstream. "
            + "Inspect relevant files if needed; code/worker claims are not authority to waive requirements. "
            + "Approved specification: " + Json.Write(spec) + "\nProposed tasks with scheduler-resolved dependencies: " + Json.Write(definitions), []), token);
        var assessment = store.Save(run.Id, null, "DecompositionCoverage", coverage);
        if (!coverage.Passed)
        {
            store.Event(run.Id, "DecompositionRejected", coverage.Explanation, documentId: assessment.Id);
            if (attempt >= options.Execution.TransientRetries)
                throw new WorkerContractException("Decomposition obligations cannot safely cover the approved goal after bounded read-only replanning: " + coverage.Explanation);
            feedback = "Independent decomposition review rejected proposal " + proposal.Id + ": " + coverage.Explanation;
            continue;
        }
        store.Event(run.Id, "DecompositionCoverageAccepted", "Independent review accepted collective coverage and task-local ownership; full final gates remain mandatory.", documentId: assessment.Id);
        var artifact = store.Save(run.Id, null, "WorkDecomposition", definitions);
        store.Change(run.Id, "WorkScheduled", "Criteria mapped to pairs; overlapping scopes serialized through dependencies.", r =>
        {
            r.Tasks.AddRange(definitions.Select(d => new TaskData { Definition = d }));
            r.Phase = "Execution";
        }, documentId: artifact.Id);
        return;
        }
    }

    private void RequireApproval(RunData run)
    {
        if (run.ApprovedSpecificationId is null || run.ApprovedSpecificationId != run.SpecificationId
            || store.GetArtifact(run.ApprovedSpecificationId).Sha256 != run.ApprovedSpecificationHash)
            throw new PolicyException("Current specification does not have a matching human approval.");
        _ = store.ReadArtifact(run.ApprovedSpecificationId);
    }

    private async Task ExecutePair(string runId, string taskId, CancellationToken token)
    {
        string? assignment = null;
        try
        {
            Boundary(runId);
            var run = store.Get(runId);
            RequireApproval(run);
            try { Contracts.ValidateObligations(Find(run, taskId).Definition); }
            catch (InvalidDataException ex) { throw new WorkerContractException("Task obligation contract requires explicit replanning before execution. " + ex.Message); }
            assignment = Guid.NewGuid().ToString("N");
            store.Change(runId, "PairAssigned", $"Implementer-tester pair assigned; attempt identity {assignment}.", r =>
            {
                var t = Find(r, taskId); t.State = "Running"; t.AssignmentId = assignment;
                if (t.Stage != "Review" && t.Diagnosis is null) t.Attempts++;
                t.Heartbeat = DateTimeOffset.UtcNow; t.StartedAt = DateTimeOffset.UtcNow; t.FinishedAt = null;
            }, taskId);
            var task = Find(store.Get(runId), taskId);
            using var assignmentScope = AuditScope.Use(new(assignment, Attempt: task.Attempts));
            var workspace = await git.TaskWorkspace(run, task, token);
            task = Find(store.Get(runId), taskId);
            if (task.NeedsSync)
            {
                if (task.Diagnosis is not null) throw new WorkerContractException("Pending diagnosis must be reconciled before synchronizing its source revision.");
                await git.SyncTask(run, task, token);
                task = Find(store.Get(runId), taskId);
            }
            var reviews = new CrossRoleReviews(store, git, processes, checks, agents);
            var diagnosis = new FailureDiagnosis(store, processes, checks, agents);
            if (task.Stage == "Review")
            {
                await reviews.Run(runId, taskId, assignment, () => Boundary(runId), token);
                task = Find(store.Get(runId), taskId);
            }
            var context = $"Approved specification: {store.ReadArtifact(run.ApprovedSpecificationId!)}\nAssignment: {Json.Write(task.Definition)}\nPrevious failure/review decision: {task.Failure}"
                + "\nExecute only this task's explicit obligations. Parent criterion IDs are traceability links, not whole-criterion completion duties. "
                + "Do not create tests, plan steps or blockers requiring downstream deliverables assigned to other tasks. Full criteria remain mandatory at the final integrated gate.";
            AgentCall Call(string role, string operation, string prompt, string[] scopes, bool commands = false) =>
                new(runId, taskId, assignment, role, operation, workspace.Path, context + "\n" + prompt, scopes, commands);

            if (task.Stage == "Plans")
            {
                Boundary(runId);
                var implementationId = task.ImplementationPlanId;
                var testId = task.TestPlanId;
                if (task.PlanRepairRole != "Tester" || implementationId is null)
                {
                    var implementation = await agents.Invoke<ImplementerPlan>(Call("SpecImplementer", "implementation-plan",
                        PlanPrompt("SpecImplementer", task.Definition, run.Request.Checks)
                        + (testId is null ? "" : "\nExisting test plan for interface alignment only: " + store.ReadArtifact(testId)), []), token);
                    implementationId = store.Save(runId, taskId, "ImplementationPlan", implementation).Id;
                }
                var savedImplementation = ReadImplementationPlan(implementationId!);
                if (task.PlanRepairRole != "SpecImplementer" || testId is null)
                {
                    var test = await agents.Invoke<TesterPlan>(Call("Tester", "test-plan",
                        PlanPrompt("Tester", task.Definition, run.Request.Checks, task.VerificationPreparation)
                        + "\nImplementation plan for interface alignment only: " + Json.Write(savedImplementation), []), token);
                    testId = store.Save(runId, taskId, "TestPlan", test).Id;
                }
                _ = ReadTestPlan(testId!);
                UpdateTask(runId, taskId, assignment, "PlansSaved", "Both detailed plans saved; automatically checked for required step contracts.", t =>
                {
                    t.ImplementationPlanId = implementationId; t.TestPlanId = testId;
                    t.Stage = t.RepairStage ?? "Scaffold"; t.StepIndex = 0; t.PlanRepairRole = null; t.RepairStage = null;
                }, testId);
            }
            task = Find(store.Get(runId), taskId);
            if (task.Stage == "Scaffold")
            {
                var plan = ReadImplementationPlan(task.ImplementationPlanId!);
                await ExecuteSteps(plan.ScaffoldingSteps, WorkerStage.Scaffold, "scaffold",
                    "Establish only the compiling API/types/project plumbing needed by this work item's tests. Align with the saved test plan. Do not complete the behavior under test or edit test files. If APIs already exist, inspect them and report why no scaffolding edit is needed.\nSaved test plan: "
                    + store.ReadArtifact(task.TestPlanId!) + "\nThat plan is reference-only interface context, not assigned steps to execute.");
                Boundary(runId);
                var prerequisites = await checks.Batch(run, taskId, workspace.Path,
                    task.Definition.Checks.Where(name => run.Request.Checks.Single(c => c.Name == name).Kind is "restore" or "build"), token);
                if (prerequisites.Any(e => !CheckRunner.Passed(e.Result)))
                    throw new PreparationException("Implementer scaffolding did not satisfy compilation/setup prerequisites: " + Json.Write(prerequisites));
                UpdateTask(runId, taskId, assignment, "ScaffoldingComplete", "Implementer-owned scaffolding is ready; tester preparation follows.", t => { t.Stage = "Tests"; t.StepIndex = 0; });
            }
            task = Find(store.Get(runId), taskId);
            if (task.Stage is "Tests" or "VerificationPreparation")
            {
                var plan = ReadTestPlan(task.TestPlanId!);
                await ExecuteSteps(plan.PreparationSteps, task.VerificationPreparation ? WorkerStage.VerificationPreparation : WorkerStage.Tests, "prepare-tests",
                    task.VerificationPreparation
                        ? "Collect only the missing assigned verification evidence on already implemented code. Use existing tester-owned paths/checks. For a required negative probe, save the exact original test content, temporarily break one assertion, build and observe its failing test, then restore the exact content and rebuild/retest green. Do not commit a broken assertion, weaken coverage, edit production or replay implementation. Cite the actual failing and restored passing receipts."
                        : "Create acceptance/regression tests against the established API. Do not edit or create production types, even as temporary stubs. Test only this work item's behavior, not future dependency stages. This phase prepares tests; final verification follows implementation.");
                UpdateTask(runId, taskId, assignment, "TestPreparationComplete", "Test plan preparation steps complete.", t =>
                { t.Stage = t.VerificationPreparation ? "Verify" : "Red"; t.StepIndex = 0; });
            }
            task = Find(store.Get(runId), taskId);
            if (task.Stage == "Red")
            {
                Boundary(runId);
                if (task.Definition.TestFirst)
                {
                    var results = await checks.Batch(run, taskId, workspace.Path, task.Definition.Checks, token);
                    if (results.Any(e => e.Kind != "test" && !CheckRunner.Passed(e.Result)))
                        throw new PreparationException("TDD prerequisites failed; tests were not run against stale binaries. Correct the implementer-owned scaffolding/test setup: " + Json.Write(results));
                    var assessment = await agents.Invoke<RedAssessment>(Call("Tester", "assess-red",
                        "Check this evidence. MissingBehavior may be true only if tests fail for the expected absent behavior, not compilation, infrastructure, or test setup.\n"
                        + Json.Write(results) + "\nReturn {\"missingBehavior\":true,\"explanation\":\"specific failing assertion and expected reason\"}.", []), token);
                    var failed = results.Any(e => !CheckRunner.Passed(e.Result));
                    if (failed && (!assessment.MissingBehavior || results.Any(e => e.Result.TimedOut || e.Result.OutputTruncated)))
                        throw new PreparationException("Red phase is not a meaningful behavior failure; repair setup under the correct role: " + assessment.Explanation);
                    store.Event(runId, failed ? "TddRedConfirmed" : "TddException",
                        failed ? assessment.Explanation : "Tests already pass before implementation; test-first red not applicable. " + assessment.Explanation, taskId);
                }
                else store.Event(runId, "TddException", task.Definition.NonTddReason!, taskId);
                UpdateTask(runId, taskId, assignment, "ImplementationReady", "Proceed to implementation using saved plan.", t => t.Stage = "Implementation");
            }
            task = Find(store.Get(runId), taskId);
            if (task.Stage == "Implementation")
            {
                var plan = ReadImplementationPlan(task.ImplementationPlanId!);
                await ExecuteSteps(plan.ImplementationSteps, WorkerStage.Implementation, "implement",
                    "Implement the current step. Keep tests intact; run authorized checks. Commit logical units, then refactor while retaining green tests.");
                UpdateTask(runId, taskId, assignment, "ImplementationComplete", "Implementation steps completed; independent verification follows.", t => { t.Stage = "Verify"; t.StepIndex = 0; });
            }
            Boundary(runId);
            run = store.Get(runId);
            task = Find(run, taskId);
            var testPlan = ReadTestPlan(task.TestPlanId!);
            await ExecuteSteps(testPlan.VerificationSteps.Select(s => s.AsStep()).ToArray(), WorkerStage.Verify, "verify-step",
                "Independently execute this verification step on the implemented revision. Do not edit code or tests. Report unmet current-step/task-obligation conditions, not unassigned parent-criterion duties.");
            var revision = await GitWorkspace.Head(processes, workspace.Path, token);
            var verificationContext = new TaskVerificationContext(Guid.NewGuid().ToString("N"), assignment!, taskId,
                run.ApprovedSpecificationId!, run.ApprovedSpecificationHash!, revision);
            var verificationMessage = store.Save(runId, taskId, "TaskVerificationMessage", verificationContext);
            UpdateTask(runId, taskId, assignment, "TaskVerificationQueued", "Assess only this task's explicit obligations; full parent criteria belong to the final integrated gate.",
                t => t.PendingVerification = verificationContext, verificationMessage.Id);
            using var verificationScope = AuditScope.Use(new(assignment, "Tester", verificationContext.MessageId, "TaskVerification", task.Attempts));
            var evidence = await checks.Batch(run, taskId, workspace.Path, task.Definition.Checks, token);
            if (evidence.Any(e => !CheckRunner.Passed(e.Result))) throw new WorkflowException("Task verification checks failed. " + Json.Write(evidence));
            var provenance = await git.TaskProvenance(run, task, revision, token);
            var provenanceArtifact = store.Save(runId, taskId, "RepositoryEvidence", new
            { workspace = workspace.Path, kind = "TaskProvenance", path = ".", observation = provenance });
            store.Event(runId, "TaskProvenanceObserved", "Pinned worktree ownership, commit ancestry and staged execution trace.", taskId, provenanceArtifact.Id);
            TaskVerificationResult verification;
            try
            {
                verification = await agents.Invoke<TaskVerificationResult>(Call("Tester", "verify-task",
                    TaskVerificationPrompt(verificationContext, task.Definition, store.ReadArtifact(task.TestPlanId!), evidence,
                        Json.Write(new { evidenceId = provenanceArtifact.Id, observation = provenance })), [])
                    with { ChangeBase = revision, Verification = verificationContext }, token);
            }
            catch (WorkerProtocolException ex)
            { throw new WorkerContractException("Tester task-assessment contract/evidence is incomplete; preserve plans/cursor and do not repair production code for missing verification. " + ex.Message); }
            if (verification.Obligations.Any(o => o.Status == ObligationStatus.NotVerified))
            {
                var missing = verification.Obligations.Where(o => o.Status == ObligationStatus.NotVerified).ToArray();
                if (task.Diagnosis is null && task.Review is null
                    && missing.All(o => o.EvidenceIds.Length != 0 && task.Definition.Obligations!.Single(d => d.Id == o.Id).Paths
                        .All(p => SafePaths.InScope(p, task.Definition.TestPaths))))
                {
                    await git.RequireCleanHandoff(workspace.Path, token);
                    if (await GitWorkspace.Head(processes, workspace.Path, token) != revision)
                        throw new WorkerContractException("Missing-verification repair became stale.");
                    var missingEvidence = store.Save(runId, taskId, "MissingVerificationEvidence", verification);
                    UpdateTask(runId, taskId, assignment, "VerificationPreparationRequested",
                        "Collect missing tester-owned evidence without replaying completed implementation; original obligations/gates remain.",
                        t => { t.VerificationPreparation = true; t.PendingVerification = null; }, missingEvidence.Id);
                    await Repair("Collect ONLY missing assigned measurements on the implemented revision. Preserve completed source and assertions. "
                        + "Plan tester preparation for the missing probes and independent verification afterwards; do not recreate existing tests or repeat implementation. "
                        + Json.Write(missing), "Tester", "VerificationPreparation", true, missingEvidence.Id);
                    return;
                }
                throw new WorkerContractException("Task obligations need verifier evidence or decomposition correction, not implementation replay: " + Json.Write(verification));
            }
            if (!verification.Passed)
                throw new WorkflowException("Independent verification found an explicitly assessed unmet task obligation: " + Json.Write(verification));
            if (await GitWorkspace.Head(processes, workspace.Path, token) != revision
                || (await GitWorkspace.Git(processes, workspace.Path, token, "status", "--porcelain")).Length != 0)
                throw new WorkflowException("Verification became stale or the workspace has uncommitted changes.");
            var verified = store.Save(runId, taskId, "TaskVerification", new { revision, verification, evidence });
            UpdateTask(runId, taskId, assignment, "PairVerified", "Pair verified its exact revision; awaiting orchestrator-owned integration gates.", t =>
            {
                t.HeadCommit = revision; t.State = "Ready"; t.Failure = null; t.FinishedAt = DateTimeOffset.UtcNow; t.PendingVerification = null; t.VerificationPreparation = false;
            }, verified.Id);

            ImplementerPlan ReadImplementationPlan(string id)
            {
                try
                {
                    var plan = Json.Read<ImplementerPlan>(store.ReadArtifact(id));
                    Contracts.ValidatePlan(plan, "SpecImplementer", task.Definition, run.Request.Checks);
                    return plan;
                }
                catch (Exception ex) when (ex is PlanContractException or InvalidDataException or System.Text.Json.JsonException)
                { throw new WorkerContractException("Saved worker plan cannot be executed; regenerate role-scoped plans before resuming: " + ex.Message); }
            }

            TesterPlan ReadTestPlan(string id)
            {
                try
                {
                    var plan = Json.Read<TesterPlan>(store.ReadArtifact(id));
                    Contracts.ValidatePlan(plan, "Tester", task.Definition, run.Request.Checks, task.VerificationPreparation);
                    return plan;
                }
                catch (Exception ex) when (ex is PlanContractException or InvalidDataException or System.Text.Json.JsonException)
                {
                    if (task.VerificationPreparation) throw new PreparationException("Saved verification preparation plan must be renewed without implementation replay: " + ex.Message);
                    throw new WorkerContractException("Saved worker plan cannot be executed; regenerate role-scoped plans before resuming: " + ex.Message);
                }
            }

            async Task ExecuteSteps(PlanStep[] steps, WorkerStage stage, string operation, string instructions)
            {
                var role = Contracts.RoleFor(stage);
                var start = Find(store.Get(runId), taskId).StepIndex;
                for (var i = start; i < steps.Length; i++)
                {
                    Boundary(runId);
                    var recovery = Find(store.Get(runId), taskId).Diagnosis;
                    var recoveryInstruction = recovery is null ? "" : await diagnosis.Instruction(runId, taskId, assignment!, token);
                    if (recovery is not null) recovery = Find(store.Get(runId), taskId).Diagnosis;
                    Boundary(runId);
                    await git.RequireCleanHandoff(workspace.Path, token);
                    var step = steps[i];
                    using var stepScope = AuditScope.Use(AuditScope.Current with
                    {
                        Role = role, StepId = step.Id,
                        PlanId = role == "Tester" ? Find(store.Get(runId), taskId).TestPlanId : Find(store.Get(runId), taskId).ImplementationPlanId
                    });
                    store.Event(runId, "PlanStepStarted", $"{role} {step.Id}: {step.Action}", taskId,
                        role == "Tester" ? Find(store.Get(runId), taskId).TestPlanId : Find(store.Get(runId), taskId).ImplementationPlanId);
                    var execution = new StepAssignment(assignment!, AuditScope.Current.PlanId!, role, stage, step,
                        new(recovery?.VerificationOnly == true ? [] : step.WritePaths, Contracts.CheckNamesFor(stage, task.Definition, run.Request.Checks), recovery?.VerificationOnly != true && step.WritePaths.Length != 0 && stage != WorkerStage.VerificationPreparation));
                    (string Revision, string Fingerprint)? probeBaseline = stage == WorkerStage.VerificationPreparation ? await checks.Snapshot(workspace.Path, token) : null;
                    var call = Call(role, operation, StepPrompt(execution, instructions + "\n" + recoveryInstruction), execution.Capabilities.WritePaths, commands: true)
                        with { ChangeBase = await GitWorkspace.Head(processes, workspace.Path, token), Execution = execution, RecoveryVerificationOnly = recovery?.VerificationOnly == true };
                    if (recovery is not null) diagnosis.Dispatch(runId, taskId, assignment!);
                    StepResult outcome;
                    try { outcome = await agents.Invoke<StepResult>(call, token); }
                    catch (WorkerProtocolException ex) when (recovery is null)
                    {
                        await diagnosis.Open(call, ex.Message, token, executionUncertain: true);
                        throw new DiagnosisPendingException();
                    }
                    var failures = diagnosis.Failures(call, out _);
                    if (failures.Any(f => f.SideEffects != ToolSideEffects.None || f.PolicyDenied)
                        || outcome.Status != StepStatus.Completed && failures.Length != 0
                        || outcome.Status != StepStatus.Completed && outcome.Blocker?.Kind is BlockerKind.Infrastructure or BlockerKind.CapabilityMismatch)
                    {
                        await diagnosis.Open(call, outcome.Summary, token);
                        throw new DiagnosisPendingException();
                    }
                    if (outcome.Status == StepStatus.ReviewRequested || outcome.Blocker?.Kind == BlockerKind.SpecificationGap)
                    {
                        if (recovery is not null) throw new WorkerContractException("Recovery cannot open nested disputes. Preserve diagnosis and escalate for explicit routing.");
                        await new WorkerTools(store, processes, checks, git, call, token).ValidateChanges();
                        await reviews.Open(runId, taskId, assignment!, execution, outcome, token);
                        throw new ReviewPendingException();
                    }
                    if (outcome.Status != StepStatus.Completed) throw new StepFailureException(outcome.Blocker!);
                    var tools = new WorkerTools(store, processes, checks, git, call, token);
                    await tools.ValidateChanges();
                    if (probeBaseline is not null && probeBaseline != await checks.Snapshot(workspace.Path, token))
                        throw new WorkerContractException("Verification preparation did not restore the exact starting source/HEAD. No probe changes were committed; preserve the worktree for reconciliation.");
                    var names = step.Checks.AsEnumerable();
                    if (step.Checks.Any(name => run.Request.Checks.Single(c => c.Name == name).Kind == "test"))
                        names = run.Request.Checks.Where(c => c.Kind is "restore" or "build").Select(c => c.Name).Concat(names);
                    var stepEvidence = await checks.Batch(run, taskId, workspace.Path, names, token);
                    if (stepEvidence.Any(e => !CheckRunner.Passed(e.Result)))
                        throw new StepFailureException(new(stage == WorkerStage.Scaffold ? BlockerKind.MissingScaffold : stage == WorkerStage.Tests ? BlockerKind.TestSetupFailure : BlockerKind.BehaviorFailure,
                            "Current-step completion checks failed: " + Json.Write(stepEvidence), null, stepEvidence[^1].Name));
                    if (stage != WorkerStage.VerificationPreparation)
                        await git.Commit(store.Get(runId), taskId, workspace.Path, $"{taskId}: {step.Action[..Math.Min(120, step.Action.Length)]}", token);
                    UpdateTask(runId, taskId, assignment, "PlanStepCompleted", $"{role} {step.Id}: {outcome.Summary}", t =>
                    {
                        t.StepIndex = i + 1;
                        t.Diagnosis = null;
                    });
                    if (recovery is not null)
                        store.Event(runId, "RecoveryVerified", "Recovery passed current-step scope/check/commit gates; original cursor advanced.", taskId, recovery.DecisionId);
                }
            }
        }
        catch (DiagnosisPendingException)
        {
            UpdateTask(runId, taskId, assignment, "DiagnosisAwaitingDispatch", "Durable diagnosis queued; original plan/cursor preserved.", t =>
            { t.State = "Pending"; t.AssignmentId = null; });
        }
        catch (ReviewPendingException)
        {
            UpdateTask(runId, taskId, assignment, "ReviewAwaitingDispatch", "Durable review queued; original plan and cursor preserved.", t =>
            { t.State = "Pending"; t.AssignmentId = null; });
        }
        catch (PauseBoundaryException)
        {
            if (assignment is not null) UpdateTask(runId, taskId, assignment, "WorkerCheckpoint", "Paused at a completed plan-step boundary.", t => t.State = "Pending");
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested) { }
        catch (Exception ex) when (ex is WorkerProtocolException or WorkspaceStateException or WorkerContractException)
        {
            var message = FailureMessage(ex.Message);
            UpdateTask(runId, taskId, assignment, ex is WorkspaceStateException ? "WorkspaceBlocked" : ex is WorkerContractException ? "WorkerContractBlocked" : "WorkerProtocolBlocked", message, t =>
            {
                t.State = "Blocked"; t.Failure = message; t.AssignmentId = null;
            });
        }
        catch (StepFailureException ex)
        {
            var current = Find(store.Get(runId), taskId);
            var owned = Contracts.ResolveSetupOwner(ex.Blocker, current.Definition);
            if (owned.Kind != ex.Blocker.Kind)
            {
                store.Event(runId, "BlockerOwnershipCorrected", $"{ex.Blocker.Kind} -> {owned.Kind}: {owned.RequiredPath}; assigned file ownership determines the setup repair role.", taskId);
                ex = new StepFailureException(owned);
            }
            var blocker = store.Save(runId, taskId, "StepBlocker", ex.Blocker);
            if (current.Diagnosis is not null)
            {
                var message = FailureMessage("Recovery did not satisfy original step gates; preserve diagnosis and cursor. " + ex.Message);
                UpdateTask(runId, taskId, assignment, "RecoveryRejected", message, t =>
                { t.State = "Blocked"; t.Failure = message; t.AssignmentId = null; }, blocker.Id);
                return;
            }
            var outsideOwner = ex.Blocker.RequiredPath is { } requiredPath
                && ex.Blocker.Kind is BlockerKind.MissingScaffold or BlockerKind.TestSetupFailure or BlockerKind.BehaviorFailure
                && !SafePaths.InScope(requiredPath, ex.Blocker.Kind == BlockerKind.TestSetupFailure ? current.Definition.TestPaths : current.Definition.CodePaths);
            if (ex.Blocker.Kind == BlockerKind.BehaviorFailure && ex.Blocker.RequiredPath is { } productionPath
                && !SafePaths.InScope(productionPath, current.Definition.CodePaths))
            {
                SafePaths.ValidateRelative(productionPath);
                var eligible = !SafePaths.InScope(productionPath, current.Definition.TestPaths)
                    && current.DependencyRepairRounds < DependencyRepairs.MaxRounds && current.Review is null;
                UpdateTask(runId, taskId, assignment, eligible ? "DependencyRepairRequested" : "WorkerContractBlocked",
                    "Production failure requires orchestration-owned assignment repair, not retry by an incapable owner: " + ex.Message, t =>
                    {
                        t.State = "Blocked"; t.Failure = Redaction.Apply(ex.Message); t.AssignmentId = null;
                        if (eligible) { t.DependencyRepairId = blocker.Id; t.DependencyRepairRounds++; }
                    }, blocker.Id);
                return;
            }
            if (ex.Blocker.Kind is BlockerKind.SpecificationGap or BlockerKind.CapabilityMismatch or BlockerKind.Infrastructure || outsideOwner)
                UpdateTask(runId, taskId, assignment, "WorkerContractBlocked",
                    $"{ex.Blocker.Kind}: {ex.Message}. {(outsideOwner ? "Required dependency is outside the owning role's work-item scope; orchestration/dependency repair is required." : ex.Blocker.Kind == BlockerKind.SpecificationGap ? "Specification review/human clarification is required; approval is not bypassed." : "Resolve this blocker without broadening worker permissions or replaying execution.")}", t =>
                    { t.State = "Blocked"; t.Failure = Redaction.Apply(ex.Message); t.AssignmentId = null; }, blocker.Id);
            else
            {
                var role = ex.Blocker.Kind == BlockerKind.TestSetupFailure ? "Tester" : "SpecImplementer";
                var stage = ex.Blocker.Kind == BlockerKind.MissingScaffold ? "Scaffold" : ex.Blocker.Kind == BlockerKind.TestSetupFailure ? "Tests" : "Implementation";
                await Repair(ex.Message, role, stage, ex.Blocker.Kind != BlockerKind.BehaviorFailure, blocker.Id);
            }
        }
        catch (PreparationException ex)
        {
            var stage = Find(store.Get(runId), taskId).Stage;
            if (stage == "Review" || Find(store.Get(runId), taskId).Diagnosis is not null)
            {
                var message = FailureMessage("Review/recovery validation failed; preserve the saved operation and cursor: " + ex.Message);
                UpdateTask(runId, taskId, assignment, "WorkerContractBlocked", message, t =>
                { t.State = "Blocked"; t.Failure = message; t.AssignmentId = null; });
            }
            else await Repair(ex.Message, stage == "Scaffold" ? "SpecImplementer" : "Tester", Find(store.Get(runId), taskId).VerificationPreparation ? "VerificationPreparation" : stage == "Scaffold" ? "Scaffold" : "Tests", true);
        }
        catch (BudgetException ex)
        {
            store.Change(runId, "RunLimitReached", ex.Message, r =>
            {
                r.State = "PauseRequested"; r.PreviousState = "Running"; r.Failure = ex.Message;
                var t = Find(r, taskId); t.State = "Pending"; t.AssignmentId = null;
            }, taskId);
        }
        catch (Exception ex) when (ex is not OutOfMemoryException)
        {
            logger.LogWarning(ex, "Pair {TaskId} needs repair", taskId);
            if (ex is PolicyException || Find(store.Get(runId), taskId).Stage == "Review" || Find(store.Get(runId), taskId).Diagnosis is not null)
            {
                var message = FailureMessage(ex.Message);
                UpdateTask(runId, taskId, assignment, "WorkerContractBlocked", message, t =>
                { t.Failure = message; t.State = "Blocked"; t.AssignmentId = null; });
            }
            else
            {
                var stage = Find(store.Get(runId), taskId).Stage;
                await Repair(ex.Message, stage == "Tests" || stage == "Red" ? "Tester" : "SpecImplementer",
                    stage == "Tests" || stage == "Red" ? "Tests" : stage == "Implementation" || stage == "Verify" ? "Implementation" : "Scaffold", stage is "Plans" or "Scaffold" or "Tests" or "Red");
            }
        }
        finally { pairs.Release(); }

        string FailureMessage(string error)
        {
            var diagnosis = Find(store.Get(runId), taskId).Diagnosis;
            return Redaction.Apply(diagnosis is null ? error
                : Json.Read<DiagnosisRequest>(store.ReadArtifact(diagnosis.RequestId)).Summary + "\nDiagnosis/recovery stopped: " + error);
        }

        async Task Repair(string message, string role, string stage, bool preparation, string? evidenceId = null)
        {
            var task = Find(store.Get(runId), taskId);
            var head = task.Workspace is null ? "no-workspace" : await GitWorkspace.Head(processes, task.Workspace, token);
            var failureKey = message;
            if (evidenceId is not null && store.GetArtifact(evidenceId).Kind == "StepBlocker")
            {
                var blocker = Json.Read<StepBlocker>(store.ReadArtifact(evidenceId));
                failureKey = Json.Write(new { blocker.Kind, blocker.RequiredPath, blocker.RequiredCheck,
                    reason = blocker.RequiredPath is null && blocker.RequiredCheck is null ? blocker.Reason : null });
            }
            var fingerprint = Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(System.Text.Encoding.UTF8.GetBytes(Json.Write(new { role, stage, failureKey, head }))));
            var repeated = task.LastBlockerFingerprint == fingerprint;
            UpdateTask(runId, taskId, assignment, preparation ? "PreparationRepairRequired" : "PairRepairRequired",
                $"{(repeated ? "Identical blocker on unchanged revision; no progress. " : "")}Repair owner={role}, stage={stage}: {message}", t =>
                {
                    t.LastBlockerFingerprint = fingerprint; t.Failure = Redaction.Apply(message);
                    if (preparation) t.PreparationRepairs++;
                    t.State = repeated || (preparation ? t.PreparationRepairs > options.Execution.TransientRetries : t.Attempts >= options.Execution.MaxRepairAttempts) ? "Blocked" : "Pending";
                    if (repeated) t.Failure = "Identical blocker repeated on the unchanged revision; no progress. " + t.Failure;
                    if (t.State == "Pending") { t.Stage = "Plans"; t.StepIndex = 0; t.PlanRepairRole = role; t.RepairStage = stage; }
                    t.AssignmentId = null;
                }, evidenceId);
        }
    }

    private async Task Integrate(string runId, string taskId, CancellationToken token)
    {
        await integration.WaitAsync(token);
        try
        {
            Boundary(runId);
            var run = store.Get(runId);
            var task = Find(run, taskId);
            using var scope = AuditScope.Use(new(task.AssignmentId, "Orchestrator", Attempt: task.Attempts));
            RequireApproval(run);
            var baseline = await git.BranchHead(run.Request.Repository, run.Request.Branch, token);
            if (task.CandidatePassed && task.CandidateCommit == baseline)
            {
                store.Change(runId, "IntegrationRecovered", "Tested candidate already advanced the branch before the previous checkpoint.", r => Find(r, taskId).State = "Integrated", taskId);
                return;
            }
            store.Change(runId, "IntegrationStarted", $"Build candidate against current integration head {baseline}.", r => Find(r, taskId).State = "Integrating", taskId);
            var candidatePath = await git.Candidate(run, task, baseline, token);
            var candidate = await GitWorkspace.Head(processes, candidatePath, token);
            if (!await checks.All(run, taskId, candidatePath, run.Request.Checks.Select(c => c.Name), token))
                throw new WorkflowException("Orchestrator integration gates failed on the combined candidate.");
            if ((await GitWorkspace.Git(processes, candidatePath, token, "status", "--porcelain")).Length != 0)
                throw new WorkflowException("Integration gates modified tracked/untracked candidate files; commit necessary generated artifacts in the task first.");
            store.Change(runId, "IntegrationIntent", $"All gates passed on {candidate}; advance only from {baseline}.", r =>
            {
                var t = Find(r, taskId); t.CandidateCommit = candidate; t.CandidateBase = baseline; t.CandidatePassed = true;
            }, taskId);
            if (await git.BranchHead(run.Request.Repository, run.Request.Branch, token) != baseline)
            {
                store.Change(runId, "CandidateInvalidated", "Integration head moved; rebuild and retest without discarding verified task work.", r =>
                { var t = Find(r, taskId); t.State = "Ready"; t.CandidatePassed = false; }, taskId);
                return;
            }
            await git.Advance(run, candidate, baseline, token);
            store.Change(runId, "TaskIntegrated", $"Integration branch advanced to tested candidate {candidate}.", r =>
            {
                var t = Find(r, taskId); t.State = "Integrated"; t.Failure = null;
            }, taskId);
        }
        catch (PauseBoundaryException) { }
        catch (OperationCanceledException) when (token.IsCancellationRequested) { }
        catch (BudgetException ex)
        {
            store.Change(runId, "RunLimitReached", ex.Message, r =>
            {
                r.State = "PauseRequested"; r.PreviousState = "Running";
                Find(r, taskId).State = "Ready";
            }, taskId);
        }
        catch (Exception ex) when (ex is not OutOfMemoryException)
        {
            store.Change(runId, "IntegrationRejected", ex.Message, r =>
            {
                var t = Find(r, taskId); t.Failure = Redaction.Apply(ex.Message); t.CandidatePassed = false;
                t.NeedsSync = true;
                t.State = ex is PolicyException || t.Attempts >= options.Execution.MaxRepairAttempts ? "Blocked" : "Pending";
                t.Stage = "Plans"; t.StepIndex = 0;
            }, taskId);
        }
        finally { integration.Release(); }
    }

    private async Task VerifyGoal(RunData run, CancellationToken token)
    {
        RequireApproval(run);
        var head = await git.BranchHead(run.Request.Repository, run.Request.Branch, token);
        var workspace = await git.FinalWorkspace(run, head, token);
        var evidence = await checks.Batch(run, null, workspace, run.Request.Checks.Select(c => c.Name), token, gate: true);
        var provenance = new List<System.Text.Json.Nodes.JsonNode>();
        foreach (var task in run.Tasks)
        {
            await GitWorkspace.Git(processes, workspace, token, "merge-base", "--is-ancestor", task.HeadCommit!, head);
            var proof = System.Text.Json.Nodes.JsonNode.Parse(await git.TaskProvenance(run, task, task.HeadCommit!, token))!;
            foreach (var entry in proof["timeline"]!.AsArray())
            {
                var row = entry!.AsObject();
                row.Remove("planId"); row.Remove("documentId"); row.Remove("attempt");
                if (row["kind"]!.GetValue<string>() == "PlanStepCompleted") row.Remove("message");
            }
            provenance.Add(proof);
        }
        var verification = await agents.Invoke<Verification>(new(run.Id, null, null, "Tester", "verify-goal", workspace,
            "Verify the entire approved goal against the integrated revision, including cross-package behavior. Do not infer completion from worker claims.\n"
            + store.ReadArtifact(run.ApprovedSpecificationId!) + "\nIntegrated revision: " + head
            + "\nIndependent command evidence: " + Json.Write(evidence)
            + "\nFreshly inspected task ownership/ancestry and historical process evidence: " + Json.Write(provenance)
            + "\nHistorical failed-check receipts can establish required negative probes only when their actual output, scope, source stability and restoration are supported. They do not replace the fresh final passing gates. Do not infer behavior from step summaries alone."
            + "\nReturn {\"passed\":true,\"criterionIds\":[\"C1\"],\"explanation\":\"evidence for every criterion or gaps\"}.", []), token);
        var spec = Json.Read<Specification>(store.ReadArtifact(run.ApprovedSpecificationId!));
        var passed = evidence.All(e => CheckRunner.Passed(e.Result)) && verification.Passed
            && !spec.Criteria.Select(c => c.Id).Except(verification.CriterionIds).Any()
            && (await GitWorkspace.Git(processes, workspace, token, "status", "--porcelain")).Length == 0;
        var artifact = store.Save(run.Id, null, "GoalVerification", new { head, passed, verification, evidence });
        if (await git.BranchHead(run.Request.Repository, run.Request.Branch, token) != head)
        {
            store.Event(run.Id, "GoalVerificationStale", "Integration head changed during verification; rerun final verification.", documentId: artifact.Id);
            return;
        }
        if (!passed)
        {
            if (run.Replans >= options.Execution.MaxRepairAttempts)
                throw new WorkflowException("Goal-level repair limit reached: " + verification.Explanation);
            store.Change(run.Id, "GoalReplanned", verification.Explanation, r =>
            {
                r.Replans++; r.Phase = "Planning";
                r.Feedback = "Final integrated verification failed: " + Json.Write(verification) + "\n" + Json.Write(evidence);
            }, documentId: artifact.Id);
            return;
        }
        store.Change(run.Id, "GoalCompleted", "Every approved criterion verified on the final integrated revision.", r =>
        {
            r.State = "Completed"; r.Phase = "Complete"; r.FinalCommit = head; r.Failure = null;
        }, documentId: artifact.Id);
    }

    private static TaskData Find(RunData run, string id) => run.Tasks.Single(t => t.Definition.Id == id);

    private void UpdateTask(string runId, string taskId, string? assignment, string kind, string message,
        Action<TaskData> change, string? documentId = null) => store.Change(runId, kind, message, r =>
    {
        var task = Find(r, taskId);
        if (assignment is not null && task.AssignmentId != assignment) throw new PolicyException("Rejected stale assignment result.");
        change(task); task.Heartbeat = DateTimeOffset.UtcNow;
    }, taskId, documentId);

    internal static string TaskVerificationPrompt(TaskVerificationContext context, WorkDefinition task, string testPlan, CheckExecution[] evidence, string? provenance = null) => $"""
        Independently assess ONLY the current task's explicit obligations at revision {context.Revision}.
        Current verification context: {Json.Write(context)}
        Assigned obligations (authoritative task-local completion conditions): {Json.Write(task.Obligations)}
        The parent specification remains authoritative, but criterionIds are TRACEABILITY, not a requirement that this task satisfy the whole criterion.
        Do not assess or require downstream README content, Controls/Sample projects, UI rendering or other tasks' duties unless explicitly assigned in these obligations.
        Example: a fixture obligation linked to C16 may pass when its fixture is verified, even while the later README obligation of C16 is still pending.
        This is NOT final goal verification. The final integrated gate still verifies every entire approved criterion and measurement without exemptions.
        Inspect relevant source/tests read-only. The orchestrator has freshly executed the supplied checks; use their actual EvidenceIds rather than requiring unavailable command tools.
        Native host Git is intentionally disabled, not proof that the supplied worktree is not a Git repository.
        For task ownership, commit ordering and staged execution, use the supplied current-message provenance EvidenceId and inspect its actual timeline/ancestry.
        Cite the provenance envelope's evidenceId, NOT historical documentId/event IDs inside its timeline.
        A truncated timeline cannot prove omitted history. Never infer source behavior from provenance; inspect source/tests as needed using sdlc_read_file and sdlc_read_diff.
        Return context, passed, obligations (an array of objects with id, status, explanation and evidenceIds), explanation.
        Return exactly one result per assigned obligation ID. status is satisfied, unsatisfied (evidence establishes a real defect), or notVerified (missing/insufficient evidence).
        Cite current-message read observations or the supplied command EvidenceIds. A passing suite alone is not proof of an unmeasured obligation.
        passed means ALL AND ONLY the assigned obligations are satisfied. Never fabricate evidence, claim unrun negative checks, or mark unrelated unfinished work as this task's defect.
        Test plan: {testPlan}
        Fresh command evidence: {Json.Write(evidence)}
        Current-message execution provenance: {provenance ?? "Not supplied; do not invent process-history evidence."}
        """;

    internal static string PlanPrompt(string role, WorkDefinition task, CheckCommand[] checks, bool verificationPreparation = false) => $$"""
        Inspect repository and develop only the detailed low-level plan owned by {{role}} for this assignment.
        Align with criteria, interface contracts and authorized checks. Include risks, edge cases, and measurement.
        The orchestrator owns role and stage ordering; never add phase labels, roles, or lifecycle stages to the plan.
        Return contractVersion={{Contracts.WorkerContractVersion}} using the supplied role-specific JSON schema. Both named step arrays are required.
        {{(role == "SpecImplementer" ? "scaffoldingSteps establish only compiling APIs/types/project plumbing; existing APIs may use read-only inspection. implementationSteps implement behavior and may include read-only self-checks. Independent acceptance verification belongs to the tester, not a new implementer phase." : "preparationSteps inspect existing APIs and write tests only. verificationSteps independently inspect the completed implementation; they cannot declare writePaths and all passChecks must succeed. Never plan preimplementation red assessment in verificationSteps.")}}
        Plan only this task's explicit obligations: {{Json.Write(task.Obligations)}}. Parent criterion IDs are traceability links, not whole-criterion completion duties.
        Do not require later tasks' documentation, projects or UI evidence to finish this task. Collective full criteria are checked at the final integrated gate.
        Writable steps declare writePaths explicitly, narrowed to this role's assigned paths: {{Json.Write(role == "Tester" ? task.TestPaths : task.CodePaths)}}. [] means read-only.
        {{(role == "Tester" ? "Never create production stubs, shared project plumbing, or dependencies. Report missing scaffold/dependency ownership instead of assigning production edits to tests." : "Do not edit tests or complete behavior during scaffolding. Keep the test plan intact.")}}
        Setup steps may declare only these successful prerequisite checks: {{Json.Write(Contracts.CheckNamesFor(verificationPreparation ? WorkerStage.VerificationPreparation : WorkerStage.Scaffold, task, checks))}}.
        {{(verificationPreparation ? "This is a recorded postimplementation missing-evidence repair, not preimplementation test creation. There may be only ONE writable preparation step: it must edit, build, run the expected failing test, restore exact original assertions and rebuild/retest green WITHIN THAT SAME STEP. Commit capability is absent and the orchestrator rejects any remaining source/HEAD change. Do not delegate the failing probe to the orchestrator; run it using your assigned check tools. Additional read-only steps are allowed. Use sdlc_read_diff with workingTreeOnly=true for uncommitted changes, since the default includes earlier committed task work. All declared checks are final restored passing conditions, not the expected temporary failure." : "")}}
        {{(role == "Tester" ? "Final passChecks" : "Implementation checks")}} may use: {{Json.Write(Contracts.CheckNamesFor(WorkerStage.Implementation, task, checks))}}.
        All declared checks are success conditions. The orchestrator alone runs and assesses meaningful behavioral red between test preparation and implementation.
        Give each step a unique id, a bounded action, an observable expectedOutcome, and verification explaining only that step's completion.
        Do not require tests for later work items. Missing APIs/compilation failures are setup problems, not red assertions.
        """;

    internal static string StepPrompt(StepAssignment execution, string instructions) => $"""
        {instructions}
        Execute ONLY the current step below. Other plan steps and stages are not assigned to this session.
        Current-step assignment: {Json.Write(execution)}
        Completion means only currentStep.expectedOutcome and currentStep.verification are satisfied. Do not assess completion of the whole plan.
        Empty writePaths intentionally means read-only; write/delete/commit tools are absent. Later steps needing those tools are NOT current-step blockers.
        Cite EvidenceIds actually returned by current-step repository/check tools. Do not reuse another step/assignment's observations.
        Code and tests are claims against the approved specification, not competing sources of authority.
        If another role's output appears defective, return status=reviewRequested and blocker=null. Supply a challenge object with target=test|implementation|specificationInterpretation,
        claim=the specific disagreement, criterionIds=assigned criteria and paths=repository-relative subject paths, with observed evidenceIds.
        The orchestrator routes read-only reassessment and a scoped owner repair; never edit another role's files or classify a suspected test defect as a product-specification gap.
        A genuine missing/ambiguous product decision goes to specificationInterpretation review by SpecCreator before any human escalation.
        Before requesting another role, commit any coherent authorized changes; a dirty handoff blocks safely and is never auto-cleaned or committed under another role.
        The orchestrator independently runs declared checks as successful completion conditions; final verification cannot demand preimplementation red.
        Run successful restore/build prerequisites before test checks. Never run stale --no-build tests after compilation fails.
        Report completed with blocker=null only when current-step evidence supports it. Otherwise report blocked/failed with a structured blocker:
        missingScaffold for absent production APIs, testSetupFailure for defective test setup, behaviorFailure for unmet implementation behavior,
        capabilityMismatch for a missing capability actually required by THIS step, specificationGap for an unresolved product decision,
        or infrastructure for an external execution problem. Do not grant yourself permissions or turn later steps into blockers.
        Return only the current-step result matching the supplied JSON schema and its exact assignmentId, planId, and stepId.
        """;
}
