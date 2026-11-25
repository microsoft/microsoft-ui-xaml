# Copyright (c) Microsoft Corporation and Contributors.
# Licensed under the MIT License.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Path,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$')]
    [string]$Version,

    [ValidatePattern('^[0-9a-fA-F]{40}$')]
    [string]$SourceRevision
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

if (-not ('WinUIBuild.LocalizedVersionResource' -as [type]))
{
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace WinUIBuild
{
    public static class LocalizedVersionResource
    {
        private delegate bool LanguageCallback(IntPtr module, IntPtr type, IntPtr name, ushort language, IntPtr parameter);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr LoadLibraryEx(string path, IntPtr file, uint flags);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool FreeLibrary(IntPtr module);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool EnumResourceLanguages(IntPtr module, IntPtr type, IntPtr name, LanguageCallback callback, IntPtr parameter);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr FindResourceEx(IntPtr module, IntPtr type, IntPtr name, ushort language);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint SizeofResource(IntPtr module, IntPtr resource);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr LoadResource(IntPtr module, IntPtr resource);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr LockResource(IntPtr resource);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr BeginUpdateResource(string path, bool deleteExisting);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool UpdateResource(IntPtr update, IntPtr type, IntPtr name, ushort language, byte[] data, uint size);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool EndUpdateResource(IntPtr update, bool discard);

        private static int Align(int value)
        {
            return (value + 3) & ~3;
        }

        private static void Pad(BinaryWriter writer)
        {
            while ((writer.BaseStream.Position & 3) != 0)
            {
                writer.Write((byte)0);
            }
        }

        // Rebuild length-prefixed, DWORD-aligned blocks so longer version strings do not
        // overwrite localized metadata, translation tables, or another string's padding.
        private static byte[] RewriteBlock(byte[] data, int start, string version, uint high, uint low, string sourceRevision)
        {
            if (start + 6 > data.Length)
            {
                throw new InvalidDataException("Truncated VERSIONINFO block.");
            }
            int end = start + BitConverter.ToUInt16(data, start);
            if (end < start + 6 || end > data.Length)
            {
                throw new InvalidDataException("Invalid VERSIONINFO block.");
            }
            ushort valueLength = BitConverter.ToUInt16(data, start + 2);
            ushort type = BitConverter.ToUInt16(data, start + 4);
            int cursor = start + 6;
            int keyStart = cursor;
            while (cursor + 2 <= end && BitConverter.ToUInt16(data, cursor) != 0)
            {
                cursor += 2;
            }
            if (cursor + 2 > end)
            {
                throw new InvalidDataException("Invalid VERSIONINFO block.");
            }
            string key = Encoding.Unicode.GetString(data, keyStart, cursor - keyStart);
            cursor = Align(cursor + 2);
            int valueSize = type == 1 ? valueLength * 2 : valueLength;
            if (cursor + valueSize > end)
            {
                throw new InvalidDataException("Invalid VERSIONINFO value.");
            }
            byte[] value = new byte[valueSize];
            Array.Copy(data, cursor, value, 0, value.Length);
            cursor += valueSize;
            if (key == "VS_VERSION_INFO")
            {
                if (value.Length < 52 || BitConverter.ToUInt32(value, 0) != 0xfeef04bd)
                {
                    throw new InvalidDataException("Missing fixed VERSIONINFO.");
                }
                Array.Copy(BitConverter.GetBytes(high), 0, value, 8, 4);
                Array.Copy(BitConverter.GetBytes(low), 0, value, 12, 4);
                Array.Copy(BitConverter.GetBytes(high), 0, value, 16, 4);
                Array.Copy(BitConverter.GetBytes(low), 0, value, 20, 4);
            }
            else if (key == "FileVersion" || key == "ProductVersion")
            {
                value = Encoding.Unicode.GetBytes(version + "\0");
                valueLength = checked((ushort)(value.Length / 2));
            }
            else if (key == "Comments" && !String.IsNullOrEmpty(sourceRevision))
            {
                value = Encoding.Unicode.GetBytes("SourceRevision=" + sourceRevision + "\0");
                valueLength = checked((ushort)(value.Length / 2));
            }

            using (MemoryStream stream = new MemoryStream())
            using (BinaryWriter writer = new BinaryWriter(stream))
            {
                writer.Write((ushort)0);
                writer.Write(valueLength);
                writer.Write(type);
                writer.Write(Encoding.Unicode.GetBytes(key + "\0"));
                Pad(writer);
                writer.Write(value);
                cursor = Align(cursor);
                bool hasComments = false;
                const string commentsKey = "Comments\0";
                while (cursor + 6 <= end)
                {
                    int childLength = BitConverter.ToUInt16(data, cursor);
                    if (childLength < 6 || cursor + childLength > end)
                    {
                        throw new InvalidDataException("Invalid VERSIONINFO child.");
                    }
                    hasComments |= childLength >= 6 + commentsKey.Length * 2 &&
                        Encoding.Unicode.GetString(data, cursor + 6, commentsKey.Length * 2) == commentsKey;
                    Pad(writer);
                    writer.Write(RewriteBlock(data, cursor, version, high, low, sourceRevision));
                    cursor = Align(cursor + childLength);
                }
                uint stringTable;
                if (!String.IsNullOrEmpty(sourceRevision) && !hasComments && key.Length == 8 &&
                    UInt32.TryParse(key, System.Globalization.NumberStyles.HexNumber,
                        System.Globalization.CultureInfo.InvariantCulture, out stringTable))
                {
                    Pad(writer);
                    byte[] commentKey = Encoding.Unicode.GetBytes(commentsKey);
                    byte[] commentValue = Encoding.Unicode.GetBytes("SourceRevision=" + sourceRevision + "\0");
                    writer.Write(checked((ushort)(Align(6 + commentKey.Length) + commentValue.Length)));
                    writer.Write(checked((ushort)(commentValue.Length / 2)));
                    writer.Write((ushort)1);
                    writer.Write(commentKey);
                    Pad(writer);
                    writer.Write(commentValue);
                }
                ushort length = checked((ushort)stream.Length);
                stream.Position = 0;
                writer.Write(length);
                return stream.ToArray();
            }
        }

        public static void Set(string path, string version)
        {
            Set(path, version, null);
        }

        public static void Set(string path, string version, string sourceRevision)
        {
            string[] parts = version.Split('.');
            ushort[] numbers = Array.ConvertAll(parts, part => ushort.Parse(part, System.Globalization.CultureInfo.InvariantCulture));
            uint high = ((uint)numbers[0] << 16) | numbers[1];
            uint low = ((uint)numbers[2] << 16) | numbers[3];
            IntPtr type = new IntPtr(16);
            IntPtr name = new IntPtr(1);
            Dictionary<ushort, byte[]> resources = new Dictionary<ushort, byte[]>();
            IntPtr module = LoadLibraryEx(path, IntPtr.Zero, 0x22);
            if (module == IntPtr.Zero)
            {
                throw new Win32Exception();
            }
            try
            {
                List<ushort> languages = new List<ushort>();
                if (!EnumResourceLanguages(module, type, name, (m, t, n, language, p) => { languages.Add(language); return true; }, IntPtr.Zero))
                {
                    throw new Win32Exception();
                }
                foreach (ushort language in languages)
                {
                    IntPtr resource = FindResourceEx(module, type, name, language);
                    IntPtr loaded = resource == IntPtr.Zero ? IntPtr.Zero : LoadResource(module, resource);
                    IntPtr address = loaded == IntPtr.Zero ? IntPtr.Zero : LockResource(loaded);
                    if (address == IntPtr.Zero)
                    {
                        throw new Win32Exception();
                    }
                    byte[] data = new byte[checked((int)SizeofResource(module, resource))];
                    Marshal.Copy(address, data, 0, data.Length);
                    resources.Add(language, RewriteBlock(data, 0, version, high, low, sourceRevision));
                }
            }
            finally
            {
                FreeLibrary(module);
            }

            IntPtr update = BeginUpdateResource(path, false);
            if (update == IntPtr.Zero)
            {
                throw new Win32Exception();
            }
            try
            {
                foreach (KeyValuePair<ushort, byte[]> resource in resources)
                {
                    if (!UpdateResource(update, type, name, resource.Key, resource.Value, (uint)resource.Value.Length))
                    {
                        throw new Win32Exception();
                    }
                }
                if (!EndUpdateResource(update, false))
                {
                    throw new Win32Exception();
                }
                update = IntPtr.Zero;
            }
            finally
            {
                if (update != IntPtr.Zero)
                {
                    EndUpdateResource(update, true);
                }
            }
        }
    }
}
'@
}

$resolvedPath = (Resolve-Path -LiteralPath $Path).ProviderPath
[WinUIBuild.LocalizedVersionResource]::Set($resolvedPath, $Version, $SourceRevision)
$info = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($resolvedPath)
$versions = @(
    (@($info.FileMajorPart, $info.FileMinorPart, $info.FileBuildPart, $info.FilePrivatePart) -join '.'),
    $info.FileVersion,
    (@($info.ProductMajorPart, $info.ProductMinorPart, $info.ProductBuildPart, $info.ProductPrivatePart) -join '.'),
    $info.ProductVersion
)
if (@($versions | Where-Object { $_ -cne $Version }).Count -gt 0)
{
    throw "Failed to stamp all native version fields in '$resolvedPath' with '$Version': $($versions -join '; ')"
}
if ($SourceRevision -and $info.Comments -cne "SourceRevision=$SourceRevision")
{
    throw "Failed to stamp the source revision in '$resolvedPath'."
}

return 0
