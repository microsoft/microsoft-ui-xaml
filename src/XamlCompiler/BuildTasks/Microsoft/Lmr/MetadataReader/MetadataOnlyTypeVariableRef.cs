// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System;
using System.Text;
using System.Globalization;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Represents a type variable extracted from memberRef signature. 
    /// It is only used for signature matching. That's why most
    /// methods throw exceptions.
    /// </summary>
    internal class MetadataOnlyTypeVariableRef : MetadataOnlyCommonType
    {
        readonly private MetadataOnlyModule m_resolver;
        readonly private EntityHandle m_ownerHandle;
        readonly private int m_position;
        readonly private bool? m_isMethodVar;

        internal MetadataOnlyTypeVariableRef(MetadataOnlyModule resolver, EntityHandle ownerHandle, int position)
        {
            Debug.Assert(resolver != null, "resolver can't be null");
            Debug.Assert(!ownerHandle.IsNil, "owner handle can't be Nil");
            Debug.Assert(position >= 0, "position must be zero or positive");

            m_resolver = resolver;
            m_ownerHandle = ownerHandle;
            m_position = position;
        }

        /// <summary>
        /// Constructor for synthetic type variable references (used during signature parsing
        /// when no generic context is available). Creates a placeholder with just position and kind.
        /// </summary>
        internal MetadataOnlyTypeVariableRef(MetadataOnlyModule resolver, int position, bool isMethodVar)
        {
            Debug.Assert(resolver != null, "resolver can't be null");
            Debug.Assert(position >= 0, "position must be zero or positive");

            m_resolver = resolver;
            m_position = position;
            m_isMethodVar = isMethodVar;
            // m_ownerHandle remains default (nil) for synthetic type vars
        }

        /// <summary>
        /// Convenience constructor for type-variable owners specified by Type object.
        /// </summary>
        internal MetadataOnlyTypeVariableRef(MetadataOnlyModule resolver, Type ownerType, int position)
            : this(resolver, MetadataTokens.EntityHandle(ownerType.MetadataToken), position)
        {
        }

        /// <summary>
        /// Convenience constructor for method-variable owners specified by MemberReferenceHandle.
        /// </summary>
        internal MetadataOnlyTypeVariableRef(MetadataOnlyModule resolver, MemberReferenceHandle ownerMethod, int position)
            : this(resolver, (EntityHandle)ownerMethod, position)
        {
        }

        private bool IsMethodVar
        {
            get
            {
                if (m_isMethodVar.HasValue)
                    return m_isMethodVar.Value;
                return m_ownerHandle.Kind == HandleKind.MemberReference || m_ownerHandle.Kind == HandleKind.MethodDefinition;
            }
        }

        #region Type Members

        public override string FullName
        {
            get { return null; }
        }

        internal override MetadataOnlyModule Resolver
        {
            get { return m_resolver; }
        }

        public override Type BaseType
        {
            get
            {
                throw new InvalidOperationException();
            }
        }

        public override bool Equals(Type other)
        {
            // Use fast comparison for our own type.
            MetadataOnlyTypeVariableRef otherRef = other as MetadataOnlyTypeVariableRef;
            if (otherRef != null)
            {
                // For synthetic type vars (no owner), compare by position and kind
                if (m_ownerHandle.IsNil && otherRef.m_ownerHandle.IsNil)
                {
                    return (this.Resolver.Equals(otherRef.Resolver) &&
                        (this.IsMethodVar == otherRef.IsMethodVar) &&
                        (m_position == otherRef.m_position));
                }
                return (this.Resolver.Equals(otherRef.Resolver) &&
                    (m_ownerHandle == otherRef.m_ownerHandle) &&
                    (m_position == otherRef.m_position));
            }

            if (other.IsGenericParameter)
            {
                // Check if both variables are MethodVars or TypeVars. 
                bool isSameKind = (this.IsMethodVar == (other.DeclaringMethod != null));
                return (m_position == other.GenericParameterPosition) && isSameKind;
            }

            return false;
        }

        public override bool IsAssignableFrom(Type c)
        {
            throw new InvalidOperationException();
        }

        public override Type UnderlyingSystemType
        {
            get { throw new InvalidOperationException(); }
        }

        public override Type GetElementType()
        {
            throw new InvalidOperationException();
        }

        public override int MetadataToken
        {
            get { throw new InvalidOperationException(); }
        }

        public override MethodInfo[] GetMethods(BindingFlags flags)
        {
            throw new InvalidOperationException();
        }

        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            throw new InvalidOperationException();
        }

        public override FieldInfo[] GetFields(BindingFlags flags)
        {
            throw new InvalidOperationException();
        }

        public override FieldInfo GetField(string name, BindingFlags flags)
        {
            throw new InvalidOperationException();
        }

        public override PropertyInfo[] GetProperties(BindingFlags flags)
        {
            throw new InvalidOperationException();
        }

        public override EventInfo[] GetEvents(System.Reflection.BindingFlags flags)
        {
            throw new InvalidOperationException();
        }

        public override EventInfo GetEvent(string name, System.Reflection.BindingFlags flags)
        {
            throw new InvalidOperationException();
        }

        public override Type MakeGenericType(Type[] argTypes)
        {
            throw new InvalidOperationException();
        }

        public override Type GetNestedType(string name, BindingFlags bindingAttr)
        {
            throw new InvalidOperationException();
        }

        public override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            throw new InvalidOperationException();
        }

        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            throw new InvalidOperationException();
        }

        protected override MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            throw new InvalidOperationException();
        }

        protected override PropertyInfo GetPropertyImpl(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            throw new InvalidOperationException();
        }

        // This is the most distinguishing property of this derived type.
        public override bool IsGenericParameter
        {
            get { return true; }
        }

        public override Type[] GetGenericArguments()
        {
            throw new InvalidOperationException();
        }

        public override Type[] GetGenericParameterConstraints()
        {
            throw new InvalidOperationException();
        }

        public override Type GetGenericTypeDefinition()
        {
            throw new InvalidOperationException();
        }

        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            throw new InvalidOperationException();
        }

        public override Type[] GetInterfaces()
        {
            throw new InvalidOperationException();
        }

        public override Type GetInterface(string name, bool ignoreCase)
        {
            throw new InvalidOperationException();
        }

        public override MemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            throw new InvalidOperationException();
        }

        public override Guid GUID
        {
            get { throw new InvalidOperationException(); }
        }

        protected override bool HasElementTypeImpl()
        {
            throw new InvalidOperationException();
        }

        public override object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
        {
            throw new NotSupportedException();
        }


        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            throw new InvalidOperationException();
        }

        public override System.Reflection.GenericParameterAttributes GenericParameterAttributes
        {
            get
            {
                throw new InvalidOperationException();
            }
        }

        public override int GenericParameterPosition
        {
            get
            {
                return m_position;
            }
        }

        #endregion


        #region MemberInfo Members

        public override MemberTypes MemberType
        {
            get { return MemberTypes.TypeInfo; }
        }

        public override Type DeclaringType
        {
            get
            {
                if (!this.IsMethodVar && !m_ownerHandle.IsNil)
                {
                    if (m_ownerHandle.Kind == HandleKind.TypeDefinition)
                    {
                        return m_resolver.Factory.CreateSimpleType(m_resolver, (TypeDefinitionHandle)m_ownerHandle);
                    }
                    else
                    {
                        return m_resolver.Factory.CreateTypeRef(m_resolver, (TypeReferenceHandle)m_ownerHandle);
                    }
                }
                return null;
            }
        }

        public override MethodBase DeclaringMethod
        {
            get
            {
                if (this.IsMethodVar && !m_ownerHandle.IsNil)
                {
                    if (m_ownerHandle.Kind == HandleKind.MethodDefinition)
                    {
                        return m_resolver.Factory.CreateMethodOrConstructor(m_resolver, (MethodDefinitionHandle)m_ownerHandle, null, null);
                    }
                    // For MemberReference owners, resolve through the module
                    return m_resolver.ResolveMethod(MetadataTokens.GetToken(m_ownerHandle));
                }
                return null;
            }
        }

        public override string Name
        {
            get { return null; }
        }

        public override string Namespace
        {
            get
            {
                return null;
            }
        }

        public override Assembly Assembly
        {
            get
            {
                // important to implement so we can create arrays of TypeVarRefs 
                return m_resolver.Assembly;
            }
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

        public override Type ReflectedType
        {
            get { throw new NotSupportedException(); }
        }

        public override string ToString()
        {
            if (this.IsMethodVar)
            {
                return "MVar!!" + this.GenericParameterPosition.ToString(CultureInfo.InvariantCulture);
            }
            else
            {
                return "Var!" + this.GenericParameterPosition.ToString(CultureInfo.InvariantCulture);
            }
        }

        #endregion

        protected override TypeCode GetTypeCodeImpl()
        {
            throw new InvalidOperationException();
        }
    }
}
