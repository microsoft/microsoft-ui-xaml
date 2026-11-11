// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text.Json;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Launch;

// The milestones the profiler finds without a provider of their own.
public enum BuiltInMilestone
{
    // The process was created: time zero of the launch timeline, when the session knows it.
    ProcessCreated,
    // The first frame drawn on the first UI thread, which the Frames tab marks as the launch.
    FirstFrameDrawn
}

// The process whose event marks a milestone.
public enum MilestoneSource
{
    // The target process.
    App,
    // The target's launcher, the process that created it: for a launch from the app, the profiler or, with private WinUI
    // DLLs, the process that starts the app for it; in a trace, such as File Explorer or a shell.
    Launcher
}

// A field value an event needs to mark a milestone. A number matches a numeric field with that value; any other value
// matches the field's value as text, case-insensitively.
public sealed record FieldMatch(string Field, string Value, decimal? Number = null)
{
    public bool Matches(object? value)
    {
        if (value is null)
        {
            return false;
        }

        if (Number is { } number)
        {
            try
            {
                return value is not bool && value is IConvertible convertible &&
                    convertible.ToDecimal(CultureInfo.InvariantCulture) == number;
            }
            catch (Exception error) when (error is FormatException or InvalidCastException or OverflowException)
            {
                return false;
            }
        }

        return string.Equals(Convert.ToString(value, CultureInfo.InvariantCulture), Value, StringComparison.OrdinalIgnoreCase);
    }
}

// A milestone of the launch timeline: built in, or the first event of a type from a provider, among those with the field
// values Where gives. Label names it; with LabelFrom instead, the first event with each value of that field is a milestone
// named by the value. Process, an executable name without .exe, limits the milestone to that app's sessions. From is the
// process whose event marks it: for the launcher, the event nearest the target's creation, within
// MarkerProviderSettings.LauncherWindow of it.
public sealed record MilestoneDefinition(string? Label, BuiltInMilestone? BuiltIn, MarkerProvider? Provider, string? Event,
    string? LabelFrom, IReadOnlyList<FieldMatch> Where, string? Process, MilestoneSource From = MilestoneSource.App)
{
    // The provider and event, and the field values the event needs, such as "Contoso.App/ReadyToInteract"; null for a
    // built-in milestone.
    public string? EventDescription => Provider is null ? null
        : $"{Provider.Name}/{Event}" + (Where.Count == 0 ? "" : $" ({string.Join(", ", Where.Select(match => $"{match.Field}={match.Value}"))})");

    // Every milestone applies to a session whose process name is not known.
    public bool AppliesTo(string? processName) =>
        Process is null || processName is null || string.Equals(Process, MarkerConfig.WithoutExe(processName), StringComparison.OrdinalIgnoreCase);

    // Whether the event is of the milestone's type and has the field values it needs.
    public bool Matches(ProfilerEvent profilerEvent)
    {
        ArgumentNullException.ThrowIfNull(profilerEvent);
        return Provider is { } provider && provider.Id == profilerEvent.ProviderId &&
            string.Equals(Event, profilerEvent.Name, StringComparison.OrdinalIgnoreCase) &&
            Where.All(match => match.Matches(FieldValue(profilerEvent, match.Field)));
    }

    // Field names match case-insensitively.
    public static object? FieldValue(ProfilerEvent profilerEvent, string name)
    {
        ArgumentNullException.ThrowIfNull(profilerEvent);
        foreach (var field in profilerEvent.Fields)
        {
            if (string.Equals(field.Name, name, StringComparison.OrdinalIgnoreCase))
            {
                return field.Value;
            }
        }

        return null;
    }
}

// The marker configuration: the providers the profiler records along with the XAML providers, for the target process and,
// when a launcher milestone names one, for the process that launched it, and the launch timeline's milestones. A JSON
// file, which can have comments and trailing commas; Launch\markers.json is the default and the README's "Launch
// timeline" describes it.
public sealed class MarkerConfig
{
    public const int Version = 1;

    private const string DefaultResource = "WinUIProfiler.Engine.Launch.markers.json";

    private static readonly Lazy<string> DefaultContent = new(ReadDefault);
    private static readonly Lazy<MarkerConfig> DefaultConfig = new(() => Parse(DefaultContent.Value));

    private MarkerConfig(string? filePath, IReadOnlyList<MarkerProviderSettings> providers, IReadOnlyList<MilestoneDefinition> milestones)
    {
        FilePath = filePath;
        Providers = providers;
        Milestones = milestones;
    }

    // The file the configuration was read from; null for the default.
    public string? FilePath { get; }

    // Never a XAML provider, DxgKrnl or Win32k, which the profiler always records.
    public IReadOnlyList<MarkerProviderSettings> Providers { get; }

    // In the configuration's order.
    public IReadOnlyList<MilestoneDefinition> Milestones { get; }

    // The user's configuration, which the profiler reads when it exists and no other file is named.
    public static string UserPath { get; } = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUIProfiler", "markers.json");

    // The configuration the profiler comes with.
    public static MarkerConfig Default => DefaultConfig.Value;

    public static string DefaultText => DefaultContent.Value;

    // The configuration in the file at path; without one, the user's configuration when it exists, otherwise the default.
    // Throws MarkerConfigException when the file can't be read or is not valid.
    public static MarkerConfig Load(string? path = null)
    {
        if (path is null)
        {
            if (!File.Exists(UserPath))
            {
                return Default;
            }

            path = UserPath;
        }

        path = Path.GetFullPath(path);
        string text;
        try
        {
            text = File.ReadAllText(path);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            throw new MarkerConfigException($"Could not read the marker configuration '{path}': {error.Message}", error);
        }

        return Parse(text, path);
    }

    // Creates the user's configuration from the default unless it exists, and returns its path.
    public static string CreateUserFile()
    {
        Directory.CreateDirectory(Path.GetDirectoryName(UserPath)!);
        try
        {
            using var stream = new FileStream(UserPath, FileMode.CreateNew, FileAccess.Write);
            using var writer = new StreamWriter(stream);
            writer.Write(DefaultText);
        }
        catch (IOException) when (File.Exists(UserPath))
        {
        }

        return UserPath;
    }

    // Path is the file the text is from, which errors name; null for the default.
    public static MarkerConfig Parse(string text, string? path = null)
    {
        ArgumentNullException.ThrowIfNull(text);
        var reader = new Reader(path);
        try
        {
            using var document = JsonDocument.Parse(text, new JsonDocumentOptions
            {
                CommentHandling = JsonCommentHandling.Skip,
                AllowTrailingCommas = true
            });
            return reader.Read(document.RootElement);
        }
        catch (JsonException error)
        {
            throw new MarkerConfigException($"{reader.Source}: not valid JSON: {error.Message}", error);
        }
    }

    internal static string WithoutExe(string name) =>
        name.EndsWith(".exe", StringComparison.OrdinalIgnoreCase) ? name[..^4] : name;

    private static string ReadDefault()
    {
        using var stream = typeof(MarkerConfig).Assembly.GetManifestResourceStream(DefaultResource)
            ?? throw new InvalidOperationException($"The resource {DefaultResource} is missing.");
        using var reader = new StreamReader(stream);
        return reader.ReadToEnd();
    }

    private sealed class Reader(string? path)
    {
        private const string Root = "the file";

        public string Source { get; } = path ?? "The default marker configuration";

        public MarkerConfig Read(JsonElement root)
        {
            if (root.ValueKind != JsonValueKind.Object)
            {
                throw Error(Root, "must be a JSON object");
            }

            Allow(root, Root, "version", "providers", "milestones");
            if (!root.TryGetProperty("version", out var version) || version.ValueKind != JsonValueKind.Number ||
                !version.TryGetInt32(out var number) || number != Version)
            {
                throw Error(Root, $"needs \"version\": {Version}");
            }

            // Milestones name a provider: one of these, or one the profiler always records.
            var known = new Dictionary<string, MarkerProvider>(StringComparer.OrdinalIgnoreCase);
            foreach (var builtIn in Enum.GetValues<XamlProvider>().Where(provider => provider != XamlProvider.Marker))
            {
                known[XamlProviders.NameOf(builtIn)] = new(XamlProviders.NameOf(builtIn), XamlProviders.IdOf(builtIn));
            }

            var providers = new List<MarkerProviderSettings>();
            foreach (var (element, where) in Items(root, "providers", "provider", required: false))
            {
                var provider = ReadProvider(element, where, known, providers);
                providers.Add(provider);
                known[provider.Provider.Name] = provider.Provider;
            }

            var milestones = Items(root, "milestones", "milestone", required: true)
                .Select(item => ReadMilestone(item.Element, item.Where, known)).ToList();

            // Only a launcher milestone's provider is read from the launcher.
            var launcherProviders = milestones.Where(milestone => milestone.From == MilestoneSource.Launcher)
                .Select(milestone => milestone.Provider!.Id).ToHashSet();
            return new(path, [.. providers.Select(provider => launcherProviders.Contains(provider.Provider.Id) ? provider with { Launcher = true } : provider)],
                milestones);
        }

        private MarkerProviderSettings ReadProvider(JsonElement element, string where, Dictionary<string, MarkerProvider> known,
            List<MarkerProviderSettings> providers)
        {
            Allow(element, where, "name", "guid", "level", "keywords");
            var name = String(element, "name", where) ?? throw Error(where, "needs \"name\"");
            if (known.TryGetValue(name, out var listed))
            {
                throw Error(where, XamlProviders.IsBuiltIn(listed.Id)
                    ? $"the profiler always records {listed.Name}; milestones name it without listing it here"
                    : $"{listed.Name} is listed twice");
            }

            Guid id;
            if (String(element, "guid", where) is { } text)
            {
                if (!Guid.TryParse(text, out id))
                {
                    throw Error(where, $"\"guid\" is not a GUID: {text}");
                }
            }
            else
            {
                // The GUID EventSource and TraceLogging providers get from their name.
                id = TraceEventProviders.GetEventSourceGuidFromName(name);
            }

            if (XamlProviders.IsBuiltIn(id))
            {
                throw Error(where, $"{id} is a provider the profiler always records");
            }

            if (providers.Find(provider => provider.Provider.Id == id) is { } same)
            {
                throw Error(where, $"{name} has the GUID of {same.Provider.Name}, {id}");
            }

            byte level = 5;
            if (element.TryGetProperty("level", out var levelValue))
            {
                level = levelValue.ValueKind == JsonValueKind.Number && levelValue.TryGetByte(out var parsed) && parsed > 0
                    ? parsed : throw Error(where, "\"level\" must be from 1 (critical) to 5 (verbose), or a provider's own higher level");
            }

            var keywords = ulong.MaxValue;
            if (element.TryGetProperty("keywords", out var keywordsValue))
            {
                keywords = Keywords(keywordsValue) ??
                    throw Error(where, "\"keywords\" must be a number or a string such as \"0x400000000000\"");
            }

            return new(new(name, id), level, keywords);
        }

        private MilestoneDefinition ReadMilestone(JsonElement element, string where, Dictionary<string, MarkerProvider> known)
        {
            Allow(element, where, "label", "builtin", "provider", "event", "labelFrom", "where", "process", "from");
            var label = String(element, "label", where);
            var process = String(element, "process", where) is { } name ? WithoutExe(name) : null;
            if (String(element, "builtin", where) is { } builtInName)
            {
                var builtIn = Enum.GetValues<BuiltInMilestone>()
                    .Cast<BuiltInMilestone?>()
                    .FirstOrDefault(value => value.ToString()!.Equals(builtInName, StringComparison.OrdinalIgnoreCase))
                    ?? throw Error(where, $"\"builtin\" must be {string.Join(" or ", Enum.GetNames<BuiltInMilestone>())}");
                if (Array.Find(["provider", "event", "labelFrom", "where", "from"], other => element.TryGetProperty(other, out _)) is { } extra)
                {
                    throw Error(where, $"a built-in milestone has no \"{extra}\"");
                }

                return new(label ?? DefaultLabel(builtIn), builtIn, null, null, null, [], process);
            }

            var providerName = String(element, "provider", where) ?? throw Error(where, "needs \"builtin\", or \"provider\" and \"event\"");
            if (!known.TryGetValue(providerName, out var provider))
            {
                throw Error(where, $"the provider {providerName} is not in \"providers\"");
            }

            var eventName = String(element, "event", where) ?? throw Error(where, "needs \"event\" with \"provider\"");
            var labelFrom = String(element, "labelFrom", where);
            if ((label is null) == (labelFrom is null))
            {
                throw Error(where, "needs either \"label\" or \"labelFrom\"");
            }

            var from = MilestoneSource.App;
            if (String(element, "from", where) is { } fromName)
            {
                from = Enum.GetValues<MilestoneSource>()
                    .Cast<MilestoneSource?>()
                    .FirstOrDefault(value => value.ToString()!.Equals(fromName, StringComparison.OrdinalIgnoreCase))
                    ?? throw Error(where, "\"from\" must be app or launcher");
            }

            if (from == MilestoneSource.Launcher && XamlProviders.IsBuiltIn(provider.Id))
            {
                throw Error(where, $"the profiler records {provider.Name} only from the target, so a launcher milestone needs a provider from \"providers\"");
            }

            return new(label, null, provider, eventName, labelFrom, Conditions(element, where), process, from);
        }

        private List<FieldMatch> Conditions(JsonElement element, string where)
        {
            if (!element.TryGetProperty("where", out var conditions))
            {
                return [];
            }

            if (conditions.ValueKind != JsonValueKind.Object)
            {
                throw Error(where, "\"where\" must be an object of field names and values, such as { \"Items\": 200 }");
            }

            var matches = new List<FieldMatch>();
            foreach (var condition in conditions.EnumerateObject())
            {
                var value = condition.Value;
                matches.Add(value.ValueKind switch
                {
                    JsonValueKind.String => new(condition.Name, value.GetString()!),
                    JsonValueKind.Number => new(condition.Name, value.GetRawText(),
                        decimal.TryParse(value.GetRawText(), NumberStyles.Float, CultureInfo.InvariantCulture, out var number) ? number
                            : throw Error(where, $"the value of \"{condition.Name}\" is out of range")),
                    JsonValueKind.True or JsonValueKind.False => new(condition.Name, value.GetRawText()),
                    _ => throw Error(where, $"the value of \"{condition.Name}\" in \"where\" must be a string, a number, true or false")
                });
            }

            return matches;
        }

        // The array's objects, and how errors name each: counted from 1, as in "milestone 3".
        private List<(JsonElement Element, string Where)> Items(JsonElement root, string name, string item, bool required)
        {
            if (!root.TryGetProperty(name, out var array))
            {
                return required ? throw Error(Root, $"needs \"{name}\"") : [];
            }

            if (array.ValueKind != JsonValueKind.Array)
            {
                throw Error(Root, $"\"{name}\" must be an array");
            }

            var items = new List<(JsonElement, string)>();
            foreach (var element in array.EnumerateArray())
            {
                var where = $"{item} {items.Count + 1}";
                if (element.ValueKind != JsonValueKind.Object)
                {
                    throw Error(where, "must be an object");
                }

                items.Add((element, where));
            }

            return items;
        }

        private static ulong? Keywords(JsonElement value)
        {
            if (value.ValueKind == JsonValueKind.Number)
            {
                return value.TryGetUInt64(out var number) ? number : null;
            }

            if (value.ValueKind != JsonValueKind.String)
            {
                return null;
            }

            var text = value.GetString()!.Trim();
            return text.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                ? ulong.TryParse(text.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var hex) ? hex : null
                : ulong.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out var parsed) ? parsed : null;
        }

        private static string DefaultLabel(BuiltInMilestone builtIn) => builtIn switch
        {
            BuiltInMilestone.ProcessCreated => "Process created",
            _ => "First frame drawn"
        };

        // Property names are case-sensitive, as in JSON, so a misspelled one is reported rather than ignored.
        private void Allow(JsonElement element, string where, params string[] names)
        {
            foreach (var property in element.EnumerateObject())
            {
                if (Array.IndexOf(names, property.Name) < 0)
                {
                    throw Error(where, $"unknown property \"{property.Name}\" (expected {string.Join(", ", names.Select(name => $"\"{name}\""))})");
                }
            }
        }

        private string? String(JsonElement element, string name, string where)
        {
            if (!element.TryGetProperty(name, out var value))
            {
                return null;
            }

            return value.ValueKind == JsonValueKind.String && !string.IsNullOrWhiteSpace(value.GetString())
                ? value.GetString()!.Trim()
                : throw Error(where, $"\"{name}\" must be a non-empty string");
        }

        private MarkerConfigException Error(string where, string message) => new($"{Source}: {where}: {message}.");
    }
}

// A marker configuration that could not be read or is not valid; the message names the file and the problem.
public sealed class MarkerConfigException : Exception
{
    public MarkerConfigException(string message)
        : base(message)
    {
    }

    public MarkerConfigException(string message, Exception innerException)
        : base(message, innerException)
    {
    }
}
