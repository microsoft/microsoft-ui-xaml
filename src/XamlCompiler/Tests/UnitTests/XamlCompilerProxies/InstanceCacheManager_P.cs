// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    /// <summary>
    /// Proxy for Microsoft.UI.Xaml.Markup.Compiler.Core.InstanceCacheManager.
    /// Clears the compiler's process-wide per-compilation caches. Tests call this before building
    /// each schema so assembly classification cannot leak between tests.
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
