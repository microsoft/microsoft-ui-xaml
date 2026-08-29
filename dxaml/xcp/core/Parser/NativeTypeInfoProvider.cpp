// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xcptypes.h"
#include "TypeTableStructs.h"
#include "MetadataAPI.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>

using namespace RuntimeFeatureBehavior;

using namespace DirectUI;

XamlNativeTypeInfoProvider::XamlNativeTypeInfoProvider(
    _In_ const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext)
    : m_spXamlSchemaContext(inXamlSchemaContext)
    , m_pCore(nullptr)
{
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::ResolveAssembly(
    _In_ const xstring_ptr& inAssemblyName,
    _Out_ XamlAssemblyToken& outAssemblyToken
    )
{
    HRESULT hr = S_OK;

    // only one assembly that the native typeinfo provider provides types
    if (inAssemblyName.Equals(STR_LEN_PAIR(L"System.Windows")))
    {
        outAssemblyToken = XamlAssemblyToken(tpkNative, 1);
    }

    RRETURN(hr);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetTypeNamespace(
    _In_ const xstring_ptr& inTypeNamespace,
    _Out_ XamlTypeNamespaceToken& outTypeNamespaceToken
    )
{
    auto pNamespace = MetadataAPI::GetBuiltinNamespaceByName(inTypeNamespace);

    if (pNamespace)
    {
        outTypeNamespaceToken = XamlTypeNamespaceToken::FromNamespace(pNamespace);
    }
    else
    {
        outTypeNamespaceToken.Reset();
    }

    outTypeNamespaceToken.AssemblyToken = XamlAssemblyToken(tpkNative, 1); // Everything is in assembly '1'.

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetTypeNamespace(
    _In_ const xstring_ptr& inAssemblyName,
    _In_ const xstring_ptr& inTypeNamespace,
    _Out_ XamlTypeNamespaceToken& outTypeNamespaceToken
    )
{
    RRETURN(GetTypeNamespace(inTypeNamespace, outTypeNamespaceToken));
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetTypeNamespaceForType(
    _In_ const XamlTypeToken sXamlTypeToken,
    _Out_ XamlTypeNamespaceToken& rXamlTypeNamespaceToken,
    _Out_ xstring_ptr* pstrOutXamlTypeNamespaceName
    )
{
    IFCEXPECT_RETURN(sXamlTypeToken.GetProviderKind() == tpkNative);

    auto pType = MetadataAPI::GetClassInfoByIndex(sXamlTypeToken.GetHandle());
    *pstrOutXamlTypeNamespaceName = pType->GetNamespace()->GetName();

    rXamlTypeNamespaceToken = XamlTypeNamespaceToken::FromNamespace(pType->GetNamespace());
    rXamlTypeNamespaceToken.AssemblyToken = XamlAssemblyToken(tpkNative, 1);

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::LookupTypeFlags(
    _In_ const XamlTypeToken sTypeToken,
    _In_ const XamlBitSet<BoolTypeBits>& btbLookupValues,
    _Out_ XamlBitSet<BoolTypeBits>& btbBitsChecked,
    _Out_ XamlBitSet<BoolTypeBits>& btbReturnValues
    )
{
    bool bIsFlagSet = false;
    const CClassInfo* pClassInfo = NULL;

    IFCEXPECT_RETURN(sTypeToken.GetProviderKind() == tpkNative);

    pClassInfo = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());

    // TODO: For now just getting the important ones
    // Can figure out a better selection later.

    btbBitsChecked.ClearAllBits();
    btbReturnValues.ClearAllBits();

    btbBitsChecked.SetBit(btbCollection);
    btbReturnValues.SetBit(pClassInfo->IsCollection() ? btbCollection : btbNone);

    btbBitsChecked.SetBit(btbDictionary);
    btbReturnValues.SetBit(pClassInfo->IsDictionary() ? btbDictionary : btbNone);

    btbBitsChecked.SetBit(btbMarkupExtension);
    btbReturnValues.SetBit(pClassInfo->IsMarkupExtension() ? btbMarkupExtension : btbNone);

    IFC_RETURN(IsTemplateType(sTypeToken, bIsFlagSet));
    btbBitsChecked.SetBit(btbTemplate);
    btbReturnValues.SetBit(bIsFlagSet ? btbTemplate : btbNone);

    IFC_RETURN(IsString(sTypeToken, bIsFlagSet));
    btbBitsChecked.SetBit(btbString);
    btbReturnValues.SetBit(bIsFlagSet ? btbString : btbNone);

    btbBitsChecked.SetBit(btbConstructible);
    btbReturnValues.SetBit(pClassInfo->IsConstructible() ? btbConstructible : btbNone);

    btbBitsChecked.SetBit(btbIsPublic);
    btbReturnValues.SetBit(pClassInfo->IsPublic() ? btbIsPublic : btbNone);

    btbBitsChecked.SetBit(btbWhitespaceSignificantCollection);
    btbReturnValues.SetBit(
        pClassInfo->IsWhitespaceSignificant()
        ? btbWhitespaceSignificantCollection
        : btbNone);

    btbBitsChecked.SetBit(btbTrimSurroundingWhitespace);
    btbReturnValues.SetBit(
        pClassInfo->TrimSurroundingWhitespace()
        ? btbTrimSurroundingWhitespace
        : btbNone);

    btbBitsChecked.SetBit(btbIsISupportInitialize);
    btbReturnValues.SetBit(
        pClassInfo->IsISupportInitialize()
        ? btbIsISupportInitialize
        : btbNone);
    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::LookupPropertyFlags(
    _In_ const XamlPropertyToken sPropertyToken,
    _In_ const XamlBitSet<BoolPropertyBits>& bpbLookupValues,
    _Out_ XamlBitSet<BoolPropertyBits>& bpbBitsChecked,
    _Out_ XamlBitSet<BoolPropertyBits>& bpbReturnValues
    )
{
    bool bIsFlagSet = false;

    ASSERT(sPropertyToken.GetProviderKind() == tpkNative);

    IFC_RETURN(IsPropertyEvent(sPropertyToken, bIsFlagSet));
    bpbBitsChecked.SetBit(bpbIsEvent);
    bpbReturnValues.SetBit(bIsFlagSet ? bpbIsEvent : bpbNone);

    // Only check the following flags if we're dealing with a property.
    if (!bIsFlagSet)
    {
        IFC_RETURN(IsPropertyReadOnly(sPropertyToken, bIsFlagSet));
        bpbBitsChecked.SetBit(bpbIsReadOnly);
        bpbReturnValues.SetBit(bIsFlagSet ? bpbIsReadOnly : bpbNone);

        IFC_RETURN(IsPropertyStatic(sPropertyToken, bIsFlagSet));
        bpbBitsChecked.SetBit(bpbIsStatic);
        bpbReturnValues.SetBit(bIsFlagSet ? bpbIsStatic : bpbNone);

        IFC_RETURN(IsPropertyPublic(sPropertyToken, bIsFlagSet));
        bpbBitsChecked.SetBit(bpbIsPublic);
        bpbReturnValues.SetBit(bIsFlagSet ? bpbIsPublic : bpbNone);

        IFC_RETURN(IsPropertyAttachable(sPropertyToken, bIsFlagSet));
        bpbBitsChecked.SetBit(bpbIsAttachable);
        bpbReturnValues.SetBit(bIsFlagSet ? bpbIsAttachable : bpbNone);
    }


    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::ResolveTypeName(
    _In_ const XamlTypeNamespaceToken inNamespaceToken,
    _In_ const xstring_ptr& inTypeName,
    _Out_ XamlTypeToken& rXamlType
    )
{
    const CClassInfo* pClassInfo = NULL;
    XamlTypeToken retTypeToken;

    //
    // TODO: until we either a) organize the type information
    // by type-namespace, or ignore the type-namespace for native types, this
    // way of looking up first and validating the type-namespace after is going to be
    // quite wasteful.
    //

    pClassInfo = DirectUI::MetadataAPI::GetBuiltinClassInfoByName(inTypeName);

    //
    // If the Class that we've found is in the correct namespace, or
    // the caller didn't care about the namespace (as indicated by passing
    // an "Unknown" XamlTypeNamespaceToken, then we have the correct type
    //
    // TFS#775353:[OneNoteMX][16.0.3030.1012]:-Application is crashing while inserting picture into the page
    // Temporarily force Point/Rect/Size to be resolved through the managed provider. We'll properly fix this in M2.
    if (pClassInfo && (
        pClassInfo->GetIndex() != KnownTypeIndex::Point &&
        pClassInfo->GetIndex() != KnownTypeIndex::Rect &&
        pClassInfo->GetIndex() != KnownTypeIndex::Size))
    {
        // TODO: Seems like it shouldn't be passing provider kind unknown.
        // I think this should probably be relaxed so that 0 handle means unknown.
        // If we have a valid 0 namespace we should probably offset it, because
        // all the other tokens use 0 to mean invalid.
        if ((inNamespaceToken.GetHandle() == pClassInfo->GetNamespace()->m_nIndex)
            || (inNamespaceToken.GetHandle() == KnownNamespaceIndex::UnknownNamespace && inNamespaceToken.GetProviderKind() == tpkUnknown)
            )
        {
            retTypeToken = XamlTypeToken::FromType(pClassInfo);
        }
    }
    else
    {
        // !!!!!!!!!!!! HACK HACK HACK !!!!!!!!!!!!!!!!!!!!!!!!
        std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
        IFC_RETURN(GetSchemaContext(spXamlSchemaContext));

        XamlTypeNamespaceToken managedTypeNamespaceToken = inNamespaceToken;

        // HACK: So incredibly hacky. Depends on the fact that System.Windows will
        // be the first registered assembly.
        managedTypeNamespaceToken.AssemblyToken.SetProviderKind(tpkManaged);
        managedTypeNamespaceToken.AssemblyToken.SetHandle(1);

        managedTypeNamespaceToken.SetHandle(KnownNamespaceIndex::UnknownNamespace);
        managedTypeNamespaceToken.SetProviderKind(tpkManaged);

        std::shared_ptr<XamlTypeInfoProvider> spManagedTypeInfoProvider;

        // This can potentially fail if no managed code is loaded, since we are doing this as a fallback.
        //
        // If we can't spin up some managed, then it is treated as if it couldn't be found.
        if (SUCCEEDED(spXamlSchemaContext->GetTypeInfoProvider(tpkManaged, spManagedTypeInfoProvider)))
        {
            IFC_RETURN(spManagedTypeInfoProvider->ResolveTypeName(managedTypeNamespaceToken, inTypeName, retTypeToken));
        }

    }

    rXamlType = retTypeToken;
    return S_OK;
}

// Resolves the token for a property name. Contains hard-coded
// logic for supporting native attached properties.
_Check_return_ HRESULT XamlNativeTypeInfoProvider::ResolvePropertyName(
    _In_ const XamlTypeToken sTypeToken,
    _In_ const xstring_ptr&  inPropertyName,
    _Out_ XamlPropertyToken& outProperty,
    _Out_ XamlTypeToken& outPropertyTypeToken
    )
{
    outProperty.Reset();
    outPropertyTypeToken.Reset();

    const auto pClassInfo = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());

    ASSERT(pClassInfo);

    // TODO: this is the slowest possible way to look up a property by name,
    // but it will be changed so no need to be suprised if (when) it shows up on a profile.
    //
    // First check as a built-in property
    // If not found, then check as a custom DP registered on a built-in type
    // If not found, then check as an attached property
    auto propertyBase = MetadataAPI::TryGetBuiltInPropertyBaseByName(pClassInfo, inPropertyName, /* bAllowDirectives */ true);
    if (propertyBase == nullptr)
    {
        const CDependencyProperty* dependencyProperty = nullptr;
        IFC_RETURN(MetadataAPI::TryGetDependencyPropertyByName(pClassInfo, inPropertyName, &dependencyProperty, /* bAllowDirectives */ true));

        if (dependencyProperty == nullptr)
        {
            IFC_RETURN(MetadataAPI::TryGetAttachedPropertyByName(inPropertyName, &dependencyProperty));
        }
        propertyBase = dependencyProperty;
    }

    if (propertyBase)
    {
        outProperty = XamlPropertyToken::FromProperty(propertyBase);
        outPropertyTypeToken = XamlTypeToken::FromType(propertyBase->GetPropertyType());
    }
    else
    {
        //
        // one last try:  There are some native types that have managed properties.  This split is silly,
        // but we have to handle it nonetheless.  (an example of this is TextBox.HorizontalScrollBarVisibility)
        // in this case, we'll go over to the ManagedTypeInfoProvider, and see if he has any idea.
        // ** Note that the ManagedTypeInfoProvider has knowledge of our native XamlTypeTokens
        //
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlTypeInfoProvider> spManagedTypeInfoProvider;

        IFC_RETURN(GetSchemaContext(spSchemaContext));

        // Because managed may not be setup, we might not get a provider
        if (SUCCEEDED(spSchemaContext->GetTypeInfoProvider(tpkManaged, spManagedTypeInfoProvider)))
        {
            IFC_RETURN(spManagedTypeInfoProvider->ResolvePropertyName(sTypeToken, inPropertyName, outProperty, outPropertyTypeToken));
        }
    }

    return S_OK;
}

// Get the name of the type represented by a token.
_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetTypeName(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ xstring_ptr* pstrOutTypeName
    )
{
    HRESULT hr = S_OK;

    // Special case the names of a few proxy types
    switch (sTypeToken.GetHandle())
    {
        case KnownTypeIndex::TypeName:
            {
                DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strType, L"Type");
                *pstrOutTypeName = XSTRING_PTR_FROM_STORAGE(c_strType);
            }
            break;

        case KnownTypeIndex::DependencyPropertyProxy:
            {
                DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strDependencyProperty, L"DependencyProperty");
                *pstrOutTypeName = XSTRING_PTR_FROM_STORAGE(c_strDependencyProperty);
            }
            break;

        default:
            {
                const CClassInfo* pClassInfo = NULL;

                pClassInfo = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());
                ASSERT(pClassInfo);
                *pstrOutTypeName = pClassInfo->GetName();
            }
            break;
    }

    RRETURN(hr);
}

// Get the full name of the type (including its CLR namespace).
_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetTypeFullName(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ xstring_ptr* pstrOutTypeFullName
    )
{
    // Special case the names of a few proxy types
    switch (sTypeToken.GetHandle())
    {
        case KnownTypeIndex::TypeName:
            {
                // TODO: "System.Type" sounds CLR specific
                DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSystemType, L"System.Type");
                *pstrOutTypeFullName = c_strSystemType;
            }
            break;

        case KnownTypeIndex::DependencyPropertyProxy:
            {
                DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strWindowsUIXamlDependencyProperty, L"Microsoft.UI.Xaml.DependencyProperty");
                *pstrOutTypeFullName = XSTRING_PTR_FROM_STORAGE(c_strWindowsUIXamlDependencyProperty);
            }
            break;

        default:
            {
                IFC_RETURN(GetFullClassNameByIndex(sTypeToken.GetHandle(), pstrOutTypeFullName));
            }
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT
XamlNativeTypeInfoProvider::GetPropertyName(
    _In_ const XamlPropertyToken sPropertyToken,
    _Out_ xstring_ptr* pstrOutPropertyName
    )
{
    xstring_ptr strPropertyName;
    xstring_ptr strPropertyNameCopy;

    const auto propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(sPropertyToken.GetHandle());
    ASSERT(propertyBase);

    strPropertyName = propertyBase->GetName();

    if (strPropertyName.IsNull())
    {
        strPropertyName = xstring_ptr::EmptyString();
    }
    else if (strPropertyName.GetCount() > 1)
    {
        //
        // Handle the case of properties that use the "Foo.Bar" property name
        // to denote attached properties. "Canvas.Left" on UIElement being one
        // example.  *n.b.: checking for names starting at index 1 for two reasons:
        // 1) the type prefix for such a property must be at least one char.
        // 2) would take care of any case where the type in question has a '.'
        //      prefix.  (see xcptypes.h for information on the name prefixes)
        //
        auto uiDotPosition = strPropertyName.FindChar(L'.', 1);
        if (uiDotPosition != xstring_ptr_view::npos)
        {
            IFC_RETURN(strPropertyName.SubString(uiDotPosition + 1, strPropertyName.GetCount(), &strPropertyNameCopy));
        }
    }

    if (strPropertyNameCopy.IsNull())
    {
        strPropertyNameCopy = std::move(strPropertyName);
    }

    *pstrOutPropertyName = strPropertyNameCopy;

    return S_OK;
}

// Returns the first core type of the token. This does NOT return the
// base of the type, the name is misleading. Effectively a no-op for
// native code.
//
//  TODO: Rename this member.  The name is misleading.
_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetBaseType(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlTypeToken& rBaseTypeToken
    )
{
    HRESULT hr = S_OK;

    rBaseTypeToken = sTypeToken;

    RRETURN(hr);
}

// Gets the token of the type that declares the given property.
_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetDeclaringType(
    _In_ const XamlPropertyToken sPropertyToken,
    _Out_ XamlTypeToken& rDeclaringTypeToken
    )
{
    IFCEXPECT_RETURN(sPropertyToken.GetProviderKind() == tpkNative);

    rDeclaringTypeToken = XamlTypeToken::FromType(
        DirectUI::MetadataAPI::GetPropertyBaseByIndex(sPropertyToken.GetHandle())->GetDeclaringType());

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsAssignableFrom(
    _In_ const XamlTypeToken sDerivedTypeToken,
    _In_ const XamlTypeToken rxtBaseType,
    _Out_ bool& bOut
    )
{
    HRESULT hr = S_OK;

    ASSERT(sDerivedTypeToken.GetProviderKind() == tpkNative);

    bOut = FALSE;

    if (rxtBaseType.GetProviderKind() == tpkNative)
    {
        bOut = DirectUI::MetadataAPI::IsAssignableFrom(rxtBaseType.GetHandle(), sDerivedTypeToken.GetHandle());
    }

    RRETURN(hr);
}

// Retrieves the content property for a native type.
_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetContentProperty(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlPropertyToken& rPropertyToken,
    _Out_ XamlTypeToken& rContentPropertyTypeToken
    )
{
    HRESULT hr = S_OK;

    ASSERT(sTypeToken.GetProviderKind() == tpkNative);

    const CClassInfo* pClassInfo = NULL;

    pClassInfo = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());
    ASSERT(pClassInfo);

    // When not using the new deferred VSM we will redirect the setting of the VisualState
    // storyboard property to set instead the hidden __DeferredStoryboard property, which
    // is a CTemplateContent property that defers creation of the storyboard. Note that when we
    // use the new VSM we are smart enough to ignore this redirection.
    if (sTypeToken.GetHandle() == KnownTypeIndex::VisualState)
    {
        const auto propertyBase = MetadataAPI::GetPropertyBaseByIndex(KnownPropertyIndex::VisualState___DeferredStoryboard);
        rPropertyToken = XamlPropertyToken::FromProperty(propertyBase);
        rContentPropertyTypeToken = XamlTypeToken::FromType(propertyBase->GetPropertyType());
    }
    // Important note about TYPE_VALUE_TYPE:
    // We've added this exclusion because value types are not content or items
    // controls; you should not be able to add a Button, for instance, to an
    // integer.
    //
    // Unrelated:
    // When m_nContent is zero it means there is no content property,
    // but we prevent the lookup because the metadata has
    // DependencyObject.Name in slot 0.
    //
    // To maintain compatibility with Blue parser behavior, we will not retrieve the content property
    // for types which are value types or primitives.
    //
    // In particular, String is a reference type but still a primitive and in Blue we
    // were not returning the ContentProperty for it.
    else if (!pClassInfo->IsValueType() &&
             !pClassInfo->IsPrimitive() &&
              pClassInfo->GetContentProperty()->GetIndex() != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        auto pdp = pClassInfo->GetContentProperty();

        if (pdp)
        {
            rPropertyToken = XamlPropertyToken::FromProperty(pdp);
            rContentPropertyTypeToken = XamlTypeToken::FromType(pdp->GetPropertyType());
        }
        else
        {
            rPropertyToken.Reset();
            rContentPropertyTypeToken.Reset();
        }
    }

    RRETURN(hr);
}

// Collection types may optionally provide a type used to wrap literal
// content from the parser before inserting into the collection.  The
// parser won't actually wrap the content, but merely knows it's safe to
// pass literal content to the collection which will handle it.
//
// TODO: Need to support multiple content wrappers per type token and let the parser resolve the most appropriate.  Change to return an xvector of type tokens.
_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetContentWrapper(
    _In_ const XamlTypeToken sTypeToken,
    _Out_opt_ XamlTypeToken& rContentWrapperTypeToken
    )
{
    HRESULT hr = S_OK;

    // Only native types should be using NativeTypeInfoProvider
    ASSERT(sTypeToken.GetProviderKind() == tpkNative);

    // Currently only the InlineCollection associated with TextBlocks requires
    // its literal string content to be wrapped with Runs, so we've hard coded
    // the check here instead of modifying the type table in XcpTypes.h with a
    // new flag.  If more types require content wrappers or if XcpTypes
    // undergoes an overhaul, this method should be updated to reflect that.
    if (sTypeToken.GetHandle() == KnownTypeIndex::InlineCollection)
    {
        rContentWrapperTypeToken = XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Run));
    }
    else
    {
        rContentWrapperTypeToken = XamlTypeToken();
    }

    RRETURN(hr);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetRuntimeNameProperty(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlPropertyToken& rPropertyToken,
    _Out_ XamlTypeToken& rPropertyTypeToken
    )
{
    ASSERT(sTypeToken.GetProviderKind() == tpkNative);

    const auto propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(KnownPropertyIndex::DependencyObject_Name);

    if (propertyBase)
    {
        rPropertyToken = XamlPropertyToken::FromProperty(propertyBase);
        rPropertyTypeToken = XamlTypeToken::FromType(propertyBase->GetPropertyType());
    }

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetDictionaryKeyProperty(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlPropertyToken& rPropertyToken,
    _Out_ XamlTypeToken& rPropertyTypeToken
    )
{
    ASSERT(sTypeToken.GetProviderKind() == tpkNative);

    // The only SL3 type/property that behaved in this way is
    // the Style.TargetType property. If we have more than that, then
    // let's put that in the type table.
    if (sTypeToken.GetHandle() == KnownTypeIndex::Style)
    {
        const auto propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(KnownPropertyIndex::Style_TargetType);

        if (propertyBase)
        {
            rPropertyToken = XamlPropertyToken::FromProperty(propertyBase);
            rPropertyTypeToken = XamlTypeToken::FromType(propertyBase->GetPropertyType());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetXmlLangProperty(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlPropertyToken& rPropertyToken,
    _Out_ XamlTypeToken& rPropertyTypeToken
    )
{
    const CPropertyBase *propertyBase = nullptr;

    ASSERT(sTypeToken.GetProviderKind() == tpkNative);

    if (DirectUI::MetadataAPI::IsAssignableFrom<KnownTypeIndex::FrameworkElement>(sTypeToken.GetHandle()))
    {
        propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(KnownPropertyIndex::FrameworkElement_Language);
    }
    else if (DirectUI::MetadataAPI::IsAssignableFrom<KnownTypeIndex::TextElement>(sTypeToken.GetHandle()))
    {
        propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(KnownPropertyIndex::TextElement_Language);
    }

    if (propertyBase)
    {
        rPropertyToken = XamlPropertyToken::FromProperty(propertyBase);
        rPropertyTypeToken = XamlTypeToken::FromType(propertyBase->GetPropertyType());
    }

    return S_OK;
}

_Check_return_ HRESULT
XamlNativeTypeInfoProvider::GetCollectionItemType(
    _In_ const XamlTypeToken sCollectionTypeToken,
    _Out_ XamlTypeToken& rCollectionItemTypeToken
    )
{
    XamlPropertyToken propertyToken;

    ASSERT(sCollectionTypeToken.GetProviderKind() == tpkNative);

    // TODO: add verification that sCollectionTypeToken is actually
    // for a collection type.

    // our item type information for the collection item is stored in the same way
    // as the content property, so we can just call GetContentProperty and disregard the
    // actual property that is returned.
    IFC_RETURN(GetContentProperty(sCollectionTypeToken, propertyToken, rCollectionItemTypeToken));

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetTextSyntaxForType(
    _In_ const XamlTypeToken sTypeToken,
    _Out_ XamlTypeToken& sTextSyntaxTypeToken
    )
{
    HRESULT hr = S_OK;

    ASSERT(sTypeToken.GetProviderKind() == tpkNative);

    const CClassInfo* pClassInfo = NULL;

    pClassInfo = MetadataAPI::GetClassInfoByIndex(sTypeToken.GetHandle());
    ASSERT(pClassInfo);

    // This is somewhat inconsistent with the original intent of the returned
    // Type for the TextSyntax, but we don't have separate TypeConverter classes
    // for these types.
    if (pClassInfo->HasTypeConverter())
    {
        sTextSyntaxTypeToken = sTypeToken;
    }
    else if (pClassInfo->GetIndex() == KnownTypeIndex::Object || pClassInfo->GetIndex() == KnownTypeIndex::Uri)
    {
        // If we're targetting a property with type=Object or Type=Uri, then treat the value as a string.
        sTextSyntaxTypeToken = XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::String));
    }
    else
    {
        // TODO: Is there a better way to indicate that there is no
        // typeConverter on a property?
        sTextSyntaxTypeToken.Reset();
    }

    RRETURN(hr);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetTextSyntaxForProperty(
    _In_ const XamlPropertyToken sPropertyToken,
    _Out_ XamlTypeToken& sTextSyntaxTypeToken
    )
{
    const auto propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(sPropertyToken.GetHandle());

    if (propertyBase)
    {
        const auto pClassInfo = MetadataAPI::GetClassInfoByIndex(propertyBase->GetPropertyType()->m_nIndex);
        ASSERT(pClassInfo);

        // This is somewhat inconsistent with the original intent of the returned
        // Type for the TextSyntax, but we don't have separate TypeConverter classes
        // for these types.

        const auto propertyAsDP = propertyBase->AsOrNull<CDependencyProperty>();
        if (propertyAsDP && propertyAsDP->IsGridLengthProperty())
        {
            sTextSyntaxTypeToken = XamlTypeToken::FromType(
                MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::LengthConverter));
        }
        else if (pClassInfo->HasTypeConverter())
        {
            sTextSyntaxTypeToken = XamlTypeToken::FromType(pClassInfo);
        }
        else if (pClassInfo->m_nIndex == KnownTypeIndex::DependencyObject || pClassInfo->m_nIndex == KnownTypeIndex::Uri)
        {
            // TODO: this special logic shouldn't have to be in the specific
            // XamlTypeInfoProvider implementation
            sTextSyntaxTypeToken = XamlTypeToken::FromType(
                MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::String));
        }
        else
        {
            sTextSyntaxTypeToken.Reset();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetCore(
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

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetSchemaContext(
    _Out_ std::shared_ptr<XamlSchemaContext>& outSchemaContext
    )
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    IFCCHECK_RETURN(outSchemaContext);
    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsMarkupExtension(
    _In_ const XamlTypeToken& inTypeToken,
    _Out_ bool& outbIsMarkupExtension
    )
{
    HRESULT hr = S_OK;
    const CClassInfo* pClassInfo = NULL;

    ASSERT(inTypeToken.GetProviderKind() == tpkNative);

    pClassInfo = MetadataAPI::GetClassInfoByIndex(inTypeToken.GetHandle());
    ASSERT(pClassInfo);

    outbIsMarkupExtension = pClassInfo->IsMarkupExtension();

    RRETURN(hr);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsTypeConstructible(
    _In_ const XamlTypeToken& inTypeToken,
    _Out_ bool& outbIsConstructible
    )
{
    HRESULT hr = S_OK;
    const CClassInfo* pClassInfo = NULL;

    ASSERT(inTypeToken.GetProviderKind() == tpkNative);

    pClassInfo = MetadataAPI::GetClassInfoByIndex(inTypeToken.GetHandle());

    outbIsConstructible = (pClassInfo != NULL) && pClassInfo->IsConstructible();

    RRETURN(hr);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsTypePublic(
    _In_ const XamlTypeToken& inTypeToken,
    _Out_ bool& outbIsPublic
    )
{
    HRESULT hr = S_OK;
    const CClassInfo* pClassInfo = NULL;

    ASSERT(inTypeToken.GetProviderKind() == tpkNative);

    pClassInfo = MetadataAPI::GetClassInfoByIndex(inTypeToken.GetHandle());

    outbIsPublic = (pClassInfo != NULL) && pClassInfo->IsPublic();

    RRETURN(hr);
}

// Determine whether the type token represents a String type.
_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsString(
    _In_ const XamlTypeToken& inTypeToken,
    _Out_ bool& outbIsString
    )
{
    HRESULT hr = S_OK;

    ASSERT(inTypeToken.GetProviderKind() == tpkNative);
    outbIsString = KnownTypeIndex::String == inTypeToken.GetHandle();

    RRETURN(hr);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsPropertyEvent(
    _In_ const XamlPropertyToken& inPropertyToken,
    _Out_ bool& outbIsEvent
    )
{
    outbIsEvent = inPropertyToken.GetHandle() == MetadataAPI::EventPropertyIndex;
    RRETURN(S_OK);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsPropertyReadOnly(
    _In_ const XamlPropertyToken& inPropertyToken,
    _Out_ bool& outbIsReadOnly
    )
{
    outbIsReadOnly = FALSE;

    const auto propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(inPropertyToken.GetHandle());

    IFCEXPECT_RETURN(propertyBase != nullptr);
    outbIsReadOnly = propertyBase->IsReadOnly();

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsPropertyStatic(
    _In_ const XamlPropertyToken& inPropertyToken,
    _Out_ bool& outbIsStatic
    )
{
    // TODO: Do we have any static native properties??
    outbIsStatic = FALSE;
    RRETURN(S_OK);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsPropertyPublic(
    _In_ const XamlPropertyToken& inPropertyToken,
    _Out_ bool& outbIsPublic
    )
{
    outbIsPublic = FALSE;

    const auto propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(inPropertyToken.GetHandle());

    IFCEXPECT_RETURN(propertyBase != nullptr);

    // TODO: Is this correct?
    // TODO: Quirks mode. ~2061 parser.cpp
    outbIsPublic = TRUE;

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsPropertyAttachable(
    _In_ const XamlPropertyToken& inPropertyToken,
    _Out_ bool& outbIsAttachable
    )
{
    outbIsAttachable = FALSE;

    const auto propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(inPropertyToken.GetHandle());
    IFCEXPECT_RETURN(propertyBase != nullptr);
    const auto dependencyProperty = propertyBase->AsOrNull<CDependencyProperty>();

    if (dependencyProperty != nullptr)
    {
        outbIsAttachable = dependencyProperty->IsAttached();
    }

    return S_OK;
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::IsTemplateType(
    _In_ const XamlTypeToken& inTypeToken,
    _Out_ bool& outbIsTemplateType
    )
{
    HRESULT hr = S_OK;

    outbIsTemplateType = DirectUI::MetadataAPI::IsAssignableFrom<KnownTypeIndex::TemplateContent>(inTypeToken.GetHandle());

    RRETURN(hr);
}

_Check_return_ HRESULT XamlNativeTypeInfoProvider::GetFullClassNameByIndex(
    _In_ const KnownTypeIndex nClassIndex,
    _Out_ xstring_ptr* pstrFullName
    )
{
    const CClassInfo* pClassInfo = DirectUI::MetadataAPI::GetClassInfoByIndex(nClassIndex);

    IFCEXPECT_RETURN(pClassInfo != NULL);
    *pstrFullName = pClassInfo->GetFullName();

    return S_OK;
}
