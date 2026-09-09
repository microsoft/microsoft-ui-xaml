// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace Win8Xaml.CompilerProxies
{
    public class XamlCodeGenTypeNames
    {
        static ProxyHelper _proxy;

        object _instance;

        static PropertyInfo _propertyStandardName;
        static PropertyInfo _propertySystemName;
        static PropertyInfo _propertyCSharpName;
        static PropertyInfo _propertyCppName;
        static PropertyInfo _propertyVBName;

        static XamlCodeGenTypeNames()
        {
            _proxy = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlCodeGenTypeNames");
            _propertyStandardName = _proxy.GetProperty("StandardName");
            _propertySystemName = _proxy.GetProperty("SystemName");
            _propertyCSharpName = _proxy.GetProperty("CSharpName");
            _propertyCppName = _proxy.GetProperty("CppName");
            _propertyVBName = _proxy.GetProperty("VBName");
        }

        public XamlCodeGenTypeNames()
        {
            _instance = _proxy.CreateInstance();
        }

        public XamlCodeGenTypeNames(object instance)
        {
            _instance = instance;
        }

        public object Instance
        { get { return _instance; } }


    }
}
