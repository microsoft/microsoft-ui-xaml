// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is no test coverage for BatchMergeXaml, MergedDictionary and StripNamespaces.
// Please manually verify them if you make change on it. For example, checkout the buildoutput intermediate files 
// and do the comparision between 19h1_generic_2dot5.prefixed.xaml and 19h1_generic_2dot5.xaml 

using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace CustomTasks
{
    public class BatchMergeXaml : Task
    {
        public string PagesFilteredBy { get; set; }

        [Required]
        public ITaskItem[] RS1Pages { get; set; }

        [Required]
        public ITaskItem[] RS2Pages { get; set; }

        [Required]
        public ITaskItem[] RS3Pages { get; set; }

        [Required]
        public ITaskItem[] RS4Pages { get; set; }

        [Required]
        public ITaskItem[] RS5Pages { get; set; }

        [Required]
        public ITaskItem[] N19H1Pages { get; set; }

        [Required]
        // The output file format is like rs1_themeresources.xaml, rs2_generic.xaml, rs2_compact_generic.xaml.
        // then PostfixForGeneratedFile is themeresources/generic/compact_generic here.
        public string PostfixForGeneratedFile { get; set; }

        [Required]
        public string OutputDirectory { get; set; }

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
        
        private string postfixForPrefixedGeneratedFile;

        // When generating merged file for RS5, we don't need to parse RS1-RS4 pages again, but put it in nextBaseFile like rs4_themeresources.prefix.xaml, then make it as the base of next merge.
        private string nextBaseFile = null;

        private void ExecuteForTaskItems(ITaskItem[] items, string targetOSVersion)
        {
            MergedDictionary mergedDictionary = MergedDictionary.CreateMergedDicionary();
            List<string> files = new List<string>();
            if (nextBaseFile != null)
            {
                files.Add(nextBaseFile);
            }
            if (items != null)
            {
                foreach (ITaskItem item in items)
                {
                    if (!string.IsNullOrEmpty(PagesFilteredBy) && !item.GetMetadata(PagesFilteredBy).Equals("true", StringComparison.OrdinalIgnoreCase))
                    {
                        Log.LogMessage(MessageImportance.Low, "Filtered item " + item.ItemSpec);
                        continue;
                    }

                    string file = item.ItemSpec;
                    if (File.Exists(file))
                    {
                        files.Add(file);
                    }
                    else
                    {
                        Log.LogError("Can't find page file " + file);
                    }
                }
            }

            int apiVersion = StripNamespaces.universalApiContractVersionMapping[targetOSVersion];
            MergeAndGenerateXaml(mergedDictionary, files, targetOSVersion.ToLower(), apiVersion);
        }

        private void MergeAndGenerateXaml(MergedDictionary mergedDictionary, List<string> files, string targetOSVersion, int apiVersion)
        {
            Log.LogMessage("Merge and generate xaml Files for target os" + targetOSVersion);

            foreach (string file in files)
            {
                try
                {
                    mergedDictionary.MergeContent(File.ReadAllText(file));
                }
                catch (Exception)
                {
                    Log.LogError("Exception found when merge file " + file);
                    throw;
                }
            }

            string content = mergedDictionary.ToString();

            string name = targetOSVersion + "_" + PostfixForGeneratedFile + ".xaml";
            string fullPath = Path.Combine(OutputDirectory, name);

            string prefixedName = targetOSVersion + "_" + postfixForPrefixedGeneratedFile + ".xaml";
            string prefixedFullPath = Path.Combine(OutputDirectory, prefixedName);

            string strippedContent = StripNamespaces.StripNamespaceForAPIVersion(content, apiVersion);

            filesWritten.Add(Utils.RewriteFileIfNecessary(prefixedFullPath, content));
            filesWritten.Add(Utils.RewriteFileIfNecessary(fullPath, strippedContent));

            nextBaseFile = prefixedFullPath;
        }

        public override bool Execute()
        {
            if (string.IsNullOrEmpty(OutputDirectory) || !Directory.Exists(OutputDirectory))
            {
                Log.LogError("OutputDirectory is empty or not existing");
            }

            if (string.IsNullOrEmpty(PostfixForGeneratedFile))
            {
                Log.LogError("PostfixForGeneratedFile is empty");
            }

            postfixForPrefixedGeneratedFile = PostfixForGeneratedFile + ".prefixed";

            if (!Log.HasLoggedErrors)
            {
                ExecuteForTaskItems(RS1Pages, "RS1");
                ExecuteForTaskItems(RS2Pages, "RS2");
                ExecuteForTaskItems(RS3Pages, "RS3");
                ExecuteForTaskItems(RS4Pages, "RS4");
                ExecuteForTaskItems(RS5Pages, "RS5");
                ExecuteForTaskItems(N19H1Pages, "19H1");
            }

            var filesRead = new List<string>();
            filesRead.AddRange(RS1Pages.Select(item => item.ItemSpec));
            filesRead.AddRange(RS2Pages.Select(item => item.ItemSpec));
            filesRead.AddRange(RS3Pages.Select(item => item.ItemSpec));
            filesRead.AddRange(RS4Pages.Select(item => item.ItemSpec));
            filesRead.AddRange(RS5Pages.Select(item => item.ItemSpec));
            filesRead.AddRange(N19H1Pages.Select(item => item.ItemSpec));

            File.WriteAllLines(TlogReadFilesOutputPath, filesRead);

            File.WriteAllLines(TlogWriteFilesOutputPath, FilesWritten);

            return !Log.HasLoggedErrors;
        }
    }
}
