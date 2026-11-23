using System.Diagnostics;
using System.Collections.Concurrent;
using System.Security.Cryptography;
using System.Text;

namespace SDLCAgent;

public sealed class ProcessRunner
{
    public async Task<CommandResult> Run(string directory, string executable, IEnumerable<string> arguments,
        int timeoutSeconds, CancellationToken cancellationToken)
    {
        if (!Path.IsPathRooted(executable) && executable.IndexOfAny(['/', '\\']) >= 0)
            executable = Path.GetFullPath(Path.Combine(directory, executable));
        var start = new ProcessStartInfo(executable)
        {
            WorkingDirectory = directory, UseShellExecute = false,
            RedirectStandardOutput = true, RedirectStandardError = true, CreateNoWindow = true
        };
        foreach (var arg in arguments) start.ArgumentList.Add(arg);
        start.Environment["GIT_TERMINAL_PROMPT"] = "0";
        start.Environment["GIT_PAGER"] = "cat";
        start.Environment["PAGER"] = "cat";
        start.Environment["CI"] = "true";
        using var process = new Process { StartInfo = start };
        if (!process.Start()) throw new IOException($"Could not start {executable}.");
        using var lifetime = NativeProcessJob.Attach(process);
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        timeout.CancelAfter(TimeSpan.FromSeconds(timeoutSeconds));
        var output = new StringBuilder();
        var error = new StringBuilder();
        async Task<bool> Drain(StreamReader stream, StringBuilder destination)
        {
            var buffer = new char[4096];
            var truncated = false;
            int count;
            try
            {
                while ((count = await stream.ReadAsync(buffer.AsMemory(), timeout.Token)) > 0)
                {
                    var remaining = Math.Max(0, 1024 * 1024 - destination.Length);
                    truncated |= count > remaining;
                    destination.Append(buffer, 0, Math.Min(count, remaining));
                }
            }
            catch (OperationCanceledException) when (timeout.IsCancellationRequested) { truncated = true; }
            return truncated;
        }
        var stdout = Drain(process.StandardOutput, output);
        var stderr = Drain(process.StandardError, error);
        var timedOut = false;
        try
        {
            await process.WaitForExitAsync(timeout.Token);
        }
        catch (OperationCanceledException)
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync(CancellationToken.None);
            timedOut = !cancellationToken.IsCancellationRequested;
            if (!timedOut) throw;
        }
        finally
        {
            lifetime?.Dispose();
            await Task.WhenAll(stdout, stderr);
        }
        cancellationToken.ThrowIfCancellationRequested();
        timedOut |= timeout.IsCancellationRequested;
        var clipped = stdout.Result || stderr.Result;
        return new CommandResult(process.ExitCode,
            Redaction.Apply(output.ToString() + error + (clipped ? "\n[output truncated to 1 MiB per stream or capture canceled]" : "")),
            timedOut, OutputTruncated: clipped, StandardOutput: output.ToString());
    }
}

public sealed class CheckRunner(Store store, ProcessRunner processes, ExecutionOptions options)
{
    private readonly ConcurrentDictionary<string, SemaphoreSlim> workspaceChecks = new();

    public async Task<CommandResult> Run(RunData run, string? taskId, string workspace, string name,
        CancellationToken cancellationToken, bool gate = false, Action? executionStarting = null)
    {
        var key = Path.TrimEndingDirectorySeparator(Path.GetFullPath(workspace));
        if (OperatingSystem.IsWindows()) key = key.ToUpperInvariant();
        var lane = workspaceChecks.GetOrAdd(key, _ => new(1));
        await lane.WaitAsync(cancellationToken);
        try { return await Execute(run, taskId, workspace, key, name, cancellationToken, gate, executionStarting); }
        finally { lane.Release(); }
    }

    private async Task<CommandResult> Execute(RunData run, string? taskId, string workspace, string key, string name,
        CancellationToken cancellationToken, bool gate, Action? executionStarting)
    {
        var sourceTree = gate ? await GitWorkspace.Git(processes, workspace, cancellationToken, "rev-parse", "HEAD^{tree}") : null;
        if (gate && (await GitWorkspace.Git(processes, workspace, cancellationToken, "status", "--porcelain")).Length != 0)
            throw new WorkflowException("Gating requires a clean immutable revision.");
        var check = run.Request.Checks.SingleOrDefault(c => c.Name == name)
            ?? throw new PolicyException($"Check {name} is not in the approved command set.");
        var before = await Snapshot(workspace, cancellationToken);
        if (check.Kind == "test")
            foreach (var build in run.Request.Checks.Where(c => c.Kind == "build"))
                if (!store.HasBuild(run.Id, key, build, before.Fingerprint))
                {
                    var message = $"Test {name} requires a successful {build.Name} build in this workspace on the current source snapshot. Run build prerequisites again; stale binaries are not valid test evidence.";
                    store.Event(run.Id, "CheckPrerequisiteBlocked", message, taskId);
                    throw new PreparationException(message);
                }
        var policy = new ExecutionPolicy(run.PolicyJson);
        if (check.Kind == "restore") policy.Require("externalAccess.allowRepositoryConfiguredDependencySourcesForRestore");
        var permission = check.Kind switch
        {
            "build" => "commands.allowBuild", "test" => "commands.allowTest",
            "format" => "commands.allowFormat", "restore" => "dependencies.allowRestoreDeclaredDependencies",
            _ => "commands.allowValidationScripts"
        };
        try { policy.Require(permission); }
        catch (PolicyException ex)
        {
            store.Event(run.Id, "PermissionDenied", ex.Message, taskId);
            throw;
        }
        if (check.Kind == "restore") store.InvalidateBuilds(run.Id, key);
        else if (check.Kind == "build") store.InvalidateBuilds(run.Id, key, check.Name);
        store.Event(run.Id, "CommandStarted", $"{name}: {check.FileName} {string.Join(' ', check.Arguments)}", taskId);
        executionStarting?.Invoke();
        var result = await processes.Run(workspace, check.FileName, check.Arguments,
            options.CommandTimeoutSeconds, cancellationToken);
        var after = await Snapshot(workspace, cancellationToken);
        var sourceChanged = before.Fingerprint != after.Fingerprint;
        var artifact = store.Save(run.Id, taskId, "CommandEvidence", new
        {
            check, workspace, gate, sourceTree, result.ExitCode, result.Output, result.TimedOut, result.OutputTruncated,
            revision = before.Revision, sourceFingerprint = before.Fingerprint, sourceChanged,
            executionContext = gate ? GateContext(check, workspace) : null
        });
        store.Event(run.Id, "CommandFinished", $"{name}: exit {result.ExitCode}; timeout={result.TimedOut}.", taskId, artifact.Id);
        if (sourceChanged)
        {
            store.InvalidateBuilds(run.Id, key);
            var message = $"Check {name} changed the source snapshot. Its result is not valid evidence; reconcile the changes and rerun prerequisites.";
            store.Event(run.Id, "ValidationSourceChanged", message, taskId, artifact.Id);
            throw new PreparationException(message);
        }
        if (check.Kind == "build" && Passed(result)) store.RecordBuild(run.Id, key, check, before.Fingerprint, artifact.Id);
        if (gate && store.RecordGate(run.Id, name, sourceTree!, artifact.Id, Passed(result), GateContext(check, workspace)) is { } failure && Passed(result))
        {
            var message = $"Gate {name} previously failed on the identical source tree and command/workspace layout (evidence {failure}). A later pass is not remediation. Repair the flaky check/environment and start a replacement run; evidence is retained.";
            store.Event(run.Id, "UnstableGateDetected", message, taskId, artifact.Id);
            throw new PolicyException(message);
        }

        return result with { EvidenceId = artifact.Id };
    }

    internal static string GateContext(CheckCommand check, string workspace)
    {
        var layout = System.Text.RegularExpressions.Regex.Replace(Path.GetFullPath(workspace),
            @"(?i)(?<![0-9a-f])[0-9a-f]{32,40}(?![0-9a-f])", match => $"<id:{match.Length}>");
        if (OperatingSystem.IsWindows()) layout = layout.ToUpperInvariant();
        return Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(Json.Write(new { check, workspaceLayout = layout }))));
    }

        public static bool Passed(CommandResult result) => result.ExitCode == 0 && !result.TimedOut && !result.OutputTruncated;

        internal async Task<(string Revision, string Fingerprint)> Snapshot(string workspace, CancellationToken token)
        {
            var head = await GitWorkspace.Head(processes, workspace, token);
            var diff = await GitWorkspace.Git(processes, workspace, token, "diff", "--no-ext-diff", "--no-textconv", "--binary", "HEAD");
            var untracked = await GitWorkspace.Git(processes, workspace, token, "ls-files", "--others", "--exclude-standard", "-z");
            using var hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
            hash.AppendData(Encoding.UTF8.GetBytes(head + "\0" + diff + "\0"));
            foreach (var path in untracked.Split('\0', StringSplitOptions.RemoveEmptyEntries).Order(StringComparer.Ordinal))
            {
                token.ThrowIfCancellationRequested();
                hash.AppendData(Encoding.UTF8.GetBytes(path + "\0"));
                using var file = File.OpenRead(SafePaths.Resolve(workspace, path));
                hash.AppendData(await SHA256.HashDataAsync(file, token));
            }
            return (head, Convert.ToHexString(hash.GetHashAndReset()));
        }

        public async Task<CheckExecution[]> Batch(RunData run, string? taskId, string workspace, IEnumerable<string> names,
            CancellationToken token, bool gate = false)
        {
            var ordered = names.Distinct().Select(name => run.Request.Checks.SingleOrDefault(c => c.Name == name)
                ?? throw new PolicyException($"Check {name} is not in the approved command set."))
                .OrderBy(c => c.Kind switch { "restore" => 0, "build" => 1, "test" => 3, _ => 2 });
            var results = new List<CheckExecution>();
            foreach (var check in ordered)
            {
                var result = await Run(run, taskId, workspace, check.Name, token, gate);
                results.Add(new(check.Name, check.Kind, result));
                if (!Passed(result)) break;
            }
            return results.ToArray();
        }
    public async Task<bool> All(RunData run, string? taskId, string workspace, IEnumerable<string> names,
        CancellationToken cancellationToken)
    {
        return (await Batch(run, taskId, workspace, names, cancellationToken, gate: true)).All(e => Passed(e.Result));
    }
}
