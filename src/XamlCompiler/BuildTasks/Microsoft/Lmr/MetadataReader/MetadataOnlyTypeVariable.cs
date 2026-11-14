// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Type impl based on System.Reflection.Metadata reader.

using Debug = System.Diagnostics.Debug;
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
    /// Represents an uninstantiated type variable.  For now, most method are unimplemented.
    /// The defining property of this is that IsGenericParameter returns true; whereas most other Type
    /// implementations return false.
    /// </summary>
    internal class MetadataOnlyTypeVariable : MetadataOnlyCommonType
    {
        //One of m_ownerType and m_ownerMethod must be default.
        readonly private TypeDefinitionHandle m_ownerType;
        readonly private MethodDefinitionHandle m_ownerMethod;

        readonly private string m_name;
        readonly private int m_position;
        readonly private MetadataOnlyModule m_resolver;
        readonly private GenericParameterHandle m_handle;
        readonly private System.Reflection.GenericParameterAttributes m_gpAttributes;

        internal MetadataOnlyTypeVariable(MetadataOnlyModule resolver, GenericParameterHandle handle)
        {
            m_handle = handle;
            m_resolver = resolver;

            int genIndex;
            m_resolver.GetGenericParameterProps(m_handle, out m_ownerType, out m_ownerMethod, out m_name, out m_gpAttributes, out genIndex);
            m_position = genIndex;
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
                Type[] typeConstraints = GetGenericParameterConstraints();
                foreach (Type t in typeConstraints)
                {
                    //There can at most be one class in the constraints.
                    if (t.IsClass)
                    {
                        return t;
                    }
                }
                //If no type constraints are specified for this type parameter,
                //return System.Object.
                return m_resolver.AssemblyResolver.GetBuiltInType(CorElementType.Object);
            }
        }

        public override bool Equals(Type txOther)
        {
            if (txOther is MetadataOnlyTypeVariableRef)
            {
                return (!m_ownerMethod.IsNil) && (m_position == txOther.GenericParameterPosition);
            }

            //if txOther is not a type parameter, return false.
            MetadataOnlyTypeVariable other = txOther as MetadataOnlyTypeVariable;
            if (other == null)
            {
                return false;
            }

            //two type variables are considered the same if both their names and declaring types/methods are the same.
            if (this.Name != other.Name)
            {
                return false;
            }
            //The type parameters can be equal only if the owners are in the same module.
            //Therefore we can simply compare the metadata token of the owners.
            //This can save us by not creating the owner type/method object.
            return this.m_ownerType == other.m_ownerType &&
                   this.m_ownerMethod == other.m_ownerMethod &&
                   this.Module.Equals(other.Module);
        }


        public override bool IsAssignableFrom(Type c)
        {
            return MetadataOnlyTypeDef.IsAssignableFromHelper(this, c);
        }

        public override Type UnderlyingSystemType
        {
            get { return this; }
        }


        public override Type GetElementType()
        {
            return null;
        }

        public override int MetadataToken
        {
            get { return MetadataTokens.GetToken(m_handle); }
        }

        internal override MethodInfo[] GetMethodsCore(BindingFlags flags)
        {
            return new MethodInfo[0];
        }

        internal override ConstructorInfo[] GetConstructorsCore(BindingFlags bindingAttr)
        {
            return new ConstructorInfo[0];
        }

        internal override FieldInfo[] GetFieldsCore(BindingFlags flags)
        {
            return new FieldInfo[0];
        }

        internal override FieldInfo GetFieldCore(string name, BindingFlags flags)
        {
            return null;
        }

        internal override PropertyInfo[] GetPropertiesCore(BindingFlags flags)
        {
            return new PropertyInfo[0];
        }

        internal override EventInfo[] GetEventsCore(System.Reflection.BindingFlags flags)
        {
            return new EventInfo[0];
        }

        internal override EventInfo GetEventCore(string name, System.Reflection.BindingFlags flags)
        {
            return null;
        }

        internal override Type MakeGenericTypeCore(Type[] argTypes)
        {
            // You can't make a generic type around a Type variable 
            throw new InvalidOperationException();
        }


        internal override Type GetNestedTypeCore(string name, BindingFlags bindingAttr)
        {
            return null;
        }

        internal override Type[] GetNestedTypesCore(BindingFlags bindingAttr)
        {
            return Type.EmptyTypes;
        }

        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            // In MSDN, the following is mentioned for the Type.TypeAttributes property.
            // If the current Type represents a generic type parameter — that is, if the 
            // IsGenericParameter property returns true — the TypeAttributes value returned 
            // by this property is unspecified. 
            // LMR returns a value that is the same as what we get from the reflection here. 
            // But since it is unspecified, people should not depend on it.
            return TypeAttributes.Public;
        }

        internal override MethodInfo GetMethodImplCore(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            return null;
        }

        internal override PropertyInfo GetPropertyImplCore(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            return null;
        }

        // This is the most distinguishing property of this derived type.
        public override bool IsGenericParameter
        {
            get { return true; }
        }

        public override Type[] GetGenericArguments()
        {
            return Type.EmptyTypes;
        }

        public override Type[] GetGenericParameterConstraints()
        {
            List<Type> l = new List<Type>(m_resolver.GetConstraintTypes(m_handle));
            return l.ToArray();
        }

        public override Type GetGenericTypeDefinition()
        {
            throw new InvalidOperationException();
        }

        internal override ConstructorInfo GetConstructorImplCore(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            return null;
        }

        internal override Type[] GetInterfacesCore()
        {
            return MetadataOnlyTypeDef.GetAllInterfacesHelper(this);
        }

        internal override Type GetInterfaceCore(string name, bool ignoreCase)
        {
            return MetadataOnlyModule.GetInterfaceHelper(GetInterfacesCore(), name, ignoreCase);
        }

        internal override MemberInfo[] GetMembersCore(BindingFlags bindingAttr)
        {
            return MetadataOnlyTypeDef.GetMembersHelper(this, bindingAttr);
        }

        public override Guid GUID
        {
            get { return Guid.Empty; }
        }

        protected override bool HasElementTypeImpl()
        {
            return false;
        }

        internal override object InvokeMemberCore(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
        {
            throw new NotSupportedException();
        }


        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return this.Resolver.GetCustomAttributeData((EntityHandle)this.m_handle);
        }

        public override System.Reflection.GenericParameterAttributes GenericParameterAttributes
        {
            get 
            {
                return m_gpAttributes;
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
                if (!m_ownerType.IsNil)
                {
                    return m_resolver.ResolveTypeLmr(MetadataTokens.GetToken(m_ownerType));
                }
                else if (DeclaringMethod != null)
                {
                    return DeclaringMethod.DeclaringType;
                }

                return null; 
            }
        }

        public override MethodBase DeclaringMethod
        {
            get
            {
                if (!m_ownerMethod.IsNil)
                {
                    return m_resolver.ResolveMethodLmr(MetadataTokens.GetToken(m_ownerMethod));
                }
                return null;
            }
        }

        public override string Name
        {
            get { return m_name; }
        }

        public override string Namespace
        {
            get 
            {
                if (this.DeclaringType != null)
                {
                    return this.DeclaringType.Namespace;
                }
                if (this.DeclaringMethod != null)
                {
                    return this.DeclaringMethod.DeclaringType.Namespace;
                }
                return null;
            }
        }

        public override Assembly Assembly
        {
            get 
            {
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
            return this.Name;
        }

        #endregion

        protected override TypeCode GetTypeCodeImpl()
        {
            // Unresolved generic type parameters are just considered objects.
            return TypeCode.Object;
        }
    }

} // namespace
