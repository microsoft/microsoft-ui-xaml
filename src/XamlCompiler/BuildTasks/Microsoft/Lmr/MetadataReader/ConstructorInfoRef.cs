// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System;
using System.Text;
using System.Globalization;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Diagnostics;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Simple proxy to just chain back to the declaring type. 
    /// This is used in custom attributes so facilitate the design pattern of:
    ///    Ca.Constructor.DeclaringType.FullName
    /// so that we can get the custom attribute name (and argument types) without resolving.
    /// </summary>
    internal class ConstructorInfoRef : ConstructorInfoProxy
    {
        // The type that this constructor is for.
        readonly Type m_declaringType;

        // The handle for this constructor which can be resolved on m_scope to get the real constructorInfo object.
        readonly MemberReferenceHandle m_handle;

        // The scope that m_handle is valid in.
        readonly MetadataOnlyModule m_scope;

        public ConstructorInfoRef(Type declaringType, MetadataOnlyModule scope, MemberReferenceHandle handle)
        {
            m_declaringType = declaringType;
            m_handle = handle;
            m_scope = scope;
        }

        protected override ConstructorInfo GetResolvedWorker()
        {
            MethodBase method = m_scope.ResolveMethod(MetadataTokens.GetToken(m_handle));
            return (ConstructorInfo)method;
        }

        public override Type DeclaringType
        {
            get { return this.m_declaringType; }
        }

        /// <summary>
        /// Get the parameter information available in the MethodRef without resolving to a def
        /// </summary>
        /// <returns>An array of ParameterInfo objects describing the types (and modifiers) of parameters (but not their
        /// names and other attributes present only in the definition)</returns>
        /// <remarks>
        /// Note that we don't override GetParameters() because it could be a breaking change - omitting information
        /// (tokens, names, in/out, etc.) that the caller may care about.  For custom attribute processing we explicitly
        /// opt-in to using this API.
        /// </remarks>
        public ParameterInfo[] GetSignatureParameters()
        {
            // Note that we avoid caching the signature and get it again as needed, consistent with LMR policy
            // In some cases this may be a little wasteful (we already called GetMemberRefData to create this object)
            EntityHandle declDummy;
            string nameDummy;
            BlobReader signatureReader;
            m_scope.GetMemberRefData(m_handle, out declDummy, out nameDummy, out signatureReader);

            // Can't be a generic instantiation
            var tempContext = new GenericContext(null, null);

            var memberRef = m_scope.RawReader.GetMemberReference(m_handle);
            var sig = memberRef.DecodeMethodSignature(m_scope.TypeProvider, tempContext);
            var descr = SignatureUtil.FromSrmSignature(sig);
            Debug.Assert(descr.CallingConvention != CorCallingConvention.Generic);

            ParameterInfo[] parameters = new SimpleParameterInfo[descr.Parameters.Length];
            for(int i = 0; i < descr.Parameters.Length; i++)
            {
                parameters[i] = new SignatureParameterInfo(this, descr.Parameters[i].Type, i, descr.Parameters[i].CustomModifiers);
            }
            return parameters;
        }
    }

    /// <summary>
    /// Parameters created from a methodRef signature
    /// </summary>
    // Re-use SimpleParameterInfo for convenience, but add custom modifiers since we have them.
    // Perhaps it would be better to override other methods and throw rather than return the placeholder values?
    internal class SignatureParameterInfo : SimpleParameterInfo
    {
        public SignatureParameterInfo(MemberInfo member, Type paramType, int position, CustomModifiers modifiers)
            : base(member, paramType, position)
        {
            m_modifiers = modifiers;
        }

        public override Type[] GetOptionalCustomModifiers()
        {
            return m_modifiers.OptionalCustomModifiers;
        }

        public override Type[] GetRequiredCustomModifiers()
        {
            return m_modifiers.RequiredCustomModifiers;
        }

        private CustomModifiers m_modifiers;
    }
}
