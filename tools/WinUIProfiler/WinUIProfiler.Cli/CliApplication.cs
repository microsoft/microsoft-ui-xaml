using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Query;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Cli;

public static class CliApplication
{
    public const int SchemaVersion = 1;
    public const int MaximumOutputBytes = ProfilerQueryService.MaximumMaterializedBytes;
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        Converters = { new JsonStringEnumConverter(allowIntegerValues: false) }
    };

    public static async Task<int> RunAsync(string[] args, TextWriter output, TextWriter error, CancellationToken cancellationToken = default)
    {
        string? command = null;
        try
        {
            cancellationToken.ThrowIfCancellationRequested();
            CliInvocation invocation;
            try { invocation = CliArguments.Parse(args); }
            catch (ArgumentException) { return await Fail(error, command, 2, "invalid_arguments", "Invalid arguments. See docs/cli.md for the bounded command contract."); }
            command = invocation.Name;
            SessionStore store;
            try
            {
                foreach (var suffix in new[] { "", ".writer", "-wal", "-shm", ".failure.json" })
                    RejectReparsePoints(invocation.DatabasePath + suffix);
                store = SessionStore.OpenSavedReadOnly(invocation.DatabasePath);
            }
            catch (Exception ex) when (ex is not OperationCanceledException)
            {
                return await Fail(error, command, 3, "database_error", "The saved local session could not be opened read-only.");
            }
            using (store)
            {
                var request = invocation.Bind(store.Scope);
                QueryValidation.Validate(request, cancellationToken);
                var result = await Route(store.Queries, request, cancellationToken);
                using var buffer = new BoundedOutput();
                JsonSerializer.Serialize(buffer, new SuccessEnvelope(SchemaVersion, command, result), JsonOptions);
                cancellationToken.ThrowIfCancellationRequested();
                await output.WriteLineAsync(Encoding.UTF8.GetString(buffer.GetBuffer(), 0, checked((int)buffer.Length)).AsMemory(), cancellationToken);
                return 0;
            }
        }
        catch (OperationCanceledException)
        {
            return await Fail(error, command, 6, "cancelled", "The query was cancelled.");
        }
        catch (NotSupportedException)
        {
            return await Fail(error, command, 4, "unsupported_query", "This operation requires later snapshot/ownership processors; no data was synthesized.");
        }
        catch (ArgumentException)
        {
            return await Fail(error, command, 2, "invalid_arguments", "The typed query bounds, identity, time or committed watermark are invalid.");
        }
        catch (Exception)
        {
            return await Fail(error, command, 5, "query_error", "The saved query or bounded JSON output could not be completed.");
        }
    }

    internal static async ValueTask<object> Route(IProfilerQueryService queries, object request, CancellationToken token) => request switch
    {
        GetSessionRequest r => await queries.GetSession(r, token),
        GetMetricsRequest r => await queries.GetMetrics(r, token),
        GetStartupTimelineRequest r => await queries.GetStartupTimeline(r, token),
        GetFramesRequest r => await queries.GetFrames(r, token),
        GetObjectHistoryRequest r => await queries.GetObjectHistory(r, token),
        GetXamlTreeRequest r => await queries.GetXamlTree(r, token),
        GetCompositionTreeRequest r => await queries.GetCompositionTree(r, token),
        GetCorrelationsRequest r => await queries.GetCorrelations(r, token),
        GetResourceUsageRequest r => await queries.GetResourceUsage(r, token),
        GetSnapshotsRequest r => await queries.GetSnapshots(r, token),
        CompareSnapshotsRequest r => await queries.CompareSnapshots(r, token),
        GetRetentionPathsRequest r => await queries.GetRetentionPaths(r, token),
        _ => throw new ArgumentException("Unknown typed query request.")
    };

    private static void RejectReparsePoints(string path)
    {
        var ancestors = new Stack<string>();
        for (var current = path; current is not null; current = Path.GetDirectoryName(current))
            ancestors.Push(current);
        foreach (var current in ancestors)
        {
            FileAttributes attributes;
            try { attributes = File.GetAttributes(current); }
            catch (FileNotFoundException) { break; }
            catch (DirectoryNotFoundException) { break; }
            if ((attributes & FileAttributes.ReparsePoint) != 0)
                throw new IOException("Linked session paths are not supported.");
        }
    }

    private static async Task<int> Fail(TextWriter error, string? command, int exitCode, string code, string message)
    {
        await error.WriteLineAsync(JsonSerializer.Serialize(new ErrorEnvelope(SchemaVersion, command, new(code, message)), JsonOptions));
        return exitCode;
    }

    private sealed record SuccessEnvelope(int SchemaVersion, string Command, object Result);
    private sealed record ErrorEnvelope(int SchemaVersion, string? Command, ErrorInfo Error);
    private sealed record ErrorInfo(string Code, string Message);

    private sealed class BoundedOutput : MemoryStream
    {
        public override void Write(byte[] buffer, int offset, int count)
        {
            Check(count);
            base.Write(buffer, offset, count);
        }
        public override void Write(ReadOnlySpan<byte> buffer)
        {
            Check(buffer.Length);
            base.Write(buffer);
        }
        private void Check(int count)
        {
            if (count > MaximumOutputBytes - Position) throw new IOException("JSON response exceeds its output budget.");
        }
    }
}
