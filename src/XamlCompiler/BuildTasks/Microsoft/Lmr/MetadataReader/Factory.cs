// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Factory for instantiating LMR objects


using System.Reflection.Adds;
using System.Reflection.Metadata;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

using System.Reflection;  
using Type = System.Type;


namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{    
    /// <summary>
    /// Factory object supplied to LMR, allow creation of custom derived objects.
    /// See code:DefaultFactory
    /// </summary>
    internal interface IReflectionFactory
    {
        // Create for a TypeDef handle
        MetadataOnlyCommonType CreateSimpleType(MetadataOnlyModule scope, TypeDefinitionHandle typeDef);

        // Create for a TypeDef handle with type arguments
        MetadataOnlyCommonType CreateGenericType(MetadataOnlyModule scope, TypeDefinitionHandle typeDef, Type[] typeArgs);

        // Create a multidimensional array type.
        // This gets invoked from Type.MakeArrayType(int rank)
        // The CLR makes a distinction between vectors (that is, one-dimensional 
        // arrays that are always zero-based) and multidimensional arrays. 
        // A vector, which always has only one dimension, is not the same
        // as a multidimensional array that happens to have only one dimension.
        // You cannot use this method overload to create a vector type; if rank
        // is 1, this method overload returns a multidimensional array type that
        // happens to have one dimension.
        MetadataOnlyCommonType CreateArrayType(MetadataOnlyCommonType elementType, int rank);

        // Create a vector type.
        // This gets invoked from Type.MakeArrayType()        
        MetadataOnlyCommonType CreateVectorType(MetadataOnlyCommonType elementType);

        // Create a modifier. T-->T&
        // This gets invoked from Type.MakeByRefType() 
        MetadataOnlyCommonType CreateByRefType(MetadataOnlyCommonType type);

        // Create a modifier T --> T*
        // This gets invoked from Type.MakePointerType()        
        MetadataOnlyCommonType CreatePointerType(MetadataOnlyCommonType type);

        /// <summary>
        /// Creates a type variable from a GenericParameter handle.
        /// </summary>
        MetadataOnlyTypeVariable CreateTypeVariable(MetadataOnlyModule resolver, GenericParameterHandle genericParam);

        // Create a field from a FieldDefinition handle.
        MetadataOnlyFieldInfo CreateField(MetadataOnlyModule resolver, FieldDefinitionHandle fieldDef, Type[] typeArgs, Type[] methodArgs);

        // Create a propertyInfo from a PropertyDefinition handle.
        MetadataOnlyPropertyInfo CreatePropertyInfo(MetadataOnlyModule resolver, PropertyDefinitionHandle propDef, Type[] typeArgs, Type[] methodArgs);

        MetadataOnlyEventInfo CreateEventInfo(MetadataOnlyModule resolver, EventDefinitionHandle eventDef, Type[] typeArgs, Type[] methodArgs);


        /// <summary>
        /// Hook creating a MethodInfo or ConstructorInfo based on a MethodDefinition handle.
        /// This does not work for member references.
        /// </summary>
        /// <param name="resolver">module that the handle is scoped to</param>
        /// <param name="methodDef">a MethodDefinition handle for a Constructor or MethodInfo.</param>
        /// <param name="typeArgs">type arguments for a generic method. May be null or 0-length. </param>
        /// <param name="methodArgs">method arguments for a generic method. May be null or 0-length. </param>
        /// <returns>a MethodBase</returns>
        MethodBase CreateMethodOrConstructor(MetadataOnlyModule resolver, MethodDefinitionHandle methodDef, Type[] typeArgs, Type[] methodArgs);


        /// <summary>
        /// Allow creating an IL method body for the given method. A method may not have a method body (such
        /// as a pinvoke). So this has 3 states:
        /// 1. If the factory does not hook, return false. Ignore body parameter. 
        /// 2. If the factory does hook, return true
        ///     2a. and there is no method body, set body=null.
        ///     2b. if there is a method body, set body= created instance of the body.
        ///  
        /// The factory can use the code:LMRMethodBody to help implement the method body.
        /// </summary>
        /// <param name="method">method to create the body for. </param>
        /// <param name="body">null or newly created method body.</param>
        /// <returns>true if the body is valid, else false.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", MessageId = "1#")]
        bool TryCreateMethodBody(MetadataOnlyMethodInfo method, ref MethodBody body);        

        
                
        /// <summary>
        /// Create for a TypeReference handle.
        /// - could implementing caching for TypeRefs
        /// - could eagerly resolve Types (so return type may not implement ITypeReference)
        /// - could return an ITypeReference with an arbitrary resolution policy)
        /// 
        /// Since a TypeRef can resolve to a non-LMR type, the return type here must be
        /// System.Type instead of a LMR-specific type.
        /// </summary>
        /// <param name="scope">the module that the handle is valid in </param>
        /// <param name="typeRef">a TypeReference handle within the module</param>
        /// <returns>a Type object corresponding to the type reference. The factory may eagerly resolve the handle, 
        /// or return a proxy object that does deferred resolution. </returns>
        Type CreateTypeRef(MetadataOnlyModule scope, TypeReferenceHandle typeRef);

        /// <summary>
        /// Create for a TypeReference handle that occurred in a signature.
        /// The rawTypeKind carries CLASS vs VALUETYPE disambiguation from the signature
        /// (0=unknown, 1=class, 2=valuetype per SRM SignatureTypeKind).
        /// </summary>
        /// <param name="scope">the module that the handle is valid in </param>
        /// <param name="typeRef">a TypeReference handle within the module</param>
        /// <param name="rawTypeKind">the raw type kind byte from the signature (SignatureTypeKind)</param>
        /// <returns>a Type object corresponding to the type reference. The factory may eagerly resolve the handle, 
        /// or return a proxy object that does deferred resolution. </returns>
        Type CreateSignatureTypeRef(MetadataOnlyModule scope, TypeReferenceHandle typeRef, byte rawTypeKind);

        /// <summary>
        /// Create for a TypeSpecification handle. This is similar to a TypeRef in that it can create a proxy type.
        /// </summary>
        /// <param name="scope">module scope that the handle is valid in. </param>
        /// <param name="typeSpec">a TypeSpecification handle in that scope</param>
        /// <param name="typeArgs">the generic type args for resolving vars</param>
        /// <param name="methodArgs">the generic method args for resolving mvars.</param>
        Type CreateTypeSpec(MetadataOnlyModule scope, TypeSpecificationHandle typeSpec, Type[] typeArgs, Type[] methodArgs);
    }
}
