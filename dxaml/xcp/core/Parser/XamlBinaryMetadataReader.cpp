// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBinaryMetadataReader.h"
#include "XamlTypeNamespace.h"
#include "XamlUnknownXmlNamespace.h"
#include "XamlSerializationHelper.h"
#include "XbfMetadataApi.h"
#include "MetadataAPI.h"
#include "XamlTypeTokens.h"

using namespace Parser;

_Check_return_ HRESULT
XamlBinaryMetadataReader::LoadMetadata()
{
    IFC_RETURN(LoadHeader());
    IFC_RETURN(LoadStringTable());
    IFC_RETURN(LoadAssemblyList());
    IFC_RETURN(LoadTypeNamespaceList());
    IFC_RETURN(LoadTypeList());
    IFC_RETURN(LoadPropertyList());
    IFC_RETURN(LoadXmlNamespaceList());
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader::LoadHeader()
{
    IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeItemFromMetadataStream(&m_header, m_version, m_spMetadataStream));
    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryMetadataReader::LoadStringTable()
{
    HRESULT hr = S_OK;
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strStringTableDescription, L"String Table");
    XUINT64 uPos = 0;
    bool fOffsetCheckFailed = true;
    bool fDeserializationFailed = true;

    IFC(m_spMetadataStream->GetPosition(&uPos));
    if (uPos != m_header.m_uStringTableOffset)
    {
        IFC(E_FAIL);
    }
    fOffsetCheckFailed = FALSE;

    IFC(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(m_vecStringTable, m_version, m_spMetadataStream));
    fDeserializationFailed = FALSE;
Cleanup:
    if (FAILED(hr))
    {
        if (fOffsetCheckFailed)
        {
            IGNOREHR(LogOffsetCheckError(c_strStringTableDescription));
        }
        else if (fDeserializationFailed)
        {
            IGNOREHR(LogDeserializationError(c_strStringTableDescription));
        }
    }

    RRETURN(hr);
}


_Check_return_ HRESULT
XamlBinaryMetadataReader::LoadAssemblyList()
{
    HRESULT hr = S_OK;
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strReferenceTableDescription, L"Reference Table");
    xvector < PersistedXamlAssembly > vecAssemblyList;
    XUINT64 uPos = 0;
    xstring_ptr spAssemblyName;
    bool fOffsetCheckFailed = true;
    bool fDeserializationFailed = true;

    IFC(m_spMetadataStream->GetPosition(&uPos));
    if (uPos != m_header.m_uAssemblyListOffset)
    {
        IFC(E_FAIL);
    }
    fOffsetCheckFailed = FALSE;

    IFC(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecAssemblyList, m_version, m_spMetadataStream));
    IFC(m_vecMasterAssemblyList.reserve(vecAssemblyList.size()));
    fDeserializationFailed = FALSE;
    
    for (xvector<PersistedXamlAssembly>::const_iterator it = vecAssemblyList.begin();
            it != vecAssemblyList.end();
                ++it)
    {
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlAssembly> spAssembly;
        XamlAssemblyToken assemblyToken;

        IFC(GetString(it->m_uiAssemblyName, spAssemblyName));
        
        IFC(m_spXamlSchemaContext->GetTypeInfoProvider(it->m_eTypeInfoProviderKind, spTypeInfoProvider));
        IFC(spTypeInfoProvider->ResolveAssembly(spAssemblyName, assemblyToken));
        IFC(m_spXamlSchemaContext->GetXamlAssembly(assemblyToken, spAssemblyName, spAssembly));

        if (!spAssembly)
        {
            IFC(E_FAIL);
        }
        else
        {
            IFC(m_vecMasterAssemblyList.push_back(spAssembly));
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        if (fOffsetCheckFailed)
        {
            IGNOREHR(LogOffsetCheckError(c_strReferenceTableDescription));
        }
        else if (fDeserializationFailed)
        {
            IGNOREHR(LogDeserializationError(c_strReferenceTableDescription));
        }
        else
        {
            IGNOREHR(LogError(AG_E_PARSER2_XBF_METADATA_ASSEMBLY_LIST, xstring_ptr::NullString(), xstring_ptr::NullString()));           
        }
    }

    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryMetadataReader::LoadTypeNamespaceList()
{
    HRESULT hr = S_OK;
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strTypeNamespaceTableDescription, L"Type Namespace Table");
    xvector < PersistedXamlTypeNamespace > vecTypeNamespaceList;
    XUINT64 uPos = 0;
    bool fOffsetCheckFailed = true;
    bool fDeserializationFailed = true;
    xstring_ptr spNamespaceName;
    std::shared_ptr<XamlAssembly> spAssembly;

    IFC(m_spMetadataStream->GetPosition(&uPos));
    if (uPos != m_header.m_uTypeNamespaceListOffset)
    {
        IFC(E_FAIL);
    }
    fOffsetCheckFailed = FALSE;

    IFC(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecTypeNamespaceList, m_version, m_spMetadataStream));
    IFC(m_vecMasterTypeNamespaceList.reserve(vecTypeNamespaceList.size()));
    fDeserializationFailed = FALSE;
    
    for (xvector<PersistedXamlTypeNamespace>::const_iterator it = vecTypeNamespaceList.begin();
            it != vecTypeNamespaceList.end();
                ++it)
    {
        std::shared_ptr<XamlTypeNamespace> spTypeNamespace;
 
        spAssembly.reset();
        spNamespaceName.Reset();

        IFC(GetString(it->m_uiNamespaceName, spNamespaceName));
        IFC(GetAssembly(it->m_uiAssembly, spAssembly));
        IFC(spAssembly->GetTypeNamespace_ForBinaryXaml(spAssembly, spNamespaceName, spTypeNamespace));

        if (!spTypeNamespace)
        {
            IFC(E_FAIL);
        }
        else
        {
            IFC(m_vecMasterTypeNamespaceList.push_back(spTypeNamespace));
        }
    }
Cleanup:
    if (FAILED(hr))
    {
        if (fOffsetCheckFailed)
        {
            IGNOREHR(LogOffsetCheckError(c_strTypeNamespaceTableDescription));
        }
        else if (fDeserializationFailed)
        {
            IGNOREHR(LogDeserializationError(c_strTypeNamespaceTableDescription));
        }
        else
        {
            IGNOREHR(LogError(AG_E_PARSER2_XBF_METADATA_TYPE_NAMESPACE_LIST, spNamespaceName, xstring_ptr::NullString()));
        }
    }

    RRETURN(hr);

}

_Check_return_ HRESULT
XamlBinaryMetadataReader::LoadTypeList()
{
    HRESULT hr = S_OK;
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strTypeTableDescription, L"Type Table");
    xvector < PersistedXamlType > vecTypeList;
    XUINT64 uPos = 0;
    bool fOffsetCheckFailed = true;
    bool fDeserializationFailed = true;
    xstring_ptr spTypeName;
    std::shared_ptr<XamlTypeNamespace> spTypeNamespace;

    IFC(m_spMetadataStream->GetPosition(&uPos));
    if (uPos != m_header.m_uTypeListOffset)
    {
        IFC(E_FAIL);
    }
    fOffsetCheckFailed = FALSE;

    IFC(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecTypeList, m_version, m_spMetadataStream));
    IFC(m_vecMasterTypeList.reserve(vecTypeList.size()));
    fDeserializationFailed = FALSE;
    
    for (xvector<PersistedXamlType>::const_iterator it = vecTypeList.begin();
            it != vecTypeList.end();
                ++it)
    {
        std::shared_ptr<XamlType> spType;

        spTypeName.Reset();
        spTypeNamespace.reset();

        IFC(GetString(it->m_uiTypeName, spTypeName));
        if (it->m_TypeFlags.IsBitSet(PersistedXamlType::IsMarkupDirective))
        {
            std::shared_ptr<XamlNamespace> spDirectiveNamespace;
            
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_directiveNamespace, L"http://schemas.microsoft.com/winfx/2006/xaml");
            IFC(m_spXamlSchemaContext->GetXamlXmlNamespace(XSTRING_PTR_FROM_STORAGE(c_directiveNamespace), spDirectiveNamespace));
            IFC(spDirectiveNamespace->GetDirectiveType(spTypeName, spType));
        }
        else
        {
            IFC(GetTypeNamespace(it->m_uiTypeNamespace, spTypeNamespace));
            IFC(spTypeNamespace->GetXamlType(spTypeName, spType));

            //
            // If we failed to find the type, it might be in IXamlMetadataProvider which is not being 
            // queried because we reset the Namespace Token if we lookup through the ManagedProvider
            // in XamlNativeTypeInfoProvider::ResolveTypeName()
            //
            // Try to resolve with the Managed Provider explicitly passing the Namespace Index.
            //

            // TODO: If Bug 121474 is fixed, we can remove this code path.
            if (!spType &&
                spTypeNamespace->get_TypeNamespaceToken().GetProviderKind() == tpkNative)
            {
                std::shared_ptr<XamlTypeInfoProvider> spXamlTypeInfoProvider;
                XamlTypeNamespaceToken managedTypeNamespaceToken = spTypeNamespace->get_TypeNamespaceToken();
                XamlTypeToken retTypeToken;

                managedTypeNamespaceToken.SetProviderKind(tpkManaged); 
                IFC(m_spXamlSchemaContext->GetTypeInfoProvider(tpkManaged, spXamlTypeInfoProvider));
                IFC(spXamlTypeInfoProvider->ResolveTypeName(managedTypeNamespaceToken, spTypeName, retTypeToken));
                IFC(m_spXamlSchemaContext->GetXamlType(retTypeToken, spTypeNamespace, spTypeName, spType));
            }                  
        }

        if (!spType)
        {
            IFC(E_FAIL);
        }
        else
        {
            IFC(m_vecMasterTypeList.push_back(spType));
        }
    }
Cleanup:
    if (FAILED(hr))
    {
        if (fOffsetCheckFailed)
        {
            IGNOREHR(LogOffsetCheckError(c_strTypeTableDescription));
        }
        else if (fDeserializationFailed)
        {
            IGNOREHR(LogDeserializationError(c_strTypeTableDescription));
        }
        else
        {
            xstring_ptr spTypeNamespaceName;

            if (spTypeNamespace)
            {
                spTypeNamespaceName = spTypeNamespace->get_TargetNamespace();
            }

            IGNOREHR(LogError(AG_E_PARSER2_XBF_TYPE_LIST, spTypeNamespaceName, spTypeName));
        }
    }

    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryMetadataReader::LoadPropertyList()
{
    HRESULT hr = S_OK;
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strPropertyTableDescription, L"Property Table");
    xvector<PersistedXamlProperty> vecPropertyList;
    XUINT64 uPos = 0;
    bool fOffsetCheckFailed = true;
    bool fDeserializationFailed = true;
    xstring_ptr spPropertyName;
    std::shared_ptr<XamlType> spType;

    IFC(m_spMetadataStream->GetPosition(&uPos));
    if (uPos != m_header.m_uPropertyListOffset)
    {
        IFC(E_FAIL);
    }
    fOffsetCheckFailed = FALSE;

    IFC(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecPropertyList, m_version, m_spMetadataStream));
    IFC(m_vecMasterPropertyList.reserve(vecPropertyList.size()));
    fDeserializationFailed = FALSE;

    for (xvector<PersistedXamlProperty>::const_iterator it = vecPropertyList.begin();
            it != vecPropertyList.end();
                ++it)
    {
        std::shared_ptr<XamlProperty> spProperty;

        spPropertyName.Reset();
        spType.reset();

        IFC(GetString(it->m_uiPropertyName, spPropertyName));
        
        if (it->m_PropertyFlags.IsBitSet(PersistedXamlProperty::IsImplicitProperty))
        {
            if (spPropertyName.Equals(STR_LEN_PAIR(L"__implicit_items")))
            {
                // The implicit items property is generically applicable
                std::shared_ptr<ImplicitProperty> implicitProperty;
                IFC(m_spXamlSchemaContext->get_ItemsProperty(spType /* unused */, implicitProperty));
                spProperty = implicitProperty;
            }
            else if (spPropertyName.Equals(STR_LEN_PAIR(L"__implicit_initialization")))
            {
                std::shared_ptr<ImplicitProperty> implicitProperty;
                IFC(m_spXamlSchemaContext->get_InitializationProperty(implicitProperty));
                spProperty = implicitProperty;
            }
            else
            {
                IFC(E_FAIL);
            }
            
        }
        else if (it->m_PropertyFlags.IsBitSet(PersistedXamlProperty::IsMarkupDirective))
        {
            // First look in Xaml markup namespace
            std::shared_ptr<XamlNamespace> spNamespace;
            std::shared_ptr<DirectiveProperty> directiveProperty;

            IFC(m_spXamlSchemaContext->GetXamlXmlNamespace(c_strMarkupUri, spNamespace));
            IFC(m_spXamlSchemaContext->GetXamlDirective(spNamespace, spPropertyName, directiveProperty));
            spProperty = directiveProperty;
            
            // if we can't find in xaml markup namespace, look in xml namespace
            if (!spProperty)
            {
                IFC(m_spXamlSchemaContext->GetXamlXmlNamespace(c_strXmlUri, spNamespace));
                IFC(m_spXamlSchemaContext->GetXamlDirective(spNamespace, spPropertyName, directiveProperty));
                spProperty = directiveProperty;
            }
        }
        else
        {
            IFC(GetType(it->m_uiType, spType));
            IFC(spType->GetProperty(spPropertyName, spProperty));
        }

        if (!spProperty)
        {
            IFC(E_FAIL);
        }
        else
        {
            IFC(m_vecMasterPropertyList.push_back(spProperty));
        }
    }
Cleanup:
    if (FAILED(hr))
    {
        if (fOffsetCheckFailed)
        {
            IGNOREHR(LogOffsetCheckError(c_strPropertyTableDescription));
        }
        else if (fDeserializationFailed)
        {
            IGNOREHR(LogDeserializationError(c_strPropertyTableDescription));
        }
        else
        {
            xstring_ptr spTypeName;

            if (spType)
            {
                IGNOREHR(spType->get_FullName(&spTypeName));
            }

            IGNOREHR(LogError(AG_E_PARSER2_XBF_METADATA_PROPERTY_LIST, spTypeName, spPropertyName));
        }
    }

    RRETURN(hr);
}

_Check_return_ HRESULT
XamlBinaryMetadataReader::LoadXmlNamespaceList()
{
    HRESULT hr = S_OK;
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strXmlNamespaceTableDescription, L"Xml Namespace Table");
    xvector<PersistedXamlXmlNamespace> vecXmlNamespaceList;
    XUINT64 uPos = 0;
    xstring_ptr spNamespaceUri;
    bool fOffsetCheckFailed = true;
    bool fDeserializationFailed = true;

    IFC(m_spMetadataStream->GetPosition(&uPos));
    if (uPos != m_header.m_uXmlNamespaceListOffset)
    {
        IFC(E_FAIL);
    }
    fOffsetCheckFailed = FALSE;
    
    IFC(XamlBinaryFormatSerializationHelper::DeserializeVectorFromMetadataStream(vecXmlNamespaceList, m_version, m_spMetadataStream));
    IFC(m_vecMasterXmlNamespaceList.reserve(vecXmlNamespaceList.size()));
    fDeserializationFailed = FALSE;

    for (xvector<PersistedXamlXmlNamespace>::const_iterator it = vecXmlNamespaceList.begin();
            it != vecXmlNamespaceList.end();
                ++it)
    {
        std::shared_ptr<XamlNamespace> spXamlNamespace;

        spNamespaceUri.Reset();

        IFC(GetString(it->m_uiNamespaceUri, spNamespaceUri));
        IFC(m_spXamlSchemaContext->GetXamlXmlNamespace(spNamespaceUri, spXamlNamespace));

        if (!spXamlNamespace)
        {
            auto spUnknownNamespace = std::make_shared<XamlUnknownXmlNamespace>(m_spXamlSchemaContext, spNamespaceUri);
            spXamlNamespace = spUnknownNamespace;
        }

        if (!spXamlNamespace)
        {
            IFC(E_FAIL);
        }
        else
        {
            IFC(m_vecMasterXmlNamespaceList.push_back(spXamlNamespace));
        }
    }
Cleanup:
    if (FAILED(hr))
    {
        if (fOffsetCheckFailed)
        {
            IGNOREHR(LogOffsetCheckError(c_strXmlNamespaceTableDescription));
        }
        else if (fDeserializationFailed)
        {
            IGNOREHR(LogDeserializationError(c_strXmlNamespaceTableDescription));
        }
        else
        {
            IGNOREHR(LogError(AG_E_PARSER2_XBF_METADATA_XML_NAMESPACE_LIST, spNamespaceUri, xstring_ptr::NullString()));
        }
    }

    RRETURN(hr);
}

_Check_return_ HRESULT XamlBinaryMetadataReader::LogOffsetCheckError(_In_ const xstring_ptr& strTableDescription)
{
    IFC_RETURN(LogError(AG_E_PARSER2_XBF_METADATA_OFFSET, strTableDescription, xstring_ptr::NullString()));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader::LogDeserializationError(_In_ const xstring_ptr& strTableDescription)
{
    IFC_RETURN(LogError(AG_E_PARSER2_XBF_METADATA_DESERIALIZE, strTableDescription, xstring_ptr::NullString()));
    return S_OK;
}


_Check_return_ HRESULT XamlBinaryMetadataReader::LogError(
    _In_ XUINT32 iErrorCode, 
    _In_ const xstring_ptr& strParam1, 
    _In_ const xstring_ptr& strParam2
    )
{
    // Log the failure to the error service
    std::shared_ptr<ParserErrorReporter> errorReporter;
    if (SUCCEEDED(m_spXamlSchemaContext->GetErrorService(errorReporter)))
    {
        // The error service can't handle NULL parameters
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strNullLiteral, L"null");

        IFC_RETURN(errorReporter->SetError(iErrorCode,
                 0,
                 0,
                 strParam1.IsNull() ? c_strNullLiteral : strParam1,
                 strParam2.IsNull() ? c_strNullLiteral : strParam2));
    }
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryMetadataReader::GetString(XUINT32 index, _Out_ xstring_ptr& result) const
{
    return m_vecStringTable.get_item(index, result);
}

_Check_return_ HRESULT XamlBinaryMetadataReader::GetAssembly(XUINT32 index, _Out_ std::shared_ptr<XamlAssembly>& result) const
{
    return m_vecMasterAssemblyList.get_item(index, result);
}

_Check_return_ HRESULT XamlBinaryMetadataReader::GetTypeNamespace(XUINT32 index, _Out_ std::shared_ptr<XamlTypeNamespace>& result) const
{
    return m_vecMasterTypeNamespaceList.get_item(index, result);
}

_Check_return_ HRESULT XamlBinaryMetadataReader::GetType(XUINT32 index, _Out_ std::shared_ptr<XamlType>& result) const
{
    return m_vecMasterTypeList.get_item(index, result);
}

_Check_return_ HRESULT XamlBinaryMetadataReader::GetProperty(XUINT32 index, _Out_ std::shared_ptr<XamlProperty>& result) const
{
    return m_vecMasterPropertyList.get_item(index, result);
}

_Check_return_ HRESULT XamlBinaryMetadataReader::GetXmlNamespace(XUINT32 index, _Out_ std::shared_ptr<XamlNamespace>& result) const
{
    return m_vecMasterXmlNamespaceList.get_item(index, result);
}

