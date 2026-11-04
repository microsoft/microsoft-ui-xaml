// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{

    public class XamlDirective
    {
        object _instance;

        public XamlDirective(object instance)
        {
            _instance = instance;
        }

        public object Instance { get { return _instance; } }
    }

    public class XamlLanguage
    {
        static ProxyHelper _xamlLanguageProxy;
        static PropertyInfo _langProperty;

        static XamlLanguage()
        {
            _xamlLanguageProxy = new ProxyHelper("System.Xaml.XamlLanguage");
            _langProperty = _xamlLanguageProxy.GetProperty("Lang");
        }

        public static XamlDirective Lang
        {
            get
            {
                return new XamlDirective(_langProperty.GetValue(null));
            }
        }
    }

    public class XamlTypeName
    {
        static ProxyHelper _xamltypeNameProxy;

        object _instance;

        static XamlTypeName()
        {
            _xamltypeNameProxy = new ProxyHelper("System.Xaml.Schema.XamlTypeName");
        }

        public XamlTypeName(string xamlNamespace, string name)
        {
            try
            {
                object[] args = new object[] { xamlNamespace, name };
                _instance = _xamltypeNameProxy.CreateInstance(args);
            }
            catch (Exception e)
            {
                throw e.InnerException;
            }
        }

        public object Instance { get { return _instance; } }
    }

    public class XamlType
    {
        static ProxyHelper _xamlTypeProxy;
        static PropertyInfo _keyTypeProperty;
        static PropertyInfo _nameProperty;
        static PropertyInfo _isNullableProperty;
        static PropertyInfo _isMarkupExtensionProperty;
        static PropertyInfo _markupExtensionReturnTypeProperty;
        static PropertyInfo _isWhitespaceSignificantCollectionProperty;
        static PropertyInfo _allowedContentTypesProperty;
        static MethodInfo _getAllMembersMethod;
        static MethodInfo _getAllAttachableMembersMethod;
        static MethodInfo _getMemberMethod;
        static MethodInfo _getAliasedPropertyMethod;

        object _instance;

        static XamlType()
        {
            _xamlTypeProxy = new ProxyHelper("System.Xaml.XamlType");
            _keyTypeProperty = _xamlTypeProxy.GetProperty("KeyType");
            _nameProperty = _xamlTypeProxy.GetProperty("Name");
            _isNullableProperty = _xamlTypeProxy.GetProperty("IsNullable");
            _isMarkupExtensionProperty = _xamlTypeProxy.GetProperty("IsMarkupExtension");
            _markupExtensionReturnTypeProperty = _xamlTypeProxy.GetProperty("MarkupExtensionReturnType");
            _isWhitespaceSignificantCollectionProperty = _xamlTypeProxy.GetProperty("IsWhitespaceSignificantCollection");
            _allowedContentTypesProperty = _xamlTypeProxy.GetProperty("AllowedContentTypes");
            _getAllMembersMethod = _xamlTypeProxy.GetMethod("GetAllMembers");
            _getAllAttachableMembersMethod = _xamlTypeProxy.GetMethod("GetAllAttachableMembers");
            _getMemberMethod = _xamlTypeProxy.GetMethod("GetMember");
            _getAliasedPropertyMethod = _xamlTypeProxy.GetMethod("GetAliasedProperty");
        }

        public XamlType(object instance)
        {
            _instance = instance;
        }

        public XamlType(Type underlyingType, object xamlSchemaContext)
        {
            try
            {
                object[] args = new object[] { underlyingType, xamlSchemaContext };
                _instance = _xamlTypeProxy.CreateInstance(args);
            }
            catch (Exception e)
            {
                throw e.InnerException;
            }
        }

        public object Instance { get { return _instance; } }

        public XamlType KeyType
        {
            get
            {
                return new XamlType(_keyTypeProperty.GetValue(_instance));
            }
        }

        public string Name
        {
            get
            {
                return (string)_nameProperty.GetValue(_instance);
            }
        }

        public bool IsNullable
        {
            get
            {
                return (bool)_isNullableProperty.GetValue(_instance);
            }
        }

        public bool IsMarkupExtension
        {
            get
            {
                return (bool)_isMarkupExtensionProperty.GetValue(_instance);
            }
        }

        public XamlType MarkupExtensionReturnType
        {
            get
            {
                return new XamlType(_markupExtensionReturnTypeProperty.GetValue(_instance));
            }
        }

        public bool IsWhitespaceSignificantCollection
        {
            get
            {
                return (bool)_isWhitespaceSignificantCollectionProperty.GetValue(_instance);
            }
        }

        public IList<XamlType> AllowedContentTypes
        {
            get
            {
                IEnumerable objectList = (IEnumerable)_allowedContentTypesProperty.GetValue(_instance);
                List<XamlType> allowedContentTypes = new List<XamlType>();
                foreach (object type in objectList)
                {
                    allowedContentTypes.Add(new XamlType(type));
                }
                return allowedContentTypes;
            }
        }

        public ICollection<XamlMember> GetAllMembers()
        {
            IEnumerable objectList = (IEnumerable)_getAllMembersMethod.Invoke(_instance, null);
            List<XamlMember> allMembers = new List<XamlMember>();
            foreach (object member in objectList)
            {
                allMembers.Add(new XamlMember(member));
            }
            return allMembers;
        }

        public ICollection<XamlMember> GetAllAttachableMembers()
        {
            IEnumerable objectList = (IEnumerable)_getAllAttachableMembersMethod.Invoke(_instance, null);
            List<XamlMember> allMembers = new List<XamlMember>();
            foreach (object member in objectList)
            {
                allMembers.Add(new XamlMember(member));
            }
            return allMembers;
        }

        public XamlMember GetMember(string name)
        {
            return new XamlMember(_getMemberMethod.Invoke(_instance, new object[] { name }));
        }

        public XamlMember GetAliasedProperty(XamlDirective directive)
        {
            return new XamlMember(_getAliasedPropertyMethod.Invoke(_instance, new object[] { directive.Instance }));
        }
    }

}