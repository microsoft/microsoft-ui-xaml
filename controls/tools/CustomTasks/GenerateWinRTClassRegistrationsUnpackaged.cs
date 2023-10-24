using Microsoft.Build.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Xml;

namespace CustomTasks
{
    public class GenerateWinRTClassRegistrationsUnpackaged : IsolatedTask
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
        public string Metadata_Dir
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

        public override bool ExecuteCore()
        {
            // Maps implementation DLLs to WinRT types
            Dictionary<string, List<string>> implementationDllToTypes = new Dictionary<string, List<string>>();

            // Maps winmds to implementation DLLs 
            Dictionary<Assembly, string> winmdToImplementationDll = new Dictionary<Assembly, string>();

            // Cache assemblies loaded into the ReflectionOnly context 
            List<Assembly> loadedAssemblies = new List<Assembly>();
            List<Assembly> unmergedWinMDs = new List<Assembly>();

            // Cache factory interfaces with a CreateInstance method
            List<string> factoryInterfaces = new List<string>();

            // Pre-load the unmerged Lifted XAML WinMDs
            foreach (ITaskItem winmd in WinMDs)
            {
                string winmdName = winmd.GetMetadata("Identity");
                string implementationDll = winmd.GetMetadata("ImplementationDll");

                if (implementationDll == string.Empty)
                {
                    LogWarning($"ImplementationDll metadata for file {winmdName} is empty. Using DefaultImplementationDll {DefaultImplementationDll} instead.");
                    implementationDll = DefaultImplementationDll;
                }

                var reflectionOnlyLoadedAssembly = Assembly.ReflectionOnlyLoadFrom(winmdName);
                unmergedWinMDs.Add(reflectionOnlyLoadedAssembly);
                loadedAssemblies.Add(reflectionOnlyLoadedAssembly);
                winmdToImplementationDll[reflectionOnlyLoadedAssembly] = implementationDll;
            }

            // Pre-load the Foundation WinMDs
            var foundationWinmds = Directory.GetFiles(Metadata_Dir, "*.winmd");
            foreach (var winmd in foundationWinmds)
            {
                loadedAssemblies.Add(Assembly.ReflectionOnlyLoadFrom(Path.GetFullPath(winmd)));
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

                        if (interfaceName.EndsWith("Factory") ||
                            interfaceName.EndsWith("FactoryPrivate")) // Special case for Microsoft.UI.Xaml.Media.Animation.Transition
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
                            implementationDllToTypes[implementationDll].Add(unmergedWinMDType.FullName);
                            unmergedWinMDTypeAdded = true;
                            break;
                        }
                    }

                    // Types with a Windows.Foundation.Metadata.Composable attribute are declared activatable unless they point to a factory interface without CreateInstance method
                    if (!unmergedWinMDTypeAdded)
                    {
                        foreach (var customAttributeData in unmergedWinMDType.GetTypeInfo().CustomAttributes)
                        {
                            if (customAttributeData.AttributeType.FullName == "Windows.Foundation.Metadata.ComposableAttribute")
                            {
                                var factoryName = customAttributeData.ConstructorArguments[0].Value.ToString();

                                if (factoryInterfaces.Contains(factoryName) ||
                                    factoryInterfaces.Contains(factoryName + "Private")) // Special case for Microsoft.UI.Xaml.Media.Animation.Transition
                                {
                                    implementationDllToTypes[implementationDll].Add(unmergedWinMDType.FullName);
                                }
                                break;
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

            // Writing tag <assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
            xmlWriter.WriteStartElement("assembly", "urn:schemas-microsoft-com:asm.v1");
            xmlWriter.WriteAttributeString("manifestVersion", "1.0");

            foreach (var implementationDll in implementationDllToTypes.Keys)
            {
                var types = implementationDllToTypes[implementationDll];

                if (types.Count > 0)
                {
                    // Writing tags of the form <file name="dcompi.dll" xmlns:s="urn:schemas-microsoft-com:winrt.v1">
                    xmlWriter.WriteStartElement("file");
                    xmlWriter.WriteAttributeString("name", implementationDll);
                    xmlWriter.WriteAttributeString("xmlns", "s", null, "urn:schemas-microsoft-com:winrt.v1");

                    foreach (var winRTType in types)
                    {
                        // Writing tags of the form <s:activatableClass name="Microsoft.UI.ContentAutomation" threadingModel="both" /> 
                        xmlWriter.WriteStartElement("activatableClass", "urn:schemas-microsoft-com:winrt.v1");
                        xmlWriter.WriteAttributeString("name", winRTType);
                        xmlWriter.WriteAttributeString("threadingModel", "both");
                        xmlWriter.WriteEndElement();
                    }

                    xmlWriter.WriteEndElement();
                }
            }

            xmlWriter.WriteEndElement();
            xmlWriter.Close();
            return true;
        }
    }
}
