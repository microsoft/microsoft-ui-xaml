// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    using DirectUI;

    internal static class FileHelpers
    {
        private static readonly char[] DirectorySeparators = { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar };

        public static void BackupFile(string fullPathFileName)
        {
            string backupfile = fullPathFileName + KnownStrings.BackupSuffix;

            if (File.Exists(backupfile))
            {
                File.Delete(backupfile);
            }

            FileInfo fileInfo = new FileInfo(fullPathFileName);
            DateTime dt = fileInfo.LastWriteTime;
            File.Move(fullPathFileName, backupfile);
            File.SetLastWriteTime(backupfile, dt);
        }

        public static bool RestoreBackupFile(string fullPathFileName)
        {
            string backupfile = fullPathFileName + KnownStrings.BackupSuffix;

            if (File.Exists(backupfile))
            {
                if (File.Exists(fullPathFileName))
                {
                    File.Delete(fullPathFileName);
                }

                FileInfo fileInfo = new FileInfo(backupfile);
                DateTime dt = fileInfo.LastWriteTime;
                File.Move(backupfile, fullPathFileName);

                // It's important to restore the preserved timestamp so that the compiler
                // doesn't incorrectly think that the restored file is "new".
                File.SetLastWriteTime(fullPathFileName, dt);

                return true;
            }
            return false;
        }

        public static void BackupIfExistsAndTruncateToNull(string filename)
        {
            // We need to clear the input file during Pass1 so that output from a prior Pass2
            // does not contaminate the skeleton assembly (whether by presenting it with inputs
            // that result in a substantively different API surface, or by having stale type information
            // results in a build break) when it is built after the current Pass1; to avoid having to
            // always recreate it during Pass2 we back up the original file and preserve its timestamp.
            // The backup is restored during Pass2 if we determine that it isn't necessary to generate
            // a fresh file.
            FileInfo fileInfo = new FileInfo(filename);
            if (fileInfo.Exists && fileInfo.Length > 0)
            {
                FileHelpers.BackupFile(filename);
            }
            DateTime dt = fileInfo.LastWriteTime;
            using (FileStream f = File.Create(filename)) { /* create empty file */ }
            File.SetLastWriteTime(filename, dt);
        }

        public static string GetSafeName(string ProjectName)
        {
            StringBuilder safeName;

            if (ProjectName == null)
            {
                return null;
            }

            safeName = new StringBuilder(ProjectName);

            UnicodeCategory uc;
            for (int i = 0; i < safeName.Length; i++)
            {
                uc = Char.GetUnicodeCategory(safeName[i]);
                bool idStart = (uc == UnicodeCategory.UppercaseLetter || // (Lu)
                        uc == UnicodeCategory.LowercaseLetter || // (Ll)
                        uc == UnicodeCategory.TitlecaseLetter || // (Lt)
                        uc == UnicodeCategory.OtherLetter || // (Lo)
                        uc == UnicodeCategory.LetterNumber || // (Nl)
                        safeName[i] == '_');
                bool idExtend = (uc == UnicodeCategory.NonSpacingMark || // (Mn)
                              uc == UnicodeCategory.SpacingCombiningMark || // (Mc)
                              uc == UnicodeCategory.ModifierLetter || // (Lm)
                              uc == UnicodeCategory.DecimalDigitNumber); // (Nd)
                if (i == 0)
                {
                    if (!idStart)
                    {
                        safeName[i] = '_';
                    }
                }
                else if (!(idStart || idExtend))
                {
                    safeName[i] = '_';
                }
            }

            return safeName.ToString();
        }

        public static String GetRelativePath(String currentDir, String filePath)
        {
            string[] currentDirSplit = Path.GetFullPath(currentDir).Split(FileHelpers.DirectorySeparators, StringSplitOptions.RemoveEmptyEntries);

            String fileName = Path.GetFileName(filePath);
            String fileDir = Path.GetDirectoryName(Path.GetFullPath(filePath));
            string[] fileDirSplit = fileDir.Split(FileHelpers.DirectorySeparators);

            if (currentDirSplit[0] != fileDirSplit[0])
            {
                // starting Dir and target file are not on the same Drive.
                // return full path of the target file.
                return Path.GetFullPath(filePath);
            }
            int lastCommonPart;
            int shortest = Math.Min(currentDirSplit.Length, fileDirSplit.Length);
            for (lastCommonPart = 0; lastCommonPart < shortest; lastCommonPart++)
            {
                if (currentDirSplit[lastCommonPart] != fileDirSplit[lastCommonPart])
                    break;
            }

            List<String> args = new List<string>();

            // path up the current directory path to the common point.
            if (currentDirSplit.Length > lastCommonPart)
            {
                for (int i = lastCommonPart; i < currentDirSplit.Length; i++)
                {
                    args.Add("..");
                }
            }

            // Path down the file path from the common point.
            if (fileDirSplit.Length > lastCommonPart)
            {
                for (int i = lastCommonPart; i < fileDirSplit.Length; i++)
                {
                    args.Add(fileDirSplit[i]);
                }
            }
            args.Add(fileName);

            String ret = Path.Combine(args.ToArray());
            return ret;
        }

        public static string ComputeBaseFolder(string folder1, string folder2)
        {
            if (folder2.StartsWith(folder1))
            {
                return folder1;
            }
            if (folder1.StartsWith(folder2))
            {
                return folder2;
            }

            return FileHelpers.ComputeBaseFolder(Path.GetDirectoryName(folder1), Path.GetDirectoryName(folder2));
        }

        // Keep the per instance cache of platform assembly names. Since these differ between C++ and C#, we need
        // to make sure we don't re-use these across msbuild instances
        private static Core.InstanceCache<string, string> _platformAssemblyNameCache = new Core.InstanceCache<string, string>();

        public static bool IsPlatformAssembly(DirectUIAssembly assembly)
        {
            // If we've previously found the assembly containing the Windows platform type info, we'll just directly compare against that.
            if (_platformAssemblyNameCache.TryGetValue(KnownStrings.PlatformAssemblySentinelType, out string cachedPlatformAssemblyName))
            {
                return cachedPlatformAssemblyName.Equals(assembly.GetName().FullName, StringComparison.OrdinalIgnoreCase);
            }
            else if (assembly.GetType(KnownStrings.PlatformAssemblySentinelType) != null)
            {
                _platformAssemblyNameCache[KnownStrings.PlatformAssemblySentinelType] = assembly.GetName().FullName;
                return true;
            }
            return false;
        }

        public static bool IsWinUIAssembly(DirectUIAssembly assembly)
        {
            // If we've previously found the assembly containing WinUI type info, we'll just directly compare against that.
            if (_platformAssemblyNameCache.TryGetValue(KnownStrings.WinUIAssemblySentinelType, out string cachedPlatformAssemblyName))
            {
                return cachedPlatformAssemblyName.Equals(assembly.GetName().FullName, StringComparison.OrdinalIgnoreCase);
            }
            else if (assembly.GetType(KnownStrings.WinUIAssemblySentinelType) != null)
            {
                _platformAssemblyNameCache[KnownStrings.WinUIAssemblySentinelType] = assembly.GetName().FullName;
                return true;
            }
            return false;
        }

        public static bool IsFacadeWinmd(Assembly asm, string windowsSdkPath)
        {
            // If we're in Razzle and don't have an SDK path, just return false (OK since this method is only used to skip some checks when the winmd really is a facade)
            if (string.IsNullOrWhiteSpace(windowsSdkPath))
            {
                return false;
            }

            bool? ret = null;
            try
            {
                ret = (asm as DirectUIAssembly)?.Location?.StartsWith(Path.Combine(windowsSdkPath, @"UnionMetadata\facade\Windows.winmd"), StringComparison.OrdinalIgnoreCase);
            }
            catch
            {
                // We should never fail to get the location of the assembly. Added this try as an extra assurance in the limited impact of the change.
                Debug.Fail("Assembly location could not be found: " + asm?.ToString());
            }
            return ret.HasValue && ret.Value;
        }
    }
}
