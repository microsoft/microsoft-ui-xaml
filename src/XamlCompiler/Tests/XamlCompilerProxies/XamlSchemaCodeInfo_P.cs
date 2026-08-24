// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class XamlSchemaCodeInfo
    {
        static ProxyHelper _xamlSchemaCodeInfoType;
        static PropertyInfo _typeTableProperty;
        static PropertyInfo _userTypeInfoProperty;
        static PropertyInfo _userMemberInfoProperty;
        static PropertyInfo _otherMetadataProvidersProperty;
        static MethodInfo _getFullGenericNestedName;
        static MethodInfo _addTypeMethod;
        static MethodInfo _addTypeAndPropertiesMethod;

        object _instance;

        static XamlSchemaCodeInfo()
        {
            _xamlSchemaCodeInfoType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlSchemaCodeInfo");
            _typeTableProperty = _xamlSchemaCodeInfoType.GetProperty("TypeTable");
            _userTypeInfoProperty = _xamlSchemaCodeInfoType.GetProperty("UserTypeInfo");
            _userMemberInfoProperty = _xamlSchemaCodeInfoType.GetProperty("UserMemberInfo");
            _getFullGenericNestedName = _xamlSchemaCodeInfoType.GetStaticMethod("GetFullGenericNestedName", 3);
            _otherMetadataProvidersProperty = _xamlSchemaCodeInfoType.GetProperty("OtherMetadataProviders");
            _addTypeMethod = _xamlSchemaCodeInfoType.GetMethod("AddType");
            _addTypeAndPropertiesMethod = _xamlSchemaCodeInfoType.GetMethod("AddTypeAndProperties");
        }

        public XamlSchemaCodeInfo(object instance)
        {
            _instance = instance;
        }

        public XamlSchemaCodeInfo()
        {
            _instance = _xamlSchemaCodeInfoType.CreateInstance(); ;
        }

        public object Instance { get { return _instance; } }

        public static string GetFullGenericNestedName(Type type, string programmingLanguage)
        {
            object[] args = new object[] { type, programmingLanguage, false };
            string result = (String)_getFullGenericNestedName.Invoke(null, args);
            return result;
        }

        public String RootNamespace
        {
            get { return (String)_userMemberInfoProperty.GetValue(_instance); }
            set { _userMemberInfoProperty.SetValue(_instance, value); }
        }

        private List<Type> _otherMetadataProviders;
        public List<Type> OtherMetadataProviders
        {
            get
            {
                // TODO - proxy these, right now, it's empty.
                Object list = _otherMetadataProvidersProperty.GetValue(_instance, null);
                return (List<Type>)list;
            }

            set
            {
                // TODO - proxy these, right now, it's empty.
                _otherMetadataProvidersProperty.SetValue(_instance, value);
                this._otherMetadataProviders = value;
            }
        }


        public List<InternalTypeEntry> TypeTable
        {
            get
            {
                List<InternalTypeEntry> result = new List<InternalTypeEntry>();
                Object list = _typeTableProperty.GetValue(_instance, null);
                foreach (Object inst in (IEnumerable)list)
                {
                    result.Add(new InternalTypeEntry(inst)); // this is a proxied type.
                }
                return result;
            }
        }

        public List<InternalUserTypeInfo> UserTypeInfo
        {
            get
            {
                List<InternalUserTypeInfo> result = new List<InternalUserTypeInfo>();
                Object list = _userTypeInfoProperty.GetValue(_instance, null);
                foreach (Object inst in (IEnumerable)list)
                {
                    result.Add(new InternalUserTypeInfo(inst)); // this is a proxied type.
                }
                return result;
            }
        }

        public List<InternalXamlUserMemberInfo> UserMemberInfo
        {
            get
            {
                List<InternalXamlUserMemberInfo> result = new List<InternalXamlUserMemberInfo>();
                Object list = _userTypeInfoProperty.GetValue(_instance, null);
                foreach (Object inst in (IEnumerable)list)
                {
                    result.Add(new InternalXamlUserMemberInfo(inst)); // this is a proxied type.
                }
                return result;
            }
        }



        public InternalTypeEntry AddType(DirectUIXamlType xamlType)
        {
            try
            {
                Object[] args = new Object[] {xamlType.Instance, false };

                Object result = _addTypeMethod.Invoke(_instance, args);
                return new InternalTypeEntry(result);
            }
            catch( Exception e)
            {
                throw e.InnerException;
            }
        }

        public InternalTypeEntry AddTypeAndProperties(DirectUIXamlType xamlType)
        {
            try
            {
                Object[] args = new Object[] { xamlType.Instance };

                Object result = _addTypeAndPropertiesMethod.Invoke(_instance, args);
                return new InternalTypeEntry(result);
            }
            catch (Exception e)
            {
                throw e.InnerException;
            }
        }

    }
}
