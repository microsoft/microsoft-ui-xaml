// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    internal class XamlHelper
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

    }
}
