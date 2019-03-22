using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System;
using System.Collections.Generic;
using System.IO;

namespace CustomTasks
{
    public class BatchMergeXaml : Task
    {
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
        public string PostfixForGeneratedFile { get; set; }

        [Required]
        public string OutputDirectory { get; set; }

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

            string name = targetOSVersion + PostfixForGeneratedFile;
            string fullPath = Path.Combine(OutputDirectory, name);

            string prefixedName = targetOSVersion + postfixForPrefixedGeneratedFile;
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

            if (string.IsNullOrEmpty(PostfixForGeneratedFile) || !PostfixForGeneratedFile.EndsWith(".xaml"))
            {
                Log.LogError("PostfixForGeneratedFile is empty or extension name is not .xaml");
            }

            postfixForPrefixedGeneratedFile = PostfixForGeneratedFile.Substring(0, PostfixForGeneratedFile.Length - 5) + ".prefixed.xaml";

            if (!Log.HasLoggedErrors)
            {
                ExecuteForTaskItems(RS1Pages, "RS1");
                ExecuteForTaskItems(RS2Pages, "RS2");
                ExecuteForTaskItems(RS3Pages, "RS3");
                ExecuteForTaskItems(RS4Pages, "RS4");
                ExecuteForTaskItems(RS5Pages, "RS5");
                ExecuteForTaskItems(N19H1Pages, "19H1");
            }
            return !Log.HasLoggedErrors;
        }
    }
}
