// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBinaryFormatReader.h"
#include "XamlSerializationHelper.h"
#include "XamlBinaryFormatCommon.h"

XamlBinaryFormatReader::~XamlBinaryFormatReader()
{
        delete[] m_pbNodeStream;
        m_pbNodeStream = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new instance of a XamlTextReader, and returns an std::shared_ptr
//      to that new instance.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
XamlBinaryFormatReader::Create(
    _In_ std::shared_ptr<XamlSchemaContext> spXamlSchemaContext,
    _In_ std::shared_ptr<XamlBinaryMetadataReader> spXamlBinaryMetadataReader,
    _In_ IPALStream* pNodeStreamStream,
    _Out_ std::shared_ptr<XamlBinaryFormatReader>& spXamlBinaryFormatReader
    )
{
    spXamlBinaryFormatReader = std::make_shared<XamlBinaryFormatReader>(spXamlSchemaContext, spXamlBinaryMetadataReader, pNodeStreamStream);
    IFC_RETURN(spXamlBinaryFormatReader->Initialize(pNodeStreamStream));

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatReader::Initialize(
    _In_ IPALStream* pNodeStreamStream
    )
{
    XUINT64 cbSize  = 0;
    XUINT32 cbRead = 0;
    std::shared_ptr<XamlType> spXamlStringType;

    IFC_RETURN(pNodeStreamStream->GetSize(&cbSize));

    IFCEXPECT_RETURN(m_cbNodeStream <= XUINT32_MAX);

    m_cbNodeStream = static_cast<XUINT32>(cbSize);

    m_pbNodeStream = new XUINT8[m_cbNodeStream];

    IFC_RETURN(pNodeStreamStream->Read(m_pbNodeStream, m_cbNodeStream, &cbRead));
    ASSERT(cbRead == m_cbNodeStream);

    IFC_RETURN(m_spXamlSchemaContext->get_StringXamlType(spXamlStringType));
    m_stringXamlTypeToken = spXamlStringType->get_TypeToken();

    return S_OK;
}


_Check_return_ HRESULT
XamlBinaryFormatReader::Read()
{
    HRESULT hr = S_OK;
    XamlNodeType nodeType;
    bool fContinue = true;

    while (fContinue)
    {
        IFC(ReadNodeType(nodeType));
        // see comment about S_FALSE in ReadNodeType.
        if (hr == S_FALSE)
        {
            goto Cleanup;
        }

        fContinue = FALSE;
        switch (nodeType)
        {
            case XamlNodeType::xntNamespace:
                IFC(ReadAddNamespaceNode());
                break;

            case XamlNodeType::xntStartObject:
                IFC(ReadStartObjectNode());
                break;

            case XamlNodeType::xntEndObject:
                IFC(ReadEndObjectNode());
                break;

            case XamlNodeType::xntStartProperty:
                IFC(ReadStartMemberNode());
                break;

            case XamlNodeType::xntEndProperty:
                IFC(ReadEndMemberNode());
                break;

            case XamlNodeType::xntText:
                IFC(ReadTextValueNode());
                break;

            case XamlNodeType::xntValue:
                IFC(ReadValueNode());
                break;

            case XamlNodeType::xntEndOfStream:
                IFC(ReadEndOfStreamNode());
                break;

            case XamlNodeType::xntEndOfAttributes:
                break;
            case XamlNodeType::xntNone:
                ASSERT(FALSE);
                break;
            case XamlNodeType::xntLineInfo:
                IFC(ReadLineInfoDelta());
                fContinue = TRUE;
                break;
            case XamlNodeType::xntLineInfoAbsolute:
                IFC(ReadLineInfoAbsolute());
                fContinue = TRUE;
                break;
            default:
                IFC(E_FAIL);
                // throw new NotImplementedException(SR.Get(SRID.MissingCaseXamlNodes));
        }
    }

Cleanup:
    RRETURN(hr);
}

const XamlNode& XamlBinaryFormatReader::CurrentNode()// const
{
    return m_currentXamlNode;
}


_Check_return_ HRESULT
XamlBinaryFormatReader::ReadNodeType(
    _Out_ XamlNodeType& nodeType
)
{
    HRESULT hr = S_OK;
    XBYTE nodeTypeByte = 0;

    if (FAILED(ReadFromNodeStreamBuffer(&nodeTypeByte, sizeof(XBYTE))))
    {
        hr = S_FALSE;
    }

    nodeType = (XamlNodeType)nodeTypeByte;

//Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadValueNodeType(
    _Out_ PersistedXamlValueNode::PersistedXamlValueNodeType& nodeType
)
{
    HRESULT hr = S_OK;
    XBYTE nodeTypeByte = 0;

    if (FAILED(ReadFromNodeStreamBuffer(&nodeTypeByte, sizeof(XBYTE))))
    {
        hr = S_FALSE;
    }

    nodeType = (PersistedXamlValueNode::PersistedXamlValueNodeType)nodeTypeByte;

//Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadLineInfoDelta()
{
    HRESULT hr = S_OK;
    LineInfoDeltaXamlNodeData lineInfoDeltaNodeData;
    XUINT32 uiLineNumberDelta        = 0;
    XUINT32 uiLinePositionDelta      = 0;
    XUINT32 uiLineNumber = m_currentLineInfo.LineNumber();
    XUINT32 uiLinePosition = m_currentLineInfo.LinePosition();

    if (FAILED(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&lineInfoDeltaNodeData), sizeof(LineInfoDeltaXamlNodeData))))
    {
        hr = S_FALSE;
    }

    uiLineNumberDelta = lineInfoDeltaNodeData.m_uiLineNumberDelta;
    uiLinePositionDelta = lineInfoDeltaNodeData.m_uiLinePositionDelta;

    uiLineNumber += uiLineNumberDelta;

    if (uiLineNumberDelta > 0)
    {
        uiLinePosition = uiLinePositionDelta;
    }
    else
    {
        uiLinePosition += uiLinePositionDelta;
    }

    m_currentLineInfo = XamlLineInfo(uiLineNumber, uiLinePosition);

//Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadLineInfoAbsolute()
{
    HRESULT hr = S_OK;
    LineInfoAbsoluteXamlNodeData lineInfoAbsoluteNodeData;
    XUINT32 uiLineNumber = m_currentLineInfo.LineNumber();
    XUINT32 uiLinePosition = m_currentLineInfo.LinePosition();

    if (FAILED(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&lineInfoAbsoluteNodeData), sizeof(LineInfoAbsoluteXamlNodeData))))
    {
        hr = S_FALSE;
    }

    uiLineNumber   = lineInfoAbsoluteNodeData.m_uiLineNumber;
    uiLinePosition = lineInfoAbsoluteNodeData.m_uiLinePosition;

    m_currentLineInfo = XamlLineInfo(uiLineNumber, uiLinePosition);

//Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadAddNamespaceNode()
{
    HRESULT hr = S_OK;
    PersistedXamlNode sPersistedXamlNode;
    xstring_ptr spPrefixString;
    std::shared_ptr<XamlNamespace> spXamlNamespace;

    IFC(ReadPersistedXamlNode(sPersistedXamlNode));
    IFC(ReadPersistedString(&spPrefixString));

    IFC(m_spXamlBinaryMetadataReader->GetXmlNamespace(sPersistedXamlNode.m_uiObjectId, spXamlNamespace));
    if (hr == S_FALSE)
    {
        IFC(E_FAIL);
    }

    m_currentXamlNode.InitAddNamespaceNode(spPrefixString, spXamlNamespace);
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);
Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT
XamlBinaryFormatReader::ReadStartObjectNode()
{
    HRESULT hr = S_OK;
    PersistedXamlNode sPersistedXamlNode;
    std::shared_ptr<XamlType> spXamlType;

    IFC(ReadPersistedXamlNode(sPersistedXamlNode));
    IFC(m_spXamlBinaryMetadataReader->GetType(sPersistedXamlNode.m_uiObjectId, spXamlType));
    if (hr == S_FALSE)
    {
        IFC(E_FAIL);
    }

    m_currentXamlNode.InitStartObjectNode(spXamlType, sPersistedXamlNode.m_NodeFlags.IsBitSet(PersistedXamlNode::IsRetrieved), sPersistedXamlNode.m_NodeFlags.IsBitSet(PersistedXamlNode::IsUnknown));
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadEndObjectNode()
{
    HRESULT hr = S_OK;

    m_currentXamlNode.InitEndObjectNode();
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);

//Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadStartMemberNode()
{
    HRESULT hr = S_OK;

    PersistedXamlNode sPersistedXamlNode;
    std::shared_ptr<XamlProperty> spXamlProperty;

    IFC(ReadPersistedXamlNode(sPersistedXamlNode));

    IFC(m_spXamlBinaryMetadataReader->GetProperty(sPersistedXamlNode.m_uiObjectId, spXamlProperty));
    if (hr == S_FALSE)
    {
        IFC(E_FAIL);
    }

    m_currentXamlNode.InitStartMemberNode(spXamlProperty);
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadEndMemberNode()
{
    HRESULT hr = S_OK;

    m_currentXamlNode.InitEndMemberNode();
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);

//Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadEndOfAttributesNode()
{
    HRESULT hr = S_OK;
    // No need for this
    // PersistedXamlNode sPersistedXamlNode;
    //
    m_currentXamlNode.InitEndOfAttributesNode();
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);

//Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadEndOfStreamNode()
{
    HRESULT hr = S_OK;
    m_currentXamlNode.InitEndOfStreamNode();
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);

//Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadTextValueNode()
{
    HRESULT hr = S_OK;
    xstring_ptr spStringValue;
    CValue vTemp;
    std::shared_ptr<XamlQualifiedObject> qo;
    PersistedXamlNode sPersistedXamlNode;

    IFC(ReadPersistedXamlNode(sPersistedXamlNode));
    qo = std::make_shared<XamlQualifiedObject>();

    if (!sPersistedXamlNode.m_NodeFlags.IsBitSet(PersistedXamlNode::IsStringValueAndUnique))
    {
        IFC(m_spXamlBinaryMetadataReader->GetString(sPersistedXamlNode.m_uiObjectId, spStringValue));
        ASSERT(hr != S_FALSE);

        vTemp.SetString(std::move(spStringValue));

        IFC(qo->SetValue(vTemp));
        qo->SetTypeToken(m_stringXamlTypeToken);
    }
    else
    {
        IFC(m_spXamlBinaryMetadataReader->GetString(sPersistedXamlNode.m_uiObjectId, spStringValue));
        ASSERT(hr != S_FALSE);

        vTemp.SetString(std::move(spStringValue));

        IFC(qo->SetValue(m_stringXamlTypeToken, vTemp));
    }

    m_currentXamlNode.InitValueNode(qo);
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadValueNode()
{
    HRESULT hr = S_OK;
    CValue vTemp;
    PersistedXamlValueNode::PersistedXamlValueNodeType nodeType;
    std::shared_ptr<XamlQualifiedObject> qo;
    XTHICKNESS *pThicknessValue = NULL;
    XGRIDLENGTH *pGridLengthValue = NULL;
    qo = std::make_shared<XamlQualifiedObject>();

    IFC(ReadValueNodeType(nodeType));

    switch (nodeType)
    {
        case PersistedXamlValueNode::IsBoolFalse:
            {
                vTemp.SetBool(FALSE);
                IFC(qo->AttachValue(std::move(vTemp)));
            }
            break;
        case PersistedXamlValueNode::IsBoolTrue:
            {
                vTemp.SetBool(TRUE);
                IFC(qo->AttachValue(std::move(vTemp)));
            }
            break;
        case PersistedXamlValueNode::IsFloat:
            {
                XFLOAT value = 0.0f;
                IFC(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&value), sizeof(XFLOAT)));
                vTemp.SetFloat(value);
                IFC(qo->AttachValue(std::move(vTemp)));
            }
            break;
        case PersistedXamlValueNode::IsSigned:
            {
                XINT32 value = 0;
                IFC(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&value), sizeof(XINT32)));
                vTemp.SetSigned(value);
                IFC(qo->AttachValue(std::move(vTemp)));
            }
            break;
        case PersistedXamlValueNode::IsCString:
            {
                xstring_ptr strString;
                IFC(ReadPersistedString(&strString));
                vTemp.SetString(std::move(strString));
                IFC(qo->AttachValue(XamlTypeToken(tpkNative, KnownTypeIndex::String), std::move(vTemp)));
                // TODO: Disabling CString creation because it confuses the TypeConverter into thinking
                // this is a DependencyObject.
                // CCoreServices* pCore = m_spXamlSchemaContext->GetCore();
                // IFC(CString::CreateFromXStringPtr(pCore, std::move(strString), &pStringAsDO));
                // pQO->SetDependencyObjectNoAddRef(pStringAsDO);
                // pQO->SetTypeToken(XamlTypeToken(tpkNative, pStringAsDO->GetTypeIndex()));
            }
            break;
        case PersistedXamlValueNode::IsKeyTime:
            {
                XFLOAT value = 0.0f;

                IFC(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&value), sizeof(XFLOAT)));
                vTemp.SetFloat(value);
                IFC(qo->AttachValue(std::move(vTemp)));
                qo->SetIsXbfOptimized();
            }
            break;
        case PersistedXamlValueNode::IsThickness:
            {
                pThicknessValue = new XTHICKNESS;

                IFC(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(pThicknessValue), sizeof(XTHICKNESS)));
                vTemp.SetThickness(pThicknessValue);
                IFC(qo->AttachValue(std::move(vTemp)));
                pThicknessValue = NULL;
            }
            break;
        case PersistedXamlValueNode::IsLengthConverter:
            {
                XFLOAT value = 0.0f;
                IFC(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&value), sizeof(XFLOAT)));
                vTemp.SetFloat(value);
                IFC(qo->AttachValue(std::move(vTemp)));
            }
            break;
        case PersistedXamlValueNode::IsGridLength:
            {
                pGridLengthValue = new XGRIDLENGTH;

                IFC(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(pGridLengthValue), sizeof(XGRIDLENGTH)));
                vTemp.SetGridLength(pGridLengthValue);
                IFC(qo->AttachValue(std::move(vTemp)));
                pGridLengthValue = NULL;
            }
            break;

        case PersistedXamlValueNode::IsColor:
            {
                XUINT32 value = 0;

                IFC(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&value), sizeof(XUINT32)));
                vTemp.SetColor(value);
                IFC(qo->AttachValue(std::move(vTemp)));
            }
            break;

        case PersistedXamlValueNode::IsDuration:
            {
                XFLOAT value = 0.0f;

                IFC(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&value), sizeof(XFLOAT)));
                vTemp.SetFloat(value);
                IFC(qo->AttachValue(std::move(vTemp)));
                qo->SetIsXbfOptimized();
            }
            break;
        default:
            break;
    }

    m_currentXamlNode.InitValueNode(qo);
    m_currentXamlNode.set_LineInfo(m_currentLineInfo);

Cleanup:
    delete pThicknessValue;
    delete pGridLengthValue;
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryFormatReader::ReadPersistedString(
    _Out_ xstring_ptr* pstrTargetString
)
{
    XUINT32 cch = 0;
    WCHAR* pch = NULL;
    XStringBuilder bufferBuilder;

    IFC_RETURN(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(&cch), sizeof(XUINT32)));
    IFCEXPECT_RETURN(cch < (XUINT32_MAX / sizeof(WCHAR)));

    IFC_RETURN(bufferBuilder.InitializeAndGetFixedBuffer(cch, &pch));
    IFC_RETURN(ReadFromNodeStreamBuffer(reinterpret_cast<XUINT8*>(pch), sizeof(WCHAR) * cch));
    IFC_RETURN(bufferBuilder.DetachString(pstrTargetString));

    return S_OK;
}

