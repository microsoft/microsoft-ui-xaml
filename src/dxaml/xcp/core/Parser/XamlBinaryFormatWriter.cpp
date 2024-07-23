// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBinaryFormatWriter.h"
#include "KeyTime.h"
#include "Duration.h"
#include "XamlBinaryFormatCommon.h"
#include "XamlSerializationHelper.h"
#include <CColor.h>
#include <StringConversions.h>
#include "WinBluePropertyTypeCompatHelper.h"

using namespace DirectUI;

XamlBinaryFormatWriter::~XamlBinaryFormatWriter()
{
}

_Check_return_ HRESULT XamlBinaryFormatWriter::WriteAllNodes(
    _In_ const std::shared_ptr<XamlReader>& spReader
    )
{
    HRESULT hr = S_OK;

    while ((hr = spReader->Read()) == S_OK)
    {        
        bool processedEvent = false;
        const XamlNode& xamlNode = spReader->CurrentNode();
        //TRACE(TraceAlways, L"XamlNode type: %i, line %i, column: %i", xamlNode.get_NodeType(), xamlNode.get_LineInfo().LineNumber(), xamlNode.get_LineInfo().LinePosition());

        // In Blue XBF1 generation, we would encode the Event property and its declaring type.
        // In Threshold we can longer get the declaring type of the Event from the Metadata so we
        // ignore the Event Property and skip writing it to the XBF.
        // This is not a behavior change because we would end up ignoring Event Properties in XBF during runtime
        // as event hookups must occur through x:ConnectionId
        if (xamlNode.get_NodeType() == XamlNodeType::xntStartProperty)
        {
            auto& xamlProperty = xamlNode.get_Property();
            bool isEventProperty = !xamlProperty->IsDirective() && !xamlProperty->IsImplicit() && xamlProperty->IsEvent();
            if (isEventProperty)
            {
                IFC_RETURN(SkipEventProperty(spReader));
                processedEvent = true;
            }
        }

        if (!processedEvent)
        {
            IFC_RETURN(PersistNode(xamlNode));
        }
    }

    IFC_RETURN(hr);
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatWriter::SkipEventProperty(
    _In_ const std::shared_ptr<XamlReader>& spReader
    )
{
    HRESULT hr = S_OK;
    int stackDepth = 0;

    while ((hr = spReader->Read()) == S_OK)
    {        
        auto& xamlNode = spReader->CurrentNode();
        if (xamlNode.get_NodeType() == XamlNodeType::xntEndProperty && stackDepth == 0)
        {
            break;
        }
        else if (xamlNode.get_NodeType() == XamlNodeType::xntStartObject)
        {
            stackDepth++;
        }
        else if (xamlNode.get_NodeType() == XamlNodeType::xntEndObject)
        {
            stackDepth--;
        }
    }    
    
    IFC_RETURN(hr);    
    return S_OK;
}


_Check_return_ HRESULT
XamlBinaryFormatWriter::SaveNodeType(
    XamlNodeType nodeType
    )
{
    ASSERT(nodeType < 256);
    XBYTE nodeTypeByte = static_cast<XBYTE>(nodeType);

    RRETURN(m_spNodeStreamStream->Write(&nodeTypeByte, sizeof(XBYTE), 0, NULL));
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::SaveValueNodeType(
    PersistedXamlValueNode::PersistedXamlValueNodeType nodeType
    )
{
    ASSERT(nodeType < 256);
    XBYTE nodeTypeByte = static_cast<XBYTE>(nodeType);

    RRETURN(m_spNodeStreamStream->Write(&nodeTypeByte, sizeof(XBYTE), 0, NULL));
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistNode(
    const XamlNode& xamlNode
    )
{
    IFC_RETURN(PersistLineInfoIfNeeded(xamlNode));

    switch (xamlNode.get_NodeType())
    {
        case XamlNodeType::xntNamespace:
            IFC_RETURN(PersistAddNamespaceNode(xamlNode));
            break;

        case XamlNodeType::xntStartObject:
            IFC_RETURN(PersistStartObjectNode(xamlNode));
            break;

        case XamlNodeType::xntEndObject:
            IFC_RETURN(PersistEndObjectNode(xamlNode));
            break;

        case XamlNodeType::xntStartProperty:
            IFC_RETURN(PersistStartMemberNode(xamlNode));
            break;

        case XamlNodeType::xntEndProperty:
            IFC_RETURN(PersistEndMemberNode(xamlNode));
            break;

        case XamlNodeType::xntText:
            IFC_RETURN(PersistTextValueNode(xamlNode));
            break;

        case XamlNodeType::xntValue:
            IFC_RETURN(PersistValueNode(xamlNode));
            break;

        case XamlNodeType::xntEndOfStream:
            IFC_RETURN(PersistEndOfStreamNode(xamlNode));
            break;

        case XamlNodeType::xntEndOfAttributes:
            break;
        case XamlNodeType::xntNone:
            ASSERT(FALSE);
            break;

        default:
            IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistLineInfoIfNeeded(const XamlNode& xamlNode)
{
    bool fStoreAbsolutePosition = false;

    if (xamlNode.get_LineInfo().LineNumber() != m_CurrentLineInfo.LineNumber()
        || xamlNode.get_LineInfo().LinePosition() != m_CurrentLineInfo.LinePosition())
    {
        XUINT32 uiLineNumberDelta        = 0;
        XUINT32 uiLinePositionDelta      = 0;

        uiLineNumberDelta = xamlNode.get_LineInfo().LineNumber() - m_CurrentLineInfo.LineNumber();
        if (XcpAbs(uiLineNumberDelta) >= 0x8000)
        {
            fStoreAbsolutePosition = TRUE;
        }

        if (uiLineNumberDelta > 0)
        {
            uiLinePositionDelta = xamlNode.get_LineInfo().LinePosition();
        }
        else
        {
            uiLinePositionDelta = xamlNode.get_LineInfo().LinePosition() - m_CurrentLineInfo.LinePosition();
        }

        if ( (XcpAbs(uiLinePositionDelta) >= 0x8000) )
        {
            fStoreAbsolutePosition = TRUE;
        }
        m_CurrentLineInfo = XamlLineInfo(xamlNode.get_LineInfo().LineNumber(), xamlNode.get_LineInfo().LinePosition());

        if (m_fGenerateLineInfo)
        {
            if (!fStoreAbsolutePosition)
            {
                // We're going to truncate these to 16 bits each to store in a LineInfoDeltaXamlNodeData, let's check if that's safe
                ASSERT(XcpAbs(uiLineNumberDelta) <= INT16_MAX);
                ASSERT(XcpAbs(uiLinePositionDelta) <= INT16_MAX);

                LineInfoDeltaXamlNodeData lineInfoDeltaNodeData;
                // [LineNumberDelta:16][LinePositionDelta:16]
                lineInfoDeltaNodeData.m_uiLineNumberDelta   = static_cast<XINT16>(uiLineNumberDelta);
                lineInfoDeltaNodeData.m_uiLinePositionDelta = static_cast<XINT16>(uiLinePositionDelta);

                IFC_RETURN(SaveNodeType(XamlNodeType::xntLineInfo));
                IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(lineInfoDeltaNodeData, GetVersion(), m_spNodeStreamStream));
            }
            else
            {
                LineInfoAbsoluteXamlNodeData lineInfoAbsoluteNodeData;
                // [LineNumber:32][LinePosition:32]
                lineInfoAbsoluteNodeData.m_uiLineNumber   = xamlNode.get_LineInfo().LineNumber();
                lineInfoAbsoluteNodeData.m_uiLinePosition = xamlNode.get_LineInfo().LinePosition();

                IFC_RETURN(SaveNodeType(XamlNodeType::xntLineInfoAbsolute));
                IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(lineInfoAbsoluteNodeData, GetVersion(), m_spNodeStreamStream));
            }
        }
    }

    return S_OK;
}



_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistAddNamespaceNode(const XamlNode& xamlNode)
{
    PersistedXamlNode sPersistedXamlNode;

    IFC_RETURN(SaveNodeType(XamlNodeType::xntNamespace));

    IFC_RETURN(m_spMetadataStore->GetXamlXmlNamespaceId(xamlNode.get_Namespace(), sPersistedXamlNode.m_uiObjectId));
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(xamlNode.get_Prefix(), m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistStartObjectNode(const XamlNode& xamlNode)
{
    PersistedXamlNode sPersistedXamlNode;

    IFC_RETURN(SaveNodeType(XamlNodeType::xntStartObject));
    IFCPTR_RETURN(m_spMetadataStore);
    IFC_RETURN(m_spMetadataStore->GetXamlTypeNode(xamlNode.get_XamlType(), sPersistedXamlNode));
    if (xamlNode.get_IsRetrievedObject())
    {
        sPersistedXamlNode.m_NodeFlags.SetBit(PersistedXamlNode::IsRetrieved);
    }
    if (xamlNode.IsUnknown())
    {
        sPersistedXamlNode.m_NodeFlags.SetBit(PersistedXamlNode::IsUnknown);
    }
    else
    {
        // please see comments in WinBluePropertyTypeCompatHelper.cpp.
        m_winbluePropertyTypeCompatHelper.IncreaseScopeDepth(xamlNode.get_XamlType());
    }

    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistEndObjectNode(const XamlNode& xamlNode)
{
    // please see comments in WinBluePropertyTypeCompatHelper.cpp.
    m_winbluePropertyTypeCompatHelper.DecreaseScopeDepth();

    IFC_RETURN(SaveNodeType(XamlNodeType::xntEndObject));

    // no needed data for this node type

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistStartMemberNode(const XamlNode& xamlNode)
{
    PersistedXamlNode sPersistedXamlNode;

    IFC_RETURN(SaveNodeType(XamlNodeType::xntStartProperty));
    IFCPTR_RETURN(m_spMetadataStore);
    IFC_RETURN(m_spMetadataStore->GetXamlPropertyNode(xamlNode.get_Property(), m_winbluePropertyTypeCompatHelper,sPersistedXamlNode));
    if (xamlNode.IsUnknown())
    {
        sPersistedXamlNode.m_NodeFlags.SetBit(PersistedXamlNode::IsUnknown);
    }
    else
    {
        m_spLastXamlProperty = xamlNode.get_Property();
    }

    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistEndMemberNode(const XamlNode& xamlNode)
{
    m_spLastXamlProperty.reset();
    IFC_RETURN(SaveNodeType(XamlNodeType::xntEndProperty));

    // no needed data for this node type
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistEndOfAttributesNode(const XamlNode& xamlNode)
{
    IFC_RETURN(SaveNodeType(XamlNodeType::xntEndOfAttributes));

    // no needed data for this node type
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistEndOfStreamNode(const XamlNode& xamlNode)
{
    IFC_RETURN(SaveNodeType(XamlNodeType::xntEndOfStream));

    // no needed data for this node type

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::TryOptimizePropertyValue(std::shared_ptr<XamlProperty>& spXamlProperty, _In_ const xstring_ptr& spTextString, bool *pfPropertyOptimized)
{
    HRESULT hr = S_OK;

    *pfPropertyOptimized = FALSE;

    if (spXamlProperty)
    {
        if (!m_spLastXamlProperty->IsImplicit())
        {
            CREATEPARAMETERS cp(m_pCore, spTextString);
            std::shared_ptr<XamlTextSyntax> spTextSyntax;

            IFC(m_spLastXamlProperty->get_TextSyntax(spTextSyntax));
            XamlTypeToken textSyntaxToken = spTextSyntax->get_TextSyntaxToken();

            switch (textSyntaxToken.GetHandle())
            {
                case KnownTypeIndex::Int32:
                    {
                        CValue value;

                        IFC(CInt32::CreateCValue(&cp, value));

                        IFC(SaveNodeType(XamlNodeType::xntValue));
                        IFC(SaveValueNodeType(PersistedXamlValueNode::IsSigned));
                        IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value.AsSigned(), m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                    }
                    *pfPropertyOptimized = TRUE;
                    break;

                case KnownTypeIndex::Boolean:
                    {
                        CValue value;

                        IFC(CBoolean::CreateCValue(&cp, value));
                        IFC(SaveNodeType(XamlNodeType::xntValue));
                        if (value.AsBool() == TRUE)
                        {
                            IFC(SaveValueNodeType(PersistedXamlValueNode::IsBoolTrue));
                        }
                        else
                        {
                            IFC(SaveValueNodeType(PersistedXamlValueNode::IsBoolFalse));
                        }
                    }
                    *pfPropertyOptimized = TRUE;
                    break;

                case KnownTypeIndex::Double:
                    {
                        CValue value;

                        IFC(CDouble::CreateCValue(&cp, value));

                        IFC(SaveNodeType(XamlNodeType::xntValue));
                        IFC(SaveValueNodeType(PersistedXamlValueNode::IsFloat));
                        IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value.AsFloat(), m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                    }
                    *pfPropertyOptimized = TRUE;
                    break;

                case KnownTypeIndex::String:
                    {
                        // In Blue, property paths were persisted as regular strings, not CStrings.
                        if (!m_spLastXamlProperty->IsDirective() && (
                            m_spLastXamlProperty->get_PropertyToken().GetHandle() == KnownPropertyIndex::Binding_Path ||
                            m_spLastXamlProperty->get_PropertyToken().GetHandle() == KnownPropertyIndex::CollectionViewSource_ItemsPath))
                        {
                            *pfPropertyOptimized = FALSE;
                        }
                        else
                        {
                            IFC(SaveNodeType(XamlNodeType::xntValue));
                            IFC(SaveValueNodeType(PersistedXamlValueNode::IsCString));
                            IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(spTextString, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                            *pfPropertyOptimized = TRUE;
                        }
                    }
                    break;

                case KnownTypeIndex::KeyTime:
                    {
                        DOUBLE value = 0.0;

                        IFC(KeyTimeFromString(spTextString, &value));
                        IFC(SaveNodeType(XamlNodeType::xntValue));
                        IFC(SaveValueNodeType(PersistedXamlValueNode::IsKeyTime));
                        IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(static_cast<XFLOAT>(value), m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                    }
                    *pfPropertyOptimized = TRUE;
                    break;

                case KnownTypeIndex::Thickness:
                    {
                        XTHICKNESS value;

                        IFC(ThicknessFromString(spTextString, &value));
                        IFC(SaveNodeType(XamlNodeType::xntValue));
                        IFC(SaveValueNodeType(PersistedXamlValueNode::IsThickness));
                        IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                    }
                    *pfPropertyOptimized = TRUE;
                    break;

                case KnownTypeIndex::LengthConverter:
                    {
                        CValue value;

                        IFC(CLengthConverter::CreateCValue(&cp, value));
                        IFC(SaveNodeType(XamlNodeType::xntValue));
                        IFC(SaveValueNodeType(PersistedXamlValueNode::IsLengthConverter));
                        IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value.AsFloat(), m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                    }
                    *pfPropertyOptimized = TRUE;
                    break;

                case KnownTypeIndex::GridLength:
                    {
                        XGRIDLENGTH value;

                        IFC(GridLengthFromString(spTextString, &value));
                        IFC(SaveNodeType(XamlNodeType::xntValue));
                        IFC(SaveValueNodeType(PersistedXamlValueNode::IsGridLength));
                        IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                    }
                    *pfPropertyOptimized = TRUE;
                    break;

                case KnownTypeIndex::Color:
                    {
                        XUINT32 value = 0;

                        IFC(CColor::ColorFromString(spTextString, &value));
                        IFC(SaveNodeType(XamlNodeType::xntValue));
                        IFC(SaveValueNodeType(PersistedXamlValueNode::IsColor));
                        IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(value, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                    }
                    *pfPropertyOptimized = TRUE;
                    break;

                case KnownTypeIndex::Duration:
                    {
                        XDOUBLE value = 0.0;
                        DurationType type;

                        IFC(DurationFromString(spTextString, &type, &value));
                        if (type == DirectUI::DurationType::TimeSpan)
                        {
                            IFC(SaveNodeType(XamlNodeType::xntValue));
                            IFC(SaveValueNodeType(PersistedXamlValueNode::IsDuration));
                            IFC(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(static_cast<XFLOAT>(value), m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
                            *pfPropertyOptimized = TRUE;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

Cleanup:
    // if the type conversion optimization failed during generation for some reason, just let the xbf writer encode as text which will be presented
    // to the user as a parser error at runtime.
    if (FAILED(hr))
    {
        *pfPropertyOptimized = FALSE;
        hr = S_OK;
    }

    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistTextValueNode(const XamlNode& xamlNode)
{
    bool fPropertyOptimized = false;
    xstring_ptr spTextString;

    IFC_RETURN(xamlNode.get_Text()->get_Text(&spTextString));

    IFC_RETURN(TryOptimizePropertyValue(m_spLastXamlProperty, spTextString, &fPropertyOptimized))

    if (!fPropertyOptimized)
    {
        PersistedXamlNode sPersistedXamlNode;
        bool fIsStringUnique = false;
        IFCPTR_RETURN(m_spMetadataStore);

        IFC_RETURN(SaveNodeType(XamlNodeType::xntText));
        sPersistedXamlNode.m_uiObjectId = 0;

        // apply heuristic to figure out reuse
        // don't attempt to reuse strings for value node if they are
        // numeric or large length strings
        if (spTextString.GetCount() > 64)
        {
            fIsStringUnique = TRUE;
        }
        else if (spTextString.GetCount() > 2)
        {
            if (xisdigit(spTextString.GetChar(0)) ||
                xisdigit(spTextString.GetChar(1)) ||
                xisdigit(spTextString.GetChar(2)))
            {
                fIsStringUnique = TRUE;
            }
        }

        if (!fIsStringUnique)
        {
            // reuse
            IFC_RETURN(m_spMetadataStore->GetStringId(spTextString, sPersistedXamlNode.m_uiObjectId));
        }
        else
        {
            // don't reuse
            IFC_RETURN(m_spMetadataStore->GetXamlTextValueId(spTextString, sPersistedXamlNode.m_uiObjectId));
            sPersistedXamlNode.m_NodeFlags.SetBit(PersistedXamlNode::IsStringValueAndUnique);
        }

        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));
    }

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatWriter::PersistValueNode(const XamlNode& xamlNode)
{
    PersistedXamlNode sPersistedXamlNode;

    IFC_RETURN(SaveNodeType(XamlNodeType::xntValue));

    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToNodeStream(sPersistedXamlNode, m_spMetadataStore->GetVersion(), m_spNodeStreamStream));

    ASSERT(FALSE);

    return S_OK;
}


