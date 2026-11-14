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
using System.Xaml.Replacements;

namespace System.Xaml
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

    }
}
