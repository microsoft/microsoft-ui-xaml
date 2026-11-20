using System.Buffers.Binary;

namespace RenderBench;

// 32bpp BGRA pixels, top-down, Width*4 bytes per row.
public sealed record BgraImage(int Width, int Height, byte[] Pixels)
{
    public BgraImage Crop(int x, int y, int width, int height)
    {
        x = Math.Clamp(x, 0, Width);
        y = Math.Clamp(y, 0, Height);
        width = Math.Clamp(width, 0, Width - x);
        height = Math.Clamp(height, 0, Height - y);
        var pixels = new byte[width * height * 4];
        for (int row = 0; row < height; row++)
        {
            Array.Copy(Pixels, ((y + row) * Width + x) * 4, pixels, row * width * 4, width * 4);
        }
        return new BgraImage(width, height, pixels);
    }
}

// Reads/writes the uncompressed 32bpp BMPs that Win32.WriteBmp and the native WINUI_IMMEDIATE_CAPTURE hook produce.
public static class BmpCodec
{
    const int FileHeaderSize = 14, InfoHeaderSize = 40;

    public static byte[] Encode(int width, int height, byte[] topDownBgra)
    {
        var bytes = new byte[FileHeaderSize + InfoHeaderSize + topDownBgra.Length];
        var span = bytes.AsSpan();
        span[0] = (byte)'B';
        span[1] = (byte)'M';
        BinaryPrimitives.WriteInt32LittleEndian(span[2..], bytes.Length);
        BinaryPrimitives.WriteInt32LittleEndian(span[10..], FileHeaderSize + InfoHeaderSize);
        BinaryPrimitives.WriteInt32LittleEndian(span[14..], InfoHeaderSize);
        BinaryPrimitives.WriteInt32LittleEndian(span[18..], width);
        BinaryPrimitives.WriteInt32LittleEndian(span[22..], -height);
        BinaryPrimitives.WriteInt16LittleEndian(span[26..], 1);
        BinaryPrimitives.WriteInt16LittleEndian(span[28..], 32);
        topDownBgra.CopyTo(span[(FileHeaderSize + InfoHeaderSize)..]);
        return bytes;
    }

    // 32bpp BI_RGB or BI_BITFIELDS (assumed BGRA masks), top-down or bottom-up; null for anything else or truncated data.
    public static BgraImage? TryDecode(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length < FileHeaderSize + InfoHeaderSize || bytes[0] != 'B' || bytes[1] != 'M')
        {
            return null;
        }
        int dataOffset = BinaryPrimitives.ReadInt32LittleEndian(bytes[10..]);
        int width = BinaryPrimitives.ReadInt32LittleEndian(bytes[18..]);
        int rawHeight = BinaryPrimitives.ReadInt32LittleEndian(bytes[22..]);
        int bits = BinaryPrimitives.ReadInt16LittleEndian(bytes[28..]);
        int compression = BinaryPrimitives.ReadInt32LittleEndian(bytes[30..]);
        int height = Math.Abs(rawHeight);
        if (bits != 32 || (compression != 0 && compression != 3) || width <= 0 || height <= 0 || dataOffset < 0)
        {
            return null;
        }
        long size = (long)width * height * 4;
        if (size > int.MaxValue || dataOffset + size > bytes.Length)
        {
            return null;
        }
        var pixels = new byte[size];
        int stride = width * 4;
        for (int row = 0; row < height; row++)
        {
            int source = rawHeight < 0 ? row : height - 1 - row;
            bytes.Slice(dataOffset + source * stride, stride).CopyTo(pixels.AsSpan(row * stride));
        }
        return new BgraImage(width, height, pixels);
    }
}

public static class PixelDiff
{
    // Mean absolute difference per color channel (B, G, R; alpha ignored) over the overlapping top-left region, 0..255.
    public static double MeanAbsDiff(BgraImage a, BgraImage b)
    {
        int width = Math.Min(a.Width, b.Width), height = Math.Min(a.Height, b.Height);
        if (width == 0 || height == 0)
        {
            return double.NaN;
        }
        long sum = 0;
        for (int y = 0; y < height; y++)
        {
            int rowA = y * a.Width * 4, rowB = y * b.Width * 4;
            for (int x = 0; x < width * 4; x += 4)
            {
                for (int c = 0; c < 3; c++)
                {
                    sum += Math.Abs(a.Pixels[rowA + x + c] - b.Pixels[rowB + x + c]);
                }
            }
        }
        return sum / (3.0 * width * height);
    }
}
