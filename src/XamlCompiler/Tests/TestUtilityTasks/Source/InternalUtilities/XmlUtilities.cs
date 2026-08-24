// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
// Copyright(c) 2014 Microsoft Corporation
//--------------------------------------------------------------------------------------------

namespace XamlCompilerTestsUtilityTasks.Utilities
{
	using System;
	using System.Collections.Generic;
	using System.IO;
	using System.Linq;
	using System.Text;
	using System.Xml;

	internal static class Utilities
	{
		internal static XmlNode SimpleSelectDocument(XmlDocument doc, string searchPath)
		{
			XmlNode result = null;
			List<string> searchPaths = searchPath.Split(new char[] { '/' }, StringSplitOptions.RemoveEmptyEntries).ToList();
			string rootNode = searchPaths[0];
			searchPaths.RemoveAt(0);
			foreach (XmlNode child in from XmlNode c in doc.ChildNodes where string.Equals(c.Name, rootNode) select c)
			{
				result = SimpleSelectNode(child, searchPaths, 0);
				if (result != null)
					break;
			}

			return result;
		}

		internal static XmlNode SimpleSelectNode(XmlNode node, List<string> searchPaths, int currentIndex)
		{
			if (searchPaths.Count == 0) throw new ArgumentException("searchPath must start with \\");

			foreach (XmlNode child in node.ChildNodes)
			{
				if (string.Equals(child.Name, searchPaths[currentIndex]))
				{
					currentIndex++;
					if (currentIndex == searchPaths.Count)
					{
						return child;
					}
					var res = SimpleSelectNode(child, searchPaths, currentIndex);

					if (res != null)
					{
						return res;
					}

					currentIndex--;
				}
			}

			return null;
		}

		internal static void WriteTextToFile(string filePath, string text)
		{
			Encoding e = null;
			using (StreamReader reader = new StreamReader(filePath))
			{
				e = reader.CurrentEncoding;
			}

			using (StreamWriter writer = new StreamWriter(filePath, false, e))
			{
				writer.Write(text);
			}
		}
	}
}
