// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class DirectUISystem
    {
        static ProxyHelper _directUiSystemType;
        static PropertyInfo _xamlTypeUniverseProperty;
        static PropertyInfo _windowsWinmdProperty;

        Object _instance;

        static DirectUISystem()
        {
            _directUiSystemType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.DirectUI.DirectUISystem");
            _xamlTypeUniverseProperty = _directUiSystemType.GetProperty("XamlTypeUniverses", true);
            _windowsWinmdProperty = _directUiSystemType.GetProperty("WindowsWinmds", true);
        }

        public DirectUISystem(object instance)
        {
            _instance = instance;
        }

        public XamlTypeUniverse XamlTypeUniverse
        {
            get
            {
                Object xamlTypeUniverse = _xamlTypeUniverseProperty.GetValue(_instance, null);
                return new XamlTypeUniverse(xamlTypeUniverse);
            }
        }

        public DirectUIAssembly WindowsWinmd
        {
            get
            {
                Object duiAsmInstance = _windowsWinmdProperty.GetValue(_instance, null);
                DirectUIAssembly duiAsm = new DirectUIAssembly(duiAsmInstance);
                return duiAsm;
            }
        }
    }
}
