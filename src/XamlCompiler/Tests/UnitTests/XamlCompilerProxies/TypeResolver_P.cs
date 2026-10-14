// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class TypeResolver
    {
        static ProxyHelper _typeResolverType;
        static MethodInfo _initializeTypeNameMap;
        static MethodInfo _getTypeByFullName;
        static MethodInfo _getDirectUIType;

        object _instance;

        static TypeResolver()
        {
            _typeResolverType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.DirectUI.TypeResolver");
            _initializeTypeNameMap = _typeResolverType.GetMethod("InitializeTypeNameMap");
            _getTypeByFullName = _typeResolverType.GetMethod("GetTypeByFullName");
            _getDirectUIType = _typeResolverType.GetMethod("GetDirectUIType");
        }

        public TypeResolver(XamlTypeUniverse typeUniverse)
        {
            Object[] args = new Object[] { typeUniverse.Instance };
            _instance = _typeResolverType.CreateInstance(args);
        }

        public TypeResolver(object instance)
        {
            _instance = instance;
        }

        public Object Instance
        {
            get { return _instance; }
        }

        public void InitializeTypeNameMap()
        {
            _initializeTypeNameMap.Invoke(_instance, null);
        }

        public Type GetTypeByFullName(string fullname)
        {
            Object[] args = new Object[] { fullname };
            Type type = (Type)_getTypeByFullName.Invoke(_instance, args);
            return type;
        }

        public Type GetDirectUIType(string name)
        {
            Object[] args = new Object[] { name };
            Type type = (Type)_getDirectUIType.Invoke(_instance, args);
            return type;
        }
    }

}
