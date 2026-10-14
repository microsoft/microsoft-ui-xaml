// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class XamlCompilerReflectionHelper
    {
        static ProxyHelper _xamlCompilerReflectionHelperType;
        static MethodInfo _createCompilerDomRootMethod;
        static MethodInfo _createXbfFilenameInfoArray;

        object _instance;

        static XamlCompilerReflectionHelper()
        {
            _xamlCompilerReflectionHelperType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Core.XamlCompilerReflectionHelper");
            _createCompilerDomRootMethod = _xamlCompilerReflectionHelperType.GetMethod("CreateCompilerDomRoot");
            _createXbfFilenameInfoArray = _xamlCompilerReflectionHelperType.GetMethod("CreateXbfFilenameInfoArray", BindingFlags.Public | BindingFlags.Static);
        }

        public XamlCompilerReflectionHelper()
        {
            _instance = _xamlCompilerReflectionHelperType.CreateInstance();
        }

        public CompilerDomRootToken CreateCompilerDomRoot(object xamlReader)
        {
            Object[] args = new Object[] { xamlReader };
            Object result = _createCompilerDomRootMethod.Invoke(_instance, args);
            return new CompilerDomRootToken(result);
        }

        public XbfFilenameInfoArrayToken CreateXbfFilenameInfoArray(string[] filenames)
        {
            Object[] args = new Object[] { filenames };
            Object result = _createXbfFilenameInfoArray.Invoke(_instance, args);
            return new XbfFilenameInfoArrayToken(result);
        }

    }
}