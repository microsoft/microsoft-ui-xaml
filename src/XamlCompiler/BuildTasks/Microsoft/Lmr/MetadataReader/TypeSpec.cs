// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;
using System.Text;
using System.Globalization;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Diagnostics;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{   
        
    /// <summary>
    /// Represent a type object around a TypeSpecification handle.  
    /// A TypeSpec could become any other type, including a TypeDef, TypeRef, Generic instantiation, array, modifier, etc.
    /// TypeSpecs become very common when dealing with generics.
    /// SRM's DecodeSignature replaces the hand-written SignatureUtil parser.
    /// </summary>
    [DebuggerDisplay("TypeSpec")]
    internal class TypeSpec : TypeProxy, ITypeSpec
    {
        // The TypeSpecification handle. Used to decode the signature via SRM.
        readonly TypeSpecificationHandle m_typeSpecHandle;

        // Provides generic type and method args, which can be referred to by the signature.
        readonly GenericContext m_context;

        /// <summary>
        /// Represent a type spec
        /// </summary>
        /// <param name="module">module scope that the handle is valid in. </param>
        /// <param name="typeSpecHandle">a TypeSpecification handle in that scope</param>
        /// <param name="typeArgs">the generic type args for resolving vars</param>
        /// <param name="methodArgs">the generic method args for resolving mvars.</param>
        public TypeSpec(MetadataOnlyModule module, TypeSpecificationHandle typeSpecHandle, Type[] typeArgs, Type[] methodArgs)
            : base(module)
        {
            m_typeSpecHandle = typeSpecHandle;
            m_context = new GenericContext(typeArgs, methodArgs);
        }

        #region ITypeSpec Members

        public TypeSpecificationHandle TypeSpecHandle
        {
            get { return m_typeSpecHandle;  }
        }

        public Module DeclaringScope
        {
            get { return this.Resolver;  }
        }
        #endregion

        protected override Type GetResolvedTypeWorker()
        {
            // Use SRM's DecodeSignature with LmrTypeProvider to resolve the type spec
            var typeSpec = this.Resolver.RawReader.GetTypeSpecification(m_typeSpecHandle);
            var type = typeSpec.DecodeSignature(this.Resolver.TypeProvider, m_context);
            return type;
        }
    } // end class TypeSpec
}