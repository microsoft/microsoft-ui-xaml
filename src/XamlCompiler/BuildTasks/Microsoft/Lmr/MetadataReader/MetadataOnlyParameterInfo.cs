// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Text;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Globalization;
using System.Runtime.InteropServices;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    // Implement a ParameterInfo based off System.Reflection.Metadata.
    // Contrast that to code:SimpleParameterInfo, which is implemented purely from the information in the
    // signature without any additional metadata.
    internal class MetadataOnlyParameterInfo : ParameterInfo
    {
        internal MetadataOnlyParameterInfo(MetadataOnlyModule resolver, ParameterHandle paramHandle, Type paramType, CustomModifiers customModifiers)
        {
            m_resolver = resolver;
            m_parameterHandle = paramHandle;

            // Parameter type information is not in the metadata for the parameter, but in the
            // metadata for the method. We pass in the parameter type computed at the parent method
            // for efficiency reason so that we do not access the metadata of the parent method multiple
            // times.
            m_paramType = paramType;
            m_customModifiers = customModifiers;

            var param = resolver.RawReader.GetParameter(paramHandle);
            m_position = param.SequenceNumber - 1;
            m_attrib = param.Attributes;
        }

        void InitializeName()
        {
            if (string.IsNullOrEmpty(m_name))
            {
                var param = m_resolver.RawReader.GetParameter(m_parameterHandle);
                m_name = m_resolver.RawReader.GetString(param.Name);
            }
        }

        #region ParameterInfo Members

        override public System.Reflection.ParameterAttributes Attributes
        {
            get { return m_attrib; }
        }

        public override Type[] GetOptionalCustomModifiers()
        {
            if (m_customModifiers == null)
            {
                return Type.EmptyTypes;
            }
            return m_customModifiers.OptionalCustomModifiers;
        }

        public override Type[] GetRequiredCustomModifiers()
        {
            if (m_customModifiers == null)
            {
                return Type.EmptyTypes;
            }
            return m_customModifiers.RequiredCustomModifiers;
        }

        override public string Name
        {
            get { InitializeName();  return m_name; }
        }

        public override MemberInfo Member
        {
            get 
            {
                // Navigate from parameter to its declaring method
                // The parent method token can be obtained from the parameter's row context
                return m_resolver.ResolveMethod(MetadataTokens.GetToken(m_resolver.GetDeclaringMethodForParameter(m_parameterHandle)));
            }
        }

        override public int Position
        {
            get { return m_position; }
        }

        override public Type ParameterType
        {
            get { return m_paramType; }
        }

        public override int MetadataToken
        {
            get
            {
                return MetadataTokens.GetToken(m_parameterHandle);
            }
        }

        override public Object DefaultValue
        {
            //Should use RawDefaultValue instead.
            get { throw new InvalidOperationException(); }
        }

        override public Object RawDefaultValue
        {
            get { throw new NotImplementedException(); }
        }

        #endregion

        public override bool Equals(object obj)
        {
            MetadataOnlyParameterInfo f = obj as MetadataOnlyParameterInfo;
            if (f != null)
            {
                return f.m_resolver.Equals(m_resolver) && (f.m_parameterHandle.Equals(m_parameterHandle));
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            return m_resolver.GetHashCode() * 32767 + m_parameterHandle.GetHashCode();
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return m_resolver.GetCustomAttributeData((EntityHandle)m_parameterHandle);
        }

        public override string ToString() 
        {
            return string.Format(
                CultureInfo.InvariantCulture, 
                "{0} {1}", 
                MetadataOnlyCommonType.TypeSigToString(ParameterType),
                Name
            );
        }

        /// <summary>
        /// New API to get at any marshaling information on this parameter
        /// </summary>
        public MarshalAsAttribute GetMarshalInfo()
        {
            var param = m_resolver.RawReader.GetParameter(m_parameterHandle);
            var marshallingDesc = param.GetMarshallingDescriptor();
            if (marshallingDesc.IsNil)
            {
                return null;
            }

            var blobReader = m_resolver.RawReader.GetBlobReader(marshallingDesc);
            UnmanagedType u = (UnmanagedType)blobReader.ReadByte();
            var attr = new MarshalAsAttribute(u);
            if (u == UnmanagedType.LPArray)
            {
                UnmanagedType et = (UnmanagedType)blobReader.ReadByte();
                attr.ArraySubType = et;
                if (blobReader.RemainingBytes > 0)
                {
                    int paramNum = blobReader.ReadCompressedInteger();
                    attr.SizeParamIndex = checked((short)paramNum);
                    if (blobReader.RemainingBytes > 0)
                    {
                        int numElem = blobReader.ReadCompressedInteger();
                        attr.SizeConst = numElem;
                    }
                }
                else
                {
                    attr.SizeParamIndex = -1;
                }
            }

            return attr;
        }
        
        readonly private MetadataOnlyModule m_resolver;
        readonly private ParameterHandle m_parameterHandle;
        readonly private ParameterAttributes m_attrib;
        readonly private Type m_paramType;
        readonly private CustomModifiers m_customModifiers;

        private string m_name;

        //position is starting from zero.
        readonly private int m_position;
    }
}
