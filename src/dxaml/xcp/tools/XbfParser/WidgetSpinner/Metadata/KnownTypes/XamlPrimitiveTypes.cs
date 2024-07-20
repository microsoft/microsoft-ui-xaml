// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.Xaml.WidgetSpinner.Metadata.KnownTypes
{
    public static class PrimitiveTypeNames
    {
        public const string Enum = "enum";
        public const string Signed = "signed";
        public const string Bool = "bool";
        public const string Float = "float";
        public const string Thickness = "Windows.UI.Xaml.Thickness";
        public const string GridLength = "Windows.UI.Xaml.GridLength";
        public const string Color = "Windows.UI.Color";
        public const string String = "string";
    }

    public struct Thickness
    {
        public Thickness(float uniform)
            : this(uniform, uniform, uniform, uniform)
        {

        }

        public Thickness(float leftRight, float topBottom)
            : this(leftRight, topBottom, leftRight, topBottom)
        {

        }

        public Thickness(float left, float top, float right, float bottom)
        {
            Left = left;
            Top = top;
            Right = right;
            Bottom = bottom;
        }

        public float Left { get; }
        public float Top { get; }
        public float Right { get; }
        public float Bottom { get; }
    }

    public enum GridUnitType
    {
        Auto = 0,
        Pixel = 1,
        Star = 2
    }

    public struct GridLength
    {
        public GridLength(GridUnitType gridUnitType, double value)
        {
            GridUnitType = gridUnitType;
            Value = value;
        }

        public GridUnitType GridUnitType { get; }
        public double Value { get; }
    }

    public struct Color
    {
        public Color(byte a, byte r, byte g, byte b)
        {
            A = a;
            R = r;
            G = g;
            B = b;
        }

        internal Color(uint argb)
            : this((byte)((argb & (0xff << 24)) >> 24),
                   (byte)((argb & (0xff << 16)) >> 16),
                   (byte)((argb & (0xff << 8)) >> 8),
                   (byte)((argb & (0xff << 0)) >> 0))
        {
        }

        public byte A { get; }
        public byte R { get; }
        public byte G { get; }
        public byte B { get; }
    }

    public struct EnumValue
    {
        public string EnumName { get; }
        public long Value { get; }

        public EnumValue(string enumName, long value) : this()
        {
            EnumName = enumName;
            Value = value;
        }
    }
}
