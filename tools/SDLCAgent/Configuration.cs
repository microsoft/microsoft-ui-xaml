using System.Security.Cryptography;
using System.Security.Principal;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace SDLCAgent;

public sealed class ExecutionPolicy
{
    private readonly JsonElement root;
    public string Text { get; }
    public string Hash => Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(Text)));

    public ExecutionPolicy(string text)
    {
        Text = text;
        root = JsonDocument.Parse(text).RootElement.Clone();
        using var stream = typeof(ExecutionPolicy).Assembly.GetManifestResourceStream("SDLCAgent.execution-policy.json")
            ?? throw new InvalidOperationException("Missing embedded policy schema.");
        using var template = JsonDocument.Parse(stream);
        ValidateShape(root, template.RootElement, "policy");
        if (root.GetProperty("schemaVersion").GetInt32() != 1 || String("defaultDecision") != "deny"
            || String("onDeniedAction") != "blockTask" || String("isolation.mode") != "native"
            || String("isolation.workspaceMode") != "gitWorktree" || !Bool("isolation.requireNonElevated")
            || !Bool("isolation.trustedRepositoriesOnly") || Bool("policyManagement.allowWorkerPolicyChanges")
            || !Bool("git.requirePassingGatesForIntegration") || !Bool("git.requireTestedCandidateAndUnchangedBase")
            || !Bool("repository.requireSpecificationApprovalForImplementation")
            || !root.GetProperty("git").GetProperty("integrationBranchWriters").EnumerateArray()
                .Select(x => x.GetString()).SequenceEqual(["orchestrator"]))
            throw new InvalidDataException("Unsupported or unsafe policy: preserve native trusted/non-elevated execution, approval, deny-by-default, and orchestrator-only gated integration.");
    }

    public bool Bool(string path) => Get(path).GetBoolean();
    public string String(string path) => Get(path).GetString() ?? throw new InvalidDataException(path);
    public bool HasService(string service) => Get("externalAccess.authorizedServices").EnumerateArray()
        .Any(s => s.GetString() == service);
    public void Require(string path)
    {
        if (!Bool(path)) throw new PolicyException($"Policy denies {path}.");
    }

    private JsonElement Get(string path)
    {
        var value = root;
        foreach (var part in path.Split('.')) value = value.GetProperty(part);
        return value;
    }

    private static void ValidateShape(JsonElement value, JsonElement template, string path)
    {
        if (value.ValueKind != template.ValueKind
            && !(value.ValueKind is JsonValueKind.True or JsonValueKind.False
                && template.ValueKind is JsonValueKind.True or JsonValueKind.False))
            throw new InvalidDataException($"Invalid policy type at {path}.");
        if (template.ValueKind == JsonValueKind.Object)
        {
            var properties = value.EnumerateObject().ToArray();
            if (properties.Select(p => p.Name).Distinct().Count() != properties.Length
                || properties.Length != template.EnumerateObject().Count())
                throw new InvalidDataException($"Unknown, duplicate, or missing policy setting at {path}.");
            foreach (var property in template.EnumerateObject())
            {
                if (!value.TryGetProperty(property.Name, out var child))
                    throw new InvalidDataException($"Missing policy setting {path}.{property.Name}.");
                ValidateShape(child, property.Value, $"{path}.{property.Name}");
            }
        }
        if (template.ValueKind == JsonValueKind.Array && value.EnumerateArray().Any(v => v.ValueKind != JsonValueKind.String))
            throw new InvalidDataException($"Policy array {path} must contain strings.");
    }

    public static void RequireNonElevated()
    {
        if (OperatingSystem.IsWindows())
        {
            using var identity = WindowsIdentity.GetCurrent();
            if (new WindowsPrincipal(identity).IsInRole(WindowsBuiltInRole.Administrator))
                throw new PolicyException("SDLCAgent refuses elevated execution. Start it from a non-administrator terminal.");
        }
        else if (Environment.UserName == "root")
            throw new PolicyException("SDLCAgent refuses root execution.");
    }
}

public static class SafePaths
{
    public static void ValidateRelative(string path)
    {
        if (string.IsNullOrWhiteSpace(path) || Path.IsPathRooted(path) || path.Contains(':')
            || path.Split(['/', '\\']).Any(p => p == ".." || p.Equals(".git", StringComparison.OrdinalIgnoreCase))
            || path.Any(char.IsControl) || path.IndexOfAny(['*', '?', '|', '"', '<', '>']) >= 0)
            throw new PolicyException($"Not a safe repository-relative path: {path}.");
    }

    public static string Resolve(string root, string relative)
    {
        ValidateRelative(relative);
        root = Path.GetFullPath(root);
        var target = Path.GetFullPath(Path.Combine(root, relative));
        if (!IsWithin(root, target)) throw new PolicyException("Path is outside the assigned workspace.");
        for (var current = target; current is not null && IsWithin(root, current); current = Path.GetDirectoryName(current))
            if ((File.Exists(current) || Directory.Exists(current))
                && (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                throw new PolicyException("Symlinks and junctions are not permitted in scoped file tools.");
        return target;
    }

    public static bool IsWithin(string root, string path) => string.Equals(root, path, StringComparison.OrdinalIgnoreCase)
        || path.StartsWith(root.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);

    public static bool InScope(string relative, IEnumerable<string> scopes)
    {
        ValidateRelative(relative);
        var path = Path.GetFullPath(Path.Combine(Path.GetTempPath(), "sdlc-scope", relative));
        return scopes.Any(scope => IsWithin(Path.GetFullPath(Path.Combine(Path.GetTempPath(), "sdlc-scope", scope)), path));
    }

    public static bool Overlap(IEnumerable<string> first, IEnumerable<string> second) => first.Any(a => second.Any(b =>
        InScope(a, [b]) || InScope(b, [a])));
}

public static partial class Redaction
{
    [GeneratedRegex(@"(?i)(?:gh[pousr]_[a-z0-9_]{15,}|github_pat_[a-z0-9_]+|Bearer\s+[a-z0-9._~+/-]+=*|(?:password|api[_-]?key|access[_-]?token|secret)\s*[:=]\s*[^\s,;""}]+)")]
    private static partial Regex Secrets();

    public static string Apply(string text)
    {
        text = Secrets().Replace(text, "[REDACTED]");
        foreach (var name in new[] { "GH_TOKEN", "GITHUB_TOKEN", "COPILOT_GITHUB_TOKEN", "AZURE_CLIENT_SECRET" })
        {
            var secret = Environment.GetEnvironmentVariable(name);
            if (!string.IsNullOrEmpty(secret) && secret.Length >= 8) text = text.Replace(secret, "[REDACTED]", StringComparison.Ordinal);
        }
        return text;
    }

    public static string Json<T>(T value)
    {
        var node = System.Text.Json.JsonSerializer.SerializeToNode(value, SDLCAgent.Json.Options);
        Redact(node);
        return node?.ToJsonString(SDLCAgent.Json.Options) ?? "null";

        static void Redact(System.Text.Json.Nodes.JsonNode? current)
        {
            if (current is System.Text.Json.Nodes.JsonObject obj)
                foreach (var property in obj.ToArray())
                {
                    if (property.Value is System.Text.Json.Nodes.JsonValue scalar && scalar.TryGetValue<string>(out var text))
                        obj[property.Key] = Apply(text);
                    else Redact(property.Value);
                }
            else if (current is System.Text.Json.Nodes.JsonArray array)
                for (var i = 0; i < array.Count; i++)
                {
                    if (array[i] is System.Text.Json.Nodes.JsonValue scalar && scalar.TryGetValue<string>(out var text))
                        array[i] = Apply(text);
                    else Redact(array[i]);
                }
        }
    }
}

public static class OptionsLoader
{
    public static AgentOptions Load(string path)
    {
        var options = Json.Read<AgentOptions>(File.ReadAllText(path));
        var e = options.Execution;
        if (options.Host.Port is < 1024 or > 65535 || e.MaxPairs is < 1 or > 16
            || e.TransientRetries is < 0 or > 10 || e.RetryBackoffSeconds is < 1 or > 300
            || e.MaxRepairAttempts is < 1 or > 100 || e.CommandTimeoutSeconds < 1
            || e.AgentTimeoutSeconds < 1 || e.MaxRunHours is <= 0 || e.MaxReportedTokens is <= 0 || e.MaxArtifactBytes < 1024
            || e.RawLogRetentionDays < 1 || (options.Copilot.CliPath is { } cli && !Path.IsPathFullyQualified(cli)))
            throw new InvalidDataException("Invalid host/execution settings.");
        return options;
    }
}
