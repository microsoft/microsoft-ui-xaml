// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MemoryStreamBuffer.h"
#include "XamlBinaryFormatSubWriter2.h"
#include "XamlBinaryFormatCommon.h"
#include "XamlSerializationHelper.h"
#include "ObjectWriterNodeList.h"
#include "ObjectWriterNode.h"
#include "ObjectWriterNodeType.h"
#include "XbfMetadataApi.h"
#include "XamlPredicateHelpers.h"

using namespace DirectUI;

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::Initialize()
{
    xref_ptr<CMemoryStreamBuffer> spSubNodeBuffer;
    xref_ptr<CMemoryStreamBuffer> spSubLineBuffer;

    // initialize the node stream buffer
    IFC_RETURN(spSubNodeBuffer.init(new CMemoryStreamBuffer((XUINT32)-1)));
    IFC_RETURN(spSubNodeBuffer->CreateStream(m_spSubNodeStream.ReleaseAndGetAddressOf()));

    // initialize the line stream buffer
    IFC_RETURN(spSubLineBuffer.init(new CMemoryStreamBuffer((XUINT32)-1)));
    IFC_RETURN(spSubLineBuffer->CreateStream(m_spSubLineStream.ReleaseAndGetAddressOf()));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistNode(
    _In_ const ObjectWriterNode& node)
{
    // we do not persist the StreamOffsetMarker
    if (node.GetNodeType() == ObjectWriterNodeType::StreamOffsetMarker)
    {
        return S_OK;
    }

    // Persist Line Information
    IFC_RETURN(PersistLineInfo(node));

    // Persist Node Type
    IFC_RETURN(PersistNodeType(node.GetNodeType()));

    switch (node.GetNodeType())
    {
        case ObjectWriterNodeType::AddNamespace:
        case ObjectWriterNodeType::PushScopeAddNamespace:
        {
            IFC_RETURN(PersistNamespace(node.GetNamespace(), node.GetStringValue()));
        }
        break;

        case ObjectWriterNodeType::SetDeferredProperty:
        {
            IFC_RETURN(PersistProperty(node.GetXamlProperty()));
        }
        break;
        
        case ObjectWriterNodeType::AddToCollection:
        case ObjectWriterNodeType::EndInitPopScope:
        case ObjectWriterNodeType::EndInitProvideValuePopScope:
        case ObjectWriterNodeType::PushScope:
        case ObjectWriterNodeType::PopScope:
        case ObjectWriterNodeType::ProvideValue:
        case ObjectWriterNodeType::AddToDictionary:
        case ObjectWriterNodeType::SetCustomRuntimeData:
        {
            // noop
        }
        break;

        case ObjectWriterNodeType::CreateType:
        case ObjectWriterNodeType::CreateTypeBeginInit:
        case ObjectWriterNodeType::PushScopeCreateType:
        case ObjectWriterNodeType::PushScopeCreateTypeBeginInit:
        {
            IFC_RETURN(PersistType(node.GetXamlType()));
        }
        break;

        case ObjectWriterNodeType::SetValue:
        case ObjectWriterNodeType::SetValueFromMarkupExtension:
        case ObjectWriterNodeType::GetValue:
        case ObjectWriterNodeType::PushScopeGetValue:
        {
            IFC_RETURN(PersistProperty(node.GetXamlProperty()));
        }
        break;

        case ObjectWriterNodeType::SetName:
        case ObjectWriterNodeType::SetConnectionId:
        case ObjectWriterNodeType::PushConstant:
        case ObjectWriterNodeType::AddToDictionaryWithKey:
        case ObjectWriterNodeType::GetResourcePropertyBag:
        case ObjectWriterNodeType::ProvideStaticResourceValue:
        case ObjectWriterNodeType::ProvideThemeResourceValue:
        {
            IFC_RETURN(PersistConstant(node.GetValue()->GetValue()));
        }
        break;

        case ObjectWriterNodeType::SetValueConstant:
        case ObjectWriterNodeType::SetValueTypeConvertedConstant:
        case ObjectWriterNodeType::SetValueFromStaticResource:
        case ObjectWriterNodeType::SetValueFromThemeResource:
        {
            IFC_RETURN(PersistProperty(node.GetXamlProperty()));
            IFC_RETURN(PersistConstant(node.GetValue()->GetValue()));
        }
        break;

        case ObjectWriterNodeType::SetValueTypeConvertedResolvedType:
        {
            IFC_RETURN(PersistProperty(node.GetXamlProperty()));
            IFC_RETURN(PersistType(node.GetXamlTypeProxy()));
        }
        break;

        case ObjectWriterNodeType::SetValueFromTemplateBinding:
        case ObjectWriterNodeType::SetValueTypeConvertedResolvedProperty:
        {
            IFC_RETURN(PersistProperty(node.GetXamlProperty()));
            IFC_RETURN(PersistProperty(node.GetXamlPropertyProxy()));
        }
        break;

        case ObjectWriterNodeType::PushScopeCreateTypeWithConstantBeginInit:
        case ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstantBeginInit:
        case ObjectWriterNodeType::CreateTypeWithConstantBeginInit:
        case ObjectWriterNodeType::CreateTypeWithTypeConvertedConstantBeginInit:
        {
            IFC_RETURN(PersistType(node.GetXamlType()));
            IFC_RETURN(PersistConstant(node.GetValue()->GetValue()));
        }
        break;

        case ObjectWriterNodeType::CheckPeerType:
        {
            IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(node.GetStringValue(), GetVersion(), m_spSubNodeStream));
        }
        break;

        case ObjectWriterNodeType::BeginConditionalScope:
        {
            auto xamlPredicateAndArgs = node.GetXamlPredicateAndArgs();
            IFC_RETURN(PersistType(xamlPredicateAndArgs->PredicateType));
            IFC_RETURN(PersistSharedString(xamlPredicateAndArgs->Arguments));
        }
        break;

        case ObjectWriterNodeType::EndConditionalScope:
        {
            // no-op
        }
        break;

        default:
        {
            // Unhandled situation - investigate
            ASSERT(false);
        }
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistNodeType(
    _In_ const ObjectWriterNodeType nodeType)
{
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(nodeType, GetVersion(), m_spSubNodeStream));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistConstantNodeType(
    _In_ const PersistedConstantType nodeType)
{
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(nodeType, GetVersion(), m_spSubNodeStream));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistNamespace(
    _In_ const std::shared_ptr<XamlNamespace>& spNamespace,
    _In_ const xstring_ptr& spPrefix)
{
    XUINT32 namespaceId = 0;
    PersistedXamlNode2 sPersistedXamlNode;
    xstring_ptr finalPrefix = spPrefix;

    if (spPrefix.IsNullOrEmpty())
    {
        finalPrefix = xstring_ptr::EmptyString();
    }

    IFC_RETURN(m_spMetadataStore->GetXamlXmlNamespaceId(spNamespace, namespaceId));
    sPersistedXamlNode.m_uiObjectId = static_cast<UINT16>(namespaceId);
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, GetVersion(), m_spSubNodeStream));
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(finalPrefix, GetVersion(), m_spSubNodeStream));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistType(
    _In_ const std::shared_ptr<XamlType>& spType)
{
    PersistedXamlNode2 sPersistedXamlNode;

    IFC_RETURN(m_spMetadataStore->GetXamlTypeNode(spType, sPersistedXamlNode));
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, GetVersion(), m_spSubNodeStream));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistProperty(
    _In_ const std::shared_ptr<XamlProperty>& spProperty)
{
    PersistedXamlNode2 sPersistedXamlNode;

    IFC_RETURN(m_spMetadataStore->GetXamlPropertyNode(spProperty, sPersistedXamlNode));
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, GetVersion(), m_spSubNodeStream));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistConstant(
    _In_ const unsigned int value)
{
    return Persist7BitEncodedInt(value, m_spSubNodeStream);
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistLineInfo(
    _In_ const ObjectWriterNode& objectNode)
{
    if (m_fGenerateLineInfo)
    {
        UINT64 currentOffset = 0;
        IFC_RETURN(GetNodeStreamOffset(&currentOffset));
        if (currentOffset == (XUINT32)-1)
        {
            currentOffset = 0;
        }
        ASSERT(currentOffset >= m_lastNodeStreamOffset);
        
        int lineOffsetDelta = objectNode.GetLineInfo().LineNumber() - m_lastLineOffset;
        int columnOffsetDelta = objectNode.GetLineInfo().LinePosition() - m_lastColumnOffset;

        // We only write the line info if it has changed since the last node. We also only 
        // update the offset when we actually write a node otherwise our deltas will be off and
        // we report garbage masquerading as line information. For example, if we have the current
        // offsets and line information:

        // 1. Offset: 4, Line: 5, 5
        // 2. Offset: 6, Line: 5, 5
        // 3. Offset: 10, Line: 6, 20

        // We won't write node 2, but we want to say the delta offset for node 3 is 6, not 4. This is ok
        // because if we reach the end of the stream while reading we use the last line info (this will be 
        // the case for 2).

        // Only writing line information when the line has changed is an optimization. If it proves we don't 
        // need the optimization, we can always write line information for every node.

        if (lineOffsetDelta != 0 || columnOffsetDelta != 0)
        {
            unsigned int streamOffsetDelta = static_cast<unsigned int>(currentOffset)-m_lastNodeStreamOffset;
            lineOffsetDelta = (lineOffsetDelta >= 0 ? 2 * lineOffsetDelta : -2 * lineOffsetDelta - 1);
            columnOffsetDelta = (columnOffsetDelta >= 0 ? 2 * columnOffsetDelta : -2 * columnOffsetDelta - 1);
            IFC_RETURN(Persist7BitEncodedInt(streamOffsetDelta, m_spSubLineStream));
            IFC_RETURN(Persist7BitEncodedInt(lineOffsetDelta, m_spSubLineStream));
            IFC_RETURN(Persist7BitEncodedInt(columnOffsetDelta, m_spSubLineStream));
           
            m_lastNodeStreamOffset = static_cast<unsigned int>(currentOffset);
            m_lastLineOffset = objectNode.GetLineInfo().LineNumber();
            m_lastColumnOffset = objectNode.GetLineInfo().LinePosition();
            
            // LOG(L"WriterLineInfo[recorded]: %d %d %d", static_cast<unsigned int>(currentOffset), objectNode.GetLineInfo().LineNumber(), objectNode.GetLineInfo().LinePosition());
        }
        else
        {
           // LOG(L"WriterLineInfo[        ]: %d %d %d", static_cast<unsigned int>(currentOffset), objectNode.GetLineInfo().LineNumber(), objectNode.GetLineInfo().LinePosition());
        }

    }

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::Persist7BitEncodedInt(
    _In_ const unsigned int value,
    _In_ const xref_ptr<IPALStream>& spStream)
{
    static const unsigned int Bit8 = 0x00000080;    // 10000000
    unsigned char currentByte = 0;
    unsigned int valueToEncode = value;

    while (valueToEncode >= Bit8)
    {
        currentByte  = static_cast<unsigned char>(valueToEncode | Bit8);
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(currentByte, GetVersion(), spStream));
        valueToEncode >>= 7;
    }
    currentByte = static_cast<unsigned char>(valueToEncode);
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(currentByte, GetVersion(), spStream));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistConstant(
    _In_ const CValue& value)
{
    switch (value.GetType())
    {
        case valueEnum:
        case valueEnum8:
        {
            // 8-bit enums are stored as 32 bit
            uint32_t enumValue = 0;
            KnownTypeIndex enumTypeIndex = KnownTypeIndex::UnknownType;
            IFC_RETURN(value.GetEnum(enumValue, enumTypeIndex));

            ASSERT(enumTypeIndex != KnownTypeIndex::UnknownType);
            auto stableIndex = Parser::GetStableXbfTypeIndex(enumTypeIndex);
            ASSERT(stableIndex != Parser::StableXbfTypeIndex::UnknownType);
            IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsEnum));
            IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(stableIndex, GetVersion(), m_spSubNodeStream));
            IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(enumValue, GetVersion(), m_spSubNodeStream));
        }
        break;

        case valueSigned:
        {
            IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsSigned));
            IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value.AsSigned(), GetVersion(), m_spSubNodeStream));
        }
        break;

        case valueBool:
        {
            if (value.AsBool() == TRUE)
            {
                IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsBoolTrue));
            }
            else
            {
                IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsBoolFalse));
            }
        }
        break;

        case valueFloat:
        {
            IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsFloat));
            IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value.AsFloat(), GetVersion(), m_spSubNodeStream));
        }
        break;

        case valueThickness:
        {
            XTHICKNESS thickness = *(value.AsThickness());
            IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsThickness));
            IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(thickness, GetVersion(), m_spSubNodeStream));
        }
        break;

        case valueGridLength:
        {
            XGRIDLENGTH gridLength = *(value.AsGridLength());
            IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsGridLength));
            IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(gridLength, GetVersion(), m_spSubNodeStream));
        }
        break;

        case valueColor:
        {
            IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsColor));
            IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value.AsColor(), GetVersion(), m_spSubNodeStream));
        }
        break;

        case valueString:
        {
            IFC_RETURN(PersistStringConstant(value.AsString()));
        }
        break;

        default:
        {
            ASSERT(false);
        }
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistStringConstant(
    _In_ const xstring_ptr strValue)
{
    PersistedXamlNode2 sPersistedXamlNode;
    bool isStringUnique = false;

    sPersistedXamlNode.m_uiObjectId = 0;

    // apply heuristic to figure out reuse
    // don't attempt to reuse strings for value node if they are
    // numeric or large length strings
    if (strValue.GetCount() > 64)
    {
        isStringUnique = true;
    }
    else if (strValue.GetCount() > 2)
    {
        if (xisdigit(strValue.GetChar(0)) ||
            xisdigit(strValue.GetChar(1)) ||
            xisdigit(strValue.GetChar(2)))
        {
            isStringUnique = true;
        }
    }

    if (strValue.IsNull())
    {
        IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsNullString));
    }
    else if (!isStringUnique)
    {
        // reuse
        XUINT32 id = 0;
        IFC_RETURN(m_spMetadataStore->GetStringId(strValue, id));
        sPersistedXamlNode.m_uiObjectId = static_cast<UINT16>(id);
        IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsSharedString));
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, GetVersion(), m_spSubNodeStream));
    }
    else
    {
        // don't reuse, write inplace
        IFC_RETURN(PersistConstantNodeType(PersistedConstantType::IsUniqueString));
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(strValue, GetVersion(), m_spSubNodeStream));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatSubWriter2::PersistSharedString(
    _In_ const xstring_ptr strValue)
{
    PersistedXamlNode2 sPersistedXamlNode;
    XUINT32 id = 0;

    sPersistedXamlNode.m_uiObjectId = 0;

    ASSERT(!strValue.IsNull());
    IFC_RETURN(m_spMetadataStore->GetStringId(strValue, id));
    sPersistedXamlNode.m_uiObjectId = static_cast<UINT16>(id);
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, GetVersion(), m_spSubNodeStream));

    return S_OK;
}
_Check_return_ HRESULT XamlBinaryFormatSubWriter2::GetNodeStreamOffset(_Out_ XUINT64 *pOffset)
{
    IFC_RETURN(m_spSubNodeStream->GetSize(pOffset));

    return S_OK;
}

xref_ptr<IPALStream> XamlBinaryFormatSubWriter2::GetNodeStream() const
{
    return m_spSubNodeStream;
}

xref_ptr<IPALStream> XamlBinaryFormatSubWriter2::GetLineStream() const
{
    return m_spSubLineStream;
}

XamlBinaryFormatSubWriter2::~XamlBinaryFormatSubWriter2()
{
}

