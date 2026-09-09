// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Text;

namespace Win8Xaml.CompilerProxies
{
    [DebuggerDisplay("{FullName} Sys={IsSystemType}")]
    public class InternalTypeEntry
    {
        static ProxyHelper _internalTypeEntryType;
        static PropertyInfo _systemNameProperty;
        static PropertyInfo _standardNameProperty;
        static PropertyInfo _nameProperty;
        static PropertyInfo _isSystemTypeProperty;
        static PropertyInfo _userTypeInfoProperty;

        object _instance;

        static InternalTypeEntry()
        {
            _internalTypeEntryType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CodeGen.InternalTypeEntry");
            _systemNameProperty = _internalTypeEntryType.GetProperty("SystemName");
            _standardNameProperty = _internalTypeEntryType.GetProperty("StandardName");
            _nameProperty = _internalTypeEntryType.GetProperty("Name");
            _isSystemTypeProperty = _internalTypeEntryType.GetProperty("IsSystemType");
            _userTypeInfoProperty = _internalTypeEntryType.GetProperty("UserTypeInfo");
        }

        public InternalTypeEntry(object instance)
        {
            _instance = instance;
        }

        public InternalTypeEntry()
        {
            _instance = _internalTypeEntryType.CreateInstance();
        }

        public InternalTypeEntry(XamlCodeGenTypeNames typeNames, DirectUIXamlType xamlType)
        {
            try
            {
                Object[] args = new Object[] { typeNames.Instance, xamlType.Instance};

                _instance = _internalTypeEntryType.CreateInstance(args);
            }
            catch (Exception e)
            {
                throw e.InnerException;
            }
        }

        public String SystemName
        {
            get { return (String)_systemNameProperty.GetValue(_instance, null); }
        }

        public String StandardName
        {
            get { return (String)_standardNameProperty.GetValue(_instance, null); }
        }

        public String Name
        {
            get { return (String)_nameProperty.GetValue(_instance, null); }
        }

        public bool IsSystemType
        {
            get { return (bool)_isSystemTypeProperty.GetValue(_instance, null); }
        }

        public InternalUserTypeInfo UserTypeInfo
        {
            get
            {
                object obj = _userTypeInfoProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalUserTypeInfo(obj);  // this is a proxied type.
            }

        }
    }
}
