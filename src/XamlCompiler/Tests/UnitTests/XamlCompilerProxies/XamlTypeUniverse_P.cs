// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace Win8Xaml.CompilerProxies
{
    public class XamlTypeUniverse : IDisposable
    {
        private sealed class LifetimeState
        {
            public bool IsDisposed;
        }

        // DirectUISystem can create multiple proxy wrappers for the same reflected universe.
        // Share disposal state by underlying instance so none of those wrappers can use it later.
        private static readonly ConditionalWeakTable<object, LifetimeState> s_lifetimes =
            new ConditionalWeakTable<object, LifetimeState>();

        static ProxyHelper _xamlTypeUniverseType;
        static MethodInfo _loadAssemblyFromFile;
        static MethodInfo _getSystemAssembly;
        static MethodInfo _dispose;
        static PropertyInfo _isSystemAssemblyLoaded;
        static EventInfo _OnResolveEvent;

        readonly object _instance;
        readonly LifetimeState _lifetime;

        static XamlTypeUniverse()
        {
            _xamlTypeUniverseType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Lmr.XamlTypeUniverse");
            _loadAssemblyFromFile = _xamlTypeUniverseType.GetMethod("LoadAssemblyFromFile");
            _getSystemAssembly = _xamlTypeUniverseType.GetMethod("GetSystemAssembly");
            _dispose = _xamlTypeUniverseType.GetMethod("Dispose");

            _isSystemAssemblyLoaded = _xamlTypeUniverseType.GetProperty("IsSystemAssemblyLoaded");

            _OnResolveEvent = _xamlTypeUniverseType.GetEvent("OnResolveEvent");
        }

        public XamlTypeUniverse(object instance)
        {
            if (instance == null)
            {
                throw new ArgumentNullException(nameof(instance));
            }

            _instance = instance;
            _lifetime = s_lifetimes.GetValue(instance, key => new LifetimeState());
        }

        public XamlTypeUniverse(bool useProjections)
        {
            Object[] args = new Object[] { useProjections };
            _instance = _xamlTypeUniverseType.CreateInstance(args);
            _lifetime = s_lifetimes.GetValue(_instance, key => new LifetimeState());
        }

        public Object Instance
        {
            get
            {
                ThrowIfDisposed();
                return _instance;
            }
        }

        public Assembly LoadAssemblyFromFile(String path)
        {
            ThrowIfDisposed();
            Object[] args = new Object[] { path };
            Object result = _loadAssemblyFromFile.Invoke(_instance, args);
            return (Assembly)result;
        }

        public Assembly GetSystemAssembly()
        {
            ThrowIfDisposed();
            Object result = _getSystemAssembly.Invoke(_instance, null);
            return (Assembly)result;
        }

        /// <summary>
        /// Releases the native metadata objects held by this universe's assemblies and modules.
        /// The universe must not be used afterwards. CompileXamlInternal.UnloadReferences does the
        /// same when it retires its cached universe.
        /// </summary>
        public void Dispose()
        {
            lock (_lifetime)
            {
                if (_lifetime.IsDisposed)
                {
                    return;
                }

                _dispose.Invoke(_instance, null);
                _lifetime.IsDisposed = true;
            }
        }


        public bool IsSystemAssemblyLoaded
        {
            get
            {
                ThrowIfDisposed();
                return (bool)_isSystemAssemblyLoaded.GetValue(_instance, null);
            }
        }

        public event EventHandler<ResolveAssemblyNameEventArgs> OnResolveEvent
        {
            add
            {
                ThrowIfDisposed();
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
                ThrowIfDisposed();
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

        private void ThrowIfDisposed()
        {
            lock (_lifetime)
            {
                if (_lifetime.IsDisposed)
                {
                    throw new ObjectDisposedException(nameof(XamlTypeUniverse));
                }
            }
        }
    }

    public class ResolveAssemblyNameEventArgs:EventArgs { }  // this needs to be a proxy
}
