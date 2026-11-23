using SDLCAgent;

namespace SDLCAgentTests;

public static partial class Program
{
    private static async Task ReviewProtocol()
    {
        foreach (var mode in new[] { "accepted", "disagreed", "implementation", "rejected-repair", "repair-transport", "repair-exception", "no-progress", "stale-revision" })
        {
            using var fixture = await Fixture.Create();
            var backend = new ReviewBackend(fixture, mode);
            var engine = fixture.Engine(backend);
            var run = await fixture.NewRun();
            await engine.Run(run.Id, CancellationToken.None);
            new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
            var approved = fixture.Store.Get(run.Id).ApprovedSpecificationId;
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(150));
            await engine.Run(run.Id, timeout.Token);
            run = fixture.Store.Get(run.Id);
            var task = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
            var events = fixture.Store.Events(run.Id, task.Definition.Id, limit: 200);
            Expect(run.ApprovedSpecificationId == approved && backend.ImplementationPlans == 1 && backend.TestPlans == 1,
                "Review changed approval or regenerated unrelated worker plans.");
            Expect(events.Any(e => e.Kind == "ReviewMessageQueued") && task.Attempts == 1,
                "Review requests were not durable or consumed implementation repair attempts.");
            if (mode is "accepted" or "disagreed" or "implementation")
            {
                Expect(run.State == "Completed" && backend.RepairCalls == 1 && backend.RepairVerificationCalls == 1 && task.Review is null,
                    "A confirmed tester defect could not complete its owner-scoped repair and resume the original cursor: " + run.Failure);
                Expect(backend.SawBeforeFailure && backend.SawAfterPass && backend.SawOriginalDiff && backend.RepairRole == (mode == "implementation" ? "SpecImplementer" : "Tester"),
                    "Test repair lacked before/after evidence, original-revision diff, or tester ownership.");
                Expect(events.Count(e => e.Kind == "PlanStepCompleted" && e.StepId == "step-1") == 1
                    && events.Count(e => e.Kind == "ReviewRepairStepCompleted") == 1,
                    "Completed implementation or review repair steps replayed.");
                Expect(backend.DecisionRoles.Contains("SpecCreator") == (mode == "disagreed"),
                    "Counterparty disagreement did not escalate to SpecCreator, or an admitted defect escalated unnecessarily.");
            }
            else if (mode == "rejected-repair")
                Expect(run.State == "Blocked" && task.Stage == "Review" && task.Review?.Stage == "VerifyRepair"
                    && backend.RepairCalls == 1 && !events.Any(e => e.Kind == "TaskIntegrated"),
                    "A rejected/coverage-weakening repair reached integration or replayed.");
            else if (mode == "no-progress")
                Expect(run.State == "Blocked" && task.ReviewRounds == Contracts.MaxReviewRounds && backend.RepairCalls == 0
                    && run.Failure!.Contains("review limit") && backend.DecisionRoles.Count(role => role == "Tester") == 1,
                    "Repeated disagreement without new evidence failed to escalate/bound review rounds.");
            else if (mode is "repair-transport" or "repair-exception")
                Expect(run.State == "Blocked" && task.Stage == "Review" && task.Review?.Stage == "Repairing" && task.Review.RepairStepIndex == 0
                    && backend.RepairCalls == 1 && !events.Any(e => e.Kind == "PairRepairRequired")
                    && File.ReadAllText(Path.Combine(task.Workspace!, "tests", "alpha.expected")).Trim() == "after",
                    "Uncertain/failed review repair replayed writes or reset the original task instead of preserving review state.");
            else
                Expect(run.State == "Blocked" && backend.RepairCalls == 0 && task.Failure!.Contains("source revision changed"),
                    "A stale source revision authorized a review repair.");
        }

        await PauseDuringReview();
        await RejectReviewContracts();
        await RejectReviewReplies();
    }

    private static async Task PauseDuringReview()
    {
        using var fixture = await Fixture.Create();
        var backend = new ReviewBackend(fixture, "accepted");
        var engine = fixture.Engine(backend);
        var run = await fixture.NewRun();
        await engine.Run(run.Id, CancellationToken.None);
        var controls = new RunControls(fixture.Store);
        controls.Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
        backend.BeforeDecision = () => controls.Pause(run.Id);
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(150));
        await engine.Run(run.Id, timeout.Token);
        run = fixture.Store.Get(run.Id);
        var paused = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
        Expect(run.State == "Paused" && paused.Review?.Stage == "RepairPlanning" && paused.Review.ReturnStepIndex == 1,
            "Pause did not checkpoint the saved review decision and original current-step cursor.");
        var decisionId = paused.Review!.DecisionId;
        var requestId = paused.Review.Id;
        var reopened = new Store(fixture.State);
        new RunControls(reopened).Resume(run.Id);
        backend.BeforeRepair = () => new RunControls(reopened).Pause(run.Id);
        var recovered = fixture.Engine(backend, reopened);
        recovered.Recover(); await recovered.Run(run.Id, timeout.Token);
        run = reopened.Get(run.Id);
        paused = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
        Expect(run.State == "Paused" && paused.Review?.Stage == "Repairing" && paused.Review.RepairStepIndex == 1
            && paused.Review.Id == requestId && paused.Review.DecisionId == decisionId,
            "A completed review repair did not durably checkpoint, or recovery lost the original decision.");
        var oldMessage = backend.LastPlanningMessage!;
        var staleTask = Json.Read<TaskData>(Json.Write(paused));
        staleTask.State = "Running"; staleTask.AssignmentId = "new-assignment";
        staleTask.Review!.PendingMessage = oldMessage;
        Throws<WorkerContractException>(() => Contracts.ValidateReviewAssignment(oldMessage, run, staleTask, oldMessage.Role),
            "A reply from the old host/assignment was accepted after recovery.");
        new RunControls(reopened).Resume(run.Id);
        recovered.Recover(); await recovered.Run(run.Id, timeout.Token);
        Expect(reopened.Get(run.Id).State == "Completed" && backend.RepairCalls == 1 && backend.DecisionRoles.Count == 1,
            "Resume replayed a completed review decision/repair or failed to resume the original plan.");
    }

    private static async Task RejectReviewContracts()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var task = new TaskData
        {
            Definition = new("alpha", "Alpha", "Fixture", ["C1"], [], ["src\\alpha.txt"], ["tests\\alpha.expected"], ["acceptance"], true, null),
            State = "Running",
            Stage = "Review",
            AssignmentId = "assignment",
            Review = new()
            {
                Id = "review",
                EnvelopeId = "request",
                Revision = run.InitialCommit,
                ReturnStage = "Implementation",
                RepairRole = "Tester",
                Stage = "RepairPlanning"
            }
        };
        var message = new ReviewAssignment("review", "message", "assignment", "spec", run.InitialCommit, "Tester", "RepairPlanning");
        task.Review.PendingMessage = message; run.SpecificationId = "spec"; run.ApprovedSpecificationId = "spec";
        var plan = new ReviewRepairPlan(message, "Correct one faulty assertion", [new("fix-test", "Correct expected value", "Matches requirement", "Inspect assertion", ["tests\\alpha.expected"], [])]);
        Contracts.ValidateRepairPlan(plan, task, run.Request.Checks);
        Throws<PlanContractException>(() => Contracts.ValidateRepairPlan(plan with { Steps = [plan.Steps[0] with { WritePaths = ["src\\alpha.txt"] }] }, task, run.Request.Checks),
            "Tester review repair could edit production code.");
        Throws<WorkerContractException>(() => Contracts.ValidateReviewAssignment(message with { Revision = "stale" }, run, task, "Tester"), "Stale revision was accepted.");
        Throws<WorkerContractException>(() => Contracts.ValidateReviewAssignment(message with { SpecificationId = "old-spec" }, run, task, "Tester"), "Superseded approval was accepted.");
        Throws<WorkerContractException>(() => Contracts.ValidateReviewAssignment(message with { MessageId = "old-message" }, run, task, "Tester"), "Stale message ID was accepted.");
        Throws<InvalidDataException>(() => Contracts.ValidateChallenge(new(ReviewTarget.Test, "Dispute", ["C99"], ["tests\\alpha.expected"]), task.Definition), "Unassigned requirement was accepted.");
        Throws<InvalidDataException>(() => Contracts.ValidateChallenge(new(ReviewTarget.Test, "Dispute", ["C1"], ["src\\alpha.txt"]), task.Definition), "A test challenge targeted another role's production file.");
    }

    private static async Task CopilotReviewSmoke()
    {
        ExecutionPolicy.RequireNonElevated();
        using var fixture = await Fixture.Create();
        await File.WriteAllTextAsync(Path.Combine(fixture.Repository, "src", "alpha.txt"),
            "Diamond graph: root A. Directed edges in document order: A>B, A>C, C>D, B>D. FIFO BFS queues B before C, so B>D discovers D first.\n");
        await File.WriteAllTextAsync(Path.Combine(fixture.Repository, "tests", "alpha.expected"), "Expected tree edges: A>B, A>C, C>D\n");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "add", ".");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "commit", "-m", "Seed isolated BFS dispute fixture");
        var run = await fixture.NewRun();
        var spec = new Specification("BFS dispute", "Resolve deterministic traversal", "A diamond graph", [], "Text fixture",
            "From each root run BFS over out-edges in edge document order. The first edge reaching an unvisited node becomes a tree edge.",
            [new("C1", "The tree follows FIFO BFS discovery order.", "A queues B before C; D must be discovered through B>D.", ["acceptance"])], []);
        var artifact = fixture.Store.Save(run.Id, "review-smoke", "Specification", spec);
        var message = new ReviewAssignment("review", "message", "assignment", artifact.Id, run.InitialCommit, "Tester", "Reassess");
        fixture.Store.Change(run.Id, "SmokeReviewSeed", "Isolated read-only provider review fixture; never a product run approval.", r =>
        {
            r.State = "Running"; r.Phase = "Execution"; r.SpecificationId = artifact.Id; r.ApprovedSpecificationId = artifact.Id; r.ApprovedSpecificationHash = artifact.Sha256;
            r.Tasks = [new()
            {
                Definition = new("review-smoke", "BFS review", "Review a disputed assertion", ["C1"], [], ["src\\alpha.txt"], ["tests\\alpha.expected"], ["acceptance"], true, null),
                State = "Running", Stage = "Review", AssignmentId = "assignment", Workspace = fixture.Repository, BaseCommit = r.InitialCommit,
                Review = new() { Id = "review", EnvelopeId = "isolated-request", Revision = r.InitialCommit, ReturnStage = "Implementation", PendingMessage = message }
            }];
        });
        var backend = new CopilotBackend(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, fixture.Options);
        var runner = new AgentRunner(fixture.Store, backend, fixture.Options.Execution);
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(180));
        var decision = await runner.Invoke<ReviewDecision>(new(run.Id, "review-smoke", "assignment", "Tester", "review-decision", fixture.Repository,
            CrossRoleReviews.DecisionPrompt + "\nCurrent review message: " + Json.Write(message) + "\nApproved specification: " + Json.Write(spec)
            + "\nChallenge: inspect src\\alpha.txt and tests\\alpha.expected. The expected tree chooses C>D, contradicting the approved FIFO BFS rule. Reassess the test, not the product requirement.",
            [], ChangeBase: run.InitialCommit, Review: message), timeout.Token);
        Expect(decision.Disposition == ReviewDisposition.TestDefect && decision.EvidenceIds.Length > 0 && decision.Citations.Length > 0
            && (await GitWorkspace.Git(fixture.Processes, fixture.Repository, timeout.Token, "status", "--porcelain")).Length == 0,
            "Real Copilot could not independently reassess the BFS test contradiction read-only with bound requirement/evidence citations.");
        Console.WriteLine("PASS: real Copilot independently identified the BFS test defect using the durable review contract, exact requirement citations, and current-message read-only observations.");
    }

    private static async Task RejectReviewReplies()
    {
        foreach (var mode in new[] { "citation", "evidence", "message" })
        {
            using var fixture = await Fixture.Create();
            var run = await fixture.NewRun();
            var spec = fixture.Store.Save(run.Id, "alpha", "Specification", new Specification("Review fixture", "Problem", "Scope", [], "Findings", "Approach",
                [new("C1", "alpha must equal after", "Exact equality", ["acceptance"])], []));
            var message = new ReviewAssignment("review", "message", "assignment", spec.Id, run.InitialCommit, "Tester", "Reassess");
            fixture.Store.Change(run.Id, "FixtureReview", "Persist a pending read-only review message.", r =>
            {
                r.State = "Running"; r.Phase = "Execution"; r.SpecificationId = spec.Id; r.ApprovedSpecificationId = spec.Id; r.ApprovedSpecificationHash = spec.Sha256;
                r.Tasks = [new()
                {
                    Definition = new("alpha", "Alpha", "Fixture", ["C1"], [], ["src\\alpha.txt"], ["tests\\alpha.expected"], ["acceptance"], true, null),
                    State = "Running", Stage = "Review", AssignmentId = "assignment", Review = new()
                    { Id = "review", EnvelopeId = "request", Revision = r.InitialCommit, ReturnStage = "Implementation", PendingMessage = message }
                }];
            });
            fixture.Options.Execution.TransientRetries = 0;
            var runner = new AgentRunner(fixture.Store, new InvalidReviewReplyBackend(fixture.Store, mode), fixture.Options.Execution);
            await ThrowsAsync<WorkerProtocolException>(() => runner.Invoke<ReviewDecision>(new(run.Id, "alpha", "assignment", "Tester", "review-decision",
                fixture.Repository, "Inspect the current assertion.", [], ChangeBase: run.InitialCommit, Review: message), CancellationToken.None),
                "A fabricated requirement, stale message, or nonexistent evidence authorized review progress.");
            Expect(fixture.Store.Get(run.Id).Tasks.Single().Review!.PendingMessage == message,
                "Invalid review reply changed the durable pending message.");
        }
    }
}

internal static class ReviewTestEvidence
{
    public static string Observe(Store store, AgentCall call, string? path = null)
    {
        path ??= "src\\alpha.txt";
        var observation = File.ReadAllText(SafePaths.Resolve(call.Workspace, path));
        var artifact = store.Save(call.RunId, call.TaskId, "RepositoryEvidence", new { path, observation });
        store.Event(call.RunId, "ReviewFixtureObserved", path, call.TaskId, artifact.Id);
        return artifact.Id;
    }

    public static RequirementCitation[] Citations(Store store, AgentCall call)
    {
        var run = store.Get(call.RunId);
        var spec = Json.Read<Specification>(store.ReadArtifact(run.ApprovedSpecificationId!));
        return [new("C1", spec.Criteria.Single(c => c.Id == "C1").Outcome)];
    }

    public static ReviewDecision Decision(Store store, AgentCall call, ReviewDisposition disposition) =>
        new(call.Review!, disposition, "Compared the actual assertion with the approved exact-value requirement.", Citations(store, call), [Observe(store, call)]);
}

internal sealed class ReviewBackend(Fixture fixture, string mode) : IAgentBackend
{
    private readonly FakeBackend inner = new(fixture.Store);
    public int ImplementationPlans { get; private set; }
    public int TestPlans { get; private set; }
    public int RepairCalls { get; private set; }
    public int RepairVerificationCalls { get; private set; }
    public string? RepairRole { get; private set; }
    public bool SawBeforeFailure { get; private set; }
    public bool SawAfterPass { get; private set; }
    public bool SawOriginalDiff { get; private set; }
    public List<string> DecisionRoles { get; } = [];
    public ReviewAssignment? LastPlanningMessage { get; private set; }
    public Action? BeforeDecision { get; set; }
    public Action? BeforeRepair { get; set; }

    public async Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        if (call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) != true) return await inner.Run(call, token);
        var store = fixture.Store;
        if (call.Operation == "implementation-plan") ImplementationPlans++;
        if (call.Operation == "test-plan") TestPlans++;
        if (call.Review is not null)
        {
            if (call.WriteScopes.Length != 0 || call.AllowCommands) throw new InvalidOperationException("Review escaped read-only capabilities.");
            if (call.Operation == "review-decision")
            {
                DecisionRoles.Add(call.Role);
                var hook = BeforeDecision; BeforeDecision = null; hook?.Invoke();
                var disposition = mode == "implementation" ? ReviewDisposition.ImplementationDefect : mode == "no-progress" ? ReviewDisposition.NoDefect
                    : mode == "disagreed" && call.Role == "Tester" ? ReviewDisposition.NoDefect : ReviewDisposition.TestDefect;
                var decision = ReviewTestEvidence.Decision(store, call, disposition);
                if (mode == "stale-revision")
                {
                    await File.WriteAllTextAsync(Path.Combine(call.Workspace, "src", "alpha.txt"), "external revision\n", token);
                    await fixture.Git.Commit(store.Get(call.RunId), call.TaskId!, call.Workspace, "Simulate independent source change during review", token);
                }
                return new(Json.Write(decision), []);
            }
            if (call.Operation == "review-repair-plan")
            {
                LastPlanningMessage = call.Review;
                var task = store.Get(call.RunId).Tasks.Single(t => t.Definition.Id == call.TaskId);
                var before = store.ReadArtifact(task.Review!.BeforeEvidenceId!);
                SawBeforeFailure = before.Contains("\"exitCode\": 1");
                return new(Json.Write(new ReviewRepairPlan(call.Review, "Correct the owner defect without weakening assertions", [
                    new("correct-owner", "Correct only the faulty value", "Required value is after", "Read exact required value", [mode == "implementation" ? "src\\alpha.txt" : "tests\\alpha.expected"], [])])), []);
            }
            if (call.Operation == "review-verify-repair")
            {
                RepairVerificationCalls++;
                if (call.Role != (mode == "implementation" ? "Tester" : "SpecCreator")) throw new InvalidOperationException("Correction lacked independent requirement review.");
                var task = store.Get(call.RunId).Tasks.Single(t => t.Definition.Id == call.TaskId);
                SawAfterPass = store.ReadArtifact(task.Review!.AfterEvidenceId!).Contains("\"exitCode\": 0");
                var path = mode == "implementation" ? "src\\alpha.txt" : "tests\\alpha.expected";
                var diff = await new WorkerTools(store, fixture.Processes, fixture.Checks, fixture.Git, call, token).ReadDiff(path);
                SawOriginalDiff = diff.Contains("-wrong") && diff.Contains("+after");
                var evidence = ReviewTestEvidence.Observe(store, call, path);
                return new(Json.Write(new ReviewVerification(call.Review, mode != "rejected-repair", mode != "rejected-repair",
                    "Verified the corrected expectation against the unchanged approved requirement and original diff.", ReviewTestEvidence.Citations(store, call), [evidence])), []);
            }
        }
        if (call.Operation == "review-repair")
        {
            RepairCalls++; RepairRole = call.Role;
            var hook = BeforeRepair; BeforeRepair = null; hook?.Invoke();
            new WorkerTools(store, fixture.Processes, fixture.Checks, fixture.Git, call, token).WriteFile(mode == "implementation" ? "src\\alpha.txt" : "tests\\alpha.expected", "after\n");
            if (mode == "repair-transport") throw new IOException("Simulated transport loss after review repair wrote a file.");
            if (mode == "repair-exception") throw new InvalidOperationException("Simulated review worker failure after a write.");
            return new(FakeBackend.Result(store, call, true, "Corrected the expected value; no assertions or checks removed."), []);
        }

        if (mode == "implementation" && call.Operation == "implementation-plan")
        {
            var response = await inner.Run(call, token);
            var plan = Json.Read<ImplementerPlan>(response.FinalText);
            return new(Json.Write(plan with { ImplementationSteps = [plan.ImplementationSteps[0]] }), []);
        }
        if (mode == "implementation" && call.Operation == "implement")
        {
            await File.WriteAllTextAsync(Path.Combine(call.Workspace, "src", "alpha.txt"), "wrong\n", token);
            return new(FakeBackend.Result(store, call, true, "Deliberate production defect for independent review."), []);
        }
        if (mode != "implementation" && call.Operation == "prepare-tests" && call.WriteScopes.Length != 0)
        {
            await File.WriteAllTextAsync(Path.Combine(call.Workspace, "tests", "alpha.expected"), "wrong\n", token);
            return new(FakeBackend.Result(store, call, true, "Prepared the fixture assertion."), []);
        }
        if ((call.Execution?.CurrentStep.Id == "self-check" && (mode == "no-progress" || File.ReadAllText(Path.Combine(call.Workspace, "tests", "alpha.expected")).Trim() == "wrong"))
            || mode == "implementation" && call.Operation == "verify-step" && File.ReadAllText(Path.Combine(call.Workspace, "src", "alpha.txt")).Trim() == "wrong")
        {
            var path = mode == "implementation" ? "src\\alpha.txt" : "tests\\alpha.expected";
            var evidence = ReviewTestEvidence.Observe(store, call, path);
            var execution = call.Execution!;
            return new(Json.Write(new StepResult(execution.AssignmentId, execution.PlanId, execution.CurrentStep.Id, StepStatus.ReviewRequested,
                "The fixture assertion contradicts the approved expected value.", [evidence], null,
                new(mode == "implementation" ? ReviewTarget.Implementation : ReviewTarget.Test, "The observed value contradicts C1: it must be after, not wrong.", ["C1"], [path]))), []);
        }
        return await inner.Run(call, token);
    }
}

internal sealed class InvalidReviewReplyBackend(Store store, string mode) : IAgentBackend
{
    public Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        var result = ReviewTestEvidence.Decision(store, call, ReviewDisposition.TestDefect);
        result = mode switch
        {
            "citation" => result with { Citations = [new("C1", "This quotation does not exist in the approved specification")] },
            "evidence" => result with { EvidenceIds = ["fabricated-evidence"] },
            _ => result with { Context = result.Context with { MessageId = "stale-message" } }
        };
        return Task.FromResult(new WorkerResponse(Json.Write(result), []));
    }
}
