using System.Text.Json;
using System.Text.Json.Serialization;

namespace SDLCAgent;

public static class Json
{
    public static readonly JsonSerializerOptions Options = new(JsonSerializerDefaults.Web)
    {
        WriteIndented = true,
        UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow,
        RespectNullableAnnotations = true,
        RespectRequiredConstructorParameters = true
    };

    static Json() => Options.Converters.Add(new JsonStringEnumConverter(JsonNamingPolicy.CamelCase, allowIntegerValues: false));

    public static string Write<T>(T value) => JsonSerializer.Serialize(value, Options);
    public static T Read<T>(string text) => JsonSerializer.Deserialize<T>(text, Options)
        ?? throw new InvalidDataException($"Expected {typeof(T).Name}, not null.");
}

public sealed record CheckCommand(string Name, string Kind, string FileName, string[] Arguments);
public sealed record RunRequest(string Repository, string Branch, string Problem, CheckCommand[] Checks,
    bool TrustRepository, string? Model = null);
public sealed record Criterion(string Id, string Outcome, string Measurement, string[] Checks);
public sealed record Specification(string Title, string Problem, string Scope, string[] NonGoals,
    string RepositoryFindings, string Approach, Criterion[] Criteria, string[] Questions, int? PlannedTestCount = null);
public sealed record WorkDefinition(string Id, string Title, string Objective, string[] CriterionIds,
    string[] Dependencies, string[] CodePaths, string[] TestPaths, string[] Checks, bool TestFirst,
    string? NonTddReason, TaskObligation[]? Obligations = null);
public sealed record TaskObligation(string Id, string CriterionId, string Outcome, string Measurement, string[] Paths, string[] Checks);
public sealed record CoverageAssessment(bool Passed, string Explanation);
public sealed record DependencyRepairContext(string TaskId, WorkDefinition Definition, StepBlocker Blocker,
    string SpecificationId, string SpecificationHash, string Revision, string Fingerprint);
public sealed record DependencyRepairProposal(string RequestId, WorkDefinition Replacement, string Explanation);
public sealed record TaskVerificationContext(string MessageId, string AssignmentId, string TaskId,
    string SpecificationId, string SpecificationHash, string Revision);
public enum ObligationStatus { Satisfied, Unsatisfied, NotVerified }
public sealed record ObligationResult(string Id, ObligationStatus Status, string Explanation, string[] EvidenceIds);
public sealed record TaskVerificationResult(TaskVerificationContext Context, bool Passed, ObligationResult[] Obligations, string Explanation);
public sealed record Decomposition(WorkDefinition[] Tasks);
public sealed record PlanStep(string Id, string Action, string ExpectedOutcome, string Verification, string[] WritePaths, string[] Checks);
public sealed record VerificationStep(string Id, string Action, string ExpectedOutcome, string Verification, string[] PassChecks)
{
    public PlanStep AsStep() => new(Id, Action, ExpectedOutcome, Verification, [], PassChecks);
}
public sealed record ImplementerPlan(int ContractVersion, string Summary, string[] Risks, PlanStep[] ScaffoldingSteps, PlanStep[] ImplementationSteps);
public sealed record TesterPlan(int ContractVersion, string Summary, string[] Risks, PlanStep[] PreparationSteps, VerificationStep[] VerificationSteps);
public enum WorkerStage { Scaffold, Tests, Implementation, Verify, ReviewRepair, VerificationPreparation }
public enum StepStatus { Completed, Blocked, Failed, ReviewRequested }
public enum BlockerKind { MissingScaffold, TestSetupFailure, BehaviorFailure, CapabilityMismatch, SpecificationGap, Infrastructure }
public sealed record StepBlocker(BlockerKind Kind, string Reason, string? RequiredPath, string? RequiredCheck);
public sealed record StepCapabilities(string[] WritePaths, string[] CheckNames, bool CanCommit);
public sealed record StepAssignment(string AssignmentId, string PlanId, string Role, WorkerStage Stage, PlanStep CurrentStep, StepCapabilities Capabilities);
public sealed record StepResult(string AssignmentId, string PlanId, string StepId, StepStatus Status, string Summary, string[] EvidenceIds, StepBlocker? Blocker,
    ReviewChallenge? Challenge = null);
public enum ReviewTarget { Test, Implementation, SpecificationInterpretation }
public enum ReviewDisposition { TestDefect, ImplementationDefect, NoDefect, NeedsSpecReview, SpecificationChange }
public sealed record ReviewChallenge(ReviewTarget Target, string Claim, string[] CriterionIds, string[] Paths);
public sealed record RequirementCitation(string CriterionId, string Quote);
public sealed record ReviewEnvelope(string Id, string RunId, string TaskId, string SourceAssignmentId, string SourceRole,
    string PlanId, string StepId, string SpecificationId, string SpecificationHash, string Revision, ReviewChallenge Challenge, string[] EvidenceIds);
public sealed record ReviewAssignment(string ReviewId, string MessageId, string AssignmentId, string SpecificationId, string Revision, string Role, string Stage);
public sealed record ReviewDecision(ReviewAssignment Context, ReviewDisposition Disposition, string Explanation, RequirementCitation[] Citations, string[] EvidenceIds);
public sealed record ReviewRepairPlan(ReviewAssignment Context, string Summary, PlanStep[] Steps);
public sealed record ReviewVerification(ReviewAssignment Context, bool Passed, bool AssertionCoveragePreserved, string Explanation,
    RequirementCitation[] Citations, string[] EvidenceIds);
public enum ToolSideEffects { None, Confirmed, Unknown }
public sealed record ToolFailure(string Tool, string Arguments, string ExceptionType, string Message,
    ToolSideEffects SideEffects, string? Correction, bool PolicyDenied);
public sealed record ToolFailureResult(bool Success, string EvidenceId, ToolFailure Error);
public sealed record DiagnosisAssignment(string Id, string MessageId, string AssignmentId, string SpecificationId,
    string SpecificationHash, string Fingerprint, string PlanId, string StepId);
public enum RecoveryDisposition { RetryCurrentStep, Escalate }
public sealed record DiagnosisDecision(DiagnosisAssignment Context, RecoveryDisposition Disposition,
    string Cause, string Instruction, string Verification, string[] EvidenceIds);
public sealed class DiagnosisState
{
    public required string Id { get; set; }
    public required string RequestId { get; set; }
    public required string PlanId { get; set; }
    public required string StepId { get; set; }
    public required string Stage { get; set; }
    public required string Revision { get; set; }
    public required string Fingerprint { get; set; }
    public required string SpecificationId { get; set; }
    public required string SpecificationHash { get; set; }
    public int StepIndex { get; set; }
    public bool SafeToRetry { get; set; }
    public bool VerificationOnly { get; set; }
    public DiagnosisAssignment? PendingMessage { get; set; }
    public string? DecisionId { get; set; }
    public string? Instruction { get; set; }
    public bool Dispatched { get; set; }
}

public sealed class ReviewState
{
    public required string EnvelopeId { get; set; }
    public required string Id { get; set; }
    public required string Revision { get; set; }
    public required string ReturnStage { get; set; }
    public int ReturnStepIndex { get; set; }
    public string Stage { get; set; } = "Reassess";
    public ReviewAssignment? PendingMessage { get; set; }
    public string? DecisionId { get; set; }
    public string? PreviousDecisionId { get; set; }
    public string? RepairRole { get; set; }
    public string? RepairPlanId { get; set; }
    public int RepairStepIndex { get; set; }
    public string? BeforeEvidenceId { get; set; }
    public string? AfterEvidenceId { get; set; }
}
public sealed record Verification(bool Passed, string[] CriterionIds, string Explanation);
public sealed record RedAssessment(bool MissingBehavior, string Explanation);
public sealed record ControlRequest(string? Version = null, string? Feedback = null);
public sealed record UnblockControlRequest(long Revision, string Instruction);
public sealed record UnblockRequest(string Instruction, string? Failure, string Phase, string? SpecificationId,
    string? ApprovedSpecificationId, string? ApprovedSpecificationHash, TaskData[] Tasks);
public sealed record UnblockDecision(string RequestId, bool Resume, string Explanation, string WorkerInstruction);
public sealed record CommandResult(int ExitCode, string Output, bool TimedOut, string? EvidenceId = null,
    bool OutputTruncated = false, [property: JsonIgnore] string? StandardOutput = null);
public sealed record CheckExecution(string Name, string Kind, CommandResult Result);
public sealed record AuditEvent(long Id, string RunId, string? TaskId, string Kind, string Message,
    string? DocumentId, DateTimeOffset At, string? AssignmentId, string? Role, string? PlanId, string? StepId, int? Attempt);
public sealed record Artifact(string Id, string RunId, string? TaskId, string Kind, string Path,
    string Sha256, DateTimeOffset At);

public sealed class TaskData
{
    public required WorkDefinition Definition { get; set; }
    public string State { get; set; } = "Pending";
    public string Stage { get; set; } = "Plans";
    public int StepIndex { get; set; }
    public string? AssignmentId { get; set; }
    public string? Workspace { get; set; }
    public string? Branch { get; set; }
    public string? BaseCommit { get; set; }
    public string? HeadCommit { get; set; }
    public string? CandidateCommit { get; set; }
    public string? CandidateBase { get; set; }
    public bool CandidatePassed { get; set; }
    public bool NeedsSync { get; set; }
    public int Attempts { get; set; }
    public int PreparationRepairs { get; set; }
    public string? PlanRepairRole { get; set; }
    public string? RepairStage { get; set; }
    public bool VerificationPreparation { get; set; }
    public string? LastBlockerFingerprint { get; set; }
    public ReviewState? Review { get; set; }
    public int ReviewRounds { get; set; }
    public string? LastReviewFingerprint { get; set; }
    public DiagnosisState? Diagnosis { get; set; }
    public TaskVerificationContext? PendingVerification { get; set; }
    public int DiagnosisRounds { get; set; }
    public string? LastDiagnosisFingerprint { get; set; }
    public string? Failure { get; set; }
    public string? ImplementationPlanId { get; set; }
    public string? DependencyRepairId { get; set; }
    public int DependencyRepairRounds { get; set; }
    public string? TestPlanId { get; set; }
    public DateTimeOffset? Heartbeat { get; set; }
    public DateTimeOffset? StartedAt { get; set; }
    public DateTimeOffset? FinishedAt { get; set; }
}

public sealed class RunData
{
    public required string Id { get; set; }
    public required RunRequest Request { get; set; }
    public required string InitialCommit { get; set; }
    public required string PolicyJson { get; set; }
    public required string PolicyHash { get; set; }
    public string State { get; set; } = "Queued";
    public string Phase { get; set; } = "Specification";
    public string? PreviousState { get; set; }
    public string? Failure { get; set; }
    public string? SpecificationId { get; set; }
    public string? ApprovedSpecificationId { get; set; }
    public string? ApprovedSpecificationHash { get; set; }
    public string? Feedback { get; set; }
    public string? PendingUnblockId { get; set; }
    public string? OperatorInstructionId { get; set; }
    public string? InspectionWorkspace { get; set; }
    public string? InspectionCommit { get; set; }
    public bool BaselinePassed { get; set; }
    public string? FinalCommit { get; set; }
    public DateTimeOffset Created { get; set; } = DateTimeOffset.UtcNow;
    public DateTimeOffset Updated { get; set; } = DateTimeOffset.UtcNow;
    public long Revision { get; set; }
    public double ActiveSeconds { get; set; }
    public long? ReportedTokens { get; set; }
    public int Replans { get; set; }
    public List<TaskData> Tasks { get; set; } = [];
}

public sealed class AgentOptions
{
    public HostOptions Host { get; set; } = new();
    public ExecutionOptions Execution { get; set; } = new();
    public CopilotOptions Copilot { get; set; } = new();
}

public sealed class HostOptions
{
    public int Port { get; set; } = 47831;
}

public sealed class ExecutionOptions
{
    public int MaxPairs { get; set; } = 2;
    public int TransientRetries { get; set; } = 3;
    public int RetryBackoffSeconds { get; set; } = 2;
    public int MaxRepairAttempts { get; set; } = 8;
    public int CommandTimeoutSeconds { get; set; } = 600;
    public int AgentTimeoutSeconds { get; set; } = 900;
    public double? MaxRunHours { get; set; }
    public long? MaxReportedTokens { get; set; }
    public int RawLogRetentionDays { get; set; } = 30;
    public long MaxArtifactBytes { get; set; } = 1073741824;
}

public sealed class CopilotOptions
{
    public string? CliPath { get; set; }
    public string? Model { get; set; }
}

public class PolicyException(string message) : Exception(message);
public sealed class BudgetException(string message) : Exception(message);
public sealed class WorkflowException(string message) : Exception(message);
public sealed class WorkerProtocolException(string message) : Exception(message);
public sealed class WorkspaceStateException(string message) : Exception(message);
public sealed record ContractViolation(string Code, string Section, string? StepId, string Field, string Message, string[] AllowedValues);
public sealed class PlanContractException(ContractViolation violation) : Exception(violation.Message)
{
    public ContractViolation Violation { get; } = violation;
}
public sealed class StepContractException(ContractViolation violation) : Exception(violation.Message)
{
    public ContractViolation Violation { get; } = violation;
}
public sealed class StepFailureException(StepBlocker blocker) : Exception(blocker.Reason)
{
    public StepBlocker Blocker { get; } = blocker;
}
public sealed class WorkerContractException(string message) : Exception(message);
public sealed class PreparationException(string message) : Exception(message);
public sealed class ReviewPendingException : Exception;
public sealed class DiagnosisPendingException : Exception;
public sealed class ToolInputException(string message) : PolicyException(message);

public sealed record AuditContext(string? AssignmentId = null, string? Role = null,
    string? PlanId = null, string? StepId = null, int? Attempt = null);

public static class AuditScope
{
    private static readonly AsyncLocal<AuditContext?> current = new();
    public static AuditContext Current => current.Value ?? new();
    public static IDisposable Use(AuditContext context)
    {
        var old = current.Value;
        current.Value = context;
        return new Restore(() => current.Value = old);
    }
    private sealed class Restore(Action restore) : IDisposable { public void Dispose() => restore(); }
}

public static class Contracts
{
    public static StepBlocker ResolveSetupOwner(StepBlocker blocker, WorkDefinition task)
    {
        if (blocker.RequiredPath is not { } path) return blocker;
        if (blocker.Kind == BlockerKind.TestSetupFailure && SafePaths.InScope(path, task.CodePaths))
            return blocker with { Kind = BlockerKind.MissingScaffold };
        if (blocker.Kind == BlockerKind.MissingScaffold && SafePaths.InScope(path, task.TestPaths))
            return blocker with { Kind = BlockerKind.TestSetupFailure };
        return blocker;
    }

    public static void ValidateChecks(CheckCommand[] checks)
    {
        if (checks is null || checks.Length == 0 || checks.Any(c => c is null)
            || checks.Select(c => c.Name).Distinct(StringComparer.Ordinal).Count() != checks.Length)
            throw new InvalidDataException("Provide uniquely named gating checks.");
        foreach (var check in checks)
        {
            if (string.IsNullOrWhiteSpace(check.Name) || string.IsNullOrWhiteSpace(check.FileName)
                || !new[] { "build", "test", "format", "validation", "restore" }.Contains(check.Kind)
                || check.Arguments is null || check.Arguments.Any(a => a is null))
                throw new InvalidDataException("Each check needs a name, supported kind, executable, and argument array.");
            if (new[] { check.FileName }.Concat(check.Arguments).Any(value => Redaction.Apply(value) != value))
                throw new PolicyException("Check executables/arguments must not contain inline credentials. Use the repository tool's normal authorized authentication mechanism.");
        }
    }

    public static void ValidateSpec(Specification spec, CheckCommand[] checks, bool forApproval)
    {
        if (string.IsNullOrWhiteSpace(spec.Title) || string.IsNullOrWhiteSpace(spec.Problem)
            || string.IsNullOrWhiteSpace(spec.Scope) || string.IsNullOrWhiteSpace(spec.Approach)
            || string.IsNullOrWhiteSpace(spec.RepositoryFindings) || spec.NonGoals is null || spec.Questions is null
            || spec.PlannedTestCount is < 0 || spec.Criteria is null || spec.Criteria.Length == 0 || spec.Criteria.Any(c => c is null)
            || spec.Criteria.Select(c => c.Id).Distinct().Count() != spec.Criteria.Length)
            throw new InvalidDataException("Specification must have findings, scope, approach, and unique measurable criteria.");
        foreach (var c in spec.Criteria)
            if (string.IsNullOrWhiteSpace(c.Id) || string.IsNullOrWhiteSpace(c.Outcome)
                || string.IsNullOrWhiteSpace(c.Measurement) || c.Checks is null || c.Checks.Length == 0
                || c.Checks.Any(name => !checks.Any(check => check.Name == name)))
                throw new InvalidDataException("Each criterion must reference authorized checks and state its measurement.");
        if (forApproval && spec.Questions.Length != 0)
            throw new InvalidDataException("Resolve specification questions with feedback before approval.");
    }

    public const int WorkerContractVersion = 3;
    public const int MaxReviewRounds = 3;

    public static string RoleFor(WorkerStage stage) => stage is WorkerStage.Scaffold or WorkerStage.Implementation ? "SpecImplementer" : "Tester";

    public static string[] CheckNamesFor(WorkerStage stage, WorkDefinition task, CheckCommand[] checks) =>
        checks.Where(c => task.Checks.Contains(c.Name) && (stage is WorkerStage.Implementation or WorkerStage.Verify or WorkerStage.ReviewRepair or WorkerStage.VerificationPreparation || c.Kind is "restore" or "build"))
            .Select(c => c.Name).ToArray();

    public static System.Text.Json.Nodes.JsonNode ResultSchema<T>()
    {
        var options = new JsonSerializerOptions(Json.Options)
        {
            TypeInfoResolver = new System.Text.Json.Serialization.Metadata.DefaultJsonTypeInfoResolver()
        };
        var schema = System.Text.Json.Schema.JsonSchemaExporter.GetJsonSchemaAsNode(options, typeof(T));
        if (typeof(T) == typeof(ImplementerPlan) || typeof(T) == typeof(TesterPlan))
            schema["properties"]!["contractVersion"]!["const"] = WorkerContractVersion;
        if (typeof(T) == typeof(Decomposition))
        {
            var task = schema["properties"]!["tasks"]!["items"]!;
            var obligations = task["properties"]!["obligations"]!;
            obligations["type"] = "array"; obligations["minItems"] = 1;
            obligations.AsObject().Remove("default");
            task["required"]!.AsArray().Add("obligations");
        }
        return schema;
    }

    public static void ValidatePlan(ImplementerPlan plan, string role, WorkDefinition task, CheckCommand[] checks) =>
        ValidateSections(plan.ContractVersion, plan.Summary, plan.Risks, role, "SpecImplementer", task.CodePaths, task, checks,
            [(WorkerStage.Scaffold, "scaffoldingSteps", plan.ScaffoldingSteps), (WorkerStage.Implementation, "implementationSteps", plan.ImplementationSteps)]);

    public static void ValidatePlan(TesterPlan plan, string role, WorkDefinition task, CheckCommand[] checks, bool verificationPreparation = false)
    {
        if (plan.VerificationSteps is null || plan.VerificationSteps.Any(s => s is null))
            Reject("InvalidSteps", "verificationSteps", null, "verificationSteps", "Verification steps must be an explicit non-null array.");
        ValidateSections(plan.ContractVersion, plan.Summary, plan.Risks, role, "Tester", task.TestPaths, task, checks,
            [(verificationPreparation ? WorkerStage.VerificationPreparation : WorkerStage.Tests, "preparationSteps", plan.PreparationSteps), (WorkerStage.Verify, "verificationSteps", plan.VerificationSteps.Select(s => s.AsStep()).ToArray())]);
        if (verificationPreparation && plan.PreparationSteps.Count(s => s.WritePaths.Length != 0) > 1)
            Reject("NonAtomicVerificationProbe", "preparationSteps", null, "writePaths", "Verification probes must edit, build, observe the expected failure, restore exact original content and rebuild/retest green in ONE writable step. Separate edit/revert steps are unsafe. Commit is disabled; read-only supporting steps are allowed.");
    }

    private static void ValidateSections(int version, string summary, string[] risks, string role, string expectedRole,
        string[] allowedPaths, WorkDefinition task, CheckCommand[] checks, (WorkerStage Stage, string Name, PlanStep[] Steps)[] sections)
    {
        if (version != WorkerContractVersion)
            Reject("ContractVersion", "plan", null, "contractVersion", "Executable plans require contractVersion=3. Historical plans require explicit regeneration; never silently migrate or execute them.", ["3"]);
        if (role != expectedRole)
            Reject("RoleMismatch", "plan", null, "role", $"{role} cannot produce a {expectedRole} plan.", [expectedRole]);
        if (string.IsNullOrWhiteSpace(summary) || risks is null || risks.Any(string.IsNullOrWhiteSpace))
            Reject("InvalidSummary", "plan", null, "summary/risks", "Provide a meaningful summary and an explicit risks array.");
        var ids = new HashSet<string>(StringComparer.Ordinal);
        foreach (var (stage, section, steps) in sections)
        {
            if (steps is null || steps.Length == 0)
                Reject("MissingSection", section, null, section, $"{section} requires at least one step; existing setup may use a read-only inspection.");
            foreach (var step in steps)
            {
                if (step is null || string.IsNullOrWhiteSpace(step.Id) || !ids.Add(step.Id) || ids.Count > 30
                    || string.IsNullOrWhiteSpace(step.Action) || string.IsNullOrWhiteSpace(step.ExpectedOutcome) || string.IsNullOrWhiteSpace(step.Verification))
                    Reject("InvalidStep", section, step?.Id, "step", "Plans require 1-30 unique steps with actions, observable outcomes, and completion conditions.");
                if (step.WritePaths is null || step.WritePaths.Any(string.IsNullOrWhiteSpace))
                    Reject("MissingWritePaths", section, step.Id, "writePaths", "Declare writePaths explicitly; [] means read-only.", allowedPaths);
                foreach (var path in step.WritePaths)
                {
                    try { SafePaths.ValidateRelative(path); }
                    catch (PolicyException ex) { Reject("UnsafePath", section, step.Id, "writePaths", ex.Message, allowedPaths); }
                    if (!SafePaths.InScope(path, allowedPaths))
                        Reject("WriteScope", section, step.Id, "writePaths", $"{role} step {step.Id} requests {path} outside its assigned paths. Do not broaden permissions or transfer work to another role.", allowedPaths);
                }
                var names = CheckNamesFor(stage, task, checks);
                if (step.Checks is null || step.Checks.Any(name => !names.Contains(name)) || step.Checks.Distinct().Count() != step.Checks.Length)
                    Reject("CheckScope", section, step.Id, stage == WorkerStage.Verify ? "passChecks" : "checks",
                        $"{section} may declare only its authorized successful checks. The orchestrator alone runs/assesses preimplementation red; it is never a final verification step.", names);
            }
        }
    }

    [System.Diagnostics.CodeAnalysis.DoesNotReturn]
    private static void Reject(string code, string section, string? step, string field, string message, string[]? allowed = null) =>
        throw new PlanContractException(new(code, section, step, field, message, allowed ?? []));

    public static void ValidateStepResult(StepResult result, StepAssignment assignment)
    {
        [System.Diagnostics.CodeAnalysis.DoesNotReturn]
        void RejectResult(string code, string field, string message, string[]? allowed = null) =>
            throw new StepContractException(new(code, assignment.Stage.ToString(), assignment.CurrentStep.Id, field, message, allowed ?? []));
        if (result.AssignmentId != assignment.AssignmentId || result.PlanId != assignment.PlanId || result.StepId != assignment.CurrentStep.Id)
            RejectResult("StepIdentity", "assignmentId/planId/stepId", "Result must identify exactly the current assignment, saved plan, and step.", [assignment.AssignmentId, assignment.PlanId, assignment.CurrentStep.Id]);
        if (!Enum.IsDefined(result.Status) || result.Blocker is { } typedBlocker && !Enum.IsDefined(typedBlocker.Kind))
            RejectResult("InvalidStatus", "status/blocker.kind", "Use a supported step status and structured blocker kind.");
        if (string.IsNullOrWhiteSpace(result.Summary) || result.EvidenceIds is null || result.EvidenceIds.Any(string.IsNullOrWhiteSpace))
            RejectResult("InvalidResult", "summary/evidenceIds", "Report a meaningful current-step summary and an explicit evidenceIds array.");
        if (result.Status == StepStatus.Completed && (result.Blocker is not null || result.EvidenceIds.Length == 0))
            RejectResult("CompletionEvidence", "status/blocker/evidenceIds", "Completion needs observed current-step evidence and no blocker. Later steps are not assigned work.");
        if (result.Status == StepStatus.ReviewRequested)
        {
            if (result.Challenge is null || result.Blocker is not null || result.EvidenceIds.Length == 0)
                RejectResult("ReviewEvidence", "challenge/evidenceIds", "Review requests require a structured challenge, observed evidence, and no competing blocker.");
            if (assignment.Stage == WorkerStage.ReviewRepair)
                RejectResult("NestedReview", "challenge", "A review repair cannot recursively open another dispute; report its unmet condition for bounded escalation.");
        }
        else if (result.Challenge is not null)
            RejectResult("UnexpectedChallenge", "challenge", "Only reviewRequested results may carry a challenge.");
        if (result.Status is StepStatus.Blocked or StepStatus.Failed && (result.Blocker is null || string.IsNullOrWhiteSpace(result.Blocker.Reason)))
            RejectResult("MissingBlocker", "blocker", "An incomplete step requires a structured blocker describing the current step's unmet condition.");
        if (result.Blocker?.RequiredPath is { } requiredPath)
            try { SafePaths.ValidateRelative(requiredPath); }
            catch (PolicyException ex) { RejectResult("UnsafeBlockerPath", "blocker.requiredPath", ex.Message); }
        if (result.Blocker is { Kind: BlockerKind.CapabilityMismatch } blocker)
        {
            if (blocker.RequiredPath is null && blocker.RequiredCheck is null)
                RejectResult("MissingCapability", "blocker", "Identify the current-step path/check requiring a missing capability; future steps are not blockers.");
            if (blocker.RequiredPath is { } path && !SafePaths.InScope(path, assignment.CurrentStep.WritePaths))
                RejectResult("FutureWriteNotAssigned", "blocker.requiredPath", "The current step does not assign this write. Lack of tools for later writing steps is expected, not failure. If the inspected API is absent, report missingScaffold with factual evidence instead.", assignment.CurrentStep.WritePaths);
            if (blocker.RequiredCheck is { } check && !assignment.CurrentStep.Checks.Contains(check))
                RejectResult("FutureCheckNotAssigned", "blocker.requiredCheck", "This check is not a current-step completion condition; later checks cannot block this step.", assignment.CurrentStep.Checks);
        }
    }

    public static void ValidateTasks(WorkDefinition[] tasks, Specification spec, CheckCommand[] checks)
    {
        if (tasks is null || tasks.Length is < 1 or > 100 || tasks.Any(t => t is null) || tasks.Select(t => t.Id).Distinct().Count() != tasks.Length)
            throw new InvalidDataException("Decomposition requires 1-100 uniquely identified tasks.");
        foreach (var task in tasks)
        {
            if (task.Id is null || !System.Text.RegularExpressions.Regex.IsMatch(task.Id, "^[a-z][a-z0-9-]{0,39}$")
                || string.IsNullOrWhiteSpace(task.Title) || string.IsNullOrWhiteSpace(task.Objective)
                || task.CriterionIds is null || task.Dependencies is null || task.CodePaths is null || task.TestPaths is null || task.Checks is null
                || task.CriterionIds.Length == 0 || task.CriterionIds.Any(id => !spec.Criteria.Any(c => c.Id == id))
                || task.Checks.Length == 0 || task.Checks.Any(name => !checks.Any(c => c.Name == name))
                || task.Dependencies.Any(id => id == task.Id || !tasks.Any(t => t.Id == id))
                || task.CodePaths.Length == 0 || task.TestPaths.Length == 0
                || (!task.TestFirst && string.IsNullOrWhiteSpace(task.NonTddReason)))
                throw new InvalidDataException($"Invalid task contract: {task.Id}.");
            if (task.Checks.Any(name => checks.Single(c => c.Name == name).Kind == "test")
                && checks.Where(c => c.Kind is "restore" or "build").Any(c => !task.Checks.Contains(c.Name)))
                throw new InvalidDataException($"Task {task.Id} must include the approved restore/build prerequisites for its tests.");
            foreach (var path in task.CodePaths.Concat(task.TestPaths)) SafePaths.ValidateRelative(path);
            if (SafePaths.Overlap(task.CodePaths, task.TestPaths))
                throw new InvalidDataException("Implementation and test write scopes must be disjoint.");
            ValidateObligations(task);
        }
        if (spec.Criteria.Any(c => !tasks.Any(t => t.CriterionIds.Contains(c.Id))))
            throw new InvalidDataException("Decomposition does not cover every parent criterion.");
        var visited = new HashSet<string>();
        while (visited.Count < tasks.Length)
        {
            var ready = tasks.Where(t => !visited.Contains(t.Id) && t.Dependencies.All(visited.Contains)).ToArray();
            if (ready.Length == 0) throw new InvalidDataException("Task dependencies contain a cycle.");
            foreach (var task in ready) visited.Add(task.Id);
        }
    }

    public static void ValidateObligations(WorkDefinition task)
    {
        if (task.Obligations is null || task.Obligations.Length == 0 || task.Obligations.Any(o => o is null)
            || task.Obligations.Select(o => o.Id).Distinct().Count() != task.Obligations.Length
            || task.CriterionIds.Distinct().Count() != task.CriterionIds.Length
            || task.CriterionIds.Except(task.Obligations.Select(o => o.CriterionId)).Any()
            || task.Obligations.Any(o => !task.CriterionIds.Contains(o.CriterionId)))
            throw new InvalidDataException($"Task {task.Id} needs explicit, uniquely identified per-task obligations for exactly its referenced criteria. Historical tasks without obligations require replanning; do not infer whole-criterion duties.");
        foreach (var obligation in task.Obligations)
        {
            if (string.IsNullOrWhiteSpace(obligation.Id) || string.IsNullOrWhiteSpace(obligation.Outcome) || string.IsNullOrWhiteSpace(obligation.Measurement)
                || obligation.Paths is null || obligation.Paths.Length == 0 || obligation.Checks is null || obligation.Checks.Length == 0
                || obligation.Checks.Any(c => !task.Checks.Contains(c)))
                throw new InvalidDataException($"Obligation {obligation.Id} needs a bounded outcome, measurement, owned paths and authorized checks.");
            foreach (var path in obligation.Paths)
            {
                SafePaths.ValidateRelative(path);
                if (!SafePaths.InScope(path, task.CodePaths.Concat(task.TestPaths)))
                    throw new InvalidDataException($"Obligation {obligation.Id} depends on {path} outside task {task.Id}'s owned paths. Split the obligation and assign the remaining duty to its owning task; never require downstream work to complete an upstream task.");
            }
        }
    }

    public static void ValidateTaskVerificationContext(TaskVerificationContext context, RunData run, TaskData task)
    {
        if (task.State != "Running" || task.Stage != "Verify" || task.PendingVerification is null
            || Json.Write(task.PendingVerification) != Json.Write(context) || task.AssignmentId != context.AssignmentId
            || task.Definition.Id != context.TaskId || context.SpecificationId != run.ApprovedSpecificationId
            || context.SpecificationHash != run.ApprovedSpecificationHash || run.ApprovedSpecificationId != run.SpecificationId)
            throw new WorkerContractException("Stale task-verification message, assignment, role/stage or approved specification.");
        ValidateObligations(task.Definition);
    }

    public static void ValidateTaskVerification(TaskVerificationContext expected, TaskVerificationResult result, RunData run, TaskData task)
    {
        ValidateTaskVerificationContext(expected, run, task);
        if (Json.Write(result.Context) != Json.Write(expected)) throw new InvalidDataException("Task verification must reproduce the exact current context, including revision and messageId.");
        if (result.Obligations is null || result.Obligations.Any(o => o is null) || string.IsNullOrWhiteSpace(result.Explanation)
            || result.Obligations.Select(o => o.Id).Distinct().Count() != result.Obligations.Length
            || !result.Obligations.Select(o => o.Id).Order().SequenceEqual(task.Definition.Obligations!.Select(o => o.Id).Order()))
            throw new InvalidDataException("Return one assessment for every assigned obligation ID, and no whole-criterion or downstream-task assessments. Missing evidence must be notVerified, not fabricated success.");
        foreach (var obligation in result.Obligations)
            if (!Enum.IsDefined(obligation.Status) || string.IsNullOrWhiteSpace(obligation.Explanation) || obligation.EvidenceIds is null
                || obligation.EvidenceIds.Any(string.IsNullOrWhiteSpace) || obligation.Status != ObligationStatus.NotVerified && obligation.EvidenceIds.Length == 0)
                throw new InvalidDataException("Each assessed obligation needs its status, explanation and actual evidence IDs; unverified obligations must say notVerified.");
        if (result.Passed != result.Obligations.All(o => o.Status == ObligationStatus.Satisfied))
            throw new InvalidDataException("Task passed must equal whether all assigned obligations are satisfied, not whether all parent criteria or the entire goal are complete.");
    }

    public static void ValidateStepAssignment(StepAssignment execution, TaskData task, string savedPlan, CheckCommand[] checks)
    {
        if (execution.Stage == WorkerStage.ReviewRepair)
        {
            var review = task.Review ?? throw new WorkerContractException("No pending review authorizes repair.");
            var plan = Json.Read<ReviewRepairPlan>(savedPlan);
            ValidateRepairPlan(plan, task, checks);
            if (task.State != "Running" || task.Stage != "Review" || review.Stage != "Repairing" || review.RepairRole != execution.Role
                || review.RepairPlanId != execution.PlanId || task.AssignmentId != execution.AssignmentId
                || review.RepairStepIndex < 0 || review.RepairStepIndex >= plan.Steps.Length
                || Json.Write(plan.Steps[review.RepairStepIndex]) != Json.Write(execution.CurrentStep))
                throw new WorkerContractException("Stale or modified review-repair assignment.");
            return;
        }
        var id = execution.Role == "Tester" ? task.TestPlanId : task.ImplementationPlanId;
        PlanStep[] steps;
        try
        {
            if (execution.Stage is WorkerStage.Scaffold or WorkerStage.Implementation)
            {
                var plan = Json.Read<ImplementerPlan>(savedPlan);
                ValidatePlan(plan, execution.Role, task.Definition, checks);
                steps = execution.Stage == WorkerStage.Scaffold ? plan.ScaffoldingSteps : plan.ImplementationSteps;
            }
            else if (execution.Stage is WorkerStage.Tests or WorkerStage.Verify or WorkerStage.VerificationPreparation)
            {
                if (execution.Stage == WorkerStage.VerificationPreparation && !task.VerificationPreparation)
                    throw new WorkerContractException("Verification preparation requires a recorded missing-evidence repair.");
                var plan = Json.Read<TesterPlan>(savedPlan);
                ValidatePlan(plan, execution.Role, task.Definition, checks, task.VerificationPreparation);
                steps = execution.Stage is WorkerStage.Tests or WorkerStage.VerificationPreparation ? plan.PreparationSteps : plan.VerificationSteps.Select(s => s.AsStep()).ToArray();
            }
            else throw new WorkerContractException("Unsupported worker stage.");
        }
        catch (Exception ex) when (ex is JsonException or PlanContractException)
        { throw new WorkerContractException("Historical or invalid saved plan cannot authorize execution: " + ex.Message); }
        if (task.State != "Running" || task.Stage != execution.Stage.ToString() || execution.Role != RoleFor(execution.Stage)
            || task.AssignmentId != execution.AssignmentId || id != execution.PlanId || task.StepIndex < 0 || task.StepIndex >= steps.Length
            || Json.Write(steps[task.StepIndex]) != Json.Write(execution.CurrentStep))
            throw new WorkerContractException("Execution must match the persisted current step, role, stage, plan, assignment, and cursor. Stale or modified step assignments cannot execute.");
    }

    public static void ValidateChallenge(ReviewChallenge challenge, WorkDefinition task)
    {
        if (!Enum.IsDefined(challenge.Target) || string.IsNullOrWhiteSpace(challenge.Claim)
            || challenge.CriterionIds is null || challenge.CriterionIds.Length == 0 || challenge.CriterionIds.Any(id => !task.CriterionIds.Contains(id))
            || challenge.Paths is null || challenge.Paths.Any(string.IsNullOrWhiteSpace))
            throw new InvalidDataException("A challenge needs a claim, assigned criterion IDs, and explicit repository-relative subject paths.");
        foreach (var path in challenge.Paths)
        {
            SafePaths.ValidateRelative(path);
            var scope = challenge.Target == ReviewTarget.Test ? task.TestPaths : challenge.Target == ReviewTarget.Implementation ? task.CodePaths : task.CodePaths.Concat(task.TestPaths);
            if (!SafePaths.InScope(path, scope)) throw new InvalidDataException("Challenge subject is outside the assigned role's work-item scope; request dependency repair instead.");
        }
    }

    public static void ValidateReviewAssignment(ReviewAssignment context, RunData run, TaskData task, string role)
    {
        var review = task.Review;
        if (review is null || task.State != "Running" || task.Stage != "Review" || review.PendingMessage is null
            || Json.Write(review.PendingMessage) != Json.Write(context) || context.ReviewId != review.Id || context.Revision != review.Revision
            || context.AssignmentId != task.AssignmentId
            || context.Stage != review.Stage || context.Role != role || context.SpecificationId != run.ApprovedSpecificationId
            || run.ApprovedSpecificationId != run.SpecificationId)
            throw new WorkerContractException("Stale review message, decision, source revision, role, or specification version. Reply cannot advance this review.");
    }

    public static void ValidateReviewReply(ReviewAssignment expected, ReviewAssignment actual, RunData run, TaskData task,
        string explanation, RequirementCitation[] citations, string[] evidenceIds)
    {
        ValidateReviewAssignment(expected, run, task, expected.Role);
        if (citations is not null && citations.Any(c => c is not null && !task.Definition.CriterionIds.Contains(c.CriterionId)))
            throw new InvalidDataException("Review citations must use only assigned criterionIds: " + string.Join(", ", task.Definition.CriterionIds)
                + ". Unassigned citations: " + string.Join(", ", citations.Where(c => c is not null && !task.Definition.CriterionIds.Contains(c.CriterionId)).Select(c => c.CriterionId)) + ". Quote their approved text exactly; do not invent wording.");
        if (Json.Write(expected) != Json.Write(actual) || string.IsNullOrWhiteSpace(explanation)
            || citations is null || citations.Length == 0 || citations.Any(c => c is null || !task.Definition.CriterionIds.Contains(c.CriterionId) || string.IsNullOrWhiteSpace(c.Quote) || c.Quote.Length < 12)
            || evidenceIds is null || evidenceIds.Length == 0 || evidenceIds.Any(string.IsNullOrWhiteSpace))
            throw new InvalidDataException("Review replies must match the exact pending message and cite approved requirements plus observed evidence.");
    }

    public static void ValidateRepairPlan(ReviewRepairPlan plan, TaskData task, CheckCommand[] checks)
    {
        var review = task.Review ?? throw new WorkerContractException("A repair plan requires an active review.");
        if (plan.Context.ReviewId != review.Id || plan.Context.Role != review.RepairRole || review.RepairRole is not ("Tester" or "SpecImplementer")
            || string.IsNullOrWhiteSpace(plan.Summary) || plan.Steps is null || plan.Steps.Length is < 1 or > 10)
            Reject("InvalidReviewRepair", "reviewRepair", null, "steps", "Review repair requires 1-10 scoped steps owned by the designated implementer or tester.");
        ValidateSections(WorkerContractVersion, plan.Summary, [], review.RepairRole, review.RepairRole,
            review.RepairRole == "Tester" ? task.Definition.TestPaths : task.Definition.CodePaths, task.Definition, checks,
            [(WorkerStage.ReviewRepair, "reviewRepair", plan.Steps)]);
    }
}
