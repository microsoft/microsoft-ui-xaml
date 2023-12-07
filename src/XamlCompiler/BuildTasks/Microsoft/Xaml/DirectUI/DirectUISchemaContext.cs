// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Reflection;
    using System.Xaml;
    using Lmr;
    using Properties;
    using Utilities;

    // Note: We only support reference assembly scenarios at this point
    internal class DirectUISchemaContext : XamlSchemaContext
    {
        // The Type Paths of the DirectUI URI.
        // This is the definition of http://schemas.microsoft.com/windows/2010/directui
        internal static List<string> DirectUI2010Paths = new List<string>()
        {
            KnownNamespaces.Xaml,
            KnownNamespaces.XamlAutomation,
            KnownNamespaces.XamlAutomationPeers,
            KnownNamespaces.XamlAutomationProvider,
            KnownNamespaces.XamlControls,
            KnownNamespaces.XamlControlsPrimitives,
            KnownNamespaces.XamlData,
            KnownNamespaces.XamlDocuments,
            KnownNamespaces.XamlInput,
            KnownNamespaces.XamlInterop,
            KnownNamespaces.XamlMarkup,
            KnownNamespaces.XamlMedia,
            KnownNamespaces.XamlMediaAnimation,
            KnownNamespaces.XamlMediaImaging,
            KnownNamespaces.XamlMediaMedia3D,
            KnownNamespaces.XamlNavigation,
            KnownNamespaces.XamlResources,
            KnownNamespaces.XamlShapes,
            KnownNamespaces.XamlThreading,
            KnownNamespaces.WindowsUI,
            KnownNamespaces.Text
        };

        private static ReadOnlyCollection<string> DirectUIXamlNamespaces = new ReadOnlyCollection<string>(new string[]
            {
                KnownNamespaceUris.Wpf2006,
                KnownNamespaceUris.DirectUI2010,
            });

        private Assembly localAssembly;
        private DirectUISystem directUISystem;
        private IDirectUIXamlLanguage directUIXamlLanguage;
        private List<string> systemExtraReferenceItems;
        private IEnumerable<DirectUIAssembly> systemExtraAssemblies;
        private ISet<string> staticLibraryAssemblies;
        private Dictionary<string, Dictionary<string, ProxyDirectUIXamlType>> proxyTypes;
        private List<Assembly> managedProjectionAssemblies = new List<Assembly>();
        private Dictionary<Type, XamlType> masterTypeTable = new Dictionary<Type, XamlType>();
        private Dictionary<string, XamlType> uiXamlCache = new Dictionary<string, XamlType>();
        private Dictionary<string, List<string>> usingNamespaces = new Dictionary<string, List<string>>();
        private Dictionary<string, XamlType> masterTypeTableByFullName = new Dictionary<string, XamlType>();
        private Dictionary<string, XamlType> domFullTypeNameCache = new Dictionary<string, XamlType>();
        private Lazy<List<XamlCompileWarning>> warningMessages = new Lazy<List<XamlCompileWarning>>(() => new List<XamlCompileWarning>());
        private Lazy<List<XamlCompileError>> errorMessages = new Lazy<List<XamlCompileError>>(() => new List<XamlCompileError>());
        private string windowsSdkPath;

        public DirectUISchemaContext(
            IEnumerable<Assembly> referenceAssemblies,
            List<string> systemExtraReferenceItems,
            Assembly localAssembly,
            ISet<string> staticLibraryAssemblies,
            string sdkPath,
            bool isStringNullable
            )
            : base(DirectUIAssembly.Wrap(referenceAssemblies))
        {
            // passing null normally means use all the currently loaded Assemblies
            // as your reference assembly list.  But given that this is the Direct UI
            // schema running on .Net the currently loaded assemblies are never appropriate.
            if (referenceAssemblies == null)
            {
                throw new ArgumentNullException("referenceAssemblies");
            }

            this.windowsSdkPath = sdkPath;
            this.directUISystem = new DirectUISystem(ReferenceAssemblies);
            this.directUIXamlLanguage = new DirectUIXamlLanguage(this, isStringNullable);

            this.systemExtraReferenceItems = systemExtraReferenceItems;
            this.localAssembly = localAssembly;
            this.staticLibraryAssemblies = staticLibraryAssemblies;

            // Managed assembly where the XAML structs and things are reflected into.
            foreach (DirectUIAssembly asm in this.ReferenceAssemblies)
            {
                AssemblyName assemblyName = asm.GetName();
                if (KS.EqIgnoreCase(assemblyName.Name, "System.Runtime.WindowsRuntime.UI.Xaml"))
                {
                    this.managedProjectionAssemblies.Add(asm);
                }
                else if (KS.EqIgnoreCase(assemblyName.Name, "System.Runtime.WindowsRuntime"))
                {
                    this.managedProjectionAssemblies.Add(asm);
                }
            }
        }

        /// <summary>
        /// For use by DirectUIXamlLanguage only
        /// This should only be called after a successfull call to the public Ctor 
        /// </summary>
        internal DirectUISchemaContext() { }

        internal List<XamlCompileWarning> SchemaWarnings { get { return this.warningMessages.Value; } }
        internal List<XamlCompileError> SchemaErrors { get { return this.errorMessages.Value; } }
        internal TypeResolver TypeResolver { get; set; }
        internal DirectUISystem DirectUISystem { get { return this.directUISystem; } }
        internal IDirectUIXamlLanguage DirectUIXamlLanguage { get { return this.directUIXamlLanguage; } }

        /// <summary>
        /// A collection of types that exist in XAML UI but don't exist as actual types in the Jupiter Object Model.
        /// The Xaml Schema needs a type to find properties, constructors etc.
        /// </summary>
        internal Dictionary<string, Dictionary<string, ProxyDirectUIXamlType>> ProxyTypes
        {
            get
            {
                if (this.proxyTypes == null)
                {
                    Dictionary<string, ProxyDirectUIXamlType> proxyDirectUITypes = new Dictionary<string, ProxyDirectUIXamlType>
                    {
                        { TypeProxyMetadata.NullExtension.Name, new ProxyDirectUIXamlType(TypeProxyMetadata.NullExtension, this) },
                        { TypeProxyMetadata.StaticResourceExtension.Name, new ProxyDirectUIXamlType(TypeProxyMetadata.StaticResourceExtension, this) },
                        { TypeProxyMetadata.ThemeResourceExtension.Name, new ProxyDirectUIXamlType(TypeProxyMetadata.ThemeResourceExtension, this) },
                        { TypeProxyMetadata.CustomResourceExtension.Name, new ProxyDirectUIXamlType(TypeProxyMetadata.CustomResourceExtension, this) },
                        { TypeProxyMetadata.TemplateBindingExtension.Name, new ProxyDirectUIXamlType(TypeProxyMetadata.TemplateBindingExtension, this) },
                        { TypeProxyMetadata.BindExtension.Name, new ProxyDirectUIXamlType(TypeProxyMetadata.BindExtension, this) },
                        { TypeProxyMetadata.Properties.Name, new ProxyDirectUIXamlType(TypeProxyMetadata.Properties, this) },
                    };

                    this.proxyTypes = new Dictionary<string, Dictionary<string, ProxyDirectUIXamlType>>
                    {
                        { KnownNamespaceUris.Wpf2006, proxyDirectUITypes },
                        { KnownNamespaceUris.DirectUI2010, proxyDirectUITypes },
                    };
                }
                return this.proxyTypes;
            }
        }

        public override XamlType GetXamlType(Type type)
        {
            if (type == null)
            {
                throw new ArgumentNullException("type");
            }

            type = this.EnsureIsLmrType(type);

            XamlType xType;

            if (!this.masterTypeTable.TryGetValue(type, out xType))
            {
                xType = new DirectUIXamlType(type, this);
                this.masterTypeTable.Add(type, xType);

                if (this.masterTypeTableByFullName.ContainsKey(type.FullName))
                {
                    SchemaErrors.Add(new XamlErrorDuplicateType(type.FullName));
                }
                else
                {
                    this.masterTypeTableByFullName.Add(type.FullName, xType);
                }
            }

            return xType;
        }

        public override ICollection<XamlType> GetAllXamlTypes(string xamlNamespace)
        {
            return this.LookupAllXamlTypes(xamlNamespace);
        }

        public bool IsLocalAssembly(DirectUIAssembly asm)
        {
            if (asm != null)
            {
                if (asm.WrappedAssembly == this.localAssembly)
                {
                    return true;
                }
                else if (staticLibraryAssemblies != null && staticLibraryAssemblies.Contains(asm.Location))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        public override XamlDirective GetXamlDirective(string xamlNamespace, string name)
        {
            XamlDirective directive = base.GetXamlDirective(xamlNamespace, name);

            if (directive == XamlLanguage.Arguments ||
                directive == XamlLanguage.AsyncRecords ||
                directive == XamlLanguage.ClassAttributes ||
                directive == XamlLanguage.ClassModifier ||
                directive == XamlLanguage.Code ||
                directive == XamlLanguage.Members ||
                directive == XamlLanguage.Subclass ||
                directive == XamlLanguage.SynchronousMode ||
                directive == XamlLanguage.TypeArguments ||
                directive == XamlLanguage.FactoryMethod)
            {
                directive = null;
            }
            return directive;
        }

        internal XamlType GetXamlType(string fullName)
        {
            XamlType xamlType;

            if (!this.masterTypeTableByFullName.TryGetValue(fullName, out xamlType))
            {
                xamlType = null;
                Type type = TypeResolver.GetTypeByFullName(fullName);
                if (type != null)
                {
                    xamlType = this.GetXamlType(type);
                }
            }
            return xamlType;
        }

        internal XamlType GetProxyType(string xamlNamespace, string name)
        {
            // StaticResource, ThemeResource, CustomResource and TemplateBinding are supported in DirectUI XAML but not in the actual OM so pick them up here
            Dictionary<string, ProxyDirectUIXamlType> proxyTypes;
            ProxyDirectUIXamlType proxyType;
            if (this.ProxyTypes.TryGetValue(xamlNamespace, out proxyTypes))
            {
                if (proxyTypes.TryGetValue(name, out proxyType))
                {
                    return proxyType;
                }

                string fallbackName = DirectUISchemaContext.GetTypeExtensionName(name);
                if (proxyTypes.TryGetValue(fallbackName, out proxyType))
                {
                    return proxyType;
                }
            }

            return null;
        }

        protected internal override XamlType GetXamlType(string xamlNamespace, string name, params XamlType[] typeArguments)
        {
            string domFullTypeName = string.Format("{0}.{1}", xamlNamespace, name);
            XamlType xamlType = null;
            ApiInformation apiInformation = null;
            Platform targetPlatform = Platform.Any;
            
            // This is a lookup of the full type name as it comes from the the dom parser.
            // It may or may not contain conditional statemets.
            if (this.domFullTypeNameCache.ContainsKey(domFullTypeName))
            {
                return this.domFullTypeNameCache[domFullTypeName];
            }

            // no x:Type, x:Static etc...
            if (KS.ContainsString(XamlLanguage.XamlNamespaces, xamlNamespace))
            {
                xamlType = DirectUIXamlLanguage.LookupXamlObjects(name);
            }
            else
            {
                string nonConditionalNamespace = xamlNamespace;

                if (xamlNamespace.IsConditionalNamespace())
                {
                    try
                    {
                        var parsedValues = ConditionalNamespace.Parse(xamlNamespace);
                        nonConditionalNamespace = parsedValues.UnconditionalNamespace;
                        apiInformation = parsedValues.ApiInfo;
                        targetPlatform = parsedValues.PlatConditional;
                    }
                    catch(ParseException)
                    {
                        Debug.Assert(false, "Validator should have checked this scenario");
                        return null;
                    }
                }

                if (KS.ContainsString(DirectUISchemaContext.DirectUIXamlNamespaces, nonConditionalNamespace))
                {
                    xamlType = this.GetDirectUIXamlType(name);
                    if (xamlType == null)
                    {
                        // Proxy Types are not cached, they have no Type
                        return this.GetProxyType(nonConditionalNamespace, name);
                    }
                }
                else
                {
                    xamlType = this.GetXamlTypeFromUsing(nonConditionalNamespace, name, typeArguments);
                }
            }

            if (xamlType != null)
            {
                if (!(xamlType is DirectUIXamlType) || !(xamlType.UnderlyingType is MetadataOnlyCommonType))
                {
                    // exchange the XamlType for a DirectUIXamlType
                    xamlType = this.GetXamlType(xamlType.UnderlyingType);
                }

                // Pass along API Information
                var duiType = xamlType as DirectUIXamlType;
                if ((apiInformation != null || targetPlatform != Platform.Any) && duiType != null)
                {
                    duiType = new DirectUIXamlType(xamlType.UnderlyingType, this, apiInformation, targetPlatform);
                    xamlType = duiType;
                }

                this.domFullTypeNameCache[domFullTypeName] = xamlType;
            }

            return xamlType;
        }

        private ICollection<XamlType> LookupAllXamlTypes(string xamlNamespace)
        {
            List<XamlType> xamlTypes = new List<XamlType>();
            if (!KS.ContainsString(DirectUISchemaContext.DirectUIXamlNamespaces, xamlNamespace))
            {
                throw new NotImplementedException(ResourceUtilities.FormatString(XamlCompilerResources.DuiSchema_GetAllXamlTypeNotImpl, xamlNamespace));
            }

            if (this.directUISystem.PlatformAssemblies.Count == 0)
            {
                this.SchemaErrors.Add(new XamlSchemaError_WRTAssembliesMissing());
                return null;
            }

            foreach (DirectUIAssembly asm in this.directUISystem.PlatformAssemblies)
            {
                Type[] types = asm.GetTypes();

                foreach (Type t in types)
                {
                    int idx = t.FullName.LastIndexOf('.');
                    string clrPath = t.FullName.Substring(0, idx);
                    if (KS.ContainsString(DirectUI2010Paths, clrPath))
                    {
                        XamlType xamlType = this.GetXamlType(t);
                        xamlTypes.Add(xamlType);
                    }
                }
            }

            return xamlTypes;
        }

        private Type EnsureIsLmrType(Type type)
        {
            Type lmrType = type as MetadataOnlyCommonType;
            if (lmrType == null)
            {
                foreach (XamlTypeUniverse typeUniverse in this.directUISystem.XamlTypeUniverses)
                {
                    lmrType = typeUniverse.FindType(type.FullName);
                    if (lmrType != null)
                    {
                        break;
                    }
                }
            }

            return lmrType;
        }

        // Deals with resolving "using:" (case sensitive).
        private XamlType GetXamlTypeFromUsing(string xamlNamespace, string name, params XamlType[] typeArguments)
        {
            if (!xamlNamespace.HasUsingPrefix())
            {
                return null;
            }

            string typePath = xamlNamespace.StripUsingPrefix();
            XamlType xamlType = this.GetXamlTypeFromAssembliesAndPath(this.ReferenceAssemblies, typePath, name);

            // If the type doesn't come from the 4 'most popular' winmds load the 'extraReferences' if there were any and see if it's in one of them
            if (xamlType == null)
            {
                if (this.systemExtraAssemblies == null && this.systemExtraReferenceItems != null)
                {
                    // Load all the extra references
                    List<Assembly> assemblies = new List<Assembly>();
                    foreach (var item in this.systemExtraReferenceItems)
                    {
                        Assembly asm = CompileXamlInternal.TryLoadAssembly(item);
                        if (asm != null)
                        {
                            assemblies.Add(asm);
                        }
                    }

                    this.systemExtraAssemblies = DirectUIAssembly.Wrap(assemblies);
                }

                if (this.systemExtraAssemblies != null)
                {
                    xamlType = this.GetXamlTypeFromAssembliesAndPath(this.systemExtraAssemblies, typePath, name);
                }

            }

            return xamlType;
        }

        private XamlType GetDirectUIXamlType(string name)
        {
            if (this.directUISystem.PlatformAssemblies.Count == 0)
            {
                this.SchemaErrors.Add(new XamlSchemaError_WRTAssembliesMissing());
                return null;
            }

            XamlType xamlType;
            if (this.uiXamlCache.TryGetValue(name, out xamlType))
            {
                return xamlType;
            }

            // Lookup A DirectUI XAML Type in the ordinary way first
            Type type = this.TypeResolver.GetDirectUIType(name);
            if (type != null)
            {
                xamlType = this.GetXamlType(type);
            }

            // Look in all the special cases
            if (xamlType == null)
            {
                switch (name)
                {
                    case "Point":
                    case "Rect":
                    case "Size":
                        foreach (DirectUIAssembly asm in this.directUISystem.PlatformAssemblies)
                        {
                            xamlType = this.GetXamlTypeFromAsmAndPath(asm, KnownNamespaces.WindowsFoundation, name);
                            if (xamlType == null)
                            {
                                xamlType = this.GetXamlTypeFromAssembliesAndPath(this.managedProjectionAssemblies, KnownNamespaces.WindowsFoundation, name);
                                if (xamlType != null)
                                {
                                    break;
                                }
                            }
                        }
                        break;
                }
            }

            // Some types in Windows.Winmd are projected into other managed assemblies if we have such a DLL's and we haven't found the type yet, then look there
            if (xamlType == null && this.managedProjectionAssemblies.Count > 0)
            {
                xamlType = this.GetXamlTypeFromAssembliesAndPaths(this.managedProjectionAssemblies, DirectUISchemaContext.DirectUI2010Paths, name);
            }

            // if the type is still not found do the slow search through everything before returning null
            if (xamlType == null)
            {
                // Save some time by skipping known non-existant type names.
                switch (name)
                {
                    case "StaticResourceExtension":
                    case "ThemeResourceExtension":
                    case "TemplateBindingExtension":
                    case "BindingExtension":
                        break;

                    default:
                        xamlType = this.GetXamlTypeFromAssembliesAndPaths(this.ReferenceAssemblies, DirectUISchemaContext.DirectUI2010Paths, name);
                        break;
                }
            }

            // Cache the result, including caching null
            this.uiXamlCache.Add(name, xamlType);

            return xamlType;
        }

        private XamlType GetXamlTypeFromAssembliesAndPaths(IEnumerable<Assembly> asmList, IEnumerable<string> pathList, string name)
        {
            foreach (Assembly asm in asmList)
            {
                XamlType xamlType = null;
                if (FileHelpers.IsFacadeWinmd(asm, this.windowsSdkPath)) { continue; } // We know no Type used directly from Xaml in a UWP project can reside
                foreach (string path in pathList)
                {
                    xamlType = this.GetXamlTypeFromAsmAndPath(asm, path, name);
                    if (xamlType != null)
                    {
                        return xamlType;
                    }
                }
            }
            return null;
        }

        private XamlType GetXamlTypeFromAssembliesAndPath(IEnumerable<Assembly> asmList, string path, string name)
        {
            foreach (Assembly asm in asmList)
            {
                XamlType xamlType = null;
                if (FileHelpers.IsFacadeWinmd(asm, this.windowsSdkPath)) { continue; } // We know no Type used directly from Xaml in a UWP project can reside
                xamlType = this.GetXamlTypeFromAsmAndPath(asm, path, name);
                if (xamlType != null)
                {
                    return xamlType;
                }
            }
            return null;
        }

        private XamlType GetXamlTypeFromAsmAndPath(Assembly asm, string path, string name)
        {
            string fullTypeName = path + "." + name;
            Type type = asm.GetType(fullTypeName);
            if (type != null)
            {
                if (type.IsPublic || this.IsLocalAssembly(asm as DirectUIAssembly))
                {
                    return this.GetXamlType(type);
                }
            }

            return null;
        }

        private static string GetTypeExtensionName(string typeName)
        {
            return typeName + "Extension";
        }
    }
}