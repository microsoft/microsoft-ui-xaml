using System.Globalization;

namespace RenderBench;

// Environment variables the native immediate backends read once at startup (test hooks).
public static class ImmediateHooks
{
    // ImmediateRenderer: after every paint, write the presented buffer as a 32bpp top-down BMP to this path (S22).
    public const string CaptureVariable = "WINUI_IMMEDIATE_CAPTURE";
    // "every:N" or "f1,f2,...": the painter reports its device as removed at those frames (S21).
    public const string SimulateDeviceLostVariable = "WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST";
    public const string DefaultDeviceLost = "every:10";

    public static bool TryNormalizeDeviceLost(string text, out string normalized)
    {
        normalized = "";
        string value = text.Trim();
        if (value.StartsWith("every:", StringComparison.OrdinalIgnoreCase))
        {
            if (!TryFrame(value["every:".Length..], out uint every))
            {
                return false;
            }
            normalized = $"every:{every}";
            return true;
        }
        var frames = new SortedSet<uint>();
        foreach (string part in value.Split(','))
        {
            if (!TryFrame(part, out uint frame))
            {
                return false;
            }
            frames.Add(frame);
        }
        normalized = string.Join(",", frames);
        return true;
    }

    static bool TryFrame(string text, out uint frame)
        => uint.TryParse(text.Trim(), NumberStyles.None, CultureInfo.InvariantCulture, out frame) && frame >= 1;
}
