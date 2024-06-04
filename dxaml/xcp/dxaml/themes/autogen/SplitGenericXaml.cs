// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Linq;

namespace SplitGenericXaml
{
    class Program
    {
        static string _xamlFragmentNamePostfix;

        static string[] _knownNamespaceAliases = {
                    "controls",
                    "primitives",
                    "animation",
                    "maps",
                    "inking"
                };

        static string MakeXamlFragmentName(string targetType)
        {
            string correctedTargetType = targetType;

            foreach (var alias in _knownNamespaceAliases)
            {
                if (targetType.StartsWith(alias + ":"))
                {
                    correctedTargetType = targetType.Substring(alias.Length + 1);
                    break;
                }
            }

            return correctedTargetType + _xamlFragmentNamePostfix;
        }

        public static class ReturnCodes
        {
            public const int IncorrectArgs = 0x1;
            public const int InvalidPath = 0x2;
            public const int FileNotExist = 0x3;
            public const int InvalidASCIIinGeneric = 0x4;
            public const int FileReadOnly = 0x5;
            public const int ThemeResourceError = 0x6;
            public const int InvalidXml = 0x6;
        }

        public static string[] ReturnCodeMeaning =
        {
            "Success",
            "Incorrect number of arguments",
            "Path is invalid",
            "Invalid ASCII characters in generic.xaml",
            "File does not exist",
            "File is read-only",
            "Could not extract ThemeResources from generic.xaml",
            "Invalid XML format"
        };

        static void Main(string[] args)
        {

#if DEBUG
            // On debug builds of SplitGenericXaml.exe, you can set the environemnt variable DEBUG_SPLITGENERIC to '1'
            // and the exe will hold until a debugger attaches.
            CheckForDebugger();
#endif
            if (!(args.Length == 2))
            {
                Console.Error.WriteLine("Error: Incorrect Usage. Should be: SplitGenericXaml.exe [path-to-generic.xaml] [output-path]");
                Environment.Exit(ReturnCodes.IncorrectArgs);
            }

            if ((args.Length == 1 && (args[0] == "/?" || args[0] == "-?")))
            {
                Console.WriteLine("Usage: SplitGenericXaml.exe [path-to-generic.xaml] [output-path]");
                Console.WriteLine();
                Console.WriteLine(String.Format(@"Possible Error Codes:
                0x{0}: {1} 
                0x{2}: {3}
                0x{4}: {5}
                0x{6}: {7}
                0x{8}: {9} 
                0x{10} : {11}",
                ReturnCodes.IncorrectArgs, ReturnCodeMeaning[ReturnCodes.IncorrectArgs],
                ReturnCodes.InvalidPath, ReturnCodeMeaning[ReturnCodes.InvalidPath],
                ReturnCodes.InvalidASCIIinGeneric, ReturnCodeMeaning[ReturnCodes.InvalidASCIIinGeneric],
                ReturnCodes.FileReadOnly, ReturnCodeMeaning[ReturnCodes.FileReadOnly],
                ReturnCodes.ThemeResourceError, ReturnCodeMeaning[ReturnCodes.ThemeResourceError],
                ReturnCodes.InvalidXml, ReturnCodeMeaning[ReturnCodes.InvalidXml]));
                Environment.Exit(0);  
            }

            string genericXamlPath = GetPath(args[0], true);
            string outputPath = GetPath(args[1]);

            var genericFileName = Path.GetFileName(genericXamlPath);
            var newGenericXamlPath = Path.Combine(outputPath, genericFileName);

            // We need to know the parent directory for placing the generic.xaml and themeresources.xaml files for publishing. 
            var previousGenericXamlPath = Path.Combine(Directory.GetParent(outputPath).FullName, genericFileName);

            ValidateGenericXaml(genericXamlPath);

            // ".generic.xaml" or ".generic.highcontrast.xaml"
            _xamlFragmentNamePostfix = "." + genericFileName;

            try
           {
                var document = XDocument.Load(genericXamlPath, LoadOptions.SetLineInfo);

                var styles = ExtractStylesWithTargetType(document);

                try
                {
                    var styleFileName = "Styles" + (genericFileName.StartsWith("generic") ? genericFileName.Substring("generic".Length) : "." + genericFileName);
                    var styleFile = Path.Combine(Path.GetDirectoryName(newGenericXamlPath), styleFileName);

                    WriteStyles(styleFile, document, styles);
                    Console.WriteLine("Wrote '{0}'", styleFile);
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                    Console.WriteLine("Could not extract and/or generate styles for {0}", genericXamlPath);
                    Environment.Exit(ReturnCodes.ThemeResourceError);
                }

                try
                {
                    var themes = GetThemeNames(document);
                    var themeResources = new List<Tuple<string, Dictionary<string, XElement>>>();

                    foreach (string theme in themes)
                    {
                        themeResources.Add(new Tuple<string, Dictionary<string, XElement>>(theme, ExtractThemeResources(document, theme)));
                    }

                    var themeResourceFileName = "ThemeResources" + (genericFileName.StartsWith("generic") ? genericFileName.Substring("generic".Length) : "." + genericFileName);
                    var themeResourcesPath = Path.Combine(Path.GetDirectoryName(newGenericXamlPath), themeResourceFileName);
                    WriteThemeResources(themeResourcesPath, document, themeResources);

                    Console.WriteLine("Wrote '{0}'", themeResourcesPath);

                    // We need to change the extension of theme resources since we will get multi-write errors due to SplitGenericXaml.exe
                    // writing to it in PASS0 and the xaml compiler writing to it in PASS1. We copy it here so we can publish it.
                    var genThemeResources = Path.Combine(Path.GetDirectoryName(previousGenericXamlPath), themeResourceFileName);
                    File.Copy(themeResourcesPath, Path.ChangeExtension(genThemeResources, ".g.xaml"), true);
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                    Console.WriteLine("Could not extract and/or generate theme resources for {0}", genericXamlPath);
                    Environment.Exit(ReturnCodes.ThemeResourceError);
                }
                
                CopyGenericXaml(genericXamlPath, previousGenericXamlPath);
            }
            catch (Exception e)
            {
                Console.WriteLine("XML syntax is invalid");
                Console.WriteLine(e.Message);
                Environment.Exit(ReturnCodes.InvalidXml);
            } 
        }

        private static string[] GetThemeNames(XDocument document)
        {
            var ns = document.Root.GetDefaultNamespace();
            var xns = document.Root.GetNamespaceOfPrefix("x");

            var themeRoot = document.Descendants(ns.GetName("ResourceDictionary.ThemeDictionaries")).FirstOrDefault();

            var themes = from XElement theme in themeRoot.Descendants(ns.GetName("ResourceDictionary")) select theme.Attribute(xns.GetName("Key")).Value;

            return themes.ToArray();
        }

        private static void WriteThemeResources(
            string filePath,
            XDocument originalDoc,
            List<Tuple<string, Dictionary<string, XElement>>> themeResources)
        {
            var ns = originalDoc.Root.GetDefaultNamespace();
            var xns = originalDoc.Root.GetNamespaceOfPrefix("x");

            var newRoot = new XElement(MakeRootElement(originalDoc.Root));
            var themeDictionaries = new XElement(ns.GetName("ResourceDictionary.ThemeDictionaries"));
            newRoot.Add(themeDictionaries);

            foreach (var theme in themeResources)
            {
                var rd = new XElement(ns.GetName("ResourceDictionary"));
                rd.Add(new XAttribute(xns.GetName("Key"), theme.Item1));
                foreach (var res in theme.Item2.Values)
                    rd.Add(res);

                themeDictionaries.Add(rd);
            }

            WriteOtherStaticResources(originalDoc, newRoot);

            var styles = ExtractStylesWithXKey(originalDoc);

            // Styles can reference framework styles. So create a dictionary of 
            // framework style resources and add the styles to it.
            var styleResources = new Dictionary<string, XElement>();
            foreach (var style in styles)
            {
                styleResources.Add(style.Attribute(xns.GetName("Key")).Value, style);
            }

            // Find the style resources referenced by other styles so they can be written first
            var referencedResources = new List<XElement>();
            foreach (var style in styles)
            {
                referencedResources.AddRange(GetExplicitResourcesReferencedInStyle(style, styleResources));
            }

            // Write the referenced style resources
            foreach (var resource in OrderResourcesForSerialization(referencedResources.Distinct()))
            {
                newRoot.Add(new XElement(resource));
            }

            // Select styles that were never referenced and write them. Referenced styles
            // will have been written by the loop above, as part of static resources.
            var stylesToWrite = from el in styles
                                where !referencedResources.Contains(el)
                                select el;
            foreach (var style in stylesToWrite)
            {
                newRoot.Add(new XElement(style));
            }

            var newDocument = new XDocument(newRoot);

            newDocument.Save(filePath);
        }

        /// <summary>
        /// The built-in style ResourceDictionary can also contain non-Style resources used by the Styles. Extract and write them to the new doc.
        /// </summary>
        private static void WriteOtherStaticResources(XDocument originalDoc, XElement newRoot)
        {
            XElement rootResourceDictionary = originalDoc.Elements().FirstOrDefault();

            foreach (XElement child in rootResourceDictionary.Elements())
            {
                if (!IsStyleElement(child) && !IsResourceDictionary(child) && !IsThemeDictionaries(child))
                {
                    newRoot.Add(new XElement(child));
                }
            }
        }

        static string ExtractStaticResourceKey(string staticResourceExpression)
        {
            return Regex.Match(staticResourceExpression, @"{StaticResource\s+(\w+)}").Groups[1].Value;
        }

        private static IEnumerable<string> GetDirectStaticResourceReferencesInStyle(XElement style)
        {
            return from el in style.Descendants()
                   from at in el.Attributes()
                   where at.Value.Contains("StaticResource")
                   select ExtractStaticResourceKey(at.Value);
        }

        private static IEnumerable<XElement> GetExplicitResourcesReferencedInStyle(XElement style, Dictionary<string, XElement> explicitResources)
        {
            var ns = style.GetDefaultNamespace();
            var xns = style.GetNamespaceOfPrefix("x");

            var directStaticResourceReferences = GetDirectStaticResourceReferencesInStyle(style);

            // A style can reference another style, which in turn can reference other static resources
            var referencedStyles = from key in directStaticResourceReferences
                                   where explicitResources.ContainsKey(key) && explicitResources[key].Name == ns.GetName("Style")
                                   select explicitResources[key];


            var indirectStaticResourceReferences = from refStyle in referencedStyles
                                                   let resources = GetExplicitResourcesReferencedInStyle(refStyle, explicitResources)
                                                   from res in resources
                                                   select res.Attribute(xns.GetName("Key")).Value;

            var allStaticResourceReferences = directStaticResourceReferences.Concat(indirectStaticResourceReferences);
            var referencedResources = new List<XElement>();
            foreach (var key in allStaticResourceReferences)
            {
                if (!explicitResources.ContainsKey(key))
                {
                    // This happens when we're referencing nested styles. For example,
                    // a TextBox style contains a Grid that defines a Button style.
                    continue;
                }
                referencedResources.Add(explicitResources[key]);
            }

            return referencedResources.Distinct();
        }

        static IEnumerable<XElement> ExtractStylesWithXKey(XDocument document)
        {
            return from el in document.Root.Elements()
                   where IsStyleElement(el) && HasXKey(el)
                   select el;
        }

        static IEnumerable<XElement> ExtractStylesWithTargetType(XDocument document)
        {
            // Do not extract elements that have both an x:Key and a TargetType, since
            // they can be requested without creating elements of their TargetType
            return from el in document.Root.Elements()
                   where IsStyleElement(el) && HasTargetType(el) && !HasXKey(el)
                   select el;
        }

        private static Dictionary<string, XElement> ExtractThemeResources(XDocument document, string themeKey)
        {
            var ns = document.Root.GetDefaultNamespace();
            var xns = document.Root.GetNamespaceOfPrefix("x");

            var themeDictionaries = document.Root.Element(ns.GetName("ResourceDictionary.ThemeDictionaries"));
            var themeDictionary = (from el in themeDictionaries.Elements()
                                   where el.Attribute(xns.GetName("Key")).Value == themeKey
                                   select el).First();
            var staticResourceElements = from el in themeDictionary.Elements()
                                         where el.Attribute(xns.GetName("Key")) != null
                                         select el;

            var explicitResources = new Dictionary<string, XElement>();
            foreach (var el in staticResourceElements)
            {
                var key = el.Attribute(xns.GetName("Key"));
                explicitResources.Add(key.Value, el);
            }

            return explicitResources;
        }

        private static void WriteStyle(XElement style, string styleFile)
        {
            var newDocument = new XDocument(MakeRootElement(style));

            newDocument.Root.Add(new XElement(style));

            newDocument.Save(styleFile);
        }

        private static void WriteStyles(string styleFile, XDocument originalDoc, IEnumerable<XElement> styles)
        {
            var newRoot = new XElement(MakeRootElement(originalDoc.Root));

            foreach(var style in styles)
            {
                newRoot.Add(new XElement(style));
            }

            var newDocument = new XDocument(newRoot);

            newDocument.Save(styleFile);
        }

        private static XElement MakeRootElement(XElement baseElement)
        {
            var newRoot = new XElement(
                baseElement.Document.Root.Name,
                new XAttribute(XNamespace.Xmlns + "x", baseElement.GetNamespaceOfPrefix("x")));

            foreach (var alias in _knownNamespaceAliases)
            {
                if (baseElement.GetNamespaceOfPrefix(alias) != null)
                {
                    newRoot.Add(
                        new XAttribute(
                            XNamespace.Xmlns + alias,
                            baseElement.GetNamespaceOfPrefix(alias)));
                }
            }

            return newRoot;
        }

        static IEnumerable<XElement> OrderResourcesForSerialization(IEnumerable<XElement> resources)
        {
            // If resource A references resource B, resource B has to be serialized before A.
            return resources.OrderBy(resource => resource, new StaticResourceComparer());
        }

        static bool IsStyleElement(XElement element)
        {
            return element.Name == element.GetDefaultNamespace().GetName("Style");
        }

        static bool IsResourceDictionary(XElement element)
        {
            return element.Name == element.GetDefaultNamespace().GetName("ResourceDictionary");
        }

        static bool IsThemeDictionaries(XElement element)
        {
            return element.Name == element.GetDefaultNamespace().GetName("ResourceDictionary.ThemeDictionaries");
        }

        static bool HasXKey(XElement element)
        {
            var xns = element.GetNamespaceOfPrefix("x");
            return element.Attribute(xns.GetName("Key")) != null;
        }

        static bool HasTargetType(XElement element)
        {
            return element.Attribute("TargetType") != null;
        }

        static string GetXKey(XElement element)
        {
            var xns = element.GetNamespaceOfPrefix("x");
            return element.Attribute(xns.GetName("Key")).Value;
        }

        class StaticResourceComparer : IComparer<XElement>
        {
            public int Compare(XElement left, XElement right)
            {
                if (IsStyleElement(left) && HasXKey(right))
                {
                    var references = GetDirectStaticResourceReferencesInStyle(left);

                    // If left references right, left > right
                    if (references.Contains(GetXKey(right)))
                        return 1;
                }

                if (IsStyleElement(right) && HasXKey(left))
                {
                    var references = GetDirectStaticResourceReferencesInStyle(right);

                    // If right references left, right > left
                    if (references.Contains(GetXKey(left)))
                        return -1;
                }

                // Neither reference the other. They're equal.
                return 0;
            }
        }

        static void CheckForDebugger()
        {
            var debug = System.Environment.GetEnvironmentVariable("DEBUG_SPLITGENERIC");
            if (debug == "1")
            {
                while (!Debugger.IsAttached)
                {
                    System.Threading.Thread.Sleep(1000);
                }
                Debugger.Break();
            }
        }

        static string GetPath(string inPath, bool isPathToFile = false)
        {
            string path = null;
            try
            {
                path = Path.GetFullPath(inPath);
                if (isPathToFile && !File.Exists(inPath))
                {
                    Console.Error.WriteLine("Error: The file '{0}' does not exist.", inPath);
                    Environment.Exit(ReturnCodes.FileNotExist);
                }

            }
            catch (ArgumentException)
            {
                Console.Error.WriteLine("Error: The path '{0}' is invalid.", inPath);
                Environment.Exit(ReturnCodes.InvalidPath);
            }

            return path;

        }

        static void ValidateGenericXaml(string genericXamlPath)
        {
            // Validate that generic.xaml doesn't have any invalid ASCII characters
            var fileText = System.IO.File.ReadAllLines(genericXamlPath);
            for (var index = 0; index < fileText.Length; index++)
            {
                var match = Regex.Match(fileText[index], "[^\u0000-\u007F\"]");
                if (match.Success)
                {
                    Console.Error.WriteLine("Error: Invalid ASCII character found in {0} [{1}, {2}]", genericXamlPath, index + 1, match.Index + 1);
                    Environment.Exit(ReturnCodes.InvalidASCIIinGeneric);
                }
            }
        }

        static void CopyGenericXaml(string oldPath, string newPath)
        {
            var fileInfo = new FileInfo(newPath);
            if (fileInfo.Exists)
            {
                fileInfo.IsReadOnly = false;
            }

            File.Copy(oldPath, newPath, true);
        }
    }
}
