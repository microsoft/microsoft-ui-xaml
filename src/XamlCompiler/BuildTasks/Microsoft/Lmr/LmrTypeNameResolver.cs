// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Parses type names (e.g. "Namespace.Type`1+Nested[[Arg, Assembly]][], Assembly") and resolves them with callbacks,
    /// the same way as <see cref="Type.GetType(string, Func{AssemblyName, Assembly}, Func{Assembly, string, bool, Type}, bool)"/>.
    /// </summary>
    /// <remarks>
    /// The LMR only uses <see cref="Type.GetType(string, Func{AssemblyName, Assembly}, Func{Assembly, string, bool, Type}, bool)"/>
    /// as a type name parser: the callbacks resolve the names against LMR assemblies and modules, and the constructed types
    /// (generic instantiations, arrays, pointers, references) are LMR types too, so it never loads any runtime type.
    /// Every use of it goes through here, so that there is one place to change how type names are parsed.
    /// </remarks>
    internal static class LmrTypeNameResolver
    {
        public static Type GetType(
            string typeName,
            Func<AssemblyName, Assembly> assemblyResolver,
            Func<Assembly, string, bool, Type> typeResolver,
            bool throwOnError)
        {
            return Type.GetType(typeName, assemblyResolver, typeResolver, throwOnError);
        }
    }
}
