// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Collections.Generic;
using System.Reflection.Metadata;
using System.Runtime.InteropServices;


namespace System.Reflection.Adds
{
    using System.Reflection;


    /// <summary>
    /// Represents a TypeSpec in the metadata.
    /// </summary>
    /// <remarks>
    /// TypeSpec signatures are decoded by SRM's DecodeSignature; raw blob access is no longer needed.
    /// </remarks>
    internal interface ITypeSpec : ITypeProxy
    {
        // A TypeSpecification handle. This is valid in the DeclaringScope module.
        TypeSpecificationHandle TypeSpecHandle
        {
            get;
        }

        /// <summary>
        /// The scope that the handle is valid in.
        /// </summary>
        Module DeclaringScope
        {
            get;
        }
    }


}