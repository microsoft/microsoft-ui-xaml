// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is no test coverage for BatchMergeXaml, MergedDictionary and StripNamespaces.
// Please manually verify them if you make change on it. For example, checkout the buildoutput intermediate files 
// and do the comparision between 19h1_generic_v1.prefixed.xaml and 19h1_generic_v1.xaml 

using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace CustomTasks
{
    public class BatchMergeXaml : CustomTask
    {
        public string PagesFilteredBy { get; set; }

        [Required]
        public ITaskItem[] Pages { get; set; }

        [Required]
        public string MergedXamlFile { get; set; }

        [Required]
        public string TlogReadFilesOutputPath { get; set; }

        [Required]
        public string TlogWriteFilesOutputPath { get; set; }

        [Output]
        public string[] FilesWritten
        {
            get { return filesWritten.ToArray(); }
        }

        private List<string> filesWritten = new List<string>();

        public override bool Execute()
        {
            MergedDictionary mergedDictionary = MergedDictionary.CreateMergedDicionary();
            List<string> pages = new List<string>();

            if (Pages != null)
            {
                foreach (ITaskItem pageItem in Pages)
                {
                    if (!string.IsNullOrEmpty(PagesFilteredBy) && !pageItem.GetMetadata(PagesFilteredBy).Equals("true", StringComparison.OrdinalIgnoreCase))
                    {
                        this.LogMessage(MessageImportance.Low, "Filtered item " + pageItem.ItemSpec);
                        continue;
                    }

                    string page = pageItem.ItemSpec;
                    if (File.Exists(page))
                    {
                        pages.Add(page);
                    }
                    else
                    {
                        LogError($"Can't find page {page}!");
                    }
                }
            }

            if (HasLoggedErrors)
            {
                return false;
            }

            LogMessage($"Merging XAML files into {MergedXamlFile}...");

            foreach (string page in pages)
            {
                try
                {
                    mergedDictionary.MergeContent(File.ReadAllText(page));
                }
                catch (Exception)
                {
                    LogError($"Exception found when merging page {page}!");
                    throw;
                }
            }

            mergedDictionary.FinalizeXaml();
            filesWritten.Add(Utils.RewriteFileIfNecessary(MergedXamlFile, mergedDictionary.ToString()));

            File.WriteAllLines(TlogReadFilesOutputPath, Pages.Select(page => page.ItemSpec));
            File.WriteAllLines(TlogWriteFilesOutputPath, FilesWritten);

            return !HasLoggedErrors;
        }
    }
}
