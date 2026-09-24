// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
// Copyright(c) 2014 Microsoft Corporation
//--------------------------------------------------------------------------------------------

namespace XamlCompilerTestsUtilityTasks
{
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using Microsoft.Build.Utilities;
    using Microsoft.Build.Framework;

    public class CleanupSubDirectories : Task
    {
        [Required]
        public string DirectoryNames { get; set; }

        [Required]
        public string RootDirectory { get; set; }

        public override bool Execute()
        {
            string[] dirsSearchStrings = this.DirectoryNames.Split(new char[] { ';' });
            List<string> directoriesToDelete = new List<string>();
            foreach (string searchString in dirsSearchStrings)
            {
                directoriesToDelete.AddRange(Directory.GetDirectories(RootDirectory, searchString, SearchOption.AllDirectories).ToList());
            }
            foreach (string dir in directoriesToDelete)
            {
                foreach (string filePath in Directory.GetFiles(dir, "*", SearchOption.AllDirectories))
                {
                    File.SetAttributes(filePath, FileAttributes.Normal);
                    File.Delete(filePath);
                }
            }

            return true;
        }
    }
}
