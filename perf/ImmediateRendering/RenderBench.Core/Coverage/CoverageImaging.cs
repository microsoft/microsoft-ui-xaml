using System.Buffers.Binary;
using System.IO.Compression;

namespace RenderBench.Coverage;

// Fidelity metric and artifacts for the XAML coverage harness (test plan, "XAML coverage: harness and results").
public static class CoverageImaging
{
    // Mean absolute per-channel difference (B, G, R; alpha ignored) normalized to 0..1. Images of different sizes are
    // compared over the union: pixels outside the overlap count as fully different, so a capture of the wrong size can't
    // pass by matching only its top-left corner. Two empty images are identical (0).
    public static double MeanAbsDiff(BgraImage a, BgraImage b)
    {
        int unionWidth = Math.Max(a.Width, b.Width), unionHeight = Math.Max(a.Height, b.Height);
        if (unionWidth == 0 || unionHeight == 0)
        {
            return 0;
        }
        int width = Math.Min(a.Width, b.Width), height = Math.Min(a.Height, b.Height);
        long sum = 0;
        for (int y = 0; y < height; y++)
        {
            int rowA = y * a.Width * 4, rowB = y * b.Width * 4;
            for (int x = 0; x < width * 4; x += 4)
            {
                sum += Math.Abs(a.Pixels[rowA + x] - b.Pixels[rowB + x]) +
                       Math.Abs(a.Pixels[rowA + x + 1] - b.Pixels[rowB + x + 1]) +
                       Math.Abs(a.Pixels[rowA + x + 2] - b.Pixels[rowB + x + 2]);
            }
        }
        long outside = (long)unionWidth * unionHeight - (long)width * height;
        sum += outside * 3 * 255;
        return sum / (3.0 * 255.0 * unionWidth * unionHeight);
    }

    // Fraction of pixels whose largest channel difference exceeds `threshold` (0..255): separates "a few pixels
    // completely wrong" from "everything slightly off". Pixels outside the overlap count as differing.
    public static double DifferingPixelFraction(BgraImage a, BgraImage b, int threshold = 16)
    {
        long total = (long)Math.Max(a.Width, b.Width) * Math.Max(a.Height, b.Height);
        if (total == 0)
        {
            return 0;
        }
        int width = Math.Min(a.Width, b.Width), height = Math.Min(a.Height, b.Height);
        long differing = 0;
        for (int y = 0; y < height; y++)
        {
            int rowA = y * a.Width * 4, rowB = y * b.Width * 4;
            for (int x = 0; x < width * 4; x += 4)
            {
                if (MaxChannelDiff(a.Pixels, rowA + x, b.Pixels, rowB + x) > threshold)
                {
                    differing++;
                }
            }
        }
        return (differing + total - (long)width * height) / (double)total;
    }

    // Diff image over the union: the reference dimmed to dark grey where equal (within 2), the difference amplified 4x
    // in red where it differs, magenta outside the overlap.
    public static BgraImage DiffImage(BgraImage reference, BgraImage actual)
    {
        int unionWidth = Math.Max(reference.Width, actual.Width), unionHeight = Math.Max(reference.Height, actual.Height);
        var pixels = new byte[unionWidth * unionHeight * 4];
        for (int y = 0; y < unionHeight; y++)
        {
            for (int x = 0; x < unionWidth; x++)
            {
                int o = (y * unionWidth + x) * 4;
                pixels[o + 3] = 255;
                if (x >= reference.Width || y >= reference.Height || x >= actual.Width || y >= actual.Height)
                {
                    pixels[o] = 255;
                    pixels[o + 2] = 255;
                    continue;
                }
                int r = (y * reference.Width + x) * 4, a = (y * actual.Width + x) * 4;
                int d = MaxChannelDiff(reference.Pixels, r, actual.Pixels, a);
                if (d <= 2)
                {
                    byte grey = (byte)((reference.Pixels[r] + reference.Pixels[r + 1] + reference.Pixels[r + 2]) / 12);
                    pixels[o] = pixels[o + 1] = pixels[o + 2] = grey;
                }
                else
                {
                    byte v = (byte)Math.Min(255, 64 + d * 4);
                    pixels[o + 2] = v;
                    pixels[o + 1] = pixels[o] = (byte)(v / 4);
                }
            }
        }
        return new BgraImage(unionWidth, unionHeight, pixels);
    }

    // Box-filtered downscale so the longer side is at most `maxSide` (thumbnails for the HTML matrix).
    public static BgraImage Thumbnail(BgraImage image, int maxSide)
    {
        int longest = Math.Max(image.Width, image.Height);
        if (longest <= maxSide || longest == 0)
        {
            return image;
        }
        double scale = (double)longest / maxSide;
        int width = Math.Max(1, (int)(image.Width / scale)), height = Math.Max(1, (int)(image.Height / scale));
        var pixels = new byte[width * height * 4];
        for (int y = 0; y < height; y++)
        {
            int y0 = (int)(y * scale), y1 = Math.Min(image.Height, Math.Max(y0 + 1, (int)((y + 1) * scale)));
            for (int x = 0; x < width; x++)
            {
                int x0 = (int)(x * scale), x1 = Math.Min(image.Width, Math.Max(x0 + 1, (int)((x + 1) * scale)));
                long b = 0, g = 0, r = 0;
                int n = 0;
                for (int sy = y0; sy < y1; sy++)
                {
                    for (int sx = x0; sx < x1; sx++)
                    {
                        int s = (sy * image.Width + sx) * 4;
                        b += image.Pixels[s];
                        g += image.Pixels[s + 1];
                        r += image.Pixels[s + 2];
                        n++;
                    }
                }
                int o = (y * width + x) * 4;
                pixels[o] = (byte)(b / n);
                pixels[o + 1] = (byte)(g / n);
                pixels[o + 2] = (byte)(r / n);
                pixels[o + 3] = 255;
            }
        }
        return new BgraImage(width, height, pixels);
    }

    // True when every pixel has the same color (a capture that saw nothing).
    public static bool IsUniform(BgraImage image)
    {
        var p = image.Pixels;
        for (int i = 4; i + 2 < p.Length; i += 4)
        {
            if (p[i] != p[0] || p[i + 1] != p[1] || p[i + 2] != p[2])
            {
                return false;
            }
        }
        return true;
    }

    static int MaxChannelDiff(byte[] a, int ia, byte[] b, int ib)
        => Math.Max(Math.Abs(a[ia] - b[ib]), Math.Max(Math.Abs(a[ia + 1] - b[ib + 1]), Math.Abs(a[ia + 2] - b[ib + 2])));
}

// Minimal PNG writer (8-bit RGB, zlib via System.IO.Compression) so reports don't ship multi-MB BMPs.
public static class PngCodec
{
    static readonly uint[] s_crcTable = BuildCrcTable();

    public static byte[] Encode(BgraImage image)
    {
        using var raw = new MemoryStream();
        using (var z = new ZLibStream(raw, CompressionLevel.Optimal, leaveOpen: true))
        {
            var row = new byte[1 + image.Width * 3];
            for (int y = 0; y < image.Height; y++)
            {
                row[0] = 0; // filter: none
                int s = y * image.Width * 4;
                for (int x = 0; x < image.Width; x++, s += 4)
                {
                    row[1 + x * 3] = image.Pixels[s + 2];
                    row[2 + x * 3] = image.Pixels[s + 1];
                    row[3 + x * 3] = image.Pixels[s];
                }
                z.Write(row);
            }
        }
        using var png = new MemoryStream();
        png.Write([0x89, (byte)'P', (byte)'N', (byte)'G', 0x0D, 0x0A, 0x1A, 0x0A]);
        var header = new byte[13];
        BinaryPrimitives.WriteInt32BigEndian(header, image.Width);
        BinaryPrimitives.WriteInt32BigEndian(header.AsSpan(4), image.Height);
        header[8] = 8; // bit depth
        header[9] = 2; // color type: truecolor
        WriteChunk(png, "IHDR", header);
        WriteChunk(png, "IDAT", raw.ToArray());
        WriteChunk(png, "IEND", []);
        return png.ToArray();
    }

    public static void Save(string path, BgraImage image)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        File.WriteAllBytes(path, Encode(image));
    }

    static void WriteChunk(Stream stream, string type, byte[] data)
    {
        Span<byte> length = stackalloc byte[4];
        BinaryPrimitives.WriteInt32BigEndian(length, data.Length);
        stream.Write(length);
        var typeBytes = System.Text.Encoding.ASCII.GetBytes(type);
        stream.Write(typeBytes);
        stream.Write(data);
        uint crc = Crc(Crc(0xFFFFFFFFu, typeBytes), data) ^ 0xFFFFFFFFu;
        Span<byte> crcBytes = stackalloc byte[4];
        BinaryPrimitives.WriteUInt32BigEndian(crcBytes, crc);
        stream.Write(crcBytes);
    }

    static uint Crc(uint crc, ReadOnlySpan<byte> data)
    {
        foreach (byte b in data)
        {
            crc = s_crcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
        }
        return crc;
    }

    static uint[] BuildCrcTable()
    {
        var table = new uint[256];
        for (uint n = 0; n < 256; n++)
        {
            uint c = n;
            for (int k = 0; k < 8; k++)
            {
                c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            table[n] = c;
        }
        return table;
    }
}
