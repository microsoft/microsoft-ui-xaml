// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Globalization;
using System.Reflection;
using System.Collections.Immutable;
using System.Diagnostics;
using System.IO;
using System.Reflection.Metadata;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Parses type names (e.g. "Namespace.Type`1+Nested[[Arg, Assembly]][], Assembly") and resolves them with callbacks,
    /// the same way as <see cref="Type.GetType(string, Func{AssemblyName, Assembly}, Func{Assembly, string, bool, Type}, bool)"/>.
    /// </summary>
    /// <remarks>
    /// The LMR only uses <see cref="Type.GetType(string, Func{AssemblyName, Assembly}, Func{Assembly, string, bool, Type}, bool)"/>
    /// as a type name parser: the callbacks resolve the names against LMR assemblies and modules, and the constructed types
    /// (generic instantiations, arrays, pointers, references) are LMR types too, so it never loads any runtime type. That API
    /// is annotated for trimming anyway (the analyzer can't know what the callbacks do), so .NET 9+ uses the type name parser
    /// of System.Reflection.Metadata instead, and does the resolution like .NET 10 does it for
    /// Type.GetType(typeName, assemblyResolver, typeResolver, throwOnError), see TypeNameResolver in dotnet/runtime.
    /// .NET Framework keeps calling Type.GetType, since TypeName doesn't exist in the System.Reflection.Metadata it uses.
    /// </remarks>
    internal static class LmrTypeNameResolver
    {
        public static Type GetType(
            string typeName,
            Func<AssemblyName, Assembly> assemblyResolver,
            Func<Assembly, string, bool, Type> typeResolver,
            bool throwOnError)
        {
            if (typeName == null)
            {
                throw new ArgumentNullException(nameof(typeName));
            }

            // Compat with Type.GetType: an empty name throws TypeLoadException instead of ArgumentException
            if (typeName.Length == 0)
            {
                if (throwOnError)
                {
                    throw new TypeLoadException("A null or zero length string does not represent a valid Type.");
                }
                return null;
            }

            // Type.GetType doesn't limit the complexity of the type names
            TypeNameParseOptions options = new TypeNameParseOptions { MaxNodes = int.MaxValue };

            TypeName parsed;
            if (throwOnError)
            {
                parsed = TypeName.Parse(typeName.AsSpan(), options);
            }
            else if (!TypeName.TryParse(typeName.AsSpan(), out parsed, options))
            {
                return null;
            }

            return new Resolver(assemblyResolver, typeResolver, throwOnError).Resolve(parsed);
        }

        private readonly struct Resolver
        {
            private readonly Func<AssemblyName, Assembly> _assemblyResolver;
            private readonly Func<Assembly, string, bool, Type> _typeResolver;
            private readonly bool _throwOnError;

            public Resolver(Func<AssemblyName, Assembly> assemblyResolver, Func<Assembly, string, bool, Type> typeResolver, bool throwOnError)
            {
                _assemblyResolver = assemblyResolver;
                _typeResolver = typeResolver;
                _throwOnError = throwOnError;
            }

            public Type Resolve(TypeName typeName)
            {
                if (typeName.IsSimple)
                {
                    return GetSimpleType(typeName);
                }
                else if (typeName.IsConstructedGenericType)
                {
                    return GetGenericType(typeName);
                }
                else if (typeName.IsArray || typeName.IsPointer || typeName.IsByRef)
                {
                    Type type = Resolve(typeName.GetElementType());
                    if (type == null)
                    {
                        return null;
                    }

                    if (typeName.IsArray)
                    {
                        return typeName.IsSZArray ? type.MakeArrayTypeLmr() : type.MakeArrayTypeLmr(typeName.GetArrayRank());
                    }
                    if (typeName.IsByRef)
                    {
                        return type.MakeByRefType();
                    }
                    else if (typeName.IsPointer)
                    {
                        return type.MakePointerType();
                    }
                }

                Debug.Fail("Expected to be unreachable");
                return null;
            }

            private Type GetSimpleType(TypeName typeName)
            {
                if (typeName.IsNested)
                {
                    TypeName current = typeName;
                    int nestingDepth = 0;
                    do
                    {
                        nestingDepth++;
                        current = current.DeclaringType;
                    }
                    while (current.IsNested);

                    string[] nestedTypeNames = new string[nestingDepth];
                    current = typeName;
                    while (current.IsNested)
                    {
                        nestedTypeNames[--nestingDepth] = TypeName.Unescape(current.Name);
                        current = current.DeclaringType;
                    }

                    return GetType(current.FullName, nestedTypeNames, typeName);
                }
                else
                {
                    return GetType(typeName.FullName, Array.Empty<string>(), typeName);
                }
            }

            private Type GetGenericType(TypeName typeName)
            {
                Type type = Resolve(typeName.GetGenericTypeDefinition());
                if (type == null)
                {
                    return null;
                }

                ImmutableArray<TypeName> genericArgs = typeName.GetGenericArguments();
                Type[] genericTypes = new Type[genericArgs.Length];
                for (int i = 0; i < genericArgs.Length; i++)
                {
                    Type genericArg = Resolve(genericArgs[i]);
                    if (genericArg == null)
                    {
                        return null;
                    }
                    genericTypes[i] = genericArg;
                }

                return type.MakeGenericTypeLmr(genericTypes);
            }

            private Type GetType(string escapedTypeName, string[] nestedTypeNames, TypeName parsedName)
            {
                Assembly assembly = null;
                if (parsedName.AssemblyName != null)
                {
                    AssemblyName assemblyName = parsedName.AssemblyName.ToAssemblyName();
                    assembly = _assemblyResolver(assemblyName);
                    if (assembly == null)
                    {
                        if (_throwOnError)
                        {
                            throw new FileNotFoundException(string.Format(CultureInfo.CurrentCulture, "Could not resolve assembly '{0}'.", assemblyName));
                        }
                        return null;
                    }
                }

                // Type.GetType doesn't support ignoreCase with these callbacks
                Type type = _typeResolver(assembly, escapedTypeName, false);
                if (type == null)
                {
                    if (_throwOnError)
                    {
                        throw new TypeLoadException(assembly == null ?
                            string.Format(CultureInfo.CurrentCulture, "Could not resolve type '{0}'.", escapedTypeName) :
                            string.Format(CultureInfo.CurrentCulture, "Could not resolve type '{0}' in assembly '{1}'.", escapedTypeName, assembly.FullName));
                    }
                    return null;
                }

                for (int i = 0; i < nestedTypeNames.Length; i++)
                {
                    type = type.GetNestedTypeLmr(nestedTypeNames[i], BindingFlags.NonPublic | BindingFlags.Public);
                    if (type == null)
                    {
                        if (_throwOnError)
                        {
                            throw new TypeLoadException(string.Format(CultureInfo.CurrentCulture, "Could not resolve nested type '{0}' in type \"{1}'.",
                                nestedTypeNames[i], (i > 0) ? nestedTypeNames[i - 1] : TypeName.Unescape(escapedTypeName)));
                        }
                        return null;
                    }
                }

                return type;
            }
        }
    }
}
