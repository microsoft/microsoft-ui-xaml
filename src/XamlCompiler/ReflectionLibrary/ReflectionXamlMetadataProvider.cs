// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using System.Reflection;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Markup
{
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    public sealed class ReflectionXamlMetadataProvider : IXamlMetadataProvider
    {
        private static List<WeakReference<ReflectionXamlMetadataProvider>> _providers;

        private static List<string> _assemblies;
        private static Object _assembliesLock = new object();

        private static bool gotAssemblies = false;

        public ReflectionXamlMetadataProvider()
        {
            lock (_assembliesLock)
            {
                if (!gotAssemblies)
                {
                    if (_providers == null) _providers = new List<WeakReference<ReflectionXamlMetadataProvider>>();
                    _providers.Add(new WeakReference<ReflectionXamlMetadataProvider>(this));

                    _assemblies = new List<string>();

                    // Add our core Windows and .NET assemblies
                    _assemblies.Add(typeof(global::Microsoft.UI.Xaml.Application).GetTypeInfo().Assembly.FullName);
                    _assemblies.Add(typeof(System.Object).GetTypeInfo().Assembly.FullName);

                    PopulateAssemblies();
                }
            }
        }

        private void PopulateAssemblies()
        {
            var asmNames = Task.Run(() => GetAssemblyListAsync()).Result;
            foreach (var asmName in asmNames)
            {
                _assemblies.Add(asmName);
            }
        }

        private async Task<List<string>> GetAssemblyListAsync()
        {
            global::Windows.Storage.StorageFolder appFolder = global::Windows.ApplicationModel.Package.Current.InstalledLocation;

            List<string> assemblies = new List<string>();

            Stack<global::Windows.Storage.StorageFolder> folderStack = new Stack<global::Windows.Storage.StorageFolder>();
            folderStack.Push(appFolder);

            while (folderStack.Count > 0)
            {
                var curFolder = folderStack.Pop();
                foreach (var folder in await curFolder.GetFoldersAsync())
                {
                    folderStack.Push(folder);
                }

                foreach (global::Windows.Storage.StorageFile file in await curFolder.GetFilesAsync())
                {
                    if (file.FileType == ".dll" || file.FileType == ".exe" || file.FileType == ".winmd")
                    {                       
                        string fileName = file.DisplayName; //DisplayName chops off the file type - we don't want the extension
                        if (!assemblies.Contains(fileName))
                        {
                            assemblies.Add(fileName);
                        }
                    }
                }
            }

            gotAssemblies = true;
            return assemblies;
        }

        // Begin IXamlMetadataProvider implementation

        public IXamlType GetXamlType(System.Type type)
        {
            return getXamlType(type);
        }

        public IXamlType GetXamlType(string fullName)
        {
            return getXamlType(fullName);
        }

        public XmlnsDefinition[] GetXmlnsDefinitions()
        {
            // While this matches the current behavior of our codegenned GetXmlnsDefinitions, this method should never be invoked.
            return new XmlnsDefinition[0];
        }

        // End IXamlMetadataProvider implementation

        private static Object _xamlCacheLock = new object();
        private static Dictionary<string, IXamlType> _xamlTypeCacheByName = new Dictionary<string, IXamlType>();
        private static Dictionary<System.Type, IXamlType> _xamlTypeCacheByType = new Dictionary<System.Type, IXamlType>();

        internal static IXamlType getXamlType(Type typeID)
        {
            IXamlType xamlType;

            // We'll grab the lock here, and keep it for the duration of the lookup (and creation if necessary), to avoid accidentally making
            // duplicate types
            lock (_xamlCacheLock)
            {
                if (_xamlTypeCacheByType.TryGetValue(typeID, out xamlType))
                {
                    return xamlType;
                }

                xamlType = CreateXamlType(typeID);
                if (xamlType != null)
                {
                    lock (_xamlCacheLock)
                    {
                        _xamlTypeCacheByName.Add(xamlType.FullName, xamlType);
                        _xamlTypeCacheByType.Add(xamlType.UnderlyingType, xamlType);
                    }
                }
            }

            // We return null instead of throwing an exception if we couldn't create the Xaml type,
            // as our codegenned version doesn't either.  We sometimes get called
            // with invalid types, especially when using Xaml diagnostic tools.
            return xamlType;
        }

        internal static IXamlType getXamlType(string typeName)
        {
            string compilerTypeName = TypeExtensions.MakeCompilerTypeName(typeName);
            if (string.IsNullOrEmpty(compilerTypeName))
            {
                // We can get queried with invalid type names - to keep parity with the codegenned implementation, we should return null, not throw
                return null;
            }
            IXamlType xamlType;
            lock (_xamlCacheLock)
            {
                if (_xamlTypeCacheByName.TryGetValue(compilerTypeName, out xamlType))
                {
                    return xamlType;
                }
                xamlType = CreateXamlType(compilerTypeName);

                if (xamlType != null)
                {
                    lock (_xamlCacheLock)
                    {
                        _xamlTypeCacheByName.Add(xamlType.FullName, xamlType);
                        _xamlTypeCacheByType.Add(xamlType.UnderlyingType, xamlType);

                        if (!xamlType.FullName.Equals(compilerTypeName))
                        {
                            throw new ReflectionHelperException($"Created Xaml type '{xamlType.FullName}' has a different name than requested type '{compilerTypeName}'");
                        }
                    }
                }
                // We return null instead of throwing an exception if we couldn't create the Xaml type,
                // as our codegenned version doesn't either.  We sometimes get called
                // with invalid types, especially when using Xaml diagnostic tools.

            }
            return xamlType;
        }

        private static IXamlType CreateXamlType(Type typeID)
        {
            return XamlReflectionType.Create(typeID);
        }

        private static IXamlType CreateXamlType(string typeName)
        {
            string compilerTypeName = TypeExtensions.MakeCompilerTypeName(typeName);
            if (IsGenericTypeName(compilerTypeName))
            {
                return ConstructGenericType(compilerTypeName);
            }
            else
            {
                return XamlReflectionType.Create(GetNonGenericType(compilerTypeName));
            }
        }

        private static Type GetNonGenericType(string compilerTypeName)
        {
            // Get the C# name in case the framework gave us a standard name, like 'Double' instead of 'System.Double'
            compilerTypeName = TypeExtensions.GetCSharpTypeName(compilerTypeName);

            // We don't need to lock on _assemblies - it's populated before any providers could actually call this,
            // and is only read, never written, from then on
            foreach (string a in _assemblies)
            {
                try
                {
                    Type t = Type.GetType(compilerTypeName + ", " + a);
                    if (t != null) { return t; }
                }
                catch (Exception)
                {
                }
            }

            // We can get queried with invalid type names - to keep parity with the codegenned implementation, we should return null, not throw
            return null;
        }

        private static IXamlType ConstructGenericType(string compilerTypeName)
        {
            StringBuilder sb = new StringBuilder();
            Stack<List<Type>> genericTypeArgsStack = new Stack<List<Type>>();
            Stack<Type> genericTypeDefinitionStack = new Stack<Type>();
            foreach (char typeChar in compilerTypeName)
            {
                switch (typeChar)
                {
                    case '<':
                        // The start of a list of generic type parameters.  Add the current parsed name
                        // to our generic type definition stack so we can populate it with the right generic
                        // type parameters after parsing them
                        string genericTypeName = sb.ToString();
                        Type genericType = GetNonGenericType(genericTypeName);

                        // If we couldn't resolve the non-generic type, we won't be able to construct the final type - just return null
                        if (genericType == null)
                        {
                            return null;
                        }

                        genericTypeDefinitionStack.Push(genericType);
                        genericTypeArgsStack.Push(new List<Type>());
                        sb.Clear();
                        break;
                    case '>':
                        // If this is the first '>' we've seen, we need to clear the buffer
                        // since it contains the last generic type parameter.  This may not always be the case,
                        // e.g. if the 2nd '>' in a substring that looks like ">>>" since several generic type definitions are ending.
                        // If there is a type name in the buffer, it's guaranteed to be not generic
                        if (sb.Length > 0)
                        {
                            string genericTypeParamName = sb.ToString();
                            sb.Clear();
                            Type genericTypeParam = GetNonGenericType(genericTypeParamName);

                            // If we couldn't resolve one of the generic type parameters, just return null since we won't be able to construct the final type
                            if (genericTypeParam == null)
                            {
                                return null;
                            }
                            genericTypeArgsStack.Peek().Add(genericTypeParam);
                        }

                        // This is the end of a list of generic type parameters, so now we need to construct the full generic
                        // type given the generic type definition currently on the stack
                        List<Type> genericTypeArgs = genericTypeArgsStack.Pop();
                        Type[] genericTypeArgsArr = genericTypeArgs.ToArray();
                        Type genericTypeDefinition = genericTypeDefinitionStack.Pop();
                        Type actualGenericType = genericTypeDefinition.MakeGenericType(genericTypeArgsArr);

                        // If we couldn't resolve the actual type, just return null
                        if (actualGenericType == null)
                        {
                            return null;
                        }

                        // The generic type we just constructed may be a generic type parameter itself.  If there's another generic type
                        // parameter on the stack before us, we should go there
                        if (genericTypeArgsStack.Count > 0)
                        {
                            genericTypeArgsStack.Peek().Add(actualGenericType);
                        }
                        else
                        {
                            genericTypeDefinitionStack.Push(actualGenericType);
                        }
                        break;
                    case ',':
                        // The buffer may contain a non-generic type name, if so we'll flush the string builder
                        // and add the type as a generic type parameter
                        if (sb.Length > 0)
                        {
                            string genericTypeParamName = sb.ToString();
                            sb.Clear();
                            Type genericTypeParam = GetNonGenericType(genericTypeParamName);

                            // If we couldn't resolve one of the generic type parameters, just return null since we won't be able to construct the final type
                            if (genericTypeParam == null)
                            {
                                return null;
                            }
                            genericTypeArgsStack.Peek().Add(genericTypeParam);
                        }
                        break;
                    case ' ':
                        //We don't want to add any whitespace to the string builder (e.g. after a comma)
                        break; 
                    default:
                        sb.Append(typeChar);
                        break;
                }
            }

            // At this point, we should have our return type as the only element on the stack
            if (genericTypeDefinitionStack.Count != 1)
            {
                throw new ReflectionHelperException($"Error constructing generic type '{compilerTypeName}'");
            }

            return XamlReflectionType.Create(genericTypeDefinitionStack.Pop());
        }

        private static bool IsGenericTypeName(string compilerTypeName)
        {
            return compilerTypeName.Contains("<") || compilerTypeName.Contains("`");
        }

        private IXamlMember CreateXamlMember(string longMemberName)
        {
            string typeName, memberName;
            GetTypeAndMember(longMemberName, out typeName, out memberName);

            return XamlReflectionMember.Create(memberName, getXamlType(typeName).UnderlyingType);
        }

        private static void GetTypeAndMember(string longMemberName, out string typeName, out string memberName)
        {
            int i = longMemberName.LastIndexOf('.');
            string typename = longMemberName.Substring(0, i);
            string membername = longMemberName.Substring(i + 1);

            typeName = typename;
            memberName = membername;
        }
    }
}

