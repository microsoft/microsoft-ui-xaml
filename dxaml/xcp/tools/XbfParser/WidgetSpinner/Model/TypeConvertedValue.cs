// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata;

namespace Microsoft.Xaml.WidgetSpinner.Model
{
    /// <summary>
    /// Represents a value that requires type conversion (i.e. the original
    /// value's type is incompatible with the target type)
    /// </summary>
    public class TypeConvertedValue
    {
        public XamlType TargetType { get; }
        public object SourceValue { get; }

        public TypeConvertedValue(XamlType targetType, object sourceValue)
        {
            TargetType = targetType;
            SourceValue = sourceValue;
        }

        public override string ToString()
        {
            return $"Type-converted value (target type: {TargetType}, original value: {SourceValue}";
        }
    }
}