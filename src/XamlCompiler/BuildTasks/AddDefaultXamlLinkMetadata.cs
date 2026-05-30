// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    using Utilities;

    public class AddDefaultXamlLinkMetadata : Task
    {
        public const string DefaultXamlLink = "DefaultXamlLink";

        #region Task Input Properties
        [Required]
        public string ProjectPath { get; set; }

        public String CIncludeDirectories { get; set; }

        [Required]
        public ITaskItem[] XamlPages { get; set; }
        #endregion

        #region Task Output Properties
        [Output]
        public ITaskItem[] OutputItems { get; set; }
        #endregion

        #region Private fields
        private List<string> _folderList;
        #endregion

        public override bool Execute()
        {
            List<ITaskItem> outputList = new List<ITaskItem>();
            string projectFolderFullpath = Path.GetDirectoryName(ProjectPath);
            foreach (ITaskItem item in XamlPages)
            {
                string link = item.GetMetadata("Link");
                if (String.IsNullOrEmpty(link))
                {
                    string fullpath = item.GetMetadata("FullPath");
                    string itemSpec = item.ItemSpec;

                    string defaultXamlLink = CompileXamlInternal.GetDefaultXamlLinkMetadata(fullpath, itemSpec, projectFolderFullpath, IncludeFolderList);

                    if (defaultXamlLink != null)
                    {
                        item.SetMetadata(DefaultXamlLink, defaultXamlLink);
                        outputList.Add(item);
                    }
                }
            }

            OutputItems = outputList.ToArray();
            return true;
        }

        // The IncludeFolderList is all the folders on the Include Path.
        private List<string> IncludeFolderList
        {
            get
            {
                if(_folderList == null)
                {
                    string projectFolder = Path.GetDirectoryName(ProjectPath);
                    _folderList = XamlHelper.SplitAndEnsureFullpaths(CIncludeDirectories, projectFolder);
                }
                return _folderList;
            }
        }

    }
}
