// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    // A placeholder to signify that there is a native type converter
    // Note: Not runtime usable.
    internal class DirectUINativeTypeConverter : TypeConverter
    {
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            return (sourceType == typeof(string));
        }
    }
}
