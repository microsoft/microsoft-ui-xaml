// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Collections.Generic;
using System.Reflection.Metadata;
using System.Runtime.InteropServices;


namespace System.Reflection.Adds
{
    using System.Reflection;
    

    // Describes a TypeReference (TypeRef row in metadata, in contrast to TypeDef).
    // The CLR Reflection API does not expose TypeRefs.
    // We try to bridge the 2 different models by having Type objects that implement both this TypeRef
    // interface and code:Type, and then forward all non-ref type calls to the resolved type. 
    // 
    // TypeRef properties are:
    // - TypeReferenceHandle
    // - resolution scope (EntityHandle — can be TypeRef, AssemblyRef, Module, ModuleRef)
    // - fullname
    // TypeRefs are then resolved by resolving the resolution scope and looking up the fullname in the new scope.
    // Except for GetResolvedType(), all the properties on this interface can succeed without invoking the
    // resolver, because they're returning the raw TypeRef properties.
    // 
    // TypeRef impls may forward all calls to the resolved type    
    internal interface ITypeReference : ITypeProxy
    {
        #region Properties directly corresponding to TypeReference row
        // code:ITypeReference.TypeRefHandle, code:ITypeReference.FullName, and
        // code:ITypeReference.ResolutionScope are properties directly on the TypeRef and should not require
        // doing resolution (calling code:ITypeReference.GetResolvedType) to answer.
        
        // The handle for the TypeRef, as opposed to the handle for the resolved type (which would be a TypeDef)
        TypeReferenceHandle TypeRefHandle
        {
            get;
        }

        // The name, including namespace, but not including enclosing types.
        // This is the name stored in the metadata table.
        string RawName
        {
            get;
        }

        // An AssemblyRef, Module, ModuleRef, or TypeRef handle for where the TypeRef should be resolved in.
        EntityHandle ResolutionScope
        {
            get;
        }
        #endregion // direct properties.

        // The fullname of the type from the metadata table. This includes namespaces and outer types.
        // This can be determined without doing any resolution. A TypeRef to a nested type has an ResolutionScope
        // of another TypeRef.
        // This should be the same as calling GetResolvedType().FullName. 
        string FullName
        {
            get;
        }

        // A backpointer to the scope the TypeRef is used in. 
        Module DeclaringScope
        {
            get;
        }
    }

} // namespace
