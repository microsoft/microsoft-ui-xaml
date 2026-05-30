// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;
    using System.ComponentModel;
    using System.Globalization;

    internal class LineNumberInfoTypeConverter : TypeConverter
    {
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                return true;
            }

            return base.CanConvertTo(context, destinationType);
        }

        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            LineNumberInfo lineNumberInfo = (LineNumberInfo)value;
            return string.Format("{0},{1}, {2},{3}", lineNumberInfo.StartLineNumber, lineNumberInfo.StartLinePosition, lineNumberInfo.EndLineNumber, lineNumberInfo.EndLinePosition);
        }

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            // You need to say "Yes I can convert from string" in order to enable converting TO string.
            if (sourceType == typeof(string))
            {
                return true;
            }

            return base.CanConvertFrom(context, sourceType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            throw new NotImplementedException("LineNumberInfo");
        }
    }
}