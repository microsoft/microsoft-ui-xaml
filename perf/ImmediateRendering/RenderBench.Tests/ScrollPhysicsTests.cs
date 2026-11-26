using RenderBench.Analysis;
using static RenderBench.Tests.Check;

namespace RenderBench.Tests;

// ---- scrollgaps ---- S33 scroll gap measurements: blit/repaint frame counts (gap 5), overpan gutter read back from the
// screen and the bounce comparison with Composition (gap 3).
static class ScrollPhysicsTests
{
    public static IEnumerable<TestCase> All() =>
    [
        new("ScrollPhysics.Stats.CountsBlitAndRepaintFrames", CountsBlitAndRepaintFrames),
        new("ScrollPhysics.Overpan.EdgeFromScreenColumn", EdgeFromScreenColumn),
        new("ScrollPhysics.Overpan.BounceMatchesAfterLag", BounceMatchesAfterLag),
        new("ScrollPhysics.Overpan.MissingBounceDeviates", MissingBounceDeviates),
    ];

    static void CountsBlitAndRepaintFrames()
    {
        string[] lines =
        [
            "frame island elements visited painted culled commands blits blitdx blitdy damagerects damagepx full walkus paintus reused scrollrepaint",
            "1 0000 23 346 23 290 33 0 0 0 0 531284 1 1117 4221 0 0",
            "# hitproxy add 0000 at 0,0",
            "6 0000 25 347 25 289 35 1 0 -16 1 531284 0 931 4920 0 0",
            "7 0000 25 347 25 289 35 1 0 -14 1 531284 0 782 3720 0 0",
            "8 0000 25 346 24 289 34 0 0 0 1 531284 0 815 3091 1 0",
        ];
        Equal((2, 2), ScrollPhysicsAnalysis.CountBlitFrames(lines, 0, 4), "all frames");
        // Frame indices count frame lines only (the step's stats window).
        Equal((2, 0), ScrollPhysicsAnalysis.CountBlitFrames(lines, 1, 3), "frames 6 and 7");
        Equal((0, 0), ScrollPhysicsAnalysis.CountBlitFrames(lines, 3, 3), "empty window");
    }

    static byte[] Column(int gutterPixels, int height)
    {
        var bits = new byte[height * 4];
        for (int y = 0; y < height; y++)
        {
            byte v = y < gutterPixels ? (byte)255 : (byte)61;
            bits[y * 4] = v; bits[y * 4 + 1] = v; bits[y * 4 + 2] = v; bits[y * 4 + 3] = 255;
        }
        return bits;
    }

    static void EdgeFromScreenColumn()
    {
        Near(0, ScrollPhysicsAnalysis.OverpanEdge(Column(0, 40), 1.0, (60, 60, 60)), 1e-9, "not overpanned");
        Near(15, ScrollPhysicsAnalysis.OverpanEdge(Column(30, 40), 2.0, (60, 60, 60)), 1e-9, "30 px gutter at 200%");
        Expect(double.IsNaN(ScrollPhysicsAnalysis.OverpanEdge(Column(40, 40), 1.0, (60, 60, 60))), "row 0 not visible");
    }

    static List<(double, double)> Bounce(double lagMs, double peak, double scale = 1)
    {
        // Pull to the peak over 100 ms, hold, then return to 0 over 300 ms (sampled every 8 ms).
        var points = new List<(double, double)>();
        for (double ms = 0; ms <= 600; ms += 8)
        {
            double t = ms - lagMs;
            double v = t <= 0 ? 0 : t < 100 ? peak * t / 100 : t < 150 ? peak : t < 450 ? peak * (1 - (t - 150) / 300) : 0;
            points.Add((ms, v * scale));
        }
        return points;
    }

    static void BounceMatchesAfterLag()
    {
        var reference = Bounce(0, 60);
        var lagged = Bounce(32, 60);
        var result = NotNull(ScrollPhysicsAnalysis.CompareBounce(lagged, reference), "comparison");
        Expect(result.MaxDeviation <= 2, $"deviation {result.MaxDeviation:F2} after shift {result.ShiftMs}");
        Expect(result.ShiftMs is >= 24 and <= 40, $"shift {result.ShiftMs}");
        Near(60, result.ReferencePeak, 1e-9, "reference peak");
    }

    static void MissingBounceDeviates()
    {
        // Immediate mode clamped at the extent: no gutter at all.
        var clamped = Bounce(0, 0);
        var result = NotNull(ScrollPhysicsAnalysis.CompareBounce(clamped, Bounce(0, 60)), "comparison");
        Expect(result.MaxDeviation > 50, $"deviation {result.MaxDeviation:F2}");
        Equal(0.0, result.TestPeak, "no overpan");
        Expect(ScrollPhysicsAnalysis.CompareBounce([(0, double.NaN)], Bounce(0, 60)) is null, "no samples");
    }
}
