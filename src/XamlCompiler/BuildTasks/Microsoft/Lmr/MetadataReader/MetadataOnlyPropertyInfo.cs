// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Text;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Implement a PropertyInfo based off System.Reflection.Metadata. 
    /// </summary>
    internal class MetadataOnlyPropertyInfo : PropertyInfo
    {
        // Public constructor for factory pattern.
        public MetadataOnlyPropertyInfo(MetadataOnlyModule resolver, PropertyDefinitionHandle propHandle, Type[] typeArgs, Type[] methodArgs)
        {           
            m_resolver = resolver;
            m_propertyDefHandle = propHandle;
            m_context = new GenericContext(typeArgs, methodArgs);

            var propDef = resolver.RawReader.GetPropertyDefinition(propHandle);
            m_attrib = propDef.Attributes;

            // Decode property signature to get the property type, unwrapping any modreq/modopt wrappers
            var signature = propDef.DecodeSignature(resolver.TypeProvider, m_context);
            m_propertyType = SignatureUnwrap.Unwrap(signature.ReturnType).Type;

            // Get accessor methods
            var accessors = propDef.GetAccessors();
            m_getterHandle = accessors.Getter;
            m_setterHandle = accessors.Setter;
        }

        private void InitializeName()
        {
            if (string.IsNullOrEmpty(m_name))
            {
                var propDef = m_resolver.RawReader.GetPropertyDefinition(m_propertyDefHandle);
                m_name = m_resolver.RawReader.GetString(propDef.Name);
            }
        }

        public override string ToString()
        {
            return DeclaringType.ToString() + "." + Name;
        }

        #region PropertyInfo Members

        public override System.Reflection.PropertyAttributes Attributes
        {
            get { return m_attrib; }
        }

        public override MemberTypes MemberType
        {
            get
            {
                return MemberTypes.Property;
            }
        }

        public override string Name
        {
            get { InitializeName(); return m_name; }
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

        public override Type PropertyType
        {
            get
            {
                return m_propertyType;
            }
        }

        public override Type DeclaringType
        {
            get
            {
                // Find declaring type by walking back from the property handle
                // PropertyDefinition doesn't directly store declaring type in SRM,
                // but we can get it from the TypeDefinition that owns this property.
                var typeDef = m_resolver.GetDeclaringTypeForProperty(m_propertyDefHandle);
                Type declaringType = m_resolver.GetGenericType((EntityHandle)typeDef, m_context);
                Debug.Assert(declaringType != null);
                return declaringType;
            }
        }

        public override Object GetConstantValue()
        {
            throw new NotImplementedException();
        }

        public override int MetadataToken 
        { 
            get 
            { 
                return MetadataTokens.GetToken(m_propertyDefHandle); 
            } 
        }

        public override bool CanRead
        {
            get
            {
                return !m_getterHandle.IsNil;
            }
        }

        public override bool CanWrite
        {
            get
            {
                return !m_setterHandle.IsNil;
            }
        }

        public override MethodInfo[] GetAccessors(bool nonPublic)
        {
            List<MethodInfo> l = new List<MethodInfo>();
            MethodInfo getter = GetGetMethod(nonPublic);
            if (getter != null)
            {
                l.Add(getter);
            }
            MethodInfo setter = GetSetMethod(nonPublic);
            if (setter != null)
            {
                l.Add(setter);
            }
            return l.ToArray();
        }

        public override MethodInfo GetGetMethod(bool nonPublic)
        {
            if (m_getterHandle.IsNil)
            {
                return null;
            }
            MethodInfo getter = m_resolver.GetGenericMethodInfo(m_getterHandle, this.m_context);
            if (nonPublic || getter.IsPublic)
            {
                return getter;
            }
            return null;
        }

        public override MethodInfo GetSetMethod(bool nonPublic)
        {
            if (m_setterHandle.IsNil)
            {
                return null;
            }
            MethodInfo setter = m_resolver.GetGenericMethodInfo(m_setterHandle, this.m_context);
            if (nonPublic || setter.IsPublic)
            {
                return setter;
            }
            return null;
        }

        public override ParameterInfo[] GetIndexParameters()
        {
            MethodInfo getter = GetGetMethod(true);
            if (getter != null)
            {
                return getter.GetParameters();
            }

            return new ParameterInfo[0];
        }

        public override object GetValue(object obj, BindingFlags invokeAttr, Binder binder, object[] index, System.Globalization.CultureInfo culture)
        {
            throw new NotSupportedException();
        }

        public override void SetValue(object obj, object value, BindingFlags invokeAttr, Binder binder, object[] index, System.Globalization.CultureInfo culture)
        {
            throw new NotSupportedException();
        }
        #endregion

        public override Module Module
        {
            get { return m_resolver; }
        }

        public override bool Equals(object obj)
        {
            MetadataOnlyPropertyInfo prop = obj as MetadataOnlyPropertyInfo;
            if (prop != null)
            {
                return prop.m_resolver.Equals(m_resolver) && (prop.m_propertyDefHandle.Equals(m_propertyDefHandle)) &&
                    (DeclaringType.Equals(prop.DeclaringType));
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            return m_resolver.GetHashCode() * 32767 + m_propertyDefHandle.GetHashCode();
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return m_resolver.GetCustomAttributeData((EntityHandle)m_propertyDefHandle);
        }
        
        readonly private MetadataOnlyModule m_resolver;
        readonly private PropertyDefinitionHandle m_propertyDefHandle;
        readonly private PropertyAttributes m_attrib;
        readonly private Type m_propertyType;
        readonly private GenericContext m_context;
        private string m_name;
        readonly private MethodDefinitionHandle m_setterHandle;
        readonly private MethodDefinitionHandle m_getterHandle;
    }
}
