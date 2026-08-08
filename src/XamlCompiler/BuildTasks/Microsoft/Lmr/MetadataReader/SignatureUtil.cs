// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection.Adds;
using System.Reflection.Metadata;

using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Encapsulates information about types as encoded in signature blobs.
    /// </summary>
    internal class TypeSignatureDescriptor
    {
        /// <summary>
        /// Final type encoded in sig blob. For generic types, this could be type after instantiation.
        /// E.g T is instantiated with int.
        /// </summary>
        public Type Type { get; set; }

        /// <summary>
        /// Custom modifiers for this parameter, if any are present.
        /// </summary>
        public CustomModifiers CustomModifiers { get; set; }

        /// <summary>
        /// Determines if parameter was pinned or not.
        /// </summary>
        public bool IsPinned { get; set; }
    }

    /// <summary>
    /// Encapsulates information encoded in method signature blobs.
    /// </summary>
    internal class MethodSignatureDescriptor
    {
        /// <summary>
        /// Method's calling convention.
        /// </summary>
        public CorCallingConvention CallingConvention { get; set; }

        /// <summary>
        /// Number of generic method arguments if this is a generic method.
        /// </summary>
        public int GenericParameterCount { get; set; }

        /// <summary>
        /// Descriptor of return parameter.
        /// </summary>
        public TypeSignatureDescriptor ReturnParameter { get; set; }

        /// <summary>
        /// Descriptors of all parameters.
        /// </summary>
        public TypeSignatureDescriptor[] Parameters { get; set; }
    }

    internal static class SignatureUtil
    {
        /// <summary>
        /// Converts an SRM MethodSignature<Type> (from DecodeMethodSignature) into a
        /// MethodSignatureDescriptor, unwrapping modifier/pinned wrapper types via SignatureUnwrap.
        /// </summary>
        internal static MethodSignatureDescriptor FromSrmSignature(MethodSignature<Type> sig)
        {
            var result = new MethodSignatureDescriptor();
            // The raw byte of SignatureHeader matches CorCallingConvention layout
            result.CallingConvention = (CorCallingConvention)sig.Header.RawValue;
            result.GenericParameterCount = sig.GenericParameterCount;
            result.ReturnParameter = SignatureUnwrap.Unwrap(sig.ReturnType);
            result.Parameters = new TypeSignatureDescriptor[sig.ParameterTypes.Length];
            for (int i = 0; i < sig.ParameterTypes.Length; i++)
            {
                result.Parameters[i] = SignatureUnwrap.Unwrap(sig.ParameterTypes[i]);
            }
            return result;
        }

        internal static bool IsVarArg(CorCallingConvention conv)
        {
            CorCallingConvention c = (conv & CorCallingConvention.Mask);
            return (c == CorCallingConvention.VarArg);
        }

        /// <summary>
        /// Gets Reflection calling convention coresponding to passed CorCallingConvention.
        /// </summary>
        internal static CallingConventions GetReflectionCallingConvention(CorCallingConvention callConvention)
        {
            CallingConventions result = (CallingConventions)0;
            if ((callConvention & CorCallingConvention.Mask) == CorCallingConvention.HasThis)
            {
                result = result | CallingConventions.HasThis;
            }
            else if ((callConvention & CorCallingConvention.Mask) == CorCallingConvention.ExplicitThis)
            {
                result = result | CallingConventions.ExplicitThis;
            }

            if (SignatureUtil.IsVarArg(callConvention))
            {
                result = result | CallingConventions.VarArgs;
            }
            else
            {
                result = result | CallingConventions.Standard;
            }

            return result;
        }

        /// <summary>
        /// Determines if method's calling convention matches passed calling convention. 
        /// </summary>
        internal static bool IsCallingConventionMatch(MethodBase method, CallingConventions callConvention)
        {
            if ((callConvention & CallingConventions.Any) == 0)
            {
                if ((callConvention & CallingConventions.VarArgs) != 0 &&
                    (method.CallingConvention & CallingConventions.VarArgs) == 0)
                    return false;

                if ((callConvention & CallingConventions.Standard) != 0 &&
                    (method.CallingConvention & CallingConventions.Standard) == 0)
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Checks if method has expected number of generic parameters.
        /// </summary>
        internal static bool IsGenericParametersCountMatch(MethodInfo method, int expectedGenericParameterCount)
        {
            int genericParameterCount = 0;
            if (method.IsGenericMethod)
            {
                genericParameterCount = method.GetGenericArguments().Length;
            }

            return (genericParameterCount == expectedGenericParameterCount);
        }

        /// <summary>
        /// Determines if method's parameter types match the passed type array.
        /// The types need to be exactly the same.
        /// </summary>
        internal static bool IsParametersTypeMatch(MethodBase method, Type[] parameterTypes)
        {
            if (parameterTypes == null)
            {
                return true;
            }

            ParameterInfo[] methodParameters = method.GetParameters();
            if (methodParameters.Length != parameterTypes.Length)
            {
                return false;
            }

            int numParams = methodParameters.Length;
            for (int i = 0; i < numParams; i++)
            {
                if (!methodParameters[i].ParameterType.Equals(parameterTypes[i]))
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Determines which string comparison should be used based on
        /// binding flags passed in. It could be either case sensitive or
        /// case insensitive.
        /// </summary>
        internal static StringComparison GetStringComparison(BindingFlags flags)
        {
            StringComparison comparison;
            if ((flags & BindingFlags.IgnoreCase) != 0)
            {
                comparison = StringComparison.OrdinalIgnoreCase;
            }
            else
            {
                comparison = StringComparison.Ordinal;
            }

            return comparison;
        }
    }
}
