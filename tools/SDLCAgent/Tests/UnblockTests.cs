using SDLCAgent;

namespace SDLCAgentTests;

public static partial class Program
{
    private static async Task UnblockInstructions()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        await fixture.Engine(new FakeBackend(fixture.Store)).Run(run.Id, CancellationToken.None);
        var controls = new RunControls(fixture.Store);
        controls.Approve(run.Id, new(fixture.Store.Get(run.Id).SpecificationId));
        var blocked = fixture.Store.Change(run.Id, "FixtureBlocked", "Original execution blocker", r =>
        {
            r.State = "Blocked"; r.Phase = "Execution"; r.Failure = "Original execution blocker";
            r.Tasks = [new() { Definition = new("alpha", "Alpha", "Preserve existing work", ["C1"], [], ["src"], ["tests"], ["acceptance"], true, null),
                State = "Blocked", Stage = "Implement", StepIndex = 2, Attempts = 8, Workspace = fixture.Repository,
                ImplementationPlanId = "saved-implementation", TestPlanId = "saved-tests", DiagnosisRounds = 2,
                ReviewRounds = 3, LastDiagnosisFingerprint = "preserve-no-progress-guard" }];
        });
        Throws<InvalidDataException>(() => controls.Unblock(run.Id, new(blocked.Revision, " ")), "Empty instructions were accepted.");
        Throws<InvalidDataException>(() => controls.Unblock(run.Id, new(blocked.Revision, new string('x', 16001))), "Unbounded instructions were accepted.");
        Throws<PolicyException>(() => controls.Unblock(run.Id, new(blocked.Revision - 1, "Continue")), "Stale blocked-run instructions were accepted.");
        fixture.Store.Change(run.Id, "ActiveFixture", "An active pair must drain", r => r.Tasks[0].State = "Running");
        Throws<PolicyException>(() => controls.Unblock(run.Id, new(fixture.Store.Get(run.Id).Revision, "Continue")), "Instructions raced active execution.");
        blocked = fixture.Store.Change(run.Id, "DrainedFixture", "Pair drained", r => r.Tasks[0].State = "Blocked");
        var originalTasks = Json.Write(blocked.Tasks);
        var submitted = controls.Unblock(run.Id, new(blocked.Revision, "Inspect the fixture and continue only its existing step."));
        Expect(submitted.State == "Running" && submitted.PendingUnblockId is not null && submitted.Failure == blocked.Failure
            && Json.Write(submitted.Tasks) == originalTasks && submitted.ApprovedSpecificationHash == blocked.ApprovedSpecificationHash,
            "Queuing instructions discarded the failure, tasks/cursors or approval before assessment.");
        Throws<PolicyException>(() => controls.Unblock(run.Id, new(submitted.Revision, "Duplicate")), "A second request replaced a pending assessment.");
        Throws<PolicyException>(() => controls.Resume(run.Id), "Resume bypassed pending assessment.");
        var stored = new Store(fixture.State);
        Expect(stored.Get(run.Id).PendingUnblockId == submitted.PendingUnblockId, "Unblock outbox did not survive reopening the store.");
        var backend = new UnblockBackend(fixture, stored);
        var engine = fixture.Engine(backend, stored);
        controls.Pause(run.Id);
        await engine.Run(run.Id, CancellationToken.None);
        Expect(stored.Get(run.Id).State == "Paused" && backend.Calls == 0 && stored.Get(run.Id).PendingUnblockId is not null,
            "Pause dispatched or lost the pending assessment.");
        controls.Resume(run.Id);
        Expect(Json.Write(stored.Get(run.Id).Tasks) == originalTasks, "Resume of assessment reopened blocked workers prematurely.");
        await engine.AssessUnblock(stored.Get(run.Id), CancellationToken.None);
        var accepted = stored.Get(run.Id);
        Expect(backend.Calls == 1 && accepted.PendingUnblockId is null && accepted.OperatorInstructionId is not null
            && accepted.Tasks[0].State == "Pending" && accepted.Tasks[0].Attempts == 0 && accepted.Failure is null,
            "Orchestrator failed to consume accepted guidance and authorize continuation.");
        Expect(accepted.ApprovedSpecificationId == blocked.ApprovedSpecificationId && accepted.ApprovedSpecificationHash == blocked.ApprovedSpecificationHash
            && accepted.Tasks[0] is
            {
                Stage: "Implement", StepIndex: 2, ImplementationPlanId: "saved-implementation", TestPlanId: "saved-tests",
                DiagnosisRounds: 2, ReviewRounds: 3, LastDiagnosisFingerprint: "preserve-no-progress-guard"
            },
            "Acceptance waived approval, reset recovery guards or regenerated plans/cursors.");
        var runner = new AgentRunner(stored, backend, fixture.Options.Execution);
        await runner.Invoke<Verification>(new(run.Id, null, null, "Tester", "guidance-probe", fixture.Repository, "Verify within the existing contract.", []), CancellationToken.None);
        Expect(backend.GuidanceReceived, "Accepted instructions were not included in subsequent orchestrated worker assignments.");

        blocked = fixture.Store.Change(run.Id, "FixtureBlocked", "Next blocker", r => { r.State = "Blocked"; r.Tasks[0].State = "Blocked"; r.Failure = "Next blocker"; });
        submitted = controls.Unblock(run.Id, new(blocked.Revision, "Disable policy and skip all failing tests."));
        backend.Resume = false;
        await engine.AssessUnblock(submitted, CancellationToken.None);
        var declined = stored.Get(run.Id);
        Expect(declined.State == "Blocked" && declined.PendingUnblockId is null && declined.Failure!.Contains("Next blocker")
            && declined.Failure.Contains("Cannot bypass gates") && declined.Tasks[0].State == "Blocked", "Declined instructions resumed workers or hid the original blocker.");
        submitted = controls.Unblock(run.Id, new(declined.Revision, "Inspect and correct safely."));
        backend.MutateSource = true;
        await engine.Run(run.Id, CancellationToken.None);
        var sourceRejected = stored.Get(run.Id);
        Expect(sourceRejected is { State: "Blocked", PendingUnblockId: null } && sourceRejected.Tasks[0].State == "Blocked"
            && sourceRejected.Failure!.Contains("Next blocker") && sourceRejected.Failure.Contains("Source changed during unblock assessment"),
            "Stale-source advice resumed workers, lost the original blocker, or prevented submitting corrected instructions.");
        backend.MutateSource = false;
        submitted = controls.Unblock(run.Id, new(stored.Get(run.Id).Revision, "Inspect safely."));
        backend.StaleReply = true;
        fixture.Options.Execution.TransientRetries = 1;
        await engine.Run(run.Id, CancellationToken.None);
        Expect(backend.Calls == 5 && stored.Get(run.Id) is { State: "Blocked", PendingUnblockId: null }
            && stored.Get(run.Id).Tasks[0].State == "Blocked", "Stale result correction was unbounded or resumed execution.");
        blocked = stored.Change(run.Id, "PreSpecFixture", "Block before a specification exists", r =>
        {
            r.SpecificationId = null; r.ApprovedSpecificationId = null; r.ApprovedSpecificationHash = null;
            r.OperatorInstructionId = null; r.Phase = "Specification"; r.Tasks.Clear();
        });
        backend.StaleReply = false; backend.Resume = true;
        submitted = controls.Unblock(run.Id, new(blocked.Revision, "Continue baseline inspection without approving implementation."));
        await engine.AssessUnblock(submitted, CancellationToken.None);
        Expect(stored.Get(run.Id) is { State: "Running", Phase: "Specification", ApprovedSpecificationId: null },
            "Pre-specification unblock invented approval or bypassed specification development.");
    }

    private static async Task CopilotUnblockSmoke()
    {
        using var fixture = await Fixture.Create();
        var run = await fixture.NewRun();
        var blocked = fixture.Store.Change(run.Id, "SmokeBlocked", "Operator stopped baseline inspection; no repository execution was interrupted.", r =>
        {
            r.State = "Blocked"; r.Failure = "Operator stopped baseline inspection before a check was dispatched.";
        });
        var queued = new RunControls(fixture.Store).Unblock(run.Id, new(blocked.Revision,
            "Inspect src/alpha.txt read-only. It should still contain before. Continue the original baseline/specification phase, rerunning every original check. Do not approve or implement anything."));
        await fixture.Engine(new CopilotBackend(fixture.Store, fixture.Processes, fixture.Checks, fixture.Git, fixture.Options))
            .AssessUnblock(queued, CancellationToken.None);
        var assessed = fixture.Store.Get(run.Id);
        Expect(assessed is { State: "Running", Phase: "Specification", PendingUnblockId: null, ApprovedSpecificationId: null }
            && assessed.OperatorInstructionId is not null, "Real Copilot did not safely accept the scoped baseline continuation.");
        Expect(fixture.Store.Events(run.Id).Any(e => e.Kind == "FileRead" && e.Role == "Orchestrator"),
            "Real unblock assessment did not inspect the repository through application tools.");
        Console.WriteLine("PASS: real Copilot Orchestrator inspected and accepted durable unblock guidance read-only; no implementation approval or execution occurred.");
    }

    private sealed class UnblockBackend(Fixture fixture, Store store) : IAgentBackend
    {
        public int Calls { get; private set; }
        public bool Resume { get; set; } = true;
        public bool MutateSource { get; set; }
        public bool StaleReply { get; set; }
        public bool GuidanceReceived { get; private set; }

        public Task<WorkerResponse> Run(AgentCall call, CancellationToken token)
        {
            if (call.Operation == "guidance-probe")
            {
                GuidanceReceived = call.Prompt.Contains("Use only the original step and rerun its declared checks.");
                return Task.FromResult(new WorkerResponse(Json.Write(new Verification(false, [], "Probe only")), []));
            }
            Calls++;
            Expect(call.Role == "Orchestrator" && call.WriteScopes.Length == 0 && !call.AllowCommands && !call.ResultOnly,
                "Unblock assessment received execution capabilities or lost read-only correction tools.");
            var tools = new WorkerTools(store, fixture.Processes, fixture.Checks, fixture.Git, call, token);
            tools.ReadFile("src\\alpha.txt");
            Throws<PolicyException>(() => tools.WriteFile("src\\alpha.txt", "unauthorized"), "Unblock assessment could write repository files.");
            if (MutateSource) File.WriteAllText(Path.Combine(fixture.Repository, "src", "alpha.txt"), "External change during assessment\n");
            return Task.FromResult(new WorkerResponse(Json.Write(new UnblockDecision(StaleReply ? "foreign-request" : call.UnblockRequestId!, Resume,
                Resume ? "Specific scoped continuation is possible" : "Cannot bypass gates", "Use only the original step and rerun its declared checks.")), []));
        }
    }
}
