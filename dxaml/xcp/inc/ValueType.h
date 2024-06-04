// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum ValueType : XUINT32
{
    valueAny                = 0,    // Allows GetValue to specify return type.
    valueNull               = 1,

    valueBool               = 2,    // Boolean value.
    valueEnum               = 3,    // 32 bit integer enumeration.

    valueSigned             = 4,    // 32 bit signed integer value.
    valueUnsigned           = 5,    // 32 bit unsigned integer value.
    valueInt64              = 6,    // 64 bit signed integer value.
    valueUInt64             = 7,    // 64 bit unsigned integer value.

    valueFloat              = 8,    // 32 bit float value.
    valueDouble             = 9,    // 64 bit double-precision float value

    valueSignedArray        = 10,   // Length specified float array.
    valueFloatArray         = 11,   // Length specified float array.
    valueDoubleArray        = 12,   // Length specified double array.
    valuePointArray         = 13,   // Length specified array of Point (pair of float)

    valueString             = 14,   // Length specified UNICODE string.

    valueColor              = 15,   // A8R8G8B8 value.
    valuePoint              = 16,   // Pair of float values.
    valueSize               = 17,   // Size (can be handled like Point for the most part)
    valueRect               = 18,   // Pair of point values.
    valueThickness          = 19,   // left, top, right, bottom
    valueGridLength         = 20,   // GridLength for specifying grid row/column dimensions
    valueCornerRadius       = 21,   // Used to describe the radius of rectangle's corners
    valueDateTime           = 22,   // DateTime
    valueTimeSpan           = 23,   // TimeSpan

    valueObject             = 24,   // Reference counted CDependencyObject.
    valueInternalHandler    = 25,   // Static method pointer to internal code
    valueIUnknown           = 26,   // IUnknown object, that is not CDependencyObject
    valueIInspectable       = 27,   // IInspectable object, that is not CDependencyObject

    valueTypeHandle         = 28,   // KnownTypeIndex
    valueThemeResource      = 29,   // Pointer to theme resource, managed lifetime.

    valuePointer            = 30,   // Raw pointer, no type information, no lifetime management, no reference counting - use at your own risk!

    valueVO                 = 31,   // Value Objects

    valueTextRange          = 32,   // TextRange struct with start index and length

    valueEnum8              = 33,   // Compact enumeration (8-bit)

    valueTypeSentinel       // Always last...
};
