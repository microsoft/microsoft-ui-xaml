// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;
using System.Text;
using System.Globalization;
using System.Reflection.Adds;
using System.Diagnostics;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Type proxy for generic instantiation around another type-proxy. 
    /// This can be used to build up type algebra trees without resolution. 
    /// This is similar to ModifierType and ArrayType. 
    /// </summary>
    class ProxyGenericType : TypeProxy
    {
        // The type being generic instantiated (the GenericTypeDefinition).
        readonly TypeProxy m_rawType;

        // The type-args.
        // This must be non-null and greater than 0 length.
        readonly Type[] m_args;

        public ProxyGenericType(TypeProxy rawType, Type[] args)
            : base(rawType.Resolver)
        {
            Debug.Assert(args.Length > 0);
            m_rawType = rawType;
            m_args = args;
        }

        protected override Type GetResolvedTypeWorker()
        {
            return this.m_rawType.GetResolvedType().MakeGenericType(m_args);
        }

        // Everything forwards except algebra operations that we can safely do.
        public override Type[] GetGenericArguments()
        {
            return (Type[])m_args.Clone();
        }

        public override Type GetGenericTypeDefinition()
        {
            return m_rawType;
        }

        #region Names

        // For generics, can't do FullName without resolution. 
        // FullName depends on the generic args. If the generic args have type variables, FullName is null.
        // For example:
        //    (open)    Gen<T, S>  --> "Foo+Gen`2"
        //    (partial) Gen<int, S> --> null
        //    (closed)  Gen<int, string> --> "Foo+Gen`2[[System.Int32, mscorlib, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089],[System.String, mscorlib, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089]]"
        // We may not be able to implement the FullName contract without doing type resolution.
        // 
        // However, in all cases,  Name is "Gen`2"
        public override string Name
        {
            get
            {
                return m_rawType.Name;
            }
        }
        public override string Namespace
        {
            get
            {
                return m_rawType.Namespace;
            }
        }       
        #endregion // Names

        #region Type Algebra
        //
        // TypeProxy's default behavior is to resolve and forward everything. 
        // Overload the type-algebra operations to avoid resolution.
        //

        protected override bool IsPointerImpl()
        {
            return false;
        }
        protected override bool IsArrayImpl()
        {
            return false;
        }
        protected override bool IsByRefImpl()
        {
            return false;
        }

        // Gets the outer type.
        public override Type DeclaringType
        {
            get
            {
                return m_rawType.DeclaringType;
            }
        }
        public override bool IsGenericParameter
        {
            get
            {
                // This is a generic type, so it can't be a generic parameter.
                return false;
            }
        }
        public override bool IsGenericType
        {
            get
            {
                // This is used to print the fullname.
                // We know we're a generic type beause the ctor validated that we have 1 or more generic arguments.
                return true;
            }
        }
        public override bool IsGenericTypeDefinition
        {
            get
            {
                // We know we can't be a generic type definition because GetResolvedType().MakeGenericType(m_args)
                // is expected to succeed - so we're assuming we've got instantiations for all type parameters.
                return false;
            }
        }
        #endregion

        public override bool IsEnum
        {
            get
            {
                return this.m_rawType.IsEnum;
            }
        }
        protected override bool IsValueTypeImpl()
        {
            return this.m_rawType.IsValueType;
        }
        protected override bool IsPrimitiveImpl()
        {
            // Generic instantiations are never primitive
            // Ensure this doesn't require resolution
            return false;
        }


        // A generic type's module is that of its type definition.
        public override Module Module
        {
            get
            {
                return m_rawType.Module;
            }
        }

        public override Assembly Assembly
        {
            get
            {
                return m_rawType.Assembly;
            }
        }

    }

}
