// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CustomWriterRuntimeData.h"
#include "XamlBinaryFormatCommon.h"
#include "XamlBinaryFormatReader2.h"
#include "XamlBinaryFormatSubReader2.h"
#include "XamlBinaryMetadataReader2.h"
#include "XamlPredicateHelpers.h"
#include "XamlSerializationHelper.h"
#include "XbfMetadataApi.h"

#pragma region Public Interface

XamlBinaryFormatSubReader2::XamlBinaryFormatSubReader2(
    std::shared_ptr<XamlBinaryFormatReader2> xamlBinaryFormatMasterReader,
    unsigned int nodeStreamMasterOffset, unsigned int nodeStreamLength,
    unsigned int lineStreamLength,
    _In_ uint8_t* nodeStream,
    _In_opt_ uint8_t* lineStream)
    : m_spXamlBinaryFormatMasterReader(std::move(xamlBinaryFormatMasterReader))
    , m_nodeStreamMasterOffset(nodeStreamMasterOffset)
    , m_nodeStreamLength(nodeStreamLength)
    , m_lineStreamLength(lineStreamLength)
    , m_nodeStream(nodeStream)
    , m_lineStream(lineStream)
    , m_currentNodeStreamOffset(0)
    , m_lastNodeStreamOffset(0)
    , m_spTemporaryValue(std::make_shared<XamlQualifiedObject>())
{
}

_Check_return_ HRESULT XamlBinaryFormatSubReader2::TryReadHRESULT(
    ObjectWriterNode& currentXamlNode, _Out_ bool* endOfStream)
{
    try
    {
        *endOfStream = !TryRead(currentXamlNode);
    }
    CATCH_RETURN();
    return S_OK;
}

bool XamlBinaryFormatSubReader2::TryRead(ObjectWriterNode& currentXamlNode)
{
    ObjectWriterNodeType nodeType;
    m_lastNodeStreamOffset = m_currentNodeStreamOffset;

    if (!TryReadNodeType(nodeType))
    {
        return false;
    }

    switch (nodeType)
    {
        case ObjectWriterNodeType::PushScope:
        {
            currentXamlNode = ObjectWriterNode::MakePushScopeNode(GetLineInfo());
        }
        break;

        case ObjectWriterNodeType::PopScope:
        {
            currentXamlNode = ObjectWriterNode::MakePopScopeNode(GetLineInfo());
        }
        break;

        case ObjectWriterNodeType::AddNamespace:
        {
            currentXamlNode = ReadAddNamespaceNode();
        }
        break;

        case ObjectWriterNodeType::PushConstant:
        {
            currentXamlNode = ReadPushConstantNode();
        }
        break;

        case ObjectWriterNodeType::SetValue:
        {
            currentXamlNode = ReadSetValueNode();
        }
        break;

        case ObjectWriterNodeType::SetValueFromMarkupExtension:
        {
            currentXamlNode = ReadSetValueFromMarkupExtensionNode();
        }
        break;

        case ObjectWriterNodeType::AddToCollection:
        {
            currentXamlNode = ObjectWriterNode::MakeAddToCollectionNode(GetLineInfo());
        }
        break;

        case ObjectWriterNodeType::AddToDictionary:
        {
            currentXamlNode = ObjectWriterNode::MakeAddToDictionaryNode(GetLineInfo());
        }
        break;

        case ObjectWriterNodeType::AddToDictionaryWithKey:
        {
            currentXamlNode = ReadAddToDictionaryWithKeyNode();
        }
        break;

        case ObjectWriterNodeType::CheckPeerType:
        {
            currentXamlNode = ReadMakeCheckPeerTypeNode();
        }
        break;

        case ObjectWriterNodeType::SetConnectionId:
        {
            currentXamlNode = ReadSetConnectionIdNode();
        }
        break;

        case ObjectWriterNodeType::SetName:
        {
            currentXamlNode = ReadSetNameNode();
        }
        break;

        case ObjectWriterNodeType::GetResourcePropertyBag:
        {
            currentXamlNode = ReadGetResourcePropertyBagNode();
        }
        break;

        case ObjectWriterNodeType::ProvideValue:
        {
            currentXamlNode = ObjectWriterNode::MakeProvideValueNode(GetLineInfo());
        }
        break;

        case ObjectWriterNodeType::SetDeferredProperty:
        {
            currentXamlNode = ReadSetDeferredPropertyNode();
        }
        break;

        case ObjectWriterNodeType::SetCustomRuntimeData:
        {
            currentXamlNode = ReadSetCustomRuntimeDataNode();
        }
        break;

        case ObjectWriterNodeType::PushScopeAddNamespace:
        {
            currentXamlNode = ReadPushScopeAddNamespaceNode();
        }
        break;

        case ObjectWriterNodeType::PushScopeGetValue:
        {
            currentXamlNode = ReadPushScopeGetValueNode();
        }
        break;

        case ObjectWriterNodeType::PushScopeCreateTypeBeginInit:
        {
            currentXamlNode = ReadPushScopeCreateTypeBeginInitNode();
        }
        break;

        case ObjectWriterNodeType::PushScopeCreateTypeWithConstantBeginInit:
        {
            currentXamlNode = ReadPushScopeCreateTypeWithConstantBeginInitNode();
        }
        break;

        case ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstantBeginInit:
        {
            currentXamlNode = ReadPushScopeCreateTypeWithTypeConvertedConstantBeginInitNode();
        }
        break;

        case ObjectWriterNodeType::CreateTypeBeginInit:
        {
            currentXamlNode = ReadCreateTypeBeginInitNode();
        }
        break;

        case ObjectWriterNodeType::CreateTypeWithConstantBeginInit:
        {
            currentXamlNode = ReadCreateTypeWithConstantBeginInitNode();
        }
        break;

        case ObjectWriterNodeType::CreateTypeWithTypeConvertedConstantBeginInit:
        {
            currentXamlNode = ReadCreateTypeWithTypeConvertedConstantBeginInitNode();
        }
        break;

        case ObjectWriterNodeType::SetValueConstant:
        {
            currentXamlNode = ReadSetValueConstantNode();
        }
        break;

        case ObjectWriterNodeType::SetValueTypeConvertedConstant:
        {
            currentXamlNode = ReadSetValueTypeConvertedConstantNode();
        }
        break;

        case ObjectWriterNodeType::SetValueTypeConvertedResolvedType:
        {
            currentXamlNode = ReadSetValueTypeConvertedResolvedTypeNode();
        }
        break;

        case ObjectWriterNodeType::SetValueTypeConvertedResolvedProperty:
        {
            currentXamlNode = ReadSetValueTypeConvertedResolvedPropertyNode();
        }
        break;

        case ObjectWriterNodeType::ProvideStaticResourceValue:
        {
            currentXamlNode = ReadProvideStaticResourceValueNode();
        }
        break;

        case ObjectWriterNodeType::SetValueFromStaticResource:
        {
            currentXamlNode = ReadSetValueFromStaticResourceNode();
        }
        break;

        case ObjectWriterNodeType::ProvideThemeResourceValue:
        {
            currentXamlNode = ReadProvideThemeResourceValueNode();
        }
        break;

        case ObjectWriterNodeType::SetValueFromThemeResource:
        {
            currentXamlNode = ReadSetValueFromThemeResourceNode();
        }
        break;

        case ObjectWriterNodeType::SetValueFromTemplateBinding:
        {
            currentXamlNode = ReadSetValueFromTemplateBindingNode();
        }
        break;

        case ObjectWriterNodeType::EndInitPopScope:
        {
            currentXamlNode = ObjectWriterNode::MakeEndInitPopScopeNode(GetLineInfo());
        }
        break;

        case ObjectWriterNodeType::EndInitProvideValuePopScope:
        {
            currentXamlNode = ObjectWriterNode::MakeEndInitProvideValuePopScopeNode(GetLineInfo());
        }
        break;

        case ObjectWriterNodeType::BeginConditionalScope:
        {
            currentXamlNode = ReadBeginConditionalScopeNode();
        }
        break;

        case ObjectWriterNodeType::EndConditionalScope:
        {
            currentXamlNode = ObjectWriterNode::MakeEndConditionalScope(GetLineInfo());
        }
        break;

        default:
        {
            ASSERT(false);
        }
        break;
    }

    return true;
}

// linear search the stream offset to find the matching line information
const XamlLineInfo XamlBinaryFormatSubReader2::ResolveLineInfo(
    _In_ const unsigned int streamOffset)
{
    if (m_lineStream)
    {
        XUINT8 *pLineBuffer = m_lineStream;
        XUINT32 lineBufferSize = m_lineStreamLength;
        XUINT32 currentBufferOffset = 0;
        unsigned int currentStreamOffset = 0;
        int currentLineOffset = 0;
        int currentPositionOffset = 0;

        for (;;)
        {
            unsigned int deltaStreamOffset = 0;
            unsigned int deltaLineOffset = 0;
            int deltaLineOffsetSigned = 0;
            unsigned int deltaPositionOffset = 0;
            int deltaPositionOffsetSigned = 0;

            if (!TryRead7BitEncodedInt(pLineBuffer, lineBufferSize, &currentBufferOffset, &deltaStreamOffset))
            {
                break;
            }
            if (!TryRead7BitEncodedInt(pLineBuffer, lineBufferSize, &currentBufferOffset, &deltaLineOffset))
            {
                break;
            }
            if (!TryRead7BitEncodedInt(pLineBuffer, lineBufferSize, &currentBufferOffset, &deltaPositionOffset))
            {
                break;
            }

            // we are past the requested entry, then use the last read line information
            if (currentStreamOffset + deltaStreamOffset > streamOffset)
            {
                break;
            }

            currentStreamOffset += static_cast<int>(deltaStreamOffset);
            deltaLineOffsetSigned = (deltaLineOffset % 2 == 0 ? deltaLineOffset / 2 : ((deltaLineOffset + 1) / 2));
            deltaLineOffsetSigned = (deltaLineOffset % 2 == 0 ? deltaLineOffsetSigned : -deltaLineOffsetSigned);
            currentLineOffset += static_cast<int>(deltaLineOffsetSigned);
            deltaPositionOffsetSigned = (deltaPositionOffset % 2 == 0 ? deltaPositionOffset / 2 : ((deltaPositionOffset + 1) / 2));
            deltaPositionOffsetSigned = (deltaPositionOffset % 2 == 0 ? deltaPositionOffsetSigned : -deltaPositionOffsetSigned);
            currentPositionOffset += static_cast<int>(deltaPositionOffsetSigned);
        }

        // LOG(L"ReaderLineInfo: %d %d %d", streamOffset, currentLineOffset, currentPositionOffset);
        return XamlLineInfo(currentLineOffset, currentPositionOffset);
    }
    return XamlLineInfo();
}
#pragma endregion

#pragma region ObjectWriterNode Decoders
ObjectWriterNode XamlBinaryFormatSubReader2::ReadPushScopeAddNamespaceNode()
{
    xstring_ptr spPrefixString;
    std::shared_ptr<XamlNamespace> spXamlNamespace;
    ReadNamespace(spXamlNamespace, &spPrefixString);
    return ObjectWriterNode::MakePushScopeAddNamespaceNode(GetLineInfo(), spPrefixString, spXamlNamespace);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadAddNamespaceNode()
{
    xstring_ptr spPrefixString;
    std::shared_ptr<XamlNamespace> spXamlNamespace;
    ReadNamespace(spXamlNamespace, &spPrefixString);
    return ObjectWriterNode::MakeAddNamespaceNode(GetLineInfo(), spPrefixString, spXamlNamespace);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadAddToDictionaryWithKeyNode()
{
    auto qoConst = ReadConstantAsQO();
    return ObjectWriterNode::MakeAddToDictionaryWithKeyNode(GetLineInfo(), qoConst);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueNode()
{
    auto prop = ReadXamlProperty();
    return ObjectWriterNode::MakeSetValueNode(GetLineInfo(), prop);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueFromMarkupExtensionNode()
{
    auto prop = ReadXamlProperty();
    return ObjectWriterNode::MakeSetValueFromMarkupExtensionNode(GetLineInfo(), prop);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetDeferredPropertyNode()
{
    std::shared_ptr<XamlProperty> property = ReadXamlProperty();
    unsigned int targetStreamIndex = ReadUInt();

    std::shared_ptr<XamlBinaryFormatSubReader2> subReader;
    THROW_IF_FAILED(m_spXamlBinaryFormatMasterReader->GetSubReader(targetStreamIndex, subReader));

    unsigned int staticResourceCount = ReadUInt();
    unsigned int themeResourceCount = ReadUInt();

    std::vector<std::pair<bool,xstring_ptr>> vecResourceList;
    vecResourceList.reserve(staticResourceCount+themeResourceCount);

    for (unsigned int i = 0; i < staticResourceCount; i++)
    {
        xstring_ptr strValue = ReadSharedString();
        vecResourceList.emplace_back(true, std::move(strValue));
    }
    for (unsigned int i = 0; i < themeResourceCount; i++)
    {
        xstring_ptr strValue = ReadSharedString();
        vecResourceList.emplace_back(false, std::move(strValue));
    }

    return ObjectWriterNode::MakeSetDeferredPropertyNodeWithReader(GetLineInfo(), property, subReader, std::move(vecResourceList));
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetCustomRuntimeDataNode()
{
    unsigned int targetStreamIndex = ReadUInt();

    std::shared_ptr<XamlBinaryFormatSubReader2> subReader;
    THROW_IF_FAILED(m_spXamlBinaryFormatMasterReader->GetSubReader(targetStreamIndex, subReader));
    auto subObjectWriterResult = std::unique_ptr<SubObjectWriterResult>(new SubObjectWriterResult(subReader));

    std::shared_ptr<CustomWriterRuntimeData> customRuntimeData;

    // Attempt to look for an already-deserialized version of the CustomRuntimeData. If found we'll
    // simply push the nodestream forward and return this shared instance.
    unsigned int prePosition = m_currentNodeStreamOffset;
    unsigned int masterRelativeOffset = prePosition + m_nodeStreamMasterOffset;

    auto cachedRuntimeData = m_spXamlBinaryFormatMasterReader->TryGetRuntimeData(masterRelativeOffset);
    if (cachedRuntimeData)
    {
        m_currentNodeStreamOffset += (*cachedRuntimeData).first;
        customRuntimeData = (*cachedRuntimeData).second;
    }
    else
    {
        {
            // No longer actively used in XBFv2. This block of code will merely skip through the static and theme
            // preresolved resources and push the stream forward. The current XBFv2 implemention will simply emit
            // zeros into the stream for these counts and we keep this logic to stay compatible with what we released
            // in Threshold M1.
            XUINT32 staticResourceCount = ReadUInt();
            XUINT32 themeResourceCount = ReadUInt();
            for (unsigned int i = 0; i < staticResourceCount + themeResourceCount; i++)
            {
                // Strings are represented as a single XamlNode2 in the ObjectStream.
                PersistedXamlNode2 sPersistedXamlNode = ReadPersistedXamlNode();
            }
        }
        std::unique_ptr<CustomWriterRuntimeData> uniqueRuntimeData;

        THROW_IF_FAILED(CustomWriterRuntimeData::Deserialize(this, uniqueRuntimeData));
        customRuntimeData = std::move(uniqueRuntimeData);

        if (customRuntimeData->ShouldShare())
        {
            m_spXamlBinaryFormatMasterReader->SetRuntimeData(masterRelativeOffset,
                std::make_pair(m_currentNodeStreamOffset - prePosition, customRuntimeData));
        }
    }

    return ObjectWriterNode::MakeSetCustomRuntimeData(GetLineInfo(), std::move(customRuntimeData), std::move(subObjectWriterResult));
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadPushScopeGetValueNode()
{
    auto property = ReadXamlProperty();
    return ObjectWriterNode::MakePushScopeGetValueNode(GetLineInfo(), property);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadPushConstantNode()
{
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakePushConstantNode(GetLineInfo(), constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetConnectionIdNode()
{
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeSetConnectionIdNode(GetLineInfo(), constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetNameNode()
{
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeSetNameNode(GetLineInfo(), constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadGetResourcePropertyBagNode()
{
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeGetResourcePropertyBagNode(GetLineInfo(), constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueConstantNode()
{
    auto property = ReadXamlProperty();
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeSetValueConstantNode(GetLineInfo(), property, constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadProvideStaticResourceValueNode()
{
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeProvideStaticResourceValueNode(GetLineInfo(), constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueFromStaticResourceNode()
{
    auto property = ReadXamlProperty();
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeSetValueFromStaticResourceNode(GetLineInfo(), property, constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadProvideThemeResourceValueNode()
{
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeProvideThemeResourceValueNode(GetLineInfo(), constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueFromThemeResourceNode()
{
    auto property = ReadXamlProperty();
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeSetValueFromThemeResourceNode(GetLineInfo(), property, constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueFromTemplateBindingNode()
{
    auto property = ReadXamlProperty();
    auto propertyProxy = ReadXamlProperty();
    return ObjectWriterNode::MakeSetValueFromTemplateBindingNode(GetLineInfo(), property, std::move(propertyProxy));
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueTypeConvertedConstantNode()
{
    std::shared_ptr<XamlProperty> property = ReadXamlProperty();
    auto constant = ReadConstantAsQO();

    std::shared_ptr<XamlTextSyntax> spConverter;
    // Property might be unknown if it was conditionally declared or declaring type was conditionally declared,
    // and neither could be resolved at runtime. This will be caught later by the binaryformatobjectwriter,
    // so goal here is to avoid crashing when deserializing the node.
    if (!property->IsUnknown())
    {
        THROW_IF_FAILED(property->get_TextSyntax(spConverter));
    }
    return ObjectWriterNode::MakeSetValueTypeConvertedConstantNode(GetLineInfo(), property, spConverter, constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueTypeConvertedResolvedTypeNode()
{
    auto property = ReadXamlProperty();
    auto type = ReadXamlType();
    std::shared_ptr<XamlTextSyntax> converter;
    THROW_IF_FAILED(property->get_TextSyntax(converter));
    return ObjectWriterNode::MakeSetValueTypeConvertedResolvedTypeNode(GetLineInfo(),property, converter, type);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadSetValueTypeConvertedResolvedPropertyNode()
{
    auto property = ReadXamlProperty();
    auto propertyProxy = ReadXamlProperty();
    std::shared_ptr<XamlTextSyntax> converter;
    THROW_IF_FAILED(property->get_TextSyntax(converter));

    return ObjectWriterNode::MakeSetValueTypeConvertedResolvedPropertyNode(GetLineInfo(), property, converter, propertyProxy);
}


ObjectWriterNode XamlBinaryFormatSubReader2::ReadPushScopeCreateTypeWithConstantBeginInitNode()
{
    auto type = ReadXamlType();
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakePushScopeCreateTypeWithConstantBeginInitNode(GetLineInfo(), type, constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadPushScopeCreateTypeWithTypeConvertedConstantBeginInitNode()
{
    auto type = ReadXamlType();
    auto constant = ReadConstantAsQO();
    std::shared_ptr<XamlTextSyntax> converter;
    THROW_IF_FAILED(type->get_TextSyntax(converter));
    return ObjectWriterNode::MakePushScopeCreateTypeWithTypeConvertedConstantBeginInitNode(GetLineInfo(), type, converter, constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadCreateTypeWithConstantBeginInitNode()
{
    auto type = ReadXamlType();
    auto constant = ReadConstantAsQO();
    return ObjectWriterNode::MakeCreateTypeWithConstantBeginInitNode(GetLineInfo(), type, constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadCreateTypeWithTypeConvertedConstantBeginInitNode()
{
    auto type = ReadXamlType();
    auto constant = ReadConstantAsQO();
    std::shared_ptr<XamlTextSyntax> converter;
    THROW_IF_FAILED(type->get_TextSyntax(converter));
    return ObjectWriterNode::MakeCreateTypeWithTypeConvertedConstantBeginInitNode(GetLineInfo(), type, converter, constant);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadPushScopeCreateTypeBeginInitNode()
{
    auto type = ReadXamlType();
    return ObjectWriterNode::MakePushScopeCreateTypeBeginInitNode(GetLineInfo(), type);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadCreateTypeBeginInitNode()
{
    auto type = ReadXamlType();
    return ObjectWriterNode::MakeCreateTypeBeginInitNode(GetLineInfo(), type);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadMakeCheckPeerTypeNode()
{
    xstring_ptr strPeerName = ReadPersistedString();
    return ObjectWriterNode::MakeCheckPeerTypeNode(GetLineInfo(), strPeerName);
}

ObjectWriterNode XamlBinaryFormatSubReader2::ReadBeginConditionalScopeNode()
{
    auto predicateType = ReadXamlType();
    auto arguments = ReadSharedString();

    return ObjectWriterNode::MakeBeginConditionalScope(
        GetLineInfo(), std::make_shared<Parser::XamlPredicateAndArgs>(predicateType, arguments));
}

#pragma endregion

#pragma region Primitive Type Decoders
bool XamlBinaryFormatSubReader2::TryReadNodeType(
    _Out_ ObjectWriterNodeType& nodeType)
{
    return TryReadFromNodeStreamBuffer(&nodeType);
}

xstring_ptr XamlBinaryFormatSubReader2::ReadPersistedString()
{
    UINT32 cch = 0;
    THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(&cch));
    THROW_HR_IF(E_UNEXPECTED, cch >= (XUINT32_MAX / sizeof(WCHAR)));

    WCHAR* pch = nullptr;
    XStringBuilder bufferBuilder;
    THROW_IF_FAILED(bufferBuilder.InitializeAndGetFixedBuffer(cch, &pch));
    THROW_HR_IF(E_FAIL, !TryReadArrayFromNodeStreamBuffer(pch, cch));

    xstring_ptr returnString;
    THROW_IF_FAILED(bufferBuilder.DetachString(&returnString));
    return returnString;
}

xstring_ptr XamlBinaryFormatSubReader2::ReadSharedString()
{
    PersistedXamlNode2 sPersistedXamlNode = ReadPersistedXamlNode();
    xstring_ptr returnString;
    THROW_IF_FAILED(m_spXamlBinaryFormatMasterReader->GetMetadataReader().GetString(sPersistedXamlNode, returnString));
    return returnString;
}

void XamlBinaryFormatSubReader2::ReadNamespace(
    _Out_ std::shared_ptr<XamlNamespace>& spNamespace,
    _Out_ xstring_ptr *pStrPrefixValue)
{
    PersistedXamlNode2 sPersistedXamlNode = ReadPersistedXamlNode();
    *pStrPrefixValue = ReadPersistedString();
    HRESULT hr = m_spXamlBinaryFormatMasterReader->GetMetadataReader().GetXmlNamespace(sPersistedXamlNode, spNamespace);
    if (hr == S_FALSE) THROW_HR(E_FAIL);
    if (!SUCCEEDED(hr)) THROW_HR(hr);
}

std::shared_ptr<XamlQualifiedObject> XamlBinaryFormatSubReader2::ReadConstantAsQO()
{
    // Note we're using this perallocated XQO...
    auto returnQO = m_spTemporaryValue;
    CValue value = ReadCValue();
    THROW_IF_FAILED(returnQO->SetValue(value));
    return returnQO;
}

CValue XamlBinaryFormatSubReader2::ReadCValue()
{
    PersistedConstantType constantType = ReadConstantNodeType();
    CValue retValue;

    switch (constantType)
    {
        case PersistedConstantType::IsEnum:
        {
            Parser::StableXbfTypeIndex stableIndex = Parser::StableXbfTypeIndex::UnknownType;
            THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(&stableIndex));
            ASSERT(stableIndex != Parser::StableXbfTypeIndex::UnknownType);
            KnownTypeIndex typeIndex = Parser::GetKnownTypeIndex(stableIndex);
            XUINT32 unsignedValue = 0;
            THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(&unsignedValue));
            if (DirectUI::MetadataAPI::GetClassInfoByIndex(typeIndex)->IsCompactEnum())
            {
                ASSERT(unsignedValue <= UINT8_MAX);
                retValue.SetEnum8(static_cast<uint8_t>(unsignedValue), typeIndex);
            }
            else
            {
                retValue.SetEnum(unsignedValue, typeIndex);
            }
        }
        break;

        case PersistedConstantType::IsNullString:
        {
            retValue.SetString(xstring_ptr::NullString());
        }
        break;

        case PersistedConstantType::IsBoolFalse:
        {
            retValue.SetBool(FALSE);
        }
        break;

        case PersistedConstantType::IsBoolTrue:
        {
            retValue.SetBool(TRUE);
        }
        break;

        case PersistedConstantType::IsColor:
        {
            XUINT32 colorValue = 0;
            THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(&colorValue));
            retValue.SetColor(colorValue);
        }
        break;

        case PersistedConstantType::IsFloat:
        {
            XFLOAT floatValue = 0;
            THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(&floatValue));
            retValue.SetFloat(floatValue);
        }
        break;

        case PersistedConstantType::IsGridLength:
        {
            auto gridLength = std::unique_ptr<XGRIDLENGTH>(new XGRIDLENGTH);
            THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(gridLength.get()));
            retValue.SetGridLength(gridLength.release());
        }
        break;

        case PersistedConstantType::IsSharedString:
        {
            xstring_ptr strValue = ReadSharedString();
            retValue.SetString(std::move(strValue));
        }
        break;

        case PersistedConstantType::IsSigned:
        {
            XINT32 signedValue = 0;
            THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(&signedValue));
            retValue.SetSigned(signedValue);
        }
        break;

        case PersistedConstantType::IsThickness:
        {
            auto thickness = std::unique_ptr<XTHICKNESS>(new XTHICKNESS);
            THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(thickness.get()));
            retValue.SetThickness(thickness.release());
        }
        break;

        case PersistedConstantType::IsUniqueString:
        {
            xstring_ptr strValue = ReadPersistedString();
            retValue.SetString(std::move(strValue));
        }
        break;

        default:
        {
            ASSERT(false);
        }
        break;
    }

    return retValue;
}

PersistedConstantType XamlBinaryFormatSubReader2::ReadConstantNodeType()
{
    PersistedConstantType nodeType;
    THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(&nodeType));
    return nodeType;
}

std::shared_ptr<XamlProperty> XamlBinaryFormatSubReader2::ReadXamlProperty()
{
    PersistedXamlNode2 sPersistedXamlNode = ReadPersistedXamlNode();

    std::shared_ptr<XamlProperty> spProperty;
    HRESULT hr = m_spXamlBinaryFormatMasterReader->GetMetadataReader().GetProperty(sPersistedXamlNode, spProperty);
    if (hr == S_FALSE) THROW_HR(E_FAIL);
    if (!SUCCEEDED(hr)) THROW_HR(hr);
    return spProperty;
}

std::shared_ptr<XamlType> XamlBinaryFormatSubReader2::ReadXamlType()
{
    PersistedXamlNode2 sPersistedXamlNode = ReadPersistedXamlNode();

    std::shared_ptr<XamlType> spType;
    HRESULT hr = m_spXamlBinaryFormatMasterReader->GetMetadataReader().GetType(sPersistedXamlNode, spType);
    if (hr == S_FALSE) THROW_HR(E_FAIL);
    if (!SUCCEEDED(hr)) THROW_HR(hr);
    return spType;
}

PersistedXamlNode2 XamlBinaryFormatSubReader2::ReadPersistedXamlNode()
{
    PersistedXamlNode2 targetNode;
    THROW_HR_IF(E_FAIL, !TryReadFromNodeStreamBuffer(&targetNode));
    return targetNode;
}

unsigned int XamlBinaryFormatSubReader2::ReadUInt()
{
    unsigned int returnValue = 0;
    THROW_HR_IF(E_FAIL,
        !TryRead7BitEncodedInt(m_nodeStream, m_nodeStreamLength, &m_currentNodeStreamOffset, &returnValue));
    return returnValue;
}

bool XamlBinaryFormatSubReader2::TryRead7BitEncodedInt(
    _In_ XUINT8 *pBuffer,
    _In_ XUINT32 cbBufferTotalSize,
    _Inout_ XUINT32 *pcbBufferOffset,
    _Out_ unsigned int *pValue)
{
    static const unsigned int Bit8 = 0x00000080;    // 10000000
    static const unsigned int BitMask = 0x0000007F; // 01111111

    unsigned int value = 0;
    unsigned int shift = 0;
    unsigned char currentByte = 0;

    do
    {
        ASSERT(shift != 5 * 7);
        if (!TryReadFromBuffer(pBuffer, cbBufferTotalSize, sizeof(XUINT8), &currentByte, pcbBufferOffset))
        {
            return false;
        }

        value |= (currentByte & BitMask) << shift;
        shift += 7;
    } while (currentByte & Bit8);

    *pValue = value;
    return true;
}
#pragma endregion

const XamlLineInfo XamlBinaryFormatSubReader2::GetLineInfo()
{
    if (m_lineStream)
    {
        return XamlLineInfo(this, m_lastNodeStreamOffset);
    }
    else
    {
        return XamlLineInfo();
    }
}

bool XamlBinaryFormatSubReader2::TryReadFromBuffer(
    _In_ XUINT8 *pBuffer,
    _In_ XUINT32 cbBufferTotalSize,
    _In_ XUINT32 cbBufferReadSize,
    _Out_writes_bytes_(cbBufferReadSize) void* pbTarget,
    _Inout_ XUINT32 *pcbBufferOffset)
{
    if (*pcbBufferOffset + cbBufferReadSize > cbBufferTotalSize)
    {
        return false;
    }

    // these are going to be small reads, so memcpy would be overkill.
    const XUINT8 *pbRead = pBuffer + *pcbBufferOffset;
    std::copy_n(pbRead, cbBufferReadSize, static_cast<XUINT8*>(pbTarget));

    *pcbBufferOffset += cbBufferReadSize;
    return true;
}

const Parser::XamlBinaryFileVersion& XamlBinaryFormatSubReader2::GetVersion() const
{
    return m_spXamlBinaryFormatMasterReader->GetMetadataReader().GetVersion();
}
