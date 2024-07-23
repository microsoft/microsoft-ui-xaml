// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xcptypes.h"
#include "TypeTableStructs.h"
#include "MetadataAPI.h"
#include "CustomClassInfo.h"
#include <TypeNamePtr.h>

using namespace DirectUI;

XamlManagedTypeInfoProvider::XamlManagedTypeInfoProvider(
    _In_ const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext)
    : m_spXamlSchemaContext(inXamlSchemaContext)
    , m_pCore(NULL)
{
}

// Resolve an assembly given its name and return a new token used to identify
// it.  This always fails with E_NOTIMPL right now.
_Check_return_ HRESULT XamlManagedTypeInfoProvider::ResolveAssembly(
    _In_ const xstring_ptr& inAssemblyName,
    _Out_ XamlAssemblyToken& outAssemblyToken
    )
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetTypeNamespace(
    _In_ const xstring_ptr& inTypeNamespace,
    _Out_ XamlTypeNamespaceToken& outTypeNamespaceToken
    )
{
    std::shared_ptr<XamlSchemaContext> schemaContext;
    xstring_ptr ssAssembly;
    xstring_ptr ssNamespace;

    IFC_RETURN(GetSchemaContext(schemaContext));
    // Since we will always try to crack anything we see as a namespace, even unknown namespaces,
    // we check for that failure and error out with S_FALSE so that we can treat it as an unknown namespace

    bool isValidXmlns = false;
    HRESULT hr = schemaContext->CrackXmlns(inTypeNamespace, ssNamespace, isValidXmlns);

    if (!isValidXmlns)
    {
        return S_FALSE;
    }

    IFC_RETURN(hr);

    IFC_RETURN(GetSourceAssembly(&ssAssembly));

    IFC_RETURN(GetTypeNamespace(ssAssembly, ssNamespace, outTypeNamespaceToken));

    if (outTypeNamespaceToken.GetProviderKind() != tpkUnknown)
    {
        // ensure that the assembly is registered.

        std::shared_ptr<XamlAssembly> spAssembly;
        XamlAssemblyToken tokAssembly(outTypeNamespaceToken.AssemblyToken.GetProviderKind(), outTypeNamespaceToken.AssemblyToken.GetHandle());

        IFC_RETURN(schemaContext->GetXamlAssembly(tokAssembly, ssAssembly, spAssembly));
        ASSERT(!!spAssembly);
    }

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetTypeNamespace(
    _In_ const xstring_ptr& inAssemblyName,
    _In_ const xstring_ptr& inTypeNamespace,
    _Out_ XamlTypeNamespaceToken& outTypeNamespaceToken
    )
{
    HRESULT hr = S_OK;
    const CNamespaceInfo* pNamespace = nullptr;
    UNREFERENCED_PARAMETER(inAssemblyName);

    IFCEXPECT(!inTypeNamespace.IsNullOrEmpty());

    if (SUCCEEDED(MetadataAPI::GetNamespaceByName(inTypeNamespace, &pNamespace)))
    {
        outTypeNamespaceToken = XamlTypeNamespaceToken::FromNamespace(pNamespace);

        // Everything is in assembly '1'.
        outTypeNamespaceToken.AssemblyToken.SetProviderKind(tpkNative);
        outTypeNamespaceToken.AssemblyToken.SetHandle(1);
    }
    else
    {
        // It's not an error not to find the namespace.
        hr = S_FALSE;
        outTypeNamespaceToken.Reset();
        outTypeNamespaceToken.AssemblyToken.Reset();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetTypeNamespaceForType(
    _In_ const XamlTypeToken sXamlTypeToken,
    _Out_ XamlTypeNamespaceToken& rXamlTypeNamespaceToken,
    _Out_ xstring_ptr* pstrOutXamlTypeNamespaceName
    )
{
    HRESULT hr = S_OK;
    const CClassInfo* pType = nullptr;

    pType = MetadataAPI::GetClassInfoByIndex(sXamlTypeToken.GetHandle());

    *pstrOutXamlTypeNamespaceName = pType->GetNamespace()->GetName();
    rXamlTypeNamespaceToken = XamlTypeNamespaceToken::FromNamespace(pType->GetNamespace());

    // According to the core this is hacky
    rXamlTypeNamespaceToken.AssemblyToken.SetProviderKind(tpkNative);
    rXamlTypeNamespaceToken.AssemblyToken.SetHandle(1);

    RRETURN(hr);
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::LookupTypeFlags(
    _In_ const XamlTypeToken sTypeToken,
    _In_ const XamlBitSet<BoolTypeBits>& btbLookupValues,
    _Out_ XamlBitSet<BoolTypeBits>& btbBitsChecked,
    _Out_ XamlBitSet<BoolTypeBits>& btbReturnValues
    )
{
    const CClassInfo *pType = NULL;
    KnownTypeIndex nTypeIndex;

    IFCEXPECT_RETURN(sTypeToken.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(sTypeToken.GetHandle() != KnownTypeIndex::UnknownType);

    // Clear all of the bits by default
    btbBitsChecked.ClearAllBits();
    btbReturnValues.ClearAllBits();

    // Get the type
    pType = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());
    nTypeIndex = pType->GetIndex();

    // Set the various bits we know how to lookup
    btbBitsChecked.SetBit(btbCollection);
    btbReturnValues.SetBit(pType->IsCollection() ? btbCollection : btbNone);
    btbBitsChecked.SetBit(btbDictionary);
    btbReturnValues.SetBit(pType->IsDictionary() ? btbDictionary : btbNone);
    btbBitsChecked.SetBit(btbMarkupExtension);
    btbReturnValues.SetBit(pType->IsMarkupExtension() ? btbMarkupExtension : btbNone);
    btbBitsChecked.SetBit(btbTemplate);
    // TODO: We should use an IsAssignableCheck if we allow custom template types
    btbReturnValues.SetBit(
        nTypeIndex == KnownTypeIndex::FrameworkTemplate ||
        nTypeIndex == KnownTypeIndex::DataTemplate ||
        nTypeIndex == KnownTypeIndex::ControlTemplate ||
        nTypeIndex == KnownTypeIndex::ItemsPanelTemplate ?
    btbTemplate :
                btbNone);
    btbBitsChecked.SetBit(btbString);
    btbReturnValues.SetBit(nTypeIndex == KnownTypeIndex::String ? btbString : btbNone);
    btbBitsChecked.SetBit(btbConstructible);
    btbReturnValues.SetBit(pType->IsConstructible() ? btbConstructible : btbNone);
    btbBitsChecked.SetBit(btbIsPublic);
    btbReturnValues.SetBit(pType->IsPublic() ? btbIsPublic : btbNone);
    btbBitsChecked.SetBit(btbWhitespaceSignificantCollection);
    btbReturnValues.SetBit(pType->IsWhitespaceSignificant() ? btbWhitespaceSignificantCollection : btbNone);
    btbBitsChecked.SetBit(btbTrimSurroundingWhitespace);
    btbReturnValues.SetBit(pType->TrimSurroundingWhitespace() ? btbTrimSurroundingWhitespace : btbNone);
    btbBitsChecked.SetBit(btbIsISupportInitialize);
    btbReturnValues.SetBit(pType->IsISupportInitialize() ? btbIsISupportInitialize : btbNone);

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::LookupPropertyFlags(
    _In_ const XamlPropertyToken sPropertyToken,
    _In_ const XamlBitSet<BoolPropertyBits>& bpbLookupValues,
    _Out_ XamlBitSet<BoolPropertyBits>& bpbBitsChecked,
    _Out_ XamlBitSet<BoolPropertyBits>& bpbReturnValues
    )
{
    ASSERT(sPropertyToken.GetProviderKind() == tpkManaged);

    const CDependencyProperty* pProperty = NULL;

    IFCEXPECT_RETURN(!sPropertyToken.IsUnknown());

    // Clear all of the bits by default.
    bpbBitsChecked.ClearAllBits();
    bpbReturnValues.ClearAllBits();

    // Get the property.
    pProperty = MetadataAPI::GetPropertyByIndex(sPropertyToken.GetHandle());

    // Set the property bits.
    bpbBitsChecked.SetBit(bpbIsEvent);
    bpbReturnValues.SetBit(bpbNone);
    bpbBitsChecked.SetBit(bpbIsReadOnly);
    bpbReturnValues.SetBit(pProperty->IsReadOnly() ? bpbIsReadOnly : bpbNone);
    bpbBitsChecked.SetBit(bpbIsStatic);
    bpbReturnValues.SetBit(pProperty->IsStatic() ? bpbIsStatic : bpbNone);
    bpbBitsChecked.SetBit(bpbIsPublic);
    bpbReturnValues.SetBit(pProperty->IsPublic() ? bpbIsPublic : bpbNone);
    bpbBitsChecked.SetBit(bpbIsAttachable);
    bpbReturnValues.SetBit(pProperty->IsAttached() ? bpbIsAttachable : bpbNone);

    return S_OK;
}

// Resolve the name of a type in a given namespace.  This returns a new token
// corresponding to the type if found or an empty token if it was not resolved.
// If the namespace token has the handle 0, we will search the known DXaml
// namespaces in order.
_Check_return_ HRESULT XamlManagedTypeInfoProvider::ResolveTypeName(
    _In_ const XamlTypeNamespaceToken inNamespaceToken,
    _In_ const xstring_ptr& inTypeName,
    _Out_ XamlTypeToken& rXamlType
    )
{
    CCoreServices *pCore = nullptr;
    IFC_RETURN(GetCore(&pCore));

    IFCEXPECT_RETURN(!inTypeName.IsNullOrEmpty());

    // By default, indicate that we've found nothing
    rXamlType.Reset();

    // If the Handle == 0, then we need to lookup the type in all of our default
    // namespace.  Otherwise we get the NamespaceInfo and lookup the type.
    if (inNamespaceToken.GetHandle() != KnownNamespaceIndex::UnknownNamespace)
    {
        TryGetTypeWithSpecifiedNamespaceId(static_cast<KnownNamespaceIndex>(inNamespaceToken.GetHandle()), inTypeName, &rXamlType);
    }
    else
    {
        // Search our known namespaces (in the same order as
        // xcp\clr\clrlib\ManagedTypeInfoProviderRPInvokes.cs::ResolveTypeName)
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Controls, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Data, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Controls_Primitives, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Automation, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Shapes, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Media_Media3D, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Media_Imaging, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Media_Animation, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Media, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Input, inTypeName, &rXamlType) ||
        TryGetTypeWithSpecifiedNamespaceId(KnownNamespaceIndex::Microsoft_UI_Xaml_Documents, inTypeName, &rXamlType);
    }

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::ResolvePropertyName(
    _In_ const XamlTypeToken sTypeToken,
    _In_ const xstring_ptr& inPropertyName,
    _Out_ XamlPropertyToken& outProperty,
    _Out_ XamlTypeToken& outPropertyTypeToken
    )
{
    IFCEXPECT_RETURN(sTypeToken.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(sTypeToken.GetHandle() != KnownTypeIndex::UnknownType);

    // By default, indicate that we've found nothing.
    outProperty.Reset();
    outPropertyTypeToken.Reset();

    // There are some built-in types (e.g. Button) that are handled by ManagedTypeInfoProvider, so first check to see if it's
    // a built-in PropertyBase
    const auto pClassInfo = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());
    auto propertyBase = MetadataAPI::TryGetBuiltInPropertyBaseByName(pClassInfo, inPropertyName, /* bAllowDirectives */ true);
    if (propertyBase == nullptr)
    {
        const CDependencyProperty* pProperty = nullptr;
        IFC_RETURN(MetadataAPI::TryGetPropertyByName(
            pClassInfo,
            inPropertyName,
            &pProperty,
            true /* bAllowDirectives */));

        if (pProperty == nullptr)
        {
            // Try resolving it as an attached property instead.
            IFC_RETURN(MetadataAPI::TryGetAttachedPropertyByName(inPropertyName, &pProperty));
        }

        if (pProperty != nullptr)
        {
            outProperty = XamlPropertyToken::FromProperty(pProperty);
            outPropertyTypeToken = XamlTypeToken::FromType(pProperty->GetPropertyType());
        }
        // For compat with Windows Blue and before, we check if this property is an event name.
        else if (MetadataAPI::IsLegacyEventName(MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle()), inPropertyName))
        {
            outProperty.SetProviderKind(tpkNative);
            outProperty.SetHandle(MetadataAPI::EventPropertyIndex);

            outPropertyTypeToken.SetProviderKind(tpkNative);
            outPropertyTypeToken.SetHandle(KnownTypeIndex::String);
        }
    }
    else
    {
        outProperty = XamlPropertyToken::FromProperty(propertyBase);
        outPropertyTypeToken = XamlTypeToken::FromType(propertyBase->GetPropertyType());
    }

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::ResolveDependencyPropertyName(
    _In_ const XamlTypeToken sTypeToken,
    _In_ const xstring_ptr& inPropertyName,
    _Out_ XamlPropertyToken& outProperty,
    _Out_ XamlTypeToken& outPropertyTypeToken
    )
{
    const CDependencyProperty* pProperty = nullptr;

    IFCEXPECT_RETURN(sTypeToken.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(sTypeToken.GetHandle() != KnownTypeIndex::UnknownType);

    // By default, indicate that we've found nothing.
    outProperty.Reset();
    outPropertyTypeToken.Reset();

    IFC_RETURN(MetadataAPI::TryGetDependencyPropertyByName(
        MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle()),
        inPropertyName,
        &pProperty,
        true /* bAllowDirectives */));

    if (pProperty == nullptr)
    {
        // Try resolving it as an attached property instead.
        IFC_RETURN(MetadataAPI::TryGetAttachedPropertyByName(inPropertyName, &pProperty));
    }

    if (pProperty != nullptr)
    {
        outProperty = XamlPropertyToken::FromProperty(pProperty);
        outPropertyTypeToken = XamlTypeToken::FromType(pProperty->GetPropertyType());
    }

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetTypeName(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ xstring_ptr* pstrOutTypeName
    )
{
    const CClassInfo* pType = nullptr;

    IFCEXPECT_RETURN(sTypeToken.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(sTypeToken.GetHandle() != KnownTypeIndex::UnknownType);

    // Get the type.
    pType = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());

    // Copy the type name.
    *pstrOutTypeName = pType->GetName();

    return S_OK;

}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetTypeFullName(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ xstring_ptr* pstrOutTypeFullName
    )
{
    const CClassInfo* pType = nullptr;

    IFCEXPECT_RETURN(sTypeToken.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(sTypeToken.GetHandle() != KnownTypeIndex::UnknownType);

    // Get the type and namespace.
    pType = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());

    // Copy the type name.
    *pstrOutTypeFullName = pType->GetFullName();

    return S_OK;

}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetPropertyName(
    _In_ const XamlPropertyToken sPropertyToken,
    _Out_ xstring_ptr* pstrOutPropertyName
    )
{
    const CDependencyProperty* pProperty = NULL;

    IFCEXPECT_RETURN(!sPropertyToken.IsUnknown());

    // Get the property
    pProperty = MetadataAPI::GetPropertyByIndex(sPropertyToken.GetHandle());

    // Copy the type name
    *pstrOutPropertyName = pProperty->GetName();

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetBaseType(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlTypeToken& rBaseTypeToken
    )
{
    const CClassInfo* pType = nullptr;

    IFCEXPECT_RETURN(sTypeToken.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(sTypeToken.GetHandle() != KnownTypeIndex::UnknownType);

    pType = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());
    rBaseTypeToken = XamlTypeToken::FromType(pType->GetBaseType());

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetDeclaringType(
    _In_ const XamlPropertyToken sPropertyToken,
    _Out_ XamlTypeToken& rDeclaringTypeToken
    )
{
    const CDependencyProperty* pDP = NULL;

    IFCEXPECT_RETURN(!sPropertyToken.IsUnknown());

    // Get the declaring type
    pDP = MetadataAPI::GetDependencyPropertyByIndex(sPropertyToken.GetHandle());
    rDeclaringTypeToken = XamlTypeToken::FromType(pDP->GetDeclaringType());

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::IsAssignableFrom(
    _In_ const XamlTypeToken sDerivedTypeToken,
    _In_ const XamlTypeToken rxtBaseType,
    _Out_ bool& bOut
    )
{
    IFCEXPECT_RETURN(sDerivedTypeToken.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(sDerivedTypeToken.GetHandle() != KnownTypeIndex::UnknownType);
    IFCEXPECT_RETURN(rxtBaseType.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(rxtBaseType.GetHandle() != KnownTypeIndex::UnknownType);

    bOut = MetadataAPI::IsAssignableFrom(rxtBaseType.GetHandle(), sDerivedTypeToken.GetHandle());

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetContentProperty(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlPropertyToken& rPropertyToken,
    _Out_ XamlTypeToken& rContentPropertyTypeToken
    )
{
    ASSERT(sTypeToken.GetProviderKind() == tpkManaged);

    rPropertyToken.Reset();
    rContentPropertyTypeToken.Reset();
    IFC_RETURN(GetContentPropertyImpl(sTypeToken, &rPropertyToken, &rContentPropertyTypeToken));

    if (rPropertyToken.IsEmpty())
    {
        // WORKAROUND: Our OM can't deal with this. The problem is we don't expose it publically as a content property in managed
        // The base class works because it is marked as PROP_CONTENT_PROP in xcptypes, but that doesn't work for derived types
        // such as HierarchicalDataTemplate.
        bool bIsFrameworkTemplate = false;
        XamlTypeToken tBaseToken;

        tBaseToken.SetProviderKind(tpkNative);
        tBaseToken.SetHandle(KnownTypeIndex::FrameworkTemplate);

        IFC_RETURN(IsAssignableFrom(sTypeToken, tBaseToken, bIsFrameworkTemplate));
        if (bIsFrameworkTemplate)
        {
            // Return template content
            rPropertyToken.SetProviderKind(tpkNative);
            rPropertyToken.SetHandle(KnownPropertyIndex::FrameworkTemplate_Template);

            rContentPropertyTypeToken.SetProviderKind(tpkNative);
            rContentPropertyTypeToken.SetHandle(KnownTypeIndex::TemplateContent);
        }
    }

    return S_OK;
}

// Collection types may optionally provide a type used to wrap literal
// content from the parser before inserting into the collection.  The
// parser won't actually wrap the content, but merely knows it's safe to
// pass literal content to the collection which will handle it.
_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetContentWrapper(
    _In_ const XamlTypeToken sTypeToken,
    _Out_opt_ XamlTypeToken& rContentWrapperTypeToken
    )
{
    HRESULT hr = S_OK;

    // TODO: Compat: This is where we would RPInvoke to check for [ContentWrapper] attributes if we had them
    rContentWrapperTypeToken = XamlTypeToken();

    RRETURN(hr);
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetRuntimeNameProperty(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlPropertyToken& rPropertyToken,
    _Out_ XamlTypeToken& rPropertyTypeToken
    )
{
    rPropertyToken = XamlPropertyToken();
    rPropertyTypeToken = XamlTypeToken();

    rPropertyToken.SetProviderKind(tpkNative);
    rPropertyToken.SetHandle(KnownPropertyIndex::DependencyObject_Name);

    rPropertyTypeToken.SetProviderKind(tpkNative);
    rPropertyTypeToken.SetHandle(KnownTypeIndex::String);

    RRETURN(S_OK);
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetDictionaryKeyProperty(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlPropertyToken& rPropertyToken,
    _Out_ XamlTypeToken& rPropertyTypeToken
    )
{
    // TODO: This is something that wasn't supplied for managed typed in
    // SL3, but can be enabled by adding an R-pinvoke, and addition of DictionaryKeyPropertyAttribute
    rPropertyToken = XamlPropertyToken();
    rPropertyTypeToken = XamlTypeToken();
    RRETURN(S_OK);
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetXmlLangProperty(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlPropertyToken& rPropertyToken,
    _Out_ XamlTypeToken& rPropertyTypeToken
    )
{
    // TODO: This is something that wasn't supplied for managed typed in
    // SL3, but can be enabled by adding an R-pinvoke, and addition of XmlLanguagePropertyAttribute
    rPropertyToken = XamlPropertyToken();
    rPropertyTypeToken = XamlTypeToken();
    RRETURN(S_OK);
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetCollectionItemType(
    _In_ const XamlTypeToken sCollectionTypeToken,
    _Out_ XamlTypeToken& rCollectionItemTypeToken
    )
{
    if (sCollectionTypeToken.GetProviderKind() == tpkUnknown || sCollectionTypeToken.GetHandle() == KnownTypeIndex::UnknownType)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    // transform sCollectionTypeToken to IXamlType
    const CClassInfo* pType = MetadataAPI::GetClassInfoByIndex(sCollectionTypeToken.GetHandle());

    // keep original behavior for built-in types
    if (pType->IsBuiltinType())
    {
        rCollectionItemTypeToken.Reset();
        return S_OK;
    }
    xaml_markup::IXamlType* collectionIXamlType = pType->AsCustomType()->GetXamlTypeNoRef();

    // get item type of collection
    xaml_markup::IXamlType* collectionItemIXamlType;
    collectionIXamlType->get_ItemType(&collectionItemIXamlType);
    const CClassInfo* pItemType = nullptr;

    TypeNamePtr typeName;
    IFC_RETURN(collectionItemIXamlType->get_UnderlyingType(typeName.ReleaseAndGetAddressOf()));
    IFC_RETURN(MetadataAPI::GetClassInfoByTypeName(typeName.Get(), &pItemType));

    rCollectionItemTypeToken = XamlTypeToken::FromType(pItemType);

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetTextSyntaxForType(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlTypeToken& rsTextSyntaxTypeToken
    )
{
    const CClassInfo *pType = NULL;

    IFCEXPECT_RETURN(sTypeToken.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(sTypeToken.GetHandle() != KnownTypeIndex::UnknownType);

    // By default, indicate that we've found nothing
    rsTextSyntaxTypeToken.Reset();

    if (sTypeToken.GetHandle() == KnownTypeIndex::Object)
    {
        rsTextSyntaxTypeToken = XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::String));
    }
    else
    {
        // Get the type.
        pType = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());

        if (pType->HasTypeConverter() && !pType->IsBuiltinType())
        {
            // The enum is its own type converter.
            rsTextSyntaxTypeToken = XamlTypeToken::FromType(pType);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetTextSyntaxForProperty(
    _In_ const XamlPropertyToken sPropertyToken,
    _Out_ XamlTypeToken& rsTextSyntaxTypeToken
    )
{
    IFCEXPECT_RETURN(!sPropertyToken.IsUnknown());

    rsTextSyntaxTypeToken.Reset();

    // Special case for MediaPlayerElement_Source which is IMediaPlaybackSource interface type, we need to route it to MediaPlaybackItemConverter parse the Uri string
    if (sPropertyToken.GetHandle() == KnownPropertyIndex::MediaPlayerElement_Source)
    {
        rsTextSyntaxTypeToken = XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::MediaPlaybackItemConverter));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetCore(
    _Outptr_ CCoreServices **ppCore
    )
{
    if (!m_pCore)
    {
        std::shared_ptr<XamlSchemaContext> schemaContext;
        IFC_RETURN(GetSchemaContext(schemaContext));
        m_pCore = schemaContext->GetCore();
    }

    *ppCore = m_pCore;

    return S_OK;
}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetSourceAssembly(
    _Out_ xstring_ptr* pstrOutssSourceAssembly
    )
{
    std::shared_ptr<XamlSchemaContext> schemaContext;

    IFC_RETURN(GetSchemaContext(schemaContext));
    *pstrOutssSourceAssembly = schemaContext->GetSourceAssembly();
    return S_OK;

}

_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetSchemaContext(
    _Out_ std::shared_ptr<XamlSchemaContext>& outSchemaContext
    )
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    IFCCHECK_RETURN(outSchemaContext);
    return S_OK;
}

// Attempt to resolve the name of a type in a given namespace.  This returns a
// new token corresponding to the type if found and a value indicating whether
// the token was found (the value is redundant since the token is empty, but the
// value makes it easier to chain together a series of lookups and short circuit
// after one is successful).  This is not a callback.
bool XamlManagedTypeInfoProvider::TryGetTypeWithSpecifiedNamespaceId(
    _In_ const KnownNamespaceIndex nNamespaceIndex,
    _In_ const xstring_ptr& spTypeName,
    _Out_ XamlTypeToken* ptokType)
{
    bool bFound = false;
    const CClassInfo* pType = nullptr;

    if (SUCCEEDED(MetadataAPI::TryGetClassInfoByName(nNamespaceIndex, spTypeName, &pType)))
    {
        if (pType && !pType->IsUnknown())
        {
            *ptokType = XamlTypeToken::FromType(pType);
            bFound = true;
        }
        else
        {
            // Indicate that we've found nothing
            ptokType->Reset();
        }
    }

    return bFound;
}

// Look for the most derived ContentProperty associated with the type.
_Check_return_ HRESULT XamlManagedTypeInfoProvider::GetContentPropertyImpl(
    _In_ const XamlTypeToken tokType,
    _Out_ XamlPropertyToken* ptokContentProperty,
    _Out_ XamlTypeToken* ptokContentPropertyType)
{
    const CClassInfo* pType = nullptr;
    const CDependencyProperty* pContentProperty = nullptr;

    IFCEXPECT_RETURN(tokType.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(tokType.GetHandle() != KnownTypeIndex::UnknownType);

    // Get the type
    pType = MetadataAPI::GetClassInfoByIndex(tokType.GetHandle());

    // Get the ContentProperty
    pContentProperty = pType->GetContentProperty();
    if (pContentProperty->GetIndex() == KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        ptokContentProperty->Reset();
        ptokContentPropertyType->Reset();
        return S_OK;
    }

    *ptokContentProperty = XamlPropertyToken::FromProperty(pContentProperty);
    *ptokContentPropertyType = XamlTypeToken::FromType(pContentProperty->GetPropertyType());

    return S_OK;
}

