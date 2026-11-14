// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Common base class of the LMR implementations of <see cref="MethodInfo"/>.
    /// </summary>
    /// <remarks>
    /// Same pattern as <see cref="LmrTypeBase"/>: the annotated members are overridden once, here, with the same annotations,
    /// and forward to unannotated "*Core" members. LMR methods are read from metadata: making a generic method instantiation
    /// over LMR types doesn't need any code, and their bodies are read from the metadata files.
    /// </remarks>
    internal abstract class LmrMethodInfoBase : MethodInfo
    {
        // The same message as the one on MethodBase.GetMethodBody
        internal const string GetMethodBodyRequiresUnreferencedCodeMessage = "Trimming may change method bodies. For example it can change some instructions, remove branches or local variables.";

        [RequiresDynamicCode(LmrTypeBase.MakeGenericTypeRequiresDynamicCodeMessage)]
        [RequiresUnreferencedCode(LmrTypeBase.MakeGenericTypeRequiresUnreferencedCodeMessage)]
        public sealed override MethodInfo MakeGenericMethod(params Type[] typeArguments)
        {
            return MakeGenericMethodCore(typeArguments);
        }

        internal abstract MethodInfo MakeGenericMethodCore(Type[] typeArguments);

        [RequiresUnreferencedCode(GetMethodBodyRequiresUnreferencedCodeMessage)]
        public sealed override MethodBody GetMethodBody()
        {
            return GetMethodBodyCore();
        }

        internal abstract MethodBody GetMethodBodyCore();
    }

    /// <summary>
    /// Common base class of the LMR implementations of <see cref="ConstructorInfo"/>, see <see cref="LmrMethodInfoBase"/>.
    /// </summary>
    internal abstract class LmrConstructorInfoBase : ConstructorInfo
    {
        [RequiresUnreferencedCode(LmrMethodInfoBase.GetMethodBodyRequiresUnreferencedCodeMessage)]
        public sealed override MethodBody GetMethodBody()
        {
            return GetMethodBodyCore();
        }

        internal abstract MethodBody GetMethodBodyCore();
    }
}
