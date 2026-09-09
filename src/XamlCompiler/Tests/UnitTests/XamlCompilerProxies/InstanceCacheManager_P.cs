// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    /// <summary>
    /// Proxy for Microsoft.UI.Xaml.Markup.Compiler.Core.InstanceCacheManager.
    ///
    /// The compiler holds a number of process-wide caches (Core.InstanceCache&lt;,&gt;), each of which
    /// registers a clear action here. They are NOT meant to outlive a single compilation:
    /// CompileXamlInternal calls ClearCache() from the finally block of every compile pass, which is
    /// what scopes them. FileHelpers' platform/WinUI assembly cache is one of them, and its comment
    /// says so explicitly - "we need to make sure we don't re-use these across msbuild instances".
    ///
    /// The unit tests drive the schema directly and never go through CompileXamlInternal, so nothing
    /// used to reset these caches: the first schema built in the process decided, for every later
    /// test, which assembly was "the platform assembly" and which was "the WinUI assembly". That made
    /// the suite order-dependent. TestHelper.LoadSchema now clears them, giving each schema the same
    /// clean slate a real compile pass gets.
    /// </summary>
    public static class InstanceCacheManager
    {
        static readonly MethodInfo _clearCache;

        static InstanceCacheManager()
        {
            var helper = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Core.InstanceCacheManager");
            _clearCache = helper.GetStaticMethod("ClearCache");
        }

        public static void ClearCache()
        {
            _clearCache.Invoke(null, null);
        }
    }
}
