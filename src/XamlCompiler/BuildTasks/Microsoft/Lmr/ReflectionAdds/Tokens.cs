// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-----------------------------------------------------------------------------
// General wrappers for metadata tokens.

using System;

namespace System.Reflection.Adds
{
    /// <summary>
    /// Types for metadata tokens. These are from the unmanaged metadata interfaces in Cor.h
    /// </summary>
    internal enum TokenType
    {
        Module = 0x00000000,
        TypeRef = 0x01000000,
        TypeDef = 0x02000000,
        FieldDef = 0x04000000,
        MethodDef = 0x06000000,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Param")]
        ParamDef = 0x08000000,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Impl")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711:IdentifiersShouldNotHaveIncorrectSuffix")]
        InterfaceImpl = 0x09000000,
        MemberRef = 0x0a000000,
        CustomAttribute = 0x0c000000,
        Permission = 0x0e000000,
        Signature = 0x11000000,
        Event = 0x14000000,
        Property = 0x17000000,
        ModuleRef = 0x1a000000,
        TypeSpec = 0x1b000000,
        Assembly = 0x20000000,
        AssemblyRef = 0x23000000,
        File = 0x26000000,
        ExportedType = 0x27000000,
        ManifestResource = 0x28000000,
        GenericPar = 0x2a000000,
        MethodSpec = 0x2b000000,
        String = 0x70000000,
        Name = 0x71000000,
        BaseType = 0x72000000,
        Invalid = 0x7FFFFFFF,
    }

    /// <summary>
    /// Metadata table indexes as defined by the CLI standard.
    /// </summary>
    internal enum MetadataTable
    {
        Module = 0x00,
        TypeRef = 0x01,
        TypeDef = 0x02,
        FieldDef = 0x04,
        MethodDef = 0x06,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Param")]
        ParamDef = 0x08,
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Impl")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711:IdentifiersShouldNotHaveIncorrectSuffix")]
        InterfaceImpl = 0x09,
        MemberRef = 0x0a,
        CustomAttribute = 0x0c,
        Permission = 0x0e,
        Signature = 0x11,
        Event = 0x14,
        Property = 0x17,
        ModuleRef = 0x1a,
        TypeSpec = 0x1b,
        Assembly = 0x20,
        AssemblyRef = 0x23,
        File = 0x26,
        ExportedType = 0x27,
        ManifestResource = 0x28,
        GenericPar = 0x2a,
        MethodSpec = 0x2b,
    }

} // namespace