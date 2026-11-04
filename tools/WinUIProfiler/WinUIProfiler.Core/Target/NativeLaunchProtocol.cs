using System.Text;
using System.Text.Json;

namespace WinUIProfiler.Engine.Target;

// Launches the target with private builds of Microsoft.UI.Xaml.dll, and of Microsoft.UI.Xaml.Controls.dll when
// RedirectControls is set, from RuntimeDirectory instead of SourceDirectory, the folder the app loads them from.
// LauncherDirectory is the deployed NativeLauncher folder; each launch writes the launcher's copy, result and log to a
// new numbered folder in LaunchDirectory.
public sealed record PrivateRuntimeLaunch(string RuntimeDirectory, string SourceDirectory, bool RedirectControls,
    string LaunchDirectory, string LauncherDirectory);

// The launcher's launch.json. Error is a Win32 error code; Stage is "ready", or the step that failed.
public sealed record NativeLaunchResult(int ProcessId, uint Error, string Stage, string PackageFullName, string ApplicationUserModelId);

// The private WinUI launcher's command line and result files (WinUIProfiler.Launcher\README.md).
public static class NativeLaunchProtocol
{
    public const string ResultFile = "launch.json";
    public const string LogFile = "redirect.jsonl";
    public const string OwnerSuffix = ".owner.json";
    public const int MaxResultBytes = 4096;

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
            Number(root, "schemaVersion") != 1 ||
            Number(root, "pid") is not { } pid || pid is < 0 or > int.MaxValue ||
            Number(root, "error") is not { } error || error is < 0 or > uint.MaxValue ||
            Text(root, "stage") is not { Length: > 0 } stage ||
            (error == 0 && (pid == 0 || stage != "ready")))
        {
            throw new InvalidDataException("The private WinUI launcher wrote an invalid result.");
        }

        return new((int)pid, (uint)error, stage, Text(root, "packageFullName") ?? "", Text(root, "aumid") ?? "");
    }

    // The launcher's process ID from the owner file it writes as it starts.
    public static int ParseOwner(string json)
    {
        using var document = JsonDocument.Parse(json);
        var root = document.RootElement;
        if (root.ValueKind != JsonValueKind.Object ||
            Number(root, "schemaVersion") != 1 ||
            Text(root, "stage") != "launcher" ||
            Number(root, "pid") is not { } pid || pid is < 1 or > int.MaxValue)
        {
            throw new InvalidDataException("The private WinUI launcher wrote an invalid owner file.");
        }

        return (int)pid;
    }

    private static long? Number(JsonElement root, string name) =>
        root.TryGetProperty(name, out var value) && value.ValueKind == JsonValueKind.Number && value.TryGetInt64(out var number)
            ? number : null;

    private static string? Text(JsonElement root, string name) =>
        root.TryGetProperty(name, out var value) && value.ValueKind == JsonValueKind.String ? value.GetString() : null;
}
