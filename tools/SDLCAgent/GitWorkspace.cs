namespace SDLCAgent;

public sealed class GitWorkspace(Store store, ProcessRunner processes)
{
    private static string[] Arguments(IEnumerable<string> args) =>
        ["-c", "core.hooksPath=" + (OperatingSystem.IsWindows() ? "NUL" : Path.Combine(Path.GetTempPath(), "sdlc-agent-no-hooks")),
         "-c", "user.name=SDLCAgent", "-c", "user.email=sdlc-agent@localhost", "--no-pager", .. args];

    public static async Task<string> Git(ProcessRunner runner, string directory, CancellationToken token, params string[] args)
    {
        var result = await runner.Run(directory, "git", Arguments(args), 120, token);
        if (result.ExitCode != 0 || result.TimedOut)
            throw new WorkflowException($"git {args.FirstOrDefault()} failed: {result.Output}");
        if (result.OutputTruncated) throw new WorkflowException("Git metadata output exceeded the capture limit; narrow the operation.");
        return (result.StandardOutput ?? throw new InvalidDataException("Missing Git stdout.")).TrimEnd('\r', '\n');
    }

    public static Task<string> Head(ProcessRunner runner, string directory, CancellationToken token) =>
        Git(runner, directory, token, "rev-parse", "HEAD");

    public async Task<string> ValidateRepository(RunRequest request, CancellationToken token)
    {
        if (!request.TrustRepository)
            throw new PolicyException("Explicit trustRepository=true is required: native workers execute repository code without an OS sandbox.");
        if (!Directory.Exists(request.Repository) || string.IsNullOrWhiteSpace(request.Problem)
            || string.IsNullOrWhiteSpace(request.Branch) || request.Branch.StartsWith('-'))
            throw new InvalidDataException("Provide an existing repository, problem, and existing integration branch.");
        await Git(processes, request.Repository, token, "check-ref-format", "refs/heads/" + request.Branch);
        var top = await Git(processes, request.Repository, token, "rev-parse", "--show-toplevel");
        if (!string.Equals(Path.GetFullPath(top), Path.GetFullPath(request.Repository), StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("Repository must be its Git top-level directory.");
        await RequireBranchNotCheckedOut(request.Repository, request.Branch, token);
        return await BranchHead(request.Repository, request.Branch, token);
    }

    public async Task RequireBranchNotCheckedOut(string repository, string branch, CancellationToken token)
    {
        var worktrees = await Git(processes, repository, token, "worktree", "list", "--porcelain");
        if (worktrees.Split('\n').Any(line => line.Trim() == "branch refs/heads/" + branch))
            throw new PolicyException("The integration branch must not be checked out elsewhere. Create a dedicated delivery branch and leave your checkout on another branch.");
    }

    public Task<string> BranchHead(string repository, string branch, CancellationToken token) =>
        Git(processes, repository, token, "rev-parse", "--verify", "refs/heads/" + branch + "^{commit}");

    public async Task<string> Inspection(RunData run, CancellationToken token)
    {
        var commit = run.InspectionCommit ?? await BranchHead(run.Request.Repository, run.Request.Branch, token);
        var path = Path.Combine(store.DataDirectory, "workspaces", run.Id, "inspection-" + commit);
        if (!Directory.Exists(path))
            await Git(processes, run.Request.Repository, token, "worktree", "add", "--detach", path, commit);
        if (await Head(processes, path, token) != commit)
            throw new WorkflowException("Inspection workspace revision changed unexpectedly.");
        store.Change(run.Id, "InspectionWorkspace", $"Repository inspection uses pinned revision {commit}.", r =>
        { r.InspectionWorkspace = path; r.InspectionCommit = commit; });
        return path;
    }

    public async Task<(string Path, string Branch, string Base)> TaskWorkspace(RunData run, TaskData task, CancellationToken token)
    {
        new ExecutionPolicy(run.PolicyJson).Require("git.allowCreateTaskBranches");
        var path = Path.Combine(store.DataDirectory, "workspaces", run.Id, task.Definition.Id);
        var branch = $"sdlc/{run.Id}/{task.Definition.Id}";
        var baseline = task.BaseCommit;
        if (task.Workspace is null)
        {
            baseline = await BranchHead(run.Request.Repository, run.Request.Branch, token);
            store.Change(run.Id, "WorkspaceIntent", $"Work item {task.Definition.Id} owns shared implementer/tester worktree {path} on {branch} from {baseline}.", r =>
            {
                var t = r.Tasks.Single(t => t.Definition.Id == task.Definition.Id);
                t.Workspace = path; t.Branch = branch; t.BaseCommit = baseline;
            }, task.Definition.Id);
        }
        else if (!SamePath(task.Workspace, path) || task.Branch != branch || baseline is null)
            throw new WorkspaceStateException($"Work item {task.Definition.Id} must use its own worktree {path} and branch {branch}; saved ownership does not match. No files were changed.");
        if (!Directory.Exists(path))
        {
            var existing = await Git(processes, run.Request.Repository, token, "for-each-ref", "--format=%(refname)", "refs/heads/" + branch);
            if (existing.Length == 0)
                await Git(processes, run.Request.Repository, token, "worktree", "add", "-b", branch, path, baseline!);
            else await Git(processes, run.Request.Repository, token, "worktree", "add", path, branch);
        }
        var actual = await Git(processes, path, token, "branch", "--show-current");
        var top = await Git(processes, path, token, "rev-parse", "--show-toplevel");
        var common = await Git(processes, path, token, "rev-parse", "--path-format=absolute", "--git-common-dir");
        var expectedCommon = await Git(processes, run.Request.Repository, token, "rev-parse", "--path-format=absolute", "--git-common-dir");
        if (actual != branch || !SamePath(top, path) || !SamePath(common, expectedCommon))
            throw new WorkspaceStateException($"Work item {task.Definition.Id} worktree ownership changed: expected branch {branch} in {path}, linked to {run.Request.Repository}. No files were changed.");
        if (task.Diagnosis is null) await RequireCleanHandoff(path, token);
        return (path, branch, baseline!);
    }

    private static bool SamePath(string first, string second) => string.Equals(
        Path.TrimEndingDirectorySeparator(Path.GetFullPath(first)), Path.TrimEndingDirectorySeparator(Path.GetFullPath(second)),
        OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);

    public async Task RequireCleanHandoff(string workspace, CancellationToken token)
    {
        var status = await Git(processes, workspace, token, "status", "--porcelain", "--untracked-files=all");
        if (status.Length != 0)
            throw new WorkspaceStateException($"Shared per-item worktree {workspace} has uncommitted changes before worker handoff. Reconcile and commit or restore them under the owning role before resuming; no changes were discarded or attributed to the next worker.\n{status}");
    }

    public async Task SyncTask(RunData run, TaskData task, CancellationToken token)
    {
        var head = await BranchHead(run.Request.Repository, run.Request.Branch, token);
        var result = await processes.Run(task.Workspace!, "git", Arguments(["merge", "--no-ff", "--no-edit", head]), 120, token);
        if (result.ExitCode is not (0 or 1) || result.TimedOut)
            throw new WorkflowException("Cannot synchronize task with current integration branch: " + result.Output);
        store.Change(run.Id, "TaskSynchronized", result.ExitCode == 0 ? "Merged latest integration revision into repair workspace."
            : "Merge conflicts require pair repair: " + result.Output, r =>
        {
            var t = r.Tasks.Single(t => t.Definition.Id == task.Definition.Id);
            t.NeedsSync = false;
            if (result.ExitCode == 1) t.Failure += "\nResolve these merge conflicts within assigned scopes: " + result.Output;
        }, task.Definition.Id);
    }

    public async Task<string> Commit(RunData run, string taskId, string workspace, string message, CancellationToken token)
    {
        new ExecutionPolicy(run.PolicyJson).Require("git.allowCommitTaskChanges");
        if (string.IsNullOrWhiteSpace(message) || message.Length > 300)
            throw new InvalidDataException("Commit message must be a short logical-change summary.");
        var status = await Git(processes, workspace, token, "status", "--porcelain");
        if (status.Length == 0) return await Head(processes, workspace, token);
        await Git(processes, workspace, token, "add", "--all", "--", ".");
        await Git(processes, workspace, token, "commit", "-m", message,
            "-m", $"Task: {taskId}\nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>\nCopilot-Session: {run.Id}");
        var head = await Head(processes, workspace, token);
        store.Event(run.Id, "LogicalCommit", $"{head}: {message}", taskId);
        return head;
    }

    public async Task<string> Candidate(RunData run, TaskData task, string baseline, CancellationToken token)
    {
        // Legacy XAML compilers still impose MAX_PATH on intermediate assemblies.
        var path = Path.Combine(store.DataDirectory, "workspaces", $"candidate-{Guid.NewGuid():N}");
        await Git(processes, run.Request.Repository, token, "worktree", "add", "--detach", path, baseline);
        await Git(processes, path, token, "merge", "--no-ff", "--no-edit", task.HeadCommit!, "-m",
            $"Integrate {task.Definition.Id}\n\nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>\nCopilot-Session: {run.Id}");
        var candidate = await Head(processes, path, token);
        if (candidate != baseline)
            store.Event(run.Id, "LogicalCommit", $"{candidate}: Integrate {task.Definition.Id} (candidate; gates still required)", task.Definition.Id);
        return path;
    }

    public async Task<string> TaskProvenance(RunData run, TaskData task, string revision, CancellationToken token)
    {
        var workspace = await TaskWorkspace(run, task, token);
        if (await Head(processes, workspace.Path, token) != revision)
            throw new WorkspaceStateException("Task provenance must describe the exact verification revision.");
        await Git(processes, workspace.Path, token, "merge-base", "--is-ancestor", workspace.Base, revision);
        var history = await Git(processes, workspace.Path, token, "log", "--reverse", "--format=%H%x09%P%x09%s",
            "--name-status", workspace.Base + ".." + revision, "--", ".");
        var trace = store.TaskExecutionTrace(run.Id, task.Definition.Id);
        if (await Head(processes, workspace.Path, token) != revision)
            throw new WorkspaceStateException("Task provenance became stale during inspection.");
        await RequireCleanHandoff(workspace.Path, token);
        return Json.Write(new
        {
            runId = run.Id, taskId = task.Definition.Id, revision,
            workspace = new { workspace.Path, workspace.Branch, workspace.Base },
            ownershipVerified = true, task.ImplementationPlanId, task.TestPlanId,
            committedChangesInAncestryOrder = history, trace.Truncated,
            historicalFailedChecks = trace.Events.Where(e => e.Kind == "CommandFinished" && e.DocumentId is not null && e.PlanId == task.TestPlanId)
                .Select(e => new { e.Id, evidenceId = e.DocumentId, receipt = System.Text.Json.Nodes.JsonNode.Parse(store.ReadArtifact(e.DocumentId!))! })
                .Where(e => e.receipt["check"]!["kind"]!.GetValue<string>() == "test" && e.receipt["exitCode"]!.GetValue<int>() != 0),
            historicalReceiptScope = "Only negative test receipts from the current saved tester plan; other command outcomes remain in the timeline and full audit artifacts. Historical failures do not replace fresh passing gates.",
            timeline = trace.Events.Select(e => new { e.Id, e.At, e.Kind, e.Role, e.PlanId, e.StepId, e.Attempt, e.Message, e.DocumentId })
        });
    }

    public async Task Advance(RunData run, string candidate, string expected, CancellationToken token)
    {
        await RequireBranchNotCheckedOut(run.Request.Repository, run.Request.Branch, token);
        await Git(processes, run.Request.Repository, token, "update-ref", "-m", $"SDLCAgent {run.Id}: gated integration",
            "refs/heads/" + run.Request.Branch, candidate, expected);
    }

    public async Task<string> FinalWorkspace(RunData run, string head, CancellationToken token)
    {
        var identity = Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(
            System.Text.Encoding.UTF8.GetBytes(run.Id + ":" + head)))[..32];
        var path = Path.Combine(store.DataDirectory, "workspaces", "final-" + identity);
        if (!Directory.Exists(path))
            await Git(processes, run.Request.Repository, token, "worktree", "add", "--detach", path, head);
        if (await Head(processes, path, token) != head) throw new WorkflowException("Final workspace revision mismatch.");
        return path;
    }
}
