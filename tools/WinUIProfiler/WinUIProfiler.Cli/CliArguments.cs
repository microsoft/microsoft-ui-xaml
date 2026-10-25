using System.Globalization;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Cli;

internal enum QueryCommand
{
    Session, Metrics, Startup, Frames, ObjectHistory, XamlTree,
    CompositionTree, Correlations, Resources, Snapshots, Compare, Retention
}

internal sealed record CliInvocation(string DatabasePath, string Name, Func<SessionScope, object> Bind);

internal static class CliArguments
{
    internal const int MaximumArgumentLength = 4096;
    internal const int MaximumArgumentCount = 65;
    internal const int MaximumArgumentCharacters = 32768;

    internal static CliInvocation Parse(string[] args)
    {
        ArgumentNullException.ThrowIfNull(args);
        if (args.Length is < 1 or > MaximumArgumentCount ||
            args.Any(arg => string.IsNullOrWhiteSpace(arg) || arg.Length > MaximumArgumentLength || arg.Contains('\0')) ||
            args.Sum(arg => arg.Length) > MaximumArgumentCharacters)
            throw new ArgumentException("Argument bounds are invalid.");
        var command = args[0] switch
        {
            "session" => QueryCommand.Session,
            "metrics" => QueryCommand.Metrics,
            "startup" => QueryCommand.Startup,
            "frames" => QueryCommand.Frames,
            "object-history" => QueryCommand.ObjectHistory,
            "xaml-tree" => QueryCommand.XamlTree,
            "composition-tree" => QueryCommand.CompositionTree,
            "correlations" => QueryCommand.Correlations,
            "resources" => QueryCommand.Resources,
            "snapshots" => QueryCommand.Snapshots,
            "compare" => QueryCommand.Compare,
            "retention" => QueryCommand.Retention,
            _ => throw new ArgumentException("Unknown query command.")
        };
        var allowed = new HashSet<string>(StringComparer.Ordinal) { "--db" };
        if (command != QueryCommand.Session)
            allowed.UnionWith(["--limit", "--offset", "--watermark-ordinal", "--watermark-qpc"]);
        if (command is QueryCommand.Metrics or QueryCommand.Frames or QueryCommand.ObjectHistory or QueryCommand.Resources)
            allowed.UnionWith(["--start-qpc", "--end-qpc"]);
        if (command is QueryCommand.XamlTree or QueryCommand.CompositionTree or QueryCommand.Correlations)
            allowed.Add("--at-qpc");
        if (command is QueryCommand.ObjectHistory or QueryCommand.XamlTree or QueryCommand.CompositionTree or
            QueryCommand.Correlations or QueryCommand.Resources or QueryCommand.Retention)
            allowed.UnionWith(["--domain", "--producer-epoch", "--runtime-id"]);
        if (command == QueryCommand.Metrics) allowed.Add("--metric");
        if (command == QueryCommand.Snapshots) allowed.Add("--kind");
        if (command == QueryCommand.Compare) allowed.UnionWith(["--baseline-id", "--snapshot-ids"]);
        if (command == QueryCommand.Retention) allowed.UnionWith(["--snapshot-id", "--maximum-depth"]);

        var options = new Dictionary<string, string>(StringComparer.Ordinal);
        for (var i = 1; i < args.Length; i += 2)
        {
            if (!allowed.Contains(args[i]) || i + 1 == args.Length || args[i + 1].StartsWith("--", StringComparison.Ordinal) ||
                !options.TryAdd(args[i], args[i + 1]))
                throw new ArgumentException("Unknown, repeated or missing option.");
        }
        string Required(string name) => options.TryGetValue(name, out var value) ? value :
            throw new ArgumentException("A required option is missing.");
        long Number(string name) => Nonnegative(Required(name));
        Guid Id(string name) => ParseId(Required(name));
        var path = LocalPath(Required("--db"));
        PageRequest page = new(128);
        if (command != QueryCommand.Session)
        {
            var limit = options.ContainsKey("--limit") ? Number("--limit") : 128;
            var offset = options.ContainsKey("--offset") ? Number("--offset") : 0;
            if (options.ContainsKey("--watermark-ordinal") != options.ContainsKey("--watermark-qpc"))
                throw new ArgumentException("Both committed watermark fields are required.");
            CommittedWatermark? watermark = options.ContainsKey("--watermark-ordinal") ?
                new(Number("--watermark-ordinal"), Number("--watermark-qpc")) : null;
            if (limit is < 1 or > PageRequest.MaximumLimit || offset > long.MaxValue - limit || offset > 0 && watermark is null)
                throw new ArgumentException("Page bounds or continuation watermark are invalid.");
            page = new((int)limit, offset, watermark);
        }

        var range = command is QueryCommand.Metrics or QueryCommand.Frames or QueryCommand.ObjectHistory or QueryCommand.Resources ?
            new TimeRange(Number("--start-qpc"), Number("--end-qpc")) : default;
        var at = command is QueryCommand.XamlTree or QueryCommand.CompositionTree or QueryCommand.Correlations ? Number("--at-qpc") : 0;
        var hasObject = options.ContainsKey("--domain") || options.ContainsKey("--producer-epoch") || options.ContainsKey("--runtime-id");
        var needsObject = command is QueryCommand.ObjectHistory or QueryCommand.Correlations or QueryCommand.Retention;
        IdentityDomain domain = default;
        Guid epoch = default;
        ulong runtimeId = 0;
        if (hasObject || needsObject)
        {
            domain = NamedEnum<IdentityDomain>(Required("--domain"));
            epoch = Id("--producer-epoch");
            if (!ulong.TryParse(Required("--runtime-id"), NumberStyles.None, CultureInfo.InvariantCulture, out runtimeId) || runtimeId == 0)
                throw new ArgumentException("Runtime identity must be a positive unsigned 64-bit decimal integer.");
            if (command == QueryCommand.XamlTree && domain != IdentityDomain.XamlCore ||
                command == QueryCommand.CompositionTree && domain is not (IdentityDomain.CompNode or IdentityDomain.CompositionVisual))
                throw new ArgumentException("Root domain does not match the requested tree.");
        }
        ObjectKey? Key(SessionScope scope) => hasObject || needsObject ? new(scope.Process, domain, epoch, runtimeId) : null;
        MetricKind? metric = options.TryGetValue("--metric", out var metricName) ? NamedEnum<MetricKind>(metricName) : null;
        SnapshotKind? kind = options.TryGetValue("--kind", out var kindName) ? NamedEnum<SnapshotKind>(kindName) : null;
        var baseline = command == QueryCommand.Compare ? Id("--baseline-id") : Guid.Empty;
        var snapshots = command == QueryCommand.Compare ? Required("--snapshot-ids").Split(',').Select(ParseId).ToArray() : [];
        if (command == QueryCommand.Compare && (snapshots.Length is < 1 or > 64 ||
            snapshots.Distinct().Count() != snapshots.Length || snapshots.Contains(baseline)))
            throw new ArgumentException("Compare requires 1–64 distinct snapshots and a separate baseline.");
        var snapshot = command == QueryCommand.Retention ? Id("--snapshot-id") : Guid.Empty;
        var depth = command == QueryCommand.Retention ? Number("--maximum-depth") : 1;
        if (depth is < 1 or > 256) throw new ArgumentException("Retention depth must be 1–256.");

        object Bind(SessionScope scope) => command switch
        {
            QueryCommand.Session => new GetSessionRequest(scope),
            QueryCommand.Metrics => new GetMetricsRequest(scope, range, page, metric),
            QueryCommand.Startup => new GetStartupTimelineRequest(scope, page),
            QueryCommand.Frames => new GetFramesRequest(scope, range, page),
            QueryCommand.ObjectHistory => new GetObjectHistoryRequest(scope, Key(scope)!.Value, range, page),
            QueryCommand.XamlTree => new GetXamlTreeRequest(scope, at, page, Key(scope)),
            QueryCommand.CompositionTree => new GetCompositionTreeRequest(scope, at, page, Key(scope)),
            QueryCommand.Correlations => new GetCorrelationsRequest(scope, Key(scope)!.Value, at, page),
            QueryCommand.Resources => new GetResourceUsageRequest(scope, range, page, Key(scope)),
            QueryCommand.Snapshots => new GetSnapshotsRequest(scope, page, kind),
            QueryCommand.Compare => new CompareSnapshotsRequest(scope, baseline, snapshots, page),
            QueryCommand.Retention => new GetRetentionPathsRequest(scope, snapshot, Key(scope)!.Value, (int)depth, page),
            _ => throw new ArgumentException("Unknown query command.")
        };
        return new(path, args[0], Bind);
    }

    private static long Nonnegative(string value) =>
        long.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var result) ? result :
            throw new ArgumentException("Expected a nonnegative signed 64-bit decimal integer.");

    private static Guid ParseId(string value) =>
        Guid.TryParseExact(value, "D", out var id) && id != Guid.Empty ? id :
            throw new ArgumentException("Expected a nonempty hyphenated GUID.");

    private static T NamedEnum<T>(string value) where T : struct, Enum =>
        Enum.GetNames<T>().Contains(value, StringComparer.OrdinalIgnoreCase) ? Enum.Parse<T>(value, ignoreCase: true) :
            throw new ArgumentException("Expected a named enum value.");

    private static string LocalPath(string value)
    {
        var path = Path.GetFullPath(value);
        if (value.Contains("://", StringComparison.Ordinal) || path.StartsWith(@"\\", StringComparison.Ordinal) ||
            path.StartsWith("//", StringComparison.Ordinal) || path.AsSpan(2).Contains(':') ||
            new DriveInfo(Path.GetPathRoot(path)!).DriveType == DriveType.Network)
            throw new ArgumentException("A local filesystem database path is required.");
        return path;
    }
}
