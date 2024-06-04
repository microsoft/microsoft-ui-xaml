using Microsoft.Build.Framework;
using Microsoft.VisualStudio.Utilities.Internal;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Xml;

namespace CustomTasks
{
    public class GenerateWinRTClassRegistrations : IsolatedTask
    {
        [Required]
        public ITaskItem[] WinMDs
        {
            get;
            set;
        }

        [Required]
        public string DefaultImplementationDll
        {
            get;
            set;
        }

        [Required]
        public ITaskItem[] Metadata_Dirs
        {
            get;
            set;
        }

        [Required]
        public string OutputFilename
        {
            get;
            set;
        }

        private readonly string[] platformDlls =
            {
                "Microsoft.UI.Xaml.dll",
                "Microsoft.UI.Xaml.Phone.dll",
                "Microsoft.UI.Xaml.Controls.dll",
                "WinUIEdit.dll",
                "Microsoft.Web.WebView2.Core.dll",
            };

        private bool IsPlatformDll(string dllName)
        {
            // If the DLL name is blank, it's the default.
            if (dllName.Length == 0)
            {
                dllName = DefaultImplementationDll;
            }

            foreach (string platformDll in platformDlls)
            {
                if (platformDll.Equals(dllName, StringComparison.InvariantCultureIgnoreCase))
                {
                    return true;
                }
            }

            return false;
        }

        private bool IsPublic(Type type)
        {
            return !type.Namespace.Contains("Microsoft.UI.Private");
        }

        // A type is activatable if it's both public and if it contains a parameterless constructor.
        private bool IsActivatable(Type type)
        {
            return IsPublic(type) &&
                type.GetConstructors().Where(ci => ci.GetParameters().Length == 0).Any() &&
                !type.IsAbstract &&
                !type.IsGenericType;
        }

        private string ToCppCxType(Type type)
        {
            switch (type.FullName)
            {
                case "System.Int32":
                    return "int";
                case "System.Single":
                    return "float";
                case "System.Double":
                    return "double";
                case "System.Boolean":
                    return "bool";
                case "System.Object":
                    return "Platform::Object^";
                default:
                    {
                        string cppCxType = type.FullName.Replace(".", "::");

                        // For types in ::Windows namespace, be specific, to avoid confusion
                        // with the ::Microsoft::Windows namespace
                        if(type.FullName.StartsWith("Windows."))
                        {
                            cppCxType = $"::{cppCxType}";
                        }

                        if (!type.IsValueType)
                        {
                            cppCxType = $"{cppCxType}^";
                        }

                        return cppCxType;
                    }
            }
        }

        public override bool ExecuteCore()
        {
            // Maps implementation DLLs to WinRT types
            Dictionary<string, List<string>> implementationDllToTypes = new Dictionary<string, List<string>>();

            // Maps winmds to implementation DLLs 
            Dictionary<Assembly, string> winmdToImplementationDll = new Dictionary<Assembly, string>();

            // Keep track of the types with constructors and types with static methods.
            List<Type> activatableTypes = new List<Type>();
            List<Type> staticTypes = new List<Type>();

            // Cache assemblies loaded into the ReflectionOnly context 
            List<Assembly> loadedAssemblies = new List<Assembly>();
            List<Assembly> unmergedWinMDs = new List<Assembly>();

            // Cache factory interfaces with a CreateInstance method
            List<string> factoryInterfaces = new List<string>();

            // Pre-load the unmerged Lifted XAML WinMDs
            foreach (ITaskItem winmd in WinMDs)
            {
                string winmdName = winmd.GetMetadata("Identity");
                var reflectionOnlyLoadedAssembly = Assembly.ReflectionOnlyLoadFrom(winmdName);
                unmergedWinMDs.Add(reflectionOnlyLoadedAssembly);
                loadedAssemblies.Add(reflectionOnlyLoadedAssembly);

                // If no 'ImplementationDll' is specified for this winmd, use 'DefaultImplementationDll' 
                winmdToImplementationDll[reflectionOnlyLoadedAssembly] = winmd.GetMetadata("ImplementationDll");
                if (winmdToImplementationDll[reflectionOnlyLoadedAssembly] == string.Empty)
                {
                    winmdToImplementationDll[reflectionOnlyLoadedAssembly] = DefaultImplementationDll;
                }
            }

            // Pre-load the dependency WinMDs
            foreach (ITaskItem item in Metadata_Dirs)
            {
                string metadata_dir = item.GetMetadata("Identity");
                var foundationWinmds = Directory.GetFiles(metadata_dir, "*.winmd");
                foreach (var winmd in foundationWinmds)
                {
                    loadedAssemblies.Add(Assembly.ReflectionOnlyLoadFrom(Path.GetFullPath(winmd)));
                }
            }

            // Hook up ReflectionOnly context dependency resolution callback for the .NET Framework
            AppDomain.CurrentDomain.ReflectionOnlyAssemblyResolve += (sender, resolveEventArgs) =>
            {
                return Assembly.ReflectionOnlyLoad(resolveEventArgs.Name);
            };

            // Hook up ReflectionOnly context depencency resolution callback for the Windows Runtime
            System.Runtime.InteropServices.WindowsRuntime.WindowsRuntimeMetadata.ReflectionOnlyNamespaceResolve += (sender, resolveEventArgs) =>
            {
                // Note that the correct way to do this is to use WindowsRuntimeMetadata.ResolveNamespace, but it doesn't
                // work for precisely the same reason that the AppX manifest fix-up is required: our namespaces do not 
                // match their containing winmd names.  Instead, the type universe is pre-loaded (winmds are loaded) and
                // passed in as resolved assemblies when the ReflectionOnly context depencency resolution callback is called.
                foreach (var winmd in loadedAssemblies)
                {
                    resolveEventArgs.ResolvedAssemblies.Add(winmd);
                }
            };

            // Build a list of all factory interfaces with a CreateInstance method.
            foreach (var unmergedWinMD in unmergedWinMDs)
            {
                var implementationDll = winmdToImplementationDll[unmergedWinMD];

                if (!implementationDllToTypes.ContainsKey(implementationDll))
                {
                    implementationDllToTypes.Add(implementationDll, new List<string>());
                }

                foreach (var type in unmergedWinMD.GetTypes())
                {
                    if (type.IsInterface)
                    {
                        var interfaceName = type.FullName;

                        if (interfaceName.EndsWith("Factory"))
                        {
                            // Microsoft.UI.Xaml.Media.Animation.Transition is a special case - it's technically activatable,
                            // but its CreateInstance method is in a private interface, meaning that it can only be activated internally
                            // by types that need to set it as their composable base - see dxaml/phone/lib/ThemeTransitions.cpp.
                            // Since it *is* a type that will be activated, however, we still need to add it to our activatable type table.
                            if (type.Name == "ITransitionFactory")
                            {
                                factoryInterfaces.Add(interfaceName);
                            }
                            else
                            {
                                foreach (var methodInfo in type.GetMethods())
                                {
                                    if (methodInfo.Name.StartsWith("CreateInstance"))
                                    {
                                        factoryInterfaces.Add(interfaceName);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Retrieve all activatable WinRT types from the unmerged WinMDs
            foreach (var unmergedWinMD in unmergedWinMDs)
            {
                var implementationDll = winmdToImplementationDll[unmergedWinMD];

                foreach (var unmergedWinMDType in unmergedWinMD.GetExportedTypes())
                {
                    bool unmergedWinMDTypeAdded = false;

                    // Types with a Windows.Foundation.Metadata.Activatable or Windows.Foundation.Metadata.Static attribute are declared activatable
                    foreach (var customAttributeData in unmergedWinMDType.GetTypeInfo().CustomAttributes)
                    {
                        var attributeTypeName = customAttributeData.AttributeType.FullName;

                        if (attributeTypeName == "Windows.Foundation.Metadata.ActivatableAttribute" ||
                            attributeTypeName == "Windows.Foundation.Metadata.StaticAttribute")
                        {
                            if (!unmergedWinMDTypeAdded)
                            {
                                implementationDllToTypes[implementationDll].Add(unmergedWinMDType.FullName);
                                unmergedWinMDTypeAdded = true;
                            }

                            if (IsPlatformDll(implementationDll))
                            {
                                if (attributeTypeName == "Windows.Foundation.Metadata.ActivatableAttribute" && IsActivatable(unmergedWinMDType) && !activatableTypes.Contains(unmergedWinMDType))
                                {
                                    activatableTypes.Add(unmergedWinMDType);
                                }
                                else if (attributeTypeName == "Windows.Foundation.Metadata.StaticAttribute" && IsPublic(unmergedWinMDType) && !staticTypes.Contains(unmergedWinMDType))
                                {
                                    staticTypes.Add(unmergedWinMDType);
                                }
                            }
                        }
                        else if (customAttributeData.AttributeType.FullName == "Windows.Foundation.Metadata.ComposableAttribute")
                        {
                            var factoryName = customAttributeData.ConstructorArguments[0].Value.ToString();

                            if (factoryInterfaces.Contains(factoryName))
                            {
                                if (!unmergedWinMDTypeAdded)
                                {
                                    implementationDllToTypes[implementationDll].Add(unmergedWinMDType.FullName);
                                    unmergedWinMDTypeAdded = true;
                                }

                                if (IsPlatformDll(implementationDll) && IsActivatable(unmergedWinMDType) && !activatableTypes.Contains(unmergedWinMDType))
                                {
                                    activatableTypes.Add(unmergedWinMDType);
                                }
                            }
                        }
                    }
                }
            }

            // Verify that we don't have instances of the same implementation DLL with different casing; this is an authoring
            // error that can lead to multiple sections in the manifest pointing to the same file (ERROR_SXS_DUPLICATE_DLL_NAME).
            // While we could compensate for this by performing case normalization, it's better to enforce consistent
            // casing at the source.
            {
                string errorMessage;
                bool isValidCasing = Utils.ValidateImplementationDllCasing(implementationDllToTypes.Keys, winmdToImplementationDll, out errorMessage);

                if (!isValidCasing)
                {
                    LogError(errorMessage);
                    return false;
                }
            }

            // Generate the AppXManifest registrations for WinRT activatable types, based on the winmd to implementation dll
            // mapping passed in as metata to the winmd items.
            XmlWriterSettings settings = new XmlWriterSettings();
            settings.Indent = true;
            settings.IndentChars = ("\t");
            settings.ConformanceLevel = ConformanceLevel.Fragment;
            XmlWriter xmlWriter = XmlWriter.Create(OutputFilename, settings);

            xmlWriter.WriteStartElement("Data", "http://schemas.microsoft.com/appx/manifest/foundation/windows10");

            foreach (var implementationDll in implementationDllToTypes.Keys)
            {
                var types = implementationDllToTypes[implementationDll];

                if (types.Count > 0)
                {
                    types.Sort();

                    xmlWriter.WriteStartElement("Extension");
                    xmlWriter.WriteAttributeString("Category", "windows.activatableClass.inProcessServer");

                    xmlWriter.WriteStartElement("InProcessServer");

                    xmlWriter.WriteStartElement("Path");
                    xmlWriter.WriteString(implementationDll);
                    xmlWriter.WriteEndElement();

                    foreach (var winRTType in types)
                    {
                        xmlWriter.WriteStartElement("ActivatableClass");
                        xmlWriter.WriteAttributeString("ActivatableClassId", winRTType);
                        xmlWriter.WriteAttributeString("ThreadingModel", "both");
                        xmlWriter.WriteEndElement();
                    }

                    xmlWriter.WriteEndElement();
                    xmlWriter.WriteEndElement();
                }
            }

            xmlWriter.WriteEndElement();
            xmlWriter.Close();
            
            // We also want to copy and paste the contents of this XML file into our MUX test app AppxManifest.xml files,
            // so we'll remove its top and bottom <Data> lines in order to make it copy and pasteable.
            List<string> fileLines = File.ReadLines(OutputFilename).ToList();
            fileLines.RemoveAt(0);
            fileLines.RemoveAt(fileLines.Count - 1);
            File.WriteAllLines(Path.Combine(Path.GetDirectoryName(OutputFilename), "TestExtensions.xml"), fileLines);

            // Finally, we'll output test cases to ensure that every activatable type actually is activatable.
            // There are a few types that we don't want to activate, though, which are called out below.
            string[] typesToSkip =
            {
                "Microsoft.UI.Xaml.Application", // We already have an application, and can't create another.
                "Microsoft.UI.Xaml.PanelEx", // Requires Feature_Xaml2018.
                "Microsoft.UI.Xaml.Controls.CommandingContainer", // Requires Feature_CommandingImprovements.
                "Microsoft.UI.Xaml.Controls.Primitives.PivotHeaderItem", // Not intended to be independently activatable.
                "Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSource", // Not activatable in a UWP context.
                "Microsoft.UI.Xaml.XamlIsland", // Not activatable in a UWP context.
                "Microsoft.UI.Xaml.Window" // more than one UWP window cannot be activated at once, the test app already creates one
            };
            activatableTypes = activatableTypes.Where(type => !typesToSkip.Contains(type.FullName)).ToList();

            int typeComparer(Type t1, Type t2)
            {
                int namespaceComparison = t1.Namespace.CompareTo(t2.Namespace);

                if (namespaceComparison != 0)
                {
                    return namespaceComparison;
                }
                else
                {
                    return t1.Name.CompareTo(t2.Name);
                }
            }

            activatableTypes.Sort(typeComparer);
            staticTypes.Sort(typeComparer);

            string activatableTypeTestCases = activatableTypes.Select((type, index) => $@"RunOnUIThread([&]() {{
    LOG_OUTPUT(L""Activating the type {type.FullName}..."");
    {ToCppCxType(type)} instance = ref new {type.FullName.Replace(".", "::")}();
}});"
            ).Join(Environment.NewLine + Environment.NewLine);

            foreach (Type type in staticTypes)
            {
                var staticProperties = type.GetProperties(BindingFlags.Static | BindingFlags.Public).Where(pi => pi.PropertyType.Name != "DependencyProperty").ToList();

                if (staticProperties.Any())
                {
                    staticProperties.Sort((pi1, pi2) => pi1.Name.CompareTo(pi2.Name));

                    activatableTypeTestCases += Environment.NewLine + Environment.NewLine + staticProperties.Select((property, index) => $@"RunOnUIThread([&](){{
    LOG_OUTPUT(L""Retrieving the static property {property.Name} on type {type.FullName}..."");
    {ToCppCxType(property.PropertyType)} value = {type.FullName.Replace(".", "::")}::{property.Name};
}});"
                    ).Join(Environment.NewLine + Environment.NewLine);
                }
            }

            File.WriteAllText(Path.Combine(Path.GetDirectoryName(OutputFilename), "ActivationTests.g.cpp"), activatableTypeTestCases);
            return true;
        }
    }
}
