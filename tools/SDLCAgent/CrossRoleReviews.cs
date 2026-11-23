namespace SDLCAgent;

public sealed class CrossRoleReviews(Store store, GitWorkspace git, ProcessRunner processes, CheckRunner checks, AgentRunner agents)
{
    public async Task Open(string runId, string taskId, string assignmentId, StepAssignment execution, StepResult result, CancellationToken token)
    {
        var run = store.Get(runId);
        var task = Find(run, taskId);
        if (task.AssignmentId != assignmentId || task.Review is not null)
            throw new WorkerContractException("Expired assignment or an existing review prevents a new challenge.");
        var challenge = result.Challenge ?? new(ReviewTarget.SpecificationInterpretation, result.Blocker!.Reason, task.Definition.CriterionIds, []);
        Contracts.ValidateChallenge(challenge, task.Definition);
        await git.RequireCleanHandoff(task.Workspace!, token);
        var revision = await GitWorkspace.Head(processes, task.Workspace!, token);
        var fingerprint = Json.Write(new { revision, challenge.Target, criteria = challenge.CriterionIds.Order().ToArray(), paths = challenge.Paths.Order().ToArray() });
        if (task.ReviewRounds >= Contracts.MaxReviewRounds)
            throw new WorkerContractException("Cross-role review limit reached. Human review is required; no gate or scope was relaxed.");
        var envelope = new ReviewEnvelope(Guid.NewGuid().ToString("N"), runId, taskId, assignmentId, execution.Role, execution.PlanId,
            execution.CurrentStep.Id, run.ApprovedSpecificationId!, run.ApprovedSpecificationHash!, revision, challenge, result.EvidenceIds);
        var artifact = store.Save(runId, taskId, "ReviewRequest", envelope);
        Change(runId, taskId, assignmentId, "ReviewRequested", $"{execution.Role} requests {challenge.Target} review: {challenge.Claim}", t =>
        {
            t.Review = new()
            {
                Id = envelope.Id, EnvelopeId = artifact.Id, Revision = revision, ReturnStage = t.Stage, ReturnStepIndex = t.StepIndex,
                Stage = challenge.Target == ReviewTarget.SpecificationInterpretation || t.LastReviewFingerprint == fingerprint ? "Adjudicate" : "Reassess"
            };
            t.Stage = "Review"; t.ReviewRounds++; t.LastReviewFingerprint = fingerprint;
            t.CandidatePassed = false; t.HeadCommit = null;
        }, artifact.Id);
    }

    public async Task Run(string runId, string taskId, string assignmentId, Action boundary, CancellationToken token)
    {
        while (true)
        {
            boundary();
            var run = store.Get(runId);
            var task = Find(run, taskId);
            var review = task.Review ?? throw new WorkerContractException("Review stage has no saved request.");
            var envelope = Json.Read<ReviewEnvelope>(store.ReadArtifact(review.EnvelopeId));
            if (envelope.SpecificationId != run.ApprovedSpecificationId || envelope.SpecificationHash != run.ApprovedSpecificationHash)
                throw new WorkerContractException("Review references a superseded approved specification; human approval cannot be inherited.");
            await Fresh(task, token);

            if (review.Stage is "Reassess" or "Adjudicate")
            {
                var role = review.Stage == "Adjudicate" ? "SpecCreator" : envelope.Challenge.Target == ReviewTarget.Test ? "Tester" : "SpecImplementer";
                var message = Message(run, task, assignmentId, role);
                var decision = await agents.Invoke<ReviewDecision>(ReadCall(run, task, assignmentId, message,
                    "review-decision", DecisionPrompt + Context(run, task, envelope)), token);
                await Fresh(Find(store.Get(runId), taskId), token);
                var artifact = store.Save(runId, taskId, "ReviewDecision", decision);
                var ownDefect = role == "Tester" && decision.Disposition == ReviewDisposition.TestDefect
                    || role == "SpecImplementer" && decision.Disposition == ReviewDisposition.ImplementationDefect;
                if (role != "SpecCreator" && !ownDefect)
                {
                    Change(runId, taskId, assignmentId, "ReviewEscalated", "Counterparty did not admit its own defect; SpecCreator must review both arguments.", t =>
                    {
                        t.Review!.PreviousDecisionId = artifact.Id; t.Review.Stage = "Adjudicate"; t.Review.PendingMessage = null;
                    }, artifact.Id);
                    continue;
                }
                if (decision.Disposition is ReviewDisposition.SpecificationChange or ReviewDisposition.NeedsSpecReview)
                {
                    var change = store.Save(runId, taskId, "SpecificationChangeRequest", new { envelope.Id, decision, run.ApprovedSpecificationId });
                    Change(runId, taskId, assignmentId, "ReviewHumanRequired", "SpecCreator identified a genuine requirement ambiguity/change. Use specification feedback and fresh human approval. " + decision.Explanation, t =>
                    { t.Review!.DecisionId = artifact.Id; t.Review.Stage = "HumanRequired"; t.Review.PendingMessage = null; }, change.Id);
                    throw new WorkerContractException("Specification decision requires human review: " + decision.Explanation);
                }
                if (decision.Disposition == ReviewDisposition.NoDefect)
                {
                    Complete(runId, taskId, assignmentId, "SpecCreator rejected the challenge: " + decision.Explanation, artifact.Id);
                    return;
                }
                Change(runId, taskId, assignmentId, "ReviewDecisionSaved", decision.Explanation, t =>
                {
                    t.Review!.DecisionId = artifact.Id;
                    t.Review.RepairRole = decision.Disposition == ReviewDisposition.TestDefect ? "Tester" : "SpecImplementer";
                    t.Review.Stage = "RepairPlanning"; t.Review.PendingMessage = null;
                }, artifact.Id);
                continue;
            }

            if (review.Stage == "RepairPlanning")
            {
                if (review.BeforeEvidenceId is null)
                {
                    var before = await checks.Batch(run, taskId, task.Workspace!, task.Definition.Checks, token);
                    var evidence = store.Save(runId, taskId, "ReviewBeforeEvidence", new { revision = review.Revision, before });
                    Change(runId, taskId, assignmentId, "ReviewBeforeMeasured", "Preserved checks before any review repair; failures remain evidence, never waived.", t => t.Review!.BeforeEvidenceId = evidence.Id, evidence.Id);
                    boundary();
                    task = Find(store.Get(runId), taskId); review = task.Review!;
                }
                var message = Message(run, task, assignmentId, review.RepairRole!);
                var plan = await agents.Invoke<ReviewRepairPlan>(ReadCall(run, task, assignmentId, message, "review-repair-plan",
                    "Plan only the smallest repair authorized by the saved review decision. Keep unrelated completed work and plans intact. "
                    + "Test repairs must correct the requirement-linked expectation, not remove assertions, skip tests, relax thresholds, or change the runner. "
                    + "Production repairs cannot edit tests. SpecCreator cannot write code. Use explicit narrow writePaths, unique step IDs and checks. "
                    + "Checks declared in a repair step must pass; use restore/build only if other behavior is still incomplete. "
                    + "The orchestrator independently rechecks the full item afterwards; never waive failures. Return context, summary and steps."
                    + Context(run, task, envelope)), token);
                await Fresh(Find(store.Get(runId), taskId), token);
                var artifact = store.Save(runId, taskId, "ReviewRepairPlan", plan);
                Change(runId, taskId, assignmentId, "ReviewRepairPlanned", "Saved owner-scoped repair; counterpart plans/cursors are preserved.", t =>
                { t.Review!.RepairPlanId = artifact.Id; t.Review.RepairStepIndex = 0; t.Review.Stage = "Repairing"; t.Review.PendingMessage = null; }, artifact.Id);
                continue;
            }

            if (review.Stage == "Repairing")
            {
                var plan = Json.Read<ReviewRepairPlan>(store.ReadArtifact(review.RepairPlanId!));
                Contracts.ValidateRepairPlan(plan, task, run.Request.Checks);
                if (review.RepairStepIndex < plan.Steps.Length)
                {
                    var step = plan.Steps[review.RepairStepIndex];
                    using var scope = AuditScope.Use(new(assignmentId, review.RepairRole, review.RepairPlanId, step.Id, task.Attempts));
                    var execution = new StepAssignment(assignmentId, review.RepairPlanId!, review.RepairRole!, WorkerStage.ReviewRepair, step,
                        new(step.WritePaths, Contracts.CheckNamesFor(WorkerStage.ReviewRepair, task.Definition, run.Request.Checks), step.WritePaths.Length != 0));
                    store.Event(runId, "ReviewRepairStepStarted", step.Action, taskId, review.RepairPlanId);
                    var call = new AgentCall(runId, taskId, assignmentId, review.RepairRole!, "review-repair", task.Workspace!,
                        Orchestrator.StepPrompt(execution, "Execute only this approved review repair. Preserve test coverage and the approved requirements. " + Context(run, task, envelope)),
                        step.WritePaths, true, review.Revision, Execution: execution);
                    var result = await agents.Invoke<StepResult>(call, token);
                    if (result.Status != StepStatus.Completed)
                        throw new WorkerContractException("Review repair could not complete safely; no nested repair/review replay: " + result.Summary);
                    await new WorkerTools(store, processes, checks, git, call, token).ValidateChanges();
                    var names = step.Checks.AsEnumerable();
                    if (step.Checks.Any(name => run.Request.Checks.Single(c => c.Name == name).Kind == "test"))
                        names = run.Request.Checks.Where(c => c.Kind is "restore" or "build").Select(c => c.Name).Concat(names);
                    var evidence = await checks.Batch(run, taskId, task.Workspace!, names, token);
                    if (evidence.Any(e => !CheckRunner.Passed(e.Result)))
                        throw new WorkerContractException("Review repair's completion checks failed: " + Json.Write(evidence));
                    var head = await git.Commit(run, taskId, task.Workspace!, $"{taskId}: review repair {step.Id}", token);
                    Change(runId, taskId, assignmentId, "ReviewRepairStepCompleted", result.Summary, t =>
                    { t.Review!.RepairStepIndex++; t.Review.Revision = head; }, review.RepairPlanId);
                    continue;
                }
                if (review.Revision == envelope.Revision
                    || (await GitWorkspace.Git(processes, task.Workspace!, token, "diff", "--name-only", envelope.Revision, review.Revision)).Length == 0)
                    throw new WorkerContractException("A confirmed defect requires an actual scoped repair; unchanged source cannot be accepted as a correction.");
                var after = await checks.Batch(run, taskId, task.Workspace!, task.Definition.Checks, token);
                var artifact = store.Save(runId, taskId, "ReviewAfterEvidence", new { revision = review.Revision, after });
                Change(runId, taskId, assignmentId, "ReviewRepairMeasured", "Fresh post-repair evidence saved. Passing repair review does not waive remaining item/integration gates.", t =>
                { t.Review!.AfterEvidenceId = artifact.Id; t.Review.Stage = "VerifyRepair"; }, artifact.Id);
                continue;
            }

            if (review.Stage == "VerifyRepair")
            {
                var role = review.RepairRole == "Tester" ? "SpecCreator" : "Tester";
                var message = Message(run, task, assignmentId, role);
                var verification = await agents.Invoke<ReviewVerification>(ReadCall(run, task, assignmentId, message, "review-verify-repair",
                    "Independently review the exact repair against the approved requirements and original challenge. Inspect the changed files and diff. "
                    + "For test corrections confirm assertions/coverage were preserved, the corrected expectation follows the cited requirement, and failures were not hidden. "
                    + "Compare before/after command evidence; do not claim remaining failed checks pass. Passed means this narrow repair resolves the challenge, "
                    + "not that the whole item is complete. Reject unjustified expectation changes, removed/skipped tests, relaxed criteria or runner changes. "
                    + "Return context, passed, assertionCoveragePreserved, explanation, citations and current-message evidenceIds."
                    + Context(run, task, envelope)), token);
                await Fresh(Find(store.Get(runId), taskId), token);
                var artifact = store.Save(runId, taskId, "ReviewVerification", verification);
                if (!verification.Passed || !verification.AssertionCoveragePreserved)
                    throw new WorkerContractException("Independent review rejected the repair; human review required without waiving gates: " + verification.Explanation);
                Complete(runId, taskId, assignmentId, "Independent review accepted the scoped repair: " + verification.Explanation, artifact.Id);
                return;
            }
            throw new WorkerContractException("Review cannot proceed automatically from stage " + review.Stage + ". Human remediation is required.");
        }
    }

    private ReviewAssignment Message(RunData run, TaskData task, string assignmentId, string role)
    {
        var review = task.Review!;
        if (review.PendingMessage?.AssignmentId == assignmentId) return review.PendingMessage;
        var message = new ReviewAssignment(review.Id, Guid.NewGuid().ToString("N"), assignmentId, run.ApprovedSpecificationId!, review.Revision, role, review.Stage);
        var artifact = store.Save(run.Id, task.Definition.Id, "ReviewMessage", new { message, assignmentId });
        Change(run.Id, task.Definition.Id, assignmentId, "ReviewMessageQueued", $"Persisted {review.Stage} message for {role} before dispatch.", t => t.Review!.PendingMessage = message, artifact.Id);
        return message;
    }

    private AgentCall ReadCall(RunData run, TaskData task, string assignmentId, ReviewAssignment message, string operation, string prompt) =>
        new(run.Id, task.Definition.Id, assignmentId, message.Role, operation, task.Workspace!,
            $"Current review message: {Json.Write(message)}\n{prompt}", [],
            ChangeBase: Json.Read<ReviewEnvelope>(store.ReadArtifact(task.Review!.EnvelopeId)).Revision, Review: message);

    private string Context(RunData run, TaskData task, ReviewEnvelope envelope)
    {
        var review = task.Review!;
        string Document(string? id) => id is null ? "none" : store.ReadArtifact(id);
        var sourceEvidence = envelope.EvidenceIds.Select(id => new { id, evidence = store.ReadArtifact(id) }).ToArray();
        return $"\nApproved specification (authoritative; do not alter): {Document(run.ApprovedSpecificationId)}"
            + $"\nCitation criterionIds must be assigned to this task: {Json.Write(task.Definition.CriterionIds)}. Do not cite other criteria from the parent specification."
            + $"\nOriginal challenge: {Json.Write(envelope)}\nOriginal observations (historical, not fresh checks): {Json.Write(sourceEvidence)}"
            + $"\nWork item: {Json.Write(task.Definition)}\nPrevious reviewer decision: {Document(review.PreviousDecisionId)}"
            + $"\nDecision authorizing repair: {Document(review.DecisionId)}\nBefore evidence: {Document(review.BeforeEvidenceId)}\nAfter evidence: {Document(review.AfterEvidenceId)}"
            + $"\nCompare current files to challenged revision {envelope.Revision}. Use scoped read/diff tools and cite their current-message EvidenceIds.";
    }

    private async Task Fresh(TaskData task, CancellationToken token)
    {
        await git.RequireCleanHandoff(task.Workspace!, token);
        if (await GitWorkspace.Head(processes, task.Workspace!, token) != task.Review!.Revision)
            throw new WorkerContractException("Review source revision changed. Stale decisions cannot authorize a repair or completion.");
    }

    private void Complete(string runId, string taskId, string assignmentId, string explanation, string documentId) =>
        Change(runId, taskId, assignmentId, "ReviewResolved", explanation, t =>
        {
            var review = t.Review!;
            t.Stage = review.ReturnStage; t.StepIndex = review.ReturnStepIndex;
            t.Failure = explanation; t.Review = null; t.CandidatePassed = false; t.HeadCommit = null;
        }, documentId);

    private void Change(string runId, string taskId, string assignmentId, string kind, string message, Action<TaskData> change, string? documentId = null) =>
        store.Change(runId, kind, message, run =>
        {
            var task = Find(run, taskId);
            if (task.AssignmentId != assignmentId) throw new WorkerContractException("Stale review assignment cannot update state.");
            change(task);
        }, taskId, documentId);

    private static TaskData Find(RunData run, string taskId) => run.Tasks.Single(t => t.Definition.Id == taskId);

    internal const string DecisionPrompt = """
        Reassess the challenge read-only. The approved specification is authoritative; code, tests and agent claims are not.
        Inspect the actual subject files. Do not defend your earlier output automatically or accept the other worker's allegation without evidence.
        Return context, disposition (testDefect, implementationDefect, noDefect, needsSpecReview, specificationChange), explanation,
        citations [{criterionId,quote}] with exact approved requirement quotations, and current-message repository evidenceIds.
        Admit your own defect when evidence supports it. A disputed interpretation or accusation against another role goes to SpecCreator.
        SpecCreator must resolve interpretations already fixed by the approved text without reopening product decisions.
        Only a genuine missing/ambiguous requirement or change to approved behavior is specificationChange and requires human approval.
        A test contradicting an explicit approved rule is testDefect, not a product-specification gap. Never weaken requirements to obtain a pass.
        """;
}
