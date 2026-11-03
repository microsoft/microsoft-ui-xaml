// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Reflection;

namespace System.Xaml.Schema
{
    // The Native AOT build of the XAML compiler: SafeReflectionInvoker creates delegates and instances and invokes methods of
    // user types (with Reflection.Emit in partial trust), which is only done by the runtime half of System.Xaml (object writer
    // and reader, invokers, value converter instances). The compiler never does that, so the members throw instead.
    static class SafeReflectionInvoker
    {
        internal static Delegate CreateDelegate(Type delegateType, Type targetType, string methodName)
        {
            throw Microsoft.UI.Xaml.Markup.Compiler.NativeAotUnsupported.Create("SafeReflectionInvoker.CreateDelegate");
        }

        internal static Delegate CreateDelegate(Type delegateType, object target, string methodName)
        {
            throw Microsoft.UI.Xaml.Markup.Compiler.NativeAotUnsupported.Create("SafeReflectionInvoker.CreateDelegate");
        }

        internal static object CreateInstance(Type type, object[] arguments)
        {
            throw Microsoft.UI.Xaml.Markup.Compiler.NativeAotUnsupported.Create("SafeReflectionInvoker.CreateInstance");
        }

        internal static object InvokeMethod(MethodInfo method, object instance, object[] args)
        {
            throw Microsoft.UI.Xaml.Markup.Compiler.NativeAotUnsupported.Create("SafeReflectionInvoker.InvokeMethod");
        }

        public static bool IsInSystemXaml(Type type)
        {
            throw Microsoft.UI.Xaml.Markup.Compiler.NativeAotUnsupported.Create("SafeReflectionInvoker.IsInSystemXaml");
        }

        internal static bool IsSystemXamlNonPublic(MethodInfo method)
        {
            throw Microsoft.UI.Xaml.Markup.Compiler.NativeAotUnsupported.Create("SafeReflectionInvoker.IsSystemXamlNonPublic");
        }
    }
}
