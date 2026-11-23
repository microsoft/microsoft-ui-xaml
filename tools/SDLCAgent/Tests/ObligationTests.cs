using SDLCAgent;

namespace SDLCAgentTests;

public static partial class Program
{
    private static async Task TaskObligationProtocol()
    {
        await DetachedCallbackEvidence();
        foreach (var mode in new[] { "shared", "coverage-repair", "provenance", "verification-repair", "verification-reconcile", "verification-repeat", "verification-dirty", "missing", "fabricated", "final-reject" })
        {
            using var fixture = await Fixture.Create();
            fixture.Options.Execution.MaxRepairAttempts = 1;
            fixture.Options.Execution.TransientRetries = 1;
            var backend = new ObligationBackend(fixture, mode);
            var engine = fixture.Engine(backend);
            var run = await fixture.NewRun();
            if (mode == "final-reject") fixture.Store.Change(run.Id, "FixtureGoalRepairLimit", "Simulate exhausted prior goal repair budget.", r => r.Replans = 1);
            await engine.Run(run.Id, CancellationToken.None);
            new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(150));
            await engine.Run(run.Id, timeout.Token);
            run = fixture.Store.Get(run.Id);
            var events = fixture.Store.Events(run.Id, limit: 200).Concat(new[] { "DecompositionCoverageAccepted", "DecompositionRejected", "VerificationPreparationRequested", "PairRepairRequired" }
                .SelectMany(kind => fixture.Store.Events(run.Id, limit: 10, kind: kind))).ToArray();
            var first = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
            var second = run.Tasks.Single(t => t.Definition.Id.EndsWith("-beta", StringComparison.Ordinal));
            Expect(first.Definition.CriterionIds.SequenceEqual(second.Definition.CriterionIds)
                && first.Definition.Obligations![0].Id != second.Definition.Obligations![0].Id,
                "Shared criterion lost distinct task-local duties.");
            Expect(!events.Any(e => e.Kind == "PairRepairRequired") && first.Attempts == (mode.StartsWith("verification-", StringComparison.Ordinal) ? 2 : 1),
                "A task-assessment/coverage problem incorrectly replayed implementation: " + mode + "; " + run.Failure);
            if (mode is "shared" or "coverage-repair" or "provenance" or "verification-repair" or "verification-reconcile")
            {
                Expect(run.State == "Completed" && run.Tasks.All(t => t.State == "Integrated") && backend.SawUpstreamVerificationWithoutReadme,
                    "Fixture obligation required downstream README completion and deadlocked: " + run.Failure);
                Expect(File.Exists(Path.Combine(fixture.Repository, "src", "alpha.txt")) && backend.FinalVerificationCalls == 1,
                    "Task slices bypassed full final-goal verification.");
                Expect(backend.CoverageCalls == (mode == "coverage-repair" ? 2 : 1)
                    && events.Any(e => e.Kind == "DecompositionCoverageAccepted"), "Collective coverage/ownership was not independently validated.");
                if (mode == "coverage-repair") Expect(events.Any(e => e.Kind == "DecompositionRejected"), "Unsafe coverage was not replanned before scheduling.");
                if (mode == "provenance") Expect(backend.SawProcessEvidence,
                    "Process-history obligation had no current-message ownership/ancestry/stage evidence and required unavailable native Git.");
                if (mode == "provenance") Expect(backend.SawBoundedGoalContext,
                    "Historical unrelated build output overflowed whole-goal verification context instead of remaining in audit artifacts.");
                if (mode == "verification-repair") Expect(backend.ImplementationPlans == 2 && backend.FirstImplementationCalls == 2
                    && backend.SawVerificationPreparation && backend.SawFailedProbeReceipt && backend.CommitDenied && backend.SawDistinctDiffs && events.Any(e => e.Kind == "VerificationPreparationRequested"),
                    "Missing tester evidence replayed implementation, bypassed preparation, or failed independent re-verification.");
                if (mode == "verification-reconcile") Expect(backend.ProbeExecutions == 1 && backend.SawReadOnlyProbeRecovery
                    && fixture.Store.Events(run.Id, first.Definition.Id, kind: "RestoredProbeReconciled").Length == 1
                    && fixture.Store.Events(run.Id, first.Definition.Id, kind: "RecoveryVerified").Length == 1,
                    "A restored, measured probe with result-only evidence loss was replayed, left blocked, or recovered with write authority.");
            }
            else if (mode == "verification-repeat")
                Expect(run.State == "Blocked" && first.PreparationRepairs == 2 && backend.FirstImplementationCalls == 2
                    && second.State == "Pending", "Repeated missing evidence was retried without a bounded repair budget or replayed implementation.");
            else if (mode == "verification-dirty")
                Expect(run.State == "Blocked" && first.Stage == "VerificationPreparation" && first.StepIndex == 1
                    && first.Failure!.Contains("did not restore the exact starting source", StringComparison.Ordinal)
                    && (await GitWorkspace.Git(fixture.Processes, first.Workspace!, timeout.Token, "status", "--porcelain")).Length != 0
                    && backend.FirstImplementationCalls == 2, "An unrestored verification probe was committed, silently cleaned, or replayed implementation.");
            else if (mode == "final-reject")
                Expect(run.State == "Blocked" && run.Tasks.All(t => t.State == "Integrated") && run.FinalCommit is null,
                    "Passing partial obligations incorrectly waived complete parent-criterion final verification.");
            else
            {
                Expect(run.State == "Blocked" && first.Stage == "Verify" && first.StepIndex == 1 && second.State == "Pending"
                    && backend.ImplementationPlans == 1 && backend.TaskVerificationCalls == 2,
                    "Missing/foreign assessment evidence reset plans, consumed implementation attempts or scheduled downstream work.");
                Expect(events.Any(e => e.Kind == "ReadOnlyContractRejected"), "Invalid obligation assessments were not corrected read-only.");
            }
        }
        ObligationContracts();
    }

    private static async Task DetachedCallbackEvidence()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var definition = new WorkDefinition("fixture", "Fixture", "Inspect existing source", ["C1"], [], ["src"], ["tests"], ["acceptance"], true, null,
            [new("source", "C1", "Source exists", "Read source", ["src\\alpha.txt"], ["acceptance"])]);
        var spec = fixture.Store.Save(run.Id, null, "Specification", new { title = "Fixture" });
        var context = new TaskVerificationContext("detached-message", "assignment", "fixture", spec.Id, spec.Sha256, run.InitialCommit);
        fixture.Store.Change(run.Id, "FixtureCallback", "SDK callbacks need explicit evidence context, not inherited AsyncLocal state.", r =>
        {
            r.State = "Running"; r.SpecificationId = spec.Id; r.ApprovedSpecificationId = spec.Id; r.ApprovedSpecificationHash = spec.Sha256;
            r.Tasks = [new() { Definition = definition, State = "Running", Stage = "Verify", AssignmentId = "assignment", PendingVerification = context }];
        });
        var call = new AgentCall(run.Id, "fixture", "assignment", "Tester", "verify-task", fixture.Repository, "Read source", [], Verification: context);
        var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, CancellationToken.None);
        Task<string> callback;
        using (System.Threading.ExecutionContext.SuppressFlow()) callback = Task.Run(() => tools.SafeReadFile("src\\alpha.txt"));
        await callback;
        var observed = fixture.Store.Events(run.Id, "fixture", limit: 1, kind: "FileRead").Single();
        Expect(fixture.Store.HasObservation(run.Id, "fixture", "assignment", context.MessageId, "TaskVerification", observed.DocumentId!),
            "An SDK callback lost its current-message evidence authority when ambient execution context was not inherited.");
    }

    private static void ObligationContracts()
    {
        var definition = new WorkDefinition("fixture", "Fixture", "Create sample only", ["C16"], [], ["fixtures"], ["tests"], ["test"], true, null,
            [new("sample", "C16", "sample.dgml parses", "Run fixture test", ["fixtures\\sample.dgml"], ["test"])]);
        Contracts.ValidateObligations(definition);
        var schema = Contracts.ResultSchema<Decomposition>()["properties"]!["tasks"]!["items"]!;
        Expect(schema["required"]!.AsArray().Any(n => n!.GetValue<string>() == "obligations")
            && schema["properties"]!["obligations"]!["type"]!.GetValue<string>() == "array",
            "Decomposition schema advertised optional/null obligations despite the execution invariant.");
        foreach (var invalid in new[]
        {
            definition with { Obligations = null }, definition with { Obligations = [] },
            definition with { Obligations = [definition.Obligations![0] with { Paths = ["README.md"] }] },
            definition with { Obligations = [definition.Obligations![0] with { Checks = ["not-authorized"] }] },
            definition with { Obligations = [definition.Obligations![0] with { CriterionId = "C1" }] },
            definition with { Obligations = [definition.Obligations![0], definition.Obligations[0]] }
        }) Throws<InvalidDataException>(() => Contracts.ValidateObligations(invalid), "Invalid/whole-criterion historical task obligations were accepted.");
        var context = new TaskVerificationContext("message", "assignment", "fixture", "spec", "hash", "head");
        var run = new RunData
        {
            Id = "run",
            Request = new("repo", "branch", "goal", [], true),
            InitialCommit = "head",
            PolicyJson = "{}",
            PolicyHash = "policy",
            SpecificationId = "spec",
            ApprovedSpecificationId = "spec",
            ApprovedSpecificationHash = "hash"
        };
        var task = new TaskData { Definition = definition, State = "Running", Stage = "Verify", AssignmentId = "assignment", PendingVerification = context };
        var result = new TaskVerificationResult(context, true, [new("sample", ObligationStatus.Satisfied, "Fixture passes; documentation is a later duty", ["evidence"])], "Only this assigned slice is complete");
        Contracts.ValidateTaskVerification(context, result, run, task);
        foreach (var invalid in new[]
        {
            result with { Obligations = [] }, result with { Context = context with { Revision = "stale" } },
            result with { Obligations = [result.Obligations[0] with { Id = "C16" }] },
            result with { Obligations = [result.Obligations[0] with { EvidenceIds = [] }] },
            result with { Obligations = [result.Obligations[0] with { Status = ObligationStatus.NotVerified, EvidenceIds = [] }] }
        }) Throws<InvalidDataException>(() => Contracts.ValidateTaskVerification(context, invalid, run, task), "Invalid task assessment passed.");
        Contracts.ValidateTaskVerification(context, result with { Passed = false, Obligations = [result.Obligations[0] with { Status = ObligationStatus.NotVerified, EvidenceIds = [] }] }, run, task);
        foreach (var stale in new[] { context with { AssignmentId = "old" }, context with { SpecificationHash = "old" }, context with { MessageId = "old" } })
            Throws<WorkerContractException>(() => Contracts.ValidateTaskVerificationContext(stale, run, task), "Stale task-verification context was accepted.");
        var check = new CheckCommand("test", "test", "fixture", []);
        var probe = new PlanStep("probe", "Measure and restore an assigned negative probe", "Original assertion restored; final test green", "Observe failure, restore and retest", ["tests"], ["test"]);
        var probePlan = new TesterPlan(3, "Missing tester-owned evidence", [], [probe], [new("verify", "Inspect restored tests", "Green", "Fresh test", ["test"])]);
        Throws<PlanContractException>(() => Contracts.ValidatePlan(probePlan, "Tester", definition, [check]), "Preimplementation test preparation gained acceptance-check authority.");
        Contracts.ValidatePlan(probePlan, "Tester", definition, [check], verificationPreparation: true);
        Throws<PlanContractException>(() => Contracts.ValidatePlan(probePlan with { PreparationSteps = [probe, probe with { Id = "revert" }] }, "Tester", definition, [check], verificationPreparation: true), "A negative probe could be split across committed edit/revert steps.");
        var probingTask = new TaskData { Definition = definition, State = "Running", Stage = "VerificationPreparation", AssignmentId = "assignment", TestPlanId = "plan" };
        var probing = new StepAssignment("assignment", "plan", "Tester", WorkerStage.VerificationPreparation, probe, new(["tests"], ["test"], false));
        Throws<WorkerContractException>(() => Contracts.ValidateStepAssignment(probing, probingTask, Json.Write(probePlan), [check]), "Unrecorded verification preparation authorized negative test execution.");
        probingTask.VerificationPreparation = true;
        Contracts.ValidateStepAssignment(probing, probingTask, Json.Write(probePlan), [check]);
    }

    private static async Task CopilotObligationsSmoke()
    {
        ExecutionPolicy.RequireNonElevated();
        using var fixture = await Fixture.Create();
        Directory.CreateDirectory(Path.Combine(fixture.Repository, "fixtures"));
        File.WriteAllText(Path.Combine(fixture.Repository, "fixtures", "sample.dgml"), "sample");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "add", ".");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "commit", "-m", "Seed isolated shared criterion fixture");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "update-ref", "refs/heads/delivery",
            await GitWorkspace.Head(fixture.Processes, fixture.Repository, CancellationToken.None));
        var run = await fixture.NewRun();
        var spec = new Specification("Shared criterion", "Sample and README", "Two dependent duties", [], "Sample exists; README is a later work item", "Create sample first and documentation later",
            [new("C16", "A sample fixture exists AND complete README documentation exists", "Read sample.dgml to confirm exact content sample; later read README to confirm the validation matrix", ["acceptance"])], []);
        var artifact = fixture.Store.Save(run.Id, null, "Specification", spec);
        var task = new WorkDefinition("fixture", "Sample fixture", "Verify sample only", ["C16"], [], ["fixtures"], ["tests"], ["acceptance"], true, null,
            [new("sample", "C16", "The sample fixture exists", "Read fixtures\\sample.dgml and assert its exact content is sample", ["fixtures\\sample.dgml"], ["acceptance"]),
             new("ownership", "C16", "Verification uses this run's owned clean task worktree", "Inspect current-message provenance for verified ownership and the exact revision; native Git is intentionally unavailable", ["fixtures\\sample.dgml"], ["acceptance"])]);
        var context = new TaskVerificationContext("message", "assignment", "fixture", artifact.Id, artifact.Sha256, run.InitialCommit);
        fixture.Store.Change(run.Id, "SmokeTaskVerification", "Isolated obligation assessment; never a product-run approval.", r =>
        {
            r.State = "Running"; r.Phase = "Execution"; r.SpecificationId = artifact.Id; r.ApprovedSpecificationId = artifact.Id; r.ApprovedSpecificationHash = artifact.Sha256;
            r.Tasks = [new() { Definition = task, State = "Running", Stage = "Verify", AssignmentId = "assignment", PendingVerification = context,
                BaseCommit = r.InitialCommit }];
        });
        var current = fixture.Store.Get(run.Id);
        var owned = await fixture.Git.TaskWorkspace(current, current.Tasks[0], CancellationToken.None);
        Expect(await File.ReadAllTextAsync(Path.Combine(owned.Path, "fixtures", "sample.dgml")) == "sample",
            "Owned smoke worktree does not contain the seeded fixture revision.");
        current = fixture.Store.Get(run.Id);
        var provenance = await fixture.Git.TaskProvenance(current, current.Tasks[0], run.InitialCommit, CancellationToken.None);
        Artifact observation;
        using (AuditScope.Use(new("assignment", "Tester", context.MessageId, "TaskVerification")))
        {
            observation = fixture.Store.Save(run.Id, "fixture", "RepositoryEvidence", new
            { workspace = owned.Path, kind = "TaskProvenance", path = ".", observation = provenance });
            fixture.Store.Event(run.Id, "TaskProvenanceObserved", "Read-only smoke uses real owned worktree provenance.", "fixture", observation.Id);
        }
        var runner = new AgentRunner(fixture.Store, new CopilotBackend(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, fixture.Options), fixture.Options.Execution);
        Expect(fixture.Store.HasObservation(run.Id, "fixture", "assignment", context.MessageId, "TaskVerification", observation.Id),
            "Smoke provenance was not bound to the current verification message.");
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(180));
        TaskVerificationResult result;
        try
        {
            result = await runner.Invoke<TaskVerificationResult>(new(run.Id, "fixture", "assignment", "Tester", "verify-task", owned.Path,
                "Approved full specification: " + Json.Write(spec) + "\n" + Orchestrator.TaskVerificationPrompt(context, task, "Read-only fixture inspection", [],
                    Json.Write(new { evidenceId = observation.Id, observation = provenance })),
                [], ChangeBase: run.InitialCommit, Verification: context), timeout.Token);
        }
        catch (WorkerProtocolException ex)
        {
            var response = fixture.Store.Artifacts(run.Id).First(a => a.Kind == "AgentResponse");
            var text = System.Text.Json.Nodes.JsonNode.Parse(fixture.Store.ReadArtifact(response.Id))!["text"]!.GetValue<string>();
            var reads = fixture.Store.Events(run.Id, "fixture", limit: 200).Where(e => e.Kind is "FileRead" or "FilesListed" or "TaskProvenanceObserved")
                .Select(e => new { e.DocumentId, e.AssignmentId, e.PlanId, e.StepId });
            throw new InvalidOperationException("Smoke provenance ID: " + observation.Id + "\nObserved evidence contexts: " + Json.Write(reads)
                + "\nLast smoke response: " + text, ex);
        }
        Expect(result.Passed && result.Obligations.Length == 2 && result.Obligations.Any(o => o.Id == "sample")
            && result.Obligations.Single(o => o.Id == "ownership").EvidenceIds.Contains(observation.Id)
            && !File.Exists(Path.Combine(owned.Path, "README.md"))
            && (await GitWorkspace.Git(fixture.Processes, owned.Path, timeout.Token, "status", "--porcelain")).Length == 0,
            "Real Copilot demanded unassigned downstream documentation or failed to verify its explicit sample obligation.");
        Console.WriteLine("PASS: real Copilot verified the upstream fixture and its evidence-bound worktree ownership without native Git; downstream README remained explicitly unverified.");
    }
}

internal sealed class ObligationBackend(Fixture fixture, string mode) : IAgentBackend
{
    private readonly FakeBackend inner = new(fixture.Store);
    public int CoverageCalls { get; private set; }
    public int TaskVerificationCalls { get; private set; }
    public int ImplementationPlans { get; private set; }
    public int FinalVerificationCalls { get; private set; }
    public bool SawUpstreamVerificationWithoutReadme { get; private set; }
    public bool SawProcessEvidence { get; private set; }
    public int FirstImplementationCalls { get; private set; }
    public bool SawVerificationPreparation { get; private set; }
    public bool SawFailedProbeReceipt { get; private set; }
    public bool CommitDenied { get; private set; }
    public bool SawDistinctDiffs { get; private set; }
    public int ProbeExecutions { get; private set; }
    public bool SawReadOnlyProbeRecovery { get; private set; }
    public bool SawBoundedGoalContext { get; private set; }

    public async Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        if (call.Operation == "specification")
            return new(Json.Write(new Specification("Shared documentation criterion", "Fixture and README", "Two dependent deliverables", [], "Fixture repository", "Sample first, documentation afterwards",
                [new("C16", "A verified sample fixture AND complete README documentation", "Fixture parses, README validation matrix exists, both are verified on the final tree", ["acceptance"])], [])), []);
        if (call.Operation == "decomposition")
        {
            var original = await inner.Run(call, token);
            var tasks = Json.Read<Decomposition>(original.FinalText).Tasks.Select(t =>
            {
                var first = t.Id == "alpha";
                var path = first ? "fixtures\\sample.dgml" : "README.md";
                return t with
                {
                    CriterionIds = ["C16"],
                    Dependencies = first ? [] : ["alpha"],
                    CodePaths = t.CodePaths.Append(path).ToArray(),
                    Obligations = [new(first ? "sample" : "readme", "C16", first ? "Verified sample fixture" : "Complete README validation matrix",
                        mode == "coverage-repair" && CoverageCalls == 0 && first ? "Requires downstream README now"
                            : mode == "provenance" && first ? "Verify the owned file and independently supplied worktree ownership and scaffold-before-test execution evidence"
                            : "Read the owned file and run acceptance",
                        mode.StartsWith("verification-", StringComparison.Ordinal) && first ? t.TestPaths : [path], ["acceptance"])]
                };
            }).ToArray();
            return new(Json.Write(new Decomposition(tasks)), []);
        }
        if (call.Operation == "verify-decomposition")
        {
            CoverageCalls++;
            return new(Json.Write(new CoverageAssessment(mode != "coverage-repair" || CoverageCalls > 1,
                "Fixture task must verify only its sample slice; complete README belongs to its dependent documentation owner. Together all C16 requirements remain mandatory.")), []);
        }
        if (call.Operation == "implementation-plan")
        {
            ImplementationPlans++;
            var original = await inner.Run(call, token);
            var plan = Json.Read<ImplementerPlan>(original.FinalText);
            var task = fixture.Store.Get(call.RunId).Tasks.Single(t => t.Definition.Id == call.TaskId);
            plan = plan with { ImplementationSteps = [plan.ImplementationSteps[0] with { WritePaths = task.Definition.CodePaths }, plan.ImplementationSteps[1]] };
            return new(Json.Write(plan), []);
        }
        if (call.Operation == "test-plan" && fixture.Store.Get(call.RunId).Tasks.Single(t => t.Definition.Id == call.TaskId).VerificationPreparation)
        {
            var response = await inner.Run(call, token);
            var plan = Json.Read<TesterPlan>(response.FinalText);
            return new(Json.Write(plan with { PreparationSteps = plan.PreparationSteps.Select(s => s.WritePaths.Length == 0 ? s : s with { Checks = ["acceptance"] }).ToArray() }), []);
        }
        if (call.Operation == "implement" && call.WriteScopes.Length != 0)
        {
            var path = call.WriteScopes[1];
            var full = SafePaths.Resolve(call.Workspace, path);
            Directory.CreateDirectory(Path.GetDirectoryName(full)!);
            File.WriteAllText(full, "after");
        }
        if (call.Operation == "implement" && call.TaskId!.EndsWith("-alpha", StringComparison.Ordinal)) FirstImplementationCalls++;
        if (mode == "provenance" && FirstImplementationCalls == 1 && call.Operation == "implement")
        {
            var unrelated = fixture.Store.Save(call.RunId, call.TaskId, "CommandEvidence", new
            { check = new { name = "old-build", kind = "build" }, exitCode = 1, output = new string('X', 1024 * 1024) });
            fixture.Store.Event(call.RunId, "CommandFinished", "Historical build failure; full output stays in audit storage.", call.TaskId, unrelated.Id);
        }
        if (call.Operation == "prepare-tests" && fixture.Store.Get(call.RunId).Tasks.Single(t => t.Definition.Id == call.TaskId).VerificationPreparation)
        {
            SawVerificationPreparation |= call.Role == "Tester" && call.Execution!.Stage == WorkerStage.VerificationPreparation
                && call.WriteScopes.All(p => SafePaths.InScope(p, ["tests"]));
            if (mode is "verification-repair" or "verification-reconcile" && call.WriteScopes.Length != 0)
            {
                ProbeExecutions++;
                var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, token);
                try { await tools.Commit("Must not commit a probe"); }
                catch (PolicyException) { CommitDenied = true; }
                var taskDiff = await tools.ReadDiff(call.WriteScopes[0]);
                var workingDiff = await tools.ReadDiff(call.WriteScopes[0], workingTreeOnly: true);
                SawDistinctDiffs = taskDiff.Contains("diff --git", StringComparison.Ordinal) && workingDiff.EndsWith('\n');
                var original = File.ReadAllText(SafePaths.Resolve(call.Workspace, call.WriteScopes[0]));
                tools.WriteFile(call.WriteScopes[0], "deliberately impossible expectation\n");
                var failed = Json.Read<CommandResult>(await tools.RunCheck("acceptance"));
                tools.WriteFile(call.WriteScopes[0], original);
                var passed = Json.Read<CommandResult>(await tools.RunCheck("acceptance"));
                if (failed.ExitCode == 0 || passed.ExitCode != 0) throw new InvalidOperationException("Negative verification probe did not fail and restore green.");
                if (mode == "verification-reconcile")
                    return new(FakeBackend.Result(fixture.Store, call, false, "Probe was measured and restored but completion receipt IDs were lost in representation repair.", BlockerKind.Infrastructure), []);
            }
            if (call.RecoveryVerificationOnly)
                SawReadOnlyProbeRecovery |= call.WriteScopes.Length == 0 && !call.Execution!.Capabilities.CanCommit;
            if (mode == "verification-dirty" && call.WriteScopes.Length != 0)
            {
                var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, token);
                tools.WriteFile(call.WriteScopes[0], "unrestored probe\n");
                return new(FakeBackend.Result(fixture.Store, call, true, "Incorrectly claimed completion with an unrestored probe."), []);
            }
        }
        if (call.Operation == "verify-task")
        {
            TaskVerificationCalls++;
            var first = call.TaskId!.EndsWith("-alpha", StringComparison.Ordinal);
            if (first) SawUpstreamVerificationWithoutReadme |= !File.Exists(Path.Combine(call.Workspace, "README.md"));
            var result = Json.Read<TaskVerificationResult>(FakeBackend.TaskResult(fixture.Store, call));
            if (mode is "verification-repair" or "verification-reconcile" && first && TaskVerificationCalls > 1)
            {
                var id = fixture.Store.Events(call.RunId, call.TaskId, limit: 1, kind: "TaskProvenanceObserved").Single().DocumentId!;
                var envelope = System.Text.Json.Nodes.JsonNode.Parse(fixture.Store.ReadArtifact(id))!;
                var proof = System.Text.Json.Nodes.JsonNode.Parse(envelope["observation"]!.GetValue<string>())!;
                SawFailedProbeReceipt = proof["historicalFailedChecks"]!.AsArray().Any(n => n!["receipt"]!["exitCode"]!.GetValue<int>() != 0
                    && !n["receipt"]!["sourceChanged"]!.GetValue<bool>() && !n["receipt"]!["outputTruncated"]!.GetValue<bool>());
            }
            if (first && (mode == "verification-repeat" || mode is "verification-repair" or "verification-reconcile" or "verification-dirty" && TaskVerificationCalls == 1))
                result = result with
                {
                    Passed = false,
                    Obligations = result.Obligations.Select(o => o with
                    { Status = ObligationStatus.NotVerified, Explanation = "Assigned tester-only negative probe has not yet been measured." }).ToArray()
                };
            if (mode == "provenance" && first)
            {
                var observed = fixture.Store.Events(call.RunId, call.TaskId, limit: 1, kind: "TaskProvenanceObserved").SingleOrDefault();
                if (observed?.DocumentId is { } id && call.Prompt.Contains(id, StringComparison.Ordinal))
                {
                    var artifact = System.Text.Json.Nodes.JsonNode.Parse(fixture.Store.ReadArtifact(id))!;
                    var proof = System.Text.Json.Nodes.JsonNode.Parse(artifact["observation"]!.GetValue<string>())!;
                    var timeline = proof["timeline"]!.AsArray();
                    var scaffold = timeline.First(n => n!["role"]?.GetValue<string>() == "SpecImplementer" && n["stepId"]?.GetValue<string>() == "scaffold-1")!;
                    var test = timeline.First(n => n!["role"]?.GetValue<string>() == "Tester")!;
                    SawProcessEvidence = proof["revision"]!.GetValue<string>() == call.Verification!.Revision
                        && proof["workspace"]!["path"]!.GetValue<string>() == call.Workspace
                        && proof["workspace"]!["branch"]!.GetValue<string>() == fixture.Store.Get(call.RunId).Tasks.Single(t => t.Definition.Id == call.TaskId).Branch
                        && proof["ownershipVerified"]!.GetValue<bool>() && !proof["truncated"]!.GetValue<bool>()
                        && scaffold["id"]!.GetValue<long>() < test["id"]!.GetValue<long>()
                        && proof["committedChangesInAncestryOrder"]!.GetValue<string>().Contains("src/alpha.txt", StringComparison.Ordinal);
                    result = result with { Obligations = result.Obligations.Select(o => o with { EvidenceIds = o.EvidenceIds.Append(id).ToArray() }).ToArray() };
                }
                if (!SawProcessEvidence)
                    result = result with { Passed = false, Obligations = result.Obligations.Select(o => o with { Status = ObligationStatus.NotVerified, EvidenceIds = [] }).ToArray() };
            }
            if (mode == "missing" && TaskVerificationCalls == 1) result = result with { Obligations = [] };
            else if (mode == "missing") result = result with { Passed = false, Obligations = result.Obligations.Select(o => o with { Status = ObligationStatus.NotVerified, EvidenceIds = [] }).ToArray() };
            if (mode == "fabricated") result = result with { Obligations = result.Obligations.Select(o => o with { EvidenceIds = ["foreign-evidence"] }).ToArray() };
            return new(Json.Write(result), []);
        }
        if (call.Operation == "verify-goal")
        {
            FinalVerificationCalls++;
            if (mode == "provenance") SawBoundedGoalContext = call.Prompt.Length < 50000
                && call.Prompt.Contains("Historical build failure; full output stays in audit storage.", StringComparison.Ordinal)
                && !call.Prompt.Contains("\\\"taskId\\\"", StringComparison.Ordinal)
                && !call.Prompt.Contains(new string('X', 1000), StringComparison.Ordinal);
            var complete = File.Exists(Path.Combine(call.Workspace, "README.md")) && File.Exists(Path.Combine(call.Workspace, "fixtures", "sample.dgml"));
            return new(Json.Write(new Verification(complete && mode != "final-reject", ["C16"], "Final gate assesses the whole approved criterion, not just completed task slices.")), []);
        }
        return await inner.Run(call, token);
    }
}
