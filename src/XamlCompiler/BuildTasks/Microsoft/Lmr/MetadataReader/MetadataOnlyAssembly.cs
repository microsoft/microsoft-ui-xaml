// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// LMR Assembly

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Reflection.PortableExecutable;
using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Represent a System.Reflection.Assembly 
    /// </summary>
    internal class MetadataOnlyAssembly : Assembly, IAssembly2, IDisposable
    {
        /// <summary>
        /// m_modules[0] is always the manifest module. It's the only module for single-module 
        /// assemblies. For multi-module assemblies m_modules[1..n] contains netmodules.
        /// Modules have to be of type Module and not MetadataOnlyModule because Module resolver
        /// could return any Module type.
        /// </summary>
        readonly private Module[] m_modules;

        /// <summary>
        /// Same as m_modules[0]. Used to avoid casting in cases when we just need LMR
        /// specific information about manifest module. Manifest module always has to be
        /// of ManifestOnlyModule type.
        /// </summary>
        readonly private MetadataOnlyModule m_manifestModule;

        /// <summary>
        /// The file containing the manifest information for the assembly.
        /// </summary>
        readonly private string m_manifestFile;

        /// <summary>
        /// A profile of running Fib(20) using a naive recursive algorithm showed that caching the
        /// AssemblyName reduced execution speed by 40%. 
        /// </summary>
        readonly private AssemblyName m_name;


        /// <summary>
        /// Creates an instance of a single-module or multi-module assembly.
        /// </summary>
        /// <param name="manifestModule">Module containing manifest for an assembly.</param>
        /// <param name="manifestFile">File containing the manifest information.</param>
        internal MetadataOnlyAssembly(MetadataOnlyModule manifestModule, string manifestFile)
            : this(new MetadataOnlyModule[] { manifestModule }, manifestFile)
        {
        }

        /// <summary>
        /// Creates an instance of a multi-module assembly.
        /// </summary>
        /// <param name="modules">Array of modules that form a multi-module assembly. The first one
        /// must be the manifest module.</param>
        /// <param name="manifestFile">File containing the manifest information.</param>
        internal MetadataOnlyAssembly(MetadataOnlyModule[] modules, string manifestFile)
        {
            Debug.Assert(m_modules == null, "m_modules can be set only once.");

            MetadataOnlyAssembly.VerifyModules(modules);

            // We verified that manifest module is ok - save it.
            m_manifestModule = modules[0];
            m_name = AssemblyNameHelper.GetAssemblyName(m_manifestModule);
            m_manifestFile = manifestFile;

            // Ensure all modules passed in have their Assembly property set properly.
            foreach (MetadataOnlyModule module in modules)
            {
                module.SetContainingAssembly(this);
            }

            // Create temporary list of netmodules (including manifest module). This list will be
            // expanded if there are any netmodules that still need to be resolved.
            List<Module> currentModules = new List<Module>(modules);

            // Extract list of netmodule names from manifest.
            bool getResources = false;
            List<string> netModuleNames = MetadataOnlyAssembly.GetFileNamesFromFilesTable(m_manifestModule, getResources);

            // Load netmodules that are not passed in (if there are any).
            foreach (string netModuleName in netModuleNames)
            {
                if (currentModules.Find(i => i.Name.Equals(netModuleName, StringComparison.OrdinalIgnoreCase)) != null)
                {
                    // Already loaded - skip.
                    continue;
                }
                else
                {
                    // Resolver can return non-LMR modules. These modules can have their own way of
                    // setting assembly property but they need to know what the containing assembly is. 
                    Module newModule = m_manifestModule.AssemblyResolver.ResolveModule(this, netModuleName);

                    if (newModule == null)
                    {
                        throw new InvalidOperationException(Resources.ResolverMustResolveToValidModule);
                    }

                    if (newModule.Assembly != this)
                    {
                        throw new InvalidOperationException(Resources.ResolverMustSetAssemblyProperty);
                    }

                    currentModules.Add(newModule);
                }
            }

            m_modules = currentModules.ToArray();
        }

        #region Disposing
        /// <summary>        
        /// This should only be called in the context of disposing the parent Universe.
        /// Caller is responsible for thread safey here and to not dispose while another thread is using.
        /// Caller should not use after this has been diposed.        
        /// </summary>
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        // The bulk of the clean-up code is implemented in Dispose(bool)
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                // free managed resources
                // Dipsose any metadata objects held in this assembly.
                if (m_modules != null)
                {
                    foreach (Module m in m_modules)
                    {
                        IDisposable d = m as IDisposable;
                        if (d != null)
                        {
                            d.Dispose();
                        }
                    }
                }

            }
            // No native resources to free directly.

        }
        #endregion // Disposing



        /// <summary>
        /// Verifies that modules have these properties:
        ///     1) First module contains manifest.
        ///     2) All other modules (if provided) do not contain manifest.
        /// </summary>
        private static void VerifyModules(MetadataOnlyModule[] modules)
        {
            // Verify that there is manifest module passed.
            if ((modules == null) || modules.Length < 1)
            {
                throw new ArgumentException(Resources.ManifestModuleMustBeProvided);
            }

            // Verify that first module contains manifest.
            if (!HasAssemblyDefinition(modules[0]))
            {
                throw new ArgumentException(Resources.NoAssemblyManifest);
            }

            // Verify that all other modules (if provided) do not contain manifest.
            for (int i = 1; i < modules.Length; i++)
            {
                if (HasAssemblyDefinition(modules[i]))
                {
                    throw new ArgumentException(Resources.ExtraAssemblyManifest);
                }
            }

            // All checks passed - verification is complete.
        }

        /// <summary>
        /// Gets list of names of all dependent files from Files table based on manifest module. 
        /// </summary>
        /// <remarks>
        /// The CLI spec is not clear on where exactly netmodules should be listed: in ModuleRefs table or in 
        /// Files table, or both. C# compiler lists them in both places. That's what Serge Lidin's book on IL
        /// Assembler describes as correct. Dynamic modules generated with Reflection.Emit have both of these
        /// tables empty - until dynamic assembly is persisted to disk, when it becomes single module assembly.
        /// The only thing stored in a manifest of a dynamic assembly is information about assembly level
        /// custom attributes.
        /// 
        /// ModuleRefs table is hard to use since, in addition to net modules, it lists native DLL dependencies.
        /// There is no way to distinguish between native and managed binaries. The table only contains names
        /// like mscoree.dll or moduleA.netmodule. ModuleRefs table doesn't list full closure of dependencies 
        /// either. It only lists direct dependencies. E.g. if manifest module has dependency on module A, and 
        /// module A in turn has dependency on module B, ModuleRefs table will only have module A listed.
        /// 
        /// In contrast, Files table lists the full closure of all dependent netmodules, direct and indirect. 
        /// </remarks>
        /// <param name="manifestModule">Module with manifest that needs to be inspected.</param>
        /// <param name="getResources">Specifies whether the result includes resource files.</param>
        private static List<string> GetFileNamesFromFilesTable(MetadataOnlyModule manifestModule, bool getResources)
        {
            List<string> fileNames = new List<string>();
            var reader = manifestModule.RawReader;

            foreach (var fileHandle in reader.AssemblyFiles)
            {
                var file = reader.GetAssemblyFile(fileHandle);

                if (!getResources)
                {
                    // Skip resource files (files that do not contain metadata).
                    if (!file.ContainsMetadata)
                        continue;
                }

                fileNames.Add(reader.GetString(file.Name));
            }

            return fileNames;
        }

        public override int GetHashCode()
        {
            // Just use hash of the manifest module. It's highly unlikely that
            // two multi-module assemblies would have the same manifest module hashes.
            return m_modules[0].GetHashCode();
        }

        public override bool Equals(object obj)
        {
            // Can't check for specific type implementation since the assembly may be a proxy.
            // So compare using public properties.
            Assembly otherAssembly = obj as Assembly;
            if (otherAssembly == null)
                return false;

            // If two assmblies have the same manifest module they are the same.
            return this.ManifestModule.Equals(otherAssembly.ManifestModule);
        }

        #region Assembly Members

        /// <summary>
        /// Gets resource stream for a given resource name using type for namespace name.
        /// </summary>
        /// <remarks>We can't currently rely on Reflection to do this part since they
        /// call their internal API from their overload (instead of calling 
        /// GetManifestResourceStream(string name) overload.</remarks>
        public override Stream GetManifestResourceStream(Type type, String name)
        {
            StringBuilder resourceName = StringBuilderPool.Get();
            if (type == null)
            {
                if (name == null)
                {
                    throw new ArgumentNullException("type");
                }
            }
            else
            {
                String nameSpace = type.Namespace;
                if (nameSpace != null)
                {
                    resourceName.Append(nameSpace);
                    if (name != null)
                    {
                        resourceName.Append(Type.Delimiter);
                    }
                }
            }

            if (name != null)
            {
                resourceName.Append(name);
            }

            string text = resourceName.ToString();
            StringBuilderPool.Release(ref resourceName);

            return GetManifestResourceStream(text);
        }

        /// <summary>
        /// Gets resource stream for a given resource name.
        /// </summary>
        public override Stream GetManifestResourceStream(string name)
        {
            var reader = m_manifestModule.RawReader;
            var peReader = m_manifestModule.RawPEReader;

            // Find the manifest resource with the given name.
            ManifestResourceHandle foundHandle = default;
            bool found = false;
            foreach (var resHandle in reader.ManifestResources)
            {
                var resource = reader.GetManifestResource(resHandle);
                if (reader.GetString(resource.Name) == name)
                {
                    foundHandle = resHandle;
                    found = true;
                    break;
                }
            }

            // If resource doesn't exist we just return null. That's how Reflection works too.
            if (!found)
            {
                return null;
            }

            var manifestResource = reader.GetManifestResource(foundHandle);
            EntityHandle implementation = manifestResource.Implementation;

            if (implementation.IsNil)
            {
                // Resource is embedded in the current file.
                var resourcesDirectory = peReader.PEHeaders.CorHeader.ResourcesDirectory;
                var sectionData = peReader.GetSectionData(resourcesDirectory.RelativeVirtualAddress);
                long offset = manifestResource.Offset;
                var blobReader = sectionData.GetReader((int)offset, 4);
                int size = blobReader.ReadInt32();
                var dataReader = sectionData.GetReader((int)offset + 4, size);
                byte[] data = dataReader.ReadBytes(size);
                return new MemoryStream(data);
            }
            else if (implementation.Kind == HandleKind.AssemblyFile)
            {
                // Resource is in an external file referenced by the Files table.
                var fileHandle = (AssemblyFileHandle)implementation;
                var file = reader.GetAssemblyFile(fileHandle);
                string fileName = reader.GetString(file.Name);

                string path = Path.GetDirectoryName(this.Location);
                string fullFileName = Path.Combine(path, fileName);
                return new FileStream(fullFileName, FileMode.Open);
            }
            else if (implementation.Kind == HandleKind.AssemblyReference)
            {
                throw new NotImplementedException();
            }
            else
            {
                throw new ArgumentException(Resources.InvalidMetadata);
            }
        }

        public override string[] GetManifestResourceNames()
        {
            List<string> resourceNames = new List<string>();
            var reader = m_manifestModule.RawReader;

            foreach (var resHandle in reader.ManifestResources)
            {
                var resource = reader.GetManifestResource(resHandle);
                resourceNames.Add(reader.GetString(resource.Name));
            }

            return resourceNames.ToArray();
        }

        override public AssemblyName GetName()
        {
            // This should get inlined and brought over to Assembly
            // default is copiedName = false
            return m_name;
        }

        override public AssemblyName GetName(bool copiedName)
        {
            // true to set CodeBase to shadow copy; 
            throw new NotImplementedException();
        }

        String _assemblyFullName;

        override public string FullName
        {
            get
            {
                if (_assemblyFullName == null)
                {
                    _assemblyFullName = m_name.FullName;
                }
                return _assemblyFullName;
            }
        }

        override public string Location
        {
            get { return m_manifestFile; }
        }

        public override bool ReflectionOnly
        {
            get { return true; }
        }

        override public Type[] GetExportedTypes()
        {
            // Return all visible types.
            Type[] allTypes = this.GetTypes();

            List<Type> list = new List<Type>();
            foreach (var t in allTypes)
            {
                if (t.IsVisible)
                    list.Add(t);
            }
            return list.ToArray();

        }

        override public Type GetType(string name)
        {
            return this.GetType(name, false, false);
        }

        override public Type GetType(string name, bool throwOnError)
        {
            return this.GetType(name, throwOnError, false);
        }


        private Dictionary<string, Type> _typeCache = new Dictionary<string, Type>();

        public override Type GetType(string name, bool throwOnError, bool ignoreCase)
        {
            Type type = null;

            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            string cacheName = (!ignoreCase) ? name : name.ToLower(CultureInfo.CurrentCulture);
            if (!_typeCache.TryGetValue(cacheName, out type))
            {
                // Check all modules. We don't want to throw on
                // no match found since we have other modules to inspect.
                for (int i = 0; i < m_modules.Length; i++)
                {
                    type = m_modules[i].GetType(name, false, ignoreCase);
                    if (type != null)
                    {
                        break;
                    }
                }

                if (type == null)
                {
                    // If no match found, check for policy about type forwarding.
                    type = m_manifestModule.Policy.TryTypeForwardResolution(this, name, ignoreCase);
                }
                if (type != null)
                {
                    _typeCache.Add(cacheName, type);
                }
            }
            if (type != null)
            {
                return type;
            }

            // No match found.
            if (throwOnError)
            {
                throw new TypeLoadException(String.Format(
                    CultureInfo.InvariantCulture, Resources.CannotFindTypeInModule, name, m_modules[0].ScopeName));
            }

            return null;
        }

        /// <summary>
        /// Gets all types on the assembly. 
        /// </summary>
        override public Type[] GetTypes()
        {
            List<Type> types = new List<Type>();
            foreach (Module module in m_modules)
            {
                types.AddRange(module.GetTypes());
            }

            return types.ToArray();
        }

        /// <summary>
        /// Gets module with the specified name. Returns null if there is no such module.
        /// </summary>
        public override Module GetModule(string name)
        {
            foreach (Module module in m_modules)
            {
                if (module.ScopeName.Equals(name, StringComparison.OrdinalIgnoreCase))
                {
                    return module;
                }
            }

            // No matches found.
            return null;
        }

        public override Module[] GetModules(bool getResourceModules)
        {
            return m_modules;
        }

        public override Module[] GetLoadedModules(bool getResourceModules)
        {
            return m_modules;
        }

        public override Module ManifestModule
        {
            get
            {
                return m_modules[0];
            }
        }

        internal MetadataOnlyModule ManifestModuleInternal
        {
            get
            {
                return m_manifestModule;
            }
        }

        public override string CodeBase 
        {
            get 
            {
                return GetCodeBaseFromManifestModule(m_manifestModule);
            }
        }

        /// <summary>
        /// The method returns a string representing the CodeBase property
        /// of an Assembly or AssemblyName from the manifest module.
        /// </summary>
        internal static string GetCodeBaseFromManifestModule(MetadataOnlyModule manifestModule) 
        {
            string modulePath = manifestModule.FullyQualifiedName;
            
            if (!Utility.IsValidPath(modulePath)) 
            {
                // The modulePath could be empty if the assembly is loaded from binary data
                // Reflection returns the caller's assembly's CodeBase in that case. 
                return String.Empty;
            } 
            else 
            {
                // Need to format the module path to match the result in the Reflection.
                try
                {
                    return new Uri(modulePath).ToString();
                }
                catch (Exception e)
                {
                    Debug.Assert(false, "Unexpected exception thrown by Uri code: " + e.Message);
                    throw;
                }
            }
        }

        override public MethodInfo EntryPoint
        {
            get
            {
                var peReader = m_manifestModule.RawPEReader;
                int entryPointToken = peReader.PEHeaders.CorHeader.EntryPointTokenOrRelativeVirtualAddress;

                if (entryPointToken == 0)
                {
                    // No entry point token. Common for dlls.
                    return null;
                }

                // See Ecma-335 II 25.3.3.2 for details. This can be a MethodDef or a File token.
                var handle = MetadataTokens.EntityHandle(entryPointToken);
                switch (handle.Kind)
                {
                    case HandleKind.AssemblyFile:
                        // Haven't implemented the file case.
                        throw new NotImplementedException();
                    case HandleKind.MethodDefinition:
                        {
                            // Token type should be a MethodDef to a MethodInfo (not a ctor).
                            MethodBase method = this.ManifestModule.ResolveMethod(entryPointToken);
                            Debug.Assert(method != null);
                            return (MethodInfo)method;
                        }
                    default:
                        throw new InvalidOperationException(Resources.InvalidMetadata);
                }
            }
        }

        public override string ImageRuntimeVersion
        {
            get
            {
                return m_manifestModule.GetRuntimeVersion();
            }
        }

        #endregion // region Assembly Members


        /// <summary>
        /// Returns true if the module contains an assembly manifest (i.e., is the manifest module).
        /// </summary>
        internal static bool HasAssemblyDefinition(MetadataOnlyModule module)
        {
            return module.RawReader.IsAssembly;
        }

        

        public override FileStream[] GetFiles(bool getResourceModules)
        {
            List<string> filenames = new List<string>();
            //Return all the module files in the assembly.
            foreach (Module m in m_modules)
            {
                filenames.Add(m.FullyQualifiedName);
            }
            if (getResourceModules)
            {
                //get all the resource files.
                string directory = Path.GetDirectoryName(m_manifestFile);

                foreach (string filename in MetadataOnlyAssembly.GetFileNamesFromFilesTable(m_manifestModule, true))
                {
                    //The filename in the metadata doesn't contain the fullpath.
                    //Assume that the files in the assembly are in the same directory as the manifest file.
                    filenames.Add(Path.Combine(directory, filename));
                }
            }
            return ConvertFileNamesToStreams(filenames.ToArray());
        }

        public override FileStream GetFile(string name)
        {
            Module m = GetModule(name);
            if (m == null)
                return null;

            return new FileStream(m.FullyQualifiedName,
                                  FileMode.Open,
                                  FileAccess.Read, FileShare.Read);
        }

        static private FileStream[] ConvertFileNamesToStreams(string[] filenames)
        {
            return Array.ConvertAll<string, FileStream>(filenames, n => new FileStream(n, FileMode.Open, FileAccess.Read, FileShare.Read));
        }

        IList<CustomAttributeData> _customAttributeDataCache;

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            if (_customAttributeDataCache == null)
            {
                // The assembly definition handle is always token 0x20000001.
                var assemblyDefHandle = MetadataTokens.EntityHandle(0x20000001);
                _customAttributeDataCache = m_manifestModule.GetCustomAttributeData(assemblyDefHandle);
            }
            return _customAttributeDataCache;
        }

        public override AssemblyName[] GetReferencedAssemblies()
        {
            // Get the list of assemblies that this assembly references.
            // The references are just stored in the metadata. The TypeUniverse (fusion) is responsible 
            // for binding the assembly names to actual assemblies. 
            // See IIB.21.5 for more information about the Assembly Reference table.
            // Traversing the list of references should absolutely not cause any references to be loaded.

            var reader = m_manifestModule.RawReader;
            List<AssemblyName> list = new List<AssemblyName>();

            foreach (var refHandle in reader.AssemblyReferences)
            {
                var name = AssemblyNameHelper.GetAssemblyNameFromRef(refHandle, m_manifestModule);
                list.Add(name);
            }

            return list.ToArray();
        }

        #region IAssembly2 Members

        public ITypeUniverse TypeUniverse
        {
            get
            {
                return m_manifestModule.AssemblyResolver;
            }
        }

        #endregion
    }
}
