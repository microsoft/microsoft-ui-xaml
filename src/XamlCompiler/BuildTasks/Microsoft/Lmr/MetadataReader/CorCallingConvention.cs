// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Calling convention flags from ECMA-335 metadata signatures.

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    internal enum CorCallingConvention
    {
        Default = 0x0,

        VarArg = 0x5,
        Field = 0x6,
        LocalSig = 0x7,
        Property = 0x8,
        Unmanaged = 0x9,
        GenericInst = 0xa,
        NativeVarArg = 0xb,

        // The high bits of the calling convention convey additional info
        Mask = 0x0f,
        HasThis = 0x20,
        ExplicitThis = 0x40,
        Generic = 0x10,
    };
}
