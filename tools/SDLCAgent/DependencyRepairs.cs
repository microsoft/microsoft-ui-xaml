namespace SDLCAgent;

public sealed class DependencyRepairs(Store store, GitWorkspace git, ProcessRunner processes, CheckRunner checks, AgentRunner agents)
{
    public const int MaxRounds = 2;

    public async Task Run(RunData run, TaskData task, CancellationToken token)
    {
        if (run.State != "Running" || task.DependencyRepairId is null || task.Workspace is null || task.Diagnosis is not null || task.Review is not null)
            throw new WorkerContractException("Dependency repair requires a drained task with no pending diagnosis/review execution.");
        var blockerId = task.DependencyRepairId;
        var blocker = Json.Read<StepBlocker>(store.ReadArtifact(blockerId));
        var path = blocker.RequiredPath ?? throw new WorkerContractException("Dependency repair requires an explicit production file.");
        SafePaths.ValidateRelative(path);
        if (blocker.Kind != BlockerKind.BehaviorFailure || SafePaths.InScope(path, task.Definition.CodePaths.Concat(task.Definition.TestPaths)))
            throw new WorkerContractException("Only out-of-owner production failures can amend an assignment.");
        SafePaths.Resolve(task.Workspace, path);
        await GitWorkspace.Git(processes, task.Workspace, token, "ls-files", "--error-unmatch", "--", path);
        await git.RequireCleanHandoff(task.Workspace, token);
        var before = await checks.Snapshot(task.Workspace, token);
        var context = new DependencyRepairContext(task.Definition.Id, task.Definition, blocker,
            run.ApprovedSpecificationId!, run.ApprovedSpecificationHash!, before.Revision, before.Fingerprint);
        var message = store.Save(run.Id, task.Definition.Id, "DependencyRepairContext", context);
        var spec = Json.Read<Specification>(store.ReadArtifact(run.ApprovedSpecificationId!));
        var proposal = await agents.Invoke<DependencyRepairProposal>(new(run.Id, null, null, "Orchestrator", "dependency-repair-plan", task.Workspace,
            "Reassign a confirmed production defect to its capable owner by revising THIS task, read-only. Do not execute any repair now. "
            + "Return requestId, replacement (full WorkDefinition), explanation. Keep the exact task ID, dependencies, criterionIds, checks and testPaths. "
            + "codePaths must be exactly its existing paths PLUS the one required production FILE; no directory-wide permission expansion. "
            + "Retain every obligation ID, criterion mapping and authorized check. Revise only task-local ownership wording/objective/paths as needed to make the existing obligations achievable. "
            + "Do not weaken approved outcomes or measurements, rewrite tests, waive failed gates, recreate committed code or add new requirements. "
            + "The existing committed tests/source and test plan are preserved. The implementer will get a new scoped repair plan; independent verification and all gates still run. "
            + "Remove contradictory report-only restrictions from this assignment where the approved spec permits corrective work. "
            + $"Request ID: {message.Id}\nContext: {Json.Write(context)}\nApproved specification: {Json.Write(spec)}\nPolicy: {run.PolicyJson}",
            [], DependencyRepairRequestId: message.Id), token);
        ValidateProposal(message.Id, proposal, context, run, spec);
        var proposed = store.Save(run.Id, task.Definition.Id, "DependencyRepairProposal", proposal);
        var review = await agents.Invoke<CoverageAssessment>(new(run.Id, null, null, "SpecCreator", "review-dependency-repair", task.Workspace,
            "Independently review this assignment revision against the EXACT approved specification and observed failure, read-only. "
            + "Return passed and explanation. Accept only a narrowly scoped correction of the existing production file satisfying already-approved requirements; "
            + "every original criterion/obligation and measurement must remain covered. Task-local report-only restrictions may be corrected only when the approved spec permits corrective production work. "
            + "Reject requirement changes, weakened tests/thresholds, recreated committed work, unsafe handoffs, unexplained permission changes, or inability to verify the repair. "
            + $"Approved specification: {Json.Write(spec)}\nOriginal context: {Json.Write(context)}\nProposed assignment: {Json.Write(proposal)}", []), token);
        var reviewed = store.Save(run.Id, task.Definition.Id, "DependencyRepairReview", review);
        if (before != await checks.Snapshot(task.Workspace, token))
            throw new WorkerContractException("Source changed during assignment review; stale scope changes are rejected.");
        var archive = store.Save(run.Id, task.Definition.Id, "TaskAssignmentBeforeDependencyRepair", task);
        store.Change(run.Id, review.Passed ? "DependencyRepairAccepted" : "DependencyRepairDeclined", review.Explanation, r =>
        {
            var current = r.Tasks.Single(t => t.Definition.Id == task.Definition.Id);
            if (r.State is not ("Running" or "PauseRequested") || current.DependencyRepairId != blockerId || Json.Write(current) != Json.Write(task)
                || r.SpecificationId != context.SpecificationId || r.ApprovedSpecificationId != context.SpecificationId || r.ApprovedSpecificationHash != context.SpecificationHash)
                throw new WorkerContractException("Assignment/specification changed during dependency review; no new scope was granted.");
            current.DependencyRepairId = null;
            if (!review.Passed) { current.State = "Blocked"; current.Failure += "\nAssignment review declined: " + review.Explanation; return; }
            current.Definition = proposal.Replacement;
            current.ImplementationPlanId = null;
            current.Stage = "Plans"; current.StepIndex = 0; current.PlanRepairRole = "SpecImplementer"; current.RepairStage = "Implementation";
            current.State = "Pending"; current.AssignmentId = null; current.Attempts = 0; current.LastBlockerFingerprint = null;
            current.CandidatePassed = false; current.PendingVerification = null;
            current.Failure = "Orchestrator-reviewed assignment correction: " + proposal.Explanation + "\nOriginal failure: " + blocker.Reason
                + "\nRepair the specific production defect now; simply rerunning the unchanged failing test is not a repair. Keep tests and acceptance criteria intact.";
        }, task.Definition.Id, reviewed.Id);
        store.Event(run.Id, "DependencyRepairEvidence", $"Proposal {proposed.Id}; original plans/cursor archived in {archive.Id}; independent review {reviewed.Id}.", task.Definition.Id, archive.Id);
    }

    public static void ValidateProposal(string requestId, DependencyRepairProposal proposal, DependencyRepairContext context, RunData run, Specification spec)
    {
        var original = context.Definition;
        var replacement = proposal.Replacement;
        if (proposal.RequestId != requestId || string.IsNullOrWhiteSpace(proposal.Explanation) || replacement.Id != original.Id
            || !replacement.Dependencies.SequenceEqual(original.Dependencies) || !replacement.CriterionIds.SequenceEqual(original.CriterionIds)
            || !replacement.Checks.SequenceEqual(original.Checks) || !replacement.TestPaths.SequenceEqual(original.TestPaths)
            || replacement.TestFirst != original.TestFirst || replacement.NonTddReason != original.NonTddReason)
            throw new InvalidDataException("Dependency repair must preserve task identity, dependencies, criteria, checks, test scopes and TDD policy.");
        var allowed = original.CodePaths.Append(context.Blocker.RequiredPath!).ToHashSet(StringComparer.OrdinalIgnoreCase);
        if (!allowed.SetEquals(replacement.CodePaths) || replacement.CodePaths.Length != allowed.Count)
            throw new InvalidDataException("Dependency repair may add exactly the required production file, never broader paths or remove original paths.");
        Contracts.ValidateObligations(replacement);
        if (replacement.Obligations!.Length != original.Obligations!.Length || replacement.Obligations.Any(o => !original.Obligations.Any(old =>
            old.Id == o.Id && old.CriterionId == o.CriterionId && old.Checks.SequenceEqual(o.Checks))))
            throw new InvalidDataException("Dependency repair must retain every original obligation identity, criterion and check.");
        var definitions = run.Tasks.Select(t => t.Definition.Id == original.Id ? replacement : t.Definition).ToArray();
        Contracts.ValidateTasks(definitions, spec, run.Request.Checks);
        foreach (var other in run.Tasks.Where(t => t.Definition.Id != original.Id && t.State != "Integrated"))
            if (SafePaths.Overlap([context.Blocker.RequiredPath!], other.Definition.CodePaths.Concat(other.Definition.TestPaths)))
                throw new InvalidDataException("Required production file overlaps another unfinished owner; explicit dependency replanning is required.");
    }
}
