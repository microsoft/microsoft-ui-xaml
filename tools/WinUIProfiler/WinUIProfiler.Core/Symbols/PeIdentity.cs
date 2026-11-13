// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Globalization;
using System.Reflection.PortableExecutable;

namespace WinUIProfiler.Engine.Symbols;

// The PDB a binary was built with, from its CodeView (RSDS) debug record. Symbol servers index a PDB by file name,
// GUID and age. Portable PDBs (managed code) carry no native function names.
internal sealed record PdbIdentity(string PdbName, Guid Guid, int Age, bool Portable = false)
{
    // The file name, GUID and age, as a symbol server keys the PDB.
    public string Key => $"{Path.GetFileName(PdbName)} {Guid.ToString("N", CultureInfo.InvariantCulture).ToUpperInvariant()}{Age:X}";
}

// Reads a binary's PdbIdentity from disk. ETW records it for each loaded image (ImageID/DbgID_RSDS); this covers
// images whose record was lost or that a trace was captured without.
internal static class PeIdentity
{
    private const string SystemRootPrefix = @"\SystemRoot\";

    // Returns null when the file is missing, is not a PE image or has no CodeView record. Non-zero expected values
    // must match the image that was loaded, so a binary rebuilt or updated since the trace is not mistaken for it.
    internal static PdbIdentity? Read(string path, int expectedTimeDateStamp = 0, int expectedSizeOfImage = 0)
    {
        var file = LocalPath(path);
        if (file is null || !File.Exists(file))
        {
            return null;
        }

        try
        {
            using var stream = new FileStream(file, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
            using var reader = new PEReader(stream);
            var headers = reader.PEHeaders;
            if (headers.PEHeader is not { } peHeader ||
                (expectedTimeDateStamp != 0 && headers.CoffHeader.TimeDateStamp != expectedTimeDateStamp) ||
                (expectedSizeOfImage != 0 && peHeader.SizeOfImage != expectedSizeOfImage))
            {
                return null;
            }

            foreach (var entry in reader.ReadDebugDirectory())
            {
                if (entry.Type == DebugDirectoryEntryType.CodeView)
                {
                    var codeView = reader.ReadCodeViewDebugDirectoryData(entry);
                    return new PdbIdentity(codeView.Path, codeView.Guid, codeView.Age, entry.IsPortableCodeView);
                }
            }

            return null;
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            Trace.TraceWarning($"Could not read the PDB identity of {file}: {error.Message}");
            return null;
        }
    }

    // Maps the kernel's spellings of a path to one the file system opens: \SystemRoot\..., \??\C:\..., \\?\C:\... and
    // \Windows\... (on the system drive). Gives null for \Device\... paths, which have no drive letter.
    internal static string? LocalPath(string path)
    {
        if (string.IsNullOrEmpty(path))
        {
            return null;
        }

        if (path.StartsWith(SystemRootPrefix, StringComparison.OrdinalIgnoreCase))
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Windows), path[SystemRootPrefix.Length..]);
        }

        if (path.StartsWith(@"\??\", StringComparison.Ordinal) || path.StartsWith(@"\\?\", StringComparison.Ordinal))
        {
            path = path[4..];
            if (path.StartsWith(@"UNC\", StringComparison.OrdinalIgnoreCase))
            {
                return @"\\" + path[4..];
            }
        }

        if (path.StartsWith(@"\Device\", StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        if (path.StartsWith('\\') && !path.StartsWith(@"\\", StringComparison.Ordinal))
        {
            return Path.GetPathRoot(Environment.SystemDirectory) + path[1..];
        }

        return path;
    }
}
