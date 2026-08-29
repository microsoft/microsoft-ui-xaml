// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MemoryStreamBuffer.h"
#include "XamlBinaryMetadataStore.h"
#include "XamlTypeNamespace.h"
#include "XamlSerializationHelper.h"
#include "MetadataAPI.h"
#include "XbfMetadataApi.h"
#include "WinBluePropertyTypeCompatHelper.h"

using namespace Parser;

// BIG TODO: The MetadataStore needs to be refactored better to handle v1 and v2.

_Check_return_ HRESULT
XamlBinaryMetadataStore::WriteMetadata(_In_ IPALStream *pStream, _In_ const std::array<byte, Parser::c_xbfHashSize>& byteHashForBinaryXaml)
{
    // Create a temporary buffer/stream to hold the metadata so we can figure out the proper
    // offsets to store in the header
    // We need to do this because the XBF writer is passed a CMemoryStreamBufferStream, which doesn't
    // implement seeking
    xref_ptr<IPALStream> spTempMetadataStream;
    xref_ptr<CMemoryStreamBuffer> spTempMetadataBuffer;
    IFC_RETURN(spTempMetadataBuffer.init(new CMemoryStreamBuffer(static_cast<XUINT32>(-1))));
    
    IFC_RETURN(spTempMetadataBuffer->CreateStream(spTempMetadataStream.ReleaseAndGetAddressOf()));

    {
        size_t cchHashBuffer = byteHashForBinaryXaml.size();

        // Account for the binary format hash provided - we expect it to be 64 bytes long
        IFCEXPECT_RETURN(cchHashBuffer == Parser::c_xbfHashSize);
        
        if (GetVersion() == Parser::XbfV1)
        {
            std::copy(byteHashForBinaryXaml.begin(), byteHashForBinaryXaml.end(), m_header.m_byteHashBuffer);
        }
        else if (GetVersion().m_uMajorBinaryFileVersion == 2)
        {
            std::copy(byteHashForBinaryXaml.begin(), byteHashForBinaryXaml.end(), m_header2.m_byteHashBuffer);
        }
    }
    
    // Lay down the (non-final) header so our offsets are correct
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToMetadataStream(GetVersion(), spTempMetadataStream.get()));
    if (GetVersion() == Parser::XbfV1)
    {
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToMetadataStream(m_header, GetVersion(), spTempMetadataStream.get()));
    }
    else if (GetVersion().m_uMajorBinaryFileVersion == 2)
    {
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToMetadataStream(m_header2, GetVersion(), spTempMetadataStream.get()));
    }

    XUINT64 uMetadataStart = 0;
    IFC_RETURN(spTempMetadataStream->GetSize(&uMetadataStart));
    
    // Write the metadata to the temp stream
    IFC_RETURN(WriteMetadataBody(spTempMetadataStream.get()));
    
    // Read from temp stream into buffer in preparation for actual write
    XUINT64 uSize = 0;
    IFC_RETURN(spTempMetadataStream->GetSize(&uSize));
    if (uSize > XUINT32_MAX)
    {
        // This is more about about validating that we can safely fit a 64-bit int 
        // (stream size) into a 32-bit int (read/write size) than it is about setting 
        // a max size on the metadata. A metadata this large certainly isn't
        // reasonable, but we don't actually have any limits on XBF size that we enforce
        IFC_RETURN(E_FAIL);
    }
    XUINT32 uBytesRead = 0;
    std::vector<XBYTE> metadataBuffer;
    metadataBuffer.resize(static_cast<size_t>(uSize));
    IFC_RETURN(spTempMetadataStream->Read(metadataBuffer.data(), static_cast<XUINT32>(uSize), &uBytesRead));
    if (uBytesRead != static_cast<XUINT32>(uSize))
    {
        IFC_RETURN(E_FAIL);
    }
    
    // Write the final metadata
    XUINT32 uBytesWritten = 0;
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToMetadataStream(GetVersion(), pStream));
    if (GetVersion() == Parser::XbfV1)
    {
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToMetadataStream(m_header, GetVersion(), pStream));
    }
    else if (GetVersion().m_uMajorBinaryFileVersion == 2)
    {
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeItemToMetadataStream(m_header2, GetVersion(), pStream));
    }
    IFC_RETURN(pStream->Write(metadataBuffer.data() + static_cast<XUINT32>(uMetadataStart), uBytesRead - static_cast<XUINT32>(uMetadataStart), 0, &uBytesWritten));
    if (uBytesWritten != (uBytesRead - static_cast<XUINT32>(uMetadataStart)))
    {
        IFC_RETURN(E_FAIL);
    }
    
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataStore::WriteMetadataBody(_In_ IPALStream *pStream)
{
    if (GetVersion() == Parser::XbfV1)
    {
        IFC_RETURN(pStream->GetSize(&(m_header.m_uStringTableOffset)));
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecStringTable, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&(m_header.m_uAssemblyListOffset)));
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterAssemblyList, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&(m_header.m_uTypeNamespaceListOffset)));
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterTypeNamespaceList, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&(m_header.m_uTypeListOffset)));
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterTypeList, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&(m_header.m_uPropertyListOffset)));
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterPropertyList, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&(m_header.m_uXmlNamespaceListOffset)));
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterXmlNamespaceList, GetVersion(), pStream));
    }
    else if (GetVersion().m_uMajorBinaryFileVersion == 2)
    {
        XUINT64 size;
        IFC_RETURN(pStream->GetSize(&size));
        m_header2.m_uStringTableOffset = static_cast<XUINT32>(size);
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecStringTable, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&size));
        m_header2.m_uAssemblyListOffset = static_cast<XUINT32>(size);
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterAssemblyList, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&size));
        m_header2.m_uTypeNamespaceListOffset = static_cast<XUINT32>(size);
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterTypeNamespaceList, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&size));
        m_header2.m_uTypeListOffset = static_cast<XUINT32>(size);
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterTypeList, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&size));
        m_header2.m_uPropertyListOffset = static_cast<XUINT32>(size);
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterPropertyList, GetVersion(), pStream));

        IFC_RETURN(pStream->GetSize(&size));
        m_header2.m_uXmlNamespaceListOffset = static_cast<XUINT32>(size);
        IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToMetadataStream(m_vecMasterXmlNamespaceList, GetVersion(), pStream));
    }

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataStore::GetXamlAssemblyId(const std::shared_ptr<XamlAssembly>& inXamlAssembly, _Out_ XUINT32& ruiXamlAssemblyId)
{
    HRESULT hr = S_OK;
    
    IFC(m_mapMasterAssemblyList.Get(inXamlAssembly->get_AssemblyToken(), ruiXamlAssemblyId));
    if (hr == S_FALSE)
    {
        PersistedXamlAssembly persistedXamlAssembly;
        xstring_ptr spAssemblyName;

        ruiXamlAssemblyId = m_vecMasterAssemblyList.size();

        IFC(inXamlAssembly->get_Name(&spAssemblyName));
        IFC(GetStringId(spAssemblyName, persistedXamlAssembly.m_uiAssemblyName));
        persistedXamlAssembly.m_eTypeInfoProviderKind = inXamlAssembly->GetTypeInfoProviderKind();
        IFC(m_vecMasterAssemblyList.push_back(persistedXamlAssembly));

        IFC(m_mapMasterAssemblyList.Add(inXamlAssembly->get_AssemblyToken(), ruiXamlAssemblyId));
    }
        
Cleanup:
    if (FAILED(hr))
    {
        ruiXamlAssemblyId = 0;
    }
    
    RRETURN(hr);
}


_Check_return_ HRESULT 
XamlBinaryMetadataStore::GetXamlTypeNamespaceId(const std::shared_ptr<XamlNamespace>& inXamlNamespace, _Out_ XUINT32& ruiXamlNamespace)
{
    HRESULT hr = S_OK;
    // TODO: !!!! is there any way to guarantee this (like typing the XamlType's Namespace to XamlTypeNamespace?
    std::shared_ptr<XamlTypeNamespace> spTypeNamespace = std::static_pointer_cast<XamlTypeNamespace>(inXamlNamespace);
    
    IFC(m_mapMasterTypeNamespaceList.Get(spTypeNamespace->get_TypeNamespaceToken(), ruiXamlNamespace));
    if (hr == S_FALSE)
    {
        ruiXamlNamespace = m_vecMasterTypeNamespaceList.size();
        PersistedXamlTypeNamespace persistedTypeNamespace;

        std::shared_ptr<XamlAssembly> spAssembly;
        IFC(spTypeNamespace->GetOwningAssembly(spAssembly));
        xstring_ptr spNamespaceUri = spTypeNamespace->get_TargetNamespace();
        
        IFC(GetXamlAssemblyId(spAssembly, persistedTypeNamespace.m_uiAssembly));
        IFC(GetStringId(spNamespaceUri, persistedTypeNamespace.m_uiNamespaceName));
        
        IFC(m_mapMasterTypeNamespaceList.Add(spTypeNamespace->get_TypeNamespaceToken(), ruiXamlNamespace));
        IFC(m_vecMasterTypeNamespaceList.push_back(persistedTypeNamespace));
    }

Cleanup:
    ASSERTSUCCEEDED(hr);
    if (FAILED(hr))
    {
        ruiXamlNamespace = 0;
    }

    RRETURN(hr);
}

_Check_return_ HRESULT 
XamlBinaryMetadataStore::GetXamlTypeNode(const std::shared_ptr<XamlType>& inXamlType, _Out_ PersistedXamlNode& xamlNode)
{
    xamlNode.m_uiObjectId = 0;

    xamlNode.m_NodeFlags.ClearBit(PersistedXamlNode::IsTrustedXbfIndex);
    IFC_RETURN(GetXamlTypeId(inXamlType, xamlNode.m_uiObjectId));

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataStore::GetXamlTypeNode(const std::shared_ptr<XamlType>& inXamlType, _Out_ PersistedXamlNode2& xamlNode)
{
    xamlNode.m_uiObjectId = 0;

    const auto& typeToken = inXamlType->get_TypeToken();

    if (GetVersion().m_uMajorBinaryFileVersion == 2 && IsStableXbfType(typeToken, GetTargetOSVersion()))
    {
        // We can depend on this type to be resolvable when we load up the XBF later
        // Skip caching all the metadata and lookup, and just set the stable/trusted index
        xamlNode.m_bIsTrusted = TRUE;
        auto stableTypeIndex = GetStableXbfTypeIndex(typeToken.GetHandle());
        xamlNode.m_uiObjectId = static_cast<UINT16>(stableTypeIndex);
    }
    else
    {
        XUINT32 id = 0;
        xamlNode.m_bIsTrusted = FALSE;
        IFC_RETURN(GetXamlTypeId(inXamlType, id));
        xamlNode.m_uiObjectId = static_cast<UINT16>(id);
    }

    return S_OK;
}

// We want to make sure Threshold generates the same type names as WinBlue. Add additional difference handling here.
_Check_return_ HRESULT XamlBinaryMetadataStore::GetWinBlueTypeName(_In_ const XamlTypeToken& inTypeToken, _Out_ xstring_ptr* pstrRetVal)
{
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strGroupStyleCollectionTypeName, L"GroupStyleCollection");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strVisualStateGroupCollectionTypeName, L"!VisualStateGroupCollection");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strVisualStateCollectionTypeName, L"!VisualStateCollection");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strVisualTransitionCollectionTypeName, L"!VisualTransitionCollection");
    xstring_ptr spTypeName;
    switch (inTypeToken.GetHandle())
    {
        case KnownTypeIndex::IObservableVectorOfGroupStyle:
            spTypeName = c_strGroupStyleCollectionTypeName;
            break;
        case KnownTypeIndex::VisualStateCollection:
            spTypeName = c_strVisualStateCollectionTypeName;
            break;
        case KnownTypeIndex::VisualStateGroupCollection:
            spTypeName = c_strVisualStateGroupCollectionTypeName;
            break;
        case KnownTypeIndex::VisualTransitionCollection:
            spTypeName = c_strVisualTransitionCollectionTypeName;
            break;
    }

    *pstrRetVal = spTypeName;
    
    return S_OK;
}

// Returns an ID for untrusted types (i.e. do not have a corresponding StableXbfTypeIndex)
_Check_return_ HRESULT XamlBinaryMetadataStore::GetXamlTypeId(const std::shared_ptr<XamlType>& inXamlType, _Out_ XUINT32& ruiXamlTypeId)
{
    ruiXamlTypeId = 0;

    XUINT32 interimTypeId;
    std::shared_ptr<XamlNamespace> spXamlNamespace;
    xstring_ptr spTypeName;
    bool isUnknown = inXamlType->IsUnknown();
    bool found = false;

    if (isUnknown)
    {
        IFC_RETURN(inXamlType->GetXamlNamespace(spXamlNamespace));
        IFC_RETURN(inXamlType->get_Name(&spTypeName));

        auto result = m_mapMasterUnknownTypeList.find(std::make_pair(spXamlNamespace->get_TargetNamespace(), spTypeName));
        if (result != m_mapMasterUnknownTypeList.end())
        {
            found = true;
            interimTypeId = result->second;
        }
    }
    else
    {
        auto result = m_mapMasterKnownTypeList.find(inXamlType->get_TypeToken());
        if (result != m_mapMasterKnownTypeList.end())
        {
            found = true;
            interimTypeId = result->second;
        }
    }
    
    if (!found)
    {
        interimTypeId = m_vecMasterTypeList.size();
        PersistedXamlType persistedXamlType;
        WCHAR chFirst = '\0';
        bool fIsDirective = false;

        // Look up namespace and type name for known types
        // (we already got that information for unknown types earlier)
        if (!isUnknown)
        {
            IFC_RETURN(inXamlType->GetXamlNamespace(spXamlNamespace));

            IFC_RETURN(GetWinBlueTypeName(inXamlType->get_TypeToken(), &spTypeName));

            if (spTypeName.IsNullOrEmpty())
            {
                IFC_RETURN(inXamlType->get_Name(&spTypeName));
            }

            IFC_RETURN(inXamlType->IsDirective(fIsDirective));

            chFirst = spTypeName.GetChar(0);
            if (chFirst == '.' || chFirst == '?')
            {
                IFC_RETURN(spTypeName.SubString(1, spTypeName.GetCount(), &spTypeName));
            }
        }

        IFC_RETURN(GetStringId(spTypeName, persistedXamlType.m_uiTypeName));


        if (fIsDirective)
        {
            // If the type is a directive type, then it doesn't necessarily have a TypeNamespace, so
            // setting this flag will indicate to the deserializer that the there is no Type Namespace to 
            // look up, but rather the type name should be looked up as a Directive type from the
            // XamlSchemaContext.
            persistedXamlType.m_TypeFlags.SetBit(PersistedXamlType::IsMarkupDirective);
        }
        else if (isUnknown || GetTargetOSVersion() >= Parser::Versioning::OSVersions::WIN10_RS2)
        {
            // Marking the PersistedXamlType as Unknown indicates that when deserializing we need to resolve it slightly
            // differently (i.e. deserialize a XamlXmlNamespace and use that to resolve the type name)
            //
            // MSFT:16178797
            // There was an annoying change to the way UWP apps are built that results in GenXbf being presented with platform
            // metadata corresponding to TPV rather than TPMV. This means that types which were added after TPMV will
            // appear to be "known". This will cause problems later during deserialization when run on TPMV because
            // those "known" types will fail to be resolved and the framework will balk since the type failed to resolve but it
            // wasn't serialized as an unknown type (this check occurs during deserialization, before the parser is even fed the node
            // referencing the type).
            // We'll work around this during generation by saying that *every* untrusted type is "unknown", thus bypassing the check
            // during deserialization. Later stages in the parser pipeline will catch if the type is truly unknown and the associated node
            // wasn't protected by conditional XAML. Note that this only applies to TPMV >= RS2, since that is when conditional XAML was introduced.
            persistedXamlType.m_TypeFlags.SetBit(PersistedXamlType::IsUnknown);
            IFC_RETURN(GetXamlXmlNamespaceId(spXamlNamespace->get_OriginalXamlXmlNamespace(), persistedXamlType.m_uiTypeNamespace));
        }
        else
        {
            IFC_RETURN(GetXamlTypeNamespaceId(spXamlNamespace, persistedXamlType.m_uiTypeNamespace));
        }

        if (isUnknown)
        {
            m_mapMasterUnknownTypeList.emplace(
                std::make_pair(spXamlNamespace->get_TargetNamespace(), spTypeName), 
                interimTypeId);
        }
        else
        {
            m_mapMasterKnownTypeList.emplace(inXamlType->get_TypeToken(), interimTypeId);
        }

        IFC_RETURN(m_vecMasterTypeList.push_back(persistedXamlType));
    }

    ruiXamlTypeId = interimTypeId;
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataStore::GetXamlPropertyNode(const std::shared_ptr<XamlProperty>& inXamlProperty, _In_ const WinBluePropertyTypeCompatHelper& winBluePropertyTypeCompatHelper, _Out_ PersistedXamlNode& xamlNode)
{
    xamlNode.m_uiObjectId = 0;

    xamlNode.m_NodeFlags.ClearBit(PersistedXamlNode::IsTrustedXbfIndex);
    IFC_RETURN(GetXamlPropertyId(inXamlProperty, winBluePropertyTypeCompatHelper, xamlNode.m_uiObjectId));

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataStore::GetXamlPropertyNode(const std::shared_ptr<XamlProperty>& inXamlProperty, _Out_ PersistedXamlNode2& xamlNode)
{
    xamlNode.m_uiObjectId = 0;

    const auto& propertyToken = inXamlProperty->get_PropertyToken();

    if (GetVersion().m_uMajorBinaryFileVersion == 2 && IsStableXbfProperty(propertyToken, GetTargetOSVersion()))
    {
        // We can depend on this property to be resolvable when we load up the XBF later
        // Skip caching all the metadata and lookup, and just set the stable/trusted index
        xamlNode.m_bIsTrusted = TRUE;
        auto stablePropertyIndex = GetStableXbfPropertyIndex(propertyToken.GetHandle());
        xamlNode.m_uiObjectId = static_cast<UINT16>(stablePropertyIndex);
    }
    else
    {
        XUINT32 id = 0;
        xamlNode.m_bIsTrusted = FALSE;
        // The WinBluePropertyTypeCompatHelper object passed in the below function is no-op because the its internal m_spCurrentXamlOjbectType is not initalized,
        // so that re-mapping will not occur in WinBluePropertyTypeCompatHelper::GetWinBlueDeclaringTypeForProperty().
        IFC_RETURN(GetXamlPropertyId(inXamlProperty, WinBluePropertyTypeCompatHelper(), id));
        xamlNode.m_uiObjectId = static_cast<UINT16>(id);
    }

    return S_OK;
}

// Returns an ID for untrusted properties (i.e. do not have a corresponding StableXbfPropertyIndex)
_Check_return_ HRESULT XamlBinaryMetadataStore::GetXamlPropertyId(const std::shared_ptr<XamlProperty>& inXamlProperty, _In_ const WinBluePropertyTypeCompatHelper& winBluePropertyTypeCompatHelper, _Out_ XUINT32& ruiXamlPropertyId)
{
    ruiXamlPropertyId = 0;

    XUINT32 interimPropertyId;
    xstring_ptr spPropertyName;
    bool isUnknown = inXamlProperty->IsUnknown();
    bool found = false;

    IFC_RETURN(inXamlProperty->get_Name(&spPropertyName));

    if (isUnknown)
    {
        // TODO: this may not be enough information to resolve the property correctly
        // if the declaring type is also unknown. Will need to evaluate/test this scenario.
        std::shared_ptr<XamlType> spDeclaringType;
        xstring_ptr spDeclaringTypeName;
        IFC_RETURN(inXamlProperty->get_DeclaringType(spDeclaringType));
        IFC_RETURN(spDeclaringType->get_Name(&spDeclaringTypeName))
        
        auto result = m_mapMasterUnknownPropertyList.find(std::make_pair(spDeclaringTypeName, spPropertyName));
        if (result != m_mapMasterUnknownPropertyList.end())
        {
            found = true;
            interimPropertyId = result->second;
        }
    }
    else
    {
        auto result = m_mapMasterKnownPropertyList.find(inXamlProperty->get_PropertyToken());
        if (result != m_mapMasterKnownPropertyList.end())
        {
            found = true;
            interimPropertyId = result->second;
        }
    }

    if (!found)
    {
        interimPropertyId = m_vecMasterPropertyList.size();
        PersistedXamlProperty persistedProperty;

        IFC_RETURN(GetStringId(spPropertyName, persistedProperty.m_uiPropertyName));

        if (inXamlProperty->IsDirective())
        {
            persistedProperty.m_PropertyFlags.SetBit(PersistedXamlProperty::IsMarkupDirective);
            persistedProperty.m_uiType = 0;
            m_mapMasterKnownPropertyList.emplace(inXamlProperty->get_PropertyToken(), interimPropertyId);
        }
        else if (inXamlProperty->IsImplicit())
        {
            std::shared_ptr<XamlType> spXamlType;
            persistedProperty.m_PropertyFlags.SetBit(PersistedXamlProperty::IsImplicitProperty);
            IFC_RETURN(inXamlProperty->get_Type(spXamlType));
            //!!!! this should only be for __implicit_items
            if (spXamlType)
            {
                // TODO: Is it possible to for an app to declare their own implicit property?
                // If not, will we ever reach this code, since we know about all the implicit properties?
                IFC_RETURN(GetXamlTypeId(spXamlType, persistedProperty.m_uiType));
            }
        }
        else
        {
            // TODO: Can apps declare new properties on our built-in types? If so, this code
            // may be encoding metadata about known types, subverting a desirable optimization
            
            // REVIEW: This version check is probably superfluous (until we gain the ability to encode
            // older versions of XBF using the latest encoder)
            if (GetVersion().m_uMajorBinaryFileVersion == 2)
            {
                // FUTURE: it's impossible to do this check on an unknown property (e.g. conditionally declared property
                // that can't be resolved at encode-time, or its declaring type is conditionally declared itself and similarly can't
                // be resolved at encode-time). Probably OK because it seems difficult to imagine a scenario in which a custom property
                // can't be resolved at encode-time. (Hi twelve months later Alan!)
                // The reason this is potentially problematic is that the metadata reader uses XamlType::GetDependencyProperty() for custom
                // dependency properties, but XamlType::GetProperty() for built-in ones.
                const CDependencyProperty *pdp = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(inXamlProperty->get_PropertyToken().GetHandle());
                if (pdp && pdp->Is<CCustomDependencyProperty>())
                {
                    persistedProperty.m_PropertyFlags.SetBit(PersistedXamlProperty::IsCustomDependencyProperty);
                }
            }
            else if (GetVersion().m_uMajorBinaryFileVersion > 2)
            {
                // REVIEW: looks like we added a new XBF major version!
                // Verify that the above block (for major == 2) isn't necessary.
                ASSERT(false);
            }
            std::shared_ptr<XamlType> spDeclaringXamlType;
            IFC_RETURN(inXamlProperty->get_DeclaringType(spDeclaringXamlType));
            
            // Marking the PersistedXamlProperty as Unknown indicates that when deserializing we need to resolve it slightly
            // differently
            //
            // MSFT:16178797
            // There was an annoying change to the way UWP apps are built that results in GenXbf being presented with platform
            // metadata corresponding to TPV rather than TPMV. This means that properties which were added after TPMV will
            // appear to be "known". This will cause problems later during deserialization when run on TPMV because
            // those "known" properties will fail to be resolved and the framework will balk since the property failed to resolve but it
            // wasn't serialized as an unknown property (this check occurs during deserialization, before the parser is even fed the node
            // referencing the property).
            // We'll work around this during generation by saying that *every* untrusted property is "unknown", thus bypassing the check
            // during deserialization. Later stages in the parser pipeline will catch if the type is truly unknown and the associated node
            // wasn't protected by conditional XAML. Note that this only applies to TPMV >= RS2, since that is when conditional XAML was introduced.
            if (isUnknown || GetTargetOSVersion() >= Parser::Versioning::OSVersions::WIN10_RS2)
            {
                persistedProperty.m_PropertyFlags.SetBit(PersistedXamlProperty::IsUnknown);
            }

            if (isUnknown)
            {
                xstring_ptr spDeclaringTypeName;
                IFC_RETURN(spDeclaringXamlType->get_Name(&spDeclaringTypeName));
                IFC_RETURN(GetXamlTypeId(spDeclaringXamlType, persistedProperty.m_uiType));

                m_mapMasterUnknownPropertyList.emplace(
                    std::make_pair(spDeclaringTypeName, spPropertyName), 
                    interimPropertyId);
            }
            else
            {
                // please see comments in WinBluePropertyTypeCompatHelper.cpp.
                spDeclaringXamlType = winBluePropertyTypeCompatHelper.GetWinBlueDeclaringTypeForProperty(inXamlProperty, spDeclaringXamlType);

                IFC_RETURN(GetXamlTypeId(spDeclaringXamlType, persistedProperty.m_uiType));
                m_mapMasterKnownPropertyList.emplace(inXamlProperty->get_PropertyToken(), interimPropertyId);
            }
        }

        IFC_RETURN(m_vecMasterPropertyList.push_back(persistedProperty));
    }

    ruiXamlPropertyId = interimPropertyId;
    return S_OK;
}


_Check_return_ HRESULT
XamlBinaryMetadataStore::GetXamlXmlNamespaceId(const std::shared_ptr<XamlNamespace>& inXmlNamespace, _Out_ XUINT32& ruiXmlNamespace)
{
    HRESULT hr = S_OK;
    xstring_ptr spNamespaceUri = inXmlNamespace->get_TargetNamespace();
    
    IFC(m_mapMasterXmlNamespaceList.Get(spNamespaceUri, ruiXmlNamespace));
    if (hr == S_FALSE)
    {
        ruiXmlNamespace = m_vecMasterXmlNamespaceList.size();
        PersistedXamlXmlNamespace persistedXmlNamespace;

        IFC(GetStringId(spNamespaceUri, persistedXmlNamespace.m_uiNamespaceUri));

        IFC(m_mapMasterXmlNamespaceList.Add(spNamespaceUri, ruiXmlNamespace));
        IFC(m_vecMasterXmlNamespaceList.push_back(persistedXmlNamespace));
    }

Cleanup:
    if (FAILED(hr))
    {
        ruiXmlNamespace = 0;
    }

    RRETURN(hr);
}

_Check_return_ HRESULT 
XamlBinaryMetadataStore::GetStringId(_In_ const xstring_ptr& inString, _Out_ XUINT32& ruiStringId)
{
    HRESULT hr = S_OK;
    
    IFC(m_mapStringTable.Get(inString, ruiStringId));
    if (hr == S_FALSE)
    {
        ruiStringId = m_vecStringTable.size();
        IFC(m_mapStringTable.Add(inString, ruiStringId));
        IFC(m_vecStringTable.push_back(inString));
    }

Cleanup:
    if (FAILED(hr))
    {
        ruiStringId = 0;
    }

    RRETURN(hr);
}

_Check_return_ HRESULT 
XamlBinaryMetadataStore::GetXamlTextValueId(_In_ const xstring_ptr& inString, _Out_ XUINT32& ruiStringId)
{
    HRESULT hr = S_OK;
    
    ruiStringId = m_vecStringTable.size();
    IFC(m_vecStringTable.push_back(inString));

Cleanup:
    if (FAILED(hr))
    {
        ruiStringId = 0;
    }

    RRETURN(hr);
}

