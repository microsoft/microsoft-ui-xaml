// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Implement a FieldInfo based off System.Reflection.Metadata. 
    /// </summary>
    internal class MetadataOnlyFieldInfo : FieldInfo
    {
        public MetadataOnlyFieldInfo(MetadataOnlyModule resolver, FieldDefinitionHandle fieldDefHandle, Type[] typeArgs, Type[] methodArgs)
        {
            m_resolver = resolver;
            m_fieldDefHandle = fieldDefHandle;
            m_context = new GenericContext(typeArgs, methodArgs);

            var fieldDef = resolver.RawReader.GetFieldDefinition(fieldDefHandle);
            m_attrib = fieldDef.Attributes;
            m_declaringTypeDef = fieldDef.GetDeclaringType();
        }
        
        /// <summary>
        /// Gets just field name. If this is never needed we avoid allocating string for it.
        /// </summary>
        private void InitializeName()
        {
            if (string.IsNullOrEmpty(m_name))
            {
                var fieldDef = m_resolver.RawReader.GetFieldDefinition(m_fieldDefHandle);
                m_name = m_resolver.RawReader.GetString(fieldDef.Name);
            }
        }

        private void Initialize()
        {
            if (m_initialized) return;

            var fieldDef = m_resolver.RawReader.GetFieldDefinition(m_fieldDefHandle);

            // Resolve declaring type context
            if (!m_declaringTypeDef.IsNil)
            {
                Type ownerType = m_resolver.ResolveTypeDef(m_declaringTypeDef);
                if (ownerType.IsGenericType && (m_context == null || m_context.TypeArgs == null || m_context.TypeArgs.Length == 0))
                {
                    if (m_context == null)
                    {
                        m_context = new GenericContext(ownerType.GetGenericArguments(), null);
                    }
                    else
                    {
                        m_context = new GenericContext(ownerType.GetGenericArguments(), m_context.MethodArgs);
                    }
                }
            }

            // Decode field signature, unwrapping modreq/modopt wrappers to get bare type + modifiers
            var descriptor = SignatureUnwrap.Unwrap(fieldDef.DecodeSignature(m_resolver.TypeProvider, m_context));
            m_fieldType = descriptor.Type;
            m_customModifiers = descriptor.CustomModifiers;

            m_initialized = true;
        }
                
        // This definition matches the CLR's 
        public override string ToString()
        {
            return (MetadataOnlyCommonType.TypeSigToString(this.FieldType) + " " + this.Name);
        } 


        //The method ParseDefaultValue() returns the value of a field stored in the metadata.
        //The method is called to get the value of a literal field.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity")]
        private object ParseDefaultValue()
        {
            Initialize();

            var fieldDef = m_resolver.RawReader.GetFieldDefinition(m_fieldDefHandle);
            var constantHandle = fieldDef.GetDefaultValue();
            if (constantHandle.IsNil)
            {
                throw new InvalidOperationException();
            }

            var constant = m_resolver.RawReader.GetConstant(constantHandle);
            var blobReader = m_resolver.RawReader.GetBlobReader(constant.Value);

            switch (constant.TypeCode)
            {
                case ConstantTypeCode.Boolean:
                    return blobReader.ReadByte() != 0;
                case ConstantTypeCode.Char:
                    return blobReader.ReadChar();
                case ConstantTypeCode.SByte:
                    return blobReader.ReadSByte();
                case ConstantTypeCode.Byte:
                    return blobReader.ReadByte();
                case ConstantTypeCode.Int16:
                    return blobReader.ReadInt16();
                case ConstantTypeCode.UInt16:
                    return blobReader.ReadUInt16();
                case ConstantTypeCode.Int32:
                    return blobReader.ReadInt32();
                case ConstantTypeCode.UInt32:
                    return blobReader.ReadUInt32();
                case ConstantTypeCode.Int64:
                    return blobReader.ReadInt64();
                case ConstantTypeCode.UInt64:
                    return blobReader.ReadUInt64();
                case ConstantTypeCode.Single:
                    return blobReader.ReadSingle();
                case ConstantTypeCode.Double:
                    return blobReader.ReadDouble();
                case ConstantTypeCode.String:
                    return blobReader.ReadUTF16(blobReader.Length);
                case ConstantTypeCode.NullReference:
                    return null;
                default:
                    throw new InvalidOperationException(Resources.IncorrectElementTypeValue);
            }
        }

        #region FieldInfo Members

        public override FieldAttributes Attributes
        {
            get { return m_attrib; }
        }

        public override MemberTypes MemberType
        {
            get
            {
                return MemberTypes.Field;
            }
        }

        public override string Name
        {
            get
            {
                InitializeName();
                return m_name;
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

        public override Type[] GetOptionalCustomModifiers()
        {
            Initialize();
            return m_customModifiers.OptionalCustomModifiers;
        }

        public override Type[] GetRequiredCustomModifiers()
        {
            Initialize();
            return m_customModifiers.RequiredCustomModifiers;
        }

        public override Type FieldType
        {
            get
            {
                Initialize();
                return m_fieldType;
            }
        }

        public override Type DeclaringType
        {
            get
            {
                Initialize();
                Type declaringType = m_resolver.GetGenericType((EntityHandle)m_declaringTypeDef, m_context);
                Debug.Assert(declaringType != null);
                return declaringType;
            }
        }

        public override Object GetValue(Object obj)
        {
            // This gets the 'live' value on the given instance objectInstance.
            // since LMR is a static metadata reader, there are no live objects and this must fail.
            //Metadata only contains value info for literal fields.
            throw new NotSupportedException();
        }

        public override Object GetRawConstantValue()
        {
            // See Ecma 15.1.2 for more information about literal fields. 
            // "Literal fields become part of the metadata but cannot be accessed by the code."
            if (!this.IsLiteral)
            {
                throw new InvalidOperationException(Resources.OperationValidOnLiteralFieldsOnly);
            }
            return ParseDefaultValue();
        }

        public override RuntimeFieldHandle FieldHandle
        {
            get { throw new NotSupportedException(); }
        }

        public override void SetValue(object obj, object value, BindingFlags invokeAttr, Binder binder, System.Globalization.CultureInfo culture)
        {
            throw new NotSupportedException();
        }

        public override int MetadataToken { get { return MetadataTokens.GetToken(m_fieldDefHandle); } }
        #endregion

        public override Module Module
        {
            get { return m_resolver; }
        }
        
        public override bool Equals(object obj)
        {
            MetadataOnlyFieldInfo f = obj as MetadataOnlyFieldInfo;
            if (f != null)
            {
                return f.m_resolver.Equals(m_resolver) && (f.m_fieldDefHandle.Equals(m_fieldDefHandle)) &&
                    (DeclaringType.Equals(f.DeclaringType));
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            return m_resolver.GetHashCode() * 32767 + m_fieldDefHandle.GetHashCode();
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return m_resolver.GetCustomAttributeData((EntityHandle)m_fieldDefHandle);
        }

        readonly private MetadataOnlyModule m_resolver;
        readonly private FieldDefinitionHandle m_fieldDefHandle;
        readonly private FieldAttributes m_attrib;
        readonly private TypeDefinitionHandle m_declaringTypeDef;
        private Type m_fieldType;
        private CustomModifiers m_customModifiers;
        private GenericContext m_context;
        private string m_name;
        private bool m_initialized;
    }
}
