// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
// Copyright(c) 2014 Microsoft Corporation
//--------------------------------------------------------------------------------------------

namespace XamlCompilerTestsUtilityTasks.TestProjectsUpdators
{
    using System.IO;
    using System.Linq;
    using System.Xml;
    using XamlCompilerTestsUtilityTasks.Utilities;

    internal class ProjectFileUpdater
    {
        private string enlistmentRootPath;

        internal ProjectFileUpdater(string enlistmentRoot)
        {
            this.enlistmentRootPath = enlistmentRoot;
        }

        internal void EnsureProjectFileUpToDate()
        {
            var xamlCompilerRegressionTestsProjectRoot = this.enlistmentRootPath + Constants.xamlCompilerRegressionTestsRootSubPath;
            var findSDKService = new FindMostRecentInstalledUapSDKVersion();
            findSDKService.Execute();
            foreach (string filePath in Directory.EnumerateFiles(xamlCompilerRegressionTestsProjectRoot, "*.*proj", SearchOption.AllDirectories).Where(c => c.EndsWith(".csproj") || c.EndsWith(".vbproj")))
            {
                ApplyReplacement(filePath, findSDKService.MostRecentVersion);
            }
        }

        private void ApplyReplacement(string filePath, string newSDKVersion)
        {
            XmlDocument doc = new XmlDocument();
            var oldText = File.ReadAllText(filePath);
            doc.Load(filePath);
            XmlNode tpvNode = Utilities.SimpleSelectDocument(doc, "/Project/PropertyGroup/TargetPlatformVersion");
            XmlNode tpmvNode = Utilities.SimpleSelectDocument(doc, "/Project/PropertyGroup/TargetPlatformMinVersion");
            var newText = oldText.Replace(tpvNode.InnerText, newSDKVersion);
            newText = newText.Replace(tpmvNode.InnerText, newSDKVersion);
            if (!newText.Equals(oldText))
            {
                Utilities.WriteTextToFile(filePath, newText);
            }
        }
    }
}
