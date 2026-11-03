// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

//
//  Microsoft Windows Client Platform
//
//
//  Description: Specifies that the whitespace surrounding an element should be trimmed.
//

using System;
using System.Reflection;
using System.ComponentModel;
using System.Globalization;
using System.Diagnostics;
#if SYSTEM_XAML
using System.Xaml.Replacements;
#endif

#if PBTCOMPILER
namespace MS.Internal.Markup
#elif SYSTEM_XAML
namespace System.Xaml
#else
namespace System.Windows.Markup
#endif
{

    /// <summary>
    /// Class that provides fucntionality to obtain a TypeConverter from a property or the
    /// type of the property, based on logic similar to TypeDescriptor.GetConverter.
    /// </summary>
    internal static class TypeConverterHelper
    {
        private static CultureInfo invariantEnglishUS = CultureInfo.InvariantCulture;

        internal static CultureInfo InvariantEnglishUS
        {
            get
            {
                return invariantEnglishUS;
            }
        }

#if !SYSTEM_XAML
        internal static MemberInfo GetMemberInfoForPropertyConverter(object dpOrPiOrMi)
        {
            MemberInfo memberInfo = dpOrPiOrMi as PropertyInfo;

            if (memberInfo == null)
            {
                MethodInfo methodInfo;
#if !PBTCOMPILER
                DependencyProperty dp = dpOrPiOrMi as DependencyProperty;

                if (dp != null)
                {
                    // While parsing styles or templates, we end up getting a DependencyProperty,
                    // even for non-attached cases. In this case, we try fetching the CLR
                    // property info and getting its attributes.
                    memberInfo = dp.OwnerType.GetProperty(
                                 dp.Name,
                                 BindingFlags.Instance | BindingFlags.Public |
                                 BindingFlags.NonPublic | BindingFlags.FlattenHierarchy);

                    // We failed to get a CLR wrapper for the DependencyProperty, so we
                    // assume that this is an attached property and look for the MethodInfo
                    // for the static getter.
                    if (memberInfo == null)
                    {
                        // Get the method member that defines the DependencyProperty
                        memberInfo = dp.OwnerType.GetMethod(
                                     "Get" + dp.Name,
                                     BindingFlags.Public | BindingFlags.NonPublic |
                                     BindingFlags.Static | BindingFlags.FlattenHierarchy);
                    }
                }
                else
#endif
                if ((methodInfo = dpOrPiOrMi as MethodInfo) != null)
                {
                    // miSetter may not be a MethodInfo when we are dealing with event handlers that
                    // belong to local assemblies. One such case is encountered when building DrtCompiler.
                    if (methodInfo.GetParameters().Length == 1)
                    {
                        // Use Getter of the attached property
                        memberInfo = methodInfo;
                    }
                    else
                    {
                        // Use the Setter of the attached property (if any)
                        memberInfo = methodInfo.DeclaringType.GetMethod(
                             "Get" + methodInfo.Name.Substring("Set".Length),
                             BindingFlags.Public | BindingFlags.NonPublic |
                             BindingFlags.Static | BindingFlags.FlattenHierarchy);
                    }
                }
            }

            return memberInfo;
        }

        internal static Type GetConverterType(MemberInfo memberInfo)
        {
            Debug.Assert(null != memberInfo, "Null passed for memberInfo to GetConverterType");

            Type converterType = null;

            // Try looking for the TypeConverter for the type using reflection.
            string converterName = ReflectionHelper.GetTypeConverterAttributeData(memberInfo, out converterType);

            if (converterType == null)
            {
                converterType = GetConverterTypeFromName(converterName);
            }

            return converterType;
        }
#endif
    }
}
