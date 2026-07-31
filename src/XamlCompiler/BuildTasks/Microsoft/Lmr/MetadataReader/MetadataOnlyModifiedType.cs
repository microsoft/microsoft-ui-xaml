// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;
using System.Text;
using System.Globalization;
using System.Reflection.Adds;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{

    // This is part of the type algebra. It handles compound types like creating
    // pointer (*) or Byref(&) types around an existing type.    
    internal class MetadataOnlyModifiedType : MetadataOnlyCommonType
    {
        // The element type.
        // This is MetadataOnlyCommonType instead of just Type because our LMRModifiedType derives from
        // MetadataOnlyCommonType and forwards to m_type.
        readonly private MetadataOnlyCommonType m_type;

        // The modifier to the element type. This will be "*" (pointer), "&" (byref). 
        readonly private string m_mod;

        // We take an MetadataOnlyCommonType here because this can only be instantiated by LMR types, so we
        // know we have an MetadataOnlyCommonType available.
        public MetadataOnlyModifiedType(MetadataOnlyCommonType type, string mod)
        {
            m_type = type;
            m_mod = mod;
            
            Debug.Assert(!mod.Equals("[]"), "Do not use this class for arrays; use ArrayType instead");
            Debug.Assert(mod == "*" || mod == "&");
        }

        #region Type Members

        public override string FullName
        {
            get 
            {
                // For a type-variable T&, element name is null. In that case, return null.
                // If the element type is a generic type definition, return null. This is what
                // the reflection does.
                var fullName = m_type.FullName;
                if (fullName == null || m_type.IsGenericTypeDefinition)
                {
                    return null;
                }
                return fullName + m_mod; 
            }
        }

        internal override MetadataOnlyModule Resolver
        {
            get { return m_type.Resolver; }
        }

        public override Type BaseType
        {
            get 
            {
                return null;
            }
        }


        public override bool Equals(Type t)
        {
            if (t == null)
            {
                return false;
            }

            // Compare just using public surface area.
            // First compare modifiers, then compare root.
            if (this.IsByRef)
            {
                if (!t.IsByRef)
                    return false;
            }
            else if (this.IsPointer)
            {
                if (!t.IsPointer)
                    return false;
            }
            else
            {
                Debug.Assert(false); // this should be either a ByRef or Pointer.
            }

            var root = t.GetElementType(); 

            return m_type.Equals(root);           
        }


        protected override bool IsByRefImpl()
        {
            return m_mod.Equals("&");
        }

        protected override bool IsPointerImpl()
        {
            return m_mod.Equals("*");
        }

        public override bool IsAssignableFrom(Type c)
        {
            if (c == null)
            {
                return false;
            }
            if ((this.IsPointer && c.IsPointer) || (this.IsByRef && c.IsByRef))
            {
                //If the IsAssignableFrom relation holds for the element types and the element type of c is not value type,
                //the relation also holds for the pointer or byref types.
                Type elemType = c.GetElementType();
                if (m_type.IsAssignableFrom(elemType) && !elemType.IsValueType)
                {
                    return true;
                }
            }
            return MetadataOnlyTypeDef.IsAssignableFromHelper(this, c);
        }

        public override Type UnderlyingSystemType
        {
            get { return this; }
        }

        public override Type GetElementType()
        {
            return m_type;
        }

        public override MethodInfo[] GetMethods(System.Reflection.BindingFlags flags)
        {
            return new MethodInfo[0];
        }

        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            return new ConstructorInfo[0];
        }

        public override FieldInfo[] GetFields(System.Reflection.BindingFlags flags)
        {
            return new FieldInfo[0];
        }

        public override PropertyInfo[] GetProperties(System.Reflection.BindingFlags flags)
        {
            return new PropertyInfo[0];
        }

        public override EventInfo[] GetEvents(System.Reflection.BindingFlags flags)
        {
            return new EventInfo[0];
        }

        public override EventInfo GetEvent(string name, System.Reflection.BindingFlags flags)
        {
            return null;
        }

        public override FieldInfo GetField(string name, BindingFlags bindingAttr)
        {
            return null;
        }

        public override Type GetNestedType(string name, BindingFlags bindingAttr)
        {
            return null;
        }

        public override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            return Type.EmptyTypes;
        }

        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            // CLR's reflection returns 0 here.
            return 0;
        }

        protected override MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            return null;
        }

        protected override PropertyInfo GetPropertyImpl(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            return null;
        }

        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            return null;
        }


        public override Type[] GetInterfaces()
        {
            return new Type[0];
        }

        public override Type GetInterface(string name, bool ignoreCase)
        {
            if (name == null) throw new ArgumentNullException("name");
            return null;
        }

        public override MemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            return MetadataOnlyTypeDef.GetMembersHelper(this, bindingAttr);
        }

        public override Guid GUID
        {
            get { return Guid.Empty; }
        }

        protected override bool HasElementTypeImpl()
        {
            return true;
        }

        public override object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
        {
            throw new NotSupportedException();
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return new CustomAttributeData[0];
        }

        #endregion

        public override string ToString()
        {
            return m_type.ToString() + m_mod;
        }


        
        //TypeDef for this type.
        public override int MetadataToken { get {             
            return (int) TokenType.TypeDef;            
        } }

        public override Type[] GetGenericArguments()
        {
            return this.m_type.GetGenericArguments();
        }

        public override Type GetGenericTypeDefinition()
        {
            throw new InvalidOperationException();
        }


        public override System.Reflection.GenericParameterAttributes GenericParameterAttributes
        {
            get { throw new InvalidOperationException(Resources.ValidOnGenericParameterTypeOnly); }
        }

        #region MemberInfo Members

        public override MemberTypes MemberType
        {
            get {
                // T& is not considered nested, so use TypeInfo instead of NestedType.
                return MemberTypes.TypeInfo;
            }
        }

        public override Type DeclaringType
        {
            get {
                // T& is never considered nested, even if T is nested. Always returns null.
                return null;
            }
        }

        public override string Name
        {
            get 
            {
                return m_type.Name + m_mod; 
            }
        }

        public override Assembly Assembly
        {
            get 
            {
                return m_type.Assembly; 
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

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override Type ReflectedType
        {
            get { throw new NotSupportedException(); }
        }

        public override string Namespace
        {
            get 
            {
                return m_type.Namespace;
            }
        }
        #endregion

        protected override TypeCode GetTypeCodeImpl()
        {
            // All modified types are just considered objects.
            return TypeCode.Object;
        }
    }

} // namespace
