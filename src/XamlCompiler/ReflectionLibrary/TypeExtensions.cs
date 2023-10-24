// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup
{
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    internal static class TypeExtensions
    {
        private static Dictionary<string, string> _projectToCompilerTypeName = new Dictionary<string, string>();
        private static Dictionary<string, string> _projectFromCompilerTypeName = new Dictionary<string, string>();

        // List of projected types we need to convert types for.  E.g. we may get a request for "String" and the
        // IXamlType's FullName for the corresponding type should be "String", but we need to search for
        // "System.String" instead
        private static string[] _projectionNames = new string[]
            {
                "Boolean",
                "Byte",
                "Char",
                "Char16",
                "DateTime",
                "Double",
                "Guid",
                "Int8",
                "Int16",
                "Int32",
                "Int64",
                "Object",
                "SByte",
                "Single",
                "String",
                "TimeSpan",
                "UInt8",
                "UInt16",
                "UInt32",
                "UInt64",
            };

		// Ensures our data structures like projection maps are initialized
        internal static void EnsureInitialized()
        {
            // Lock is somewhat arbitrary - all other methods using the collections we initialize here
            // only read from them, so we only lock on one collection go guard against multiple threads
            // running EnsureInitialized at the same time
            lock (_projectToCompilerTypeName)
            {
                if (_projectToCompilerTypeName.Count == 0)
                {
                    foreach (string winrtTypeName in _projectionNames)
                    {
                        string systemName = $"System.{winrtTypeName}";
                        _projectToCompilerTypeName.Add(systemName, winrtTypeName);
                        _projectFromCompilerTypeName.Add(winrtTypeName, systemName);
                    }
                }
            }
        }

        // Given a type name like 'System.String', returns the standard type name like 'String'.
        // Returns the original type name if no changes are needed.
        // Only valid for non-generic type names.
        public static string GetStandardTypeName(string typeName)
        {
            EnsureInitialized();
            if (_projectToCompilerTypeName.ContainsKey(typeName))
            {
                return _projectToCompilerTypeName[typeName];
            }

            return typeName;
        }

        // Given a type name like 'String', returns the standard type name like 'System.String'.
        // Returns the original type name if no changes are needed.
        // Only valid for non-generic type names.
        public static string GetCSharpTypeName(string typeName)
        {
            EnsureInitialized();
            if (_projectFromCompilerTypeName.ContainsKey(typeName))
            {
                return _projectFromCompilerTypeName[typeName];
            }

            return typeName;
        }
        
        public static PropertyInfo GetStaticProperty(this Type type, string propertyName)
        {
            foreach (var pi in type.GetTypeInfo().DeclaredProperties)
            {
                if (pi.Name.Equals(propertyName))
                {
                    return pi;
                }
            }

            return null;
        }

        public static FieldInfo GetStaticField(this Type type, string fieldName)
        {
            foreach (var fi in type.GetTypeInfo().DeclaredFields)
            {
                if (fi.Name.Equals(fieldName))
                {
                    return fi;
                }
            }

            return null;
        }

        // Assumes the string passed in is from type.ToString(), or a compiler type name from non-reflection type info.
        // Used to convert a CLR type name to the compiler's type naming convention
        // The main distinction between a CLR type name and compiler type name is brackets are replaced by angled brackets
        public static string MakeCompilerTypeName(string typeToString)
        {
            string openBracketsReplaced = typeToString.Replace('[', '<');
            string closedBracketsReplaced = openBracketsReplaced.Replace(']', '>');
            return closedBracketsReplaced;
        }

        // Based off the GetFullGenericNestedName from the Xaml compiler.
		// Gets the "compiler type-name" with angled brackets instead of brackets,
		// and projects type names (including those in a generic type) if necessary.
        public static string GetFullGenericNestedName(Type type)
        {
            string typeName = MakeCompilerTypeName(type.ToString());
            string simpleTypeName = GetStandardTypeName(typeName);

            if (!type.GetTypeInfo().IsGenericType)
            {
                return simpleTypeName;
            }

            string OpenTypeParameters = "<";
            string CloseTypeParameters = ">";

            Type[] typeArguments = type.GetTypeInfo().GenericTypeArguments;

            Type generTypeDef = type.GetTypeInfo().GetGenericTypeDefinition();

            string genericTypeName = MakeCompilerTypeName(generTypeDef.ToString());
            // Strip off the generic arguments by looking for a '<' and only taking the type name up to there
            genericTypeName = genericTypeName.Substring(0, genericTypeName.IndexOf('<'));
            string result = genericTypeName;
            result += OpenTypeParameters;
            for (int i = 0; i < typeArguments.Length; i++)
            {
                result += GetFullGenericNestedName(typeArguments[i]);

                if (i < typeArguments.Length - 1)
                {
                    result += ", ";
                }
            }

            result += CloseTypeParameters;
            return result;
        }
    }
}