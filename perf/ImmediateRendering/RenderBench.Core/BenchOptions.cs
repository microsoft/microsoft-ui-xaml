using System.Globalization;

namespace RenderBench;

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

    // Scene name used in RunStarted and in reports; variants are separate experiments, so they get separate labels.
    public string SceneLabel => Scene switch
    {
        SceneId.Gallery when DecodeToSize => "Gallery-DecodeToSize",
        SceneId.Animated when NoProgressRing => "Animated-NoProgressRing",
        SceneId.MultiWindow => Tabs ? "MultiWindow-Tabs" : $"MultiWindow-{Windows}",
        _ => SceneCatalog.Get(Scene).DisplayName,
    };

    public static readonly string Usage =
        $"RenderBench --scene {SceneCatalog.CliNames} (or s1..s10) [--warmup s] [--settle s] [--interaction s] [--recovery s] " +
        "[--seed n] [--width px] [--height px] [--sample-ms n] [--tiles n] [--update-fraction f] [--update-hz f] " +
        "[--input-hz f] [--input-mode auto|sendinput|programmatic] [--scroll-speed dips/s] [--items n] " +
        "[--decode-to-size] [--no-progressring] [--windows n] [--tabs] [--step-seconds s] " +
        "[--screenshot path.bmp] [--screenshot-method auto|screen|printwindow] [--no-exit]";

    static readonly Dictionary<string, SceneId> s_variantScenes = new()
    {
        ["--decode-to-size"] = SceneId.Gallery,
        ["--no-progressring"] = SceneId.Animated,
        ["--tabs"] = SceneId.MultiWindow,
        ["--windows"] = SceneId.MultiWindow,
        ["--step-seconds"] = SceneId.MultiWindow,
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
                        error = $"Unknown scene '{value}'; expected {SceneCatalog.CliNames} or s1..s10";
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
        };
        return true;
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
