using SDLCAgent;

namespace SDLCAgentTests;

public static partial class Program
{
    private static async Task DiagnosisProtocol()
    {
        foreach (var mode in new[] { "known", "unfamiliar", "committed-preflight", "legacy-committed-preflight", "unknown", "policy", "repeat", "limit", "stale", "interrupted", "failed-gate", "diagnosis-exception", "pause" })
        {
            using var fixture = await Fixture.Create();
            var backend = new DiagnosisBackend(fixture, mode);
            var engine = fixture.Engine(backend);
            var run = await fixture.NewRun();
            if (mode is "committed-preflight" or "legacy-committed-preflight")
                fixture.Store.Change(run.Id, "FixtureBuildPrerequisite", "Require a source-matched build before fixture tests.", r =>
                    r.Request = r.Request with { Checks = [new("build", "build", Environment.ProcessPath!, ["--check", "--build-only"]), .. r.Request.Checks] });
            await engine.Run(run.Id, CancellationToken.None);
            var controls = new RunControls(fixture.Store);
            controls.Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
            var approved = fixture.Store.Get(run.Id).ApprovedSpecificationId;
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(150));
            await engine.Run(run.Id, timeout.Token);
            run = fixture.Store.Get(run.Id);
            var task = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
            if (mode == "pause")
            {
                Expect(run.State == "Paused" && task.Diagnosis?.DecisionId is not null && !task.Diagnosis.Dispatched,
                    "Pause lost the diagnosis decision or dispatched recovery past its boundary.");
                var decision = task.Diagnosis!.DecisionId;
                var reopened = new Store(fixture.State);
                new RunControls(reopened).Resume(run.Id);
                engine = fixture.Engine(backend, reopened);
                engine.Recover(); await engine.Run(run.Id, timeout.Token);
                run = reopened.Get(run.Id);
                task = run.Tasks.Single(t => t.Definition.Id.EndsWith("-alpha", StringComparison.Ordinal));
                Expect(backend.DiagnosisCalls == 1 && reopened.Artifacts(run.Id).Count(a => a.TaskId == task.Definition.Id && a.Kind == "DiagnosisDecision") == 1,
                    "Restart replayed a completed diagnosis decision: " + decision);
            }
            var events = fixture.Store.Events(run.Id, task.Definition.Id, limit: 200);
            Expect(task.Attempts == 1 && run.ApprovedSpecificationId == approved && backend.Plans == 2,
                "Diagnosis consumed implementation attempts, changed approval or regenerated original plans: " + mode + ": " + run.Failure);
            Expect(events.Any(e => e.Kind == "DiagnosisRequested") && events.Any(e => e.Kind == "DiagnosisMessageQueued"),
                "Diagnosis request/outbox was not persisted before dispatch.");
            if (mode is "known" or "unfamiliar" or "pause" or "committed-preflight" or "legacy-committed-preflight")
            {
                Expect(run.State == "Completed" && backend.RecoveryCalls == 1 && task.Diagnosis is null,
                    "A safe corrected call did not complete the original step and normal integration gates: " + run.Failure);
                Expect(events.Any(e => e.Kind == "RecoveryVerified") && events.Count(e => e.Kind == "PlanStepCompleted" && e.StepId == "test-1") == 1,
                    "Recovery was not independently verified or completed step replayed.");
                Expect(backend.DiagnosisCalls == (mode is "known" or "committed-preflight" or "legacy-committed-preflight" ? 0 : 1), "Known input rejection used model diagnosis, or unfamiliar failure bypassed it: " + mode);
                if (mode is "committed-preflight" or "legacy-committed-preflight") Expect(events.Any(e => e.Kind == "CommittedStepReconciled") && backend.ReadOnlyRecovery,
                    "Already committed work was replayed or prerequisite rejection retained unknown effects instead of read-only completion.");
            }
            else
            {
                Expect(run.State == "Blocked" && task.StepIndex == (mode == "failed-gate" ? 0 : 1)
                    && task.Stage == (mode == "failed-gate" ? "Implementation" : "Tests") && task.Diagnosis is not null,
                    "Unsafe recovery changed the original stage/cursor or failed to block: " + run.Failure);
                Expect(!events.Any(e => e.Kind == "RecoveryVerified") && !events.Any(e => e.Kind == "TaskIntegrated"),
                    "Unverified or unsafe recovery reached integration.");
                if (mode is "unknown" or "policy" or "stale" or "diagnosis-exception")
                    Expect(backend.RecoveryCalls == 0, "Unknown side effects, policy denial or stale advice dispatched recovery.");
                else Expect(backend.RecoveryCalls == (mode == "limit" ? FailureDiagnosis.MaxRounds : 1), "Recovery loop repeated execution without progress.");
                if (mode == "unknown")
                    Expect(File.ReadAllText(Path.Combine(task.Workspace!, "tests", "alpha.expected")) == "partial",
                        "Uncertain changes were discarded or committed by diagnosis.");
                if (mode == "failed-gate")
                    Expect(events.Any(e => e.Kind == "RecoveryRejected") && events.Any(e => e.Kind == "CommandFinished" && e.Message.Contains("exit 1")),
                        "Recovery completion bypassed the original failing command gate.");
                if (mode == "interrupted")
                {
                    controls.Resume(run.Id);
                    engine.Recover(); await engine.Run(run.Id, timeout.Token);
                    Expect(fixture.Store.Get(run.Id).State == "Blocked" && backend.RecoveryCalls == 1,
                        "Manual resume automatically replayed an interrupted recovery dispatch.");
                }
                if (mode == "diagnosis-exception")
                    Expect(run.Failure!.Contains("Tool rejected the current step") && run.Failure.Contains("Diagnosis service unavailable"),
                        "A diagnosis failure hid the original worker error.");
            }
        }
        await ToolFailureContract();
        await RejectDiagnosisContracts();
    }

    private static async Task ToolFailureContract()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        fixture.Store.Change(run.Id, "FixtureAssignment", "Assign scoped tool test.", r =>
        {
            r.State = "Running"; r.SpecificationId = "spec"; r.ApprovedSpecificationId = "spec";
            r.Tasks = [new() { Definition = new("alpha", "Alpha", "Fixture", ["C1"], [], ["src"], ["tests"], ["acceptance"], true, null), State = "Running", AssignmentId = "assignment" }];
        });
        var call = new AgentCall(run.Id, "alpha", "assignment", "SpecImplementer", "tool-test", fixture.Repository, "", ["src"]);
        using var scope = AuditScope.Use(new("assignment", "SpecImplementer", "plan", "step"));
        var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, CancellationToken.None);
        var failure = Json.Read<ToolFailureResult>(await tools.SafeWriteFile("src\\fixture.csproj", "<Project />"));
        Expect(!failure.Success && failure.Error.SideEffects == ToolSideEffects.None && failure.Error.Correction!.Contains("dependencyJustification")
            && !File.Exists(Path.Combine(fixture.Repository, "src", "fixture.csproj")), "Missing justification did not return precise evidence-backed pre-write rejection.");
        Expect(fixture.Store.GetArtifact(failure.EvidenceId).Kind == "ToolFailure" && fixture.Store.ReadArtifact(failure.EvidenceId).Contains("ToolInputException"),
            "Actual tool exception was not retained.");
        await tools.SafeWriteFile("src\\fixture.csproj", "<Project />", "Fixture-copy configuration only; dependencies unchanged.");
        Expect(File.ReadAllText(Path.Combine(fixture.Repository, "src", "fixture.csproj")) == "<Project />", "Corrected authorized manifest call did not succeed.");
        var secret = Json.Read<ToolFailureResult>(await tools.SafeWriteFile("tests\\outside.txt", "password=synthetic-secret", "password=synthetic-secret"));
        Expect(!fixture.Store.ReadArtifact(secret.EvidenceId).Contains("synthetic-secret") && secret.Error.PolicyDenied,
            "Tool failure leaked argument content or failed to classify policy denial.");
        var stopped = Json.Read<ToolFailureResult>(await tools.SafeWriteFile("src\\alpha.txt", "should-not-write"));
        Expect(!stopped.Success && File.ReadAllText(Path.Combine(fixture.Repository, "src", "alpha.txt")) == "before\n",
            "Policy denial did not stop subsequent mutations in the same worker session.");
        run = fixture.Store.Change(run.Id, "FixtureCheckPreflight", "Add a build prerequisite to the existing test check.", r =>
            r.Request = r.Request with { Checks = [new("build", "build", Environment.ProcessPath!, ["--check"]), .. r.Request.Checks] });
        var checkTools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call with { AllowCommands = true }, CancellationToken.None);
        var prerequisite = Json.Read<ToolFailureResult>(await checkTools.SafeRunCheck("acceptance"));
        Expect(prerequisite.Error.ExceptionType == "PreparationException" && prerequisite.Error.SideEffects == ToolSideEffects.None
            && !fixture.Store.Events(run.Id).Any(e => e.Kind == "CommandStarted"),
            "A pre-dispatch prerequisite rejection was falsely classified as an unknown command execution.");
        Expect(Json.Read<CommandResult>(await checkTools.SafeRunCheck("build")).ExitCode == 0,
            "A no-effects prerequisite rejection disabled the authorized build correction.");
    }

    private static async Task RejectDiagnosisContracts()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        run.State = "Running"; run.SpecificationId = "spec"; run.ApprovedSpecificationId = "spec"; run.ApprovedSpecificationHash = "hash";
        var context = new DiagnosisAssignment("diagnosis", "message", "assignment", "spec", "hash", "source", "plan", "step");
        var task = new TaskData
        {
            Definition = new("alpha", "Alpha", "Fixture", ["C1"], [], ["src"], ["tests"], ["acceptance"], true, null),
            State = "Running",
            Stage = "Tests",
            AssignmentId = "assignment",
            TestPlanId = "plan",
            Diagnosis = new()
            {
                Id = "diagnosis",
                RequestId = "request",
                PlanId = "plan",
                StepId = "step",
                Stage = "Tests",
                Revision = "head",
                Fingerprint = "source",
                SpecificationId = "spec",
                SpecificationHash = "hash",
                SafeToRetry = true,
                PendingMessage = context
            }
        };
        FailureDiagnosis.ValidateAssignment(context, run, task);
        foreach (var stale in new[] { context with { AssignmentId = "old" }, context with { MessageId = "old" }, context with { SpecificationId = "old" },
            context with { SpecificationHash = "old" }, context with { Fingerprint = "old" }, context with { StepId = "old" } })
            Throws<WorkerContractException>(() => FailureDiagnosis.ValidateAssignment(stale, run, task), "Stale diagnosis context was accepted.");
        task.Diagnosis.SafeToRetry = false;
        Throws<WorkerContractException>(() => FailureDiagnosis.ValidateDecision(context,
            new(context, RecoveryDisposition.RetryCurrentStep, "Guess", "Retry", "Pass checks", ["evidence"]), run, task), "Diagnosis authorized unknown write replay.");
        task.Diagnosis.SafeToRetry = true;
        task.Diagnosis.Dispatched = true;
        Throws<WorkerContractException>(() => FailureDiagnosis.ValidateAssignment(context, run, task), "Already-dispatched recovery accepted stale diagnosis advice.");
        task.Diagnosis.Dispatched = false;
        fixture.Store.Change(run.Id, "FixtureDiagnosis", "Persist read-only diagnosis capability test.", r =>
        {
            r.State = run.State; r.SpecificationId = run.SpecificationId; r.ApprovedSpecificationId = run.ApprovedSpecificationId;
            r.ApprovedSpecificationHash = run.ApprovedSpecificationHash; r.Tasks = [task];
        });
        var call = new AgentCall(run.Id, "alpha", "assignment", "Diagnosis", "diagnose-failure", fixture.Repository, "", ["src"], Diagnosis: context);
        var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, CancellationToken.None);
        Throws<PolicyException>(() => tools.WriteFile("src\\alpha.txt", "unexpected"), "Diagnosis acquired write capability.");
        call = call with { WriteScopes = [], AllowCommands = true };
        tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, CancellationToken.None);
        await ThrowsAsync<PolicyException>(() => tools.RunCheck("acceptance"), "Diagnosis acquired check/shell capability.");
    }

    private static async Task CopilotDiagnosisSmoke()
    {
        ExecutionPolicy.RequireNonElevated();
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var spec = fixture.Store.Save(run.Id, null, "Specification", new Specification("Diagnosis fixture", "Inspect the alpha source API", "Read-only inspection", [],
            "src\\alpha.txt is the existing source", "Inspect the existing API without changing it.", [new("C1", "Inspect alpha", "Read existing source", ["acceptance"])], []));
        var step = new PlanStep("inspect", "Inspect the existing alpha source API; do not create missing files.", "The existing alpha source is read", "Inspect its contents", [], []);
        var plan = fixture.Store.Save(run.Id, "alpha", "TestPlan", new TesterPlan(3, "Inspect existing alpha API", [], [step],
            [new("verify", "Verify implementation", "Acceptance passes", "Run acceptance", ["acceptance"])]));
        fixture.Store.Change(run.Id, "SmokeDiagnosisSeed", "Isolated diagnosis smoke; never product-run approval.", r =>
        {
            r.State = "Running"; r.Phase = "Execution"; r.SpecificationId = spec.Id; r.ApprovedSpecificationId = spec.Id; r.ApprovedSpecificationHash = spec.Sha256;
            r.Tasks = [new()
            {
                Definition = new("alpha", "Inspect alpha", "Inspect existing src\\alpha.txt", ["C1"], [], ["src"], ["tests"], ["acceptance"], true, null),
                State = "Running", Stage = "Tests", AssignmentId = "assignment", Workspace = fixture.Repository, BaseCommit = r.InitialCommit, TestPlanId = plan.Id
            }];
        });
        var execution = new StepAssignment("assignment", plan.Id, "Tester", WorkerStage.Tests, step, new([], [], false));
        var call = new AgentCall(run.Id, "alpha", "assignment", "Tester", "prepare-tests", fixture.Repository, "Inspect alpha", [], ChangeBase: run.InitialCommit, Execution: execution);
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(180));
        using (AuditScope.Use(new("assignment", "Tester", plan.Id, step.Id)))
        {
            var failure = Json.Read<ToolFailureResult>(await new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, timeout.Token)
                .SafeReadFile("src\\alphaa.txt"));
            Expect(failure.Error.SideEffects == ToolSideEffects.None && failure.Error.ExceptionType == "FileNotFoundException", "Provider smoke did not reproduce a real safe tool failure.");
        }
        var backend = new CopilotBackend(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, fixture.Options);
        var coordinator = new FailureDiagnosis(fixture.Store, fixture.Processes, fixture.Checks, new AgentRunner(fixture.Store, backend, fixture.Options.Execution));
        await coordinator.Open(call, "Reading the existing alpha API failed: source path not found.", timeout.Token);
        string instruction;
        try { instruction = await coordinator.Instruction(run.Id, "alpha", "assignment", timeout.Token); }
        catch
        {
            foreach (var response in fixture.Store.Artifacts(run.Id).Where(a => a.Kind == "AgentResponse").Reverse())
                Console.Error.WriteLine(fixture.Store.ReadArtifact(response.Id));
            throw;
        }
        Expect(instruction.Contains("alpha.txt", StringComparison.OrdinalIgnoreCase)
            && (await GitWorkspace.Git(fixture.Processes, fixture.Repository, timeout.Token, "status", "--porcelain")).Length == 0,
            "Real Copilot diagnosis failed to identify the correct existing source path read-only.");
        Console.WriteLine("PASS: real Copilot diagnosed an actual pre-read file-not-found failure through the production read-only diagnosis contract, with current-message evidence and no repository changes.");
    }
}

internal sealed class DiagnosisBackend(Fixture fixture, string mode) : IAgentBackend
{
    private readonly FakeBackend inner = new(fixture.Store);
    private bool injected;
    public int Plans { get; private set; }
    public int DiagnosisCalls { get; private set; }
    public int RecoveryCalls { get; private set; }
    public bool ReadOnlyRecovery { get; private set; }

    public async Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
    {
        if (mode is "committed-preflight" or "legacy-committed-preflight" && call.Operation == "decomposition")
        {
            var response = await inner.Run(call, token);
            return new(Json.Write(new Decomposition(Json.Read<Decomposition>(response.FinalText).Tasks.Select(t => t with { Checks = ["build", "acceptance"] }).ToArray())), []);
        }
        if (mode is "committed-preflight" or "legacy-committed-preflight" && call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true && call.Operation == "implement")
        {
            var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, token);
            if (call.RecoveryVerificationOnly)
            {
                RecoveryCalls++;
                ReadOnlyRecovery = call.WriteScopes.Length == 0 && !call.Execution!.Capabilities.CanCommit;
                await tools.SafeRunCheck("build"); await tools.SafeRunCheck("acceptance");
                return new(FakeBackend.Result(fixture.Store, call, true, "Independently checked the already committed step without writes."), []);
            }
            if (!injected && call.WriteScopes.Length != 0)
            {
                injected = true;
                await inner.Run(call, token);
                await tools.Commit("Commit the fixture implementation before its final check");
                ToolFailureResult failure;
                if (mode == "legacy-committed-preflight")
                {
                    var historical = new ToolFailure("sdlc_run_check", "{\"name\":\"acceptance\"}", "PreparationException",
                        "Test acceptance requires a successful build build in this workspace on the current source snapshot. Run build prerequisites again; stale binaries are not valid test evidence.",
                        ToolSideEffects.Unknown, null, false);
                    var artifact = fixture.Store.Save(call.RunId, call.TaskId, "ToolFailure", historical);
                    fixture.Store.Event(call.RunId, "ToolFailed", historical.Message, call.TaskId, artifact.Id);
                    var stopped = new ToolFailure("sdlc_run_check", "{\"name\":\"build\"}", "PolicyException",
                        "Further execution is disabled after uncertain effects, policy denial, or three rejected calls. Report the failure for orchestrator diagnosis.", ToolSideEffects.None, null, true);
                    var stoppedArtifact = fixture.Store.Save(call.RunId, call.TaskId, "ToolFailure", stopped);
                    fixture.Store.Event(call.RunId, "ToolFailed", stopped.Message, call.TaskId, stoppedArtifact.Id);
                    failure = new(false, artifact.Id, historical);
                }
                else failure = Json.Read<ToolFailureResult>(await tools.SafeRunCheck("acceptance"));
                return new(Json.Write(new StepResult(call.AssignmentId!, call.Execution!.PlanId, call.Execution.CurrentStep.Id,
                    StepStatus.Blocked, "Committed implementation; final test rejected before command dispatch.", [failure.EvidenceId],
                    new(BlockerKind.Infrastructure, "Build prerequisite is stale after commit.", null, "acceptance"))), []);
            }
        }
        if (call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true && call.Operation is "test-plan" or "implementation-plan") Plans++;
        if (mode == "failed-gate" && call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true && call.Operation == "implementation-plan")
        {
            var response = await inner.Run(call, token);
            var plan = Json.Read<ImplementerPlan>(response.FinalText);
            return new(Json.Write(plan with { ImplementationSteps = [plan.ImplementationSteps[0] with { Checks = ["acceptance"] }, plan.ImplementationSteps[1]] }), []);
        }
        if (call.Operation == "diagnose-failure")
        {
            DiagnosisCalls++;
            if (mode == "diagnosis-exception") throw new InvalidOperationException("Diagnosis service unavailable");
            if (call.WriteScopes.Length != 0 || call.AllowCommands || call.Role != "Diagnosis") throw new InvalidOperationException("Diagnosis gained execution capabilities.");
            var tools = new WorkerTools(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, call, token);
            var read = tools.ReadFile("src\\alpha.txt");
            var id = read.Split('\n')[0]["EvidenceId: ".Length..];
            var decision = new DiagnosisDecision(call.Diagnosis!, mode is "unknown" or "policy" ? RecoveryDisposition.Escalate : RecoveryDisposition.RetryCurrentStep,
                mode == "unknown" ? "Unknown partial write; operator reconciliation required." : "Read path was incorrect; source inspection confirms the current API.",
                "Use the authorized test path and complete the original step.", "Run the original step checks; retain all final gates.", [id]);
            if (mode == "stale") File.WriteAllText(Path.Combine(call.Workspace, "src", "alpha.txt"), "external-change");
            if (mode == "pause") new RunControls(fixture.Store).Pause(call.RunId);
            return new(Json.Write(decision), []);
        }
        if (mode is not ("committed-preflight" or "legacy-committed-preflight") && call.TaskId?.EndsWith("-alpha", StringComparison.Ordinal) == true && call.Operation == (mode == "failed-gate" ? "implement" : "prepare-tests") && call.WriteScopes.Length != 0)
        {
            if (!injected)
            {
                injected = true;
                if (mode == "unknown") File.WriteAllText(Path.Combine(call.Workspace, "tests", "alpha.expected"), "partial");
                return Failure(call);
            }
            RecoveryCalls++;
            if (!call.Prompt.Contains("Orchestrator-authorized recovery")) throw new InvalidOperationException("Recovery lacked a targeted instruction.");
            if (mode is "repeat" or "limit") return Failure(call);
            if (mode == "interrupted") throw new IOException("Interrupted after dispatch; side effects uncertain.");
            if (mode == "failed-gate")
            {
                File.WriteAllText(Path.Combine(call.Workspace, "src", "alpha.txt"), "wrong");
                return new(FakeBackend.Result(fixture.Store, call, true, "Claimed recovery completion; independent checks must reject this."), []);
            }
        }
        return await inner.Run(call, token);
    }

    private WorkerResponse Failure(AgentCall call)
    {
        var failure = new ToolFailure("sdlc_write_file", "{\"path\":\"tests/alpha.expected\"}", mode == "known" || mode == "repeat" ? "ToolInputException" : "IOException",
            mode == "limit" ? "Fixture rejection " + RecoveryCalls : "Fixture rejection", mode == "unknown" ? ToolSideEffects.Unknown : ToolSideEffects.None,
            mode is "known" or "repeat" ? "Supply the required justification; dependencies unchanged." : null, mode == "policy");
        var artifact = fixture.Store.Save(call.RunId, call.TaskId, "ToolFailure", failure);
        fixture.Store.Event(call.RunId, "ToolFailed", failure.Message, call.TaskId, artifact.Id);
        return new(Json.Write(new StepResult(call.AssignmentId!, call.Execution!.PlanId, call.Execution.CurrentStep.Id,
            StepStatus.Blocked, "Tool rejected the current step", [artifact.Id], new(BlockerKind.Infrastructure, "Tool rejected the current step", null, null))), []);
    }
}
