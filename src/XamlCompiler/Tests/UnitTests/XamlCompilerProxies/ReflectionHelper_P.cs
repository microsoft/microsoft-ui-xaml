// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    /// <summary>
    /// Proxy for Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper.
    ///
    /// The compiler caches custom-attribute data in a process-wide static
    /// (<c>_typeAttrs</c>), keyed by assembly-qualified type *name*. It is deliberately not an
    /// <c>InstanceCache&lt;,&gt;</c>, so <c>InstanceCacheManager.ClearCache()</c> does not touch it;
    /// the only reset is <c>Release()</c>, which <c>CompileXamlInternal.UnloadReferences</c> calls
    /// when it tears the type universe down.
    ///
    /// This matters to the tests because the cached <c>CustomAttributeData</c> values reach
    /// <c>Constructor.DeclaringType -&gt; Module -&gt; Assembly</c>, so anything left in this cache
    /// pins the whole type universe it came from. TestHelper releases it alongside the universes it
    /// owns, mirroring UnloadReferences.
    /// </summary>
    public static class ReflectionHelper
    {
        static readonly MethodInfo _release;

        static ReflectionHelper()
        {
            var helper = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper");
            _release = helper.GetMethod("Release", BindingFlags.NonPublic | BindingFlags.Static);
        }

        public static void Release()
        {
            _release.Invoke(null, null);
        }
    }
}
