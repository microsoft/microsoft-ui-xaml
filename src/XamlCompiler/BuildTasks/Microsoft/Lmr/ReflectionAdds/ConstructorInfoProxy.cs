// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
namespace System.Reflection.Adds
{

    using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using System;
    using System.Text;
    using System.Globalization;
    using System.Reflection.Adds;
    using System.Diagnostics;

    using System.Reflection;  

    /// <summary>
    /// Helper for proxy ConstructorInfo that forwards all calls.
    /// </summary>
    internal abstract class ConstructorInfoProxy : ConstructorInfo
    {
        ConstructorInfo m_cachedResolved;

        /// <summary>
        /// Derived classes implement to provide the real constructor object that we forward to. 
        /// </summary>
        /// <returns></returns>
        protected abstract ConstructorInfo GetResolvedWorker();

        public ConstructorInfo GetResolvedConstructor()
        {
            if (m_cachedResolved == null)
            {
                m_cachedResolved = this.GetResolvedWorker();
            }
            return m_cachedResolved;
        }

        public override object Invoke(BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
        {
            return GetResolvedConstructor().Invoke(invokeAttr, binder, parameters, culture);
        }

        public override System.Reflection.MethodAttributes Attributes
        {
            // Can we infer this from the calling convention and binding flags?
            get { return GetResolvedConstructor().Attributes; }
        }

        public override CallingConventions CallingConvention
        {
            // does this match the calling convention we asked for?
            get { return GetResolvedConstructor().CallingConvention; }
        }

        public override ParameterInfo[] GetParameters()
        {
            // Do parameters have to be an exact match?
            return GetResolvedConstructor().GetParameters();
        }

        public override bool IsGenericMethodDefinition
        {
            get
            {
                return GetResolvedConstructor().IsGenericMethodDefinition;
            }
        }

        public override Type[] GetGenericArguments()
        {
            return GetResolvedConstructor().GetGenericArguments();
        }

        public override bool ContainsGenericParameters
        {
            get { return GetResolvedConstructor().ContainsGenericParameters; }
        }

        public override MethodBody GetMethodBody()
        {
            return GetResolvedConstructor().GetMethodBody();
        }

        public override System.Reflection.MethodImplAttributes GetMethodImplementationFlags()
        {
            return GetResolvedConstructor().GetMethodImplementationFlags();
        }

        public override object Invoke(object obj, BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
        {
            return GetResolvedConstructor().Invoke(obj, invokeAttr, binder, parameters, culture);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override RuntimeMethodHandle MethodHandle
        {
            get { throw new NotImplementedException(); }
        }

        public override MemberTypes MemberType
        {
            get { return MemberTypes.Constructor; }
        }

        public override Type DeclaringType
        {
            get { return this.GetResolvedConstructor().DeclaringType; }
        }

        public override string Name
        {
            // We could infer this
            get { return GetResolvedConstructor().Name; }
        }

        public override int MetadataToken
        {
            get { return GetResolvedConstructor().MetadataToken; }
        }

        public override Module Module
        {
            // Module that we resolve to
            get { return GetResolvedConstructor().Module; }
        }

        public override Type ReflectedType
        {
            get { return GetResolvedConstructor().ReflectedType; }
        }

        public override object[] GetCustomAttributes(bool inherit)
        {
            return GetResolvedConstructor().GetCustomAttributes(inherit);
        }


        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return GetResolvedConstructor().GetCustomAttributes(attributeType, inherit);
        }

        public override bool IsDefined(Type attributeType, bool inherit)
        {
            return GetResolvedConstructor().IsDefined(attributeType, inherit);
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return GetResolvedConstructor().GetCustomAttributesData();
        }
    } // end class ConstructorInfoProxy

}