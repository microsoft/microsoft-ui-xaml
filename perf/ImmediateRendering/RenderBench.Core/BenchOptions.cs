using System.Globalization;

namespace RenderBench;

public enum SceneId
{
    Settings,   // S1: static settings form
    Dashboard,  // S2: live dashboard, sparse updates
}

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

    public const string Usage =
        "RenderBench --scene settings|dashboard [--warmup s] [--settle s] [--interaction s] [--recovery s] " +
        "[--seed n] [--width px] [--height px] [--sample-ms n] [--tiles n] [--update-fraction f] [--update-hz f] " +
        "[--input-hz f] [--screenshot path.bmp] [--no-exit]";

    public static bool TryParse(IReadOnlyList<string> args, out BenchOptions options, out string? error)
    {
        options = new BenchOptions();
        error = null;
        for (int i = 0; i < args.Count; i++)
        {
            string arg = args[i];
            if (arg == "--no-exit")
            {
                options = options with { Exit = false };
                continue;
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
                    if (!Enum.TryParse<SceneId>(value, ignoreCase: true, out var scene) || !Enum.IsDefined(scene))
                    {
                        error = $"Unknown scene '{value}'";
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
                case "--screenshot": options = options with { ScreenshotPath = value }; break;
                default:
                    error = $"Unknown argument {arg}";
                    return false;
            }
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
