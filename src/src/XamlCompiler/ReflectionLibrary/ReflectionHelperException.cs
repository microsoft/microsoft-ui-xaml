// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup
{
    internal sealed class ReflectionHelperException : Exception
    {
        private static string ReflectionExceptionMessage = @"Error in reflection helper.  Please add '<PropertyGroup><EnableTypeInfoReflection>false</EnableTypeInfoReflection></PropertyGroup>' to your project file.";

        public ReflectionHelperException(string message)
            :base($"{ReflectionExceptionMessage}.  {message}")
        {
        }
    }
}