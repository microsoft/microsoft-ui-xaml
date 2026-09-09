// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class DirectUISystem
    {
        static ProxyHelper _directUiSystemType;
        static PropertyInfo _xamlTypeUniversesProperty;
        static PropertyInfo _platformAssembliesProperty;

        Object _instance;

        static DirectUISystem()
        {
            _directUiSystemType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.DirectUI.DirectUISystem");
            _xamlTypeUniversesProperty = _directUiSystemType.GetProperty("XamlTypeUniverses", true);
            _platformAssembliesProperty = _directUiSystemType.GetProperty("PlatformAssemblies", true);
        }

        public DirectUISystem(object instance)
        {
            _instance = instance;
        }

        public IReadOnlyList<XamlTypeUniverse> XamlTypeUniverses
        {
            get
            {
                return ((IEnumerable)_xamlTypeUniversesProperty.GetValue(_instance, null))
                    .Cast<Object>()
                    .Select(item => new XamlTypeUniverse(item))
                    .ToList();
            }
        }

        public IReadOnlyList<DirectUIAssembly> PlatformAssemblies
        {
            get
            {
                return ((IEnumerable)_platformAssembliesProperty.GetValue(_instance, null))
                    .Cast<Object>()
                    .Select(item => new DirectUIAssembly(item))
                    .ToList();
            }
        }
    }
}
