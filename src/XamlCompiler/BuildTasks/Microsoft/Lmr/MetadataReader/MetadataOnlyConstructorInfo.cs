// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

using System.Collections.Generic;
using System.Reflection.Adds;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Represent an System.Reflection.ConstructorInfo. 
    /// 
    /// Reflection requires that constructors derive from ConstructorInfo, not MethodInfo.
    /// However, Constructors are just methods at the metadata level complete with MethodDef tokens.
    /// Since we can only have a single base class, this makes it hard to share between Constructors and Methods.
    /// 
    /// This class is an adapter that lets LMR represent constructors as MethodInfos underneath 
    /// (which leads to a more natural implementation and significantly better sharing with methods)
    /// but wraps everything and expose sit as a ConstructorInfo.
    /// </summary>
    internal class MetadataOnlyConstructorInfo : ConstructorInfo
    {
        // All constructors are actually a method. This is the underlying method. 
        readonly private MethodBase m_method;

        /// <summary>
        /// Create a ConstructorInfo around the given method base. This is done purely for conformance to
        /// reflection's object model and does not provide any additional benefit.
        /// </summary>
        public MetadataOnlyConstructorInfo(MethodBase method)
        {
            m_method = method;
        }

        public override int MetadataToken { get { return m_method.MetadataToken; } }

        public override string ToString()
        {
            return m_method.ToString();
        }

        public override Module Module
        {
            get { return m_method.Module; }
        }

        // Get the Type that this is declared in.
        public override Type DeclaringType
        {
            get
            {
                return m_method.DeclaringType;
            }
        }

        // Name of just the method (not type).
        public override string Name
        {
            get { return m_method.Name; }
        }

        public override object[] GetCustomAttributes(bool inherit)
        {
            throw new NotSupportedException();
        }

        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            throw new NotSupportedException();
        }

        public override bool IsDefined(Type attributeType, bool inherit)
        {
            throw new NotSupportedException();
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override Type ReflectedType
        {
            get { throw new NotSupportedException(); }
        }

        public override ParameterInfo[] GetParameters()
        {
            return m_method.GetParameters();
        }

        public override MethodAttributes Attributes
        {
            get { return m_method.Attributes; }
        }

        public override CallingConventions CallingConvention 
        { 
            get 
            {
                return m_method.CallingConvention;
            }
        }

        public override MemberTypes MemberType
        {
            get { return MemberTypes.Constructor; }
        }

        public override bool IsGenericMethodDefinition 
        {
            get
            {
                return m_method.IsGenericMethodDefinition;
            }
        }

        public override MethodBody GetMethodBody()
        {
            return m_method.GetMethodBody();
        }

        public override object Invoke(BindingFlags invokeAttr, Binder binder, object[] parameters, System.Globalization.CultureInfo culture)
        {
            throw new NotSupportedException();
        }

        public override System.Reflection.MethodImplAttributes GetMethodImplementationFlags()
        {
            return this.m_method.GetMethodImplementationFlags();
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override RuntimeMethodHandle MethodHandle
        {
            get { throw new NotSupportedException(); }
        }

        public override object Invoke(object obj, BindingFlags invokeAttr, Binder binder, object[] parameters, System.Globalization.CultureInfo culture)
        {
            throw new NotSupportedException();
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return m_method.GetCustomAttributesData();
        }

        public override bool Equals(object obj)
        {
            // Only works with other LMR based constructors (same as MethodInfo).
            MetadataOnlyConstructorInfo ci = obj as MetadataOnlyConstructorInfo;

            if (ci == null)
            {
                return false;
            }

            return m_method.Equals(ci.m_method);
        }

        public override int GetHashCode()
        {
            return m_method.GetHashCode();
        }
    }

} // namespace
