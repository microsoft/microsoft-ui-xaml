// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;

// The type converters that the XAML schema associates with built-in types and with the XAML language types
// (BuiltInValueConverter and the [TypeConverter] attributes of the markup extensions). The XAML compiler never converts
// values with them: it only needs to know that a type has a converter (XamlType.TypeConverter, XamlValueConverter.ConverterType),
// so they have no conversion logic. Their names and visibilities are the ones of System.Xaml, which XamlValueConverter
// exposes through Name and IsPublic.

namespace System.Xaml
{
    internal class EventConverter : TypeConverter
    {
    }
}

namespace System.Xaml.Replacements
{
    internal class TypeTypeConverter : TypeConverter
    {
    }

    internal class TypeListConverter : TypeConverter
    {
    }

    internal class DateTimeConverter2 : TypeConverter
    {
    }
}

namespace System.Xaml.Schema
{
    public class XamlTypeTypeConverter : TypeConverter
    {
    }
}

namespace System.Windows.Markup
{
    internal class DateTimeOffsetConverter2 : TypeConverter
    {
    }

    internal class StaticExtensionConverter : TypeConverter
    {
    }

    internal class TypeExtensionConverter : TypeConverter
    {
    }
}