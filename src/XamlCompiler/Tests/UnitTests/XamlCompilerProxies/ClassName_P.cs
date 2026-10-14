// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class ClassName
    {
        static ProxyHelper _xcciType;
        static PropertyInfo _namespaceProperty;
        static PropertyInfo _shortNameProperty;
        static PropertyInfo _fullNameProperty;
        static MethodInfo _toStringMethod;

        public object Instance { get; }

        static ClassName()
        {
            _xcciType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.ClassName");
            _namespaceProperty = _xcciType.GetProperty("Namespace");
            _shortNameProperty = _xcciType.GetProperty("ShortName");
            _fullNameProperty = _xcciType.GetProperty("FullName");
            _toStringMethod = _xcciType.GetMethod("ToString");
        }

        public ClassName(string fullName)
        {
            Object[] args = new Object[] { fullName };
            Instance = _xcciType.CreateInstance(args);
        }

        public ClassName(object instance)
        {
            Instance = instance;
        }

        public string Namespace
        {
            get { return (string)_namespaceProperty.GetValue(Instance, null); }
            set { _namespaceProperty.SetValue(Instance, value); }
        }

        public string ShortName
        {
            get { return (string)_shortNameProperty.GetValue(Instance, null); }
            set { _shortNameProperty.SetValue(Instance, value); }
        }

        public string FullName
        {
            get { return (string)_fullNameProperty.GetValue(Instance, null); }
            set { _fullNameProperty.SetValue(Instance, value); }
        }
    }
}