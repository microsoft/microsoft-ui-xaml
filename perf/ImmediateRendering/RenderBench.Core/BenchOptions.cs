using System.Globalization;

namespace RenderBench;

// Composition engine for the composition backend: Lifted (WinAppSDK in-process, default) or System (OS compositor via
// CompositionEngine.TrySetProcessEngine; a Limited Access Feature, or the internal Switcher\ProcessOverrides key).
public enum CompositionEngineChoice { Lifted, System }

// XamlOptionalChanges opt-ins applied before Application.Start.
public enum OptionalChangesChoice { None, All }

public sealed record BenchOptions
{
    public SceneId Scene { get; init; } = SceneId.Settings;
    public double WarmupSeconds { get; init; } = 5;
    public double SettleSeconds { get; init; } = 10;
    public double InteractionSeconds { get; init; } = 30;
    public double RecoverySeconds { get; init; } = 5;
    public int Seed { get; init; } = 1;
    public int Width { get; init; } = 1280;
    public int Height { get; init; } = 800;
    public int SampleIntervalMs { get; init; } = 500;
    public int Tiles { get; init; } = 200;
    public double UpdateFraction { get; init; } = 0.10;
    public double UpdateHz { get; init; } = 10;
    public double InputHz { get; init; } = 2;
    public string? ScreenshotPath { get; init; }
    public bool Exit { get; init; } = true;
    // ---- Extended scenes (S3-S10) ----
    public InputMode InputMode { get; init; } = InputMode.Auto;
    public ScreenshotMethod ScreenshotMethod { get; init; } = ScreenshotMethod.Auto;
    // Programmatic scroll speed in DIPs per second (S3, S4, S5, S8); 0 for scenes that don't scroll.
    public double ScrollSpeed { get; init; }
    // Items in the scene's collection: S3 mails, S4 paragraphs, S5 images, S8 rows.
    public int Items { get; init; }
    public bool DecodeToSize { get; init; }      // S5: DecodePixelWidth = 128
    public bool NoProgressRing { get; init; }    // S6: exclude the Lottie-based ProgressRing
    public int Windows { get; init; } = 10;      // S9
    public bool Tabs { get; init; }              // S9: one window with a 20-tab TabView instead of N windows
    public double StepSeconds { get; init; } = 2; // S9: time between lifecycle steps
    public CompositionEngineChoice CompositionEngine { get; init; } = CompositionEngineChoice.Lifted;
    public OptionalChangesChoice OptionalChanges { get; init; } = OptionalChangesChoice.None;
    // ---- Full-backend scenes (S11-S23) ----
    public bool NoInterop { get; init; }   // S11: same layout without ElementCompositionPreview/connected animation
    public bool NoPlayer { get; init; }    // S12: without the custom AnimatedVisualPlayer
    public bool NoMedia { get; init; }     // S16: SwapChainPanel only, no MediaPlayerElement
    // S21: WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST value ("every:N" or a sorted frame list); null for other scenes.
    public string? DeviceLost { get; init; }
    // ---- coverage ----
    // XAML coverage page to render instead of a scene ("F8-controls/Button"), and the directory for its app.json and
    // capture.bmp (scripts\Run-Coverage.ps1). The window defaults to 1000x700 client pixels for coverage pages.
    public string? CoveragePage { get; init; }
    public string? CoverageOut { get; init; }
    // Writes the discovered coverage catalog (catalog.json) to this path and exits.
    public string? ListCoverage { get; init; }
    public const int CoverageWidth = 1000, CoverageHeight = 700;

    // Scene name used in RunStarted and in reports; variants are separate experiments, so they get separate labels.
    public string SceneLabel => Scene switch
    {
        SceneId.Gallery when DecodeToSize => "Gallery-DecodeToSize",
        SceneId.Animated when NoProgressRing => "Animated-NoProgressRing",
        SceneId.MultiWindow => Tabs ? "MultiWindow-Tabs" : $"MultiWindow-{Windows}",
        SceneId.CompositionInterop when NoInterop => "CompositionInterop-NoInterop",
        SceneId.Lottie when NoPlayer => "Lottie-NoPlayer",
        SceneId.SwapChainMedia when NoMedia => "SwapChainMedia-NoMedia",
        _ => SceneCatalog.Get(Scene).DisplayName,
    };

    public static readonly string Usage =
        $"RenderBench --scene {SceneCatalog.CliNames} (or {SceneCatalog.PlanIdRange}) [--warmup s] [--settle s] [--interaction s] [--recovery s] " +
        "[--seed n] [--width px] [--height px] [--sample-ms n] [--tiles n] [--update-fraction f] [--update-hz f] " +
        "[--input-hz f] [--input-mode auto|sendinput|programmatic] [--scroll-speed dips/s] [--items n] " +
        "[--decode-to-size] [--no-progressring] [--windows n] [--tabs] [--step-seconds s] " +
        "[--no-interop] [--no-player] [--no-media] [--device-lost every:N|frame,frame,...] " +
        "[--screenshot path.bmp] [--screenshot-method auto|screen|printwindow|xaml] " +
        "[--coverage F<n>-family/name --coverage-out dir] [--list-coverage catalog.json] " +
        "[--composition-engine lifted|system] [--optional-changes none|all] [--no-exit]";

    // The engine and optional changes must be set before Application.Start, before OnLaunched parses the full command
    // line; this lenient pre-pass reads only those two (TryParse validates them later).
    public static (CompositionEngineChoice Engine, OptionalChangesChoice Changes) ParseStartup(IReadOnlyList<string> args)
    {
        var engine = CompositionEngineChoice.Lifted;
        var changes = OptionalChangesChoice.None;
        for (int i = 0; i + 1 < args.Count; i++)
        {
            if (args[i] == "--composition-engine" && Enum.TryParse(args[i + 1], ignoreCase: true, out CompositionEngineChoice e))
            {
                engine = e;
            }
            else if (args[i] == "--optional-changes" && Enum.TryParse(args[i + 1], ignoreCase: true, out OptionalChangesChoice c))
            {
                changes = c;
            }
        }
        return (engine, changes);
    }

    static readonly Dictionary<string, SceneId> s_variantScenes = new()
    {
        ["--decode-to-size"] = SceneId.Gallery,
        ["--no-progressring"] = SceneId.Animated,
        ["--tabs"] = SceneId.MultiWindow,
        ["--windows"] = SceneId.MultiWindow,
        ["--step-seconds"] = SceneId.MultiWindow,
        ["--no-interop"] = SceneId.CompositionInterop,
        ["--no-player"] = SceneId.Lottie,
        ["--no-media"] = SceneId.SwapChainMedia,
        ["--device-lost"] = SceneId.DeviceLost,
    };

    public static bool TryParse(IReadOnlyList<string> args, out BenchOptions options, out string? error)
    {
        options = new BenchOptions();
        error = null;
        var seen = new HashSet<string>();
        for (int i = 0; i < args.Count; i++)
        {
            string arg = args[i];
            seen.Add(arg);
            switch (arg)
            {
                case "--no-exit": options = options with { Exit = false }; continue;
                case "--decode-to-size": options = options with { DecodeToSize = true }; continue;
                case "--no-progressring": options = options with { NoProgressRing = true }; continue;
                case "--tabs": options = options with { Tabs = true }; continue;
                case "--no-interop": options = options with { NoInterop = true }; continue;
                case "--no-player": options = options with { NoPlayer = true }; continue;
                case "--no-media": options = options with { NoMedia = true }; continue;
            }
            if (i + 1 >= args.Count)
            {
                error = $"Missing value for {arg}";
                return false;
            }
            string value = args[++i];
            switch (arg)
            {
                case "--scene":
                    if (!SceneCatalog.TryParse(value, out var scene))
                    {
                        error = $"Unknown scene '{value}'; expected {SceneCatalog.CliNames} or {SceneCatalog.PlanIdRange}";
                        return false;
                    }
                    options = options with { Scene = scene };
                    break;
                case "--warmup": if (!TryPositive(value, arg, out var w, ref error)) return false; options = options with { WarmupSeconds = w }; break;
                case "--settle": if (!TryPositive(value, arg, out var s, ref error)) return false; options = options with { SettleSeconds = s }; break;
                case "--interaction": if (!TryPositive(value, arg, out var n, ref error)) return false; options = options with { InteractionSeconds = n }; break;
                case "--recovery": if (!TryPositive(value, arg, out var r, ref error)) return false; options = options with { RecoverySeconds = r }; break;
                case "--update-hz": if (!TryPositive(value, arg, out var hz, ref error, allowZero: false)) return false; options = options with { UpdateHz = hz }; break;
                case "--input-hz": if (!TryPositive(value, arg, out var ihz, ref error)) return false; options = options with { InputHz = ihz }; break;
                case "--scroll-speed": if (!TryPositive(value, arg, out var speed, ref error, allowZero: false)) return false; options = options with { ScrollSpeed = speed }; break;
                case "--step-seconds": if (!TryPositive(value, arg, out var step, ref error, allowZero: false)) return false; options = options with { StepSeconds = step }; break;
                case "--update-fraction":
                    if (!TryPositive(value, arg, out var f, ref error) || f > 1)
                    {
                        error ??= $"{arg} must be in [0,1]";
                        return false;
                    }
                    options = options with { UpdateFraction = f };
                    break;
                case "--seed": if (!TryInt(value, arg, 0, out var seed, ref error)) return false; options = options with { Seed = seed }; break;
                case "--width": if (!TryInt(value, arg, 320, out var width, ref error)) return false; options = options with { Width = width }; break;
                case "--height": if (!TryInt(value, arg, 240, out var height, ref error)) return false; options = options with { Height = height }; break;
                case "--sample-ms": if (!TryInt(value, arg, 50, out var ms, ref error)) return false; options = options with { SampleIntervalMs = ms }; break;
                case "--tiles": if (!TryInt(value, arg, 1, out var tiles, ref error)) return false; options = options with { Tiles = tiles }; break;
                case "--items": if (!TryInt(value, arg, 1, out var items, ref error)) return false; options = options with { Items = items }; break;
                case "--windows":
                    if (!TryInt(value, arg, 1, out var windows, ref error) || windows > 64)
                    {
                        error ??= $"{arg} expects 1..64, got '{value}'";
                        return false;
                    }
                    options = options with { Windows = windows };
                    break;
                case "--input-mode":
                    if (!TryEnum<InputMode>(value, arg, out var mode, ref error)) return false;
                    options = options with { InputMode = mode };
                    break;
                case "--screenshot-method":
                    if (!TryEnum<ScreenshotMethod>(value, arg, out var method, ref error)) return false;
                    options = options with { ScreenshotMethod = method };
                    break;
                case "--screenshot": options = options with { ScreenshotPath = value }; break;
                case "--coverage": options = options with { CoveragePage = value }; break;
                case "--coverage-out": options = options with { CoverageOut = value }; break;
                case "--list-coverage": options = options with { ListCoverage = value }; break;
                case "--device-lost":
                    if (!ImmediateHooks.TryNormalizeDeviceLost(value, out var spec))
                    {
                        error = $"{arg} expects every:N (N >= 1) or a comma-separated list of frame numbers >= 1, got '{value}'";
                        return false;
                    }
                    options = options with { DeviceLost = spec };
                    break;
                case "--composition-engine":
                    if (!TryEnum<CompositionEngineChoice>(value, arg, out var engine, ref error)) return false;
                    options = options with { CompositionEngine = engine };
                    break;
                case "--optional-changes":
                    if (!TryEnum<OptionalChangesChoice>(value, arg, out var changes, ref error)) return false;
                    options = options with { OptionalChanges = changes };
                    break;
                default:
                    error = $"Unknown argument {arg}";
                    return false;
            }
        }

        foreach (var (variant, owner) in s_variantScenes)
        {
            if (seen.Contains(variant) && options.Scene != owner)
            {
                error = $"{variant} applies only to --scene {SceneCatalog.Get(owner).CliName}";
                return false;
            }
        }

        var defaults = SceneCatalog.Get(options.Scene).Defaults;
        options = options with
        {
            InteractionSeconds = seen.Contains("--interaction") ? options.InteractionSeconds : defaults.InteractionSeconds,
            UpdateHz = seen.Contains("--update-hz") ? options.UpdateHz : defaults.UpdateHz,
            InputHz = seen.Contains("--input-hz") ? options.InputHz : defaults.InputHz,
            ScrollSpeed = seen.Contains("--scroll-speed") ? options.ScrollSpeed : defaults.ScrollSpeed,
            Items = seen.Contains("--items") ? options.Items : defaults.Items,
            DeviceLost = options.Scene == SceneId.DeviceLost ? options.DeviceLost ?? ImmediateHooks.DefaultDeviceLost : null,
        };
        if (options.CoveragePage is not null)
        {
            if (seen.Contains("--scene"))
            {
                error = "--coverage and --scene are exclusive";
                return false;
            }
            options = options with
            {
                Width = seen.Contains("--width") ? options.Width : CoverageWidth,
                Height = seen.Contains("--height") ? options.Height : CoverageHeight,
            };
        }
        else if (options.CoverageOut is not null)
        {
            error = "--coverage-out applies only to --coverage";
            return false;
        }
        return true;
    }

    // Native test hooks are read once at startup, so Program.Main sets them before Application.Start. Only immediate
    // modes honor them: Composition has no device-lost hook, and S22 captures the screen instead. A value already in the
    // environment wins (manual experiments). Returns nothing when the command line does not parse.
    public static IReadOnlyDictionary<string, string> StartupEnvironment(IReadOnlyList<string> args, string? backend, int processId,
        string dataDirectory, Func<string, string?> getEnvironment)
    {
        var result = new Dictionary<string, string>(StringComparer.Ordinal);
        if (!TryParse(args, out var options, out _) ||
            !BackendModes.TryParse(backend, out var mode) || mode == BackendMode.Composition)
        {
            return result;
        }
        void Set(string name, string value)
        {
            if (string.IsNullOrEmpty(getEnvironment(name)))
            {
                result[name] = value;
            }
        }
        if (options.Scene == SceneId.DeviceLost && options.DeviceLost is { } spec)
        {
            Set(ImmediateHooks.SimulateDeviceLostVariable, spec);
        }
        if (options.Scene == SceneId.Accessibility)
        {
            Set(ImmediateHooks.CaptureVariable, Path.Combine(dataDirectory, $"capture-{processId}.bmp"));
        }
        return result;
    }

    static bool TryEnum<T>(string value, string arg, out T result, ref string? error) where T : struct, Enum
    {
        if (!value.All(char.IsLetter) || !Enum.TryParse(value, ignoreCase: true, out result))
        {
            result = default;
            error = $"{arg} expects {string.Join("|", Enum.GetNames<T>().Select(n => n.ToLowerInvariant()))}, got '{value}'";
            return false;
        }
        return true;
    }

    static bool TryPositive(string value, string arg, out double result, ref string? error, bool allowZero = true)
    {
        if (!double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out result) ||
            result < 0 || (!allowZero && result == 0) || double.IsNaN(result) || double.IsInfinity(result))
        {
            error = $"{arg} expects a {(allowZero ? "non-negative" : "positive")} number, got '{value}'";
            return false;
        }
        return true;
    }

    static bool TryInt(string value, string arg, int min, out int result, ref string? error)
    {
        if (!int.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out result) || result < min)
        {
            error = $"{arg} expects an integer >= {min}, got '{value}'";
            return false;
        }
        return true;
    }
}
