// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// LMR implementation of a method body using System.Reflection.Metadata

using System;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System.Globalization;
using System.Collections.Immutable;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Implementation for a MethodBody using System.Reflection.Metadata.
    /// Uses PEReader.GetMethodBody() which provides MethodBodyBlock with IL, locals, and exception regions.
    /// </summary>
    internal class MetadataOnlyMethodBody : MethodBody
    {
        // The method that this body is for.
        readonly MetadataOnlyMethodInfo m_method;
        readonly MethodBodyBlock m_bodyBlock;

        /// <summary>
        /// constructor
        /// </summary>
        protected MetadataOnlyMethodBody(MetadataOnlyMethodInfo method, MethodBodyBlock bodyBlock)
        {
            Debug.Assert(method != null);
            Debug.Assert(bodyBlock != null);
            m_method = method;
            m_bodyBlock = bodyBlock;
        }

        /// <summary>
        /// Helper to create a method body and invoke the reflection factory.
        /// </summary>
        /// <param name="method">method to request the body for</param>
        /// <returns>null if the method does not have an IL body, else the instantiated body.</returns>
        internal static MethodBody TryCreate(MetadataOnlyMethodInfo method)
        {
            // First give the factory an opportunity to create.
            MetadataOnlyModule scope = method.Resolver;

            MethodBody b = null;
            if (scope.Factory.TryCreateMethodBody(method, ref b))            
            {
                // Factory claimed ownership, return the results. 
                return b;
            }

            // For static metadata, if there's no RVA, there's no method body.
            var methodDefHandle = MetadataTokens.MethodDefinitionHandle(method.MetadataToken);
            int rva = scope.GetMethodRva(methodDefHandle);
            if (rva == 0)
                return null;

            var bodyBlock = scope.RawPEReader.GetMethodBody(rva);
            if (bodyBlock == null)
                return null;

            return new MetadataOnlyMethodBody(method, bodyBlock);
        }

        /// <summary>
        /// Method that this body belongs to.
        /// </summary>
        protected MetadataOnlyMethodInfo Method
        {
            get { return m_method; }
        }

        public override IList<ExceptionHandlingClause> ExceptionHandlingClauses
        {
            get
            {
                var regions = m_bodyBlock.ExceptionRegions;
                if (regions.Length == 0)
                {
                    return new ExceptionHandlingClause[0];
                }

                var clauses = new ExceptionHandlingClause[regions.Length];
                for (int i = 0; i < regions.Length; i++)
                {
                    clauses[i] = new ExceptionHandlingClauseWrapper(m_method, regions[i]);
                }
                return Array.AsReadOnly(clauses);
            }
        }

        public override bool InitLocals
        {
            get
            {
                return m_bodyBlock.LocalSignature.IsNil ? false : m_bodyBlock.LocalVariablesInitialized;
            }
        }

        public override int LocalSignatureMetadataToken
        {
            get
            {
                if (m_bodyBlock.LocalSignature.IsNil)
                    return 0;
                return MetadataTokens.GetToken(m_bodyBlock.LocalSignature);
            }
        }

        public override IList<LocalVariableInfo> LocalVariables
        {
            get
            {
                if (m_bodyBlock.LocalSignature.IsNil)
                {
                    return new MetadataOnlyLocalVariableInfo[0];
                }

                var reader = m_method.Resolver.RawReader;
                var sig = reader.GetStandaloneSignature(m_bodyBlock.LocalSignature);
                GenericContext context = new GenericContext(m_method);
                var types = sig.DecodeLocalSignature(m_method.Resolver.TypeProvider, context);

                var locals = new MetadataOnlyLocalVariableInfo[types.Length];
                for (int i = 0; i < types.Length; i++)
                {
                    var descriptor = SignatureUnwrap.Unwrap(types[i]);
                    bool isPinned = descriptor.IsPinned;
                    Type localType = descriptor.Type;
                    locals[i] = new MetadataOnlyLocalVariableInfo(i, localType, isPinned);
                }

                return locals;
            }
        }

        public override int MaxStackSize
        {
            get
            {
                return m_bodyBlock.MaxStack;
            }
        }

        public override byte[] GetILAsByteArray()
        {
            return m_bodyBlock.GetILBytes();
        }

        /// <summary>
        /// Wrapper for SRM ExceptionRegion to implement ExceptionHandlingClause.
        /// </summary>
        private class ExceptionHandlingClauseWrapper : ExceptionHandlingClause
        {
            private readonly MethodInfo m_method;
            private readonly ExceptionRegion m_region;

            public ExceptionHandlingClauseWrapper(MethodInfo method, ExceptionRegion region)
            {
                m_method = method;
                m_region = region;
            }

            public override Type CatchType
            {
                get
                {
                    if (m_region.CatchType.IsNil)
                        return null;

                    var module = m_method.Module;
                    int token = MetadataTokens.GetToken(m_region.CatchType);
                    var t = module.ResolveType(token, m_method.DeclaringType.GetGenericArguments(), m_method.GetGenericArguments());
                    return t;
                }
            }

            public override int FilterOffset
            {
                get { return m_region.FilterOffset; }
            }

            public override System.Reflection.ExceptionHandlingClauseOptions Flags
            {
                get { return (System.Reflection.ExceptionHandlingClauseOptions)m_region.Kind; }
            }

            public override int HandlerLength
            {
                get { return m_region.HandlerLength; }
            }

            public override int HandlerOffset
            {
                get { return m_region.HandlerOffset; }
            }

            public override int TryLength
            {
                get { return m_region.TryLength; }
            }

            public override int TryOffset
            {
                get { return m_region.TryOffset; }
            }
        }
    }

} // namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
