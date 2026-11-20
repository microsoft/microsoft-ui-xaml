using System.Buffers.Binary;
using System.Text;

namespace RenderBench;

// S19: finds an installed OpenType font with CFF outlines ('OTTO' sfnt version) and reads its family name.
public static class FontProbe
{
    public sealed record CffFont(string Path, string Family);

    public static bool IsCff(ReadOnlySpan<byte> font) => font.Length >= 4 && font[..4].SequenceEqual("OTTO"u8);

    // Typographic family (nameID 16) when present, else family (nameID 1); Windows platform, Unicode BMP (UTF-16BE).
    public static string? TryReadFamilyName(ReadOnlySpan<byte> font)
    {
        try
        {
            if (font.Length < 12)
            {
                return null;
            }
            int tables = BinaryPrimitives.ReadUInt16BigEndian(font[4..]);
            for (int i = 0; i < tables; i++)
            {
                var record = font.Slice(12 + i * 16, 16);
                if (!record[..4].SequenceEqual("name"u8))
                {
                    continue;
                }
                int offset = (int)BinaryPrimitives.ReadUInt32BigEndian(record[8..]);
                var name = font[offset..];
                int count = BinaryPrimitives.ReadUInt16BigEndian(name[2..]);
                int strings = BinaryPrimitives.ReadUInt16BigEndian(name[4..]);
                string? family = null, typographic = null;
                for (int n = 0; n < count; n++)
                {
                    var entry = name.Slice(6 + n * 12, 12);
                    int platform = BinaryPrimitives.ReadUInt16BigEndian(entry);
                    int encoding = BinaryPrimitives.ReadUInt16BigEndian(entry[2..]);
                    int nameId = BinaryPrimitives.ReadUInt16BigEndian(entry[6..]);
                    int length = BinaryPrimitives.ReadUInt16BigEndian(entry[8..]);
                    int start = BinaryPrimitives.ReadUInt16BigEndian(entry[10..]);
                    if (platform != 3 || encoding != 1 || (nameId != 1 && nameId != 16))
                    {
                        continue;
                    }
                    string text = Encoding.BigEndianUnicode.GetString(name.Slice(strings + start, length));
                    if (nameId == 16)
                    {
                        typographic ??= text;
                    }
                    else
                    {
                        family ??= text;
                    }
                }
                return typographic ?? family;
            }
            return null;
        }
        catch (ArgumentOutOfRangeException)
        {
            return null;
        }
    }

    public static CffFont? FindCffFont(IEnumerable<string> paths)
    {
        foreach (string path in paths)
        {
            try
            {
                byte[] head = new byte[4];
                using (var stream = File.OpenRead(path))
                {
                    if (stream.Read(head, 0, 4) != 4 || !IsCff(head))
                    {
                        continue;
                    }
                }
                if (TryReadFamilyName(File.ReadAllBytes(path)) is { Length: > 0 } family)
                {
                    return new CffFont(path, family);
                }
            }
            catch (IOException)
            {
            }
            catch (UnauthorizedAccessException)
            {
            }
        }
        return null;
    }

    public static IEnumerable<string> SystemFontFiles()
    {
        string fonts = Environment.GetFolderPath(Environment.SpecialFolder.Fonts);
        string user = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Microsoft", "Windows", "Fonts");
        return new[] { fonts, user }
            .Where(Directory.Exists)
            .SelectMany(d => Directory.EnumerateFiles(d, "*.otf"))
            .Order(StringComparer.OrdinalIgnoreCase);
    }
}
