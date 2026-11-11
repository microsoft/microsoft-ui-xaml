using System.Text;
using System.Text.Json;

namespace WinUIProfiler.Engine.Target;

// Launches the target with private builds of Microsoft.UI.Xaml.dll, and of Microsoft.UI.Xaml.Controls.dll when
// RedirectControls is set, from RuntimeDirectory instead of SourceDirectory, the folder the app loads them from.
// LauncherDirectory is the deployed NativeLauncher folder; each launch writes the launcher's copy, result and log to a
// new numbered folder in LaunchDirectory.
public sealed record PrivateRuntimeLaunch(string RuntimeDirectory, string SourceDirectory, bool RedirectControls,
    string LaunchDirectory, string LauncherDirectory);

// The launcher's launch.json. Error is a Win32 error code; Stage is "ready", or the step that failed. ProcessId and
// CreationTimeFileTime (a UTC FILETIME) name the app's process instance, since a later process can reuse the ID.
// ProcessId is 0 when no process started; CreationTimeFileTime is 0 when the launcher could not read it.
public sealed record NativeLaunchResult(int ProcessId, long CreationTimeFileTime, uint Error, string Stage,
    string PackageFullName, string ApplicationUserModelId)
{
    public int? ActivationProcessId { get; init; }
    public IReadOnlyList<NativeLaunchProcess> Processes { get; init; } = [];
}

public sealed record NativeLaunchProcess(int ProcessId, int ParentProcessId, long CreationTimeFileTime,
    bool HelperReady, bool RuntimeLoaded, uint Error);

// The private WinUI launcher's command line and result files (WinUIProfiler.Launcher\README.md).
public static class NativeLaunchProtocol
{
    public const string ResultFile = "launch.json";
    public const string LogFile = "redirect.jsonl";
    public const string OwnerSuffix = ".owner.json";
    public const string AcceptedSuffix = ".accepted";
    public const int MaxResultBytes = 16384;
    public const int MaxStartupProcesses = 8;

    public static string LogFileFor(NativeLaunchResult result, int? processId = null)
    {
        var pid = processId ?? result.ProcessId;
        return result.ActivationProcessId is not { } activation || pid == activation
            ? LogFile : $"redirect.{pid}.jsonl";
    }

    // Quotes an argument so that CommandLineToArgvW reads it back unchanged.
    public static string Quote(string value)
    {
        ArgumentNullException.ThrowIfNull(value);
        if (value.Length > 0 && !value.Any(c => char.IsWhiteSpace(c) || c == '"'))
        {
            return value;
        }

        var quoted = new StringBuilder("\"");
        var backslashes = 0;
        foreach (var c in value)
        {
            if (c == '\\')
            {
                backslashes++;
                continue;
            }

            quoted.Append('\\', c == '"' ? backslashes * 2 + 1 : backslashes).Append(c);
            backslashes = 0;
        }

        return quoted.Append('\\', backslashes * 2).Append('"').ToString();
    }

    // A packaged app is activated without arguments, so applicationUserModelId and arguments are exclusive.
    public static string BuildArguments(string executable, PrivateRuntimeLaunch runtime, string arguments, string log,
        string result, string? applicationUserModelId, string? packageFullName)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        ArgumentNullException.ThrowIfNull(arguments);
        if ((applicationUserModelId is null) != (packageFullName is null) ||
            (applicationUserModelId is not null && arguments.Length > 0))
        {
            throw new ArgumentException("A packaged launch needs its package and takes no arguments.");
        }

        List<string> words =
        [
            "--exe", executable, "--source", runtime.SourceDirectory, "--private", runtime.RuntimeDirectory,
            "--controls", runtime.RedirectControls ? "1" : "0", "--log", log, "--result", result
        ];
        if (applicationUserModelId is not null)
        {
            words.AddRange(["--aumid", applicationUserModelId, "--package", packageFullName!, "--expected-aumid", applicationUserModelId]);
        }
        else if (arguments.Length > 0)
        {
            words.AddRange(["--arguments", arguments]);
        }

        return string.Join(' ', words.Select(Quote));
    }

    public static NativeLaunchResult ParseResult(string json)
    {
        using var document = JsonDocument.Parse(json);
        var root = document.RootElement;
        if (root.ValueKind != JsonValueKind.Object ||
            Number(root, "schemaVersion") is not (1 or 2) ||
            Number(root, "pid") is not { } pid || pid is < 0 or > int.MaxValue ||
            Number(root, "creationTimeFileTime") is not { } created || created < 0 ||
            Number(root, "error") is not { } error || error is < 0 or > uint.MaxValue ||
            Text(root, "stage") is not { Length: > 0 } stage ||
            (error == 0 && (pid == 0 || created == 0 || stage != "ready")))
        {
            throw new InvalidDataException("The private WinUI launcher wrote an invalid result.");
        }

        var result = new NativeLaunchResult((int)pid, created, (uint)error, stage,
            Text(root, "packageFullName") ?? "", Text(root, "aumid") ?? "");
        if (Number(root, "schemaVersion") == 1)
        {
            return result;
        }

        if (Number(root, "activationPid") is not { } activation || activation is < 0 or > int.MaxValue ||
            !root.TryGetProperty("processes", out var entries) || entries.ValueKind != JsonValueKind.Array ||
            entries.GetArrayLength() > MaxStartupProcesses)
        {
            throw new InvalidDataException("The private WinUI launcher's startup process inventory is invalid.");
        }

        var processes = new Dictionary<int, NativeLaunchProcess>();
        foreach (var entry in entries.EnumerateArray())
        {
            if (entry.ValueKind != JsonValueKind.Object ||
                Number(entry, "pid") is not { } entryPid || entryPid is < 1 or > int.MaxValue ||
                Number(entry, "parentPid") is not { } parentPid || parentPid is < 0 or > int.MaxValue ||
                Number(entry, "creationTimeFileTime") is not { } entryCreated ||
                entryCreated is < 1 or > 2650467743999999999L ||
                Boolean(entry, "helperReady") is not { } helperReady ||
                Boolean(entry, "runtimeLoaded") is not { } runtimeLoaded ||
                Number(entry, "error") is not { } entryError || entryError is < 0 or > uint.MaxValue ||
                (runtimeLoaded && !helperReady) ||
                processes.ContainsKey((int)entryPid) ||
                (parentPid == 0 ? entryPid != activation :
                    !processes.TryGetValue((int)parentPid, out var parent) || parent.CreationTimeFileTime > entryCreated))
            {
                throw new InvalidDataException("The private WinUI launcher's startup ancestry or process identity is invalid.");
            }

            processes.Add((int)entryPid, new((int)entryPid, (int)parentPid, entryCreated, helperReady, runtimeLoaded,
                (uint)entryError));
        }

        if (error == 0 &&
            (!processes.TryGetValue((int)pid, out var selected) || selected.CreationTimeFileTime != created ||
                !selected.RuntimeLoaded || !selected.HelperReady || selected.Error != 0 ||
                processes.Values.Any(process => process.Error != 0)))
        {
            throw new InvalidDataException("The private WinUI launcher's selected process did not load the private runtime.");
        }

        return result with { ActivationProcessId = (int)activation, Processes = processes.Values.ToArray() };
    }

    // The launcher's process ID and creation time from the owner file it writes as it starts.
    public static (int ProcessId, long CreationTimeFileTime) ParseOwner(string json)
    {
        using var document = JsonDocument.Parse(json);
        var root = document.RootElement;
        if (root.ValueKind != JsonValueKind.Object ||
            Number(root, "schemaVersion") != 1 ||
            Text(root, "stage") != "launcher" ||
            Number(root, "pid") is not { } pid || pid is < 1 or > int.MaxValue ||
            Number(root, "creationTimeFileTime") is not { } created || created < 1)
        {
            throw new InvalidDataException("The private WinUI launcher wrote an invalid owner file.");
        }

        return ((int)pid, created);
    }

    private static long? Number(JsonElement root, string name) =>
        root.TryGetProperty(name, out var value) && value.ValueKind == JsonValueKind.Number && value.TryGetInt64(out var number)
            ? number : null;

    private static string? Text(JsonElement root, string name) =>
        root.TryGetProperty(name, out var value) && value.ValueKind == JsonValueKind.String ? value.GetString() : null;

    private static bool? Boolean(JsonElement root, string name) =>
        root.TryGetProperty(name, out var value) && value.ValueKind is JsonValueKind.True or JsonValueKind.False
            ? value.GetBoolean() : null;
}
