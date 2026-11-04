// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace Win8Xaml.CompilerProxies
{
    [DebuggerDisplay("{FullName}")]
    public class InternalUserTypeInfo
    {
        static ProxyHelper _internalXamlUserTypeInfoType;
        static PropertyInfo _nameEntryProperty;
        static PropertyInfo _baseTypeProperty;
        static PropertyInfo _contentPropertyProperty;
        static PropertyInfo _isArrayProperty;
        static PropertyInfo _isCollectionProperty;
        static PropertyInfo _isConstructibleProperty;
        static PropertyInfo _isDictionaryProperty;
        static PropertyInfo _isMarkupExtensionProperty;
        static PropertyInfo _itemTypeProperty;
        static PropertyInfo _keyTypeProperty;
        static PropertyInfo _membersProperty;

        object _instance;

        static InternalUserTypeInfo()
        {
            _internalXamlUserTypeInfoType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.InternalUserTypeInfo");
            _nameEntryProperty = _internalXamlUserTypeInfoType.GetProperty("NameEntry");
            _baseTypeProperty = _internalXamlUserTypeInfoType.GetProperty("BaseType");
            _contentPropertyProperty = _internalXamlUserTypeInfoType.GetProperty("ContentProperty");
            _isArrayProperty = _internalXamlUserTypeInfoType.GetProperty("IsArray");
            _isCollectionProperty = _internalXamlUserTypeInfoType.GetProperty("IsCollection");
            _isConstructibleProperty = _internalXamlUserTypeInfoType.GetProperty("IsConstructible");
            _isDictionaryProperty = _internalXamlUserTypeInfoType.GetProperty("IsDictionary");
            _isMarkupExtensionProperty = _internalXamlUserTypeInfoType.GetProperty("IsMarkupExtension");
            _itemTypeProperty = _internalXamlUserTypeInfoType.GetProperty("ItemType");
            _keyTypeProperty = _internalXamlUserTypeInfoType.GetProperty("KeyType");
            _membersProperty = _internalXamlUserTypeInfoType.GetProperty("Members");
        }

        public InternalUserTypeInfo()
        {
            _instance = _internalXamlUserTypeInfoType.CreateInstance();
        }
        public InternalUserTypeInfo(object instance)
        {
            _instance = instance;
        }

        public InternalTypeEntry NameEntry
        {
            get
            {
                object obj = _nameEntryProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalTypeEntry(obj);  // this is a proxied type.
            }
        }

        public InternalTypeEntry BaseType
        {
            get
            {
                object obj = _baseTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalTypeEntry(obj);  // this is a proxied type.
            }
        }

        public InternalXamlUserMemberInfo ContentProperty
        {
            get
            {
                object obj = _itemTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalXamlUserMemberInfo(obj);  // this is a proxied type.
            }
        }

        public bool IsArray
        {
            get { return (bool)_isArrayProperty.GetValue(_instance, null); }
        }

        public bool IsCollection
        {
            get { return (bool)_isCollectionProperty.GetValue(_instance, null); }
        }

        public bool IsConstructible
        {
            get { return (bool)_isConstructibleProperty.GetValue(_instance, null); }
        }

        public bool IsDictionary
        {
            get { return (bool)_isDictionaryProperty.GetValue(_instance, null); }
        }

        public bool IsMarkupExtension
        {
            get { return (bool)_isMarkupExtensionProperty.GetValue(_instance, null); }
        }

        public InternalTypeEntry ItemType
        {
            get
            {
                object obj = _itemTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalTypeEntry(obj);    // this is a proxied type.
            }
        }

        public InternalTypeEntry KeyType
        {
            get
            {
                object obj = _itemTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalTypeEntry(obj);    // this is a proxied type.
            }
        }

        public List<InternalXamlUserMemberInfo> Members
        {
            get
            {
                List<InternalXamlUserMemberInfo> result = new List<InternalXamlUserMemberInfo>();
                Object list = _membersProperty.GetValue(_instance, null);
                foreach (Object inst in (IEnumerable)list)
                {
                    result.Add(new InternalXamlUserMemberInfo(inst)); // this is a proxied type.
                }
                return result;
            }
        }

    }
}
