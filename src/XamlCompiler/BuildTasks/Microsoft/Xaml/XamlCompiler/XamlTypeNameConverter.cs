// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Globalization;
using System.Xaml;
using System.Xaml.Schema;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    class XamlTypeNameConverter : TypeConverter
    {
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
            {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            String stringName = value as String;
            if (stringName != null)
            {
                IXamlNamespaceResolver ixnr = context.GetService(typeof(IXamlNamespaceResolver)) as IXamlNamespaceResolver;
                if (ixnr != null)
                {
                    return XamlTypeName.Parse(stringName, ixnr);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(String))
                return true;
            return base.CanConvertTo(context, destinationType);
        }

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            XamlTypeName xamlTypeName = value as XamlTypeName;
            if (xamlTypeName == null || destinationType != typeof(String))
            {
                throw new InvalidOperationException("Bad argument to XamlTypeNameConverter");  // Internal Error
            }

            INamespacePrefixLookup inpl = context.GetService(typeof(INamespacePrefixLookup)) as INamespacePrefixLookup;
            if (inpl != null)
            {
                return xamlTypeName.ToString(inpl);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
