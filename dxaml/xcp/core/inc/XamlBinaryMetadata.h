// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TypeBits.h"
#include <cstdint>
#include "XamlTypeInfoProviderKind.h"
#include "StableXbfIndexes.g.h"

// The persisted NodeStream is in the format:
// [XamlNode::XamlNodeType][XxxxXamlNodeData][(optional) variably sized data] ... (repeated until the end of file is reached)
class XamlPullParser;

namespace Parser
{

    // Define the size of the XBF Hash that is contained in the header.
    // This seems like a good place to define it as it is common to both
    // GenXbf and the Parser.
    const int c_xbfHashSize = 64;

    struct XamlBinaryFileVersion
    {
        uint32_t m_uMajorBinaryFileVersion;
        uint32_t m_uMinorBinaryFileVersion;

        bool operator ==(_In_ const XamlBinaryFileVersion& other) const
        {
            return ((m_uMajorBinaryFileVersion == other.m_uMajorBinaryFileVersion) &&
                (m_uMinorBinaryFileVersion == other.m_uMinorBinaryFileVersion));
        }

        bool ShouldNullTerminateStrings() const
        {
            return m_uMajorBinaryFileVersion == 2 &&
                m_uMinorBinaryFileVersion >= 1;
        }
    };

    static const XamlBinaryFileVersion XbfV1 = { 1, 0 };
    static const XamlBinaryFileVersion XbfV2 = { 2, 0 };
    static const XamlBinaryFileVersion XbfV2_1 = { 2, 1 };
}

struct XamlBinaryFileHeader
{
    XamlBinaryFileHeader()
        : m_uStringTableOffset(0)
        , m_uAssemblyListOffset(0)
        , m_uTypeNamespaceListOffset(0)
        , m_uTypeListOffset(0)
        , m_uPropertyListOffset(0)
        , m_uXmlNamespaceListOffset(0)
    {
        std::fill_n(m_byteHashBuffer, Parser::c_xbfHashSize, static_cast<BYTE>(0));
    }
    XUINT64 m_uStringTableOffset;
    XUINT64 m_uAssemblyListOffset;
    XUINT64 m_uTypeNamespaceListOffset;
    XUINT64 m_uTypeListOffset;
    XUINT64 m_uPropertyListOffset;
    XUINT64 m_uXmlNamespaceListOffset;
    BYTE   m_byteHashBuffer[Parser::c_xbfHashSize];
};

struct XamlBinaryFileHeader2
{
    XamlBinaryFileHeader2()
    : m_uStringTableOffset(0)
    , m_uAssemblyListOffset(0)
    , m_uTypeNamespaceListOffset(0)
    , m_uTypeListOffset(0)
    , m_uPropertyListOffset(0)
    , m_uXmlNamespaceListOffset(0)
    {
        std::fill_n(m_byteHashBuffer, Parser::c_xbfHashSize, static_cast<BYTE>(0));
    }
    XUINT64 m_uStringTableOffset;
    XUINT64 m_uAssemblyListOffset;
    XUINT64 m_uTypeNamespaceListOffset;
    XUINT64 m_uTypeListOffset;
    XUINT64 m_uPropertyListOffset;
    XUINT64 m_uXmlNamespaceListOffset;
    BYTE   m_byteHashBuffer[Parser::c_xbfHashSize];
};

struct PersistedXamlAssembly
{
    PersistedXamlAssembly()
        : m_eTypeInfoProviderKind(tpkUnknown)
        , m_uiAssemblyName(0)
    {
    }
    XamlTypeInfoProviderKind m_eTypeInfoProviderKind;
    XUINT32 m_uiAssemblyName;
};

struct PersistedXamlTypeNamespace
{
    PersistedXamlTypeNamespace()
        : m_uiAssembly(0)
        , m_uiNamespaceName(0)
    {
    }
    XUINT32 m_uiAssembly;
    XUINT32 m_uiNamespaceName;
};

struct PersistedXamlType
{
    PersistedXamlType()
        : m_TypeFlags(None)
        , m_uiTypeNamespace(0)
        , m_uiTypeName(0)
    {
    }

    enum PersistedXamlTypeFlags
    {
        None,
        IsMarkupDirective   = 0x01,
        IsUnknown           = 0x02,
    };

    XamlBitSet<PersistedXamlTypeFlags> m_TypeFlags;

    // If this is an unknown type, then m_uiTypeNamespace refers to a XamlXmlNamespace
    XUINT32 m_uiTypeNamespace;
    XUINT32 m_uiTypeName;
};

struct PersistedXamlProperty
{
    PersistedXamlProperty()
        : m_PropertyFlags(None)
        , m_uiType(0)
        , m_uiPropertyName(0)
    {
    }

    enum PersistedXamlPropertyFlags
    {
        None,
        IsXmlProperty              = 0x01,
        IsMarkupDirective          = 0x02,
        IsImplicitProperty         = 0x04,
        IsCustomDependencyProperty = 0x08,
        IsUnknown                  = 0x10,
    };

    XamlBitSet<PersistedXamlPropertyFlags> m_PropertyFlags;
    XUINT32 m_uiType;
    XUINT32 m_uiPropertyName;
};

struct PersistedXamlXmlNamespace
{
    PersistedXamlXmlNamespace()
        : m_uiNamespaceUri(0)
    {
    }
    XUINT32 m_uiNamespaceUri;
};

struct PersistedXamlNode
{
    PersistedXamlNode()
        : m_uiObjectId(0)
        , m_NodeFlags(None)
    {
    }

    enum PersistedXamlNodeFlags
    {
        None,
        IsRetrieved            = 0x01,
        IsUnknown              = 0x02,
        IsStringValueAndUnique = 0x04,
        IsTrustedXbfIndex      = 0x08,
    };

    XUINT32 m_uiObjectId;
    XamlBitSet<PersistedXamlNodeFlags> m_NodeFlags;
};

struct PersistedXamlNode2
{
    PersistedXamlNode2()
    : m_uiObjectId(0)
    , m_bIsTrusted(FALSE)
    {
    }

    // If this static_assert is hit, then that means we now require 14 bits to
    // hold a StableXbfTypeIndex. XBFv2 currently only allocates 15 bits to hold
    // a StableXbfTypeIndex, so consider this your halfway-mark warning to create
    // a plan to update the XBFv2 format to avoid issues if we breach the 15-bit
    // limit. Oh, and go ahead and update the static_assert to 0x4000.
    static_assert(Parser::StableXbfTypeCount < 0x2000, "See accompanying wall of text in the source code.");

    UINT16 m_uiObjectId : 15;
    UINT16 m_bIsTrusted : 1;
};

struct PersistedXamlSubStream
{
    PersistedXamlSubStream()
    : m_nodeStreamOffset(0)
    , m_lineStreamOffset(0)
    {
    }

    UINT32 m_nodeStreamOffset;
    UINT32 m_lineStreamOffset;
};

enum class PersistedConstantType : std::uint8_t
{
    None = 0,
    IsBoolFalse,
    IsBoolTrue,
    IsFloat,
    IsSigned,
    IsSharedString,
    IsThickness,
    IsGridLength,
    IsColor,
    IsUniqueString,
    IsNullString,
    IsEnum
};

struct PersistedXamlValueNode
{
    enum PersistedXamlValueNodeType
    {
        None,
        IsBoolFalse,
        IsBoolTrue,
        IsFloat,
        IsSigned,
        IsCString,
        IsKeyTime,
        IsThickness,
        IsLengthConverter,
        IsGridLength,
        IsColor,
        IsDuration
    };
};

