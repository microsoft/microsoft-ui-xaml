using System.IO;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace CustomTasks
{
    public class GenerateMergedXaml : Task
    {
        [Required]
        public ITaskItem[] Sources { get; set; }

        [Required]
        public string XamlFileGenerated
        {
            get; set;
        }

        [Required]
        public string ForOSVersion
        {
            get; set;
        }

        [Output]
        public string[] FilesWritten
        {
            get; set;
        }

        public override bool Execute()
        {
            if (string.IsNullOrEmpty(ForOSVersion) || !StripNamespaces.universalApiContractVersionMapping.ContainsKey(ForOSVersion))
            {
                Log.LogError("Unknown OS version, and valid version is ", string.Join(",", StripNamespaces.universalApiContractVersionMapping.Keys));
            }

            MergedDictionary mergedDictionary = MergedDictionary.CreateMergedDicionary();
            foreach (ITaskItem item in Sources)
            {
                string file = item.ItemSpec;
                if (File.Exists(file))
                {
                    mergedDictionary.MergeContent(File.ReadAllText(file));
                }
                else
                {
                    Log.LogError("Error to look for file " + item.ItemSpec);
                }

            }

            int apiVersion = StripNamespaces.universalApiContractVersionMapping[ForOSVersion];
            string content = mergedDictionary.ToString();

            string prefixedName = XamlFileGenerated.Substring(0, XamlFileGenerated.Length - 5) + ".prefixed.xaml";
            string strippedContent = StripNamespaces.StripNamespaceForAPIVersion(content, apiVersion);

            string[] filesWritten = new string[2];
            filesWritten[0] = Utils.RewriteFileIfNecessary(prefixedName, content);
            filesWritten[1] = Utils.RewriteFileIfNecessary(XamlFileGenerated, strippedContent);

            FilesWritten = filesWritten;

            return !Log.HasLoggedErrors;
        }

    }
}
