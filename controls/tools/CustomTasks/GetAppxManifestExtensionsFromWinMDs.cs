using Microsoft.Build.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices.WindowsRuntime;

namespace CustomTasks
{
    public class GetAppxManifestExtensionsFromWinMDs : IsolatedTask
    {
        [Required]
        public ITaskItem[] ReferencePaths
        {
            get;
            set;
        }

        [Required]
        public string[] ReferenceWinMDs
        {
            get;
            set;
        }

        [Output]
        public string AppxManifestExtensions
        {
            get;
            set;
        }

        public override bool ExecuteCore()
        {
            Dictionary<string, List<string>> typesImplementedByImplementationDlls = new Dictionary<string, List<string>>();

            foreach (ITaskItem referenceItem in ReferencePaths)
            {
                // If this isn't a WinMD file, if it's the merged Microsoft WinMDs, or if it has no implementation DLL, then we'll ignore it.
                if (!string.Equals(referenceItem.GetMetadata("Extension"), ".winmd", StringComparison.OrdinalIgnoreCase) ||
                    referenceItem.GetMetadata("Identity").ToLower().Contains("microsoft.ui.private.winmd") ||
                    referenceItem.GetMetadata("Identity").ToLower().Contains("microsoft.ui.winmd") ||
                    referenceItem.GetMetadata("Identity").ToLower().Contains("microsoft.ui.xaml.winmd") ||
                    string.IsNullOrEmpty(referenceItem.GetMetadata("Implementation")))
                {
                    continue;
                }

                string referencePath = referenceItem.GetMetadata("Identity");

                AppDomain.CurrentDomain.ReflectionOnlyAssemblyResolve += (sender, eventArgs) =>
                {
                    Assembly assembly = null;
                    string assemblyName = eventArgs.Name.Split(',')[0];

                    foreach (var reference in ReferenceWinMDs)
                    {
                        var referenceName = System.IO.Path.GetFileNameWithoutExtension(reference);
                        if (referenceName == assemblyName)
                        {
                            assembly = Assembly.ReflectionOnlyLoadFrom(reference);
                            break;
                        }
                    }

                    return assembly ?? Assembly.ReflectionOnlyLoad(eventArgs.Name);
                };

                WindowsRuntimeMetadata.ReflectionOnlyNamespaceResolve += (sender, eventArgs) =>
                {
                    foreach (var refer in ReferenceWinMDs)
                    {
                        eventArgs.ResolvedAssemblies.Add(Assembly.ReflectionOnlyLoadFrom(refer));
                    }

                    return;
                };

                string fullWinMDPath = Path.GetFullPath(referencePath);
                Assembly winMDAssembly = null;
                foreach (var loadedAssembly in AppDomain.CurrentDomain.ReflectionOnlyGetAssemblies())
                {
                    if (String.Equals(loadedAssembly.Location, fullWinMDPath, StringComparison.OrdinalIgnoreCase))
                    {
                        winMDAssembly = loadedAssembly;
                        break;
                    }
                }

                if (winMDAssembly == null)
                {
                    winMDAssembly = Assembly.ReflectionOnlyLoadFrom(fullWinMDPath);
                }

                List<string> typesImplemented = new List<string>();

                foreach (string winMDType in winMDAssembly.GetExportedTypes().Select(type => type.FullName))
                {
                    typesImplemented.Add(winMDType);
                }

                if (typesImplemented.Count > 0)
                {
                    string implementationDll = referenceItem.GetMetadata("Implementation");
                    List<string> typesImplementedInDll;

                    if (typesImplementedByImplementationDlls.TryGetValue(implementationDll, out typesImplementedInDll))
                    {
                        typesImplementedInDll.AddRange(typesImplemented);
                    }
                    else
                    {
                        typesImplementedByImplementationDlls.Add(implementationDll, typesImplemented);
                    }
                }
            }

            List<string> extensionStrings = new List<string>();

            foreach (string implementationDll in typesImplementedByImplementationDlls.Keys.OrderBy(key => key))
            {
                string extensionString = string.Empty;
                extensionString += "    <Extension Category=\"windows.activatableClass.inProcessServer\">" + Environment.NewLine;
                extensionString += "      <InProcessServer>" + Environment.NewLine;
                extensionString += "        <Path>" + implementationDll + "</Path>" + Environment.NewLine;

                foreach (string typeImplemented in typesImplementedByImplementationDlls[implementationDll].OrderBy(typeName => typeName))
                {
                    extensionString += "        <ActivatableClass ActivatableClassId=\"" + typeImplemented + "\" ThreadingModel=\"both\" />" + Environment.NewLine;
                }

                extensionString += "      </InProcessServer>" + Environment.NewLine;
                extensionString += "    </Extension>";

                extensionStrings.Add(extensionString);
            }

            AppxManifestExtensions = string.Join(Environment.NewLine, extensionStrings);
            return true;
        }
    }
}
