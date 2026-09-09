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
    public class DirectUIAssembly: Assembly
    {
        static ProxyHelper _directUIAssemblyType;
        static PropertyInfo _wrappedAssemblyProperty;

        object _instance;

        static DirectUIAssembly()
        {
            _directUIAssemblyType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.DirectUI.DirectUIAssembly");
            _wrappedAssemblyProperty = _directUIAssemblyType.GetProperty("WrappedAssembly");
        }

        public DirectUIAssembly(Object instance)
        {
            _instance = instance;
        }

        public Assembly WrappedAssembly
        {
            get
            {
                Assembly asm = (Assembly)_wrappedAssemblyProperty.GetValue(_instance, null);
                return asm;
            }
        }
    }
}
