// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlSchemaContext.h"

#include "XamlPredicateService.h"
#include "xamlxmlnamespace.h"
#include "XamlTypeNamespace.h"
#include "XamlSpecialXmlNamespace.h"
#include "xcptypes.h"
#include "NodeStreamCache.h"
#include "XbfMetadataApi.h"
#include "MetadataAPI.h"
#include <XStringUtils.h>
#include <XStringBuilder.h>

using namespace Parser;

#pragma region Creation Methods

// factory creator with no parser error service
_Check_return_ HRESULT XamlSchemaContext::Create(
    _In_ IParserCoreServices *pCore,
    _Out_ std::shared_ptr<XamlSchemaContext>& outXamlSchemaContext
    )
{
    // Passing in null will let it choose the default.
    std::shared_ptr<ParserErrorReporter> nullErrorReporter;
    return XamlSchemaContext::Create(pCore, nullErrorReporter, outXamlSchemaContext);
}

// factory creator using an existing parser error service
_Check_return_ HRESULT XamlSchemaContext::Create(
    _In_ IParserCoreServices *pCore,
    _In_ const std::shared_ptr<ParserErrorReporter>& inErrorReporter,
    _Out_ std::shared_ptr<XamlSchemaContext>& outXamlSchemaContext
    )
{
    outXamlSchemaContext = std::make_shared<XamlSchemaContext>();

    //
    // The core will have a reference to the XamlSchemaContext, and if the XamlSchemaContext
    // kept a back-reference, then it would be a circular ref.
    //
    outXamlSchemaContext->m_pCore = pCore;

    if (!inErrorReporter)
    {
        std::shared_ptr<ParserErrorService> errorService = std::make_shared<ParserErrorService>();
        errorService->Initialize(pCore);
        outXamlSchemaContext->m_ErrorService = errorService;
    }
    else
    {
        // Assumes it's already initialized when you pass it in.
        outXamlSchemaContext->m_ErrorService = inErrorReporter;
    }

    IFC_RETURN(outXamlSchemaContext->Initialize());

    return S_OK;
}

#pragma endregion

#pragma region Schema Context APIs for Assembly, Namespace, TypeNamespace, Type and Property

xstring_ptr XamlSchemaContext::GetSourceAssembly() const
{
    if (!m_SourceAssemblies.empty())
    {
        return m_SourceAssemblies.top();
    }
    else
    {
        return GetDefaultAssemblyName();
    }
}

_Check_return_ HRESULT XamlSchemaContext::PushSourceAssembly(
    _In_ const xstring_ptr& inssSourceAssembly
    )
{
    try
    {
        m_SourceAssemblies.push(inssSourceAssembly);
    }
    CATCH_RETURN();
    return S_OK;
}

void XamlSchemaContext::PopSourceAssembly()
{
    m_SourceAssemblies.pop();
}

xstring_ptr XamlSchemaContext::GetDefaultAssemblyName() const
{
    DECLARE_CONST_XSTRING_PTR_STORAGE(c_strDefaultAssemblyName, L"Microsoft.UI.Xaml");
    return xstring_ptr(c_strDefaultAssemblyName);
}

// create and add a new assembly token to the cache
_Check_return_ HRESULT XamlSchemaContext::AddAssembly(
    _In_ const XamlAssemblyToken& inAssemblyToken,
    _In_ const xstring_ptr& strAssemblyName
    )
{
    try
    {
        m_mapMasterAssemblyList.emplace(inAssemblyToken,
            std::make_shared<XamlAssembly>(shared_from_this(), strAssemblyName, inAssemblyToken));
    }
    CATCH_RETURN();

    return S_OK;
}

// Either gets the cached singleton XamlAssembly object associated
// with a XamlAssemblyToken, or creates a new one and caches it.
_Check_return_ HRESULT XamlSchemaContext::GetXamlAssembly(
    _In_ const XamlAssemblyToken& inAssemblyToken,
    _In_ const xstring_ptr& inAssemblyName,
    _Out_ std::shared_ptr<XamlAssembly>& outXamlAssembly
    )
{
    outXamlAssembly.reset();

    auto pos = m_mapMasterAssemblyList.find(inAssemblyToken);
    if (pos != m_mapMasterAssemblyList.end())
    {
        outXamlAssembly = pos->second;
    }
    if (!outXamlAssembly)
    {
        try
        {
            auto assembly = std::make_shared<XamlAssembly>(shared_from_this(), inAssemblyName, inAssemblyToken);
            m_mapMasterAssemblyList.emplace(inAssemblyToken, assembly);
            outXamlAssembly = std::move(assembly);
        }
        CATCH_RETURN();
    }

    return S_OK;
}

// Either gets the cached singleton XamlAssembly object associated
// with a XamlAssemblyToken, or fails.
_Check_return_ HRESULT XamlSchemaContext::GetXamlAssembly(
    _In_ const XamlAssemblyToken& inAssemblyToken,
    _Out_ std::shared_ptr<XamlAssembly>& outXamlAssembly
    )
{
    outXamlAssembly.reset();
    auto pos = m_mapMasterAssemblyList.find(inAssemblyToken);
    if (pos != m_mapMasterAssemblyList.end())
    {
        outXamlAssembly = pos->second;
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

// Helper method that validates a passed-in namespace string of the form
// "using:<code namespace>" and returns the code namespace component.
// Sets 'isValidXmlns' parameter to false if input is not of this form.
_Check_return_ HRESULT XamlSchemaContext::CrackXmlns(
    _In_ const xstring_ptr& strXmlns,
    _Out_ xstring_ptr& strNamespace,
    _Out_ bool& isValidXmlns)
{
    isValidXmlns = false;

    // String can't be empty/null, and no spaces allowed
    if (   strXmlns.IsNullOrEmpty()
        || strXmlns.FindChar(L' ') != xstring_ptr_view::npos)
    {
        return S_OK;
    }

    auto colonIndex = strXmlns.FindChar(L':');
    xstring_ptr car, cdr;
    IFC_RETURN(strXmlns.SubString(0, colonIndex, &car));
    IFC_RETURN(strXmlns.SubString(colonIndex + 1, strXmlns.GetCount(), &cdr));

    // First substring should be "using", second substring should be non-null/empty, and
    // not have any additional colons
    if (   !car.Equals(L"using")
        || cdr.IsNullOrEmpty()
        || cdr.FindChar(L':') != xstring_ptr_view::npos)
    {
        return S_OK;
    }
    else
    {
        isValidXmlns = true;
        strNamespace = cdr;
    }

    return S_OK;
}

// Adds the xmlns - Type Namespace mapping that the Assembly
// represented by inAssembly is signing up for.  In managed
// code this is indicated by the XmlnsDefinitionAttribute
//
// A note on why inXmlnsTypeNamespace is represented by a
// XamlTypeNamespaceToken rather than a string:  This information
// is provided by the XamlTypeInfoProvider that provides type information
// for types in inAssembly.  So it doesn't matter so much what
// is used to identify the TypeNamespace, but rather that the
// TypeInfoProvider will recognize it later when
// XamlTypeInfoProvider::ResolveTypeName() is called.
_Check_return_ HRESULT XamlSchemaContext::AddAssemblyXmlnsDefinition(
    _In_ const XamlAssemblyToken& inAssembly,
    _In_ const xstring_ptr& strXmlnsUri,
    _In_ const XamlTypeNamespaceToken& inTypeNamespaceToken,
    _In_ const xstring_ptr& strTypeNamespace
    )
{
    std::shared_ptr<XamlAssembly> spAssembly;
    std::shared_ptr<XamlXmlNamespace> spXmlNamespace;

    // get the XamlAssembly for the passed in token.  This entry
    // should already exist, and if it doesn't then we fail.
    auto pos = m_mapMasterAssemblyList.find(inAssembly);
    if (pos != m_mapMasterAssemblyList.end())
    {
        spAssembly = pos->second;
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    xstring_ptr baseXmlns;
    xstring_ptr conditionalPredicate;
    XamlPredicateService::CrackConditionalXmlns(strXmlnsUri, baseXmlns, conditionalPredicate);

    {
        // REVIEW: This should probably be 'baseXmlns' here, not 'strXmlnsUri',
        // but OK for now because only internal namespaces are registered via this method
        // and those are never conditional. If we expose 3rd-party registration of xmlns similar
        // to Silverlight's XmlnsDefinitionAttribute, then we will need to revist. Or just
        // forbid them from having conditional predicates.
        // Assert that baseXmlns == strXmlnsUri to catch ourselves in the act of shooting off our own feet.
        ASSERT(strXmlnsUri.Equals(baseXmlns));

        auto itXmlNamespace = m_mapUriToXmlNamespace.find(strXmlnsUri);
        // get or create the XamlXmlNamespace for the passed in Uri
        if (itXmlNamespace == m_mapUriToXmlNamespace.end())
        {
            // If we are registering a *new* XmlNamespace uri, then any questions of
            // ignored elements or attributes due to ignored xml namespaces would need
            // to be re-evaluated for cached NodeStreams, so we must flush the XamlNodeStreamCacheManager
            IFC_RETURN(FlushXamlNodeStreamCacheManager());

            spXmlNamespace = std::make_shared<XamlXmlNamespace>(shared_from_this(), baseXmlns, conditionalPredicate, m_xmlNamespaceVector.size());
            m_xmlNamespaceVector.emplace_back(spXmlNamespace);
            VERIFY_COND(m_mapUriToXmlNamespace.insert({ strXmlnsUri, spXmlNamespace }), .second);
        }
        else{
            spXmlNamespace = itXmlNamespace->second;
        }
    }

    {
        std::shared_ptr<XamlTypeNamespace> spTypeNamespace;
        IFC_RETURN(GetXamlTypeNamespace(inTypeNamespaceToken, strTypeNamespace, spAssembly, spXmlNamespace, spTypeNamespace));
        IFC_RETURN(spXmlNamespace->AddTypeNamespace(spTypeNamespace));
    }

    return S_OK;
}

_Check_return_ HRESULT
XamlSchemaContext::GetXamlXmlNamespace(
    _In_ const xstring_ptr& inXmlns,
    _Out_ std::shared_ptr<XamlNamespace>& outNamespace)
{
    // TODO: This suggests that perhaps XamlNamespace can be replaced with
    // XamlXmlNamespace.  Leave this here so that no code outside of XamlSchemaContext expects
    // XamlXmlContext.
    std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;

    {
        xstring_ptr baseXmlns;
        xstring_ptr conditionalPredicate;
        XamlPredicateService::CrackConditionalXmlns(inXmlns, baseXmlns, conditionalPredicate);

        auto itXmlNamespace = m_mapUriToXmlNamespace.find(baseXmlns);
        if (itXmlNamespace == m_mapUriToXmlNamespace.end())
        {
            // The managed provider will fall back on the native provider.
            IFC_RETURN(GetTypeInfoProvider(tpkManaged, spTypeInfoProvider));

            if (spTypeInfoProvider)
            {
                XamlTypeNamespaceToken namespaceToken;
                HRESULT hr = spTypeInfoProvider->GetTypeNamespace(baseXmlns, namespaceToken);

                // If it's S_FALSE we couldn't find the namespace, so fall out.
                if (hr == S_FALSE)
                {
                    return S_FALSE;
                }
                else
                {
                    IFC_RETURN(hr);

                    auto xamlXmlNamespace = std::make_shared<XamlXmlNamespace>(shared_from_this(), baseXmlns, conditionalPredicate);

                    xstring_ptr typeNamespaceName;
                    IFC_RETURN(ExtractTypeNamespace(baseXmlns, typeNamespaceName));
                    std::shared_ptr<XamlTypeNamespace> typeNamespace;
                    IFC_RETURN(GetXamlTypeNamespace(namespaceToken, typeNamespaceName, std::shared_ptr<XamlAssembly>(), xamlXmlNamespace, typeNamespace));
                    IFC_RETURN(xamlXmlNamespace->AddTypeNamespace(typeNamespace));

                    m_xmlNamespaceVector.emplace_back(xamlXmlNamespace);
                    VERIFY_COND(m_mapUriToXmlNamespace.insert({ baseXmlns, xamlXmlNamespace }), .second);

                    outNamespace = xamlXmlNamespace;
                }
            }
        }
        else
        {
            // We found a previously registered XamlNamespace with the same base URI
            if (!conditionalPredicate.IsNullOrEmpty())
            {
                // The xmlns URI we're currently looking up is conditional, so clone
                // the non-conditional XamlNamespace and attach the conditional predicate
                outNamespace = itXmlNamespace->second->Clone();
                outNamespace->set_ConditionalPredicateString(conditionalPredicate);
            }
            else
            {
                outNamespace = itXmlNamespace->second;
            }
        }
    }

    return S_OK;
}

// Gets the XamlType from an index that was specified by the
// XamlSchemaContext at the time that the XamlNamespace was created.
const std::shared_ptr<XamlNamespace>& XamlSchemaContext::GetXamlNamespaceFromRuntimeIndex(
    _In_ const size_t uiIndex
    ) const
{
    return m_xmlNamespaceVector[uiIndex];
}

// Either gets the cached singleton XamlTypeNamespace object associated
// with a XamlTypeNamespaceToken, or creates a new one and caches it.
_Check_return_ HRESULT XamlSchemaContext::GetXamlTypeNamespace(
    _In_ const XamlTypeNamespaceToken& inTypeNamespaceToken,
    _In_ const xstring_ptr& inTypeNamespaceName,
    _In_ const std::shared_ptr<XamlAssembly>& inXamlAssembly,
    _In_ const std::shared_ptr<XamlXmlNamespace>& xamlXmlNamespace,
    _Out_ std::shared_ptr<XamlTypeNamespace>& outXamlTypeNamespace
    )
{
    outXamlTypeNamespace.reset();

    auto pos = m_mapMasterTypeNamespaceList.find(inTypeNamespaceToken);
    if (pos != m_mapMasterTypeNamespaceList.end())
    {
        outXamlTypeNamespace = pos->second;
    }
    if (!outXamlTypeNamespace)
    {
        if (inTypeNamespaceToken.GetHandle() != KnownNamespaceIndex::UnknownNamespace)
        {
            auto temp = std::make_shared<XamlTypeNamespace>(shared_from_this(), inTypeNamespaceName, inTypeNamespaceToken, inXamlAssembly, xamlXmlNamespace);
            m_mapMasterTypeNamespaceList.emplace(inTypeNamespaceToken, temp);
            outXamlTypeNamespace = std::move(temp);

            if (!xamlXmlNamespace)
            {
                // We weren't called with a valid XamlXmlNamespace (possibly because we're in the middle of resolving a base class whose type namespace doesn't
                // have a corresponding xmlns that we've encountered yet [and we may never encounter it, as this could happen if in the markup a derived type
                // is setting a property declared by the base type, but the base type is otherwise never directly used in the markup]). Therefore, we'll
                // back-construct a XamlXmlNamespace object using the type namespace's name, and then use that to replace the "originalXamlXmlNamespace" stored
                // in the XamlTypeNamespace object we just created.

                // ManagedTypeInfoProvider::GetTypeNamespace() expects the namespace string to still have "using:" prepended, so get the name into the expected form
                XStringBuilder namespaceBuilder;
                IFC_RETURN(namespaceBuilder.Initialize(c_strUsing));
                IFC_RETURN(namespaceBuilder.Append(inTypeNamespaceName));
                xstring_ptr normalizedTypeNamespaceName;
                IFC_RETURN(namespaceBuilder.DetachString(&normalizedTypeNamespaceName));

                std::shared_ptr<XamlNamespace> actualXamlXmlNamespace;
                IFC_RETURN(GetXamlXmlNamespace(normalizedTypeNamespaceName, actualXamlXmlNamespace));
                outXamlTypeNamespace->set_OriginalXamlXmlNamespace(actualXamlXmlNamespace);
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlType(
    _In_ const XamlTypeToken& inTypeToken,
    _Out_ std::shared_ptr<XamlType>& outXamlType
    )
{
    RRETURN(GetXamlType(inTypeToken, std::shared_ptr<XamlNamespace>(), xstring_ptr(), outXamlType));
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlType(
    _In_ const XamlTypeToken& inTypeToken,
    _In_ const std::shared_ptr<XamlNamespace>& inNamespace,
    _Out_ std::shared_ptr<XamlType>& outXamlType
    )
{
    RRETURN(GetXamlType(inTypeToken, inNamespace, xstring_ptr(), outXamlType));
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlType(
    _In_ const XamlTypeToken& inTypeToken,
    _In_ const std::shared_ptr<XamlNamespace>& inNamespace,
    _In_ const xstring_ptr& inTypeName,
    _Out_ std::shared_ptr<XamlType>& outXamlType
    )
{
    ASSERT(m_typeVector.size() >= StableXbfTypeCount);

    outXamlType.reset();

    // Known stable types get a priveleged fast lookup
    if (IsStableXbfType(inTypeToken))
    {
        auto runtimeIndex = static_cast<uint32_t>(GetStableXbfTypeIndex(inTypeToken.GetHandle()));
        outXamlType = m_typeVector[runtimeIndex];
    }
    else
    {
        auto pos = m_mapMasterTypeList.find(inTypeToken);
        if (pos != m_mapMasterTypeList.end())
        {
            outXamlType = pos->second;
        }
    }

    if (!outXamlType)
    {
        try
        {
            std::shared_ptr<XamlType> temp;
            size_t runtimeIndex = -1;
            THROW_IF_FAILED(KnownXamlType::Create(shared_from_this(), inNamespace, inTypeName, inTypeToken, temp));

            if (IsStableXbfType(inTypeToken))
            {
                runtimeIndex = static_cast<uint32_t>(GetStableXbfTypeIndex(inTypeToken.GetHandle()));
                ASSERT(!m_typeVector[runtimeIndex]);
                m_typeVector[runtimeIndex] = temp;
            }
            else
            {
                m_typeVector.emplace_back(temp);
                runtimeIndex = static_cast<uint32_t>(m_typeVector.size() - 1);
                m_mapMasterTypeList.emplace(inTypeToken, temp);
            }
            outXamlType = std::move(temp);
            outXamlType->SetRuntimeIndex(runtimeIndex);
        }
        CATCH_RETURN();
    }

    return S_OK;
}

// Gets the XamlType from an index that was specified by the
// XamlSchemaContext at the time that the XamlType was created.
_Check_return_ const std::shared_ptr<XamlType>&
    XamlSchemaContext::GetXamlTypeFromRuntimeIndex(
    _In_ const size_t uiIndex
    ) const
{
    return m_typeVector[uiIndex];
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlTypeFromStableXbfIndex(
    _In_ StableXbfTypeIndex index,
    _Out_ std::shared_ptr<XamlType>& outXamlType
    )
{
    auto runtimeIndex = static_cast<std::underlying_type<StableXbfTypeIndex>::type>(index);

    // GenXbf will always have the type tables corresponding to the SDK version
    // which means that it will consider types introduced after TPMV to be "trusted".
    // However, if the generated XBF is then loaded on TPMV, the StableXbfPropertyIndex
    // for those "trusted" types won't actually be valid. Therefore, we will create an UnknownType,
    // with as much information filled in as possible so that the XBF can be safely deserialized.
    // An error will be raised later by the BinaryFormatObjectWriter if conditional XAML syntax
    // wasn't used.
    if (runtimeIndex >= StableXbfTypeCount)
    {
        auto result = m_unknownStableXbfTypeIndexes.find(runtimeIndex);

        if (result == m_unknownStableXbfTypeIndexes.end())
        {
            xstring_ptr typeName;
            typeName = StringCchPrintfWWrapper(L"StableXbfTypeIndex::%d", runtimeIndex);
            std::shared_ptr<XamlType> unknownType;
            IFC_RETURN(UnknownType::Create(shared_from_this(), std::shared_ptr<XamlNamespace>(), typeName, unknownType));

            outXamlType = unknownType;
        }
        else
        {
            outXamlType = result->second;
        }

        return S_OK;
    }

    auto& result = m_typeVector[static_cast<size_t>(runtimeIndex)];

    if (!result)
    {
        auto classInfo = DirectUI::MetadataAPI::GetClassInfoByIndex(GetKnownTypeIndex(index));
        IFC_RETURN(KnownXamlType::Create(shared_from_this(), XamlTypeToken::FromType(classInfo), result));
        result->SetRuntimeIndex(static_cast<size_t>(runtimeIndex));
    }
    outXamlType = result;
    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::RegisterXamlProperty(
    _In_ const std::shared_ptr<XamlProperty>& spXamlProperty
    )
{
    ASSERT(m_memberVector.size() >= StableXbfPropertyCount);
    XamlPropertyToken propertyToken = spXamlProperty->get_PropertyToken();

    IFCEXPECT_RETURN(propertyToken.GetProviderKind() != tpkUnknown);

    size_t runtimeIndex = -1;

    if (IsStableXbfProperty(propertyToken))
    {
        runtimeIndex = static_cast<size_t>(GetStableXbfPropertyIndex(propertyToken.GetHandle()));
        ASSERT(!m_memberVector[runtimeIndex]);
        m_memberVector[runtimeIndex] = spXamlProperty;
    }
    else
    {
        IFCEXPECT_ASSERT_RETURN(m_mapMasterMemberList.find(propertyToken) == m_mapMasterMemberList.end());
        try
        {
            m_mapMasterMemberList.emplace(propertyToken, spXamlProperty);
            m_memberVector.emplace_back(spXamlProperty);
            runtimeIndex = m_memberVector.size() - 1;
        }
        CATCH_RETURN();
    }

    spXamlProperty->SetRuntimeIndex(runtimeIndex);

    return S_OK;
}

// Gets the XamlProperty associated with a XamlPropertyToken.  It is
// assumed (for this overload) that the property has already been
// registered with the XamlSchemaContext if it's not a built-in/known property
_Check_return_ HRESULT XamlSchemaContext::GetXamlProperty(
    _In_ const XamlPropertyToken& inPropertyToken,
    _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
    )
{
    if (IsStableXbfProperty(inPropertyToken))
    {
        auto stablePropertyIndex = GetStableXbfPropertyIndex(inPropertyToken.GetHandle());
        auto runtimeIndex = static_cast<uint32_t>(stablePropertyIndex);
        outXamlProperty = m_memberVector[runtimeIndex];

        // For known properties, we know the type, so it's safe to put it into the known lookup table
        if (!outXamlProperty)
        {
            auto knownPropertyIndex = GetKnownPropertyIndex(stablePropertyIndex);
            const auto propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(knownPropertyIndex);
            IFC_RETURN(GetXamlProperty(inPropertyToken, XamlTypeToken::FromType(propertyBase->GetPropertyType()), outXamlProperty));
        }
    }
    else
    {
        auto pos = m_mapMasterMemberList.find(inPropertyToken);
        if (pos != m_mapMasterMemberList.end())
        {
            outXamlProperty = pos->second;
        }
    }
    IFCEXPECT_RETURN(outXamlProperty);

    return S_OK;
}

// Gets a XamlProperty from the fully-qualified property name.
// This property name should be in the form [ns-uri]Type.Property.
_Check_return_ HRESULT XamlSchemaContext::GetXamlProperty(
    _In_ const xstring_ptr& spFullyQualifiedPropertyName,
    _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
    )
{
    outXamlProperty.reset();

    if (spFullyQualifiedPropertyName.IsNullOrEmpty())
    {
        IFC_RETURN(E_FAIL);
    }

    {
        auto itXamlProperty = m_mapFullyQualifiedProperty.find(spFullyQualifiedPropertyName);
        if (itXamlProperty == m_mapFullyQualifiedProperty.end())
        {
            xstring_ptr spNamespaceName;
            xstring_ptr spTypeName;
            xstring_ptr spPropertyName;
            std::shared_ptr<XamlNamespace> spXamlNamespace;
            std::shared_ptr<XamlType> spXamlType;

            IFC_RETURN(GetFullyQualifiedNameParts(spFullyQualifiedPropertyName, &spNamespaceName, &spTypeName, &spPropertyName));

            if (spNamespaceName.IsNullOrEmpty() || spTypeName.IsNullOrEmpty() || spPropertyName.IsNullOrEmpty())
            {
                IFC_RETURN(E_FAIL);
            }

            IFC_RETURN(GetXamlXmlNamespace(spNamespaceName, spXamlNamespace));
            if (spXamlNamespace)
            {
                IFC_RETURN(spXamlNamespace->GetXamlType(spTypeName, spXamlType));
                if (spXamlType)
                {
                    IFC_RETURN(spXamlType->GetProperty(spPropertyName, outXamlProperty));
                    {
                        if (outXamlProperty)
                        {
                            VERIFY_COND(m_mapFullyQualifiedProperty.insert({ spFullyQualifiedPropertyName, outXamlProperty }), .second);
                        }
                    }
                }
            }
        }
        else
        {
            outXamlProperty = itXamlProperty->second;
        }

        IFCCHECK_RETURN(outXamlProperty);
    }

    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlProperty(
    _In_ const XamlPropertyToken& inPropertyToken,
    _In_ const XamlTypeToken& inPropertyTypeToken,
    _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
    )
{
    RRETURN(GetXamlProperty(inPropertyToken, inPropertyTypeToken, xstring_ptr(), outXamlProperty));
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlProperty(
    _In_ const XamlPropertyToken& inPropertyToken,
    _In_ const XamlTypeToken& inPropertyTypeToken,
    _In_ const xstring_ptr& inPropertyName,
    _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
    )
{
    ASSERT(m_memberVector.size() >= StableXbfPropertyCount);

    outXamlProperty.reset();

    if (IsStableXbfProperty(inPropertyToken))
    {
        auto runtimeIndex = static_cast<uint32_t>(GetStableXbfPropertyIndex(inPropertyToken.GetHandle()));
        outXamlProperty = m_memberVector[runtimeIndex];
    }
    else
    {
        auto pos = m_mapMasterMemberList.find(inPropertyToken);
        if (pos != m_mapMasterMemberList.end())
        {
            outXamlProperty = pos->second;
        }
    }

    if (!outXamlProperty)
    {
        if (!inPropertyToken.IsUnknown())
        {
            try
            {
                std::shared_ptr<XamlType> propertyType;
                size_t runtimeIndex = -1;

                // Get XamlTypeNamespace for property type
                std::shared_ptr<XamlTypeNamespace> xamlTypeNamespace;

                // Unit tests don't fully set up the metadata tables. Two specific tests (XamlOptimizedNodeListUnitTests::GetNormalizedCountForOldDictionary
                // and XamlSchemaContextUnitTests::XamlPropertiesInSchema) will trip because they invoke this method with an artificially constructed
                // custom XamlType that has not gone through the full type registration process. Because the type itself is tangential to what is
                // being verified, this hook exists to avoid some of the run-time validation that would otherwise fail.
                if (!get_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty())
                {
                    std::shared_ptr<XamlTypeInfoProvider> typeInfoProvider;
                    XamlTypeNamespaceToken xamlTypeNamespaceToken;
                    xstring_ptr xamlTypeNamespaceName;

                    THROW_IF_FAILED(GetTypeInfoProvider(inPropertyTypeToken.GetProviderKind(), typeInfoProvider));
                    THROW_IF_FAILED(typeInfoProvider->GetTypeNamespaceForType(inPropertyTypeToken, xamlTypeNamespaceToken, &xamlTypeNamespaceName));
                    THROW_IF_FAILED(GetXamlTypeNamespace(xamlTypeNamespaceToken, xamlTypeNamespaceName, std::shared_ptr<XamlAssembly>(), std::shared_ptr<XamlXmlNamespace>(), xamlTypeNamespace));
                }

                THROW_IF_FAILED(GetXamlType(inPropertyTypeToken, xamlTypeNamespace, propertyType));
                auto xamlProperty = std::make_shared<XamlProperty>(shared_from_this(), inPropertyToken, inPropertyTypeToken, inPropertyName, propertyType);

                if (IsStableXbfProperty(inPropertyToken))
                {
                    runtimeIndex = static_cast<size_t>(GetStableXbfPropertyIndex(inPropertyToken.GetHandle()));
                    ASSERT(!m_memberVector[runtimeIndex]);
                    m_memberVector[runtimeIndex] = xamlProperty;
                }
                else
                {
                    m_memberVector.emplace_back(xamlProperty);
                    runtimeIndex = m_memberVector.size() - 1;
                    m_mapMasterMemberList.emplace(inPropertyToken, xamlProperty);
                }
                outXamlProperty = std::move(xamlProperty);
                outXamlProperty->SetRuntimeIndex(runtimeIndex);
            }
            CATCH_RETURN();
        }
    }
    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlDirective(
    _In_ const std::shared_ptr<XamlNamespace>& inXamlNamespace,
    _In_ const xstring_ptr_view& inssName,
    _Out_ std::shared_ptr<DirectiveProperty>& outXamlProperty
    )
{
    return inXamlNamespace->GetDirectiveProperty(inssName, outXamlProperty);
}

// Gets the XamlProperty from an index that was specified by the
// XamlSchemaContext at the time that the XamlType was created.
_Check_return_ const std::shared_ptr<XamlProperty>&
    XamlSchemaContext::GetXamlPropertyFromRuntimeIndex(
    _In_ const size_t uiIndex
    ) const
{
    return m_memberVector[uiIndex];
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlPropertyFromStableXbfIndex(
    _In_ StableXbfPropertyIndex index,
    _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
    )
{
    auto runtimeIndex = static_cast<std::underlying_type<StableXbfPropertyIndex>::type>(index);

    // GenXbf will always have the type tables corresponding to the SDK version
    // which means that it will consider properties introduced after TPMV to be "trusted".
    // However, if the generated XBF is then loaded on TPMV, the StableXbfPropertyIndex
    // for those "trusted" properties won't actually be valid. Therefore, we will create an UnknownProperty,
    // with as much information filled in as possible so that the XBF can be safely deserialized.
    // An error will be raised later by the BinaryFormatObjectWriter if conditional XAML syntax
    // wasn't used.
    if (runtimeIndex >= StableXbfPropertyCount)
    {
        auto result = m_unknownStableXbfPropertyIndexes.find(runtimeIndex);

        if (result == m_unknownStableXbfPropertyIndexes.end())
        {
            xstring_ptr propertyName;
            propertyName = StringCchPrintfWWrapper(L"StableXbfPropertyIndex::%d", runtimeIndex);
            std::shared_ptr<UnknownProperty> unknownProperty;
            IFC_RETURN(UnknownProperty::Create(shared_from_this(), propertyName, m_spUnknownType, FALSE, unknownProperty));
            outXamlProperty = unknownProperty;
        }
        else
        {
            outXamlProperty = result->second;
        }

        return S_OK;
    }

    auto& result = m_memberVector[static_cast<size_t>(runtimeIndex)];

    if (!result)
    {
        const auto propertyInfo = DirectUI::MetadataAPI::GetPropertyBaseByIndex(GetKnownPropertyIndex(index));
        result = std::make_shared<XamlProperty>(
            shared_from_this(),
            XamlPropertyToken::FromProperty(propertyInfo),
            XamlTypeToken::FromType(propertyInfo->GetPropertyType()),
            xstring_ptr(),
            std::shared_ptr<XamlType>());
        result->SetRuntimeIndex(static_cast<size_t>(runtimeIndex));
    }
    outXamlProperty = result;
    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::GetXamlTextSyntax(
    _In_ const XamlTypeToken& sTypeToken,
    _Out_ std::shared_ptr<XamlTextSyntax>& outXamlTextSyntax
    )
{
    HRESULT hr = S_OK;
    std::shared_ptr<XamlTextSyntax> spStringTextSyntax;

    //
    // Currently the only TextSyntax that's retrieved from a source other than
    // the type that it represents is the TextSyntax for string (it's used for
    // String and Object.  If this ceases to be the case then we'll actually
    // need to keep a map of tokens to TextSyntax instances so that we don't
    // have more than one TextSyntax for the same token.
    //
    // Something like this:
    //
    // IFC(m_mapMasterTextSyntaxList.Get(sTypeToken, outXamlTextSyntax));
    // if (hr == S_FALSE)
    // {
    //      // ...

    IFC(get_StringSyntax(spStringTextSyntax));

    if (spStringTextSyntax->get_TextSyntaxToken() == sTypeToken)
    {
        // *** note: if this is converted to a more generic lookup that
        // doesn't special-case StringTextSyntax, then the implementation of
        // get_StringSynax() should be changed to call GetXamlTextSyntax().
        outXamlTextSyntax = spStringTextSyntax;
    }
    else
    {
        outXamlTextSyntax = std::make_shared<XamlTextSyntax>(sTypeToken);
    }

Cleanup:
    if (FAILED(hr))
    {
        outXamlTextSyntax.reset();
    }
    RRETURN(hr);
}

#pragma endregion

#pragma region Schema Context APIs for retrieving TypeInfoProvider and Runtime based on Token.

_Check_return_ HRESULT XamlSchemaContext::GetTypeInfoProvider(
    _In_ XamlTypeInfoProviderKind tpk,
    _Out_ std::shared_ptr<XamlTypeInfoProvider>& outTypeInfoProvider
    )
{
    switch (tpk)
    {
    case tpkNative:
        if (!m_spNativeTypeInfoProvider)
        {
            m_spNativeTypeInfoProvider = std::make_shared<XamlNativeTypeInfoProvider>(shared_from_this());
        }

        outTypeInfoProvider = m_spNativeTypeInfoProvider;
        break;
    case tpkManaged:
        if (!m_spManagedTypeInfoProvider)
        {
            m_spManagedTypeInfoProvider = std::make_shared<XamlManagedTypeInfoProvider>(shared_from_this());
        }

        outTypeInfoProvider = m_spManagedTypeInfoProvider;
        break;
    default:
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}
#pragma endregion

#pragma region Schema Context Cached Properties and Types

// Get the x:Key property
_Check_return_ HRESULT XamlSchemaContext::get_X_KeyProperty(
    _Out_ std::shared_ptr<DirectiveProperty>& rspOut
    )
{
    if (!m_spXKeyProperty)
    {
        std::shared_ptr<XamlNamespace> spDirectiveNamespace;

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strKey, L"Key");

        IFC_RETURN(GetXamlXmlNamespace(c_strMarkupUri, spDirectiveNamespace));
        IFC_RETURN(GetXamlDirective(spDirectiveNamespace, c_strKey, m_spXKeyProperty));
    }

    rspOut = m_spXKeyProperty;

    return S_OK;
}

// Get the x:Name property
_Check_return_ HRESULT XamlSchemaContext::get_X_NameProperty(
    _Out_ std::shared_ptr<DirectiveProperty>& rspOut
    )
{
    if (!m_spXNameProperty)
    {
        std::shared_ptr<XamlNamespace> spDirectiveNamespace;

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strName, L"Name");

        IFC_RETURN(GetXamlXmlNamespace(c_strMarkupUri, spDirectiveNamespace));
        IFC_RETURN(GetXamlDirective(spDirectiveNamespace, c_strName, m_spXNameProperty));
    }

    rspOut = m_spXNameProperty;

    return S_OK;
}

// Get the XamlType corresponding to the Windows.Foundation.String type.
_Check_return_ HRESULT XamlSchemaContext::get_StringXamlType(
    _Out_ std::shared_ptr<XamlType>& spStringType
    )
{
    if (!m_spStringType)
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strString, L"String");
        std::shared_ptr<XamlNamespace> spSystemNamespace;

        IFC_RETURN(GetXamlXmlNamespace(c_strUsingWindowsFoundation, spSystemNamespace));

        IFC_RETURN(spSystemNamespace->GetXamlType(c_strString, m_spStringType));
    }

    spStringType = m_spStringType;

    return S_OK;
}

// Get the XamlType corresponding to the Windows.Foundation.Int32 type.
_Check_return_ HRESULT XamlSchemaContext::get_Int32XamlType(
    _Out_ std::shared_ptr<XamlType>& spInt32Type
    )
{
    if (!m_spInt32Type)
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strInt32, L"Int32");
        std::shared_ptr<XamlNamespace> spSystemNamespace;

        IFC_RETURN(GetXamlXmlNamespace(c_strUsingWindowsFoundation, spSystemNamespace));

        IFC_RETURN(spSystemNamespace->GetXamlType(c_strInt32, m_spInt32Type));
    }

    spInt32Type = m_spInt32Type;

    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::get_StringSyntax(
    _Out_ std::shared_ptr<XamlTextSyntax>& outStringSyntax
    )
{
    if (!m_spStringSyntax)
    {
        std::shared_ptr<XamlType> spStringType;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        XamlTypeToken ttTextSyntaxTypeToken;

        IFC_RETURN(get_StringXamlType(spStringType));

        IFCEXPECT_ASSERT_RETURN(spStringType != NULL);

        // if we provide a generic caching of XamlTextSyntax by token, then we should call
        // spStringType->get_TextSyntax(m_spStringSyntax) rather than doing it manually here.

        IFC_RETURN(GetTypeInfoProvider(spStringType->get_TypeToken().GetProviderKind(), spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->GetTextSyntaxForType(spStringType->get_TypeToken(), ttTextSyntaxTypeToken));
        m_spStringSyntax = std::make_shared<XamlTextSyntax>(ttTextSyntaxTypeToken);
    }

    outStringSyntax = m_spStringSyntax;

    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::get_Int32Syntax(
    _Out_ std::shared_ptr<XamlTextSyntax>& outInt32Syntax
    )
{
    if (!m_spInt32Syntax)
    {
        std::shared_ptr<XamlType> spInt32Type;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        XamlTypeToken ttTextSyntaxTypeToken;

        IFC_RETURN(get_Int32XamlType(spInt32Type));

        IFCEXPECT_ASSERT_RETURN(spInt32Type != NULL);

        // if we provide a generic caching of XamlTextSyntax by token, then we should call
        // spInt32Type->get_Int32Syntax(m_spInt32Syntax) rather than doing it manually here.

        IFC_RETURN(GetTypeInfoProvider(spInt32Type->get_TypeToken().GetProviderKind(), spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->GetTextSyntaxForType(spInt32Type->get_TypeToken(), ttTextSyntaxTypeToken));
        m_spInt32Syntax = std::make_shared<XamlTextSyntax>(ttTextSyntaxTypeToken);
    }

    outInt32Syntax = m_spInt32Syntax;

    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::get_InitializationProperty(
    _Out_ std::shared_ptr<ImplicitProperty>& outInitializationProperty
    )
{
    if (!m_spInitializationProperty)
    {
        std::shared_ptr<ImplicitProperty> spInitializationProperty;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strImplicitInitialization, L"__implicit_initialization");
        IFC_RETURN(ImplicitProperty::Create(
                    shared_from_this(),
                    iptInitialization,
                    c_strImplicitInitialization,
                    std::shared_ptr<XamlType>(),
                    std::shared_ptr<XamlNamespace>(),
                    FALSE,
                    spInitializationProperty));

        IFC_RETURN(RegisterXamlProperty(spInitializationProperty));
        m_spInitializationProperty = spInitializationProperty;
    }

    outInitializationProperty = m_spInitializationProperty;
    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::get_ItemsProperty(
    _In_ const std::shared_ptr<XamlType>& inCollectionType,
    _Out_ std::shared_ptr<ImplicitProperty>& outItemsProperty
    )
{
    if (!m_spItemsProperty)
    {
        std::shared_ptr<ImplicitProperty> implicitProperty;

        // TODO: Temp: Is there a real name that can go here?
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strImplicitItems, L"__implicit_items");

        // TODO: We don't have a namespace in this context?
        IFC_RETURN(ImplicitProperty::Create(
                    shared_from_this(),
                    iptItems,
                    c_strImplicitItems,
                    std::shared_ptr<XamlType>(),
                    std::shared_ptr<XamlNamespace>(),
                    FALSE,
                    implicitProperty));

        IFC_RETURN(RegisterXamlProperty(implicitProperty));

        m_spItemsProperty = implicitProperty;
    }

    outItemsProperty = m_spItemsProperty;

    return S_OK;
}

std::shared_ptr<XamlQualifiedObject> XamlSchemaContext::get_NullKeyedResourceXQO()
{
    if (!m_spNullKeyedResourceXQO)
    {
        m_spNullKeyedResourceXQO = std::make_shared<XamlQualifiedObject>();
        CValue nullValue;
        nullValue.SetNull();
        IFCFAILFAST(m_spNullKeyedResourceXQO->SetValue(XamlTypeToken(tpkNative, KnownTypeIndex::NullKeyedResource), nullValue));
    }

    return m_spNullKeyedResourceXQO;
}

// Initialize the parts of the schema context to prepare for
// type resolution and other schema queries
_Check_return_ HRESULT XamlSchemaContext::Initialize()
{
    // Pre-allocate enough room for known types/properties, leaving some slack at the end for a small number of customs
    try
    {
        m_typeVector.reserve(StableXbfTypeCount + 32);
        m_memberVector.reserve(StableXbfPropertyCount + 128);
        m_typeVector.resize(StableXbfTypeCount);
        m_memberVector.resize(StableXbfPropertyCount);

        InitializeSpecialXmlNamespaceMap();
        THROW_IF_FAILED(InitializeXamlNamespaceDefinitions());

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_unknownTypeName, L"UnknownType");
        IFC_RETURN(UnknownType::Create(shared_from_this(), std::shared_ptr<XamlNamespace>(), c_unknownTypeName, m_spUnknownType));
    }
    CATCH_RETURN();
    return S_OK;
}

// Initialize Known Namespaces that that we know from the static metadata
// which map to Microsoft.UI.Xaml.dll
_Check_return_ HRESULT XamlSchemaContext::InitializeXamlNamespaceDefinitions()
{
    HRESULT hr = S_OK;
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSystemWindows, L"System.Windows");
    XamlAssemblyToken assemblyToken;

    ENTERSECTION(InitializeSchemaContext);

    // Add an assembly entry for System.Windows
    assemblyToken.SetProviderKind(tpkNative);
    assemblyToken.SetHandle(1);

    IFC(AddAssembly(assemblyToken, c_strSystemWindows));

    //
    // For each Xml-Namespace that system.windows supports
    //
    for (XUINT32 uiXmlNamespaceId = 0; uiXmlNamespaceId < ARRAY_SIZE(sa_xmlnsDefinitions); uiXmlNamespaceId++)
    {
        const KnownNamespaceIndex *puiNamespaceIds = sa_xmlnsDefinitions[uiXmlNamespaceId].pNamespaceIndices;
        XUINT32 cNamespaceIds = sa_xmlnsDefinitions[uiXmlNamespaceId].cNamespaceIndices;

        for (XUINT32 i = 0; i < cNamespaceIds; i++)
        {
            XUINT32 iNamespaceIndex = static_cast<XUINT32>(puiNamespaceIds[i]);
            XamlTypeNamespaceToken typeNamespaceToken;
            typeNamespaceToken.SetProviderKind(tpkNative);
            typeNamespaceToken.SetHandle(c_aNamespaces[iNamespaceIndex].m_nIndex);
            typeNamespaceToken.AssemblyToken = assemblyToken;

            IFC(AddAssemblyXmlnsDefinition(
                assemblyToken,
                XSTRING_PTR_FROM_STORAGE(sa_xmlnsDefinitions[uiXmlNamespaceId].strNamespaceUriStorage),
                typeNamespaceToken,
                xstring_ptr(c_aNamespaces[iNamespaceIndex].m_strNameStorage)));
        }
    }

Cleanup:
    LEAVESECTION(InitializeSchemaContext);
    RRETURN(hr);
}

// Initialize Special Namespaces that don't map to type information
// in any assembly.
void XamlSchemaContext::InitializeSpecialXmlNamespaceMap()
{
    std::shared_ptr<XamlSpecialXmlNamespace> spXmlNamespace;
    std::shared_ptr<XamlMarkupXmlNamespace> spMarkupNamespace;

    spXmlNamespace = std::make_shared<XamlSpecialXmlNamespace>(shared_from_this(), c_strXmlUri);
    m_xmlNamespaceVector.emplace_back(spXmlNamespace);
    spXmlNamespace->SetRuntimeIndex(m_xmlNamespaceVector.size() - 1);

    spMarkupNamespace = std::make_shared<XamlMarkupXmlNamespace>(shared_from_this(), c_strMarkupUri);
    m_xmlNamespaceVector.emplace_back(spMarkupNamespace);
    spMarkupNamespace->SetRuntimeIndex(m_xmlNamespaceVector.size() - 1);

    // Clear the existing mapping of Uri-to-Namespace
    m_mapUriToXmlNamespace.clear();

    m_mapUriToXmlNamespace.insert({ c_strXmlUri, spXmlNamespace });
    m_mapUriToXmlNamespace.insert({ c_strMarkupUri, spMarkupNamespace });
}

#pragma endregion

#pragma region External Services: Core, ErrorService

CCoreServices* XamlSchemaContext::GetCore()
{
    ASSERT(m_pCore);
    return static_cast<CCoreServices*>(m_pCore);
}

_Check_return_ HRESULT XamlSchemaContext::GetErrorService(
    _Out_ std::shared_ptr<ParserErrorReporter>& outErrorService
    )
{
    outErrorService = m_ErrorService;
    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::SetErrorService(
    _In_ const std::shared_ptr<ParserErrorReporter>& inErrorService
    )
{
    m_ErrorService = inErrorService;
    return S_OK;
}

#pragma endregion

#pragma region Miscellaneous

_Check_return_ HRESULT XamlSchemaContext::CheckPeerType(
    _In_ const std::shared_ptr<XamlQualifiedObject>& inQO,
    _In_ const xstring_ptr& inssClassName,
    _In_ const bool bIsExactMatchRequired
    )
{
    CDependencyObject* pDO = inQO->GetDependencyObject();
    IFCPTR_RETURN(pDO);
    IFC_RETURN(FxCallbacks::FrameworkCallbacks_CheckPeerType(pDO, inssClassName, bIsExactMatchRequired));

    return S_OK;
}

#pragma endregion

#pragma region Helpers

_Check_return_ HRESULT XamlSchemaContext::FlushXamlNodeStreamCacheManager()
{
    std::shared_ptr<XamlNodeStreamCacheManager> spXamlNodeStreamCacheManager;

    IFC_RETURN(m_pCore->GetXamlNodeStreamCacheManager(spXamlNodeStreamCacheManager));
    if (spXamlNodeStreamCacheManager)
    {
        spXamlNodeStreamCacheManager->Flush();
    }

    return S_OK;
}

//  Breaks a fully qualified name in the format [ns-uri]Type.Property
//  down to its constituent parts.
//
//  Notes:
//      -- not having the brackets ([,]) is an error, but if there is no
//         '.' then the remainder after the bracket is assumed to be the
//         type name, and that there is no property.
_Check_return_ HRESULT XamlSchemaContext::GetFullyQualifiedNameParts(
    _In_ const xstring_ptr& spFullyQualifiedName,
    _Out_ xstring_ptr* pstrOutNamespaceUri,
    _Out_ xstring_ptr* pstrOutTypeName,
    _Out_ xstring_ptr* pstrOutPropertyName
    )
{
    pstrOutNamespaceUri->Reset();
    pstrOutTypeName->Reset();
    pstrOutPropertyName->Reset();

    if (spFullyQualifiedName.GetCount() < 1
        || spFullyQualifiedName.GetBuffer()[0] != L'[')
    {
        IFC_RETURN(E_FAIL);
    }

    auto ichRightBracket = spFullyQualifiedName.FindChar(L']');
    if (ichRightBracket == xstring_ptr_view::npos)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(spFullyQualifiedName.SubString(1, ichRightBracket, pstrOutNamespaceUri));

    auto ichDot = spFullyQualifiedName.FindChar(L'.', ichRightBracket + 1);
    if (ichDot != xstring_ptr_view::npos)
    {
        IFC_RETURN(spFullyQualifiedName.SubString(ichRightBracket + 1, ichDot, pstrOutTypeName));
        IFC_RETURN(spFullyQualifiedName.SubString(ichDot + 1, spFullyQualifiedName.GetCount(), pstrOutPropertyName));
    }
    else
    {
        IFC_RETURN(spFullyQualifiedName.SubString(ichRightBracket + 1, spFullyQualifiedName.GetCount(), pstrOutTypeName));
        pstrOutPropertyName->Reset();
    }

    return S_OK;
}

_Check_return_ HRESULT XamlSchemaContext::ExtractTypeNamespace(
    _In_ const xstring_ptr& rstrXmlNs,
    _Out_ xstring_ptr& strOutTypeNamespace
    )
{
    if (rstrXmlNs.StartsWith(c_strUsing, xstrCompareCaseSensitive))
    {
        IFC_RETURN(rstrXmlNs.SubString(
            c_strUsing.GetCount(),
            rstrXmlNs.GetCount(),
            &strOutTypeNamespace));
    }
    else
    {
        // All other cases fall out here and we return a copy of the original string.
        strOutTypeNamespace = rstrXmlNs;
    }

    return S_OK;
}

#pragma endregion

