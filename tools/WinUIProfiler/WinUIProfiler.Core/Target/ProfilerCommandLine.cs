using WinUIProfiler.Engine.Symbols;

namespace WinUIProfiler.Engine.Target;

// The profiler's command line: the app to launch, optionally with private WinUI DLLs, and whether to connect at once. The
// symbol path, which may be given alone, is where the recording's CPU samples are symbolized from.
public sealed record ProfilerCommandLine(TargetLaunchKind? Kind, string Target, string Arguments, string? PrivateRuntime,
    string? RuntimeSource, bool RedirectControls, string? LaunchDirectory, bool TrustLocalBinaries, bool Connect,
    string? SymbolPath = null)
{
    public static ProfilerCommandLine Empty { get; } = new(null, "", "", null, null, false, null, false, false);

    private static readonly string[] Values =
        ["--launch-exe", "--launch-aumid", "--arguments", "--private-runtime", "--runtime-source", "--launch-directory",
            "--symbol-path"];
    private static readonly string[] Flags = ["--redirect-controls", "--trust-local-binaries", "--connect"];

    public static ProfilerCommandLine Parse(IReadOnlyList<string> arguments)
    {
        ArgumentNullException.ThrowIfNull(arguments);
        var options = new Dictionary<string, string>(StringComparer.Ordinal);
        for (var index = 0; index < arguments.Count; index++)
        {
            var name = arguments[index];
            string value;
            if (Values.Contains(name, StringComparer.Ordinal))
            {
                if (++index == arguments.Count)
                {
                    throw new ArgumentException($"{name} needs a value.");
                }

                value = arguments[index];
            }
            else if (Flags.Contains(name, StringComparer.Ordinal))
            {
                value = "";
            }
            else
            {
                throw new ArgumentException($"Unknown option: {name}");
            }

            if (!options.TryAdd(name, value))
            {
                throw new ArgumentException($"{name} is repeated.");
            }
        }

        string? Value(string name) => options.GetValueOrDefault(name);
        var executable = Value("--launch-exe");
        var applicationId = Value("--launch-aumid");
        var privateRuntime = Value("--private-runtime");
        var runtimeSource = Value("--runtime-source");
        var launchDirectory = Value("--launch-directory");
        var symbolPath = Value("--symbol-path");
        if (symbolPath is not null)
        {
            try
            {
                _ = SymbolPaths.Normalize(symbolPath);
            }
            catch (ArgumentException error)
            {
                throw new ArgumentException($"--symbol-path: {error.Message}", error);
            }
        }

        if (executable is not null && applicationId is not null)
        {
            throw new ArgumentException("Use --launch-exe or --launch-aumid, not both.");
        }

        if (executable is null && applicationId is null)
        {
            return options.Count == (symbolPath is null ? 0 : 1) ? Empty with { SymbolPath = symbolPath } :
                throw new ArgumentException("The options need an app: --launch-exe <path> or --launch-aumid <AUMID>.");
        }

        if ((executable is not null && !Path.IsPathFullyQualified(executable)) || string.IsNullOrWhiteSpace(applicationId ?? executable))
        {
            throw new ArgumentException("--launch-exe needs a full path; --launch-aumid needs an AUMID.");
        }

        string?[] folders = [privateRuntime, runtimeSource, launchDirectory];
        if (folders.Any(folder => folder is not null) &&
            !folders.All(folder => folder is not null && Path.IsPathFullyQualified(folder)))
        {
            throw new ArgumentException("--private-runtime, --runtime-source and --launch-directory go together, as full paths.");
        }

        if (options.ContainsKey("--redirect-controls") && privateRuntime is null)
        {
            throw new ArgumentException("--redirect-controls needs --private-runtime.");
        }

        if (applicationId is not null && privateRuntime is not null && options.ContainsKey("--arguments"))
        {
            throw new ArgumentException("A packaged app launched with private WinUI DLLs is activated without --arguments.");
        }

        return new(executable is null ? TargetLaunchKind.Packaged : TargetLaunchKind.Unpackaged, (applicationId ?? executable)!,
            Value("--arguments") ?? "", privateRuntime, runtimeSource, options.ContainsKey("--redirect-controls"), launchDirectory,
            options.ContainsKey("--trust-local-binaries"), options.ContainsKey("--connect"), symbolPath);
    }

    public PrivateRuntimeLaunch? GetPrivateRuntime(string launcherDirectory) => PrivateRuntime is null
        ? null
        : new(PrivateRuntime, RuntimeSource!, RedirectControls, LaunchDirectory!, launcherDirectory);
}
