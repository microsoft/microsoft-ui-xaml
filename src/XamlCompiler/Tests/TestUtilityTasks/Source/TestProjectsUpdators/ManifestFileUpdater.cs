// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
// Copyright(c) 2014 Microsoft Corporation
//--------------------------------------------------------------------------------------------

namespace XamlCompilerTestsUtilityTasks.TestProjectsUpdators
{
	using System;
	using System.Collections.Generic;
	using System.IO;
	using System.Xml;
	using XamlCompilerTestsUtilityTasks.Utilities;

	internal class ManifestFileUpdater
	{
		private string enlistmentRootPath;

		internal ManifestFileUpdater(string enlistmentRoot)
		{
			this.enlistmentRootPath = enlistmentRoot;
		}

		internal void EnsureManifestFilesUpToDate()
		{
			var xamlCompilerRegressionTestsProjectRoot = this.enlistmentRootPath + Constants.xamlCompilerRegressionTestsRootSubPath;

			foreach (string filePath in Directory.EnumerateFiles(xamlCompilerRegressionTestsProjectRoot, Constants.packageManifestFileName, SearchOption.AllDirectories))
			{
				ApplyReplacementDictionary(filePath, GetFileReplacementDictionary(filePath));
			}
		}

		private Dictionary<string, string> GetFileReplacementDictionary(string filePath)
		{
			if (Path.GetFileName(filePath) != Constants.packageManifestFileName)
				throw new ArgumentException("Invlaid package manifest file path");

			Dictionary<string, string> result = new Dictionary<string, string>();

			XmlDocument doc = new XmlDocument();

			doc.Load(filePath);

			XmlNode node = Utilities.SimpleSelectDocument(doc, "/Package/Identity");
			result["$guid9$"] = node.Attributes["Name"].Value;
			result["$XmlEscapedPublisherDistinguishedName$"] = node.Attributes["Publisher"].Value;

			node = Utilities.SimpleSelectDocument(doc, "/Package/Properties/PublisherDisplayName");
			result["$XmlEscapedPublisher$"] = node.InnerText;

			node = Utilities.SimpleSelectDocument(doc, "/Package/Properties/DisplayName");
			result["$projectname$"] = node.InnerText;

			node = Utilities.SimpleSelectDocument(doc, "/Package/Applications/Application");
			var safeprojectname = node.Attributes["EntryPoint"].Value;
			result["$safeprojectname$"] = safeprojectname.Substring(0, safeprojectname.IndexOf(".App"));

			return result;
		}

		private void ApplyReplacementDictionary(string filePath, Dictionary<string, string> replacementDictionary)
		{
			string template = File.ReadAllText(enlistmentRootPath + Constants.uapManagedManifestSubPath);
			foreach (KeyValuePair<string, string> t in replacementDictionary)
			{
				template = template.Replace(t.Key, t.Value);
			}

			var oldtext = File.ReadAllText(filePath);

			if (!oldtext.Equals(template))
			{
				Utilities.WriteTextToFile(filePath, template);
			}
		}
	}
}
