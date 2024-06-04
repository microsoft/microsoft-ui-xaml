// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a xaml type. A layer of abstraction above the native
//      and managed type system.

#include "precomp.h"
#include "XamlTypeNamespace.h"

//  TODO: - we end up looking up the TypeInfoProvider quite
//  a bit, would it just be better to either ...
//
//      a) have the TypeInfoProvider as a member-variable
//       - or -
//      b) Add members to XamlSchemaContext and have *it* un-pack
//          the correct TypeInfoProvider, etc.

HRESULT XamlType::CreateInstance(
    _Out_ std::shared_ptr<XamlQualifiedObject>& rqoOut
    )
{
    HRESULT hr = S_OK;
    TraceCreateInstanceBegin();
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;
    
    IFC(GetSchemaContext(spXamlSchemaContext));
    IFC(spXamlSchemaContext->GetRuntime(m_sTypeToken.GetRuntimeKind(), spXamlRuntime));
    IFC(spXamlRuntime->CreateInstance(m_sTypeToken, rqoOut));
    
Cleanup:
    if (EventEnabledCreateInstanceEnd())
    {
        xstring_ptr fullName;
        get_FullName(&fullName);
        TraceCreateInstanceEnd(m_sTypeToken.GetRuntimeKind(), fullName.GetBuffer());
    }
    RRETURN(hr);
}


HRESULT XamlType::CreateFromValue(
    _In_ std::shared_ptr<XamlServiceProviderContext> spTextSyntaxContext,  // NOTE: This is deliberately no const-ref because is cast
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue,
    _In_ bool fIsPropertyAssignment,
    _Out_ std::shared_ptr<XamlQualifiedObject>& rqoOut
    )
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;
    std::shared_ptr<XamlQualifiedObject> qoRootInstance; 

    IFC_RETURN(GetSchemaContext(spXamlSchemaContext));
    
    IFC_RETURN(spXamlSchemaContext->GetRuntime(spTextSyntax->get_TextSyntaxToken().GetRuntimeKind(), spXamlRuntime));
    
    IFC_RETURN(spXamlRuntime->CreateFromValue(spTextSyntaxContext, 
                                            spTextSyntax->get_TextSyntaxToken(), 
                                            qoValue, 
                                            XamlPropertyToken(),
                                            qoRootInstance,
                                            fIsPropertyAssignment,
                                            rqoOut));

    return S_OK;
}

HRESULT XamlType::get_Name(_Out_ xstring_ptr* pstrOutName)
{
    if (m_spName.IsNull())
    {
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        
        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
        
        // If it's a valid type, this should be valid. If it was an unknown, m_spName should 
        // have already been set, and we wouldn't be in this branch.
        ASSERT(!!spTypeInfoProvider);
        IFC_RETURN(spTypeInfoProvider->GetTypeName(m_sTypeToken, &m_spName));
    }
    
    ASSERT(!m_spName.IsNull());
    *pstrOutName = m_spName;
    
    return S_OK;
}

// Gets the full name of the type (i.e. Namespace.Type).
HRESULT XamlType::get_FullName(_Out_ xstring_ptr* pstrOutFullName)
{
    if (m_spFullName.IsNull())
    {
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        
        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
        
        ASSERT(!!spTypeInfoProvider);
        IFC_RETURN(spTypeInfoProvider->GetTypeFullName(m_sTypeToken, &m_spFullName));
    }
    
    ASSERT(!m_spFullName.IsNull());
    *pstrOutFullName = m_spFullName;
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsUnknown
//
//  Synopsis:
//      Whether the type is unknown. We create instances of UnknownType
//      to represent types we didn't know.
//
//------------------------------------------------------------------------
bool XamlType::IsUnknown() const
{
    return (m_sTypeToken.GetHandle() == KnownTypeIndex::UnknownType);
}

HRESULT XamlType::IsPublic(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbIsPublic, outValue));
}

HRESULT XamlType::IsConstructible(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbConstructible, outValue));
}

HRESULT XamlType::IsString(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbString, outValue));
}

HRESULT XamlType::IsCollection(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbCollection, outValue));
}

HRESULT XamlType::IsDictionary(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbDictionary, outValue));
}

HRESULT XamlType::IsMarkupExtension(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbMarkupExtension, outValue));
}

HRESULT XamlType::IsTemplate(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbTemplate, outValue));
}

HRESULT XamlType::IsDirective(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbIsDirective, outValue));
}

HRESULT XamlType::IsISupportInitialize(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbIsISupportInitialize, outValue));
}

HRESULT XamlType::TrimSurroundingWhitespace(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbTrimSurroundingWhitespace, outValue));
}

HRESULT XamlType::IsWhitespaceSignificantCollection(bool& outValue)
{
    RRETURN(GetBoolTypeBit(btbWhitespaceSignificantCollection, outValue));
}

HRESULT XamlType::get_TextSyntax(std::shared_ptr<XamlTextSyntax>& outTextSyntax)
{
    if (!m_spXamlTextSyntax)
    {
        XamlTypeToken ttTextSyntaxTypeToken;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;

        IFC_RETURN(GetSchemaContext(spSchemaContext));

        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->GetTextSyntaxForType(m_sTypeToken, ttTextSyntaxTypeToken));
        IFC_RETURN(spSchemaContext->GetXamlTextSyntax(ttTextSyntaxTypeToken, m_spXamlTextSyntax));
    }

    outTextSyntax = m_spXamlTextSyntax;
    
    return S_OK;
}

HRESULT XamlType::get_ContentProperty(
    std::shared_ptr<XamlProperty>& outContentProperty
    )
{
    // We sometimes want to cache that there is no content property.
    if (!m_spContentProperty && m_bHasContentProperty)
    {
        XamlPropertyToken tpProperty;
        XamlTypeToken ttPropertyType;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        xstring_ptr ssPropertyName;

        IFC_RETURN(GetSchemaContext(spSchemaContext));
        if (FAILED(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider)))
        {
            m_bHasContentProperty = FALSE;
        }
        else
        {
            IFC_RETURN(spTypeInfoProvider->GetContentProperty(m_sTypeToken, tpProperty, ttPropertyType));

            if (!tpProperty.IsUnknown())
            {
                if (m_sTypeToken.GetProviderKind() != tpProperty.GetProviderKind())
                {
                    IFC_RETURN(spSchemaContext->GetTypeInfoProvider(tpProperty.GetProviderKind(), spTypeInfoProvider));
                }

                IFC_RETURN(spTypeInfoProvider->GetPropertyName(tpProperty, &ssPropertyName));
                IFC_RETURN(spSchemaContext->GetXamlProperty(tpProperty, ttPropertyType, ssPropertyName, m_spContentProperty));
            }
            else
            {
                m_bHasContentProperty = FALSE;
            }
        }
    }

    outContentProperty = m_spContentProperty;
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_ContentWrapper
//
//  Synopsis:
//      Collection types may optionally provide a type used to wrap literal
//      content from the parser before inserting into the collection.  The
//      parser won't actually wrap the content, but merely know it's safe to
//      pass to the collection which will handle it.
//      
//      For the native side, this means the Append override that takes a CValue
//      should start handling strings.  On the managed side, once implemented,
//      this would mean your collection should have an Add overload that takes
//      the desired type (a la InlineCollection.Add(string text)).
//
//------------------------------------------------------------------------
HRESULT XamlType::get_ContentWrapper(
    _Out_opt_ std::shared_ptr<XamlType>& spContentWrapperType)
{
    if (!m_spContentWrapper)
    {
        XamlTypeToken ttContentWrapper;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        
        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->GetContentWrapper(m_sTypeToken, ttContentWrapper));
        IFC_RETURN(spSchemaContext->GetXamlType(ttContentWrapper, m_spContentWrapper));
    }

    spContentWrapperType = m_spContentWrapper;
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_RuntimeNameProperty
//
//  Synopsis:
//      Gets the Property that should be treated as the equivalent of x:Name
//      and that the parser should set when it sees an x:Name property in 
//      the Xaml
//
//------------------------------------------------------------------------
HRESULT XamlType::get_RuntimeNameProperty(std::shared_ptr<XamlProperty>& outRuntimeNameProperty)
{
    if (!m_spRuntimeName)
    {
        XamlPropertyToken tpProperty;
        XamlTypeToken ttPropertyType;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        xstring_ptr ssPropertyName;

        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->GetRuntimeNameProperty(m_sTypeToken, tpProperty, ttPropertyType));

        if (!tpProperty.IsEmpty())
        {
            if (m_sTypeToken.GetProviderKind() != tpProperty.GetProviderKind())
            {
                spTypeInfoProvider.reset();
                IFC_RETURN(spSchemaContext->GetTypeInfoProvider(tpProperty.GetProviderKind(), spTypeInfoProvider));
            }
            
            IFC_RETURN(spTypeInfoProvider->GetPropertyName(tpProperty, &ssPropertyName));
            IFC_RETURN(spSchemaContext->GetXamlProperty(tpProperty, ttPropertyType, ssPropertyName, m_spRuntimeName));
        }
    }

    outRuntimeNameProperty = m_spRuntimeName;
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_XmlLangProperty
//
//  Synopsis:
//      Gets the Property for this Type that should be treated as the 
//      equivalent of the xml:lang property, and should set when it sees
//      an xml:lang property in the Xaml.
//
//------------------------------------------------------------------------
HRESULT XamlType::get_XmlLangProperty(std::shared_ptr<XamlProperty>& outXmlLangProperty)
{
    XamlPropertyToken tpProperty;
    XamlTypeToken ttPropertyType;
    std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    xstring_ptr ssPropertyName;

    IFC_RETURN(GetSchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
    IFC_RETURN(spTypeInfoProvider->GetXmlLangProperty(m_sTypeToken, tpProperty, ttPropertyType));

    if (!tpProperty.IsEmpty())
    {
        IFC_RETURN(spTypeInfoProvider->GetPropertyName(tpProperty, &ssPropertyName));
        IFC_RETURN(spSchemaContext->GetXamlProperty(tpProperty, ttPropertyType, ssPropertyName, outXmlLangProperty));
    }
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_DictionaryKeyProperty
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlType::get_DictionaryKeyProperty(std::shared_ptr<XamlProperty>& outDictionaryKeyProperty)
{
    XamlPropertyToken tpProperty;
    XamlTypeToken ttPropertyType;
    std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    xstring_ptr ssPropertyName;

    IFC_RETURN(GetSchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
    IFC_RETURN(spTypeInfoProvider->GetDictionaryKeyProperty(m_sTypeToken, tpProperty, ttPropertyType));

    if (!tpProperty.IsEmpty())
    {
        if (m_sTypeToken.GetProviderKind() != tpProperty.GetProviderKind())
        {
            spTypeInfoProvider.reset();
            IFC_RETURN(spSchemaContext->GetTypeInfoProvider(tpProperty.GetProviderKind(), spTypeInfoProvider));
        }
    
        IFC_RETURN(spTypeInfoProvider->GetPropertyName(tpProperty, &ssPropertyName));
        IFC_RETURN(spSchemaContext->GetXamlProperty(tpProperty, ttPropertyType, ssPropertyName, outDictionaryKeyProperty));
    }
        
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   get_BaseType
//
//  Synopsis:
//      If the passed type is derived directly or indirectly from a core
//      Silverlight type, then return the name of that type.
// 
//  TODO: Rename this member.  The name is misleading.
//------------------------------------------------------------------------
//
HRESULT XamlType::get_BaseType(
    std::shared_ptr<XamlType>& outBaseType
    )
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    IFC_RETURN(GetSchemaContext(spSchemaContext));    

    if (!m_flagsNonBoolTypeValidBits.IsBitSet(nbtbBaseType))
    {
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;

        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->GetBaseType(m_sTypeToken, m_sBaseTypeToken));
        
        m_flagsNonBoolTypeValidBits.SetBit(nbtbBaseType);
    }

    if (!m_sBaseTypeToken.IsEmpty())
    {
        IFC_RETURN(spSchemaContext->GetXamlType(m_sBaseTypeToken, outBaseType));
    }
    else
    {
        outBaseType.reset();
    }
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_MarkupExtensionReturnType
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlType::get_MarkupExtensionReturnType(std::shared_ptr<XamlType>& outReturnType)
{
    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
//  Method:   get_MarkupExtensionExpressionType
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlType::get_MarkupExtensionExpressionType(std::shared_ptr<XamlType>& outExpressionType)
{
    RRETURN(E_NOTIMPL);
}

//  Return the collection item type.
HRESULT XamlType::get_CollectionItemType(std::shared_ptr<XamlType>& collectionItemType)
{
    // get associated XamlTypeInfoProvider of XamlType
    std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;

    IFC_RETURN(GetSchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));

    XamlTypeToken itemTypeToken;
    IFC_RETURN(spTypeInfoProvider->GetCollectionItemType(m_sTypeToken, itemTypeToken));

    // get XamlType from XamlTypeToken
    IFC_RETURN(spSchemaContext->GetXamlType(itemTypeToken, collectionItemType));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetProperty
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlType::GetProperty(
    _In_ const xstring_ptr& inPropertyName, 
    std::shared_ptr<XamlProperty>& outProperty
    )
{
    if (!m_sTypeToken.IsEmpty())
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        XamlPropertyAndTypeToken tPropertyAndTypeToken;

        ASSERT(inPropertyName.GetCount() > 0);

        IFC_RETURN(GetSchemaContext(spSchemaContext));

        if (TryGetPropertyFromCache(inPropertyName, tPropertyAndTypeToken) != S_OK)
        {
            std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;

            IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
            IFC_RETURN(spTypeInfoProvider->ResolvePropertyName(
                        m_sTypeToken, 
                        inPropertyName, 
                        tPropertyAndTypeToken.m_tpProperty, 
                        tPropertyAndTypeToken.m_ttPropertyType));
            IFC_RETURN(AddPropertyToCache(inPropertyName, tPropertyAndTypeToken));
        }

        IFC_RETURN(spSchemaContext->GetXamlProperty(
                    tPropertyAndTypeToken.m_tpProperty, 
                    tPropertyAndTypeToken.m_ttPropertyType, 
                    inPropertyName, 
                    outProperty));
    }

    return S_OK;
}

HRESULT XamlType::GetDependencyProperty(
    _In_ const xstring_ptr& inPropertyName, 
    std::shared_ptr<XamlProperty>& outProperty
    )
{
    if (!m_sTypeToken.IsEmpty())
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        XamlPropertyAndTypeToken tPropertyAndTypeToken;

        ASSERT(inPropertyName.GetCount() > 0);

        IFC_RETURN(GetSchemaContext(spSchemaContext));

        if (TryGetDependencyPropertyFromCache(inPropertyName, tPropertyAndTypeToken) != S_OK)
        {
            std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;

            IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
            IFC_RETURN(spTypeInfoProvider->ResolveDependencyPropertyName(
                        m_sTypeToken, 
                        inPropertyName, 
                        tPropertyAndTypeToken.m_tpProperty, 
                        tPropertyAndTypeToken.m_ttPropertyType));
            IFC_RETURN(AddDependencyPropertyToCache(inPropertyName, tPropertyAndTypeToken));
        }

        IFC_RETURN(spSchemaContext->GetXamlProperty(
                    tPropertyAndTypeToken.m_tpProperty, 
                    tPropertyAndTypeToken.m_ttPropertyType, 
                    inPropertyName, 
                    outProperty));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetXamlNamespace
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlType::GetXamlNamespace(std::shared_ptr<XamlNamespace>& outNamespace)
{
    if (m_spXamlNamespace.expired())
    {
        std::shared_ptr<XamlTypeNamespace> spXamlTypeNamespace;
        
        if (!m_sTypeToken.IsEmpty())
        {
            std::shared_ptr<XamlSchemaContext> spSchemaContext;
            XamlTypeNamespaceToken sXamlTypeNamespaceToken;
            std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
            xstring_ptr spXamlTypeNamespaceName;
            std::shared_ptr<XamlAssembly> spXamlAssembly;
            XamlAssemblyToken sXamlAssemblyToken;
            
            IFC_RETURN(GetSchemaContext(spSchemaContext));
    
            IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
            IFC_RETURN(spTypeInfoProvider->GetTypeNamespaceForType(
                        m_sTypeToken, 
                        sXamlTypeNamespaceToken, 
                        &spXamlTypeNamespaceName));
            sXamlAssemblyToken = XamlAssemblyToken(sXamlTypeNamespaceToken.AssemblyToken.GetProviderKind(),
                                                   sXamlTypeNamespaceToken.AssemblyToken.GetHandle());
            
            IFC_RETURN(spSchemaContext->GetXamlAssembly(sXamlAssemblyToken, spXamlAssembly));
            
            IFC_RETURN(spSchemaContext->GetXamlTypeNamespace(sXamlTypeNamespaceToken, spXamlTypeNamespaceName, spXamlAssembly, std::shared_ptr<XamlXmlNamespace>(), spXamlTypeNamespace) );
        }
        m_spXamlNamespace = spXamlTypeNamespace;
    }
    
    outNamespace = m_spXamlNamespace.lock();
    if (!outNamespace)
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAttachableProperty
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlType::GetAttachableProperty(
    _In_ const xstring_ptr& inPropertyName, 
    std::shared_ptr<XamlProperty>& outProperty
    )
{
    // Return S_OK and null in outProperty if not found.
    // Many places fallback through a series of properties
    // when not found and try other things.
    // 
    // Not finding the property shouldn't return an error code.

    // TODO: This code seems redundant and like it doesn't work?
    IFC_RETURN(GetProperty(inPropertyName, outProperty));
    if (!outProperty || !outProperty->IsAttachable())
    {
        outProperty.reset();
    }
    
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   IsAssignableFrom
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlType::IsAssignableFrom(
    const std::shared_ptr<XamlType>& xamlType,
    _Out_ bool &bIsAssignableFrom)
{
    bIsAssignableFrom = FALSE;

    if (!xamlType->IsUnknown() && !IsUnknown())
    {
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;

        IFC_RETURN(GetSchemaContext(spSchemaContext));

        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(xamlType->m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->IsAssignableFrom(xamlType->m_sTypeToken, m_sTypeToken, bIsAssignableFrom));
    }

    return S_OK;
}

void XamlType::SetBoolTypeBits(const XamlBitSet<BoolTypeBits>& fTypeBits, const XamlBitSet<BoolTypeBits>& fValueBits)
{
    
    // Shouldn't be trying to set bits that aren't in the mask
    ASSERT(fTypeBits.AreBitsSet(fValueBits));
    
    // Clear every bit requesting to be set, so that setting bits to 0 works
    m_flagsBoolTypeBits.ClearBits(fTypeBits);

    // Set back to one any that were wanted.
    m_flagsBoolTypeBits.SetBits(fValueBits);

    // mark the bits as having been set.
    m_flagsBoolTypeValidBits.SetBits(fTypeBits);
}

//------------------------------------------------------------------------
//
//  Method:   GetBoolTypeBits
//
//  Synopsis:
//      Gets and caches values for properties that can be represented as
//      Booleans
//
//------------------------------------------------------------------------
//
HRESULT XamlType::GetBoolTypeBit(BoolTypeBits fTypeBit, bool& fValue)
{
    if (!m_flagsBoolTypeValidBits.IsBitSet(fTypeBit))
    {
        // TODO: optionally request a larger set of bits.
        XamlBitSet<BoolTypeBits> fRequestedBits(fTypeBit);
        IFC_RETURN(RetrieveBoolTypeBits(fRequestedBits));
        IFCEXPECT_RETURN(m_flagsBoolTypeValidBits.IsBitSet(fTypeBit));

        // Note: If the expectation that we've looked up your type info isn't
        // met for native types, be sure to check the
        // XamlNativeTypeInfoProvider::LookupTypeFlags method and verify that
        // your type bit is in the hard coded list it provides.
    }

    fValue = m_flagsBoolTypeBits.IsBitSet(fTypeBit);
    
    return S_OK;
}

HRESULT XamlType::RetrieveBoolTypeBits(const XamlBitSet<BoolTypeBits>& fTypeBits)
{
    if (!m_flagsBoolTypeValidBits.AreBitsSet(fTypeBits))
    {
        XamlBitSet<BoolTypeBits> fBitsChecked;
        XamlBitSet<BoolTypeBits> fReturnedValues;
        XamlBitSet<BoolTypeBits> fBitsToRequest = fTypeBits;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;

        IFCEXPECT_ASSERT_RETURN(!IsUnknown());

        // Clear the ones we already know.
        fBitsToRequest.ClearBits(m_flagsBoolTypeValidBits);

        if (!fBitsToRequest.AreAllZero())
        {
            IFC_RETURN(GetSchemaContext(spSchemaContext));   
            IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sTypeToken.GetProviderKind(), spTypeInfoProvider));
            IFC_RETURN(spTypeInfoProvider->LookupTypeFlags(m_sTypeToken, fBitsToRequest, fBitsChecked, fReturnedValues));
            SetBoolTypeBits(fBitsChecked, fReturnedValues);
        }
    }
    
    return S_OK;
}

void XamlType::SetBoolTypeBit(BoolTypeBits fTypeBit, bool bValue)
{
    m_flagsBoolTypeValidBits.SetBit(fTypeBit);

    if (bValue)
    {
        m_flagsBoolTypeBits.SetBit(fTypeBit);
    }
    else
    {
        m_flagsBoolTypeBits.ClearBit(fTypeBit);
    }
}

void XamlType::SetIsDirective()
{
    SetBoolTypeBit(btbIsDirective, TRUE);
}

HRESULT XamlType::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}


HRESULT UnknownType::Create(
        const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
        const std::shared_ptr<XamlNamespace>& inNamespace,
        _In_ const xstring_ptr& inTypeName,
        std::shared_ptr<XamlType>& outType)
{
    XamlTypeToken ttkNullToken;
    outType = std::make_shared<UnknownType>(spSchemaContext, inNamespace, ttkNullToken, inTypeName);
    return S_OK;
}


HRESULT KnownXamlType::Create(
    const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
    const std::shared_ptr<XamlNamespace>& inNamespace,
    _In_ const xstring_ptr& inTypeName,
    const XamlTypeToken sTypeToken,
    std::shared_ptr<XamlType>& outType
    )
{
    outType = std::make_shared<KnownXamlType>(spSchemaContext, inNamespace, sTypeToken, inTypeName);
    return S_OK;
}



