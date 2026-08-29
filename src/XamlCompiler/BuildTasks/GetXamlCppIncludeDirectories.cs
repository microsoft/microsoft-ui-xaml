// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    public class GetXamlCppIncludeDirectories : Task
    {
        #region Task Input Properties

        [Required]
        public ITaskItem[] ClCompile { get; set; }

        #endregion

        #region Task Output Properties

        [Output]
        public String ComputedIncludeDirectories { get; set; }

        #endregion

        public override bool Execute()
        {
            List<String> includeDirs = GetAdditionalIncludeDirectories(ClCompile);
            ComputedIncludeDirectories = CombineListOfFolders(includeDirs);
            return true;
        }

        private string CombineListOfFolders(List<String> folders)
        {
            string result = String.Empty;
            if (folders.Count > 0)
            {
                result = folders[0];
            }
            for (int i = 1; i < folders.Count; i++)
            {
                result += ";" + folders[i];
            }
            return result;
        }

        private static List<String> GetAdditionalIncludeDirectories(IEnumerable<ITaskItem> items)
        {
            List<ITaskItem> xamlDependentItems = GetXamlDependentItems(items);
            List<String> includeDirectories = GetUnionOfAdditionalIncludeDirectoriesElements(xamlDependentItems);
            return includeDirectories;
        }

        // Find the Items with a XAML file DependentUpon.
        private static List<ITaskItem> GetXamlDependentItems(IEnumerable<ITaskItem> items)
        {
            List<ITaskItem> xamlDependentItems = new List<ITaskItem>();
            foreach (ITaskItem item in items)
            {
                string dep = item.GetMetadata("DependentUpon");
                if (dep.EndsWith(KnownStrings.XamlExtension, StringComparison.OrdinalIgnoreCase))
                {
                    xamlDependentItems.Add(item);
                }
            }
            return xamlDependentItems;
        }

        private static List<String> GetUnionOfAdditionalIncludeDirectoriesElements(IEnumerable<ITaskItem> items)
        {
            HashSet<String> includeDirsProperties = new HashSet<String>();
            foreach (ITaskItem item in items)
            {
                string incdirs = item.GetMetadata("AdditionalIncludeDirectories");
                if (incdirs != null)
                {
                    includeDirsProperties.Add(incdirs);
                }
            }

            // In the normal case the AdditionalIncludeDirectories are the same on each item.
            // So at this point the includeDirsProperties HashSet will normally only have one element in it.
            // But it might have several if the project has assigned different values to some items.
            HashSet<String> includeDirectories = new HashSet<string>();
            foreach (string includePath in includeDirsProperties)
            {
                string[] paths = includePath.Split( new String[]{";", "\r\n"}, System.StringSplitOptions.RemoveEmptyEntries);
                foreach (string path in paths)
                {
                    // Trim the front and the tail.  Of course
                    // white space in the middle is preserved
                    string trimedPath = path.Trim();
                    includeDirectories.Add(trimedPath);
                }
            }

            List<String> result = new List<string>();
            foreach (string path in includeDirectories)
            {
                result.Add(path);
            }
            return result;
        }

    }
}
