namespace SDLCAgent;

public sealed class RunControls(Store store)
{
    public RunData Approve(string id, ControlRequest request)
    {
        var run = store.Get(id);
        if (run.State != "AwaitingApproval" || request.Version != run.SpecificationId)
            throw new PolicyException("Approval requires AwaitingApproval and the exact current specification artifact ID.");
        Contracts.ValidateSpec(Json.Read<Specification>(store.ReadArtifact(request.Version!)), run.Request.Checks, forApproval: true);
        var spec = store.GetArtifact(request.Version!);
        var approval = store.Save(id, null, "HumanApproval", new
        {
            specificationId = spec.Id, spec.Sha256, policyHash = run.PolicyHash, at = DateTimeOffset.UtcNow
        });
        return store.Change(id, "SpecificationApproved", "Human approved the exact specification and execution policy snapshot.", r =>
        {
            if (r.State != "AwaitingApproval" || r.SpecificationId != request.Version)
                throw new PolicyException("Specification changed during approval.");
            r.ApprovedSpecificationId = spec.Id; r.ApprovedSpecificationHash = spec.Sha256;
            r.Phase = "Planning"; r.State = "Running"; r.Failure = null;
        }, documentId: approval.Id);
    }

    public RunData Feedback(string id, ControlRequest request)
    {
        if (string.IsNullOrWhiteSpace(request.Feedback)) throw new InvalidDataException("Feedback is required.");
        var run = store.Get(id);
        if (request.Version is not null && request.Version != run.SpecificationId)
            throw new PolicyException("Feedback refers to a stale specification. Reload and review the current version before submitting.");
        if (run.State is not ("AwaitingApproval" or "Blocked" or "Paused") || run.Tasks.Any(t => t.State is "Running" or "Integrating"))
            throw new PolicyException("Wait until specification input is requested, or pause and drain execution before changing the specification.");
        var history = store.Save(id, null, "SpecificationRevisionRequested", new
        {
            request.Feedback, run.SpecificationId, run.ApprovedSpecificationId, run.Tasks
        });
        return store.Change(id, "SpecificationRevisionRequested", request.Feedback, r =>
        {
            if (r.Revision != run.Revision) throw new PolicyException("Run changed during feedback; reload and retry.");
            r.Feedback = Redaction.Apply(request.Feedback); r.ApprovedSpecificationId = null; r.ApprovedSpecificationHash = null;
            r.PendingUnblockId = null; r.OperatorInstructionId = null;
            r.InspectionCommit = null; r.InspectionWorkspace = null; r.BaselinePassed = false;
            r.Tasks.Clear(); r.Replans++; r.Phase = "Specification"; r.State = "Running"; r.Failure = null;
        }, documentId: history.Id);
    }

    public RunData Pause(string id) => store.Change(id, "PauseRequested", "Human requested a graceful pause.", r =>
    {
        if (r.State is "Completed" or "Canceled" or "Blocked") throw new PolicyException("Run is terminal/blocked, not executing.");
        if (r.State is "Paused" or "PauseRequested") return;
        r.PreviousState = r.State == "AwaitingApproval" ? "AwaitingApproval" : "Running";
        r.State = r.State == "Running" ? "PauseRequested" : "Paused";
    });

    public RunData Unblock(string id, UnblockControlRequest request)
    {
        if (string.IsNullOrWhiteSpace(request.Instruction)) throw new InvalidDataException("Unblock instructions are required.");
        if (request.Instruction.Length > 16000) throw new InvalidDataException("Unblock instructions must not exceed 16,000 characters.");
        var run = store.Get(id);
        Validate(run);
        var instruction = store.Save(id, null, "OperatorUnblockRequest", new UnblockRequest(Redaction.Apply(request.Instruction),
            run.Failure, run.Phase, run.SpecificationId, run.ApprovedSpecificationId, run.ApprovedSpecificationHash, run.Tasks.ToArray()));
        return store.Change(id, "OperatorUnblockRequested", "Human instructions queued for read-only orchestrator assessment; tasks and approval preserved.", r =>
        {
            Validate(r);
            r.PendingUnblockId = instruction.Id;
            r.State = "Running";
        }, documentId: instruction.Id);

        void Validate(RunData r)
        {
            if (r.State != "Blocked" || r.Revision != request.Revision || r.PendingUnblockId is not null
                || r.Tasks.Any(t => t.State is "Running" or "Integrating"))
                throw new PolicyException("Unblock instructions require the exact current blocked run revision with all active work drained. Reload before submitting.");
        }
    }

    public RunData Resume(string id) => store.Change(id, "ResumeRequested", "Resume after operator remediation; previous evidence remains available.", r =>
    {
        if (r.PendingUnblockId is not null && r.State != "Paused") throw new PolicyException("An orchestrator unblock assessment is pending; do not bypass it with Resume.");
        if (r.State is not ("Paused" or "Blocked")) throw new PolicyException("Only paused or blocked runs can resume.");
        if (r.Tasks.Any(t => t.State is "Running" or "Integrating")) throw new PolicyException("Wait for active work to drain.");
        r.State = r.PreviousState == "AwaitingApproval" ? "AwaitingApproval" : "Running";
        r.PreviousState = null;
        if (r.PendingUnblockId is null) r.Failure = null;
        if (r.PendingUnblockId is null)
            foreach (var t in r.Tasks.Where(t => t.State == "Blocked")) { t.State = "Pending"; t.Attempts = 0; }
    });

    public RunData Cancel(string id) => store.Change(id, "RunCanceled", "Human canceled this run. Commits and audit evidence remain; branch ownership released.", r =>
    {
        if (r.State is not ("Paused" or "Blocked" or "AwaitingApproval") || r.Tasks.Any(t => t.State is "Running" or "Integrating"))
            throw new PolicyException("Pause and drain execution before canceling; only paused, blocked, or awaiting-approval runs can be canceled.");
        r.State = "Canceled"; r.PendingUnblockId = null;
    });
}
