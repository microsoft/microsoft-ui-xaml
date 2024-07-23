// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   get_Name
//
//  Synopsis:
//      Get the name of a XAML member.
//
//------------------------------------------------------------------------
//
HRESULT XamlProperty::get_Name(_Out_ xstring_ptr* pstrOutName)
{
    if (m_spName.IsNull())
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        
        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sPropertyToken.GetProviderKind(), spTypeInfoProvider));
        IFC_RETURN(spTypeInfoProvider->GetPropertyName(m_sPropertyToken, &m_spName));
    }
    
    *pstrOutName = m_spName;
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_FullName
//
//  Synopsis:
//      Get the fully qualified name of the member, which is prefixed by the
//      fully qualified name of the member's declaring type.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlProperty::get_FullName(
    _Out_ xstring_ptr* pstrOutFullName)
{
    // Note: We don't currently cache the full name because it's only used for
    // error reporting
    
    xstring_ptr spDeclaringTypeName;
    xstring_ptr spName;
    XStringBuilder outFullNameBuilder;
    XUINT32 uiLength;
    
    // Get the names of the property and declaring type
    IFC_RETURN(get_Name(&spName));
    if (m_sPropertyToken.GetProviderKind() == tpkNative &&
        m_sPropertyToken.GetHandle() == KnownPropertyIndex::DependencyObject_Name)
    {
        // Note: We're special casing DependencyObject.Name
        // because it's an implementation detail we don't want to expose to
        // users.  See bug 86379.
        DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strWindowsUIXamlFrameworkElement, L"Microsoft.UI.Xaml.FrameworkElement");
        spDeclaringTypeName = c_strWindowsUIXamlFrameworkElement;
    }
    else
    {
        std::shared_ptr<XamlType> spDeclaringType;
        IFC_RETURN(get_DeclaringType(spDeclaringType));
        if (spDeclaringType)
        {
            IFC_RETURN(spDeclaringType->get_FullName(&spDeclaringTypeName));
        }
    }
    
    // Create a new string with room for the length of the declaring type name,
    // a period in the middle, and the property name
    uiLength =
        (!spDeclaringTypeName.IsNullOrEmpty() ? 1 + spDeclaringTypeName.GetCount() : 0) +
        (!spName.IsNullOrEmpty() ? spName.GetCount() : 0);

    IFC_RETURN(outFullNameBuilder.Initialize(uiLength));

    // Populate the string with the full type name
    if (!spDeclaringTypeName.IsNullOrEmpty())
    {
        IFC_RETURN(outFullNameBuilder.Append(spDeclaringTypeName));
        IFC_RETURN(outFullNameBuilder.AppendChar(L'.'));
    }
    IFC_RETURN(outFullNameBuilder.Append(spName));
    IFC_RETURN(outFullNameBuilder.DetachString(pstrOutFullName));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsImplicit
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsImplicit() const
{
    return false;
}

//------------------------------------------------------------------------
//
//  Method:   IsUnknown
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsUnknown()
{
    return m_sPropertyToken.IsUnknown();
}

//------------------------------------------------------------------------
//
//  Method:   IsPublic
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsPublic()
{
    bool bValue = false;
    VERIFYHR(GetBoolPropertyBit(bpbIsPublic, bValue));
    return bValue;
}

//------------------------------------------------------------------------
//
//  Method:   IsBrowsable
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsBrowsable()
{
    bool bValue = false;
    VERIFYHR(GetBoolPropertyBit(bpbIsBrowsable, bValue));
    return bValue;
}

//------------------------------------------------------------------------
//
//  Method:   IsReadOnly
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsReadOnly()
{
    bool bValue = false;
    VERIFYHR(GetBoolPropertyBit(bpbIsReadOnly, bValue));
    return bValue;
}

//------------------------------------------------------------------------
//
//  Method:   IsStatic
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsStatic()
{
    bool bValue = false;
    VERIFYHR(GetBoolPropertyBit(bpbIsStatic, bValue));
    return bValue;
}

//------------------------------------------------------------------------
//
//  Method:   IsAttachable
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsAttachable()
{
    bool bValue = false;
    VERIFYHR(GetBoolPropertyBit(bpbIsAttachable, bValue));
    return bValue;
}

//------------------------------------------------------------------------
//
//  Method:   IsEvent
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsEvent()
{
    bool bValue = false;
    VERIFYHR(GetBoolPropertyBit(bpbIsEvent, bValue));
    return bValue;
}

//------------------------------------------------------------------------
//
//  Method:   IsAmbient
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsAmbient()
{
    bool bValue = false;
    VERIFYHR(GetBoolPropertyBit(bpbIsAmbient, bValue));
    return bValue;
}

//------------------------------------------------------------------------
//
//  Method:   IsDirective
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
bool XamlProperty::IsDirective() const
{
    return false;
}

//------------------------------------------------------------------------
//
//  Method:   get_DeclaringType
//
//  Synopsis:
//      Gets the type that declares this property.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlProperty::get_DeclaringType(
    _Out_ std::shared_ptr<XamlType>& outDeclaringType)
{
    // Get a reference to the cached declaring type
    if (!m_spDeclaringType.expired())
    {
        outDeclaringType = m_spDeclaringType.lock();
        if (!outDeclaringType)
        {
            IFC_RETURN(E_FAIL);
        }
    }
    
    // If we don't have a declaring type and our tokens are known
    if (!outDeclaringType &&
        m_sPropertyToken.GetProviderKind() != tpkUnknown &&
        m_sPropertyTypeToken.GetHandle() != KnownTypeIndex::UnknownType)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        XamlTypeToken tDeclaringTypeToken;
        
        // Get the type info provider for our property
        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sPropertyToken.GetProviderKind(), spTypeInfoProvider));
        ASSERT(!!spTypeInfoProvider);
        
        // Get the declaring type
        IFC_RETURN(spTypeInfoProvider->GetDeclaringType(m_sPropertyToken, tDeclaringTypeToken));
        IFC_RETURN(spSchemaContext->GetXamlType(tDeclaringTypeToken, outDeclaringType));
        IFCEXPECT_RETURN(outDeclaringType);
        
        // Save a weak reference to the declaring type
        m_spDeclaringType = outDeclaringType;
    }
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_Type
//
//  Synopsis:
//      Get the type of the XamlProperty.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlProperty::get_Type(_Out_ std::shared_ptr<XamlType>& outType)
{
    // Get a reference to the cached type
    if (!m_spType.expired())
    {
        outType = m_spType.lock();
    }

    // If we don't have a type but do have a valid type token
    if (!outType &&
        m_sPropertyTypeToken.GetProviderKind() != tpkUnknown &&
        m_sPropertyTypeToken.GetHandle() != KnownTypeIndex::UnknownType)
    {
        // Lookup the type
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetXamlType(m_sPropertyTypeToken, outType));
        IFCEXPECT_RETURN(outType);

        m_spType = outType;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_TargetType
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlProperty::get_TargetType(std::shared_ptr<XamlType>& outTargetType)
{
    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
//  Method:   get_TextSyntax
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlProperty::get_TextSyntax(std::shared_ptr<XamlTextSyntax>& outTextSyntax)
{
    if (!m_spXamlTextSyntax)
    {
        XamlTypeToken ttTextSyntaxTypeToken;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;

        if (!IsEvent())
        {
            IFC_RETURN(GetSchemaContext(spSchemaContext));

            IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sPropertyToken.GetProviderKind(), spTypeInfoProvider));
            IFC_RETURN(spTypeInfoProvider->GetTextSyntaxForProperty(m_sPropertyToken, ttTextSyntaxTypeToken));
        }
        
        if (ttTextSyntaxTypeToken.GetProviderKind() != tpkUnknown && ttTextSyntaxTypeToken.GetHandle() != KnownTypeIndex::UnknownType)
        {
            IFC_RETURN(spSchemaContext->GetXamlTextSyntax(ttTextSyntaxTypeToken, m_spXamlTextSyntax));
        }
        else
        {
            std::shared_ptr<XamlType> spType;
            IFC_RETURN(get_Type(spType));
            IFC_RETURN(spType->get_TextSyntax(m_spXamlTextSyntax));
        }
    }

    outTextSyntax = m_spXamlTextSyntax;
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   DependsOn
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlProperty::DependsOn(std::shared_ptr<XamlProperty>& outDependsOn)
{
    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
//  Method:   GetValue
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
_Check_return_ HRESULT XamlProperty::GetValue(
    _In_ const XamlQualifiedObject& qoInstance,
    _Out_ XamlQualifiedObject& rqoOut
    )
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;
    
    IFC_RETURN(GetSchemaContext(spXamlSchemaContext));
    
    IFC_RETURN(spXamlSchemaContext->GetRuntime(m_sPropertyToken.GetRuntimeKind(), spXamlRuntime));
    IFC_RETURN(spXamlRuntime->GetValue(qoInstance, m_sPropertyToken, rqoOut));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAmbientValue
//
//  Synopsis:
//      Get ambient property value only if value exists. This is different
//      from GetValue which will force on demand properties to be created.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT
XamlProperty::GetAmbientValue(
    _In_ const XamlQualifiedObject& qoInstance,
    _Out_ XamlQualifiedObject& rqoOut
    )
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;
    
    IFC_RETURN(GetSchemaContext(spXamlSchemaContext));
    
    IFC_RETURN(spXamlSchemaContext->GetRuntime(m_sPropertyToken.GetRuntimeKind(), spXamlRuntime));
    IFC_RETURN(spXamlRuntime->GetAmbientValue(qoInstance, m_sPropertyToken, rqoOut));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   SetValue
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT
XamlProperty::SetValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance, 
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue,
    _In_ bool bBindTemplates
    )
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;
    
    IFC_RETURN(GetSchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetRuntime(m_sPropertyToken.GetRuntimeKind(), spXamlRuntime));
    IFC_RETURN(spXamlRuntime->SetValue(qoInstance, m_sPropertyToken, qoValue, bBindTemplates));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetBoolPropertyBits
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
void XamlProperty::SetBoolPropertyBits(const XamlBitSet<BoolPropertyBits>& fPropertyBits, const XamlBitSet<BoolPropertyBits>& fValueBits)
{
    // Shouldn't be trying to set bits that aren't in the mask
    ASSERT(fPropertyBits.AreBitsSet(fValueBits));
    
    // Clear every bit requesting to be set, so that setting bits to 0 works
    m_flagsBoolPropertyBits.ClearBits(fPropertyBits);

    // Set back to one any that were wanted.
    m_flagsBoolPropertyBits.SetBits(fValueBits);

    // mark the bits as having been set.
    m_flagsBoolPropertyValidBits.SetBits(fPropertyBits);
}

//------------------------------------------------------------------------
//
//  Method:   SetBoolPropertyBit
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
void XamlProperty::SetBoolPropertyBit(BoolPropertyBits fPropertyBit, bool bValue)
{
    m_flagsBoolPropertyValidBits.SetBit(fPropertyBit);

    if (bValue)
    {
        m_flagsBoolPropertyBits.SetBit(fPropertyBit);
    }
    else
    {
        m_flagsBoolPropertyBits.ClearBit(fPropertyBit);
    }
}

//------------------------------------------------------------------------
//
//  Method:   GetBoolPropertyBit
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlProperty::GetBoolPropertyBit(BoolPropertyBits fPropertyBit, bool& fValue)
{
    if (!m_flagsBoolPropertyValidBits.IsBitSet(fPropertyBit))
    {
        // TODO: optionally request a larger set of bits.
        XamlBitSet<BoolPropertyBits> fRequestedBits(fPropertyBit);
        IFC_RETURN(RetrieveBoolPropertyBits(fRequestedBits));
        IFCEXPECT_RETURN(m_flagsBoolPropertyValidBits.IsBitSet(fPropertyBit));
    }

    fValue = m_flagsBoolPropertyBits.IsBitSet(fPropertyBit);
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RetrieveBoolPropertyBits
//
//  Synopsis:
//
//------------------------------------------------------------------------
//
HRESULT XamlProperty::RetrieveBoolPropertyBits(const XamlBitSet<BoolPropertyBits>& fPropertyBits)
{
    if (!m_flagsBoolPropertyValidBits.AreBitsSet(fPropertyBits))
    {
        XamlBitSet<BoolPropertyBits> fBitsChecked;
        XamlBitSet<BoolPropertyBits> fReturnedValues;
        XamlBitSet<BoolPropertyBits> fBitsToRequest = fPropertyBits;
        std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;

        // Clear the ones we already know.
        fBitsToRequest.ClearBits(m_flagsBoolPropertyValidBits);

        // TODO: Can this be true since we already checked in the outer
        // if whether there were any unset.
        if (!fBitsToRequest.AreAllZero())
        {
            IFC_RETURN(GetSchemaContext(spSchemaContext));   
            IFC_RETURN(spSchemaContext->GetTypeInfoProvider(m_sPropertyToken.GetProviderKind(), spTypeInfoProvider));
            IFC_RETURN(spTypeInfoProvider->LookupPropertyFlags(m_sPropertyToken, fBitsToRequest, fBitsChecked, fReturnedValues));
            SetBoolPropertyBits(fBitsChecked, fReturnedValues);
        }
    }
    
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   GetSchemaContext
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT XamlProperty::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT UnknownProperty::Create(
            const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
            _In_ const xstring_ptr& inName,
            const std::shared_ptr<XamlType>& inAttachedOwnerType,
            bool bIsAttachable,
            std::shared_ptr<UnknownProperty>& outProperty)
{
    std::shared_ptr<XamlNamespace> spXamlNamespace;
    IFC_RETURN(Create(spSchemaContext, inName, inAttachedOwnerType, spXamlNamespace, bIsAttachable, outProperty));
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT UnknownProperty::Create(
            const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
            _In_ const xstring_ptr& inName,
            const std::shared_ptr<XamlType>& inAttachedOwnerType,
            const std::shared_ptr<XamlNamespace>& inNamespace,
            bool bIsAttachable,
            std::shared_ptr<UnknownProperty>& outProperty)
{
    XamlPropertyToken ptUnknownPropertyToken;
    
    outProperty = std::make_shared<UnknownProperty>(spSchemaContext, ptUnknownPropertyToken, inName, inAttachedOwnerType);
    outProperty->SetBoolPropertyBit(bpbIsAttachable, bIsAttachable);
    //  TODO: doing nothing with the inNamespace right nows
    
    // If the declaring type is unknown...
    if (inAttachedOwnerType)
    {
        if (inAttachedOwnerType->IsUnknown())
        {
            // ... save a reference on our unknown property because the schema
            // context won't hold a reference and it would otherwise be
            // destroyed before we need it.  See the comment on
            // UnknownProperty::m_spUnknownDeclaringType for more details.
            outProperty->m_spUnknownDeclaringType = inAttachedOwnerType;
        }
    }
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   get_DeclaringType
//
//  Synopsis:
//      Gets the type that declares this property.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT UnknownProperty::get_DeclaringType(
    _Out_ std::shared_ptr<XamlType>& outDeclaringType)
{
    // First try our base method to see if we have a declaring type
    IFC_RETURN(XamlProperty::get_DeclaringType(outDeclaringType));
    
    // If we didn't find one, try the strong reference that we keep to any
    // unknown types (see the comment on m_spUnknownDeclaringType for more
    // details)
    if (!outDeclaringType)
    {
        outDeclaringType = m_spUnknownDeclaringType;
    }
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT ImplicitProperty::Create(
            const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
            const ImplicitPropertyType& inImplicitPropertyType,
            std::shared_ptr<ImplicitProperty>& outProperty)
{
    xstring_ptr spEmptyName(xstring_ptr::EmptyString());
    std::shared_ptr<XamlNamespace> spXamlNamespace;
    std::shared_ptr<XamlType> spNoOwnerType;
    
    IFC_RETURN(Create(spSchemaContext, inImplicitPropertyType, spEmptyName, spNoOwnerType, spXamlNamespace, FALSE, outProperty));
    return S_OK;
}
    
//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT ImplicitProperty::Create(
            const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
            const ImplicitPropertyType& inImplicitPropertyType,
            _In_ const xstring_ptr& inName,
            const std::shared_ptr<XamlType>& inPropertyType,
            const std::shared_ptr<XamlNamespace>& inNamespace,
            bool bIsAttachable,
            std::shared_ptr<ImplicitProperty>& outProperty)
{
    XamlPropertyToken ptImplicitPropertyToken;

    ptImplicitPropertyToken.SetProviderKind(tpkParser);
    
    switch (inImplicitPropertyType)
    {
    case iptInitialization:
        ptImplicitPropertyToken.SetDirectiveHandle(xdInitialization);
        break;
    case iptItems:
        ptImplicitPropertyToken.SetDirectiveHandle(xdItems);
        break;
    case iptPositionalParameters:
        ptImplicitPropertyToken.SetDirectiveHandle(xdPositionalParameters);
        break;
    default:
        IFC_RETURN(E_UNEXPECTED);
        break;
    }
    
    outProperty = std::make_shared<ImplicitProperty>(spSchemaContext, inImplicitPropertyType, ptImplicitPropertyToken, inName, inPropertyType);
    outProperty->SetBoolPropertyBit(bpbIsAttachable, bIsAttachable);
    //  TODO: doing nothing with the inNamespace right nows
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
HRESULT DirectiveProperty::Create(
            const std::shared_ptr<XamlSchemaContext>& spSchemaContext, 
            _In_ const xstring_ptr& inPropertyName,
            const std::shared_ptr<XamlType>& inPropertyType,
            const std::shared_ptr<XamlNamespace>& inNamespace,
            const std::shared_ptr<XamlTextSyntax>& inXamlTextSyntax,
            XamlDirectives inDirective,
            std::shared_ptr<DirectiveProperty>& outProperty)
{
    XamlPropertyToken ptDirectivePropertyToken;
    
    ptDirectivePropertyToken.SetProviderKind(tpkParser);
    ptDirectivePropertyToken.SetDirectiveHandle(inDirective);
    
    outProperty = std::shared_ptr<DirectiveProperty>(
        new DirectiveProperty(spSchemaContext, ptDirectivePropertyToken, 
            inNamespace, inPropertyName, inPropertyType, inXamlTextSyntax));
    
    return S_OK;
}