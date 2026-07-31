// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.IO;

using System.Reflection;  


namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Helpers for loading LMR assemblies into a universe.
    /// This provides various friendly Load() overloads using System.Reflection.Metadata.
    /// </summary>
    internal class Loader
    {
        // The universe that this loader is applying to.
        readonly private IMutableTypeUniverse m_universe;

        // Factory that modules are created with. Uses DefaultFactory instead of null.
        private IReflectionFactory m_factory;

        /// <summary>
        /// Options used when creating MetadataReader instances.
        /// Set to ApplyWindowsRuntimeProjections for managed languages (C#)
        /// so that WinRT types like IVector<T> are projected to IList<T>.
        /// Set to None for C++ to preserve raw WinRT types.
        /// </summary>
        public MetadataReaderOptions ReaderOptions { get; set; } = MetadataReaderOptions.None;

        /// <summary>
        /// Creates a metadata loader object and associate it with a universe.
        /// </summary>
        public Loader(IMutableTypeUniverse universe)
        {
            m_universe = universe;
        }

        /// <summary>
        /// Gets or sets the LMR Factory object associated with new modules.
        /// </summary>
        public IReflectionFactory Factory
        {
            get
            {
                if (m_factory == null)
                {
                    m_factory = new DefaultFactory(); 
                }
                return m_factory; 
            }
            set
            {                
                this.m_factory = value;                
            }
        }

        #region Various Load overloads

        /// <summary>
        /// Load an assembly at the given filename
        /// </summary>
        public Assembly LoadAssemblyFromFile(string file)
        {
            Assembly a = ReadAssemblyFromFile(file);
            m_universe.AddAssembly(a);
            return a;
        }

        public Assembly ReadAssemblyFromFile(string file)
        {
            string fullPath = Path.GetFullPath(file);
            var peReader = new PEReader(File.OpenRead(fullPath));
            var reader = peReader.GetMetadataReader(ReaderOptions);

            Assembly a = AssemblyFactory.CreateAssembly(m_universe, peReader, reader, this.Factory, fullPath);
            return a;
        }
                
        /// <summary>
        /// Load a multi-module assembly, explicitly specifying all modules.
        /// </summary>
        public Assembly LoadAssemblyFromFile(string manifestFile, string[] netModuleFiles)
        {
            string fullPath = Path.GetFullPath(manifestFile);
            var manifestPeReader = new PEReader(File.OpenRead(fullPath));
            var manifestReader = manifestPeReader.GetMetadataReader(ReaderOptions);

            PEReader[] netModulePeReaders = null;
            MetadataReader[] netModuleReaders = null;
            if ((netModuleFiles != null) && (netModuleFiles.Length > 0))
            {
                netModulePeReaders = new PEReader[netModuleFiles.Length];
                netModuleReaders = new MetadataReader[netModuleFiles.Length];
                for (int i = 0; i < netModuleFiles.Length; i++)
                {
                    netModulePeReaders[i] = new PEReader(File.OpenRead(netModuleFiles[i]));
                    netModuleReaders[i] = netModulePeReaders[i].GetMetadataReader(ReaderOptions);
                }
            }

            Assembly assembly = AssemblyFactory.CreateAssembly(
                m_universe, manifestPeReader, manifestReader,
                netModulePeReaders, netModuleReaders,
                this.Factory, fullPath, netModuleFiles);
            m_universe.AddAssembly(assembly);
            return assembly;
        }

        /// <summary>
        /// Open assembly from a byte-array containing the same contents as the file.
        /// </summary>
        public Assembly LoadAssemblyFromByteArray(byte[] data)
        {
            Debug.Assert(data != null);

            var peReader = new PEReader(System.Collections.Immutable.ImmutableArray.Create(data));
            var reader = peReader.GetMetadataReader(ReaderOptions);

            Assembly assembly = AssemblyFactory.CreateAssembly(m_universe, peReader, reader, this.Factory, String.Empty);
            m_universe.AddAssembly(assembly);
            return assembly;
        }


        /// <summary>
        /// Load just the module (without the assembly) given the filename.
        /// </summary>
        public MetadataOnlyModule LoadModuleFromFile(string moduleFileName)
        {
            var peReader = new PEReader(File.OpenRead(moduleFileName));
            var reader = peReader.GetMetadataReader(ReaderOptions);

            return new MetadataOnlyModule(m_universe, peReader, reader, this.Factory, moduleFileName);
        }

        /// <summary>
        /// Load a module with the given name as part of a multi-module assembly.
        /// </summary>
        public Module ResolveModule(Assembly containingAssembly, string moduleName)
        {
            if ((containingAssembly == null) || string.IsNullOrEmpty(containingAssembly.Location))
            {
                throw new ArgumentException("manifestModule needs to be associated with an assembly with valid location");
            }

            string assemblyFolder = Path.GetDirectoryName(containingAssembly.Location);
            string moduleLocation = Path.Combine(assemblyFolder, moduleName);

            var peReader = new PEReader(File.OpenRead(moduleLocation));
            var reader = peReader.GetMetadataReader(ReaderOptions);

            var module = new MetadataOnlyModule(m_universe, peReader, reader, this.Factory, moduleLocation);
            module.SetContainingAssembly(containingAssembly);

            return module;
        }

        #endregion // Various Load overloads
    } // end class Loader

}
