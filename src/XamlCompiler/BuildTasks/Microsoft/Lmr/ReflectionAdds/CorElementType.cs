// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Globalization;
using System.Runtime.InteropServices;

namespace System.Reflection.Adds
{
    using System.Reflection;

    /// <summary>
    /// Represents a raw element type.  Values in this enum are chosen to be castable to the CorElementType enumeration
    /// used by the CLR api's, as well as the internal PrimitiveType enumeration.
    /// </summary>
    internal enum CorElementType : int
    {
        // Fields
        Array = 20,
        Bool = 2,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Byref")]
        Byref = 0x10,
        Char = 3,
        Class = 0x12,
        CModOpt = 0x20,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Reqd")]
        CModReqd = 0x1f,
        End = 0,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", MessageId = "Fn")]
        FnPtr = 0x1b,
        IntPtr = 0x18,
        SByte = 4,
        Short = 6,
        Int = 8,
        Long = 10,
        Internal = 0x21,
        Max = 0x22,
        Modifier = 0x40,
        Object = 0x1c,
        Pinned = 0x45,
        Pointer = 15,
        Float = 12,
        Double = 13,
        Sentinel = 0x41,
        String = 14,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Sz")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", MessageId = "Sz")]
        SzArray = 0x1d,
        TypedByRef = 0x16,
        UIntPtr = 0x19,
        Byte = 5,
        UShort = 7,
        UInt = 9,
        ULong = 11,
        ValueType = 0x11,
        Void = 1,
        TypeVar = 0x13,
        MethodVar = 0x1e,
        GenericInstantiation = 0x15,
        Type = 0x50,
        Enum = 0x55, // used in custom attributes
    }

    /// <summary>
    /// Provides a utility mapping to get string names from built in types.  
    /// </summary>
    internal static class ElementTypeUtility
    {
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity")]
        public static string GetNameForPrimitive(CorElementType value)
        {
            switch (value)
            {
                case CorElementType.Object:
                    return "System.Object";
                case CorElementType.Void:
                    return "System.Void";
                case CorElementType.Int:
                    return "System.Int32";
                case CorElementType.UInt:
                    return "System.UInt32";
                case CorElementType.UShort:
                    return "System.UInt16";
                case CorElementType.ULong:
                    return "System.UInt64";
                case CorElementType.Char:
                    return "System.Char";
                case CorElementType.Byte:
                    return "System.Byte";
                case CorElementType.SByte:
                    return "System.SByte";
                case CorElementType.Short:
                    return "System.Int16";
                case CorElementType.Long:
                    return "System.Int64";
                case CorElementType.Float:
                    return "System.Single";
                case CorElementType.Double:
                    return "System.Double";
                case CorElementType.Bool:
                    return "System.Boolean";
                case CorElementType.IntPtr:
                    return "System.IntPtr";
                case CorElementType.UIntPtr:
                    return "System.UIntPtr";
                case CorElementType.String:
                    return "System.String";
                case CorElementType.Type:
                    return "System.Type";
                default:
                    throw new ArgumentException(string.Format(
                        CultureInfo.InvariantCulture, Resources.IllegalElementType, value));
            }
        }
    } // end class

}
