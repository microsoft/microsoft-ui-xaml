// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    // Path helpers, shared by the compiler and the AddDefaultXamlLinkMetadata task (Microsoft.UI.Xaml.Markup.Compiler.Tasks)
    internal static class XamlHelper
    {
        internal static List<String> SplitAndEnsureFullpaths(string path, string projectFolderFullpath)
        {
            List<string> result;
            if (path == null)
            {
                result = new List<string>();
            }
            else
            {
                result = EnsureFullpaths(path.Split(';'), projectFolderFullpath);
            }
            return result;
        }

        internal static List<String> EnsureFullpaths(string [] paths, string projectFolderFullpath)
        {
            List<string> result = new List<string>();
            if (paths != null)
            {
                String[] pathDirs = paths;
                foreach (string p in pathDirs)
                {
                    string rootedPath = EnsureFullpath(p, projectFolderFullpath);
                    result.Add(rootedPath);
                }
            }
            return result;
        }

        private static String EnsureFullpath(string path, string projectFolderFullpath)
        {
            string rootedPath = path;
            if (!Path.IsPathRooted(rootedPath))
            {
                // ProjectPath is rooted.
                rootedPath = Path.Combine(projectFolderFullpath, path);
            }

            // Path.GetFullPath will press out the '\..\'s and other confusion.
            rootedPath = Path.GetFullPath(rootedPath);
            return rootedPath;
        }
        private static readonly char[] s_separators = new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar };

        // C++ does not use <Link> data so this will compute "implied" link data based on the IncludePath
        // Managed Items should use <Link> metadata in the PROj file and should not use this.
        // If nothing usefull can be found, the "relative path" is the bare filename.
        // returns null if the fileItemSpec (relative file path specified on the item) is in the project tree and useable as is.
        internal static string GetDefaultXamlLinkMetadata(string fileFullpath, string fileItemSpec, string projectFolderFullpath, IEnumerable<string> includeFullpathList)
        {
            string relativePath = null;
            string tail;
            if (!IsFilePathInOrUnderFolderPath(fileFullpath, projectFolderFullpath, out tail))
            {
                relativePath = GetBestSubPath(fileFullpath, includeFullpathList);
                if (relativePath == null)
                {
                    relativePath = Path.GetFileName(fileFullpath);
                }
            }
            else
            {
                // If the XAML page is in the project but the path is
                // not a canonical relative path; i.e. either rooted or with "..".
                // Then use the XAML Default Link metadata to correct that.
                if (Path.IsPathRooted(fileItemSpec) || PathContainsComponent(fileItemSpec, ".."))
                {
                    relativePath = fileFullpath.Substring(projectFolderFullpath.Length + 1);
                }
            }
            return relativePath;
        }


        internal static bool IsFilePathInOrUnderFolderPath(string file, string folder, out string relativePath)
        {
            relativePath = null;
            if (String.IsNullOrWhiteSpace(file) || String.IsNullOrWhiteSpace(folder))
            {
                return false;
            }
            if (file.Length <= folder.Length)
            {
                return false;
            }
            if (!folder.EndsWith("\\"))
            {
                folder += '\\';
            }
            if (file.StartsWith(folder, StringComparison.OrdinalIgnoreCase))
            {
                relativePath = file.Substring(folder.Length);
                return true;
            }
            return false;
        }

        // Find the shortest "downward" relative path for fullpath that is "below"
        // a folder on the folderlist.
        // returns NULL if none match.
        internal static string GetBestSubPath(string fullpath, IEnumerable<string> folderList)
        {
            string result = null;

            foreach (string folder in folderList)
            {
                string tail = null;
                if (IsFilePathInOrUnderFolderPath(fullpath, folder, out tail))
                {
                    if (result == null || tail.Length < result.Length)
                    {
                        result = tail;
                    }
                }
            }
            return result;
        }

        internal static bool PathContainsComponent(string path, string component)
        {
            string[] nodes = path.Split(s_separators);

            foreach (string node in nodes)
            {
                if (String.Equals(node, component, StringComparison.OrdinalIgnoreCase))
                {
                    return true;
                }
            }
            return false;
        }
    }
}
