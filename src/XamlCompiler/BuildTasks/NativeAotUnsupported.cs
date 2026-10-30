// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#if NET10_0_OR_GREATER
using System;
using System.Globalization;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    /// <summary>
    /// The exception thrown by the members of the shared sources (mostly the runtime half of the System.Xaml fork: object
    /// writing/reading, invokers, value serializers and type converters, markup extension evaluation) that are compiled out
    /// of the Native AOT build of the XAML compiler.
    /// </summary>
    /// <remarks>
    /// Those members create, invoke or convert runtime objects through reflection or code generation, which isn't
    /// supported by Native AOT. The compiler never calls them (it only reads XAML and reflects over metadata): the
    /// Native AOT compiler doesn't even include them in the binary (they're not reachable from the entry point).
    /// They keep their signature, so the code that references them still compiles, but they throw instead of doing
    /// something wrong silently, should that ever change.
    /// </remarks>
    internal static class NativeAotUnsupported
    {
        public static NotSupportedException Create(string member)
        {
            return new NotSupportedException(string.Format(CultureInfo.InvariantCulture,
                "{0} isn't supported in the Native AOT build of the XAML compiler: it needs runtime reflection or code generation, and the compiler doesn't use it.",
                member));
        }
    }
}
#endif
