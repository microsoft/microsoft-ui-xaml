// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class TypeForCodeGen
    {
        static ProxyHelper _typeForCodeGenType;

        static PropertyInfo _standardNameProperty;
        static PropertyInfo _systemNameProperty;
        static PropertyInfo _cFullNameProperty;

        object _instance;

        static TypeForCodeGen()
        {
            _typeForCodeGenType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CodeGen.TypeForCodeGen");
            _standardNameProperty = _typeForCodeGenType.GetProperty("StandardName");
            _systemNameProperty = _typeForCodeGenType.GetProperty("SystemName");
            _cFullNameProperty = _typeForCodeGenType.GetProperty("FullName");
        }

        public TypeForCodeGen(object instance)
        {
            _instance = instance;
        }

        public object Instance
        {
            get { return _instance; }
        }

        public TypeForCodeGen(XamlType xamlType)
        {
            object[] args = new object[] { xamlType.Instance };
            _instance = _typeForCodeGenType.CreateInstance(args);
        }

        public String StandardName
        {
            get { return (String)_standardNameProperty.GetValue(_instance, null); }
        }

        public String SystemName
        {
            get { return (String)_systemNameProperty.GetValue(_instance, null); }
        }

        public Object FullName
        {
            get { return _cFullNameProperty.GetValue(_instance, null); }
        }
    }
}
