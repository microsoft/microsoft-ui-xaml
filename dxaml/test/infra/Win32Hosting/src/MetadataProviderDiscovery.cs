// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.Toolkit.Win32.UI.XamlHost
{
    /// <summary>
    /// MetadataProviderDiscovery is responsible for loading all metadata providers for custom UWP XAML
    /// types.  In this implementation, reflection is used at runtime to probe for metadata providers in
    /// the working directory, allowing any type that includes metadata (compiled in to a .NET framework
    /// assembly) to be used without explicit metadata handling by the application developer.  This
    /// internal class will be amended or removed when additional type loading support is available.
    /// </summary>
    internal static class MetadataProviderDiscovery
    {
        /// <summary>
        /// Probes working directory for all available metadata providers
        /// </summary>
        /// <param name="filteredTypes">Types to ignore</param>
        /// <returns>List of UWP XAML metadata providers</returns>
        internal static List<IXamlMetadataProvider> DiscoverMetadataProviders(List<Type> filteredTypes)
        {
            // List of discovered UWP XAML metadata providers
            var metadataProviders = new List<IXamlMetadataProvider>();

            // Get all assemblies loaded in app domain and placed side-by-side from all DLL and EXE
            var loadedAssemblies = GetAssemblies();
            var uniqueAssemblies = new global::System.Collections.Generic.HashSet<Assembly>(loadedAssemblies, EqualityComparerFactory<Assembly>.CreateComparer(
                a => a.GetName().FullName.GetHashCode(),
                (a, b) => a.GetName().FullName.Equals(b.GetName().FullName, StringComparison.OrdinalIgnoreCase)));

            // Load all types loadable from the assembly, ignoring any types that could not be resolved due to an issue in the dependency chain
            foreach (var assembly in uniqueAssemblies)
            {
                try
                {
                    LoadTypesFromAssembly(assembly, ref metadataProviders, ref filteredTypes);
                }
                catch (FileLoadException)
                {
                    // These exceptions are expected
                }
            }

            return metadataProviders;
        }

        private static IEnumerable<Assembly> GetAssemblies()
        {
            yield return Assembly.GetExecutingAssembly();

            // Get assemblies already loaded in the current app domain
            foreach (var a in AppDomain.CurrentDomain.GetAssemblies())
            {
                yield return a;
            }

            // Reflection-based runtime metadata probing
            var currentDirectory = new FileInfo(typeof(MetadataProviderDiscovery).Assembly.Location).Directory;

            foreach (var file in currentDirectory.GetFiles("*.dll").Union(currentDirectory.GetFiles("*.exe")))
            {
                Assembly a = null;

                try
                {
                    a = Assembly.LoadFrom(file.FullName);
                }
                catch (FileLoadException)
                {
                    // These exceptions are expected
                }
                catch (BadImageFormatException)
                {
                    // DLL is not loadable by CLR (e.g. Native)
                }

                if (a != null)
                {
                    yield return a;
                }
            }
        }

        /// <summary>
        /// Loads all types from the specified assembly and caches metadata providers
        /// </summary>
        /// <param name="assembly">Target assembly to load types from</param>
        /// <param name="metadataProviders">List of metadata providers</param>
        /// <param name="filteredTypes">List of types to ignore</param>
        private static void LoadTypesFromAssembly(Assembly assembly, ref List<IXamlMetadataProvider> metadataProviders, ref List<Type> filteredTypes)
        {
            // Load types inside the executing assembly
            foreach (var type in GetLoadableTypes(assembly))
            {
                if (filteredTypes.Contains(type))
                {
                    continue;
                }

                // TODO: More type checking here
                // Not interface, not abstract, not generic, etc.
                if (typeof(IXamlMetadataProvider).IsAssignableFrom(type))
                {
                    var provider = (IXamlMetadataProvider)Activator.CreateInstance(type);
                    metadataProviders.Add(provider);
                }
            }
        }

        // Algorithm from StackOverflow answer here:
        // http://stackoverflow.com/questions/7889228/how-to-prevent-reflectiontypeloadexception-when-calling-assembly-gettypes
        private static IEnumerable<Type> GetLoadableTypes(Assembly assembly)
        {
            if (assembly == null)
            {
                throw new ArgumentNullException(nameof(assembly));
            }

            try
            {
                return assembly.DefinedTypes.Select(t => t.AsType());
            }
            catch (ReflectionTypeLoadException ex)
            {
                return ex.Types.Where(t => t != null);
            }
        }

        private static class EqualityComparerFactory<T>
        {
            private class MyComparer : IEqualityComparer<T>
            {
                private readonly Func<T, int> _getHashCodeFunc;
                private readonly Func<T, T, bool> _equalsFunc;

                public MyComparer(Func<T, int> getHashCodeFunc, Func<T, T, bool> equalsFunc)
                {
                    _getHashCodeFunc = getHashCodeFunc;
                    _equalsFunc = equalsFunc;
                }

                public bool Equals(T x, T y) => _equalsFunc(x, y);

                public int GetHashCode(T obj) => _getHashCodeFunc(obj);
            }

            public static IEqualityComparer<T> CreateComparer(Func<T, int> getHashCodeFunc, Func<T, T, bool> equalsFunc)
            {
                if (getHashCodeFunc == null)
                {
                    throw new ArgumentNullException(nameof(getHashCodeFunc));
                }

                if (equalsFunc == null)
                {
                    throw new ArgumentNullException(nameof(equalsFunc));
                }

                return new MyComparer(getHashCodeFunc, equalsFunc);
            }
        }
    }
}
