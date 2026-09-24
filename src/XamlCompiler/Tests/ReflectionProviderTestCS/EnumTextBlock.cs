// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace ReflectionProviderTestCS
{
    public enum SByte: sbyte
    {
        NegativeOne=-1,
        Zero,
        One
    };

    public enum Byte: byte
    {
        Zero,
        One
    };

    public enum Short : short
    {
        Zero,
        One
    };

    public enum UShort : ushort
    {
        Zero,
        One
    };

    public enum Int : int
    {
        Zero,
        One
    };

    public enum UInt : uint
    {
        Zero,
        One
    };

    public enum Long : long
    {
        NegativeOne=-1,
        Zero,
        One
    };

    public enum ULong : ulong
    {
        Zero=0,
        One,
        Two,
        Three
    };

    public sealed class EnumTextBox : TextBox
    {
        public SByte SByteP { get; set; }
        public Byte ByteP { get; set; }
        public Short ShortP { get; set; }
        public UShort UShortP { get; set; }
        public Int IntP { get; set; }
        public UInt UIntP { get; set; }
        public Long LongP { get; set; }
        public ULong ULongP { get; set; }
    }
}
