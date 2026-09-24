// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
// Copyright(c) 2014 Microsoft Corporation
//--------------------------------------------------------------------------------------------
namespace XamlCompilerTestsUtilityTasks
{
    using System;
    using Microsoft.Build.Utilities;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.Versioning;
    using Microsoft.Build.Framework;
    using System.Diagnostics;
    using System.IO;

    public class FindMostRecentInstalledUapSDKVersion : Task
    {
        [Output]
        public string MostRecentVersion { get; set; }

        public override bool Execute()
        {
            return this.executeInternal();
        }

        private bool executeInternal()
        {
            IEnumerable<TargetPlatformSDK> platformSdks = ToolLocationHelper.GetTargetPlatformSdks();
            TargetPlatformSDK sdk = platformSdks.Where(p => p.TargetPlatformIdentifier.Equals(Constants.WindowsSDKsString, StringComparison.OrdinalIgnoreCase))
                .OrderByDescending(s => s.TargetPlatformVersion)
                .FirstOrDefault();

            if (sdk == null)
            {
                return false;
            }

            string platformDirectoryPath = ToolLocationHelper.GetPlatformSDKLocation(Constants.WindowsSDKsString, sdk.TargetPlatformVersion) + @"\Platforms\UAP\";
            Version greatest = new Version();
            foreach (string dirName in Directory.GetDirectories(platformDirectoryPath, "*", SearchOption.TopDirectoryOnly))
            {
                var currentVersion = new Version(Path.GetFileName(dirName));
                if (currentVersion > greatest) { greatest = currentVersion; }
            }

            this.MostRecentVersion = greatest.ToString();

            return true;
        }
    }
}
