// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// 

using System.Diagnostics;

using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;
using System.Text;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Reflection.Adds;

using System.Reflection;  


namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Convenience class to group type arg and method args together.
    /// This is similar to:
    ///   Type.GetGenericArguments.
    ///   Method.GetGenericArguments.
    ///   
    /// Signatures also can directly refer to type arguments from the generic context 
    /// ( eg Type arg #2; method arg #1), so this can be passed to a signature resolver to convert an open
    /// generic type to a closed generic type.
    /// </summary>
    internal class GenericContext
    {
        public GenericContext(Type[] typeArgs, Type[] methodArgs)
        {
            // Create empty arrays if needed so we can avoid null checks in 
            // our implementation.
            TypeArgs = (typeArgs == null) ? Type.EmptyTypes : typeArgs;
            MethodArgs = (methodArgs == null) ? Type.EmptyTypes : methodArgs;
        }

        public GenericContext(MethodBase methodTypeContext)
            : this(methodTypeContext.DeclaringType.GetGenericArguments(), methodTypeContext.GetGenericArguments())
        {
        }

        public Type[] TypeArgs { get; protected set; }
        public Type[] MethodArgs { get; protected set; }

        public override bool Equals(object obj)
        {
            GenericContext context = (GenericContext)obj;
            if (context == null)
                return false;
            return IsArrayEqual(TypeArgs, context.TypeArgs) && IsArrayEqual(MethodArgs, context.MethodArgs);
        }

        public override int GetHashCode()
        {
            return GetArrayHashCode(TypeArgs) * 32768 + GetArrayHashCode(MethodArgs);
        }

        /// <summary>
        /// Verifies that generic context contains the right number of method arguments.
        /// </summary>
        /// <remarks>
        /// Checking number of type arguments is easy since we can get them from class' metadata
        /// (regardless if we have TypeDef or TypeRef tokens). 
        /// But method arguments are not as easy and we can only get them from signature blob
        /// if we have MemberRef token. That's why we use this API to check if we are in
        /// consistent state after we read signature blob context. See OpenGenericContext class
        /// for alternative implementation.
        /// </remarks>
        public virtual GenericContext VerifyAndUpdateMethodArguments(int expectedNumberOfMethodArgs)
        {
            if (this.MethodArgs.Length != expectedNumberOfMethodArgs)
            {
                throw new ArgumentException(Resources.InvalidMetadataSignature);
            }

            return this;
        }

        private static int GetArrayHashCode<T>(T[] a)
        {
            Debug.Assert(a != null, "array can't be null.");

            int hash = 0;
            for (int i = 0; i < a.Length; i++)
            {
                hash += a[i].GetHashCode() * i;
            }

            return hash;
        }

        private static bool IsArrayEqual<T>(T[] a1, T[] a2) where T : Type
        {
            Debug.Assert(a1 != null, "first array can't be null.");
            Debug.Assert(a2 != null, "second array can't be null.");

            if (a1.Length == a2.Length)
            {
                for (int i = 0; i < a1.Length; i++)
                {
                    if (!a1[i].Equals(a2[i]))
                        return false;
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        private static string ArrayToString<T>(T[] a)
        {
            // This can happen from ToString calls by the debugger.
            if (a == null)
            {
                return "empty";
            }

            StringBuilder sb = StringBuilderPool.Get();
            for (int i = 0; i < a.Length; i++)
            {
                if (i != 0)
                    sb.Append(",");
                sb.Append(a[i]);
            }

            string result = sb.ToString();
            StringBuilderPool.Release(ref sb);
            return result;
        }

        public override string ToString()
        {
            return "Type: " + ArrayToString(TypeArgs) + ", Method: " + ArrayToString(MethodArgs);
        }

        /// <summary>
        /// Returns a generic context with the method args removed.
        /// </summary>
        public GenericContext DeleteMethodArgs()
        {
            if (this.MethodArgs.Length == 0)
            {
                return this;
            }
            else
            {
                return new GenericContext(TypeArgs, null);
            }
        }

        /// <summary>
        /// Checks if generic context is null or has empty method arguments list.
        /// </summary>
        public static bool IsNullOrEmptyMethodArgs(GenericContext context)
        {
            if ((context == null) || (context.MethodArgs.Length == 0))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Checks if generic context is null or has empty type arguments list.
        /// </summary>
        public static bool IsNullOrEmptyTypeArgs(GenericContext context)
        {
            if ((context == null) || (context.TypeArgs.Length == 0))
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}
