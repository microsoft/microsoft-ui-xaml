// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Type Name parser.
// This is like a Type.GetType(string) that takes in an ITypeUniverse to work across Universes.
// Delegates to Type.GetType(string, assemblyResolver, typeResolver) for actual parsing;
// the value here is the custom resolution callbacks that search across the ITypeUniverse.
//
using System;
using System.Diagnostics;
using System.Globalization;

namespace System.Reflection.Adds
{
    // Usings need to be declared within the namespace so that they take precedence over binding based off
    // the containing namespace name. 
    using System.Reflection;  

    internal static class TypeNameParser
    {

        /// <summary>
        /// Parses type name and returns type instance represented by type name.  
        /// </summary>
        /// <param name="universe">If non-null, the univese that the returned type is valid in. This 
        /// can be used to search for names.</param>
        /// <param name="module">If non-null, this is the module that input is valid in. 
        /// If null, then universe must be non-null and the type is search throughout the entire universe.</param>
        /// <param name="input">String containing type name.</param>
        /// <returns>Type that corresponds to given type name.</returns>
        /// <exception cref="TypeLoadException">Thrown when type can't be found in a given module/universe.</exception>
        /// <exception cref="ArgumentException">Thrown when input contains more characters than expected.</exception>
        public static Type ParseTypeName(ITypeUniverse universe, Module module, string input)
        {
            bool throwOnError = true;
            return ParseTypeName(universe, module, input, throwOnError);
        }


        /// <summary>
        /// Parses type name and returns type instance represented by type name. Uses CLR's type name
        /// parser.
        /// </summary>
        /// <param name="universe">If non-null, the univese that the returned type is valid in. This 
        /// can be used to search for names.</param>
        /// <param name="module">If non-null, this is the module that input is valid in. 
        /// If null, then universe must be non-null and the type is search throughout the entire universe.</param>
        /// <param name="input">String containing type name.</param>
        /// <param name="throwOnError">Controls behavior for cases when type with a given name cannot be found.</param>
        /// <returns>Type that corresponds to given type name. Can return null if type cannot be found and 
        /// throwOnError flag is false.</returns>
        /// <exception cref="TypeLoadException">Thrown when type can't be found in a given module/universe and
        /// throwOnError flag is true.</exception>
        /// <exception cref="ArgumentException">Thrown when input contains more characters than expected.</exception>
        public static Type ParseTypeName(ITypeUniverse universe, Module module, string input, bool throwOnError)
        {
            return ParseTypeName(universe, module, input, false, false, throwOnError);
        }

        /// <summary>
        /// Parses type name and returns type instance represented by type name. Uses CLR's type name
        /// parser.
        /// </summary>
        /// <param name="universe">If non-null, the univese that the returned type is valid in. This 
        /// can be used to search for names.</param>
        /// <param name="module">If non-null, this is the module that input is valid in. 
        /// If null, then universe must be non-null and the type is search throughout the entire universe.</param>
        /// <param name="input">String containing type name.</param>
        /// <param name="useSystemAssemblyToResolveTypes">True if the system assembly(mscorlib) should be used to 
        /// resolve types when the type cannot be found in module</param>
        /// <param name="useWindowsRuntimeResolution">True if Windows Runtime type resolution should be used</param>
        /// <param name="throwOnError">Controls behavior for cases when type with a given name cannot be found.</param>
        /// <returns>Type that corresponds to given type name. Can return null if type cannot be found and 
        /// throwOnError flag is false.</returns>
        /// <exception cref="TypeLoadException">Thrown when type can't be found in a given module/universe and
        /// throwOnError flag is true.</exception>
        /// <exception cref="ArgumentException">Thrown when input contains more characters than expected.</exception>
        /// <returns></returns>
        public static Type ParseTypeName(
            ITypeUniverse universe,
            Module module,
            string input,
            bool useSystemAssemblyToResolveTypes,
            bool useWindowsRuntimeResolution,
            bool throwOnError)
        {
            Module systemModule = universe.GetSystemAssembly().ManifestModule;

            Debug.Assert((universe != null) || (module != null));

            Func<AssemblyName, Assembly> assemblyResolver = delegate(AssemblyName assemblyName)
            {
                Debug.Assert(assemblyName != null);
                return DetermineAssembly(assemblyName, module, universe, throwOnError);
            };

            Func<Assembly, string, bool, Type> typeResolver = delegate(Assembly assembly, string simpleTypeName, bool ignoreCase)
            {
                Debug.Assert(!TypeNameParser.IsCompoundType(simpleTypeName));

                bool throwOnErrorInCallback = false;
                Type result = null;

                if(assembly != null)
                {
                    // Try to get the type from the assembly returned from the assembly resolver
                    result = assembly.GetType(simpleTypeName, throwOnErrorInCallback, ignoreCase);
                }
                else
                {
                    // If type name doesn't contain assembly name
                    if(null == result) 
                    {
                        // Try to get the type from the module containing the typename
                        Debug.Assert(module != null);
                        result = module.GetType(simpleTypeName, throwOnErrorInCallback, ignoreCase);
                    }

                    if(null == result && useSystemAssemblyToResolveTypes) 
                    {
                        // Try to get the type from the system module(mscorlib)
                        Debug.Assert(systemModule != null);
                        result = systemModule.GetType(simpleTypeName, throwOnErrorInCallback, ignoreCase);
                    }

                    if(null == result && useWindowsRuntimeResolution) 
                    {
                        // Try to get the type using Windows Runtime resolution. 
                        result = universe.ResolveWindowsRuntimeType(simpleTypeName, throwOnErrorInCallback, ignoreCase);
                    }
                }

                return result;
            };

            return Type.GetType(input, assemblyResolver, typeResolver, throwOnError);
        }

        // Determine which assembly to do a lookup in.
        // a universe + assemblyName are explicitly supplied, use that.
        // Else use the default token resolver supplied. 
        // This will throw if not enough information is given to determine the module.
        // Lokoup in an assembly is responsible for locating the right module within that assembly that contains the type.
        static Assembly DetermineAssembly(AssemblyName assemblyName,
            Module defaultTokenResolver,
            ITypeUniverse universe,
            bool throwOnError = true)
        {
            //Find out which token resolver to use
            Module tr = defaultTokenResolver;
            if (assemblyName != null)
            {
                if (universe == null)
                {
                    throw new ArgumentException(Resources.HostSpecifierMissing);
                }

                Assembly a = universe.ResolveAssembly(assemblyName, throwOnError);

                if (a == null && throwOnError)
                {
                    throw new ArgumentException(string.Format(
                        CultureInfo.InvariantCulture, Resources.UniverseCannotResolveAssembly, assemblyName));
                }
                return a;
            }
            else if (defaultTokenResolver == null)
            {
                throw new ArgumentException(Resources.DefaultTokenResolverRequired);
            }

            Debug.Assert(tr != null);
            return tr.Assembly;
        }

        /// <summary>
        /// Detemines if given type name represents a compound type (e.g. generic, array, nested,
        /// pointer, or reference).
        /// </summary>
        /// <returns>True if the typename requires any parsing and is not just found in the TypeDef name table.</returns>
        public static bool IsCompoundType(string name)
        {
            return name.IndexOfAny(compoundTypeNameCharacters) > 0;
        }

        static readonly char[] compoundTypeNameCharacters = new char[] { '+', ',', '[', '*', '&' };
    }
}
