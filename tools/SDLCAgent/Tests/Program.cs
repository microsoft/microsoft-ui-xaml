using System.Diagnostics;
using System.Text.Json.Nodes;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Logging.Abstractions;
using SDLCAgent;
using Microsoft.Agents.AI;
using Microsoft.Extensions.AI;

namespace SDLCAgentTests;

public static partial class Program
{
    private static int assertions;

    public static async Task<int> Main(string[] args)
    {
        try
        {
            if (args.FirstOrDefault() == "--host-smoke") return await HostSmoke();
            if (args.FirstOrDefault() == "--copilot-smoke") return await CopilotSmoke();
            if (args.FirstOrDefault() == "--copilot-review-smoke") { await CopilotReviewSmoke(); return 0; }
            if (args.FirstOrDefault() == "--copilot-diagnosis-smoke") { await CopilotDiagnosisSmoke(); return 0; }
            if (args.FirstOrDefault() == "--copilot-obligations-smoke") { await CopilotObligationsSmoke(); return 0; }
            if (args.FirstOrDefault() == "--copilot-unblock-smoke") { await CopilotUnblockSmoke(); return 0; }
            if (args.FirstOrDefault() == "--viewer-smoke") { await ViewerSmoke(); return 0; }
            if (args.FirstOrDefault() == "--dependency-repairs")
            {
                await DependencyRepairProtocol(); Console.WriteLine($"PASS: {assertions} dependency-assignment repair assertions."); return 0;
            }
            if (args.FirstOrDefault() == "--workspace-gates")
            {
                await CompactGateWorkspaces(); await GateContextRemediation(); await RejectFlakyGate(); await RejectIntegration(); await AtomicBranchUpdate();
                Console.WriteLine($"PASS: {assertions} compact workspace and integration-gate assertions."); return 0;
            }
            if (args.FirstOrDefault() == "--dashboard-controls")
            {
                await UnblockInstructions(); await DashboardEvidence();
                Console.WriteLine($"PASS: {assertions} dashboard and orchestrator unblock assertions."); return 0;
            }
            if (args.FirstOrDefault() == "--obligations")
            {
                await TaskObligationProtocol();
                Console.WriteLine($"PASS: {assertions} scoped task-obligation assertions."); return 0;
            }
            if (args.FirstOrDefault() == "--diagnosis")
            {
                await DiagnosisProtocol();
                await WorkerProtocolRecovery();
                Console.WriteLine($"PASS: {assertions} durable diagnosis/recovery assertions."); return 0;
            }
            if (args.FirstOrDefault() == "--reviews")
            {
                await ReviewProtocol();
                Console.WriteLine($"PASS: {assertions} durable cross-role review assertions."); return 0;
            }
            if (args.FirstOrDefault() == "--contracts")
            {
                await CorrectAgentContracts(); RoleScopedPlans(); await CurrentStepContracts(); await BlockerRouting(); await InvalidPlansStopBeforeExecution();
                await FreshBuildEvidence(); await PauseDuringScaffolding(); await DashboardEvidence();
                Console.WriteLine($"PASS: {assertions} focused worker-plan, scaffold recovery, and real-compilation evidence assertions."); return 0;
            }
            if (args.FirstOrDefault() == "--check")
            {
                if (args.Contains("--build-only")) { Console.WriteLine("Build-only fixture prerequisite completed."); return 0; }
                var lengthLimit = Array.IndexOf(args, "--max-workspace-length");
                if (lengthLimit >= 0 && Environment.CurrentDirectory.Length > int.Parse(args[lengthLimit + 1], System.Globalization.CultureInfo.InvariantCulture))
                { Console.WriteLine("Deterministic legacy compiler workspace-path limit exceeded."); return 1; }
                var marker = Array.IndexOf(args, "--fail-once");
                if (marker >= 0 && !File.Exists(args[marker + 1]))
                { File.WriteAllText(args[marker + 1], "failed"); Console.WriteLine("Deliberate intermittent gate failure."); return 1; }
                if (args.Contains("--fail-candidate") && Path.GetFileName(Environment.CurrentDirectory).StartsWith("candidate-"))
                { Console.WriteLine("Deliberate integration-only regression."); return 1; }
                foreach (var test in Directory.EnumerateFiles("tests", "*.expected"))
                {
                    var name = Path.GetFileNameWithoutExtension(test);
                    if (File.ReadAllText(test).Trim() != File.ReadAllText(Path.Combine("src", name + ".txt")).Trim())
                    { Console.WriteLine($"FAIL: Assertion failed: {name} has not implemented the expected behavior."); return 1; }
                }
                Console.WriteLine("All fixture acceptance tests passed.");
                return 0;
            }
            if (args.FirstOrDefault() == "--sleep")
            { Console.WriteLine(Environment.ProcessId); await Task.Delay(TimeSpan.FromMinutes(5)); return 0; }
            if (args.FirstOrDefault() == "--spawn-child")
            {
                var start = new ProcessStartInfo(Environment.ProcessPath!) { UseShellExecute = false, CreateNoWindow = true };
                start.ArgumentList.Add("--sleep");
                using var child = Process.Start(start)!;
                Console.WriteLine(child.Id);
                return 0;
            }
            await PolicyAndPersistence();
            await DependencyRepairProtocol();
            await UnblockInstructions();
            await TaskObligationProtocol();
            await CorrectAgentContracts();
            RoleScopedPlans();
            await CurrentStepContracts();
            await BlockerRouting();
            await ReviewProtocol();
            await DiagnosisProtocol();
            await InvalidPlansStopBeforeExecution();
            await FreshBuildEvidence();
            await PauseDuringScaffolding();
            SeparateWorkerMessages();
            await WorkerProtocolRecovery();
            await DashboardEvidence();
            await PerItemWorktrees();
            await CompactGateWorkspaces();
            await GateContextRemediation();
            await DirtyWorkerHandoff();
            await EndToEnd();
            await PauseAndRestart();
            await RejectIntegration();
            await AtomicBranchUpdate();
            await RejectFlakyGate();
            await CommandTimeout();
            await HostRoutes();
            Console.WriteLine($"PASS: {assertions} assertions across policy, persistence, approval, paired TDD, repairs, parallelism, pause/restart, integration recovery, stale workers, gated rejection, atomic refs, process timeout, and live HTTP/viewer routing.");
            return 0;
        }
        catch (Exception ex) { Console.Error.WriteLine(ex); return 1; }
    }

    private static void Expect(bool condition, string message)
    {
        assertions++;
        if (!condition) throw new InvalidOperationException(message);
    }

    private static void Throws<T>(Action action, string message) where T : Exception
    {
        try { action(); }
        catch (T) { assertions++; return; }
        throw new InvalidOperationException(message);
    }

    private static async Task ThrowsAsync<T>(Func<Task> action, string message) where T : Exception
    {
        try { await action(); }
        catch (T) { assertions++; return; }
        throw new InvalidOperationException(message);
    }

    private static async Task PolicyAndPersistence()
    {
        using var fixture = await Fixture.Create();
        var root = JsonNode.Parse(fixture.Policy.Text)!;
        root["unexpected"] = true;
        Throws<InvalidDataException>(() => _ = new ExecutionPolicy(root.ToJsonString()), "Unknown policy key accepted.");
        root = JsonNode.Parse(fixture.Policy.Text)!;
        root["isolation"]!["requireNonElevated"] = false;
        Throws<InvalidDataException>(() => _ = new ExecutionPolicy(root.ToJsonString()), "Elevated worker policy accepted.");
        Throws<PolicyException>(() => SafePaths.Resolve(fixture.Repository, "..\\outside"), "Traversal allowed.");
        Throws<PolicyException>(() => SafePaths.Resolve(fixture.Repository, ".GIT\\config"), "Case-insensitive Git metadata access allowed.");
        Throws<PolicyException>(() => fixture.Policy.Require("git.allowPushRemoteBranches"), "Remote pushes allowed by default.");
        Throws<PolicyException>(() => Contracts.ValidateChecks([new("unsafe", "test", "validator.exe", ["--api-key=synthetic-secret"])]), "Inline check credentials accepted.");
        Expect(Redaction.Apply("Authorization: Bearer abcdef1234 ghp_12345678901234567890").Contains("[REDACTED]"), "Redaction missing.");

        var run = await fixture.NewRun();
        try { await fixture.NewRun(); throw new InvalidOperationException("Duplicate branch owner accepted."); }
        catch (PolicyException) { assertions++; }
        var engine = fixture.Engine(new FakeBackend(fixture.Store));
        await engine.Run(run.Id, CancellationToken.None);
        run = fixture.Store.Get(run.Id);
        Expect(run.State == "AwaitingApproval", "Specification must wait for human approval.");
        Expect(run.Tasks.Count == 0, "Implementation scheduled before approval.");
        var controls = new RunControls(fixture.Store);
        Throws<PolicyException>(() => controls.Approve(run.Id, new("wrong-version")), "Stale approval accepted.");
        controls.Pause(run.Id);
        var reopened = new Store(fixture.State);
        Expect(reopened.Get(run.Id).State == "Paused", "Pause did not persist.");
        new RunControls(reopened).Resume(run.Id);
        Expect(reopened.Get(run.Id).State == "AwaitingApproval", "Resume bypassed human approval.");
        var previous = run.SpecificationId;
        controls.Feedback(run.Id, new(Feedback: "Clarify measurable outcomes."));
        await engine.Run(run.Id, CancellationToken.None);
        run = fixture.Store.Get(run.Id);
        Expect(previous != run.SpecificationId, "Specification version was overwritten.");
        Throws<PolicyException>(() => controls.Approve(run.Id, new(previous)), "Old version approved after feedback.");
        Throws<PolicyException>(() => controls.Feedback(run.Id, new(previous, "Stale browser feedback")), "Feedback against a stale specification was accepted.");
        Expect(fixture.Store.Get(run.Id).SpecificationId == run.SpecificationId && fixture.Store.Get(run.Id).State == "AwaitingApproval", "Stale feedback changed the current review state.");
        controls.Approve(run.Id, new(run.SpecificationId));
        Expect(fixture.Store.Get(run.Id).ApprovedSpecificationHash is not null, "Approval not bound to content hash.");
        var artifact = fixture.Store.Save(run.Id, null, "IntegrityTest", new { value = "original" });
        File.AppendAllText(Path.Combine(fixture.State, "artifacts", artifact.Path), "tampered");
        Throws<InvalidDataException>(() => fixture.Store.ReadArtifact(artifact.Id), "Tampered artifact accepted.");
        var quota = new Store(Path.Combine(fixture.Root, "quota"), 1024);
        Throws<BudgetException>(() => quota.Save(run.Id, null, "Big", new string('x', 2048)), "Artifact quota ignored.");
        var secret = fixture.Store.Save(run.Id, null, "EscapedRedaction", new { text = "quoted \"password=synthetic-secret\" and a backslash \\" });
        Expect(JsonNode.Parse(fixture.Store.ReadArtifact(secret.Id))!["text"]!.GetValue<string>().Contains("[REDACTED]"), "Redaction broke escaped JSON evidence or retained a secret.");
        Throws<PolicyException>(() => controls.Cancel(run.Id), "Active run canceled without draining.");
        controls.Pause(run.Id);
        await engine.Run(run.Id, CancellationToken.None);
        controls.Cancel(run.Id);
        Expect(fixture.Store.Get(run.Id).State == "Canceled", "Cancellation not persisted.");
        Expect((await fixture.NewRun()).Id != run.Id, "Canceled run did not release branch ownership.");
    }

    private static async Task CorrectAgentContracts()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var backend = new MalformedPlanBackend();
        fixture.Store.Change(run.Id, "FixturePlanAssignment", "Assign read-only planning.", r => r.Tasks =
            [new() { Definition = new("plan", "Plan", "Fixture", ["C1"], [], ["src"], ["tests"], ["acceptance"], true, null) }]);
        var runner = new AgentRunner(fixture.Store, backend, fixture.Options.Execution);
        var plan = await runner.Invoke<TesterPlan>(new(run.Id, "plan", null, "Tester", "test-plan", fixture.Repository, "Return a detailed test plan.", []), CancellationToken.None);
        Expect(backend.Calls == 3 && plan.VerificationSteps.Length == 1, "Malformed JSON/incorrect test-plan scopes were not autonomously corrected.");
        Expect(backend.LastPrompt.Contains("Previous plan rejected"), "Role-plan correction did not include actionable feedback.");
        Expect(backend.RepairCalls == 1, "Semantic planning errors were treated as formatting-only repairs or execution was replayed.");
        Expect(fixture.Store.Events(run.Id).Any(e => e.Kind == "AgentContractRejected") && fixture.Store.Events(run.Id).Any(e => e.Kind == "PlanContractRejected"), "Distinct protocol/role-plan repair history missing.");
        var repair = new AgentCall(run.Id, null, null, "Tester", "repair-response", fixture.Repository, "Result formatting only", ["src"], true, ResultOnly: true);
        var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, repair, CancellationToken.None);
        Throws<PolicyException>(() => tools.ReadFile("src\\alpha.txt"), "Result-only repair obtained repository file access.");
        Throws<PolicyException>(() => tools.WriteFile("src\\alpha.txt", "unexpected write"), "Result-only repair retained write access despite the guard.");
    }

    private static void SeparateWorkerMessages()
    {
        var final = Json.Write(new StepResult("assignment", "plan", "step", StepStatus.Completed, "Build and tests pass; the temporary negative edit was reverted.", ["evidence"], null));
        var response = new AgentResponse
        {
            Messages = [new(ChatRole.Assistant, "Break Core temporarily (tests dir is outside my write scope)."),
                new(ChatRole.Tool, "Tool output is not the final result."),
                new(ChatRole.Assistant, "Non-zero confirmed. Reverting."), new(ChatRole.Assistant, final)]
        };
        Expect(response.Text.StartsWith("Break Core", StringComparison.Ordinal), "Recorded-failure fixture did not reproduce aggregated commentary.");
        var separated = WorkerResponse.FromMessages(response);
        Expect(Json.Read<StepResult>(separated.FinalText).Status == StepStatus.Completed && separated.Progress.Length == 2,
            "Worker commentary contaminated the final typed result or was lost from the audit transcript.");
        response.Messages = [new(ChatRole.Assistant, final), new(ChatRole.Assistant, "The final response is malformed, not the earlier JSON.")];
        Throws<System.Text.Json.JsonException>(() => Json.Read<StepResult>(WorkerResponse.FromMessages(response).FinalText), "An earlier successful JSON message was substituted for an invalid final response.");
        response.Messages = [new(ChatRole.Assistant, "Prose before the JSON\n" + final)];
        Throws<System.Text.Json.JsonException>(() => Json.Read<StepResult>(WorkerResponse.FromMessages(response).FinalText), "The adapter scraped arbitrary JSON out of an unstructured final message.");
        response.Messages = [new(ChatRole.Tool, final)];
        Throws<WorkerProtocolException>(() => WorkerResponse.FromMessages(response), "Tool output was accepted as an assistant result.");
        response.Messages = [new(ChatRole.Assistant, final), new(ChatRole.Tool, "Tool finished, but no final assistant result followed.")];
        Throws<WorkerProtocolException>(() => WorkerResponse.FromMessages(response), "An earlier assistant result was reused after an unfinished tool turn.");
        response.Messages = [new(ChatRole.Assistant, [new FunctionCallContent("call-1", "sdlc_run_check", new Dictionary<string, object?>())])];
        Throws<WorkerProtocolException>(() => WorkerResponse.FromMessages(response), "An unfinished tool call was accepted as a terminal worker result.");
    }

    private static void RoleScopedPlans()
    {
        var assignment = new WorkDefinition("plan", "Plan", "Fixture", ["C1"], [], ["src"], ["tests"], ["build", "test"], true, null);
        CheckCommand[] checks = [new("build", "build", "build.exe", []), new("test", "test", "test.exe", [])];
        var implementation = new ImplementerPlan(3, "API then behavior and self-check", [],
            [new("api", "Declare API", "Compiles", "Build", ["src\\Core"], ["build"])],
            [new("behavior", "Implement", "Works", "Tests", ["src\\Core"], []), new("self-check", "Self-check", "Passes", "Tests", [], ["test"])]);
        Contracts.ValidatePlan(implementation, "SpecImplementer", assignment, checks);
        var tests = new TesterPlan(3, "Tests then independent green verification", [],
            [new("tests", "Prepare tests", "Assertions compile", "Build", ["tests\\Core"], ["build"])],
            [new("verify", "Verify", "Passes after implementation", "Tests", ["test"])]);
        Contracts.ValidatePlan(tests, "Tester", assignment, checks);
        foreach (var invalid in new[]
        {
            tests with { ContractVersion = 2 },
            tests with { PreparationSteps = [tests.PreparationSteps[0] with { WritePaths = null! }] },
            tests with { PreparationSteps = [tests.PreparationSteps[0] with { WritePaths = ["src\\Core"] }] },
            tests with { PreparationSteps = [tests.PreparationSteps[0] with { Checks = ["test"] }] },
            tests with { VerificationSteps = [] }
        }) Throws<PlanContractException>(() => Contracts.ValidatePlan(invalid, "Tester", assignment, checks), "An unsafe/legacy/red-during-setup worker plan was accepted.");
        Throws<PlanContractException>(() => Contracts.ValidatePlan(implementation with { ScaffoldingSteps = [] }, "SpecImplementer", assignment, checks), "Implementation without an API scaffolding contract was accepted.");
        Throws<PlanContractException>(() => Contracts.ValidatePlan(implementation, "Tester", assignment, checks), "A tester was allowed to produce an implementer plan.");
        var unsafePlan = JsonNode.Parse(Json.Write(tests))!;
        unsafePlan["verificationSteps"]![0]!["writePaths"] = new JsonArray("tests");
        Throws<System.Text.Json.JsonException>(() => Json.Read<TesterPlan>(unsafePlan.ToJsonString()), "Independent verification accepted an editable step contract.");
        unsafePlan = JsonNode.Parse(Json.Write(implementation))!;
        unsafePlan["implementationSteps"]![0]!["phase"] = "verify";
        Throws<System.Text.Json.JsonException>(() => Json.Read<ImplementerPlan>(unsafePlan.ToJsonString()), "Workers could invent lifecycle phase labels.");
        var schema = Contracts.ResultSchema<TesterPlan>();
        Expect(schema["properties"]!["verificationSteps"] is not null && schema.ToJsonString().Contains("passChecks")
            && !schema.ToJsonString().Contains("\"phase\"") && schema["properties"]!["contractVersion"]!["const"]!.GetValue<int>() == 3,
            "The advertised schema differs from the executable role contract.");
    }

    private static async Task CurrentStepContracts()
    {
        using var fixture = await Fixture.Create();
        var backend = new StepClarificationBackend(fixture.Store);
        var engine = fixture.Engine(backend);
        var run = await fixture.NewRun();
        await engine.Run(run.Id, CancellationToken.None);
        new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(120));
        await engine.Run(run.Id, timeout.Token);
        run = fixture.Store.Get(run.Id);
        Expect(run.State == "Completed" && run.Tasks.All(t => t.Attempts == 1 && t.PreparationRepairs == 0),
            "A read-only inspection treated later write steps as a setup failure: " + run.Failure);
        Expect(backend.InspectionCalls == 2 && backend.ClarificationCalls == 2 && backend.PlanCalls == 4,
            "Step clarification replayed inspection/writes or regenerated role plans.");
        Expect(backend.SawScopedRequest && fixture.Store.Events(run.Id, kind: "StepContractRejected").Length == 2,
            "Current-step capabilities and structured rejection were not explicit.");
        var alpha = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
        var plan = Json.Read<ImplementerPlan>(fixture.Store.ReadArtifact(alpha.ImplementationPlanId!));
        var assignment = new StepAssignment("assignment", alpha.ImplementationPlanId!, "SpecImplementer", WorkerStage.Implementation,
            plan.ImplementationSteps[0], new(plan.ImplementationSteps[0].WritePaths, ["acceptance"], true));
        var completed = new StepResult("assignment", assignment.PlanId, assignment.CurrentStep.Id, StepStatus.Completed, "Observed current-step work", ["evidence"], null);
        Throws<StepContractException>(() => Contracts.ValidateStepResult(completed with { StepId = "later-step" }, assignment), "A later step's result was accepted as current-step completion.");
        Throws<StepContractException>(() => Contracts.ValidateStepResult(completed with { EvidenceIds = [] }, assignment), "Completion without evidence was accepted.");
        Throws<StepContractException>(() => Contracts.ValidateStepResult(completed with { Blocker = new(BlockerKind.SpecificationGap, "Unresolved decision", null, null) }, assignment), "Completion simultaneously reported a blocker.");
        Expect(!fixture.Store.HasStepEvidence(run.Id, alpha.Definition.Id, assignment, "not-an-artifact"), "Fabricated evidence was accepted.");
        var evidence = fixture.Store.Events(run.Id, alpha.Definition.Id, kind: "FixtureObserved").First();
        Expect(!fixture.Store.HasStepEvidence(run.Id, alpha.Definition.Id, assignment, evidence.DocumentId!), "Another assignment's evidence was accepted.");
        var executing = Json.Read<TaskData>(Json.Write(alpha));
        executing.State = "Running"; executing.Stage = "Implementation"; executing.AssignmentId = "assignment"; executing.StepIndex = 0;
        Contracts.ValidateStepAssignment(assignment, executing, Json.Write(plan), run.Request.Checks);
        Throws<WorkerContractException>(() => Contracts.ValidateStepAssignment(assignment with
        { CurrentStep = assignment.CurrentStep with { WritePaths = ["tests"] } }, executing, Json.Write(plan), run.Request.Checks), "A modified step broadened its persisted permissions.");
        Throws<WorkerContractException>(() => Contracts.ValidateStepAssignment(assignment, executing, Json.Write(plan with { ContractVersion = 2 }), run.Request.Checks), "A legacy contract version authorized execution.");
        executing.StepIndex++;
        Throws<WorkerContractException>(() => Contracts.ValidateStepAssignment(assignment, executing, Json.Write(plan), run.Request.Checks), "A completed step's stale assignment remained executable.");
    }

    private static async Task BlockerRouting()
    {
        foreach (var kind in new[] { BlockerKind.MissingScaffold, BlockerKind.TestSetupFailure, BlockerKind.BehaviorFailure, BlockerKind.SpecificationGap })
        {
            using var fixture = await Fixture.Create();
            var backend = new RoutedBlockerBackend(fixture.Store, kind);
            var engine = fixture.Engine(backend);
            var run = await fixture.NewRun();
            await engine.Run(run.Id, CancellationToken.None);
            new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
            var approved = fixture.Store.Get(run.Id).ApprovedSpecificationId;
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(120));
            await engine.Run(run.Id, timeout.Token);
            run = fixture.Store.Get(run.Id);
            var alpha = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
            if (kind == BlockerKind.SpecificationGap)
                Expect(run.State == "Blocked" && alpha.Stage == "Review" && alpha.Review?.Stage == "HumanRequired" && alpha.StepIndex == 0 && alpha.Attempts == 1
                    && run.ApprovedSpecificationId == approved && backend.ImplementerPlans == 1 && backend.TesterPlans == 1,
                    "A product clarification rewrote approval, replayed work, or entered implementation repair.");
            else if (kind == BlockerKind.BehaviorFailure)
                Expect(run.State == "Blocked" && alpha.Stage == "Implementation" && alpha.StepIndex == 1 && alpha.Attempts == 2
                    && alpha.PreparationRepairs == 0 && alpha.Failure!.Contains("Identical blocker") && backend.ImplementerPlans == 2 && backend.TesterPlans == 1,
                    "Changing evidence/diagnostic IDs disguised an identical failed check or reset its current-step cursor.");
            else
                Expect(run.State == "Completed" && alpha.Attempts == 2 && alpha.PreparationRepairs == 1
                    && backend.ImplementerPlans == (kind == BlockerKind.MissingScaffold ? 2 : 1)
                    && backend.TesterPlans == (kind == BlockerKind.TestSetupFailure ? 2 : 1)
                    && backend.ScaffoldingCalls == (kind == BlockerKind.MissingScaffold ? 2 : 1),
                    "A setup blocker failed to replan only its owning role/stage while retaining the counterpart's plan: " + run.Failure);
        }
    }

    private static async Task InvalidPlansStopBeforeExecution()
    {
        foreach (var setupFailure in new[] { false, true })
        {
            using var fixture = await Fixture.Create();
            var backend = new InvalidPlanBackend(fixture.Store) { FailScaffolding = setupFailure };
            var engine = fixture.Engine(backend);
            var run = await fixture.NewRun();
            await engine.Run(run.Id, CancellationToken.None);
            new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(90));
            await engine.Run(run.Id, timeout.Token);
            run = fixture.Store.Get(run.Id);
            var task = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
            if (setupFailure)
                Expect(run.State == "Blocked" && task.PreparationRepairs == 2 && task.Failure!.Contains("Identical blocker")
                    && task.Attempts < fixture.Options.Execution.MaxRepairAttempts && backend.TesterPreparationCalls == 0
                    && fixture.Store.Events(run.Id, task.Definition.Id, limit: 200).Any(e => e.Kind == "PreparationRepairRequired"),
                    "Setup failures were not routed to bounded implementer preparation repair before tester execution.");
            else
                Expect(run.State == "Blocked" && task.Attempts == 1 && task.Stage == "Plans" && backend.ExecutionCalls == 0
                    && fixture.Store.Events(run.Id, task.Definition.Id, limit: 200).Any(e => e.Kind == "WorkerContractBlocked"),
                    "An invalid tester plan reached execution or burned the whole-task repair loop.");
        }
    }

    private static async Task FreshBuildEvidence()
    {
        using var fixture = await Fixture.Create();
        var directory = Path.Combine(fixture.Repository, "compile-contract");
        Directory.CreateDirectory(directory);
        var project = Path.Combine(directory, "CompileContract.csproj");
        var program = Path.Combine(directory, "Program.cs");
        var api = Path.Combine(directory, "Api.cs");
        var config = Path.Combine(directory, "NuGet.config");
        await File.WriteAllTextAsync(Path.Combine(fixture.Repository, ".gitignore"), "**/bin/\n**/obj/\n");
        await File.WriteAllTextAsync(project, "<Project Sdk=\"Microsoft.NET.Sdk\"><PropertyGroup><TargetFramework>net10.0</TargetFramework><OutputType>Exe</OutputType></PropertyGroup></Project>");
        var source = "var api = new MissingApi(); System.Console.WriteLine(api.Value == 42 ? \"PASS\" : \"FAIL: missing behavior\"); return api.Value == 42 ? 0 : 1;";
        await File.WriteAllTextAsync(program, source);
        await File.WriteAllTextAsync(config, "<configuration><packageSources><clear /></packageSources></configuration>");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "add", ".");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "commit", "-m", "Add real compilation contract fixture");
        var dotnet = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "dotnet", "dotnet.exe");
        var run = await fixture.NewRun();
        run = fixture.Store.Change(run.Id, "FixtureChecks", "Authorize real compilation/build/test checks.", r => r.Request = r.Request with
        {
            Checks = [new("compile-restore", "restore", dotnet, ["restore", project, "--configfile", config, "--verbosity", "quiet"]),
                new("compile-build", "build", dotnet, ["build", project, "--no-restore", "--verbosity", "quiet", "/nologo"]),
                new("compile-test", "test", dotnet, ["run", "--project", project, "--no-build", "--no-restore"])]
        });
        var checks = new CheckRunner(fixture.Store, fixture.Processes, fixture.Options.Execution);
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(120));
        await ThrowsAsync<PreparationException>(() => checks.Run(run, null, fixture.Repository, "compile-test", timeout.Token), "Tests ran without any successful build evidence.");
        var names = new[] { "compile-test", "compile-build", "compile-restore" };
        var missing = await checks.Batch(run, null, fixture.Repository, names, timeout.Token);
        Expect(missing.Length == 2 && missing[^1].Kind == "build" && missing[^1].Result.Output.Contains("CS0246")
            && !fixture.Store.Events(run.Id).Any(e => e.Kind == "CommandStarted" && e.Message.StartsWith("compile-test:")),
            "Missing APIs did not stop the batch at compilation, or stale binaries were executed.");
        await File.WriteAllTextAsync(api, "public sealed class MissingApi { public int Value => 41; }");
        var red = await checks.Batch(run, null, fixture.Repository, names, timeout.Token);
        Expect(red.Length == 3 && red.Take(2).All(e => CheckRunner.Passed(e.Result)) && red[^1].Result.ExitCode == 1
            && red[^1].Result.Output.Contains("FAIL: missing behavior"), "Compiling scaffolding did not permit a genuine behavioral red.");
        var reopened = new CheckRunner(new Store(fixture.State), fixture.Processes, fixture.Options.Execution);
        Expect((await reopened.Run(run, null, fixture.Repository, "compile-test", timeout.Token)).ExitCode == 1,
            "Same-snapshot build evidence was not durable across host/check-runner recovery.");
        var goodApi = "public sealed class MissingApi { public int Value => 42; }";
        await File.WriteAllTextAsync(api, goodApi);
        await ThrowsAsync<PreparationException>(() => reopened.Run(run, null, fixture.Repository, "compile-test", timeout.Token), "Changed untracked source reused stale build evidence.");
        Expect(CheckRunner.Passed(await reopened.Run(run, null, fixture.Repository, "compile-build", timeout.Token))
            && CheckRunner.Passed(await reopened.Run(run, null, fixture.Repository, "compile-test", timeout.Token)), "Freshly built implemented behavior did not pass.");
        await File.WriteAllTextAsync(program, source + "\n// Changed tracked source\n");
        await ThrowsAsync<PreparationException>(() => reopened.Run(run, null, fixture.Repository, "compile-test", timeout.Token), "Changed tracked source reused stale build evidence.");
        await File.WriteAllTextAsync(program, source);
        await File.WriteAllTextAsync(api, "public sealed class MissingApi { public int Value => Undefined; }");
        Expect(!CheckRunner.Passed(await reopened.Run(run, null, fixture.Repository, "compile-build", timeout.Token)), "Broken implementation unexpectedly compiled.");
        await File.WriteAllTextAsync(api, goodApi);
        await ThrowsAsync<PreparationException>(() => reopened.Run(run, null, fixture.Repository, "compile-test", timeout.Token), "An older successful build receipt survived a later failed build.");
    }

    private static async Task PauseDuringScaffolding()
    {
        using var fixture = await Fixture.Create();
        var backend = new FakeBackend(fixture.Store);
        var engine = fixture.Engine(backend);
        var run = await fixture.NewRun();
        await engine.Run(run.Id, CancellationToken.None);
        var controls = new RunControls(fixture.Store);
        controls.Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
        backend.BeforeFirstScaffolding = () => controls.Pause(run.Id);
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(90));
        await engine.Run(run.Id, timeout.Token);
        run = fixture.Store.Get(run.Id);
        var saved = run.Tasks.FirstOrDefault(t => t.Stage == "Scaffold" && t.StepIndex == 1);
        Expect(run.State == "Paused" && saved is not null, "Scaffolding did not persist its completed step at the pause boundary.");
        var reopened = new Store(fixture.State);
        new RunControls(reopened).Resume(run.Id);
        var recovered = fixture.Engine(backend, reopened);
        recovered.Recover(); await recovered.Run(run.Id, timeout.Token);
        Expect(reopened.Get(run.Id).State == "Completed" && reopened.Events(run.Id, saved!.Definition.Id, limit: 200)
            .Count(e => e.Kind == "PlanStepCompleted" && e.StepId == "scaffold-1") == 1, "Scaffolding replayed on resume or prevented completion.");
        var steps = reopened.Events(run.Id, saved!.Definition.Id, limit: 200).Where(e => e.Kind == "PlanStepStarted").OrderBy(e => e.Id).ToArray();
        Expect(steps.Select(e => e.StepId).SequenceEqual(new[] { "scaffold-1", "inspect-test-api", "test-1", "step-1", "self-check", "test-2" }),
            "The durable phase order was not scaffold -> prepare -> implement -> verify.");
    }

    private static async Task DashboardEvidence()
    {
        Expect(TestOutput.Parse("51 tests, 51 passed, 0 failed") == new TestCounts(51, 51, 0), "Console test counts were not parsed.");
        Expect(TestOutput.Parse("Failed! - Failed: 2, Passed: 8, Skipped: 1, Total: 11, Duration: 1s") == new TestCounts(8, 11, 2, 1), "Dotnet test counts were not parsed.");
        Expect(TestOutput.Parse("51 tests, 50 passed, 0 failed") is null && TestOutput.Parse("All tests passed") is null
            && TestOutput.Parse("999999999999999999 tests, 0 passed, 0 failed") is null, "Missing, inconsistent, or overflowing counts were invented.");
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var dashboard = new Dashboard(fixture.Store, fixture.Processes, fixture.Git);
        var empty = await dashboard.Get(run.Id, CancellationToken.None);
        Expect(empty.Tests.Counts is null && empty.PlannedTestCount is null && empty.Branch.TotalCommits == 1, "Empty dashboard fabricated counts or lost branch history.");
        var integrationHistory = await dashboard.Commits(run.Id, null, 0, CancellationToken.None);
        Expect(integrationHistory.Commits.Single().Hash == run.InitialCommit && integrationHistory.NextOffset is null, "Managed-branch commit history is incorrect.");
        Expect(dashboard.ExecutionCommits(run.Id, long.MaxValue).Commits.Length == 0, "Execution commits included the inherited baseline.");
        var spec = fixture.Store.Save(run.Id, null, "Specification", new Specification("Dashboard fixture", "Problem", "Scope", [], "Findings", "Approach", [], [], 70));
        var implementation = fixture.Store.Save(run.Id, "alpha", "ImplementationPlan", new ImplementerPlan(3, "Implement", [], [], []));
        var testPlan = fixture.Store.Save(run.Id, "alpha", "TestPlan", new TesterPlan(3, "Test", [], [], []));
        var historical = fixture.Store.Save(run.Id, "alpha", "ImplementationPlan", new { contractVersion = 2, summary = "Previous attempt", risks = Array.Empty<string>(), steps = Array.Empty<object>() });
        var started = DateTimeOffset.UtcNow.AddMinutes(-5);
        WorkDefinition Definition(string name) => new(name, name, "Fixture objective", [], [], ["src"], ["tests"], ["acceptance"], true, null);
        fixture.Store.Change(run.Id, "PairAssigned", "Fixture assignment.", r =>
        {
            r.State = "Running"; r.Phase = "Execution"; r.SpecificationId = spec.Id;
            r.Tasks = [new() { Definition = Definition("alpha"), State = "Running", Attempts = 1, StartedAt = started,
                ImplementationPlanId = implementation.Id, TestPlanId = testPlan.Id },
                new() { Definition = Definition("beta") }, new() { Definition = Definition("done"), State = "Integrated" }];
        }, "alpha");
        var evidence = fixture.Store.Save(run.Id, "alpha", "CommandEvidence", new { output = "51 tests, 51 passed, 0 failed", exitCode = 0, timedOut = false, revision = run.InitialCommit });
        fixture.Store.Event(run.Id, "CommandFinished", "acceptance: exit 0; timeout=False.", "alpha", evidence.Id);
        for (var i = 0; i < 205; i++) fixture.Store.Event(run.Id, "FixtureNoise", "Later unrelated activity.");
        var active = await dashboard.Get(run.Id, CancellationToken.None);
        Expect(active.ItemCompleteCount == 1 && active.TotalItemCount == 3 && active.Backlog.Single().Id == "beta"
            && active.ActivePairs.Single().StartedAt == started, "Dashboard work counts/backlog/assignment time are incorrect.");
        Expect(active.Plans.Length == 3 && active.Plans.Take(2).All(p => p.Active) && active.Plans[^1].Plan.Id == historical.Id,
            "Active plans were not ordered before the complete historical plan list.");
        Expect(active.Tests.Counts == new TestCounts(51, 51, 0) && active.Tests.EvidenceId == evidence.Id && active.PlannedTestCount is null,
            "Older test evidence was missed, repeated runs were summed, or a draft target was treated as approved.");
        fixture.Store.Change(run.Id, "FixtureApproval", "Approve the explicit target and preserve a legacy assignment without a snapshot timestamp.", r =>
        {
            r.ApprovedSpecificationId = spec.Id; r.Tasks[0].StartedAt = null;
        });
        var legacy = await dashboard.Get(run.Id, CancellationToken.None);
        Expect(legacy.ActivePairs.Single().StartedAt is not null && legacy.PlannedTestCount == 70, "Legacy assignment event or approved planned-test target was not projected.");
        fixture.Store.Change(run.Id, "WorkerProtocolBlocked", "Specific worker failure.", r => { r.State = "Blocked"; r.Tasks[0].State = "Blocked"; }, "alpha");
        var blockedEvent = fixture.Store.Events(run.Id).First(e => e.Kind == "WorkerProtocolBlocked");
        fixture.Store.Event(run.Id, "RunBlocked", "Generic repair-budget message.");
        var blocked = await dashboard.Get(run.Id, CancellationToken.None);
        Expect(blocked.AttentionEvent?.Id == blockedEvent.Id && blocked.ActivePairs.Length == 0, "Blocker link lost the relevant worker failure or counted a stopped pair as running.");
        var unknown = fixture.Store.Save(run.Id, null, "CommandEvidence", new { output = "Custom test runner succeeded.", exitCode = 0, timedOut = false, revision = run.InitialCommit });
        fixture.Store.Event(run.Id, "CommandFinished", "acceptance: exit 0; timeout=False.", documentId: unknown.Id);
        Expect((await dashboard.Get(run.Id, CancellationToken.None)).Tests.Counts is null, "Unknown latest counts incorrectly reused older passing counts.");
        var truncated = fixture.Store.Save(run.Id, null, "CommandEvidence", new { output = "51 tests, 51 passed, 0 failed", outputTruncated = true, exitCode = 0, timedOut = false, revision = run.InitialCommit });
        fixture.Store.Event(run.Id, "CommandFinished", "acceptance: exit 0; timeout=False.", documentId: truncated.Id);
        Expect((await dashboard.Get(run.Id, CancellationToken.None)).Tests.Counts is null, "Truncated output was accepted as complete test counts.");
        var changed = fixture.Store.Save(run.Id, null, "CommandEvidence", new { output = "51 tests, 51 passed, 0 failed", sourceChanged = true, exitCode = 0, timedOut = false, revision = run.InitialCommit });
        fixture.Store.Event(run.Id, "CommandFinished", "acceptance: exit 0; timeout=False.", documentId: changed.Id);
        Expect((await dashboard.Get(run.Id, CancellationToken.None)).Tests.Counts is null, "Source-mutating tests were displayed as valid passing test counts.");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "checkout", "-b", "dashboard-worker");
        await File.WriteAllTextAsync(Path.Combine(fixture.Repository, "src", "alpha.txt"), "after\n");
        string workerHead;
        using (AuditScope.Use(new(Role: "SpecImplementer")))
            workerHead = await fixture.Git.Commit(fixture.Store.Get(run.Id), "alpha", fixture.Repository, "Dashboard worker commit", CancellationToken.None);
        fixture.Store.Change(run.Id, "FixtureBranch", "Assign the worker branch.", r => { r.Tasks[0].Branch = "dashboard-worker"; r.Tasks[0].BaseCommit = run.InitialCommit; });
        var workerHistory = await dashboard.Commits(run.Id, "alpha", 0, CancellationToken.None);
        Expect(workerHistory.Commits.Single() is { Role: "SpecImplementer", Subject: "Dashboard worker commit" }, "Worker history lost role attribution or included the baseline.");
        var executionHistory = dashboard.ExecutionCommits(run.Id, long.MaxValue);
        Expect(executionHistory.Commits.Single() is { Role: "SpecImplementer", TaskId: "alpha", Subject: "Dashboard worker commit" }
            && executionHistory.Commits[0].Hash == workerHead, "Execution history omitted an unintegrated worker commit or lost its role/task.");
        for (var i = 0; i < 53; i++) fixture.Store.Event(run.Id, "LogicalCommit", $"{i:D40}: Additional recorded commit {i}", "alpha");
        var firstCommits = dashboard.ExecutionCommits(run.Id, long.MaxValue);
        var restCommits = dashboard.ExecutionCommits(run.Id, firstCommits.NextBefore!.Value);
        Expect(firstCommits.Commits.Length == 50 && restCommits.Commits.Length == 4 && restCommits.NextBefore is null
            && !firstCommits.Commits.Select(c => c.EventId).Intersect(restCommits.Commits.Select(c => c.EventId)).Any(), "Execution commit pagination omitted or duplicated entries.");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "update-ref", "refs/heads/delivery", workerHead, run.InitialCommit);
        var advanced = await dashboard.Get(run.Id, CancellationToken.None);
        Expect(advanced.Branch.TotalCommits == 2 && advanced.Branch.AddedCommits == 1, "Managed-branch total/added commit counts were not refreshed.");
        var candidateTask = fixture.Store.Get(run.Id).Tasks[0]; candidateTask.HeadCommit = workerHead;
        string candidatePath;
        using (AuditScope.Use(new(Role: "Orchestrator")))
            candidatePath = await fixture.Git.Candidate(run, candidateTask, run.InitialCommit, CancellationToken.None);
        var candidateHead = await GitWorkspace.Head(fixture.Processes, candidatePath, CancellationToken.None);
        Expect(dashboard.ExecutionCommits(run.Id, long.MaxValue).Commits[0] is { Role: "Orchestrator", TaskId: "alpha" } candidateCommit
            && candidateCommit.Hash == candidateHead && candidateCommit.Subject.Contains("gates still required"),
            "Execution history lost a new integration candidate or mislabeled it as validated delivery.");
        using (var db = new SqliteConnection(new SqliteConnectionStringBuilder { DataSource = Path.Combine(fixture.State, "sdlc.db") }.ToString()))
        {
            db.Open(); using var command = db.CreateCommand();
            command.CommandText = """
                WITH RECURSIVE copies(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM copies WHERE n<1002)
                INSERT INTO artifacts SELECT lower(hex(randomblob(16))),run_id,task_id,kind,path,sha256,at
                FROM artifacts,copies WHERE id=$source
                """;
            command.Parameters.AddWithValue("$source", historical.Id); command.ExecuteNonQuery();
        }
        Expect((await dashboard.Get(run.Id, CancellationToken.None)).Plans.Length == 1005, "Plan history was truncated by the generic 1,000-artifact cap.");
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "branch", "-D", "delivery");
        Expect((await dashboard.Get(run.Id, CancellationToken.None)).Branch.Error is not null, "Unavailable branch metadata was silently replaced with success-shaped defaults.");
    }

    private static async Task WorkerProtocolRecovery()
    {
        foreach (var mode in new[] { "repaired", "exhausted", "transport" })
        {
            using var fixture = await Fixture.Create();
            fixture.Options.Execution.MaxPairs = 1;
            fixture.Options.Execution.TransientRetries = 1;
            var backend = new ProtocolBackend(fixture.Store) { RejectRepairs = mode == "exhausted", ThrowAfterExecution = mode == "transport" };
            var engine = fixture.Engine(backend);
            var run = await fixture.NewRun();
            await engine.Run(run.Id, CancellationToken.None);
            new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(90));
            await engine.Run(run.Id, timeout.Token);
            run = fixture.Store.Get(run.Id);
            Expect(backend.ExecutionCalls == 2 && run.Tasks.All(t => t.Attempts == 1), "Protocol repair reexecuted an implementation or consumed whole-task repair attempts.");
            if (mode != "transport") Expect(backend.RepairCalls >= 2 && fixture.Store.Artifacts(run.Id).Any(a => a.Kind == "WorkerResultRepair"), "Result-only repair evidence was not persisted.");
            if (mode != "repaired")
            {
                Expect(run.State == "Blocked" && run.Tasks.All(t => t.State == "Blocked" && t.Stage == "Implementation" && t.ImplementationPlanId is not null),
                    "Exhausted protocol repair reset implementation plans/cursors instead of preserving the blocked step.");
                Expect(run.Failure!.Contains(mode == "transport" ? "transport failed" : "result-only repairs"), "The run hid the protocol failure behind a generic task failure.");
                Expect(!fixture.Store.Events(run.Id, limit: 200).Any(e => e.Kind == "PairRepairRequired"), "A protocol failure entered implementation replanning.");
            }
            else Expect(run.State == "Completed", "A corrected result did not let the original implementation continue: " + run.Failure);
        }
    }

    private static async Task EndToEnd()
    {
        using var fixture = await Fixture.Create();
        var fake = new FakeBackend(fixture.Store) { FailFirstImplementation = true };
        var engine = fixture.Engine(fake);
        var run = await fixture.NewRun();
        await engine.Run(run.Id, CancellationToken.None);
        run = fixture.Store.Get(run.Id);
        new RunControls(fixture.Store).Approve(run.Id, new(run.SpecificationId));
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(90));
        await engine.Run(run.Id, timeout.Token);
        run = fixture.Store.Get(run.Id);
        Expect(run.State == "Completed", "Autopilot did not complete: " + run.Failure + "\n" + Json.Write(run.Tasks.Select(t => new { t.Definition.Id, t.Failure })));
        Expect(run.Tasks.Count == 2 && run.Tasks.All(t => t.State == "Integrated"), "Not all tasks integrated.");
        Expect(fake.MaxConcurrent >= 2, "Independent pairs never ran concurrently.");
        Expect(run.Tasks.Any(t => t.Attempts > 1), "Deliberate failure did not trigger autonomous repair.");
        var events = fixture.Store.Events(run.Id, limit: 200);
        Expect(events.Any(e => e.Kind == "TddRedConfirmed"), "Meaningful red phase not recorded.");
        Expect(events.Any(e => e.Kind == "PlanStepCompleted" && e.PlanId is not null && e.StepId is not null && e.AssignmentId is not null), "Structured plan-to-execution trace missing.");
        var artifacts = fixture.Store.Artifacts(run.Id);
        Expect(artifacts.Count(a => a.Kind == "ImplementationPlan") >= 2 && artifacts.Count(a => a.Kind == "TestPlan") >= 2, "Detailed plans not saved.");
        Expect(artifacts.Any(a => a.Kind == "GoalVerification"), "Final criterion evidence missing.");
        var head = await fixture.Git.BranchHead(fixture.Repository, "delivery", CancellationToken.None);
        Expect(head == run.FinalCommit && head != run.InitialCommit, "Final branch revision not verified.");
        var log = await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "log", "--format=%s", "delivery");
        Expect(log.Split('\n').Length >= 7, "Changes collapsed into one monolithic commit.");
        var last = run.Tasks.Single(t => t.CandidateCommit == head);
        fixture.Store.Change(run.Id, "SimulatedCrash", "Simulate branch update before receipt checkpoint.", r =>
        {
            r.State = "Running"; r.Phase = "Execution";
            r.Tasks.Single(t => t.Definition.Id == last.Definition.Id).State = "Integrating";
        });
        var recovered = fixture.Engine(fake);
        recovered.Recover();
        await recovered.Run(run.Id, timeout.Token);
        Expect(await fixture.Git.BranchHead(fixture.Repository, "delivery", timeout.Token) == head, "Integration recovery duplicated a merge.");
        Expect(fixture.Store.Events(run.Id, limit: 200).Any(e => e.Kind == "IntegrationRecovered"), "Integration recovery receipt missing.");
        fixture.Store.Change(run.Id, "AssignmentReplaced", "Set a newer assignment identity.", r =>
        {
            r.State = "Running"; r.Tasks[0].State = "Running"; r.Tasks[0].AssignmentId = "new-owner";
        });
        var task = fixture.Store.Get(run.Id).Tasks[0];
        var call = new AgentCall(run.Id, task.Definition.Id, "old-owner", "SpecImplementer", "late-result", task.Workspace!, "", task.Definition.CodePaths);
        var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, CancellationToken.None);
        Throws<PolicyException>(() => tools.WriteFile(task.Definition.CodePaths[0], "stale"), "Expired worker modified workspace.");
        call = call with { AssignmentId = "new-owner", ChangeBase = await GitWorkspace.Head(fixture.Processes, task.Workspace!, CancellationToken.None) };
        tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, CancellationToken.None);
        var source = task.Definition.CodePaths[0];
        tools.WriteFile(source, "uncommitted-change");
        Expect((await tools.ReadDiff(source)).Contains("uncommitted-change"), "Diff tool omitted working-tree changes.");
        await fixture.Git.Commit(fixture.Store.Get(run.Id), task.Definition.Id, task.Workspace!, "Fixture authorized source update", CancellationToken.None);
        var outside = " " + source;
        Directory.CreateDirectory(Path.GetDirectoryName(Path.Combine(task.Workspace!, outside))!);
        File.WriteAllText(Path.Combine(task.Workspace!, outside), "outside-scope");
        await fixture.Git.Commit(fixture.Store.Get(run.Id), task.Definition.Id, task.Workspace!, "Fixture committed out-of-scope change", CancellationToken.None);
        try { await tools.ValidateChanges(); throw new InvalidOperationException("Committed out-of-scope path with leading whitespace was accepted."); }
        catch (PolicyException) { assertions++; }
        call = call with { WriteScopes = ["src"], ChangeBase = await GitWorkspace.Head(fixture.Processes, task.Workspace!, CancellationToken.None) };
        tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, CancellationToken.None);
        Throws<PolicyException>(() => tools.WriteFile("src\\fixture.csproj", "<Project />"), "Dependency manifest changed without justification.");
        tools.WriteFile("src\\fixture.csproj", "<Project />", "Fixture manifest contains no new dependencies.");
        await tools.Commit("Add justified fixture manifest");
        call = call with { ChangeBase = await GitWorkspace.Head(fixture.Processes, task.Workspace!, CancellationToken.None) };
        tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, CancellationToken.None);
        Throws<PolicyException>(() => tools.DeleteFile("tests\\.gitkeep", "Attempt outside role scope"), "Out-of-scope deletion allowed.");
        Throws<PolicyException>(() => tools.DeleteFile("src", "Attempt recursive deletion"), "Directory deletion allowed.");
        tools.DeleteFile("src\\fixture.csproj", "Remove the fixture-only manifest; no dependencies are required.");
        await tools.ValidateChanges();
        Expect(!File.Exists(Path.Combine(task.Workspace!, "src", "fixture.csproj")), "Authorized individual-file deletion did not persist.");
    }

    private static async Task PerItemWorktrees()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        run = fixture.Store.Change(run.Id, "FixtureItems", "Create two independent work items.", r => r.Tasks = new[] { "alpha", "beta" }
            .Select(name => new TaskData { Definition = new(name, name, "Fixture", [], [], ["src"], ["tests"], ["acceptance"], true, null) }).ToList());
        var alpha = await fixture.Git.TaskWorkspace(run, run.Tasks[0], CancellationToken.None);
        var beta = await fixture.Git.TaskWorkspace(run, run.Tasks[1], CancellationToken.None);
        Expect(alpha.Path != beta.Path && alpha.Branch != beta.Branch && alpha.Path != fixture.Repository,
            "Independent work items shared a worktree/branch or modified the source checkout.");
        var reopened = new Store(fixture.State);
        var reused = await new GitWorkspace(reopened, fixture.Processes).TaskWorkspace(reopened.Get(run.Id), reopened.Get(run.Id).Tasks[0], CancellationToken.None);
        Expect(reused == alpha, "A retry/restart allocated a new worktree instead of reusing the item's worktree.");
        var file = Path.Combine(alpha.Path, "src", "alpha.txt");
        var original = await File.ReadAllTextAsync(file);
        await File.WriteAllTextAsync(file, "uncommitted implementation\n");
        Expect(await File.ReadAllTextAsync(Path.Combine(beta.Path, "src", "alpha.txt")) == original,
            "An item's uncommitted changes leaked into another item's worktree.");
        try { await fixture.Git.TaskWorkspace(reopened.Get(run.Id), reopened.Get(run.Id).Tasks[0], CancellationToken.None); throw new InvalidOperationException("Dirty item was handed to another worker."); }
        catch (WorkspaceStateException ex) { Expect(ex.Message.Contains("uncommitted changes") && await File.ReadAllTextAsync(file) == "uncommitted implementation\n", "Dirty handoff discarded edits or hid the actual blocker."); }
        await File.WriteAllTextAsync(file, original);
        var untracked = Path.Combine(alpha.Path, "untracked-fixture.txt");
        await File.WriteAllTextAsync(untracked, "pending");
        try { await fixture.Git.RequireCleanHandoff(alpha.Path, CancellationToken.None); throw new InvalidOperationException("Untracked files were handed to another role."); }
        catch (WorkspaceStateException) { Expect(File.Exists(untracked), "Untracked edits were silently deleted."); }
        File.Delete(untracked);
        var wrong = reopened.Get(run.Id).Tasks[0]; wrong.Workspace = beta.Path; wrong.Branch = beta.Branch;
        try { await fixture.Git.TaskWorkspace(reopened.Get(run.Id), wrong, CancellationToken.None); throw new InvalidOperationException("Another item's worktree was accepted."); }
        catch (WorkspaceStateException) { assertions++; }
        await GitWorkspace.Git(fixture.Processes, alpha.Path, CancellationToken.None, "checkout", "--detach");
        try { await fixture.Git.TaskWorkspace(reopened.Get(run.Id), reopened.Get(run.Id).Tasks[0], CancellationToken.None); throw new InvalidOperationException("Changed item branch was accepted."); }
        catch (WorkspaceStateException) { assertions++; }
        await GitWorkspace.Git(fixture.Processes, alpha.Path, CancellationToken.None, "checkout", alpha.Branch);
        Directory.Move(alpha.Path, alpha.Path + "-preserved");
        Directory.CreateDirectory(alpha.Path);
        await GitWorkspace.Git(fixture.Processes, alpha.Path, CancellationToken.None, "init", "-b", alpha.Branch);
        try { await fixture.Git.TaskWorkspace(reopened.Get(run.Id), reopened.Get(run.Id).Tasks[0], CancellationToken.None); throw new InvalidOperationException("An unrelated repository with the same branch name was accepted."); }
        catch (WorkspaceStateException) { assertions++; }
    }

    private static async Task CompactGateWorkspaces()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var taskId = "p0-p7-controls-" + new string('a', 40);
        run = fixture.Store.Change(run.Id, "FixtureItem", "Exercise gate workspace allocation for a long item identifier.", r =>
            r.Tasks = [new TaskData { Definition = new(taskId, taskId, "Fixture", [], [], ["src"], ["tests"], ["acceptance"], true, null) }]);
        var workspace = await fixture.Git.TaskWorkspace(run, run.Tasks[0], CancellationToken.None);
        await File.WriteAllTextAsync(Path.Combine(workspace.Path, "src", "alpha.txt"), "after\n");
        var head = await fixture.Git.Commit(run, taskId, workspace.Path, "Implement fixture change", CancellationToken.None);
        var task = fixture.Store.Get(run.Id).Tasks[0]; task.HeadCommit = head;
        var candidate = await fixture.Git.Candidate(run, task, workspace.Base, CancellationToken.None);
        var typicalWorker = Path.Combine(fixture.State, "workspaces", run.Id, "p0-p7-controls");
        Expect(candidate.Length <= typicalWorker.Length,
            "Gate allocation adds path depth beyond a normal worker, breaking legacy XAML intermediate assembly resolution.");
        var candidateHead = await GitWorkspace.Head(fixture.Processes, candidate, CancellationToken.None);
        var second = await fixture.Git.Candidate(run, task, workspace.Base, CancellationToken.None);
        Expect(second != candidate && (await File.ReadAllTextAsync(Path.Combine(candidate, "src", "alpha.txt"))).Trim() == "after",
            "Fresh candidates reused or changed a previous gate workspace.");
        Expect(await fixture.Git.BranchHead(fixture.Repository, run.Request.Branch, CancellationToken.None) == workspace.Base,
            "Workspace creation advanced the delivery branch before gates.");
        var final = await fixture.Git.FinalWorkspace(run, candidateHead, CancellationToken.None);
        Expect(final.Length <= typicalWorker.Length, "Final verification reintroduces excessive compiler path depth.");
        Expect(await fixture.Git.FinalWorkspace(run, candidateHead, CancellationToken.None) == final,
            "Restart lost the pinned final verification workspace.");
        var earlier = await fixture.Git.FinalWorkspace(run, workspace.Base, CancellationToken.None);
        Expect(earlier != final && await GitWorkspace.Head(fixture.Processes, earlier, CancellationToken.None) == workspace.Base
            && await GitWorkspace.Head(fixture.Processes, final, CancellationToken.None) == candidateHead,
            "Final workspaces shared mutable state between source revisions.");
        Expect((await GitWorkspace.Git(fixture.Processes, final, CancellationToken.None, "status", "--porcelain")).Length == 0,
            "Final workspace was not a clean detached source snapshot.");
        await ThrowsAsync<WorkspaceStateException>(() => fixture.Git.TaskProvenance(run, task, workspace.Base, CancellationToken.None),
            "Provenance accepted a different source revision.");
        await File.WriteAllTextAsync(Path.Combine(workspace.Path, "src", "alpha.txt"), "pending\n");
        await ThrowsAsync<WorkspaceStateException>(() => fixture.Git.TaskProvenance(run, task, head, CancellationToken.None),
            "Provenance certified a dirty worktree as a clean handoff.");
    }

    private static async Task DirtyWorkerHandoff()
    {
        using var fixture = await Fixture.Create();
        var backend = new DirtyFailureBackend(fixture.Store);
        var engine = fixture.Engine(backend);
        var run = await fixture.NewRun();
        await engine.Run(run.Id, CancellationToken.None);
        new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(90));
        await engine.Run(run.Id, timeout.Token);
        run = fixture.Store.Get(run.Id);
        var task = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
        Expect(run.State == "Blocked" && task.State == "Blocked" && task.Failure!.Contains("uncommitted changes")
            && backend.AlphaPrepareCalls == 1, "A failed implementer's dirty workspace was handed to the next tester or misattributed to its scope.");
        var blocked = fixture.Store.Events(run.Id, limit: 200).First(e => e.Kind == "WorkspaceBlocked");
        Expect((await new Dashboard(fixture.Store, fixture.Processes, fixture.Git).Get(run.Id, timeout.Token)).AttentionEvent?.Id == blocked.Id,
            "Workspace ownership/handoff blocker was not linked from the dashboard.");
        var planId = task.ImplementationPlanId;
        fixture.Store.Change(run.Id, "FixtureCursor", "Preserve a resumable implementation step after an uncertain worker result.", r =>
        {
            var pending = r.Tasks.Single(t => t.Definition.Id == task.Definition.Id);
            pending.Stage = "Implementation"; pending.StepIndex = 0;
        });
        new RunControls(fixture.Store).Resume(run.Id);
        await engine.Run(run.Id, timeout.Token);
        task = fixture.Store.Get(run.Id).Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
        Expect(backend.AlphaPrepareCalls == 1 && task.ImplementationPlanId == planId && task.Stage == "Implementation" && task.StepIndex == 0
            && (await File.ReadAllTextAsync(Path.Combine(task.Workspace!, "src", "alpha.txt"))).Contains("pending failed step"),
            "Dirty resume replayed worker actions, replaced saved plans, or discarded prior edits.");
        using (AuditScope.Use(new(Role: "SpecImplementer")))
            await fixture.Git.Commit(fixture.Store.Get(run.Id), task.Definition.Id, task.Workspace!, "Reconcile the failed implementation under its owning role", timeout.Token);
        new RunControls(fixture.Store).Resume(run.Id);
        await engine.Run(run.Id, timeout.Token);
        Expect(fixture.Store.Get(run.Id).State == "Completed", "A reconciled item could not resume normal work on its existing worktree.");
    }

    private static async Task PauseAndRestart()
    {
        using var fixture = await Fixture.Create();
        var fake = new FakeBackend(fixture.Store);
        var engine = fixture.Engine(fake);
        var run = await fixture.NewRun();
        await engine.Run(run.Id, CancellationToken.None);
        var controls = new RunControls(fixture.Store);
        controls.Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
        fake.BeforeFirstImplementation = () => controls.Pause(run.Id);
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(90));
        await engine.Run(run.Id, timeout.Token);
        run = fixture.Store.Get(run.Id);
        Expect(run.State == "Paused", "Pause did not drain to a durable boundary.");
        Expect(run.Tasks.Any(t => t.StepIndex > 0 || t.Stage == "Verify"), "Completed plan progress was not checkpointed.");
        var reopened = new Store(fixture.State);
        new RunControls(reopened).Resume(run.Id);
        var restarted = fixture.Engine(fake, reopened);
        restarted.Recover();
        await restarted.Run(run.Id, timeout.Token);
        Expect(reopened.Get(run.Id).State == "Completed", "Resume after process-state reconstruction failed: " + reopened.Get(run.Id).Failure);
    }

    private static async Task RejectIntegration()
    {
        using var fixture = await Fixture.Create(failCandidate: true);
        fixture.Options.Execution.MaxRepairAttempts = 2;
        var engine = fixture.Engine(new FakeBackend(fixture.Store));
        var run = await fixture.NewRun();
        await engine.Run(run.Id, CancellationToken.None);
        new RunControls(fixture.Store).Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(90));
        await engine.Run(run.Id, timeout.Token);
        run = fixture.Store.Get(run.Id);
        Expect(run.State == "Blocked", "Permanent integration failure should be blocked, never completed.");
        Expect(await fixture.Git.BranchHead(fixture.Repository, "delivery", timeout.Token) == run.InitialCommit, "Failed integration changed the owned branch.");
        Expect(fixture.Store.Events(run.Id, limit: 200).Any(e => e.Kind == "IntegrationRejected"), "Rejected candidate audit missing.");
    }

    private static async Task AtomicBranchUpdate()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var initial = run.InitialCommit;
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "commit", "--allow-empty", "-m", "Concurrent external update");
        var external = await GitWorkspace.Head(fixture.Processes, fixture.Repository, CancellationToken.None);
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "update-ref", "refs/heads/delivery", external, initial);
        try
        {
            await fixture.Git.Advance(run, initial, initial, CancellationToken.None);
            throw new InvalidOperationException("Stale-green candidate overwrote a newer integration head.");
        }
        catch (WorkflowException) { assertions++; }
        Expect(await fixture.Git.BranchHead(fixture.Repository, "delivery", CancellationToken.None) == external, "External head was lost.");
    }

    private static async Task CommandTimeout()
    {
        var result = await new ProcessRunner().Run(Environment.CurrentDirectory, Environment.ProcessPath!, ["--sleep"], 1, CancellationToken.None);
        Expect(result.TimedOut && result.ExitCode != 0, "Hung process was not terminated on timeout.");
        if (OperatingSystem.IsWindows())
        {
            result = await new ProcessRunner().Run(Environment.CurrentDirectory, Environment.ProcessPath!, ["--spawn-child"], 10, CancellationToken.None);
            var pid = int.Parse(result.StandardOutput!.Split('\n')[0]);
            Process? child = null;
            try
            {
                try { child = Process.GetProcessById(pid); }
                catch (ArgumentException) { }
                Expect(!result.TimedOut && result.ExitCode == 0 && (child is null || child.WaitForExit(3000)), "Command descendants survived parent exit or held stdout open.");
            }
            finally
            {
                if (child is not null && !child.HasExited) { child.Kill(entireProcessTree: true); await child.WaitForExitAsync(); }
                child?.Dispose();
            }
        }
        using var fixture = await Fixture.Create();
        var runner = Path.Combine(fixture.Repository, "relative-runner");
        Directory.CreateDirectory(runner);
        foreach (var file in Directory.EnumerateFiles(AppContext.BaseDirectory).Where(p => Path.GetExtension(p) is ".dll" or ".exe" or ".json"))
            File.Copy(file, Path.Combine(runner, Path.GetFileName(file)));
        result = await new ProcessRunner().Run(fixture.Repository, Path.Combine("relative-runner", Path.GetFileName(Environment.ProcessPath!)), ["--check"], 10, CancellationToken.None);
        Expect(result.ExitCode == 0 && !result.TimedOut, "Relative check executable did not resolve inside its assigned workspace: " + result.Output);
    }

    private static async Task ViewerSmoke()
    {
        var browser = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Microsoft", "Edge", "Application", "msedge.exe");
        if (!File.Exists(browser)) throw new FileNotFoundException("Viewer smoke requires an installed Microsoft Edge browser.", browser);
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        await fixture.Engine(new FakeBackend(fixture.Store)).Run(run.Id, CancellationToken.None);
        var original = fixture.Store.Get(run.Id);
        var spec = Json.Read<Specification>(fixture.Store.ReadArtifact(original.SpecificationId!));
        var questionSpec = fixture.Store.Save(run.Id, null, "Specification", spec with { Questions = ["Which label should be shown? <img src=x onerror=alert(1)>"] });
        fixture.Store.Change(run.Id, "FixtureQuestion", "Ask a review question before approval.", r => r.SpecificationId = questionSpec.Id);
        var listener = new System.Net.Sockets.TcpListener(System.Net.IPAddress.Loopback, 0);
        listener.Start(); fixture.Options.Host.Port = ((System.Net.IPEndPoint)listener.LocalEndpoint).Port; listener.Stop();
        var (app, token) = AgentHost.Build(fixture.State, fixture.Options, fixture.Policy, new FakeBackend(fixture.Store));
        await using (app)
        {
            await app.StartAsync();
            try
            {
                var profile = Path.Combine(fixture.Root, "browser-profile");
                var start = new ProcessStartInfo(browser) { UseShellExecute = false, CreateNoWindow = true };
                foreach (var argument in new[] { "--headless", "--disable-gpu", "--disable-background-networking", "--disable-sync", "--no-first-run", "--no-default-browser-check",
                    "--user-data-dir=" + profile, "--remote-debugging-port=0", $"http://127.0.0.1:{fixture.Options.Host.Port}/#token={token}" }) start.ArgumentList.Add(argument);
                using var process = Process.Start(start)!;
                using var lifetime = NativeProcessJob.Attach(process);
                using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(60));
                var portFile = Path.Combine(profile, "DevToolsActivePort");
                while (!File.Exists(portFile)) await Task.Delay(100, timeout.Token);
                var port = (await File.ReadAllLinesAsync(portFile, timeout.Token))[0];
                using var http = new HttpClient();
                var targets = JsonNode.Parse(await http.GetStringAsync($"http://127.0.0.1:{port}/json/list", timeout.Token))!.AsArray();
                var target = targets.First(t => t!["type"]!.GetValue<string>() == "page")!;
                using var socket = new System.Net.WebSockets.ClientWebSocket();
                await socket.ConnectAsync(new Uri(target["webSocketDebuggerUrl"]!.GetValue<string>()), timeout.Token);
                var sequence = 0;
                try
                {
                    var navigate = System.Text.Encoding.UTF8.GetBytes(Json.Write(new { id = 0, method = "Page.navigate", @params = new { url = $"http://127.0.0.1:{fixture.Options.Host.Port}/#token={token}" } }));
                    await socket.SendAsync(navigate, System.Net.WebSockets.WebSocketMessageType.Text, true, timeout.Token);
                    await Task.Delay(500, timeout.Token);
                    await Until("document.querySelector('#runs button')?.textContent || ''", "delivery - AwaitingApproval");
                    await Evaluate("document.querySelector('#runs button').click(); 'clicked'");
                    await Until("document.getElementById('overview').textContent", "Human input required");
                    Expect(await Evaluate("Array.from(document.querySelector('main').children).filter(e => ['overview','execution','logs','plans','approved-spec'].includes(e.id)).map(e => e.id).join(',')") == "overview,execution,logs,plans,approved-spec", "Dashboard sections are not in the requested order.");
                    await Until("document.getElementById('execution-commits').textContent", "No commits recorded");
                    Expect(await Evaluate("String(document.querySelector('#execution #branches') !== null)") == "true", "Branch history is not under Execution.");
                    Expect(await Evaluate("String(!document.getElementById('logs').open)") == "true", "Recent log panel was not initially collapsible.");
                    await Evaluate("document.getElementById('current-spec').click(); 'clicked'");
                    await Until("document.getElementById('detail').textContent", "Fixture change");
                    Expect(await Evaluate("String(document.querySelectorAll('#detail details').length > 0)") == "true", "Specification was not rendered as structured drill-down.");
                    fixture.Store.Change(run.Id, "RunBlocked", "Viewer fixture blocker.", r => { r.State = "Blocked"; r.Failure = "Viewer fixture blocker."; r.ReportedTokens = 1250000; });
                    for (var i = 0; i < 235; i++) fixture.Store.Event(run.Id, "ViewerFixtureLog", "Historical activity " + i);
                    await Until("document.getElementById('overview').textContent", "State: Blocked");
                    await Until("document.getElementById('overview').textContent", "1.250 M");
                    await Until("document.getElementById('unblock').hidden ? '' : document.getElementById('unblock').textContent", "Instructions for the orchestrator");
                    Expect(await Evaluate("String(document.getElementById('submit-unblock').disabled)") == "true", "Empty unblock instructions can be sent.");
                    await Evaluate("document.getElementById('unblock-instruction').value = 'Keep blocked until the fixture check is repaired <img src=x onerror=alert(1)>'; document.getElementById('unblock-instruction').dispatchEvent(new Event('input')); document.getElementById('refresh').click(); 'drafted'");
                    await Task.Delay(2300, timeout.Token);
                    Expect(await Evaluate("document.getElementById('unblock-instruction').value") == "Keep blocked until the fixture check is repaired <img src=x onerror=alert(1)>", "Live refresh discarded unblock instructions.");
                    Expect(await Evaluate("document.querySelector('#runs button').textContent") == "delivery - Blocked\n" + run.Id[..12], "Live sidebar status is stale.");
                    await Evaluate("document.getElementById('attention-log').click(); 'clicked'");
                    await Until("document.querySelector('#events details[id^=event-][open]')?.textContent || ''", "Viewer fixture blocker.");
                    await Evaluate("document.getElementById('expand-history').click(); 'clicked'");
                    await Until("document.getElementById('event-context').textContent", "complete structured history:");
                    var firstPage = fixture.Store.Events(run.Id, limit: 200);
                    var historyCount = firstPage.Length + fixture.Store.Events(run.Id, before: firstPage[^1].Id, limit: 200).Length;
                    Expect(int.Parse(await Evaluate("String(document.querySelectorAll('#events .log-event').length)")) == historyCount, "Complete history omitted records across pages or duplicated the focused blocker.");
                    await Evaluate("document.querySelector('#branches > details > summary').click(); 'clicked'");
                    await Until("document.getElementById('branches').textContent", "Fixture baseline");
                    fixture.Store.Event(run.Id, "LogicalCommit", new string('a', 40) + ": Viewer execution commit", "alpha");
                    await Until("document.getElementById('execution-commits').textContent", "Viewer execution commit");
                    await Evaluate("document.getElementById('submit-unblock').click(); 'sent'");
                    await Until("document.getElementById('action-status').textContent", "Instructions sent to the orchestrator");
                    await Until("document.getElementById('overview').textContent", "Orchestrator: Fixture asks to remain blocked");
                    Expect(fixture.Store.Events(run.Id).Any(e => e.Kind == "OperatorUnblockRequested")
                        && fixture.Store.Events(run.Id).Any(e => e.Kind == "OperatorUnblockDeclined"), "Browser instructions did not reach durable orchestrator assessment.");
                    Expect(await Evaluate("String(document.getElementById('unblock-instruction').value === '' && !document.querySelector('#unblock img'))") == "true", "Submitted unblock draft was not cleared or rendered as HTML.");
                    await Evaluate("document.getElementById('latest-events').click(); 'clicked'");
                    fixture.Store.Change(run.Id, "FixtureRestored", "Return to review.", r => { r.State = "AwaitingApproval"; r.Failure = null; });
                    await Until("document.getElementById('overview').textContent", "State: AwaitingApproval");
                    await Until("document.getElementById('spec-questions')?.textContent || ''", "Which label should be shown?");
                    Expect(await Evaluate("String(document.getElementById('approve-spec').disabled && !document.querySelector('#interaction img'))") == "true", "Unresolved questions allowed approval or question text was rendered as HTML.");
                    await Evaluate("Array.from(document.querySelectorAll('#overview button')).find(b => b.textContent === 'Pause run').click(); 'paused'");
                    await Until("document.getElementById('overview').textContent", "State: Paused");
                    await Evaluate("Array.from(document.querySelectorAll('#overview button')).find(b => b.textContent === 'Resume run').click(); 'resumed'");
                    await Until("document.getElementById('overview').textContent", "State: AwaitingApproval");
                    Expect(fixture.Store.Get(run.Id).ApprovedSpecificationId is null, "UI pause/resume bypassed approval.");
                    await Evaluate("document.getElementById('spec-answer-0').value = 'Use the node label'; document.getElementById('spec-answer-0').dispatchEvent(new Event('input')); document.getElementById('spec-feedback').value = 'Preserve all measurable criteria'; document.getElementById('spec-feedback').dispatchEvent(new Event('input')); document.getElementById('refresh').click(); 'drafted'");
                    await Task.Delay(2800, timeout.Token);
                    Expect(await Evaluate("document.getElementById('spec-answer-0').value + '|' + document.getElementById('spec-feedback').value") == "Use the node label|Preserve all measurable criteria", "Polling/manual refresh discarded an unsent review draft.");
                    await Evaluate("document.getElementById('submit-feedback').click(); 'sent'");
                    await Until("document.getElementById('action-status').textContent", "Feedback sent");
                    await Until("document.getElementById('interaction').hidden ? '' : document.getElementById('spec-questions').textContent", "No outstanding questions");
                    var revised = fixture.Store.Get(run.Id);
                    Expect(revised.SpecificationId != questionSpec.Id && revised.Feedback!.Contains("Use the node label") && revised.Feedback.Contains("Preserve all measurable criteria"), "Browser feedback did not persist answers and generate a revised specification.");
                    Expect(await Evaluate("String(document.getElementById('approve-spec').disabled)") == "true", "Approval did not require explicit review confirmation.");
                    await Evaluate("document.getElementById('review-spec').click(); document.getElementById('approve-spec').click(); 'approved'");
                    await Until("document.getElementById('action-status').textContent", "Approved the displayed specification");
                    Expect(fixture.Store.Get(run.Id).ApprovedSpecificationId == revised.SpecificationId && fixture.Store.Get(run.Id).ApprovedSpecificationHash is not null, "Browser approval was not bound to the displayed specification version and hash.");
                    await Until("document.getElementById('approved-content').textContent", revised.SpecificationId!);
                    Console.WriteLine("PASS: headless Edge authenticated, drilled into the spec, preserved a draft, submitted answers, received a revised spec, and explicitly approved its exact version.");
                }
                finally
                {
                    var message = System.Text.Encoding.UTF8.GetBytes(Json.Write(new { id = ++sequence, method = "Browser.close" }));
                    if (socket.State == System.Net.WebSockets.WebSocketState.Open)
                        await socket.SendAsync(message, System.Net.WebSockets.WebSocketMessageType.Text, true, CancellationToken.None);
                }

                async Task Until(string expression, string expected)
                {
                    var observed = "";
                    for (var attempt = 0; attempt < 100; attempt++)
                    {
                        observed = await Evaluate(expression);
                        if (observed.Contains(expected)) { assertions++; return; }
                        await Task.Delay(100, timeout.Token);
                    }
                    throw new InvalidOperationException("Viewer did not render " + expected + "; observed=" + observed
                        + "; UI error=" + await Evaluate("document.getElementById('error')?.textContent || 'none'"));
                }

                async Task<string> Evaluate(string expression)
                {
                    var id = ++sequence;
                    var message = System.Text.Encoding.UTF8.GetBytes(Json.Write(new { id, method = "Runtime.evaluate", @params = new { expression, returnByValue = true } }));
                    await socket.SendAsync(message, System.Net.WebSockets.WebSocketMessageType.Text, true, timeout.Token);
                    while (true)
                    {
                        using var data = new MemoryStream();
                        var buffer = new byte[8192];
                        System.Net.WebSockets.WebSocketReceiveResult part;
                        do
                        {
                            part = await socket.ReceiveAsync(new ArraySegment<byte>(buffer), timeout.Token);
                            if (part.MessageType == System.Net.WebSockets.WebSocketMessageType.Close) throw new IOException("Browser debugger closed before replying.");
                            data.Write(buffer, 0, part.Count);
                        } while (!part.EndOfMessage);
                        var response = JsonNode.Parse(data.ToArray())!;
                        if (response["id"]?.GetValue<int>() != id) continue;
                        if (response["error"] is not null || response["result"]?["exceptionDetails"] is not null)
                            throw new InvalidDataException("Viewer browser evaluation failed: " + response);
                        return response["result"]!["result"]!["value"]!.GetValue<string>();
                    }
                }
            }
            finally { await app.StopAsync(); }
        }
    }

    private static async Task GateContextRemediation()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var task = new TaskData { Definition = new("p0-p7-controls", "Controls", "Fixture", [], [], ["src"], ["tests"], ["acceptance"], true, null), HeadCommit = run.InitialCommit };
        var compact = await fixture.Git.Candidate(run, task, run.InitialCommit, CancellationToken.None);
        var legacy = Path.Combine(fixture.State, "workspaces", run.Id, "candidate-p0-p7-controls-" + Guid.NewGuid().ToString("N"));
        await GitWorkspace.Git(fixture.Processes, fixture.Repository, CancellationToken.None, "worktree", "add", "--detach", legacy, run.InitialCommit);
        run = fixture.Store.Change(run.Id, "FixtureCheck", "Measure deterministic path remediation without changing source.", r =>
            r.Request = r.Request with
            {
                Checks = [new("acceptance", "test", Environment.ProcessPath!,
                ["--check", "--max-workspace-length", (compact.Length + 10).ToString(System.Globalization.CultureInfo.InvariantCulture)])]
            });
        var failed = await fixture.Checks.Run(run, null, legacy, "acceptance", CancellationToken.None, gate: true);
        Expect(!CheckRunner.Passed(failed), "Long-workspace failure was not reproduced.");
        var tree = await GitWorkspace.Git(fixture.Processes, legacy, CancellationToken.None, "rev-parse", "HEAD^{tree}");
        using (var db = new SqliteConnection("Data Source=" + Path.Combine(fixture.State, "sdlc.db")))
        {
            db.Open(); using var command = db.CreateCommand();
            command.CommandText = "DELETE FROM failed_gate_contexts; INSERT INTO failed_gates VALUES ($run,'acceptance',$tree,$evidence)";
            command.Parameters.AddWithValue("$run", run.Id); command.Parameters.AddWithValue("$tree", tree); command.Parameters.AddWithValue("$evidence", failed.EvidenceId);
            command.ExecuteNonQuery();
        }
        var reopened = new Store(fixture.State);
        var checks = new CheckRunner(reopened, fixture.Processes, fixture.Options.Execution);
        var passed = await checks.Run(run, null, compact, "acceptance", CancellationToken.None, gate: true);
        Expect(CheckRunner.Passed(passed) && reopened.ReadArtifact(failed.EvidenceId!).Contains("path limit exceeded", StringComparison.Ordinal),
            "A demonstrated workspace-layout correction was treated as flaky success or erased historical evidence.");
        Expect(reopened.Events(run.Id).Any(e => e.Kind == "GateContextChanged")
            && await GitWorkspace.Git(fixture.Processes, compact, CancellationToken.None, "rev-parse", "HEAD^{tree}") == tree,
            "Remediation was not audited against unchanged source and a genuinely different execution context.");
        var second = await fixture.Git.Candidate(run, task, run.InitialCommit, CancellationToken.None);
        run = reopened.Change(run.Id, "FixtureFlakyCheck", "A fresh random candidate is not an environment correction.", r =>
            r.Request = r.Request with { Checks = [new("acceptance", "test", Environment.ProcessPath!, ["--check", "--fail-once", Path.Combine(fixture.Root, "context-fail-once")])] });
        Expect(!CheckRunner.Passed(await checks.Run(run, null, compact, "acceptance", CancellationToken.None, gate: true)), "First flaky gate did not fail.");
        await ThrowsAsync<PolicyException>(() => checks.Run(run, null, second, "acceptance", CancellationToken.None, gate: true),
            "Random candidate identifiers bypassed same-context flaky-gate rejection.");
    }

    private static async Task RejectFlakyGate()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        run = fixture.Store.Change(run.Id, "FixtureCheck", "Install a deterministic fail-once gate fixture.", r =>
            r.Request = r.Request with { Checks = [new("acceptance", "test", Environment.ProcessPath!, ["--check", "--fail-once", Path.Combine(fixture.Root, "failed-once")])] });
        Expect(!await fixture.Checks.All(run, null, fixture.Repository, ["acceptance"], CancellationToken.None), "Deliberate first gate failure was accepted.");
        var reopened = new Store(fixture.State);
        var checks = new CheckRunner(reopened, fixture.Processes, fixture.Options.Execution);
        try { await checks.All(run, null, fixture.Repository, ["acceptance"], CancellationToken.None); throw new InvalidOperationException("A flaky gate passed on retry without remediation."); }
        catch (PolicyException) { assertions++; }
        Expect(reopened.Events(run.Id).Any(e => e.Kind == "UnstableGateDetected"), "Durable flaky-gate evidence missing.");
    }

    private static async Task HostRoutes()
    {
        using var fixture = await Fixture.Create();
        var listener = new System.Net.Sockets.TcpListener(System.Net.IPAddress.Loopback, 0);
        listener.Start(); fixture.Options.Host.Port = ((System.Net.IPEndPoint)listener.LocalEndpoint).Port; listener.Stop();
        var (app, token) = AgentHost.Build(Path.Combine(fixture.Root, "http-only"), fixture.Options, fixture.Policy);
        await using (app)
        {
            await app.StartAsync();
            try
            {
                using var client = new HttpClient { BaseAddress = new Uri($"http://127.0.0.1:{fixture.Options.Host.Port}") };
                using var denied = await client.GetAsync("/api/runs");
                Expect(denied.StatusCode == System.Net.HttpStatusCode.Unauthorized, "Unauthenticated audit access accepted.");
                using var deniedControl = await client.PostAsync("/api/runs/does-not-exist/approve", new StringContent("{}", System.Text.Encoding.UTF8, "application/json"));
                Expect(deniedControl.StatusCode == System.Net.HttpStatusCode.Unauthorized, "Unauthenticated UI control access accepted.");
                client.DefaultRequestHeaders.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);
                Expect((await client.GetStringAsync("/api/health")).Contains("ready"), "HTTP host is not responsive.");
                var html = await client.GetStringAsync("/");
                Expect(html.Contains("Structured detail") && html.Contains("Execution and decision timeline"), "Viewer missing overview/drill-down.");
                Expect((await client.GetStringAsync("/app.js")).Contains("textContent"), "Viewer script missing or unsafe rendering contract absent.");
                using var invalid = await client.PostAsync("/api/runs", new StringContent(Json.Write(new RunRequest(fixture.Repository, "delivery", "invalid", [], true)),
                    System.Text.Encoding.UTF8, "application/json"));
                Expect(invalid.StatusCode == System.Net.HttpStatusCode.BadRequest, "Invalid checks not rejected through HTTP.");
                using var malformed = await client.PostAsync("/api/runs", new StringContent("{\"repository\":null}", System.Text.Encoding.UTF8, "application/json"));
                Expect(malformed.StatusCode == System.Net.HttpStatusCode.BadRequest, "Malformed nullable/missing HTTP request fields were not rejected.");
                using var missing = await client.GetAsync("/api/runs/does-not-exist");
                Expect(missing.StatusCode == System.Net.HttpStatusCode.NotFound, "Unknown run did not return 404.");
                using var forged = new HttpRequestMessage(HttpMethod.Get, "/api/health"); forged.Headers.Host = "untrusted.example";
                using var response = await client.SendAsync(forged);
                Expect(response.StatusCode == System.Net.HttpStatusCode.BadRequest, "Unexpected Host header accepted.");
            }
            finally { await app.StopAsync(); }
        }
    }

    private static async Task<int> HostSmoke()
    {
        ExecutionPolicy.RequireNonElevated();
        using var fixture = await Fixture.Create();
        var listener = new System.Net.Sockets.TcpListener(System.Net.IPAddress.Loopback, 0);
        listener.Start();
        fixture.Options.Host.Port = ((System.Net.IPEndPoint)listener.LocalEndpoint).Port;
        listener.Stop();
        var config = Path.Combine(fixture.Root, "host-settings.json");
        await File.WriteAllTextAsync(config, Json.Write(fixture.Options));
        var state = Path.Combine(fixture.Root, "host-state");
        var savedOutput = Console.Out;
        HostConnection? connection = null;
        Process? hostProcess = null;
        try
        {
            using var hidden = new StringWriter();
            Console.SetOut(hidden);
            try
            {
                await Cli.Run(["start", "--data", state, "--config", config,
                    "--policy", Path.Combine(AppContext.BaseDirectory, "execution-policy.json")]);
            }
            finally { Console.SetOut(savedOutput); }
            connection = Cli.Connection(state);
            hostProcess = Process.GetProcessById(connection.Pid);
            using var client = Cli.Client(connection);
            using var health = await client.GetAsync("/api/health");
            Expect(health.IsSuccessStatusCode, "Background host is not responsive after starting CLI exited.");
            using var untrusted = new HttpClient { BaseAddress = client.BaseAddress };
            using var denied = await untrusted.GetAsync("/api/runs");
            Expect(denied.StatusCode == System.Net.HttpStatusCode.Unauthorized, "Audit API permits unauthenticated access.");
            var html = await client.GetStringAsync("/");
            Expect(html.Contains("Structured detail") && html.Contains("Execution and decision timeline"), "HTML viewer missing drill-down sections.");
            Expect((await client.GetStringAsync("/app.js")).Contains("textContent"), "Viewer assets missing.");
            using var invalid = await client.PostAsync("/api/runs", new StringContent(Json.Write(new RunRequest(fixture.Repository, "delivery", "invalid", [], true)),
                System.Text.Encoding.UTF8, "application/json"));
            Expect(invalid.StatusCode == System.Net.HttpStatusCode.BadRequest, "Invalid run configuration accepted.");
            using var missing = await client.GetAsync("/api/runs/does-not-exist");
            Expect(missing.StatusCode == System.Net.HttpStatusCode.NotFound, "Unknown run did not return 404.");
            using var stop = await client.PostAsync("/api/stop", new StringContent("{}", System.Text.Encoding.UTF8, "application/json"));
            Expect(stop.IsSuccessStatusCode, "Explicit host stop failed.");
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(15));
            await hostProcess.WaitForExitAsync(timeout.Token);
            connection = null;
            Console.WriteLine("PASS: native non-elevated background start/health, independent lifetime, authenticated API, HTML/JS assets, validation, and graceful stop.");
            return 0;
        }
        finally
        {
            Console.SetOut(savedOutput);
            if (connection is not null)
            {
                if (hostProcess is not null && !hostProcess.HasExited)
                { hostProcess.Kill(entireProcessTree: true); await hostProcess.WaitForExitAsync(); }
            }
            hostProcess?.Dispose();
        }
    }

    private static async Task<int> CopilotSmoke()
    {
        ExecutionPolicy.RequireNonElevated();
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        fixture.Store.Change(run.Id, "SmokeStarted", "Read-only real-provider smoke test.", r => r.State = "Running");
        var backend = new CopilotBackend(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, fixture.Options);
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(360));
        var runner = new AgentRunner(fixture.Store, backend, fixture.Options.Execution);
        var result = await runner.Invoke<ProviderSmokeResult>(new(run.Id, null, null, "SpecCreator", "provider-smoke", fixture.Repository,
            "Provide a brief progress update before using sdlc_list_files then sdlc_read_file to inspect src\\alpha.txt. Finish with only {\"value\":\"the trimmed file contents\"}. Do not make changes.", []), timeout.Token);
        Expect(result.Value == "before", "Copilot did not return the inspected fixture value: " + Json.Write(result));
        Expect(fixture.Store.Events(run.Id).Any(e => e.Kind == "FileRead"), "Copilot did not exercise the scoped file tool.");
        Expect(fixture.Store.Events(run.Id).Any(e => e.Kind == "FilesListed"), "Copilot did not exercise the scoped list tool.");
        Expect(fixture.Store.Events(run.Id).Any(e => e.Kind == "ToolAuthorized" && e.Message.EndsWith(": sdlc_read_file", StringComparison.Ordinal)),
            "Copilot did not route through the namespaced scoped read tool.");
        var assignment = new WorkDefinition("plan-smoke", "Change alpha", "Change alpha.txt from before to after.", ["C1"], [],
            ["src\\alpha.txt"], ["tests\\alpha.expected"], ["acceptance"], true, null,
            [new("value", "C1", "alpha equals after", "Read source and assert exact equality", ["src\\alpha.txt"], ["acceptance"])]);
        fixture.Store.Change(run.Id, "SmokePlanAssignment", "Read-only provider planning against the versioned worker contract.", r => r.Tasks =
            [new() { Definition = assignment, State = "Running", AssignmentId = "plan-smoke" }]);
        var context = "Read src\\alpha.txt. Plan only; do not make changes. This fixture has no compiled API: use a read-only scaffolding inspection step.\nAssignment: " + Json.Write(assignment);
        var implementation = await runner.Invoke<ImplementerPlan>(new(run.Id, assignment.Id, "plan-smoke", "SpecImplementer", "implementation-plan", fixture.Repository,
            context + "\n" + Orchestrator.PlanPrompt("SpecImplementer", assignment, run.Request.Checks), []), timeout.Token);
        var testPlan = await runner.Invoke<TesterPlan>(new(run.Id, assignment.Id, "plan-smoke", "Tester", "test-plan", fixture.Repository,
            context + "\n" + Orchestrator.PlanPrompt("Tester", assignment, run.Request.Checks) + "\nImplementation plan: " + Json.Write(implementation), []), timeout.Token);
        Expect(implementation.ContractVersion == 3 && implementation.ScaffoldingSteps.Length > 0 && testPlan.VerificationSteps.Length > 0
            && (await GitWorkspace.Git(fixture.Processes, fixture.Repository, timeout.Token, "status", "--porcelain")).Length == 0,
            "Real Copilot worker plans did not satisfy role/phase/path contracts or read-only planning modified the fixture.");
        var inspection = new PlanStep("smoke-inspect-current-api", "Read src\\alpha.txt without editing tests", "Current source contains before", "Cite the observed current file contents", [], []);
        var executionPlan = testPlan with { PreparationSteps = [inspection, .. testPlan.PreparationSteps] };
        Contracts.ValidatePlan(executionPlan, "Tester", assignment, run.Request.Checks);
        var testArtifact = fixture.Store.Save(run.Id, assignment.Id, "TestPlan", executionPlan);
        var implementationArtifact = fixture.Store.Save(run.Id, assignment.Id, "ImplementationPlan", implementation);
        fixture.Store.Change(run.Id, "SmokeStepAssignment", "Execute one read-only inspection with later test-writing steps intentionally unassigned.", r =>
        {
            var task = r.Tasks.Single(); task.Stage = "Tests"; task.BaseCommit = r.InitialCommit;
            task.ImplementationPlanId = implementationArtifact.Id; task.TestPlanId = testArtifact.Id;
        });
        var execution = new StepAssignment("plan-smoke", testArtifact.Id, "Tester", WorkerStage.Tests, inspection, new([], [], false));
        var inspected = await runner.Invoke<StepResult>(new(run.Id, assignment.Id, "plan-smoke", "Tester", "prepare-tests", fixture.Repository,
            Orchestrator.StepPrompt(execution, "Inspect this API only. The saved plan has later writable steps; they are intentionally not assigned."), [], true,
            ChangeBase: run.InitialCommit, Execution: execution), timeout.Token);
        Expect(inspected.Status == StepStatus.Completed && inspected.EvidenceIds.Length > 0
            && (await GitWorkspace.Git(fixture.Processes, fixture.Repository, timeout.Token, "status", "--porcelain")).Length == 0,
            "Real Copilot failed the current read-only inspection because later writing steps lack tools.");
        Console.WriteLine("PASS: real GitHub Copilot-backed MAF tools, role-specific v3 plans, and evidence-backed current-step inspection with later writes unassigned.");
        return 0;
    }
}

internal sealed class Fixture : IDisposable
{
    public required string Root { get; init; }
    public string Repository => Path.Combine(Root, "repository");
    public string State => Path.Combine(Root, "state");
    public required Store Store { get; init; }
    public required ExecutionPolicy Policy { get; init; }
    public required CheckRunner Checks { get; init; }
    public required GitWorkspace Git { get; init; }
    public ProcessRunner Processes { get; } = new();
    public AgentOptions Options { get; } = new();
    private bool failCandidate;

    public static async Task<Fixture> Create(bool failCandidate = false)
    {
        var policy = new ExecutionPolicy(File.ReadAllText(Path.Combine(AppContext.BaseDirectory, "execution-policy.json")));
        var bootstrap = Path.Combine(Path.GetTempPath(), "SDLCAgent.Tests-" + Guid.NewGuid().ToString("N"));
        var store = new Store(Path.Combine(bootstrap, "state"));
        var processes = new ProcessRunner();
        var fixture = new Fixture
        {
            Root = bootstrap,
            Store = store,
            Policy = policy,
            Checks = new CheckRunner(store, processes, new ExecutionOptions()),
            Git = new GitWorkspace(store, processes),
            failCandidate = failCandidate
        };
        // All generated repositories and worktrees remain in this uniquely owned fixture directory.
        Directory.CreateDirectory(fixture.Repository);
        Directory.CreateDirectory(Path.Combine(fixture.Repository, "src"));
        Directory.CreateDirectory(Path.Combine(fixture.Repository, "tests"));
        foreach (var name in new[] { "alpha", "beta" }) await File.WriteAllTextAsync(Path.Combine(fixture.Repository, "src", name + ".txt"), "before\n");
        await File.WriteAllTextAsync(Path.Combine(fixture.Repository, "tests", ".gitkeep"), "");
        await GitWorkspace.Git(processes, fixture.Repository, CancellationToken.None, "init", "-b", "main");
        await GitWorkspace.Git(processes, fixture.Repository, CancellationToken.None, "add", ".");
        await GitWorkspace.Git(processes, fixture.Repository, CancellationToken.None, "commit", "-m", "Fixture baseline");
        await GitWorkspace.Git(processes, fixture.Repository, CancellationToken.None, "branch", "delivery");
        return fixture;
    }

    public Task<RunData> NewRun() => CreateRun();
    private async Task<RunData> CreateRun()
    {
        var args = failCandidate ? new[] { "--check", "--fail-candidate" } : new[] { "--check" };
        var request = new RunRequest(Repository, "delivery", "Change alpha and beta to after, verified by independent tests.",
            [new("acceptance", "test", Environment.ProcessPath!, args)], true);
        var baseline = await Git.ValidateRepository(request, CancellationToken.None);
        return Store.Create(request, baseline, Policy);
    }

    public Orchestrator Engine(IAgentBackend backend, Store? store = null)
    {
        store ??= Store;
        var checks = new CheckRunner(store, Processes, Options.Execution);
        var git = new GitWorkspace(store, Processes);
        return new Orchestrator(store, git, Processes, checks, new AgentRunner(store, backend, Options.Execution),
            Options, NullLogger<Orchestrator>.Instance);
    }

    public void Dispose()
    {
        SqliteConnection.ClearAllPools();
        foreach (var path in new[] { Root })
        {
            if (Path.GetDirectoryName(path) == Path.GetTempPath().TrimEnd(Path.DirectorySeparatorChar)
                && Path.GetFileName(path).StartsWith("SDLCAgent.Tests-", StringComparison.Ordinal))
            {
                foreach (var file in Directory.EnumerateFiles(path, "*", SearchOption.AllDirectories))
                {
                    try { File.SetAttributes(file, FileAttributes.Normal); }
                    catch (FileNotFoundException) { }
                }
                for (var attempt = 0; ; attempt++)
                {
                    try { Directory.Delete(path, recursive: true); break; }
                    catch (IOException ex) when (attempt < 9 && (ex.HResult & 0xFFFF) is 32 or 33)
                    {
                        Console.Error.WriteLine($"Waiting for fixture file handle to close before cleanup: {ex.Message}");
                        Thread.Sleep(100 * (attempt + 1));
                    }
                }
            }
        }
    }
}

internal sealed record ProviderSmokeResult(string Value);

internal sealed class FakeBackend(Store store) : IAgentBackend
{
    private int concurrent;
    private int firstImplementation;
    private int firstScaffolding;
    private int failureInjected;
    private int maximum;
    public int MaxConcurrent => maximum;
    public bool FailFirstImplementation { get; init; }
    public Action? BeforeFirstImplementation { get; set; }
    public Action? BeforeFirstScaffolding { get; set; }

    public async Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        var count = Interlocked.Increment(ref concurrent);
        int observed;
        do { observed = maximum; } while (count > observed && Interlocked.CompareExchange(ref maximum, count, observed) != observed);
        try
        {
            await Task.Delay(100, token);
            var name = call.TaskId?.EndsWith("alpha") == true ? "alpha" : "beta";
            var result = call.Operation switch
            {
                "specification" => Json.Write(new Specification("Fixture change", "before becomes after", "alpha and beta", [],
                    "src/*.txt and tests/*.expected", "Add expected values, change each independent source, verify together.",
                    [new("C1", "alpha is after", "Exact fixture assertion", ["acceptance"]), new("C2", "beta is after", "Exact fixture assertion", ["acceptance"])], [])),
                "decomposition" => Json.Write(new Decomposition(new[] { "alpha", "beta" }.Select((n, i) => new WorkDefinition(
                    n, "Change " + n, "Make " + n + " equal after", ["C" + (i + 1)], [], ["src\\" + n + ".txt"],
                    ["tests\\" + n + ".expected"], ["acceptance"], true, null,
                    [new("value", "C" + (i + 1), n + " equals after", "Exact production/test equality", ["src\\" + n + ".txt", "tests\\" + n + ".expected"], ["acceptance"])])).ToArray())),
                "verify-decomposition" => Json.Write(new CoverageAssessment(true, "Both independent obligations cover the complete approved fixture criteria within their owned paths.")),
                "assess-unblock" => Json.Write(new UnblockDecision(call.UnblockRequestId!, false, "Fixture asks to remain blocked until operator remediation.", "")),
                "implementation-plan" => Json.Write(new ImplementerPlan(3, "Bounded fixture implementation plan", [],
                    [new("scaffold-1", "Inspect existing API " + name, "Compiling interface exists", "Inspect fixture source contract", [], [])],
                    [new("step-1", "Implement " + name, "Expected fixture behavior", "Inspect assertion", ["src\\" + name + ".txt"], []),
                     new("self-check", "Read-only implementer self-check", "Acceptance passes", "Run acceptance", [], ["acceptance"])])),
                "test-plan" => Json.Write(new TesterPlan(3, "Independent fixture test plan", [],
                    [new("inspect-test-api", "Inspect current API", "Existing source file inspected", "Read source without writing later tests", [], []),
                     new("test-1", "Prepare " + name, "Behavior assertion exists", "Inspect test; orchestrator owns red", ["tests\\" + name + ".expected"], [])],
                    [new("test-2", "Verify " + name, "Measured behavior", "Pass after implementation", ["acceptance"])])),
                "scaffold" => Scaffold(call),
                "prepare-tests" => call.WriteScopes.Length == 0 ? Result(store, call, true, "Read-only inspection complete; later writing steps are not assigned.") : Write(call, "after\n"),
                "assess-red" => Json.Write(new RedAssessment(true, "The assertion reports the source still contains before, while the expected value is after.")),
                "implement" => Implement(call),
                "verify-step" => Result(store, call, File.ReadAllText(Path.Combine(call.Workspace, "src", name + ".txt")).Trim() == "after",
                    "Independently verified implementation occurred before the saved test-plan verification step."),
                "verify-task" => TaskResult(store, call),
                "verify-goal" => Json.Write(new Verification(true, ["C1", "C2"], "Both source values and independent tests verified together.")),
                "review-decision" => Json.Write(ReviewTestEvidence.Decision(store, call, ReviewDisposition.SpecificationChange)),
                _ => throw new InvalidOperationException(call.Operation)
            };
            return new(result, []);
        }

        finally { Interlocked.Decrement(ref concurrent); }
    }

    private string Implement(AgentCall call)
    {
        if (Interlocked.Increment(ref firstImplementation) == 1) BeforeFirstImplementation?.Invoke();
        if (FailFirstImplementation && Interlocked.Exchange(ref failureInjected, 1) == 0)
            return Result(store, call, false, "Deliberate first implementation failure; replan and repair.");
        return call.WriteScopes.Length == 0 ? Result(store, call, true, "Read-only self-check confirms implemented source.") : Write(call, "after\n");
    }

    internal static string TaskResult(Store store, AgentCall call)
    {
        var task = store.Get(call.RunId).Tasks.Single(t => t.Definition.Id == call.TaskId);
        var path = task.Definition.Obligations![0].Paths[0];
        var evidence = store.Save(call.RunId, call.TaskId, "RepositoryEvidence", new { path, observation = File.ReadAllText(SafePaths.Resolve(call.Workspace, path)) });
        store.Event(call.RunId, "TaskObligationObserved", path, call.TaskId, evidence.Id);
        return Json.Write(new TaskVerificationResult(call.Verification!, true,
            task.Definition.Obligations.Select(o => new ObligationResult(o.Id, ObligationStatus.Satisfied, "Exact fixture duty verified against current source and checks.", [evidence.Id])).ToArray(),
            "All assigned task obligations verified; this does not certify unassigned parent criteria."));
    }

    private string Scaffold(AgentCall call)
    {
        if (Interlocked.Increment(ref firstScaffolding) == 1) BeforeFirstScaffolding?.Invoke();
        return Result(store, call, true, "Existing text fixture API needs no scaffolding changes.");
    }

    private string Write(AgentCall call, string text)
    {
        var path = SafePaths.Resolve(call.Workspace, call.WriteScopes[0]);
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllText(path, text);
        return Result(store, call, true, "One logical change ready for independent verification.");
    }

    public static string Result(Store store, AgentCall call, bool completed, string summary, BlockerKind kind = BlockerKind.BehaviorFailure)
    {
        var execution = call.Execution!;
        string[] evidence = [];
        if (completed)
        {
            var path = call.WriteScopes.FirstOrDefault() ?? (call.TaskId!.EndsWith("-alpha", StringComparison.Ordinal) ? "src\\alpha.txt" : "src\\beta.txt");
            var artifact = store.Save(call.RunId, call.TaskId, "RepositoryEvidence", new { path, observation = File.ReadAllText(SafePaths.Resolve(call.Workspace, path)) });
            store.Event(call.RunId, "FixtureObserved", path, call.TaskId, artifact.Id);
            evidence = [artifact.Id];
        }
        return Json.Write(new StepResult(execution.AssignmentId, execution.PlanId, execution.CurrentStep.Id,
            completed ? StepStatus.Completed : StepStatus.Failed, summary, evidence, completed ? null : new(kind, summary, null, null)));
    }
}

internal sealed class InvalidPlanBackend(Store store) : IAgentBackend
{
    private readonly FakeBackend inner = new(store);
    public int ExecutionCalls { get; private set; }
    public int TesterPreparationCalls { get; private set; }
    public bool FailScaffolding { get; init; }
    public Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        if (call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true)
        {
            if (call.Operation == "test-plan" && !FailScaffolding)
            {
                if (call.WriteScopes.Length != 0 || call.AllowCommands || call.ResultOnly)
                    throw new InvalidOperationException("Semantic plan repair was not read-only planning.");
                return Task.FromResult(new WorkerResponse(Json.Write(new TesterPlan(3, "Invalid production edits in test plan", [],
                    [new("bad", "Create production stubs", "Compiles", "Build", ["src"], [])],
                    [new("verify", "Verify", "Passes", "Tests", [])])), []));
            }
            if (call.Operation is "scaffold" or "prepare-tests" or "implement") ExecutionCalls++;
            if (call.Operation == "prepare-tests") TesterPreparationCalls++;
            if (call.Operation == "scaffold" && FailScaffolding)
                return Task.FromResult(new WorkerResponse(FakeBackend.Result(store, call, false, "Missing API scaffolding; repair the implementer-owned setup plan.", BlockerKind.MissingScaffold), []));
        }
        return inner.Run(call, token);
    }
}

internal sealed class DirtyFailureBackend(Store store) : IAgentBackend
{
    private readonly FakeBackend inner = new(store);
    private int failed;
    public int AlphaPrepareCalls { get; private set; }

    public Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        if (call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true)
        {
            if (call.Operation == "prepare-tests" && call.WriteScopes.Length != 0) AlphaPrepareCalls++;
            if (call.Operation == "implement" && Interlocked.Exchange(ref failed, 1) == 0)
            {
                File.AppendAllText(Path.Combine(call.Workspace, "src", "alpha.txt"), "pending failed step\n");
                return Task.FromResult(new WorkerResponse(FakeBackend.Result(store, call, false, "Deliberate failed implementation with retained changes."), []));
            }
        }
        return inner.Run(call, token);
    }
}

internal sealed class MalformedPlanBackend : IAgentBackend
{
    public int Calls { get; private set; }
    public string LastPrompt { get; private set; } = "";
    public int RepairCalls { get; private set; }
    public Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        LastPrompt = call.Prompt;
        if (call.ResultOnly)
        {
            if (call.WriteScopes.Length != 0 || call.AllowCommands || call.Operation != "repair-response") throw new InvalidOperationException("Repair retained task-execution permissions.");
            RepairCalls++;
        }
        return Task.FromResult(new WorkerResponse(++Calls switch
        {
            1 => "not JSON",
            2 => Json.Write(new TesterPlan(3, "Wrong-scope test plan", [], [new("s1", "Prepare", "Measured outcome", "Check assertions", ["src"], [])],
                [new("s2", "Verify", "Measured outcome", "Check assertions", [])])),
            _ => Json.Write(new TesterPlan(3, "Corrected test plan", [], [new("s1", "Prepare", "Test exists", "Inspect test", ["tests"], [])],
                [new("s2", "Verify", "Measured outcome", "Check assertions", [])]))
        }, []));
    }
}

internal sealed class ProtocolBackend(Store store) : IAgentBackend
{
    private readonly FakeBackend inner = new(store);
    private readonly Dictionary<string, string> results = new();
    public bool RejectRepairs { get; init; }
    public bool ThrowAfterExecution { get; init; }
    public int ExecutionCalls { get; private set; }
    public int RepairCalls { get; private set; }

    public async Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        if (call.Operation == "diagnose-failure")
            return new(Json.Write(new DiagnosisDecision(call.Diagnosis!, RecoveryDisposition.Escalate,
                "Original protocol/transport completion is uncertain; source observation does not authorize replay.",
                "Reconcile the saved execution outcome under operator control.", "Confirm source and the original step's checks before further execution.",
                [ReviewTestEvidence.Observe(store, call)])), []);
        if (call.ResultOnly)
        {
            if (call.WriteScopes.Length != 0 || call.AllowCommands || call.Operation != "repair-response") throw new InvalidOperationException("Protocol repair can replay repository actions.");
            RepairCalls++;
            return new(RejectRepairs ? "Still not JSON" : results[call.TaskId!], []);
        }
        var result = await inner.Run(call, token);
        if (call.Operation != "implement" || call.WriteScopes.Length == 0) return result;
        ExecutionCalls++;
        if (ThrowAfterExecution) throw new IOException("Simulated transport loss after the implementation wrote its changes.");
        results[call.TaskId!] = result.FinalText;
        return new("Commentary in the final message needs formatting-only repair.\n" + result.FinalText, []);
    }
}

internal sealed class StepClarificationBackend(Store store) : IAgentBackend
{
    private readonly FakeBackend inner = new(store);
    private readonly System.Collections.Concurrent.ConcurrentDictionary<string, string> inspected = new();
    private int inspections, clarifications, plans;
    public int InspectionCalls => inspections;
    public int ClarificationCalls => clarifications;
    public int PlanCalls => plans;
    public bool SawScopedRequest { get; private set; }

    public async Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        if (call.ResultOnly)
        {
            if (call.WriteScopes.Length != 0 || call.AllowCommands || call.Operation != "repair-response") throw new InvalidOperationException("Step clarification retained execution tools.");
            Interlocked.Increment(ref clarifications);
            return new(inspected[call.TaskId!], []);
        }
        if (call.Operation is "implementation-plan" or "test-plan") Interlocked.Increment(ref plans);
        var response = await inner.Run(call, token);
        if (call.Operation != "prepare-tests" || call.WriteScopes.Length != 0) return response;
        Interlocked.Increment(ref inspections);
        var result = Json.Read<StepResult>(response.FinalText);
        inspected[call.TaskId!] = response.FinalText;
        SawScopedRequest = call.Execution!.CurrentStep.Id == "inspect-test-api" && !call.Execution.Capabilities.CanCommit
            && call.Execution.Capabilities.WritePaths.Length == 0 && !call.Prompt.Contains("Full saved plan") && !call.Prompt.Contains("\"id\": \"test-1\"");
        return new(Json.Write(result with
        {
            Status = StepStatus.Blocked,
            Summary = "Current API inspection succeeded, but later test-writing steps need write/commit tools.",
            Blocker = new(BlockerKind.CapabilityMismatch, "Later steps need write access", "tests\\" + (call.TaskId!.EndsWith("-alpha", StringComparison.Ordinal) ? "alpha" : "beta") + ".expected", null)
        }), []);
    }
}

internal sealed class RoutedBlockerBackend(Store store, BlockerKind kind) : IAgentBackend
{
    private readonly FakeBackend inner = new(store);
    private int failed;
    public int ImplementerPlans { get; private set; }
    public int TesterPlans { get; private set; }
    public int ScaffoldingCalls { get; private set; }

    public Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        if (call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true)
        {
            if (call.Operation == "implementation-plan") ImplementerPlans++;
            if (call.Operation == "test-plan") TesterPlans++;
            if (call.Operation == "scaffold") ScaffoldingCalls++;
            if (kind == BlockerKind.BehaviorFailure && call.Execution?.CurrentStep.Id == "self-check")
            {
                var result = Json.Read<StepResult>(FakeBackend.Result(store, call, false, "Check failure diagnostic " + Guid.NewGuid().ToString("N"), kind));
                return Task.FromResult(new WorkerResponse(Json.Write(result with { Blocker = result.Blocker! with { RequiredCheck = "acceptance" } }), []));
            }
            if (kind != BlockerKind.BehaviorFailure && call.Operation == "prepare-tests" && call.WriteScopes.Length == 0 && Interlocked.Exchange(ref failed, 1) == 0)
                return Task.FromResult(new WorkerResponse(FakeBackend.Result(store, call, false, "Deliberate routed blocker: " + kind, kind), []));
        }
        return inner.Run(call, token);
    }
}
