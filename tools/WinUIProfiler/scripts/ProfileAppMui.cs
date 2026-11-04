using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

public sealed class ProfileAppMuiInfo
{
    public int FileType { get; set; }
    public string Checksum { get; set; }
    public string ServiceChecksum { get; set; }
    public string Language { get; set; }
    public string MainTypes { get; set; }
    public string MuiTypes { get; set; }
}

public static class ProfileAppMui
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool GetFileMUIInfo(uint flags, string path, IntPtr info, ref uint size);

    public static ProfileAppMuiInfo Read(string path)
    {
        uint size = 4096;
        for (int attempt = 0; attempt != 9; ++attempt)
        {
            IntPtr buffer = Marshal.AllocHGlobal(checked((int)size));
            try
            {
                Marshal.Copy(new byte[size], 0, buffer, checked((int)size));
                Marshal.WriteInt32(buffer, checked((int)size));
                Marshal.WriteInt32(buffer, 4, 1);
                uint required = size;
                if (!GetFileMUIInfo(15, path, buffer, ref required))
                {
                    int error = Marshal.GetLastWin32Error();
                    if (error == 122 && size < 1048576 && required <= 1048576)
                    {
                        size = Math.Max(required, size * 2);
                        continue;
                    }
                    throw new Win32Exception(error, "Cannot read MUI metadata: " + path);
                }
                var bytes = new byte[size];
                Marshal.Copy(buffer, bytes, 0, bytes.Length);
                return new ProfileAppMuiInfo
                {
                    FileType = BitConverter.ToInt32(bytes, 8),
                    Checksum = Convert.ToHexString(bytes.AsSpan(12, 16)),
                    ServiceChecksum = Convert.ToHexString(bytes.AsSpan(28, 16)),
                    Language = ReadStrings(bytes, BitConverter.ToInt32(bytes, 44), false),
                    MainTypes = ReadTypes(bytes, 48),
                    MuiTypes = ReadTypes(bytes, 60)
                };
            }
            finally { Marshal.FreeHGlobal(buffer); }
        }
        throw new InvalidDataException("MUI metadata changed while reading: " + path);
    }

    private static string ReadStrings(byte[] bytes, int offset, bool multiple)
    {
        if (offset == 0) return "";
        var strings = new List<string>();
        do
        {
            int start = offset;
            while (true)
            {
                CheckRange(bytes, offset, 2);
                if (BitConverter.ToUInt16(bytes, offset) == 0) break;
                offset += 2;
            }
            if (offset == start) break;
            strings.Add(Encoding.Unicode.GetString(bytes, start, offset - start));
            offset += 2;
        } while (multiple);
        return string.Join("|", strings);
    }

    private static string ReadTypes(byte[] bytes, int field)
    {
        int count = BitConverter.ToInt32(bytes, field);
        int offset = BitConverter.ToInt32(bytes, field + 4);
        CheckRange(bytes, offset, checked(count * 4));
        var ids = new uint[count];
        for (int i = 0; i < ids.Length; ++i) ids[i] = BitConverter.ToUInt32(bytes, offset + i * 4);
        return string.Join(",", ids) + ":" + ReadStrings(bytes, BitConverter.ToInt32(bytes, field + 8), true);
    }

    private static void CheckRange(byte[] bytes, int offset, int count)
    {
        if (offset < 0 || count < 0 || offset > bytes.Length - count)
            throw new InvalidDataException("Invalid PE or MUI metadata bounds.");
    }

    private sealed class Image
    {
        public byte[] Bytes;
        public byte[] Header;
        public readonly Dictionary<string, byte[]> Sections = new Dictionary<string, byte[]>(StringComparer.Ordinal);
        public readonly Dictionary<string, byte[]> Resources = new Dictionary<string, byte[]>(StringComparer.Ordinal);
        public byte[] Overlay;
        private readonly List<(uint Rva, uint Size, int Raw)> mappings = new List<(uint, uint, int)>();
        private int resourceRoot;

        public Image(string path)
        {
            Bytes = File.ReadAllBytes(path);
            if (Read16(0) != 0x5a4d) throw new InvalidDataException("Invalid DOS header.");
            int pe = checked((int)Read32(0x3c));
            if (Read32(pe) != 0x4550) throw new InvalidDataException("Invalid PE signature.");
            int optional = checked(pe + 24);
            ushort magic = Read16(optional);
            if (magic != 0x20b && magic != 0x10b) throw new InvalidDataException("Invalid PE optional header.");
            int directory = checked(optional + (magic == 0x20b ? 112 : 96) + 16);
            uint resourceRva = Read32(directory);
            int headersSize = checked((int)Read32(optional + 60));
            CheckRange(Bytes, 0, headersSize);
            Header = Bytes.AsSpan(0, headersSize).ToArray();
            // MUIRCT can rebuild the resource container and update these size/checksum fields.
            Array.Clear(Header, optional + 8, 4);
            Array.Clear(Header, optional + 64, 4);
            Array.Clear(Header, directory + 4, 4);
            int table = checked(optional + Read16(pe + 20));
            int fileEnd = headersSize;
            for (int i = 0; i < Read16(pe + 6); ++i)
            {
                int entry = checked(table + i * 40);
                CheckRange(Bytes, entry, 40);
                string name = Encoding.ASCII.GetString(Bytes, entry, 8).TrimEnd('\0');
                uint virtualSize = Read32(entry + 8);
                uint rva = Read32(entry + 12);
                int rawSize = checked((int)Read32(entry + 16));
                int raw = checked((int)Read32(entry + 20));
                CheckRange(Bytes, raw, rawSize);
                mappings.Add((rva, Math.Max(virtualSize, (uint)rawSize), raw));
                if (name == ".rsrc")
                    Array.Clear(Header, entry + 8, 4);
                else
                    Sections.Add(name, SHA256.HashData(Bytes.AsSpan(raw, rawSize)));
                fileEnd = Math.Max(fileEnd, checked(raw + rawSize));
            }
            Overlay = Bytes.AsSpan(fileEnd).ToArray();
            if (resourceRva != 0)
            {
                resourceRoot = Map(resourceRva, 16);
                Walk(resourceRoot, "", 0);
            }
        }

        private ushort Read16(int offset)
        {
            CheckRange(Bytes, offset, 2);
            return BitConverter.ToUInt16(Bytes, offset);
        }

        private uint Read32(int offset)
        {
            CheckRange(Bytes, offset, 4);
            return BitConverter.ToUInt32(Bytes, offset);
        }

        private int Map(uint rva, int count)
        {
            foreach (var mapping in mappings)
            {
                if (rva >= mapping.Rva && (ulong)rva + (uint)count <= (ulong)mapping.Rva + mapping.Size)
                {
                    int result = checked(mapping.Raw + (int)(rva - mapping.Rva));
                    CheckRange(Bytes, result, count);
                    return result;
                }
            }
            throw new InvalidDataException("Resource RVA is not mapped to a PE section.");
        }

        private void Walk(int directory, string prefix, int depth)
        {
            if (depth > 2) throw new InvalidDataException("Unexpected resource directory depth.");
            int count = checked(Read16(directory + 12) + Read16(directory + 14));
            CheckRange(Bytes, directory + 16, checked(count * 8));
            for (int i = 0; i < count; ++i)
            {
                int entry = checked(directory + 16 + i * 8);
                uint name = Read32(entry);
                string key;
                if ((name & 0x80000000) != 0)
                {
                    int text = checked(resourceRoot + (int)(name & 0x7fffffff));
                    int size = checked(Read16(text) * 2);
                    CheckRange(Bytes, text + 2, size);
                    key = "s:" + Encoding.Unicode.GetString(Bytes, text + 2, size);
                }
                else { key = "i:" + name; }
                key = prefix + "/" + key;
                uint child = Read32(entry + 4);
                int offset = checked(resourceRoot + (int)(child & 0x7fffffff));
                if ((child & 0x80000000) != 0)
                {
                    Walk(offset, key, depth + 1);
                }
                else
                {
                    if (depth != 2) throw new InvalidDataException("Unexpected resource leaf depth.");
                    int size = checked((int)Read32(offset + 4));
                    int data = Map(Read32(offset), size);
                    key += "/cp:" + Read32(offset + 8);
                    Resources.Add(key, Bytes.AsSpan(data, size).ToArray());
                }
            }
        }
    }

    public static void VerifyChecksumOnlyChange(string source, string prepared, string oldChecksum, string newChecksum)
    {
        var before = new Image(source);
        var after = new Image(prepared);
        if (before.Bytes.Length != after.Bytes.Length ||
            !before.Header.AsSpan().SequenceEqual(after.Header) ||
            !before.Overlay.AsSpan().SequenceEqual(after.Overlay) ||
            before.Sections.Count != after.Sections.Count || before.Resources.Count != after.Resources.Count)
            throw new InvalidDataException("MUI preparation changed the PE layout or non-resource metadata.");
        foreach (var section in before.Sections)
        {
            if (!after.Sections.TryGetValue(section.Key, out byte[] value) || !section.Value.AsSpan().SequenceEqual(value))
                throw new InvalidDataException("MUI preparation changed PE section " + section.Key);
        }
        byte[] oldValue = Convert.FromHexString(oldChecksum);
        byte[] newValue = Convert.FromHexString(newChecksum);
        if (oldValue.Length != 16 || newValue.Length != 16)
            throw new InvalidDataException("Invalid MUI association checksum.");
        int changed = 0;
        foreach (var resource in before.Resources)
        {
            if (!after.Resources.TryGetValue(resource.Key, out byte[] value))
            {
                // MUIRCT rebuilds these binary resource entries with code page 1252.
                bool binaryResource = resource.Key.StartsWith("/s:MUI/", StringComparison.Ordinal) ||
                    resource.Key.StartsWith("/s:WEVT_TEMPLATE/", StringComparison.Ordinal) ||
                    resource.Key.StartsWith("/i:10/", StringComparison.Ordinal) ||
                    resource.Key.StartsWith("/i:16/", StringComparison.Ordinal);
                if (!binaryResource || !resource.Key.EndsWith("/cp:0", StringComparison.Ordinal) ||
                    !after.Resources.TryGetValue(resource.Key.Substring(0, resource.Key.Length - 1) + "1252", out value))
                    throw new InvalidDataException("MUI preparation changed resource identity " + resource.Key);
            }
            if (resource.Value.AsSpan().SequenceEqual(value)) continue;
            if (!resource.Key.StartsWith("/s:MUI/", StringComparison.Ordinal) || resource.Value.Length != value.Length)
                throw new InvalidDataException("MUI preparation changed resource " + resource.Key);
            var expected = (byte[])resource.Value.Clone();
            int found = -1;
            for (int i = 0; i <= expected.Length - oldValue.Length; ++i)
            {
                if (!expected.AsSpan(i, oldValue.Length).SequenceEqual(oldValue)) continue;
                if (found != -1) throw new InvalidDataException("Ambiguous MUI association checksum.");
                found = i;
            }
            if (found == -1) throw new InvalidDataException("Original MUI association checksum is missing.");
            newValue.CopyTo(expected, found);
            if (!expected.AsSpan().SequenceEqual(value))
                throw new InvalidDataException("MUI preparation changed more than the association checksum.");
            ++changed;
        }
        if (changed != 1) throw new InvalidDataException("Expected exactly one MUI association resource update.");
    }
}
