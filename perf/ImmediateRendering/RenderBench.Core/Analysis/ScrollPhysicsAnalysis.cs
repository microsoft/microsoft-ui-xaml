namespace RenderBench.Analysis;

// ---- scrollgaps ---- S33 scroll physics helpers shared by the scene (RenderBench) and the kit tests: blit/repaint frame
// counts from the native stats file (gap 5), the overpan gutter read back from the screen and the bounce comparison
// against Composition (gap 3).
public static class ScrollPhysicsAnalysis
{
    // WINUI_IMMEDIATE_STATS frame lines: frame island elements visited painted culled commands blits ...
    const int BlitsColumn = 7;

    // Frame lines [from, to) (header and '#' lines are not frames): frames with at least one scroll blit, and frames
    // that presented without one.
    public static (int Blit, int Repaint) CountBlitFrames(IEnumerable<string> statsLines, int from, int to)
    {
        int blit = 0, repaint = 0;
        foreach (var line in statsLines.Where(l => l.Length > 0 && char.IsDigit(l[0])).Skip(from).Take(Math.Max(0, to - from)))
        {
            var columns = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
            if (columns.Length > BlitsColumn && int.TryParse(columns[BlitsColumn], out int blits))
            {
                if (blits > 0) blit++; else repaint++;
            }
        }
        return (blit, repaint);
    }

    // A one-pixel screen column (BGRA, top to bottom) from the viewport's top: DIPs above the first pixel of the
    // reference color (the content's first row), so 0 when not overpanned; NaN when the color is not in the column.
    public static double OverpanEdge(ReadOnlySpan<byte> bgra, double scale, (byte R, byte G, byte B) reference, int tolerance = 6)
    {
        for (int y = 0; y * 4 + 2 < bgra.Length; y++)
        {
            if (Math.Abs(bgra[y * 4 + 2] - reference.R) <= tolerance && Math.Abs(bgra[y * 4 + 1] - reference.G) <= tolerance
                && Math.Abs(bgra[y * 4] - reference.B) <= tolerance)
            {
                return y / scale;
            }
        }
        return double.NaN;
    }

    public sealed record BounceComparison(double MaxDeviation, double ShiftMs, int Samples, double TestPeak, double ReferencePeak);

    // Compares two overpan gutter trajectories (ms since input, gutter DIPs; NaN samples dropped). The test trajectory is
    // moved earlier by the 4 ms step in [0, maxShiftMs] that minimises the largest deviation (residual start lag), and
    // both are linearly interpolated every 16 ms over the reference's span. Null when either has fewer than 2 samples.
    public static BounceComparison? CompareBounce(IReadOnlyList<(double Ms, double Edge)> test, IReadOnlyList<(double Ms, double Edge)> reference,
        double maxShiftMs = 64)
    {
        var t = test.Where(p => !double.IsNaN(p.Edge)).OrderBy(p => p.Ms).ToList();
        var r = reference.Where(p => !double.IsNaN(p.Edge)).OrderBy(p => p.Ms).ToList();
        if (t.Count < 2 || r.Count < 2)
        {
            return null;
        }
        BounceComparison? best = null;
        for (double shift = 0; shift <= maxShiftMs + 1e-9; shift += 4)
        {
            double worst = 0;
            int samples = 0;
            for (double ms = r[0].Ms; ms <= r[^1].Ms + 1e-9; ms += 16)
            {
                worst = Math.Max(worst, Math.Abs(Sample(t, ms + shift) - Sample(r, ms)));
                samples++;
            }
            if (best is null || worst < best.MaxDeviation - 1e-9)
            {
                best = new BounceComparison(worst, shift, samples, t.Max(p => p.Edge), r.Max(p => p.Edge));
            }
        }
        return best;
    }

    static double Sample(List<(double Ms, double Edge)> points, double ms)
    {
        if (ms <= points[0].Ms) return points[0].Edge;
        for (int i = 1; i < points.Count; i++)
        {
            if (points[i].Ms >= ms)
            {
                var (t0, v0) = points[i - 1];
                var (t1, v1) = points[i];
                return t1 <= t0 ? v1 : v0 + (v1 - v0) * (ms - t0) / (t1 - t0);
            }
        }
        return points[^1].Edge;
    }
}
