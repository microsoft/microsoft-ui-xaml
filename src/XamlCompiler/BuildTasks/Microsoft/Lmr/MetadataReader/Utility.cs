// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection.Adds;
using System.Text;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Provide utility functionality to LMR consumers.
    /// </summary>
    internal static class Utility
    {        
        /// <summary>
        /// Do a string comparison, respecting case sensitivity flag.
        /// </summary>
        /// <param name="string1">first string</param>
        /// <param name="string2">second string</param>
        /// <param name="ignoreCase">true if strings should be compares case insensitive; else false to compare case sensitive</param>
        /// <returns>true if strings are equal, else false</returns>
        /// <remarks>
        /// Many of the name lookup functions take a boolean ignoreCase flag. This helper provides a convenient signature that matches up 
        /// with the reflection conventions.
        /// </remarks>
        public static bool Compare(string string1, string string2, bool ignoreCase)
        {
            return String.Equals(string1, string2, ignoreCase ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);            
        }

        // Chaining overloads.
        public static bool IsBindingFlagsMatching(MethodBase method, bool isInherited, BindingFlags bindingFlags)
        {
            return IsBindingFlagsMatching(method, method.IsStatic, method.IsPublic, isInherited, bindingFlags);
        }
        public static bool IsBindingFlagsMatching(FieldInfo fieldInfo, bool isInherited, BindingFlags bindingFlags)
        {
            return IsBindingFlagsMatching(fieldInfo, fieldInfo.IsStatic, fieldInfo.IsPublic, isInherited, bindingFlags);
        }

        /// <summary>
        /// Helper to filter our a MemberInfo against BindingFlags.
        /// </summary>
        /// <param name="memberInfo">member to check against binding flags.</param>
        /// <param name="isStatic">true iff the member is static.</param>
        /// <param name="isPublic">true iff the member is public. </param>
        /// <param name="isInherited">true if filtering will allow inherited members. </param>
        /// <param name="bindingFlags">binding flags to match against the member</param>
        /// <returns>true if the members static,public,inherited values match that of the binding flags.</returns>
        /// <remarks>Public and Static can be fetched directly from certain derived members, like FieldInfo. But not other members, like PropertyInfo. </remarks>
        public static bool IsBindingFlagsMatching(MemberInfo memberInfo, bool isStatic, bool isPublic, bool isInherited, BindingFlags bindingFlags)
        {

            if ((bindingFlags & BindingFlags.DeclaredOnly) != 0 && isInherited)
            {
                return false;
            }

            if (isPublic)
            {
                if ((bindingFlags & BindingFlags.Public) == 0)
                    return false;
            }
            else
            {
                if ((bindingFlags & BindingFlags.NonPublic) == 0)
                    return false;
            }

            if (memberInfo.MemberType != MemberTypes.TypeInfo &&
                memberInfo.MemberType != MemberTypes.NestedType)
            {
                if (isStatic)
                {
                    if ((bindingFlags & BindingFlags.FlattenHierarchy) == 0 && isInherited)
                        return false;
                    if ((bindingFlags & BindingFlags.Static) == 0)
                        return false;
                }
                else
                {
                    if ((bindingFlags & BindingFlags.Instance) == 0)
                        return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Extracts namespace portion of type's name. Assumes that fullName
        /// parameter doesn't contain any generic arguments listed.
        /// </summary>
        static internal string GetNamespaceHelper(string fullName)
        {
            Debug.Assert(fullName.IndexOf('[') < 0, "GetNamespaceHelper cannot be called with type name that contains generic arguments.");
            if (fullName.Contains("."))
            {
                // The part before the last '.' is the namespace.
                int i = fullName.LastIndexOf('.');
                Debug.Assert(i > 0);
                return fullName.Substring(0, i);
            }

            return null;
        }

        /// <summary>
        /// Gets the type name from its full name.
        /// </summary>
        /// <param name="fullname">Type's full name. Can't have any generic arguments listed.</param>
        /// <param name="isNested">Is type nested or not.</param>
        internal static string GetTypeNameFromFullNameHelper(string fullName, bool isNested)
        {
            Debug.Assert(fullName.IndexOf('[') < 0, "GetTypeNameFromFullNameHelper cannot be called with type name that contains generic arguments.");
            if (isNested)
            {
                // The full name has the format of EnclosingTypeName+Name.
                int i = fullName.LastIndexOf('+');
                Debug.Assert(i < fullName.Length - 1);
                return fullName.Substring(i + 1);
            }
            else
            {
                // The full name has the format of Namespace.Name.
                int i = fullName.LastIndexOf('.');
                Debug.Assert(i < fullName.Length - 1);
                return fullName.Substring(i + 1);
            }
        }

        /// <summary>
        /// Checks path to a module.
        /// </summary>
        /// <remarks>
        /// We use this instead of File.Exists for these reasons:
        /// 
        /// 1)	File.Exists throws first-chance exceptions if the path isn’t valid. 
        ///     It catches them internally, which makes debugging more difficult.
        /// 2)	File.Exist() is not correct because, as long as the path is valid, we still want to 
        ///     call the Uri constructor, even if the path doesn't exist on the local machine. 
        /// 3)	We want to disallow cases where a filename is specified without a full path. 
        ///     File.Exist() will succeed here, but the Uri constructor down the line will throw.
        /// </remarks>
        internal static bool IsValidPath(string modulePath)
        {
            if (String.IsNullOrEmpty(modulePath))
            {
                return false;
            }

            foreach (char c in Path.GetInvalidPathChars())
            {
                foreach (char c1 in modulePath)
                {
                    if (c == c1)
                    {
                        return false;
                    }
                }
            }

            try
            {
                if (!Path.IsPathRooted(modulePath))
                {
                    return false;
                }
            }
            catch (Exception e)
            {
                Debug.Assert(false, "Unexpected exception thrown from Path.IsPathRooted(): " + e.Message);
                throw;
            }

            return true;
        }
    }
}

