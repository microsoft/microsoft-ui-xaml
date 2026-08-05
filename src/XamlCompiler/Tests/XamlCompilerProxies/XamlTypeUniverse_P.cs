// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class XamlTypeUniverse
    {
        static ProxyHelper _xamlTypeUniverseType;
        static MethodInfo _loadAssemblyFromFile;
        static MethodInfo _getSystemAssembly;
        static PropertyInfo _isSystemAssemblyLoaded;
        static EventInfo _OnResolveEvent;

        object _instance;

        static XamlTypeUniverse()
        {
            _xamlTypeUniverseType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Lmr.XamlTypeUniverse");
            _loadAssemblyFromFile = _xamlTypeUniverseType.GetMethod("LoadAssemblyFromFile");
            _getSystemAssembly = _xamlTypeUniverseType.GetMethod("GetSystemAssembly");

            _isSystemAssemblyLoaded = _xamlTypeUniverseType.GetProperty("IsSystemAssemblyLoaded");

            _OnResolveEvent = _xamlTypeUniverseType.GetEvent("OnResolveEvent");
        }

        public XamlTypeUniverse(object instance)
        {
            _instance = instance;
        }

        public XamlTypeUniverse(bool useProjections)
        {
            Object[] args = new Object[] { useProjections };
            _instance = _xamlTypeUniverseType.CreateInstance(args);
        }

        public Object Instance
        {
            get { return _instance; }
        }

        public Assembly LoadAssemblyFromFile(String path)
        {
            Object[] args = new Object[] { path };
            Object result = _loadAssemblyFromFile.Invoke(_instance, args);
            return (Assembly)result;
        }

        public Assembly GetSystemAssembly()
        {
            Object result = _getSystemAssembly.Invoke(_instance, null);
            return (Assembly)result;
        }


        public bool IsSystemAssemblyLoaded
        {
            get { return (bool)_isSystemAssemblyLoaded.GetValue(_instance, null); }
            set { _isSystemAssemblyLoaded.SetValue(_instance, value); }
        }

        public event EventHandler<ResolveAssemblyNameEventArgs> OnResolveEvent
        {
            add
            {
                OnResolveEvent_Proxy += value;
                if (!_haveRegisteredForEvent)
                {
                    _haveRegisteredForEvent = true;
                    var del = new EventHandler<ResolveAssemblyNameEventArgs>(OnResolveEvent_ProxyHandler);
                    _OnResolveEvent.AddEventHandler(this, del);
                }
            }
            remove
            {
                OnResolveEvent_Proxy -= value;
            }
        }

        event EventHandler<ResolveAssemblyNameEventArgs> OnResolveEvent_Proxy;
        bool _haveRegisteredForEvent;

        private void OnResolveEvent_ProxyHandler(object sender, ResolveAssemblyNameEventArgs e)
        {
            if (OnResolveEvent_Proxy != null)
            {
                OnResolveEvent_Proxy(sender, e);
            }
        }

    }

    public class ResolveAssemblyNameEventArgs:EventArgs { }  // this needs to be a proxy
}
