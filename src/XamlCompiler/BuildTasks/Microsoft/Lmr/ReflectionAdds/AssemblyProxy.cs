// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
namespace System.Reflection.Adds
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Reflection.Adds;
    using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
    using System.IO;

    using System.Reflection;  

    /// <summary>
    /// An assembly proxy object. All methods just forward to the resolved assembly.
    /// Derived classes can implemen the resolution function.
    /// This is a general purpose object that can work with different type providers. 
    /// Explicitly have a DebuggerDisplay here to avoid calling the ToString() method,
    /// which could throw exceptions when the resolution fails. 
    /// </summary>
    [DebuggerDisplay("AssemblyProxy")]
    internal abstract class AssemblyProxy : Assembly, IAssembly2, IDisposable
    {
        private readonly ITypeUniverse m_universe;

        // Cached result of the assembly we resolve to
        private Assembly m_assembly;

        protected AssemblyProxy(ITypeUniverse universe)
        {
            Debug.Assert(universe != null);
            m_universe = universe;
        }

        public Assembly GetResolvedAssembly()
        {
            if (m_assembly == null)
            {
                m_assembly = GetResolvedAssemblyWorker();
                if (m_assembly == null)
                {
                    throw new UnresolvedAssemblyException(string.Format(
                       CultureInfo.InvariantCulture, Resources.UniverseCannotResolveAssembly, GetNameWithNoResolution()));
                }
            }
            return m_assembly;
        }

        public ITypeUniverse TypeUniverse
        {
            get { return this.m_universe; }
        }

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
                if (m_assembly != null)
                {
                    IDisposable d = m_assembly as IDisposable;
                    if (d != null)
                    {
                        d.Dispose();
                    }
                }
            }
            // No native resources to free directly.

        }

        #region Equality
        
        public override int GetHashCode()
        {
            return GetResolvedAssembly().GetHashCode();
        }
        public override string ToString()
        {
            return GetResolvedAssembly().ToString();
        }

        public override bool Equals(object obj)
        {
            return GetResolvedAssembly().Equals(obj);
        
        }
        #endregion // Equality

        /// <summary>
        /// Implemented by derived class to resolve the assembly.
        /// </summary>
        protected abstract Assembly GetResolvedAssemblyWorker();
        protected abstract AssemblyName GetNameWithNoResolution();

        public override AssemblyName GetName()
        {
            return GetResolvedAssembly().GetName();
        }

        public override AssemblyName GetName(bool copiedName)
        {
            return GetResolvedAssembly().GetName(copiedName);
        }

        public override string FullName
        {
            get { return GetResolvedAssembly().FullName; }
        }

        public override string Location
        {
            get { return GetResolvedAssembly().Location; }
        }

        public override Type[] GetExportedTypes()
        {
            return GetResolvedAssembly().GetExportedTypes();
        }

        public override bool ReflectionOnly
        {
            get { return true; }
        }

        public override Type[] GetTypes()
        {
            return GetResolvedAssembly().GetTypes();
        }

        public override string CodeBase 
        {
            get { return GetResolvedAssembly().CodeBase; }
        }

        public override string EscapedCodeBase {
            get { return GetResolvedAssembly().EscapedCodeBase; }
        }

        public override MethodInfo EntryPoint
        {
            get { return GetResolvedAssembly().EntryPoint; }
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return GetResolvedAssembly().GetCustomAttributesData();
        }

        public override object[] GetCustomAttributes(bool inherit)
        {
            return GetResolvedAssembly().GetCustomAttributes(inherit);
        }

        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return GetResolvedAssembly().GetCustomAttributes(attributeType, inherit);
        }

        public override Type GetType(String name, bool throwOnError, bool ignoreCase)
        {
            return GetResolvedAssembly().GetType(name, throwOnError, ignoreCase);
        }

        public override Module GetModule(String name)
        {
            return GetResolvedAssembly().GetModule(name);
        }

        public override Module[] GetLoadedModules(bool getResourceModules)
        {
            return GetResolvedAssembly().GetLoadedModules(getResourceModules);
        }

        public override Module[] GetModules(bool getResourceModules)
        {
            return GetResolvedAssembly().GetModules(getResourceModules);
        }

        public override Module ManifestModule
        {
            get
            {
                return GetResolvedAssembly().ManifestModule;
            }
        }

        public override AssemblyName[] GetReferencedAssemblies()
        {
            return GetResolvedAssembly().GetReferencedAssemblies();
        }

        public override Assembly GetSatelliteAssembly(System.Globalization.CultureInfo culture)
        {
            return GetResolvedAssembly().GetSatelliteAssembly(culture);
        }

        public override Assembly GetSatelliteAssembly(System.Globalization.CultureInfo culture,
                                               Version version)
        {
            return GetResolvedAssembly().GetSatelliteAssembly(culture, version);
        }

        public override bool GlobalAssemblyCache
        {
            get
            {
                return GetResolvedAssembly().GlobalAssemblyCache;
            }
        }

        public override Int64 HostContext
        {
            get
            {
                return GetResolvedAssembly().HostContext;
            }
        }

        public override Stream GetManifestResourceStream(Type type, String name)
        {
            return GetResolvedAssembly().GetManifestResourceStream(type, name);
        }

        public override Stream GetManifestResourceStream(String name)
        {
            return GetResolvedAssembly().GetManifestResourceStream(name);
        }

        public override String[] GetManifestResourceNames()
        {
            return GetResolvedAssembly().GetManifestResourceNames();
        }

        public override ManifestResourceInfo GetManifestResourceInfo(String resourceName)
        {
            return GetResolvedAssembly().GetManifestResourceInfo(resourceName);
        }

        public override FileStream[] GetFiles(bool getResourceModules)
        {
            return GetResolvedAssembly().GetFiles(getResourceModules);
        }

        public override FileStream[] GetFiles()
        {
            return GetResolvedAssembly().GetFiles();
        }

        public override FileStream GetFile(String name)
        {
            return GetResolvedAssembly().GetFile(name);
        }


    }
}
