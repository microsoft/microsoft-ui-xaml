// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
namespace System.Reflection.Adds
{
    using Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal;
    using System.Reflection.Metadata;

    using System.Reflection;
    using Type = System.Type;

    /// <summary>
    /// Extra APIs on System.Reflection.Assembly
    /// </summary>
    internal interface IAssembly2
    {
        // Get the type universe that this assembly is in.
        // All Assemblies (and Types) and their transitive closure are contained in a single universe.
        // This should never return null. 
        ITypeUniverse TypeUniverse { get; }
    }

    /// <summary>
    /// Extra APIs on System.Reflection.Module
    /// </summary>
    internal interface IModule2
    {
        /// <summary>
        /// Gets number of rows in a metadata table.
        /// </summary>
        /// <param name="metadataTableIndex">Metadata table index.</param>
        /// <returns>Number of rows in the specified metadata table.</returns>
        int RowCount(MetadataTable metadataTableIndex);


        /// <summary>
        /// Get the assembly name for the given assembly reference handle in the metadata scope.
        /// </summary>
        AssemblyName GetAssemblyNameFromAssemblyRef(AssemblyReferenceHandle assemblyRefHandle);
    }


    // Additional helpers
    internal static class Helpers
    {
        /// <summary>
        /// Get the type universe from a type.
        /// </summary>
        /// <param name="type">type to get universe.</param>
        /// <returns>Returns null if type is not in a universe (such as with reflection types) 
        /// For ITypeProxy, get universe without resolving. </returns>
        public static ITypeUniverse Universe(Type type)
        {
            // If it's a type proxy (including type refs), get the universe via the type proxy interface
            // so that we don't accidentally resolve.
            ITypeProxy proxy = type as ITypeProxy;
            if (proxy != null)
            {
                return proxy.TypeUniverse;
            }
            
            // Not a proxy, we can safely get the assembly and resolve.
            Assembly a = type.Assembly;
            
            var ia2 = a as IAssembly2;
            if (ia2 == null)
            {
                return null;
            }

            return ia2.TypeUniverse;
        }


        /// <summary>
        /// Return a resolved version of the type, if applicable.
        /// </summary>
        /// <param name="type">type to ensure resolved</param>
        /// <returns>a resolved version of the type</returns>
        /// <remarks>LMR's deferred resolution is directly at odds with Reflection's eager validation.
        /// This can be used to resolve a type and force validation to occur to get reflection error semantics.
        /// </remarks>
        public static Type EnsureResolve(Type type)
        {
            while (true)
            {
                var proxy = type as ITypeProxy;
                if (proxy == null)
                    break;

                type = proxy.GetResolvedType();
            }
            return type;
        }
    }
}
