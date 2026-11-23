using System.Globalization;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace SDLCAgent;

public sealed record TestCounts(int Passed, int Total, int Failed, int Skipped = 0);
public sealed record TestStatus(TestCounts? Counts, string? EvidenceId, string? TaskId, string? Revision,
    DateTimeOffset? At, int? ExitCode, bool TimedOut, string Description);
public sealed record PlanHistory(string Id, string TaskId, string Role, DateTimeOffset Created, DateTimeOffset? LastStepAt);
public sealed record PlanStatus(PlanHistory Plan, bool Active, bool Current, DateTimeOffset? FinishedAt);
public sealed record PairStatus(string TaskId, string Title, string? Branch, DateTimeOffset? StartedAt, string? Role, string Stage);
public sealed record BranchStatus(string Branch, string? Head, long? TotalCommits, long? AddedCommits, string? Error);
public sealed record BranchCommit(string Hash, DateTimeOffset At, string Subject, string? Role);
public sealed record BranchHistory(string Branch, BranchCommit[] Commits, int? NextOffset);
public sealed record ExecutionCommit(long EventId, string Hash, DateTimeOffset At, string Subject, string? TaskId, string? Role);
public sealed record ExecutionHistory(ExecutionCommit[] Commits, long? NextBefore);
public sealed record RunDashboard(RunData Run, int ItemCompleteCount, int TotalItemCount, PairStatus[] ActivePairs,
    WorkDefinition[] Backlog, PlanStatus[] Plans, TestStatus Tests, int? PlannedTestCount,
    AuditEvent? AttentionEvent, BranchStatus Branch);

public static partial class TestOutput
{
    [GeneratedRegex(@"(?m)^\s*(?<total>\d+) tests?,\s*(?<passed>\d+) passed,\s*(?<failed>\d+) failed(?:,\s*(?<skipped>\d+) skipped)?\s*$", RegexOptions.CultureInvariant)]
    private static partial Regex ConsoleSummary();

    [GeneratedRegex(@"(?m)^(?:Passed!|Failed!)\s*-\s*Failed:\s*(?<failed>\d+),\s*Passed:\s*(?<passed>\d+),\s*Skipped:\s*(?<skipped>\d+),\s*Total:\s*(?<total>\d+)\b", RegexOptions.CultureInvariant)]
    private static partial Regex DotnetSummary();

    public static TestCounts? Parse(string output)
    {
        var matches = ConsoleSummary().Matches(output).Concat(DotnetSummary().Matches(output)).OrderBy(m => m.Index).ToArray();
        if (matches.Length == 0) return null;
        var match = matches[^1];
        int? Count(string key) => !match.Groups[key].Success ? 0 : int.TryParse(match.Groups[key].Value, CultureInfo.InvariantCulture, out var value) ? value : null;
        if (Count("passed") is not { } passed || Count("total") is not { } total || Count("failed") is not { } failed || Count("skipped") is not { } skipped) return null;
        var counts = new TestCounts(passed, total, failed, skipped);
        return (long)counts.Passed + counts.Failed + counts.Skipped == counts.Total ? counts : null;
    }
}

public sealed class Dashboard(Store store, ProcessRunner processes, GitWorkspace git)
{
    public ExecutionHistory ExecutionCommits(string id, long before)
    {
        store.Get(id);
        var events = store.Events(id, before: before, limit: 51, kind: "LogicalCommit");
        var page = events.Take(50).Select(e =>
        {
            var separator = e.Message.IndexOf(": ", StringComparison.Ordinal);
            if (separator < 0) throw new InvalidDataException("Malformed recorded execution commit.");
            return new ExecutionCommit(e.Id, e.Message[..separator], e.At, e.Message[(separator + 2)..], e.TaskId, e.Role);
        }).ToArray();
        return new(page, events.Length > 50 ? page[^1].EventId : null);
    }

    public async Task<RunDashboard> Get(string id, CancellationToken token)
    {
        var run = store.Get(id);
        var milestones = store.DashboardEvents(id, run.Request.Checks.Where(c => c.Kind == "test").Select(c => c.Name).ToArray());
        AuditEvent? Last(string taskId, params string[] kinds) => milestones.FirstOrDefault(e => e.TaskId == taskId && kinds.Contains(e.Kind));
        var active = run.Tasks.Where(t => t.State == "Running").Select(t => new PairStatus(t.Definition.Id, t.Definition.Title,
            t.Branch, t.StartedAt ?? Last(t.Definition.Id, "PairAssigned")?.At, Last(t.Definition.Id, "AgentStarted")?.Role, t.Stage)).ToArray();
        var plans = store.PlanHistory(id).Select(plan =>
        {
            var task = run.Tasks.SingleOrDefault(t => t.Definition.Id == plan.TaskId);
            var current = task is not null && (task.ImplementationPlanId == plan.Id || task.TestPlanId == plan.Id || task.Review?.RepairPlanId == plan.Id);
            var isActive = current && task!.State == "Running";
            var finished = isActive ? null : current ? task?.FinishedAt ?? Last(plan.TaskId, "PairVerified", "TaskIntegrated", "WorkerProtocolBlocked", "WorkspaceBlocked", "WorkerContractBlocked", "PreparationRepairRequired", "PairRepairRequired")?.At ?? plan.LastStepAt : plan.LastStepAt;
            return new PlanStatus(plan, isActive, current, finished);
        }).OrderByDescending(p => p.Active).ThenByDescending(p => p.FinishedAt ?? p.Plan.Created).ToArray();
        var attentionKinds = run.State == "Blocked" ? new[] { "WorkerProtocolBlocked", "WorkspaceBlocked", "WorkerContractBlocked", "PreparationRepairRequired", "PairRepairRequired", "IntegrationRejected", "RunBlocked" }
            : new[] { "RunLimitReached", "PauseRequested", "RunPaused" };
        var attention = run.State == "Blocked" ? milestones.FirstOrDefault(e => e.TaskId is not null && attentionKinds.Contains(e.Kind)
            && run.Tasks.Any(t => t.Definition.Id == e.TaskId && t.State == "Blocked")) : null;
        attention ??= milestones.FirstOrDefault(e => attentionKinds.Contains(e.Kind));
        var tests = ReadTests(milestones.FirstOrDefault(e => e.Kind == "CommandFinished"));
        var planned = run.ApprovedSpecificationId is null ? null : Json.Read<Specification>(store.ReadArtifact(run.ApprovedSpecificationId)).PlannedTestCount;
        BranchStatus branch;
        try
        {
            var head = await git.BranchHead(run.Request.Repository, run.Request.Branch, token);
            var total = await GitWorkspace.Git(processes, run.Request.Repository, token, "rev-list", "--count", head);
            var added = await GitWorkspace.Git(processes, run.Request.Repository, token, "rev-list", "--count", run.InitialCommit + ".." + head);
            branch = new(run.Request.Branch, head, long.Parse(total, CultureInfo.InvariantCulture), long.Parse(added, CultureInfo.InvariantCulture), null);
        }
        catch (Exception ex) when (ex is WorkflowException or IOException)
        {
            branch = new(run.Request.Branch, null, null, null, Redaction.Apply(ex.Message));
        }
        return new(run, run.Tasks.Count(t => t.State == "Integrated"), run.Tasks.Count, active,
            run.Tasks.Where(t => t.State == "Pending" && t.Attempts == 0).Select(t => t.Definition).ToArray(), plans, tests, planned, attention, branch);
    }

    private TestStatus ReadTests(AuditEvent? latest)
    {
        if (latest?.DocumentId is not { } id) return new(null, null, null, null, null, null, false, "No test execution evidence yet.");
        using var document = JsonDocument.Parse(store.ReadArtifact(id));
        var data = document.RootElement;
        var truncated = data.TryGetProperty("outputTruncated", out var clipping) && clipping.GetBoolean();
        var sourceChanged = data.TryGetProperty("sourceChanged", out var changed) && changed.GetBoolean();
        var counts = truncated || sourceChanged ? null : TestOutput.Parse(data.GetProperty("output").GetString() ?? "");
        return new(counts, id, latest.TaskId, data.GetProperty("revision").GetString(), latest.At,
            data.GetProperty("exitCode").GetInt32(), data.GetProperty("timedOut").GetBoolean(),
            sourceChanged ? "Latest test changed the source snapshot; its output is not valid test evidence." : truncated ? "Latest test output was truncated; counts are unknown." : counts is null ? "Latest test command did not report recognized test counts." : "Latest reported suite only; not a sum of repeated runs or parallel branches.");
    }

    public async Task<BranchHistory> Commits(string id, string? taskId, int offset, CancellationToken token)
    {
        if (offset < 0) throw new InvalidDataException("Commit offset must be non-negative.");
        var run = store.Get(id);
        var task = taskId is null ? null : run.Tasks.SingleOrDefault(t => t.Definition.Id == taskId) ?? throw new KeyNotFoundException("Task not found.");
        var branch = task is null ? run.Request.Branch : task.Branch ?? throw new InvalidDataException("Task branch has not been created yet.");
        var head = await git.BranchHead(run.Request.Repository, branch, token);
        var args = new List<string> { "log", "--format=%H%x00%aI%x00%s%x1e", "--max-count=50", "--skip=" + offset.ToString(CultureInfo.InvariantCulture), head };
        if (task?.BaseCommit is { } baseline) args.Add("^" + baseline);
        var output = await GitWorkspace.Git(processes, run.Request.Repository, token, args.ToArray());
        var commits = output.Split('\x1e', StringSplitOptions.RemoveEmptyEntries).Where(t => !string.IsNullOrWhiteSpace(t)).Select(text =>
        {
            var fields = text.TrimStart('\r', '\n').Split('\0');
            return new BranchCommit(fields[0], DateTimeOffset.Parse(fields[1], CultureInfo.InvariantCulture), Redaction.Apply(fields[2]), null);
        }).ToArray();
        var roles = new Dictionary<string, string?>();
        long before = long.MaxValue;
        while (commits.Any(c => !roles.ContainsKey(c.Hash)))
        {
            var events = store.Events(id, taskId, before, 200, "LogicalCommit");
            foreach (var e in events) roles.TryAdd(e.Message.Split(':')[0], e.Role);
            if (events.Length < 200) break;
            before = events[^1].Id;
        }
        return new(branch, commits.Select(c => c with { Role = roles.GetValueOrDefault(c.Hash) }).ToArray(), commits.Length == 50 ? offset + 50 : null);
    }
}
