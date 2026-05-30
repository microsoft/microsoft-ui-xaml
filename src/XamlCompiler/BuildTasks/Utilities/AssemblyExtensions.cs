// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Linq;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    internal static class AssemblyExtensions
    {
        public static bool IsClsCompliant(this Assembly instance)
        {
            foreach (var a in instance.GetCustomAttributesData().Where(a =>
                a.AttributeType.FullName == typeof(System.CLSCompliantAttribute).FullName && a.ConstructorArguments.Any()))
            {
                var arg = a.ConstructorArguments[0];
                if (arg.ArgumentType.FullName == typeof(bool).FullName)
                {
                    return (bool) arg.Value;
                }
            }
            return false;
        }
    }
}
