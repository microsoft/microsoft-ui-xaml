// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Factory for creating LMR assemblies

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using System.IO;
using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

using System.Reflection;


namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Public factory to allow callers to create an Assembly implementation without needing to expose the assembly
    /// implementation directly. 
    /// </summary>
    internal static class AssemblyFactory
    {
        /// <summary>
        /// Pass in TokenResolver (module) so that the caller can create a derived instance.
        /// This only supports creating single-module assemblies.
        /// </summary>
        public static Assembly CreateAssembly(MetadataOnlyModule manifestModule, string manifestFile)
        {
            MetadataOnlyAssembly a = new MetadataOnlyAssembly(manifestModule, manifestFile);
            return a;
        }

        /// <summary>
        /// Create a single-module assembly from a file path.
        /// </summary>
        public static Assembly CreateAssembly(ITypeUniverse typeUniverse, string filePath)
        {
            return CreateAssembly(typeUniverse, filePath, new DefaultFactory());
        }

        /// <summary>
        /// Create a single-module assembly from a file path with a custom factory.
        /// </summary>
        public static Assembly CreateAssembly(ITypeUniverse typeUniverse, string filePath, IReflectionFactory factory,
            MetadataReaderOptions readerOptions = MetadataReaderOptions.None)
        {
            var peReader = new PEReader(File.OpenRead(filePath));
            var reader = peReader.GetMetadataReader(readerOptions);
            var module = new MetadataOnlyModule(typeUniverse, peReader, reader, factory, filePath);
            return new MetadataOnlyAssembly(module, filePath);
        }

        /// <summary>
        /// Create a single-module assembly from pre-opened PEReader/MetadataReader.
        /// </summary>
        public static Assembly CreateAssembly(ITypeUniverse typeUniverse, PEReader peReader, MetadataReader reader, string manifestFile)
        {
            return CreateAssembly(typeUniverse, peReader, reader, new DefaultFactory(), manifestFile);
        }

        /// <summary>
        /// Create a single-module assembly from pre-opened PEReader/MetadataReader with a custom factory.
        /// </summary>
        public static Assembly CreateAssembly(ITypeUniverse typeUniverse, PEReader peReader, MetadataReader reader, IReflectionFactory factory, string manifestFile)
        {
            var module = new MetadataOnlyModule(typeUniverse, peReader, reader, factory, manifestFile);
            return new MetadataOnlyAssembly(module, manifestFile);
        }

        /// <summary>
        /// Create a multi-module assembly from pre-opened PEReader/MetadataReader pairs.
        /// </summary>
        public static Assembly CreateAssembly(
            ITypeUniverse typeUniverse,
            PEReader manifestPeReader,
            MetadataReader manifestReader,
            PEReader[] netModulePeReaders,
            MetadataReader[] netModuleReaders,
            IReflectionFactory factory,
            string manifestFile,
            string[] netModuleFiles)
        {
            int numberOfModules = 1;
            if (netModulePeReaders != null)
            {
                numberOfModules += netModulePeReaders.Length;
            }

            MetadataOnlyModule[] modules = new MetadataOnlyModule[numberOfModules];

            MetadataOnlyModule manifestModule = new MetadataOnlyModule(typeUniverse, manifestPeReader, manifestReader, factory, manifestFile);
            modules[0] = manifestModule;

            if (numberOfModules > 1)
            {
                for (int i = 0; i < netModulePeReaders.Length; i++)
                {
                    modules[i + 1] = new MetadataOnlyModule(typeUniverse, netModulePeReaders[i], netModuleReaders[i], factory, netModuleFiles[i]);
                }
            }

            Assembly a = new MetadataOnlyAssembly(modules, manifestFile);
            return a;
        }
    }
}
