// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Helper for extracting AssemblyName from metadata using System.Reflection.Metadata

using System;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;

using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Helper class for extracting assembly names from metadata.
    /// With System.Reflection.Metadata, this is largely built-in via
    /// AssemblyDefinition.GetAssemblyName() and AssemblyReference.GetAssemblyName().
    /// </summary>
    internal static class AssemblyNameHelper
    {
        /// <summary>
        /// Get the AssemblyName for the assembly defined in this module.
        /// </summary>
        public static AssemblyName GetAssemblyName(MetadataOnlyModule module)
        {
            var reader = module.RawReader;
            if (!reader.IsAssembly)
            {
                // Module doesn't have an assembly definition (it's a netmodule)
                return null;
            }
            var assemblyDef = reader.GetAssemblyDefinition();
            return assemblyDef.GetAssemblyName();
        }

        /// <summary>
        /// Get the AssemblyName for an assembly reference in this module.
        /// </summary>
        public static AssemblyName GetAssemblyNameFromRef(AssemblyReferenceHandle assemblyRefHandle, MetadataOnlyModule module)
        {
            var reader = module.RawReader;
            var assemblyRef = reader.GetAssemblyReference(assemblyRefHandle);
            return assemblyRef.GetAssemblyName();
        }
    }
}
