// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Describes a "TypeUniverse".


namespace System.Reflection.Adds
{
    using System.Reflection.Metadata;
    using System.Reflection;
    using Type = System.Type;

    /// <summary>
    /// This describes a "TypeUniverse".
    /// This is a full transitive closure of all possible types, along with key operations on the universe
    /// such as type-lookup and resolution between scopes. 
    /// Universes are completely isolated and so a host can maintain multiple type universes.
    /// 
    /// Some functionality is explicitly not on the type universe:
    /// - Enumerate all assemblies: this can impede lazily loading assemblies into the universe. 
    /// </summary>
    /// <remarks>
    /// An ITypeUniverse is read-only and doesn't provide loading facilities. See IMutableTypeUniverse for loading. 
    /// </remarks>
    internal interface ITypeUniverse
    {
        /// <summary>
        /// Callback to the universe to resolve the builtin types.
        /// 
        /// From standard I.8.2.2: Builtin types, determined by the virtual execution system.
        /// Also, encoding for builtin types must use the short form (I4) instead of the long form
        /// (TypeRef("System.Int32", mscorlib).        
        /// Get a type for a given CLR element type (eg, COR_ELEMENT_TYPE I4)                
        /// This is redundant with GetSystemAssembly().GetType(NameFrom(elementType), true, false)), however
        /// the lookup by type code allows a more efficient lookup without having to go through strings.
        /// </summary>
        Type GetBuiltInType(CorElementType elementType);

        /// <summary>
        /// Get a type from the given full name, (eg  "System.Object").
        /// This is only valid for types in mscorlib.
        /// This is redundant with GetSystemAssembly().GetType()
        /// </summary>        /// 
        Type GetTypeXFromName(string fullName);

        /// <summary>
        /// The CLR execution implicitly requires a single system assembly.  On desktop CLR, this is "mscorlib.dll".
        /// This is used for things including:
        /// - resolving built-in types such as I4 --> "mscorlib!System.Int32". 
        /// - providing critical types such as System.Object and System.Array (base class for all Arrays)
        /// - providing System.ValueType, System.Enum, System.Delegate
        /// Many APIs on reflection require knowing the system assembly such as Type.IsEnum
        ///
        /// The type-universe can fail to implement this, but many APIs that require knowledge of the system
        /// assembly will fail. APIs that don't innately require that knowledge should still be able to work.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1024:UsePropertiesWhereAppropriate")]
        Assembly GetSystemAssembly();

        /// <summary>
        /// For UAP scenarios, mscorlib type forwards all types to system.runtime.dll. Checks previously dependent on the value for type.Assembly
        /// Should account for the new dll as well.
        /// </summary>
        /// <returns></returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1024:UsePropertiesWhereAppropriate")]
        Assembly GetSystemRuntimeAssembly();

        #region Assembly and Module Resolution

        // The ResolveAssembly and Resolve Module overloads are used to resolve assemblies  and modules 
        // within the universe. All overloads have the following invariants:
        // - Throw on error, don't return null.
        // - This may load a new assembly/module into the universe and return that.
        // - Must return an assembly/module within the same universe.         

        /// <summary>
        /// Simple assembly resolution by name. This is similar to Assembly.Load(AssemblyName).         
        /// This can not properly model multiple loader contexts (see Assembly.LoadFrom), and therefore it
        /// can not fully model an existing process. 
        /// </summary>
        Assembly ResolveAssembly(AssemblyName name);

        /// <summary>
        /// Simple assembly resolution by name. This is similar to Assembly.Load(AssemblyName).         
        /// This can not properly model multiple loader contexts (see Assembly.LoadFrom), and therefore it
        /// can not fully model an existing process. 
        /// </summary>
        /// <param name="name">Assembly name to resolve</param>
        /// <param name="throwOnError">True to throw an exception if the assembly cannot be found otherwise
        /// false to return null when the assembly cannot be found</param>
        /// <returns></returns>
        Assembly ResolveAssembly(AssemblyName name, bool throwOnError);

        /// <summary>
        /// Fine-grain Assembly resolution using the calling assembly and the assembly reference handle. 
        /// Scope represents a module containing metadata.
        /// assemblyRefHandle is an assembly reference handle valid in the given scope. 
        /// </summary>
        Assembly ResolveAssembly(Module scope, AssemblyReferenceHandle assemblyRefHandle);

        /// <summary>
        /// Test whether assembly resolution would resolve the specified name to the given assembly
        /// if it was part of this universe.
        /// This doesn't actually do a resolution, just uses the same algorithm that ResolveAssembly
        /// uses to compare names to Assembly definitions.
        /// Note that the caller would need to also check that the supplied assembly is actually part 
        /// of this universe before knowing that ResolveAssembly would succeed on the supplied name.
        /// </summary>
        /// <param name="name">The assembly name that would be resolved</param>
        /// <param name="assembly">The assembly to test</param>
        /// <returns>True if (and only if) the name would resolve to the supplied assembly if it were 
        /// part of this universe</returns>
        bool WouldResolveToAssembly(AssemblyName name, Assembly assembly);

        /// <summary>
        /// Netmodule resolution by name for multi-module assemblies. 
        /// </summary>
        /// <param name="containingAssembly">Multi-module assembly resolved module will be part of. 
        /// It should already contain manifest module. 
        /// ITypeUniverse implementation can use information like assembly location to determine
        /// where dependent netmodules should be loaded from.</param>
        /// <param name="moduleName">Name of a net module that needs to be resolved. This name comes from
        /// the manifest module and its Files table. It's equal to the module's Name property.</param>
        Module ResolveModule(Assembly containingAssembly, string moduleName);

        /// <summary>
        /// Resolves a Windows Runtime type using windows metadata rules. Example: if the type name
        /// is RootNamespace.SubNamespace.TypeName first look in RootNamespace.Subnamespace.winmd for 
        /// TypeName if the assembly does not exist or it is not found look in RootNamespace.winmd.
        /// </summary>
        /// <param name="typeName">The type name to resolve</param>
        /// <param name="throwOnError">True to throw an exception if the type cannot be found otherwise
        /// false to return null when the type cannot be found</param>
        /// <param name="ignoreCase">true to perform a case-insensitive search for typeName, false to 
        /// perform a case-sensitive search for typeName.</param>
        /// <returns></returns>
        Type ResolveWindowsRuntimeType(string typeName, bool throwOnError, bool ignoreCase);

        #endregion // Assembly and Module Resolution
    }


}
