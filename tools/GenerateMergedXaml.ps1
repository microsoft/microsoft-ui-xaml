Param(
    [string]$BaseXamlFile,

    [Parameter(Mandatory = $true)] 
    [string]$XamlFileList,

    [string]$MergedXamlName,

    [Parameter(Mandatory = $true)] 
    [string]$MergedXamlFilePath,
    
    [ValidateSet("Ignore", "Reorder")]
    [System.String]$DependencyHandling = "Ignore",

    [string[]]$ExcludeStyleKeys = @(),

    [switch]$RemoveComments,

    [switch]$RemoveUsings,

    [Switch]$RemoveMUXPrefixFromResourceKeys
)

Import-Module $PSScriptRoot\Utils.psm1 -DisableNameChecking

if ($BaseXamlFile.Length -gt 0 -and $MergedXamlName.Length -eq 0)
{
    Write-Error "One of BaseXamlFile and MergedXamlName is unset - if one is provided, the other must be provided as well."
    exit 1
}

if ($BaseXamlFile.Length -gt 0)
{
    $BeginMergedXamlSectionComment = " Begin $MergedXamlName resources - DO NOT MANUALLY EDIT BELOW THIS LINE! "
    $EndMergedXamlSectionComment = " End $MergedXamlName resources - DO NOT MANUALLY EDIT ABOVE THIS LINE! "
}
else
{
    $BeginMergedXamlSectionComment = ""
    $EndMergedXamlSectionComment = ""
}

# If we have a base XAML file, we still want to merge it together with the other files,
# so we'll add it to our list of files to merge.  However, we'll put comments around the
# elements that came from files other than this file so we can remove them upon running
# this script a second time on the same file.
if ($BaseXamlFile.Length -gt 0)
{
    $XamlFileList = "$BaseXamlFile;$XamlFileList"
}

Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xaml @"
using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xaml;

namespace MergedXamlGeneration
{
    public static class ExclusionExtensions
    {
        private static List<string> ExcludeStyleKeys = new List<string> { $(($ExcludeStyleKeys | % { """$_""" }) -join ",") };
        private static List<string> ExcludeBasedOnKeys = new List<string> { $(($ExcludeStyleKeys | % { """{StaticResource $_}""" }) -join ",") };
        private static List<Func<XmlNode, bool>> checkShouldExcludeList = new List<Func<XmlNode, bool>>()
        {
            (XmlNode node) => {
                bool exclude = false;
                if (node.Name == "Style")
                {
                    if (ExcludeStyleKeys.Contains(node.AttributeValue("x:Key")))
                    {
                        System.Console.WriteLine("Excluding Style {0}", node.AttributeValue("x:Key"));
                        exclude = true;
                    }
                    else if (ExcludeBasedOnKeys.Contains(node.AttributeValue("BasedOn")))
                    {
                        System.Console.WriteLine("Excluding Style {0}", node.AttributeValue("BasedOn"));
                        exclude = true;
                    }
                }

                return exclude;
                }
        };

        public static string AttributeValue(this XmlNode node, string attributeName)
        {
            if (node.Attributes != null)
            {
                foreach (XmlAttribute attribute in node.Attributes)
                {
                    if (attribute.Name == attributeName)
                    {
                        return attribute.Value;
                    }
                }
            }

            return string.Empty;
        }

        public static bool ShouldExclude(this XmlNode node)
        {
            foreach (Func<XmlNode, bool> checkShouldExcludeFunc in checkShouldExcludeList)
            {
                if (checkShouldExcludeFunc(node))
                {
                    return true;
                }
            }

            return false;
        }
    }

    public class MergedDictionary
    {
        public MergedDictionary(XmlDocument document) : this(document, null) { }

        public MergedDictionary(XmlDocument document, MergedXamlGeneration.MergedDictionary parentDictionary)
        {
            owningDocument = document;
            xmlElement = owningDocument.CreateElement("ResourceDictionary", "http://schemas.microsoft.com/winfx/2006/xaml/presentation");

            if (parentDictionary == null)
            {
                xmlElement = owningDocument.AppendChild(xmlElement) as XmlElement;
                xmlElement.SetAttribute("xmlns:x", "http://schemas.microsoft.com/winfx/2006/xaml");
            }

            nodeList = new List<XmlNode>();
            nodeListNodesToIgnore = new List<int>();
            nodeKeyToNodeListIndexDictionary = new Dictionary<string, int>();
            mergedThemeDictionaryByKeyDictionary = new Dictionary<string, MergedDictionary>();
            namespaceList = new List<string>();
            this.parentDictionary = parentDictionary;
        }

        public void AddNamespace(string xmlnsString, string namespaceString)
        {
            if (!namespaceList.Contains(namespaceString))
            {
                xmlElement.SetAttribute("xmlns:" + xmlnsString, namespaceString);
                namespaceList.Add(namespaceString);
            }
        }

        public void AddNode(XmlNode node, Dictionary<string, string> xmlnsReplacementDictionary)
        {
            if (node.ShouldExclude())
            {
                return;
            }

            $(if ($RemoveComments)
            {
@"
            if (node is XmlComment)
            {
                return;
            }
"@
            })

            node = owningDocument.ImportNode(node, true);
            ReplaceNamespacePrefix(node, xmlnsReplacementDictionary);

            $(if ($RemoveMUXPrefixFromResourceKeys)
            {
@"
            FixResourceKeyStrings(node);
"@
            })


            if (node.Name == "ResourceDictionary.ThemeDictionaries")
            {
                // This will be a list of either ResourceDictionaries or comments.
                // We'll figure out what the ResourceDictionaries' keys are,
                // then either add their contents to the existing theme dictionaries we have,
                // or create new ones if we've found new keys.
                foreach (XmlNode childNode in node.ChildNodes)
                {
                    string nodeKey = GetKey(childNode);

                    if (nodeKey == null || nodeKey.Length == 0)
                    {
                        continue;
                    }
                    else if (nodeKey == "Dark")
                    {
                        // If we don't specify the dictionary "Dark", then "Default" will be used instead.
                        // Having both of those, however, will result in "Default" never being used, even
                        // if it contains something that "Dark" does not.
                        // Since we're merging everything into a single dictionary, we need to standardize
                        // to only one of the two. Since most dictionaries use "Default", we'll go with that.
                        nodeKey = "Default";
                    }

                    MergedDictionary mergedThemeDictionary = null;

                    if (mergedThemeDictionaryByKeyDictionary.TryGetValue(nodeKey, out mergedThemeDictionary) == false)
                    {
                        mergedThemeDictionary = new MergedDictionary(owningDocument, this);
                        mergedThemeDictionaryByKeyDictionary.Add(nodeKey, mergedThemeDictionary);
                    }

                    foreach (XmlNode resourceDictionaryChild in childNode.ChildNodes)
                    {
                        mergedThemeDictionary.AddNode(resourceDictionaryChild, xmlnsReplacementDictionary);
                    }
                }
            }
            else
            {
                // First, we need to check if this is a node with a key.  If it is, then we'll replace
                // the previous node we saw with this key, in order to have only one entry per key.
                // if it's not, or if we haven't seen a previous node with this key, then we'll just
                // add it to our list.
                string nodeKey = GetKey(node);

                $(if ($DependencyHandling -eq "Reorder")
                {
@"
                // Next, we need to check what dependencies this node has.  If we do, and if it has any dependencies
                // that don't yet exist within our node list, then we'll want to add a placeholder for that dependency's key name
                // in our node list, which will be overwritten with the contents of the dependency once we find it.
                foreach (string dependencyKey in GetDependencies(node))
                {
                    if (!IsDependencyBeforeNode(nodeKey, dependencyKey))
                    {
                        if (nodeKey.Length > 0 && nodeKeyToNodeListIndexDictionary.ContainsKey(nodeKey))
                        {
                            int insertionPosition = nodeKeyToNodeListIndexDictionary[nodeKey];
                            int previousDependencyPosition = -1;

                            // If the dependency key already exists in the dictionary, then we'll bump it up in the list.
                            if (nodeKeyToNodeListIndexDictionary.TryGetValue(dependencyKey, out previousDependencyPosition))
                            {
                                if (!nodeListNodesToIgnore.Contains(previousDependencyPosition))
                                {
                                    nodeListNodesToIgnore.Add(previousDependencyPosition);
                                }

                                nodeKeyToNodeListIndexDictionary[dependencyKey] = insertionPosition;
                                nodeList.Insert(insertionPosition, nodeList[previousDependencyPosition]);
                            }
                            else
                            {
                                nodeKeyToNodeListIndexDictionary.Add(dependencyKey, insertionPosition);
                                nodeList.Insert(insertionPosition, null);
                            }

                            // We've had to insert a node in the middle of nodeList, so now we need to
                            // bump up all the indexes higher than its insertion position by one.
                            List<KeyValuePair<string, int>> keysToReplace = new List<KeyValuePair<string, int>>();

                            foreach (string key in nodeKeyToNodeListIndexDictionary.Keys)
                            {
                                if (key != dependencyKey && nodeKeyToNodeListIndexDictionary[key] >= insertionPosition)
                                {
                                    // We can't modify a list we're iterating over, so we'll store changes we want to make and then
                                    // make those changes later.
                                    keysToReplace.Add(new KeyValuePair<string, int>(key, nodeKeyToNodeListIndexDictionary[key] + 1));
                                }
                            }

                            foreach (KeyValuePair<string, int> keyToReplace in keysToReplace)
                            {
                                nodeKeyToNodeListIndexDictionary[keyToReplace.Key] = keyToReplace.Value;
                            }

                            for (int i = nodeListNodesToIgnore.Count - 1; i >= 0; i--)
                            {
                                int nodeIndex = nodeListNodesToIgnore[i];

                                if (nodeIndex > insertionPosition)
                                {
                                    nodeListNodesToIgnore.RemoveAt(i);
                                    nodeListNodesToIgnore.Insert(i, nodeIndex + 1);
                                }
                            }
                        }
                        else
                        {
                            int insertionPosition = nodeList.Count;
                            int previousDependencyPosition = -1;

                            // If the dependency key already exists in the dictionary, then we'll bump it up in the list.
                            if (nodeKeyToNodeListIndexDictionary.TryGetValue(dependencyKey, out previousDependencyPosition))
                            {
                                if (!nodeListNodesToIgnore.Contains(previousDependencyPosition))
                                {
                                    nodeListNodesToIgnore.Add(previousDependencyPosition);
                                }

                                nodeKeyToNodeListIndexDictionary[dependencyKey] = insertionPosition;
                                nodeList.Insert(insertionPosition, nodeList[previousDependencyPosition]);
                            }
                            else
                            {
                                nodeKeyToNodeListIndexDictionary.Add(dependencyKey, insertionPosition);
                                nodeList.Insert(insertionPosition, null);
                            }
                        }
                    }
                }
"@
                }
                elseif ($DependencyHandling -eq "Validate")
                {
@"
                // Next, we need to validate the dependencies this node has.  If find a dependency that doesn't exist
                // prior to the node that we're currently adding, then we'll throw an exception.
                foreach (string dependencyKey in GetDependencies(node))
                {
                    if (!IsDependencyBeforeNode(nodeKey, dependencyKey))
                    {
                        throw new XamlParseException(string.Format("Dependency '{0}' not found prior to the key '{1}'", dependencyKey, nodeKey));
                    }
                }
"@
                })

                if (nodeKey.Length == 0 || !nodeKeyToNodeListIndexDictionary.ContainsKey(nodeKey))
                {
                    if (nodeKey.Length != 0)
                    {
                        nodeKeyToNodeListIndexDictionary.Add(nodeKey, nodeList.Count);
                    }

                    nodeList.Add(node);
                }
                else
                {
                    int previousNodeIndex = nodeKeyToNodeListIndexDictionary[nodeKey];
                    nodeList[previousNodeIndex] = node;
                }

                if (nodeKey.Length > 0 && parentDictionary != null)
                {
                    parentDictionary.RemoveAncestorNodesWithKey(nodeKey);
                }

                if (nodeKey.Length > 0 && parentDictionary != null)
                {
                    parentDictionary.RemoveAncestorNodesWithKey(nodeKey);
                }
            }
        }

        public void MarkEndOfBaseFile()
        {
            nodeList.Add(owningDocument.CreateComment("$BeginMergedXamlSectionComment"));

            foreach (MergedXamlGeneration.MergedDictionary mergedDictionary in mergedThemeDictionaryByKeyDictionary.Values)
            {
                mergedDictionary.MarkEndOfBaseFile();
            }
        }

        public void MarkEndOfMergedFiles()
        {
            nodeList.Add(owningDocument.CreateComment("$EndMergedXamlSectionComment"));

            foreach (MergedXamlGeneration.MergedDictionary mergedDictionary in mergedThemeDictionaryByKeyDictionary.Values)
            {
                mergedDictionary.MarkEndOfMergedFiles();
            }
        }

        public XmlNode GetXaml()
        {
            if (mergedThemeDictionaryByKeyDictionary.Keys.Count > 0)
            {
                XmlElement themeDictionariesElement = owningDocument.CreateElement("ResourceDictionary.ThemeDictionaries", "http://schemas.microsoft.com/winfx/2006/xaml/presentation");
                xmlElement.AppendChild(themeDictionariesElement);

                foreach (string themeDictionaryKey in mergedThemeDictionaryByKeyDictionary.Keys)
                {
                    XmlElement themeResourceDictionaryElement = mergedThemeDictionaryByKeyDictionary[themeDictionaryKey].GetXaml() as XmlElement;
                    themeDictionariesElement.AppendChild(themeResourceDictionaryElement);
                    themeResourceDictionaryElement.SetAttribute("Key", "http://schemas.microsoft.com/winfx/2006/xaml", themeDictionaryKey);
                }
            }

            for (int i = 0; i < nodeList.Count; i++)
            {
                if (!nodeListNodesToIgnore.Contains(i))
                {
                    // We may have null placeholders left over that were never found.  If there are,
                    // that's not necessarily an indication that something's wrong - for example,
                    // we might have nodes that are part of the built-in list of theme resources.
                    // We'll just ignore null nodes.
                    XmlNode node = nodeList[i];

                    if (node != null)
                    {
                        xmlElement.AppendChild(node);
                    }
                }
            }

            if (parentDictionary == null)
            {
                return owningDocument;
            }
            else
            {
                return xmlElement;
            }
        }

        private static void ReplaceNamespacePrefix(XmlNode currentNode, Dictionary<string, string> xmlnsReplacementDictionary)
        {
            if (currentNode.Prefix != null && currentNode.Prefix.Length > 0)
            {
                if (xmlnsReplacementDictionary.ContainsKey(currentNode.Prefix))
                {
                    currentNode.Prefix = xmlnsReplacementDictionary[currentNode.Prefix];
                }
            }

            // We also want to check the node's attributes for any of the prefixes -
            // e.g. Style.TargetType may contain an instance of "local:..."
            if (currentNode.Attributes != null)
            {
                foreach (XmlAttribute attribute in currentNode.Attributes)
                {
                    if (attribute.Value != null)
                    {
                        foreach (string xmlnsToReplace in xmlnsReplacementDictionary.Keys)
                        {
                            attribute.Value = attribute.Value.Replace(xmlnsToReplace + ":", xmlnsReplacementDictionary[xmlnsToReplace] + ":");
                        }
                    }
                }
            }

            foreach (XmlNode node in currentNode.ChildNodes)
            {
                ReplaceNamespacePrefix(node, xmlnsReplacementDictionary);
            }
        }
        
        private static string GetKey(XmlNode node)
        {
            if (node.Attributes == null)
            {
                return string.Empty;
            }

            string key = string.Empty;

            foreach (XmlAttribute attribute in node.Attributes)
            {
                if (attribute.Name == "x:Key" || attribute.Name == "x:Name")
                {
                    key = attribute.Value;
                    break;
                }
            }

            // If we didn't find an "x:Key" or "x:Name" attribute, then try looking for a "TargetType" attribute
            // if this is a "Style" tag - that functions in the same way.
            if (key.Length == 0 && node.Name == "Style" && node.Attributes != null)
            {
                foreach (XmlAttribute attribute in node.Attributes)
                {
                    if (attribute.Name == "TargetType")
                    {
                        key = attribute.Value;
                        break;
                    }
                }
            }

            if (key.Length > 0)
            {
                // If this node has a key and a conditional-inclusion namespace, we'll attach a prefix
                // to the key corresponding to the condition we checked in order to allow multiple such nodes
                // with the same key to exist.
                int indexOfContractPresent = node.NamespaceURI.IndexOf("IsApiContract");

                if (indexOfContractPresent >= 0)
                {
                    key = node.NamespaceURI.Substring(indexOfContractPresent) + ":" + key;
                }
            }

            return key;
        }
        
        private static Regex dependenciesRegex = new Regex("{(?:StaticResource|ThemeResource)\\s+(\\w+)}", RegexOptions.Compiled | RegexOptions.IgnoreCase);

        private static string ResourceKeyPrefixToRemove = "MUX_";

        private static void FixResourceKeyStrings(XmlNode node)
        {
            // If the resource key starts with the "MUX_" prefix we remove the prefix.
            if (node.Attributes != null)
            {
                foreach (XmlAttribute attribute in node.Attributes)
                {
                    if (attribute.Name == "x:Key" || attribute.Name == "x:Name")
                    {
                        if(attribute.Value.StartsWith(ResourceKeyPrefixToRemove))
                        {
                            attribute.Value = attribute.Value.Replace(ResourceKeyPrefixToRemove, "");
                        }
                    }
                }
            }

            // Also fix up any references to resources with keys starting with "MUX_";
            FixResourceKeyReferences(node);
        }

        private static void FixResourceKeyReferences(XmlNode currentNode)
        {
            if (currentNode.Attributes != null)
            {
                foreach (XmlAttribute attribute in currentNode.Attributes)
                {
                    if (attribute.Value != null)
                    {
                        foreach (Match match in dependenciesRegex.Matches(attribute.Value))
                        {
                            var dependency = match.Groups[1].Captures[0].Value;
                            if(dependency.StartsWith(ResourceKeyPrefixToRemove))
                            {
                                attribute.Value = attribute.Value.Replace(ResourceKeyPrefixToRemove, "");
                            }
                        }
                    }
                }
            }

            foreach (XmlNode node in currentNode.ChildNodes)
            {
                FixResourceKeyReferences(node);
            }
        }

        private static IEnumerable<string> GetDependencies(XmlNode node)
        {
            foreach (Match match in dependenciesRegex.Matches(node.OuterXml))
            {
                yield return match.Groups[1].Captures[0].Value;
            }

            if (node.Name == "StaticResource" && node.Attributes != null)
            {
                foreach (XmlAttribute attribute in node.Attributes)
                {
                    if (attribute.Name == "ResourceKey")
                    {
                        yield return attribute.Value;
                        break;
                    }
                }
            }
        }

        private bool IsDependencyBeforeNode(string nodeKey, string dependencyKey)
        {
            // If one of the merged theme dictionaries contains this key at all,
            // then it'll always appear before this node.
            foreach (MergedXamlGeneration.MergedDictionary mergedDictionary in mergedThemeDictionaryByKeyDictionary.Values)
            {
                if (mergedDictionary.IsDependencyFound(dependencyKey))
                {
                    return true;
                }
            }

            // Otherwise, we'll check the relative positions of the two keys -
            // if both exist and if the dependency key occurs before the node key,
            // then we're good.
            return
                nodeKeyToNodeListIndexDictionary.ContainsKey(dependencyKey) && 
                nodeKeyToNodeListIndexDictionary.ContainsKey(nodeKey) &&
                nodeKeyToNodeListIndexDictionary[dependencyKey] < nodeKeyToNodeListIndexDictionary[nodeKey];
        }

        private bool IsDependencyFound(string key)
        {
            if (nodeKeyToNodeListIndexDictionary.ContainsKey(key))
            {
                return true;
            }
            else
            {
                foreach (MergedXamlGeneration.MergedDictionary mergedDictionary in mergedThemeDictionaryByKeyDictionary.Values)
                {
                    if (mergedDictionary.IsDependencyFound(key))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        private void RemoveAncestorNodesWithKey(string key)
        {
            if (nodeKeyToNodeListIndexDictionary.ContainsKey(key))
            {
                nodeListNodesToIgnore.Add(nodeKeyToNodeListIndexDictionary[key]);
            }

            if (parentDictionary != null)
            {
                parentDictionary.RemoveAncestorNodesWithKey(key);
            }
        }

        private XmlElement xmlElement;
        private XmlDocument owningDocument;
        private List<XmlNode> nodeList;

        // We'll want to remove nodes from the node list if a child dictionary has the same node,
        // but if we just removed the nodes then the values in nodeKeyToNodeListIndexDictionary would be wrong.
        // To make things easier, we'll instead just mark those nodes as ones to ignore rather than actually
        // removing them from the list.
        private List<int> nodeListNodesToIgnore;

        private Dictionary<string, int> nodeKeyToNodeListIndexDictionary;
        private Dictionary<string, MergedXamlGeneration.MergedDictionary> mergedThemeDictionaryByKeyDictionary;
        private List<string> namespaceList;
        private MergedXamlGeneration.MergedDictionary parentDictionary;
    }
}
"@

$global:conditionalContractRegex = New-Object System.Text.RegularExpressions.Regex -ArgumentList "xmlns:(\w+)\s*=\s*`"\s*.*?\?\s*(IsApiContract(?:Not|)Present)\(([a-zA-Z0-9\.]+),\s*([0-9]+)\)",Compiled
$global:usingsToParseRegex = New-Object System.Text.RegularExpressions.Regex -ArgumentList "xmlns:(\w+)\s*=\s*`"\s*using:\s*((?:Microsoft\.UI\.[a-zA-Z0-9\.]+|Windows.UI.[a-zA-Z0-9\.]+))",Compiled
$global:namespacesToRemoveRegex = New-Object System.Text.RegularExpressions.Regex -ArgumentList "(\s*using:\s*Microsoft\.UI\.[a-zA-Z0-9\.]+|http://schemas\.microsoft\.com/winfx/2006/xaml/presentation\?IsApiContract(?:Not|)Present\([a-zA-Z0-9\.]+,[0-9]+\))",Compiled

function Parse-Usings
{
    param(
        [string]$xamlFileContent,
        [System.Collections.Generic.Dictionary[string, string]]$xmlnsReplacementDictionary,
        [MergedXamlGeneration.MergedDictionary]$mergedResourceDictionary
    )

    $global:conditionalContractRegex.Matches($xamlFileContent) | ForEach-Object {
        $attributeLocalName = $_.Groups[1].Captures[0].Value
        $booleanCheck = $_.Groups[2].Captures[0].Value
        $contractName = $_.Groups[3].Captures[0].Value
        $contractVersion = $_.Groups[4].Captures[0].Value
        $namespace = "http://schemas.microsoft.com/winfx/2006/xaml/presentation?$($booleanCheck)($($contractName),$($contractVersion))"
        $xmlns = "contract$($contractVersion)$(if ($booleanCheck -ilike "*not*") { "Not" } else { })Present"

        $mergedResourceDictionary.AddNamespace($xmlns, $namespace)

        if (-not $xmlnsReplacementDictionary.ContainsKey($attributeLocalName))
        {
            $xmlnsReplacementDictionary.Add($attributeLocalName, $xmlns)
        }
        else
        {
            $xmlnsReplacementDictionary[$attributeLocalName] = $xmlns
        }
    }

    $global:usingsToParseRegex.Matches($xamlFileContent) | ForEach-Object {
        $attributeLocalName = $_.Groups[1].Captures[0].Value
        $using = $_.Groups[2].Captures[0].Value
        $namespaceTokens = $using.Split(".")
        $xmlns = $namespaceTokens[$namespaceTokens.Count - 1].ToLower()
    
        $mergedResourceDictionary.AddNamespace($xmlns, "using:$using")

        if (-not $xmlnsReplacementDictionary.ContainsKey($attributeLocalName))
        {
            $xmlnsReplacementDictionary.Add($attributeLocalName, $xmlns)
        }
        else
        {
            $xmlnsReplacementDictionary[$attributeLocalName] = $xmlns
        }
    }
}

$xmlDocument = New-Object System.Xml.XmlDocument
$headerComment = $xmlDocument.CreateComment(@"

//
// Copyright (c) Microsoft Corporation. All Rights Reserved.
//

"@)
$xmlDocument.AppendChild($headerComment) 2>&1> $null

[MergedXamlGeneration.MergedDictionary]$mergedResourceDictionary = New-Object MergedXamlGeneration.MergedDictionary -ArgumentList $xmlDocument
[System.Collections.Generic.Dictionary[string, string]]$elementsToPreserveWhitespace = @{}

foreach ($xamlFile in $XamlFileList.Split(';'))
{
    if ($xamlFile.Length -eq 0)
    {
        continue
    }

    [string]$xamlFileContent = Get-Content $xamlFile -Raw

    # We need to make sure that every newline is of the form CR LF since some of our regular expressions depend on that,
    # so let's replace every newline with that to make sure that we've got the correct newline pattern.
    $xamlFileContent = $xamlFileContent -replace "\r\n","`n"
    $xamlFileContent = $xamlFileContent -replace "\r","`n"
    $xamlFileContent = $xamlFileContent -replace "\n","`r`n"

    # If this is our base file, then we'll want to remove the parts that came from the merged files, since we'll be re-merging
    # those files back in.
    if ($xamlFile -ilike $BaseXamlFile)
    {
        $xamlFileContent = $xamlFileContent -replace "(?smi)\s*<!--$BeginMergedXamlSectionComment-->.*?<!--$EndMergedXamlSectionComment-->",""
    }

    # The XmlWriter will "helpfully" escape characters in the form &#xE0E5 unless we escape the ampersand.
    $xamlFileContent = $xamlFileContent.Replace("&", "&amp;")
    
    # Whitespace between attributes is considered irrelevant when parsing XML, so in order to preserve it,
    # we need to do something external to our XML parsing and writing.  In order to achieve that,
    # we'll detect all of the elements that have nonstandard spaces between attributes and associate how they were
    # originally written with how they'll appear when we save our XML to a file.
    # Then, we'll replace every instance of the keys in this dictionary with their associated values
    # in order to return them back to their original formatting.
    # The easiest way to do this is to just parse the XAML in question, if it's a well-formed XAML snippet,
    # and see what the resulting generated XAML looks like.  If that doesn't work (e.g. if this is an open tag with no closing tag),
    # then we'll manually convert it to what the parsed XAML will look like.
    foreach ($match in ([regex]"( +)<\w+(?:\s+[\w:\.]+\s*=\s*(?:'[^']*'|`"[^`"]*`")\s*)*/?>").Matches($xamlFileContent))
    {
        # We only care about matches that actually contain newlines in them, so ignore any that don't.
        if (-not $match.Value.Contains("`n"))
        {
            continue
        }

        try
        {
            $xmlStringReader = New-Object System.IO.StringReader -ArgumentList $match.Value
            $xmlReader = New-Object System.Xml.XmlTextReader -ArgumentList $xmlStringReader
            $xmlReader.Namespaces = $false # We don't care if there are unresolved namespaces - we're just interested in the text.
            $xmlDocument = New-Object System.Xml.XmlDocument
            $xmlDocument.Load($xmlReader)

            $xmlWriterSettings = New-Object System.Xml.XmlWriterSettings
            $xmlWriterSettings.ConformanceLevel = [System.Xml.ConformanceLevel]::Fragment
            $xmlWriterSettings.OmitXmlDeclaration = $true

            $xmlStringWriter = New-Object System.IO.StringWriter
            $xmlWriter = [System.Xml.XmlWriter]::Create($xmlStringWriter, $xmlWriterSettings)
            $xmlDocument.Save($xmlWriter)
            $parsedXaml = $xmlStringWriter.ToString()
        }
        catch [System.Management.Automation.MethodInvocationException]
        {
            $parsedXaml = $match.Value -replace "/>"," />"
            $parsedXaml = $parsedXaml -replace "\s+"," "
            $parsedXaml = $parsedXaml -replace " ?= ?","="
            $parsedXaml = $parsedXaml -replace " >",">"
            $parsedXaml = $parsedXaml -replace " <","<"

            # The XML writer will also turn apostrophes surrounding attributes into quotation marks, so we need to account for that, too.
            $parsedXaml = $parsedXaml -replace "'([^']*)'","`"`$1`""
        }

        # We can sometimes run into the situation where the same text appears with multiple levels of indentation,
        # so we prepend the whitespace before the matched value in order to differentiate between those cases.
        $parsedXamlWithWhitespace = $match.Groups[1].Captures[0].Value + $parsedXaml

        if ($parsedXaml -ne $match.Value -and -not $elementsToPreserveWhitespace.ContainsKey($parsedXamlWithWhitespace))
        {
            $elementsToPreserveWhitespace.Add($parsedXamlWithWhitespace, $match.Value)
        }
    }

    # We also want to preserve newlines before elements where there are more than one,
    # so we'll save the whitespace before any elements and restore it in post-processing.
    foreach ($match in ([regex]"(?m)\s*\n\s*\n\s*?( *)(<[^>]*?>)").Matches($xamlFileContent))
    {
        # We can sometimes run into the situation where the same text appears with multiple levels of indentation,
        # so we prepend the whitespace before the matched value in order to differentiate between those cases.
        $parsedXamlWithWhitespace = [Environment]::NewLine + $match.Groups[1].Captures[0].Value + $match.Groups[2].Captures[0].Value

        if ($parsedXaml -ne $match.Value -and -not $elementsToPreserveWhitespace.ContainsKey($parsedXamlWithWhitespace))
        {
            $elementsToPreserveWhitespace.Add($parsedXamlWithWhitespace, $match.Value)
        }
    }

    # If we want to remove usings, we'll parse the topmost element of the document to find out what the usings are,
    # then remove them from the XAML content before fully parsing it.  We have to do it this way because namespaces
    # are locked in place once we parse XML into XmlNode objects.
    [System.Collections.Generic.List[string]]$prefixesToRemove = @()
    [System.Collections.Generic.List[string]]$namespacesToRemove = @()

    if ($RemoveUsings)
    {
        $resourceDictionaryNode = ([xml]$xamlFileContent).ResourceDictionary
        $resourceDictionaryNode.Attributes | ForEach-Object {
            $attribute = $_

            $global:namespacesToRemoveRegex.Matches($_.Value) | ForEach-Object {
                $prefixesToRemove.Add($attribute.LocalName)
                $namespacesToRemove.Add($_.Groups[1].Captures[0].Value)
            }
        }
    }

    foreach ($prefixToRemove in $prefixesToRemove)
    {
        if ($prefixToRemove -like "*NotPresent*")
        {
            # Remove the entire tag
            $xamlFileContent = $xamlFileContent -replace "<$prefixToRemove[^>]*/>",""
            $xamlFileContent = $xamlFileContent -replace "<$prefixToRemove[.\S\s]*?</$prefixToRemove\:[^>]*>","" 

            #   <Rectangle x:Name="colorRectangle" Width="200" Height="200"
            #      contract5NotPresent:Fill="{x:Bind ((SolidColorBrush)((FrameworkElement)colorComboBox.SelectedItem).Tag), Mode=OneWay}">
            #
            $xamlFileContent = $xamlFileContent -replace "(?m)\s$prefixToRemove\:\w+?=`"[^`"]+`"", ""
        }
        else
        {
            # Remove only the prefix
            $xamlFileContent = $xamlFileContent -replace ($prefixToRemove + ":"),""
        }
    }

    foreach ($namespaceToRemove in $namespacesToRemove)
    {
        $escapedNamespace = $namespaceToRemove.Replace("?", "\?").Replace("(", "\(").Replace(")", "\)")
        $xamlFileContent = $xamlFileContent -replace "\s*xmlns:\w+\s*=\s*`"\s*$escapedNamespace\s*`"",""
    }

    # Before we merge this XAML, we need to standardize its namespace usings.
    # We'll detect its usings and replace those with lowercase versions of the
    # leaf namespace component (e.g. "Microsoft.UI.Xaml.Controls" -> "controls")
    [System.Collections.Generic.Dictionary[string, string]]$xmlnsReplacementDictionary = @{}
    Parse-Usings $xamlFileContent $xmlnsReplacementDictionary $mergedResourceDictionary

    $resourceDictionaryNode = ([xml]$xamlFileContent).ResourceDictionary
    
    $resourceDictionaryNode.ChildNodes | ForEach-Object {
        $mergedResourceDictionary.AddNode($_, $xmlnsReplacementDictionary)
    }

    if ($xamlFile -ilike $BaseXamlFile)
    {
        $mergedResourceDictionary.MarkEndOfBaseFile()
    }
}

if ($BaseXamlFile.Length -gt 0)
{
    $mergedResourceDictionary.MarkEndOfMergedFiles()
}

$xmlWriterSettings = New-Object System.Xml.XmlWriterSettings
$xmlWriterSettings.OmitXmlDeclaration = $true
$xmlWriterSettings.Indent = $true
$xmlWriterSettings.IndentChars = "    "
$xmlWriterSettings.NewLineChars = [Environment]::NewLine
$xmlWriterSettings.NewLineHandling = [System.Xml.NewLineHandling]::Replace
$xmlWriterSettings.Encoding = [System.Text.Encoding]::UTF8

[System.Xml.XmlWriter]$xmlWriter = [System.Xml.XmlWriter]::Create($MergedXamlFilePath, $xmlWriterSettings)
[System.Xml.XmlDocument]($mergedResourceDictionary.GetXaml()).Save($xmlWriter)
$xmlWriter.Close()

$mergedXaml = Get-Content $MergedXamlFilePath -Raw

foreach ($elementWithoutExtraWhitespace in $elementsToPreserveWhitespace.Keys)
{
    $mergedXaml = $mergedXaml.Replace($elementWithoutExtraWhitespace, $elementsToPreserveWhitespace[$elementWithoutExtraWhitespace])
}

# Now we'll unescape our ampersands, now that we've written the XML to disk.
$mergedXaml = $mergedXaml.Replace("&amp;", "&")

# If we have a merged section in this file, we'll prepend a couple newlines before it to make it more clearly demarcated from the rest of the file.
if ($BaseXamlFile.Length -gt 0)
{
    $mergedXaml = $mergedXaml -replace "( *<!--$BeginMergedXamlSectionComment-->)","$([Environment]::NewLine + [Environment]::NewLine)`$1"
}

Set-Content $MergedXamlFilePath $mergedXaml -NoNewline -Encoding UTF8