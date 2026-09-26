// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Normalizes volatile lines in copied XAML compiler masters for stable codegen comparisons while
// preserving each file's UTF-8 byte order mark.
// A refresh reconstructs both baselines from RegressionProjects\common plus their flavor-specific
// files, replaces only the selected flavor with normalized codegen, then stores byte-identical
// files in common and differences under chk or fre. The replacement is staged before installation.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace FixMasters
{
    class Program
    {
        static int Main(string[] args)
        {
            // Generated paths in a deep enlistment can exceed .NET Framework's legacy MAX_PATH limit.
            AppContext.SetSwitch("Switch.System.IO.UseLegacyPathHandling", false);
            AppContext.SetSwitch("Switch.System.IO.BlockLongPaths", false);
            try
            {
                if (args.Length != 4)
                {
                    Console.Error.WriteLine($"Usage: fixmasters.exe <masters-root> <{Flavor.Chk}|{Flavor.Fre}> <codegen-root> <targets-file>");
                    return 1;
                }

                string mastersRoot = ToExtendedLengthPath(args[0]);
                string buildFlavor = args[1].ToLowerInvariant();
                if (buildFlavor != Flavor.Chk && buildFlavor != Flavor.Fre)
                {
                    throw new ArgumentException($"Flavor must be {Flavor.Chk} or {Flavor.Fre}.");
                }
                if (!Directory.Exists(Path.Combine(mastersRoot, Flavor.Common)))
                {
                    throw new InvalidDataException("Common masters are missing from '" + mastersRoot +
                        "'. Restore them before refreshing either flavor.");
                }

                // Reconstruct both effective baselines (common plus flavor-specific files) before
                // replacing the rebuilt flavor, so the other flavor remains unchanged.
                var commonMasters = FileTree.ReadFromDirectory(Path.Combine(mastersRoot, Flavor.Common));
                var chkMasters = FileTree.ReadFromDirectory(Path.Combine(mastersRoot, Flavor.Chk), commonMasters);
                var freMasters = FileTree.ReadFromDirectory(Path.Combine(mastersRoot, Flavor.Fre), commonMasters);
                var generatedMasters = ReadGeneratedFileTree(args[2], args[3]);
                if (buildFlavor == Flavor.Chk)
                {
                    chkMasters = generatedMasters;
                }
                else
                {
                    freMasters = generatedMasters;
                }

                RepartitionAndReplaceMasters(mastersRoot, chkMasters, freMasters);
                return 0;
            }
            catch (InvalidDataException exception)
            {
                Console.Error.WriteLine("ERROR: " + exception.Message);
                return 1;
            }
        }

        // Use extended-length paths for both local drives and UNC shares.
        static string ToExtendedLengthPath(string path)
        {
            string fullPath = Path.GetFullPath(path);
            if (fullPath.StartsWith(@"\\?\", StringComparison.Ordinal))
            {
                return fullPath;
            }
            return fullPath.StartsWith(@"\\", StringComparison.Ordinal)
                ? @"\\?\UNC\" + fullPath.Substring(2)
                : @"\\?\" + fullPath;
        }

        // Validate and normalize every target before changing the baseline on disk.
        static FileTree ReadGeneratedFileTree(string codegenRoot, string targetsFile)
        {
            codegenRoot = ToExtendedLengthPath(codegenRoot);
            var generatedMasters = new FileTree();
            var targetDirectories = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var missingTargets = new List<string>();
            foreach (string rawLine in File.ReadAllLines(targetsFile))
            {
                string targetLine = rawLine.Trim();
                if (targetLine.Length == 0 || targetLine.StartsWith("#"))
                {
                    continue;
                }

                string[] mappedPaths = targetLine.Split('|');
                if (mappedPaths.Length != 2 || string.IsNullOrWhiteSpace(mappedPaths[0]) || string.IsNullOrWhiteSpace(mappedPaths[1]))
                {
                    throw new InvalidDataException("Malformed codegen target: " + rawLine);
                }

                string masterRelativeDirectory = NormalizeManifestPath(mappedPaths[0]);
                string codegenRelativeDirectory = NormalizeManifestPath(mappedPaths[1]);
                if (!targetDirectories.Add(masterRelativeDirectory))
                {
                    throw new InvalidDataException("Duplicate codegen target: " + masterRelativeDirectory);
                }

                string codegenDirectory = Path.Combine(codegenRoot, codegenRelativeDirectory);
                string[] generatedFiles = Directory.Exists(codegenDirectory)
                    ? Directory.GetFiles(codegenDirectory, "*.g.*", SearchOption.AllDirectories).Where(IsIncludedCodegenFile).ToArray()
                    : new string[0];
                if (generatedFiles.Length == 0)
                {
                    missingTargets.Add(masterRelativeDirectory + " (" + codegenDirectory + ")");
                    continue;
                }

                foreach (string generatedFile in generatedFiles)
                {
                    string masterRelativePath = Path.Combine(masterRelativeDirectory,
                        generatedFile.Substring(codegenDirectory.Length + 1));
                    generatedMasters.Add(masterRelativePath, ReadNormalizedGeneratedFile(generatedFile));
                }
            }

            if (targetDirectories.Count == 0)
            {
                throw new InvalidDataException("No targets in " + targetsFile);
            }
            if (missingTargets.Count > 0)
            {
                throw new InvalidDataException(missingTargets.Count + " target(s) have no codegen:" + Environment.NewLine +
                    string.Join(Environment.NewLine, missingTargets) +
                    Environment.NewLine + "Build every target and re-run copynewmasters.cmd; no masters were changed.");
            }
            Console.WriteLine("Read {0} generated files from {1} targets.", generatedMasters.Count, targetDirectories.Count);
            return generatedMasters;
        }

        // Manifest paths must stay inside their generated-files and masters roots.
        static string NormalizeManifestPath(string path)
        {
            string normalizedPath = path.Trim().Replace('/', '\\');
            if (Path.IsPathRooted(normalizedPath) || normalizedPath.Split('\\').Any(segment =>
                segment.Length == 0 || segment == "." || segment == ".."))
            {
                throw new InvalidDataException("Invalid codegen target path: " + path);
            }
            return normalizedPath;
        }

        // Exclude build intermediates and backups that also match *.g.*.
        static bool IsIncludedCodegenFile(string path)
        {
            string fileName = Path.GetFileName(path);
            return !fileName.EndsWith(".g.obj", StringComparison.OrdinalIgnoreCase) &&
                   fileName.IndexOf(".nuget.g.", StringComparison.OrdinalIgnoreCase) < 0 &&
                   !fileName.EndsWith(".backup", StringComparison.OrdinalIgnoreCase);
        }

        static void RepartitionAndReplaceMasters(string mastersRoot, FileTree chkMasters, FileTree freMasters)
        {
            // Only files present in both flavors with identical bytes belong in common.
            var commonMasters = new FileTree();
            var chkSpecificMasters = new FileTree();
            var freSpecificMasters = new FileTree();
            foreach (var chkFile in chkMasters.Entries)
            {
                if (freMasters.HasSameContents(chkFile.Key, chkFile.Value))
                {
                    commonMasters.Add(chkFile.Key, chkFile.Value);
                }
                else
                {
                    chkSpecificMasters.Add(chkFile.Key, chkFile.Value);
                }
            }
            foreach (var freFile in freMasters.Entries)
            {
                if (!commonMasters.Contains(freFile.Key))
                {
                    freSpecificMasters.Add(freFile.Key, freFile.Value);
                }
            }

            // Stage the complete layout beside the existing tree and keep the old tree for rollback.
            string mastersParentDirectory = Directory.GetParent(mastersRoot).FullName;
            string uniqueSuffix = Guid.NewGuid().ToString("N").Substring(0, 8);
            string stagingDirectory = Path.Combine(mastersParentDirectory, ".masters-new-" + uniqueSuffix);
            string backupDirectory = Path.Combine(mastersParentDirectory, ".masters-old-" + uniqueSuffix);
            Directory.CreateDirectory(stagingDirectory);
            try
            {
                commonMasters.WriteToDirectory(Path.Combine(stagingDirectory, Flavor.Common));
                chkSpecificMasters.WriteToDirectory(Path.Combine(stagingDirectory, Flavor.Chk));
                freSpecificMasters.WriteToDirectory(Path.Combine(stagingDirectory, Flavor.Fre));

                MoveDirectoryWithRetry(mastersRoot, backupDirectory);
                try
                {
                    MoveDirectoryWithRetry(stagingDirectory, mastersRoot);
                }
                catch
                {
                    // Restore the original tree if the staged tree cannot take its place.
                    Console.Error.WriteLine("Could not install staged masters. Destination exists: {0}; backup: {1}",
                        Directory.Exists(mastersRoot), backupDirectory);
                    MoveDirectoryWithRetry(backupDirectory, mastersRoot);
                    throw;
                }
                Directory.Delete(backupDirectory, true);
            }
            finally
            {
                if (Directory.Exists(stagingDirectory))
                {
                    Directory.Delete(stagingDirectory, true);
                }
            }

            Console.WriteLine($"Masters: {commonMasters.Count} {Flavor.Common}, " +
                $"{chkSpecificMasters.Count} {Flavor.Chk}-only, {freSpecificMasters.Count} {Flavor.Fre}-only.");
        }

        // Directory.Move can fail while another process briefly holds a tree open; retry only
        // when the failed move left the source intact and the destination absent.
        static void MoveDirectoryWithRetry(string source, string destination)
        {
            for (int attempt = 0; ; attempt++)
            {
                try
                {
                    Directory.Move(source, destination);
                    return;
                }
                catch (IOException) when (attempt < 10 && Directory.Exists(source) && !Directory.Exists(destination))
                {
                    Thread.Sleep(100 * (attempt + 1));
                }
            }
        }

        // Preserve each generated file's UTF-8 BOM while normalizing volatile lines.
        static byte[] ReadNormalizedGeneratedFile(string generatedFilePath)
        {
            using (var outputStream = new MemoryStream())
            {
                bool hasByteOrderMark = HasUtf8ByteOrderMark(new FileInfo(generatedFilePath));
                using (var normalizedWriter = new StreamWriter(outputStream, new UTF8Encoding(hasByteOrderMark)))
                {
                    foreach (string generatedLine in File.ReadAllLines(generatedFilePath))
                    {
                        normalizedWriter.WriteLine(NormalizeVolatileLine(generatedLine));
                    }
                }
                return outputStream.ToArray();
            }
        }

        static bool HasUtf8ByteOrderMark(FileInfo file)
        {
            byte[] preamble = Encoding.UTF8.GetPreamble();
            byte[] start = new byte[preamble.Length];

            using (var stream = file.OpenRead())
            {
                if (stream.Read(start, 0, start.Length) < start.Length)
                {
                    return false;
                }
            }

            for (int i = 0; i < preamble.Length; i++)
            {
                if (start[i] != preamble[i])
                {
                    return false;
                }
            }

            return true;
        }

        static string NormalizeVolatileLine(string line)
        {
            // Lines that are truncated because they carry a checksum or a tool version that
            // legitimately changes between builds. CodegenTests.IsException skips the same set when
            // diffing, so the two must be kept in agreement.
            string[] ignoreLines = {
                "#pragma checksum",
                "#ExternalChecksum",
                "// WARNING: Please don't edit this file",
            };

            foreach (var ignoreLine in ignoreLines)
            {
                if (line.StartsWith(ignoreLine, StringComparison.OrdinalIgnoreCase))
                {
                    return $"{ignoreLine}...";
                }
            }

            return line;
        }
    }

    // Directory names for the shared master layer and the two build flavors.
    internal static class Flavor
    {
        public const string Common = "common";
        public const string Chk = "chk";
        public const string Fre = "fre";
    }

    // Keep relative-path matching and disk traversal consistent across shared and flavor trees.
    internal sealed class FileTree
    {
        private readonly Dictionary<string, byte[]> _files;

        public FileTree()
        {
            _files = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase);
        }

        private FileTree(FileTree sourceTree)
        {
            _files = new Dictionary<string, byte[]>(sourceTree._files, StringComparer.OrdinalIgnoreCase);
        }

        public int Count { get { return _files.Count; } }

        public IEnumerable<KeyValuePair<string, byte[]>> Entries { get { return _files; } }

        public void Add(string relativePath, byte[] contents)
        {
            _files.Add(relativePath, contents);
        }

        public bool Contains(string relativePath)
        {
            return _files.ContainsKey(relativePath);
        }

        public bool HasSameContents(string relativePath, byte[] contents)
        {
            byte[] existingContents;
            return _files.TryGetValue(relativePath, out existingContents) && existingContents.SequenceEqual(contents);
        }

        // Overlay a flavor onto common without allowing duplicate Windows filenames.
        public static FileTree ReadFromDirectory(string directory, FileTree commonMasters = null)
        {
            var loadedTree = commonMasters == null ? new FileTree() : new FileTree(commonMasters);
            if (Directory.Exists(directory))
            {
                foreach (string filePath in Directory.GetFiles(directory, "*", SearchOption.AllDirectories))
                {
                    string relativePath = filePath.Substring(directory.Length + 1);
                    if (commonMasters != null && loadedTree.Contains(relativePath))
                    {
                        throw new InvalidDataException("Master exists in both common and a flavor: " + relativePath);
                    }
                    loadedTree.Add(relativePath, File.ReadAllBytes(filePath));
                }
            }
            return loadedTree;
        }

        public void WriteToDirectory(string directory)
        {
            Directory.CreateDirectory(directory);
            foreach (var fileEntry in _files)
            {
                string destinationPath = Path.Combine(directory, fileEntry.Key);
                Directory.CreateDirectory(Path.GetDirectoryName(destinationPath));
                File.WriteAllBytes(destinationPath, fileEntry.Value);
            }
        }
    }
}
