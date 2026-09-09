// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
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
            // DirectUISystem's single 'WindowsWinmds' property became the 'PlatformAssemblies' list
            // when WinUI gained more than one platform winmd. Both are lists; the proxy exposes the
            // first entry, which is what the single-valued property used to return.
            _platformAssembliesProperty = _directUiSystemType.GetProperty("PlatformAssemblies", true);
        }

        public DirectUISystem(object instance)
        {
            _instance = instance;
        }

        public XamlTypeUniverse XamlTypeUniverse
        {
            get
            {
                Object xamlTypeUniverse = First(_xamlTypeUniversesProperty.GetValue(_instance, null));
                return new XamlTypeUniverse(xamlTypeUniverse);
            }
        }

        public DirectUIAssembly WindowsWinmd
        {
            get
            {
                Object duiAsmInstance = First(_platformAssembliesProperty.GetValue(_instance, null));
                DirectUIAssembly duiAsm = new DirectUIAssembly(duiAsmInstance);
                return duiAsm;
            }
        }

        private static Object First(Object list)
        {
            IEnumerable items = list as IEnumerable;
            if (items == null)
            {
                return list;
            }
            foreach (Object item in items)
            {
                return item;
            }
            return null;
        }
    }
}
