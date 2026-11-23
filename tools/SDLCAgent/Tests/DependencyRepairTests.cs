using SDLCAgent;

namespace SDLCAgentTests;

public static partial class Program
{
    private static async Task DependencyRepairProtocol()
    {
        var ownership = new WorkDefinition("owner", "Owner", "Fixture", ["C1"], [], ["src"], ["tests"], ["check"], true, null);
        Expect(Contracts.ResolveSetupOwner(new(BlockerKind.TestSetupFailure, "Restore failure", "src\\control.csproj", "check"), ownership).Kind == BlockerKind.MissingScaffold,
            "Production project setup was routed to tester-owned paths.");
        Expect(Contracts.ResolveSetupOwner(new(BlockerKind.MissingScaffold, "Test project failure", "tests\\test.csproj", "check"), ownership).Kind == BlockerKind.TestSetupFailure,
            "Test project setup was routed to implementer-owned paths.");
        var foreign = new StepBlocker(BlockerKind.TestSetupFailure, "Foreign dependency", "other\\project.csproj", "check");
        Expect(Contracts.ResolveSetupOwner(foreign, ownership) == foreign, "Unknown ownership silently broadened permissions.");
        var behavior = new StepBlocker(BlockerKind.BehaviorFailure, "Unmet requirement", "tests\\assertion.cs", "check");
        Expect(Contracts.ResolveSetupOwner(behavior, ownership) == behavior, "Setup ownership correction reinterpreted a behavioral assertion defect.");
        using (var fixture = await Fixture.Create())
        {
            var backend = new SetupOwnerBackend(fixture.Store);
            var engine = fixture.Engine(backend);
            var run = await fixture.NewRun();
            await engine.Run(run.Id, CancellationToken.None);
            new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
            await engine.Run(run.Id, CancellationToken.None);
            Expect(fixture.Store.Get(run.Id).State == "Completed" && backend.ImplementationPlans == 2 && backend.TestPlans == 1,
                "Misclassified production setup did not repair only its capable implementer owner.");
            Expect(fixture.Store.Events(run.Id, kind: "BlockerOwnershipCorrected").Length == 1,
                "Setup ownership correction was not explicitly audited.");
        }
        foreach (var mode in new[] { "accepted", "declined", "broad" })
        {
            using var fixture = await Fixture.Create();
            fixture.Options.Execution.TransientRetries = 1;
            var backend = new DependencyBackend(fixture, mode);
            var engine = fixture.Engine(backend);
            var run = await fixture.NewRun();
            await engine.Run(run.Id, CancellationToken.None);
            new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
            var approval = fixture.Store.Get(run.Id).ApprovedSpecificationHash;
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(180));
            await engine.Run(run.Id, timeout.Token);
            run = fixture.Store.Get(run.Id);
            var task = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
            var events = fixture.Store.Events(run.Id, limit: 200);
            Expect(backend.IncapableCalls == 1 && task.DependencyRepairRounds == 1
                && events.Any(e => e.Kind == "DependencyRepairRequested"), "Out-of-owner behavior failure retried the incapable implementer.");
            Expect(run.ApprovedSpecificationHash == approval, "Assignment repair changed approved requirements.");
            if (mode == "accepted")
            {
                Expect(run.State == "Completed" && run.FinalCommit is not null && task.Definition.CodePaths.SequenceEqual(new[] { "fixtures\\placeholder.txt", "src\\alpha.txt" }),
                    "Scoped assignment correction did not reach gated completion: " + run.Failure);
                Expect(backend.TestPlans == 1 && backend.RepairPlans == 1 && events.Any(e => e.Kind == "DependencyRepairAccepted"),
                    "Scope correction did not independently adjudicate or discarded the existing tester plan.");
                var archive = fixture.Store.Artifacts(run.Id).Single(a => a.Kind == "TaskAssignmentBeforeDependencyRepair");
                Expect(Json.Read<TaskData>(fixture.Store.ReadArtifact(archive.Id)).ImplementationPlanId is not null,
                    "Original plans/cursors were not archived before assignment revision.");
            }

            else
                Expect(run.State == "Blocked" && !task.Definition.CodePaths.Contains("src\\alpha.txt") && backend.RepairPlans == 0,
                    "Rejected/broad assignment proposal granted new scope or dispatched a repair.");
            Expect(!events.Any(e => e.Kind == "PairRepairRequired" && e.Message.Contains("Identical blocker")),
                "Incapable assignment consumed whole-task no-progress retries.");
        }
    }

    private sealed class SetupOwnerBackend(Store store) : IAgentBackend
    {
        private readonly FakeBackend inner = new(store);
        private bool failed;
        public int ImplementationPlans { get; private set; }
        public int TestPlans { get; private set; }

        public Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
        {
            if (call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true)
            {
                if (call.Operation == "implementation-plan") ImplementationPlans++;
                if (call.Operation == "test-plan") TestPlans++;
                if (call.Operation == "scaffold" && !failed)
                {
                    failed = true;
                    var execution = call.Execution!;
                    return Task.FromResult(new WorkerResponse(Json.Write(new StepResult(execution.AssignmentId, execution.PlanId,
                        execution.CurrentStep.Id, StepStatus.Failed, "Production setup not verified", [],
                        new(BlockerKind.TestSetupFailure, "Production setup failure mislabeled as tester setup", "src\\alpha.txt", null))), []));
                }
            }
            return inner.Run(call, token);
        }
    }

    private sealed class DependencyBackend(Fixture fixture, string mode) : IAgentBackend
    {
        private readonly FakeBackend inner = new(fixture.Store);
        public int IncapableCalls { get; private set; }
        public int RepairPlans { get; private set; }
        public int TestPlans { get; private set; }

        public async Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
        {
            if (call.Operation == "dependency-repair-plan")
            {
                Expect(call.Role == "Orchestrator" && call.WriteScopes.Length == 0 && !call.AllowCommands && !call.ResultOnly,
                    "Assignment planning gained execution capabilities or lost read-only correction tools.");
                var context = Json.Read<DependencyRepairContext>(fixture.Store.ReadArtifact(call.DependencyRepairRequestId!));
                var replacement = context.Definition with
                {
                    Objective = "Repair the approved alpha behavior while preserving existing tests.",
                    CodePaths = mode == "broad" ? ["src"] : context.Definition.CodePaths.Append(context.Blocker.RequiredPath!).ToArray()
                };
                return new(Json.Write(new DependencyRepairProposal(call.DependencyRepairRequestId!, replacement, "The original owner could inspect, but could not correct the approved production defect.")), []);
            }
            if (call.Operation == "review-dependency-repair")
            {
                Expect(call.Role == "SpecCreator" && call.WriteScopes.Length == 0 && !call.AllowCommands,
                    "Assignment revision lacked independent read-only specification review.");
                return new(Json.Write(new CoverageAssessment(mode != "declined", mode == "declined" ? "Correction not justified by evidence." : "Exact approved alpha behavior and measurements are retained; only the existing source file becomes owned.")), []);
            }
            if (call.Operation == "decomposition")
            {
                var original = Json.Read<Decomposition>((await inner.Run(call, token)).FinalText);
                return new(Json.Write(original with
                {
                    Tasks = original.Tasks.Select(t => t.Id == "alpha" ? t with
                    { CodePaths = ["fixtures\\placeholder.txt"], Obligations = t.Obligations!.Select(o => o with { Paths = ["tests\\alpha.expected"] }).ToArray() } : t).ToArray()
                }), []);
            }
            if (call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true)
            {
                var task = fixture.Store.Get(call.RunId).Tasks.Single(t => t.Definition.Id == call.TaskId);
                if (call.Operation == "test-plan") TestPlans++;
                if (call.Operation == "implementation-plan")
                {
                    if (task.Definition.CodePaths.Contains("src\\alpha.txt")) { RepairPlans++; return await inner.Run(call, token); }
                    return new(Json.Write(new ImplementerPlan(3, "Inspect existing behavior; production correction needs a capable owner", [],
                        [new("inspect", "Inspect current source", "API exists", "Read existing source", [], [])],
                        [new("report", "Measure alpha", "Approved value is after", "Inspect failure", [], [])])), []);
                }
                if (call.Operation == "implement" && !task.Definition.CodePaths.Contains("src\\alpha.txt"))
                {
                    IncapableCalls++;
                    var execution = call.Execution!;
                    return new(Json.Write(new StepResult(execution.AssignmentId, execution.PlanId, execution.CurrentStep.Id, StepStatus.Failed,
                        "Alpha is before, expected after; source is outside my assignment", [],
                        new(BlockerKind.BehaviorFailure, "Correct src/alpha.txt without weakening the existing assertion", "src\\alpha.txt", "acceptance"))), []);
                }
            }
            return await inner.Run(call, token);
        }
    }
}
