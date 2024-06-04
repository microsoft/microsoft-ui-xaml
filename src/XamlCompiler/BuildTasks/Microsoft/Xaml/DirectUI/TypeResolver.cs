// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Reflection;
    using Lmr;

    internal class TypeResolver
    {
        private HashSet<string> knownAssemblies = new HashSet<string>();
        private Dictionary<string, string> fullNameToAsmQName = null;
        private Dictionary<string, string> directUiToAsmQName = null;

        private XamlTypeUniverse typeUniverse;

        public TypeResolver(XamlTypeUniverse typeUniverse)
        {
            this.typeUniverse = typeUniverse;
        }

        public bool IsInitialized { get { return this.typeUniverse != null; } }

        public void InitializeTypeNameMap()
        {
            Debug.Assert(this.fullNameToAsmQName == null, "TypeResolver is already initialized");

            this.fullNameToAsmQName = new Dictionary<string, string>();
            this.directUiToAsmQName = new Dictionary<string, string>();

            List<Assembly> clrAssemblies;
            List<Assembly> winmdAssemblies;
            this.GetSortedReferenceAssemblies(out clrAssemblies, out winmdAssemblies);

            this.AddClrAssemblies(clrAssemblies);
            this.AddWinmdAssemblies(winmdAssemblies);
        }

        public void AddLocalAssemblyToTypeNameMap(Assembly localAssembly)
        {
            this.AddWinmdAssembly(localAssembly, true);
        }

        public Type GetTypeByFullName(string fullName)
        {
            string qName;
            if (this.fullNameToAsmQName.TryGetValue(fullName, out qName))
            {
                Type type = this.typeUniverse.GetTypeXFromName(qName);
                return type;
            }
            return null;
        }

        public Type GetDirectUIType(string name)
        {
            string qName;
            if (this.directUiToAsmQName.TryGetValue(name, out qName))
            {
                Type type = this.typeUniverse.GetTypeXFromName(qName);
                return type;
            }
            return null;
        }

        private void AddClrAssemblies(IEnumerable<Assembly> clrAssemblies)
        {
            foreach (Assembly asm in clrAssemblies)
            {
                knownAssemblies.Add(asm.ToString());
                foreach (Type type in asm.GetTypes())
                {
                    if (!type.IsPublic)
                    {
                        continue;
                    }

                    string fullName = type.FullName;
                    string assemblyQualifiedTypeName = type.AssemblyQualifiedName;
                    if (!this.fullNameToAsmQName.ContainsKey(fullName))
                    {
                        this.fullNameToAsmQName.Add(fullName, assemblyQualifiedTypeName);
                    }
                    else
                    {
                        string debugMsg = "Duplicate type name encountered: {0} already known as {1} with same key.  {2}.  Resolution behavior for this type is undefined.";
                        Debug.WriteLine(string.Format(debugMsg, fullName, this.fullNameToAsmQName[fullName], assemblyQualifiedTypeName));
                    }
                }
            }
        }

        private void AddWinmdAssemblies(IEnumerable<Assembly> winmdAssemblies)
        {
            foreach (Assembly winmdAsm in winmdAssemblies)
            {
                AddWinmdAssembly(winmdAsm, false);
            }
        }

        private void AddWinmdAssembly(Assembly winmdAsm, bool isLocalAssembly)
        {
            string assemblyName = winmdAsm.ToString();

            // For local assemblies, we want to also include private types, which are normally skipped.
            // The local assembly may've already been processed without us knowing it was a local assembly,
            // in which case it'll be in our knownAssemblies list and we only want to add its private types
            // since its public types were already added.
            bool onlyAddPrivateTypes = false;
            if (knownAssemblies.Contains(assemblyName))
            {
                onlyAddPrivateTypes = true;
            }
            else
            {
                knownAssemblies.Add(assemblyName);
            }

            foreach (Type type in winmdAsm.GetTypes())
            {
                if (IsClrImplementationOfWinRTType(type))
                {
                    continue;
                }

                // Skip non-public types for non-local assemblies
                if (!type.IsPublic && !isLocalAssembly)
                {
                    continue;
                }


                if (type.IsPublic && onlyAddPrivateTypes)
                {
                    continue;
                }

                string fullName = type.FullName;
                string assemblyQualifiedTypeName = type.AssemblyQualifiedName;
                bool added = false;

                if (!this.fullNameToAsmQName.ContainsKey(fullName))
                {
                    this.fullNameToAsmQName.Add(fullName, assemblyQualifiedTypeName);
                    added = true;
                }
                else
                {
                    string debugMsg = "Duplicate type name encountered: {0} already known as {1} with same key.  {2}.  Resolution behavior for this type is undefined.";
                    Debug.WriteLine(string.Format(debugMsg, fullName, this.fullNameToAsmQName[fullName], assemblyQualifiedTypeName));
                }

                // Build a special list for the XAML Presentation namespace Types.
                if (added)
                {
                    string baseName;
                    if (this.IsDirectUIType(fullName, out baseName))
                    {
                        this.directUiToAsmQName.Add(baseName, assemblyQualifiedTypeName);
                    }
                }
            }
        }

        private bool IsDirectUIType(string fullName, out string name)
        {
            name = null;
            if (fullName.StartsWith(KnownNamespaces.WindowsUI))
            {
                int lastDot = fullName.LastIndexOf('.');
                string path = fullName.Substring(0, lastDot);

                if (KS.ContainsString(DirectUISchemaContext.DirectUI2010Paths, path))
                {
                    name = fullName.Substring(lastDot + 1);
                    return true;
                }
            }
            if (fullName.StartsWith(KnownNamespaces.WindowsFoundation))
            {
                string baseName = fullName.Substring(KnownNamespaces.WindowsFoundationPrefix.Length);

                if (KS.ContainsString(DirectUIXamlType.WindowsFoundationSystemTypes, baseName))
                {
                    name = baseName;
                    return true;
                }
            }
            return false;
        }

        private void GetSortedReferenceAssemblies(out List<Assembly> clrAssemblies, out List<Assembly> winmdAssemblies)
        {
            clrAssemblies = new List<Assembly>();
            winmdAssemblies = new List<Assembly>();

            foreach (Assembly asm in this.typeUniverse.Assemblies)
            {
                AssemblyName asmName = asm.GetName();

                if (asmName.ContentType == AssemblyContentType.WindowsRuntime)
                {
                    if (asmName.Name.Equals("platform", StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }
                    winmdAssemblies.Add(asm);
                }
                else
                {
                    clrAssemblies.Add(asm);
                }
            }
        }

        private Type ResolveTypeByReflectionFromAssemblyQualifiedName(string assemblyQualifiedTypeName)
        {
            if (string.IsNullOrEmpty(assemblyQualifiedTypeName) || assemblyQualifiedTypeName.IndexOf(',') < 0)
            {
                return null;
            }

            Type result = GetTypeFromUniverse(assemblyQualifiedTypeName);
            return result;
        }

        private Type GetTypeFromUniverse(string name)
        {
            try
            {
                return this.typeUniverse.GetTypeXFromName(name);
            }
            catch (TypeLoadException e)
            {
                if (name.StartsWith("Platform.", StringComparison.OrdinalIgnoreCase))
                {
                    // Platform.Object gets special treatment as a failure case because it projects to a class without a base class.
                    // This causes confusion in type resolution, but this particular failure is by-design.
                }
                else
                {
                    Debug.Fail(string.Format("{0}: Failed to resolve type {1}", e.GetType().FullName, name));
                }
            }
            catch (BadImageFormatException)
            {
                // CLR throws these if there is a native EXE in the iteration path (tries to load it as a managed EXE)
            }
            catch (ArgumentException)
            {
                Debug.Fail(string.Format("Bug 422976 without repro: Failed to resolve type {0}. Please notify platform team.", name));
            }

            return null;
        }


        // In a managed WinRT class library compiled as a .winmd, there are two TypeDefs for each
        // type:  one is the type exposed to WinRT, the other is the CLR implementation of the
        // type.  The CLR implementation type has a "<CLR>" prefix in the type name.  The CLR
        // reflection API will never give us these types, but LMR does because they really do exist
        // in the metadata.
        private static bool IsClrImplementationOfWinRTType(Type type)
        {
            return type.Name.StartsWith("<CLR>", StringComparison.Ordinal);
        }
    }
}