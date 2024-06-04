// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Binding.g.h"
#include "CustomClassInfo.h"
#include "CustomDependencyProperty.h"
#include "PropertyPath.g.h"
#include "XamlParserCallbacks.h"
// Pull in Framework DirectUI::XamlQualifiedObject
#include "DirectUIXamlQualifiedObject.h"
// Pull in Core XamlQualifiedObject
#include "XamlQualifiedObject.h"
#include "MarkupExtension.g.h"
#include "ParserServiceProvider.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// TODO: Provide more detailed error information (on par with the error details
// in System.Windows.dll) once we've settled on our error handling solution for
// DXaml.

// Create an instance of the given DXaml type.  This is currently only defined
// for custom DXaml types derived from DependencyObject and uses
// TypeInfo::CreateInstance to instantiate the object
// (including it's peer core instance).
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_CreateInstance(
    _In_ XamlTypeToken tokType,
    _Out_ ::XamlQualifiedObject* pqoInstance)
{
    HRESULT hr = S_OK;

    const CClassInfo* pType = nullptr;
    ctl::ComPtr<IInspectable> spInstance;
    ctl::ComPtr<ISupportInitialize> spInitializable;
    DirectUI::XamlQualifiedObject* pValue = NULL;

    IFCEXPECT(tokType.GetProviderKind() != tpkUnknown);
    IFCEXPECT(tokType.GetHandle() != KnownTypeIndex::UnknownType);
    IFCPTR(pqoInstance);

    // Get the type.
    pType = MetadataAPI::GetClassInfoByIndex(tokType.GetHandle());

    // Create a new instance.
    IFC(ActivationAPI::ActivateInstance(pType, &spInstance));

    // If the object is an ISupportInitialize call it now.
    if (pType->IsISupportInitialize())
    {
        IFC(spInstance.As(&spInitializable));
        IFC(spInitializable->BeginInit());
    }

    // Store the instance in the XQO.
    pValue = (DirectUI::XamlQualifiedObject*)pqoInstance;
    IFC(pValue->SetValue(spInstance.Get(), pType, true));

Cleanup:
    RRETURN(hr);
}

// Create an instance of the given DXaml type by parsing a textual
// representation of the value.  This only DXaml types that have an associated
// text syntax are custom enums.  This method is more closely tied to the static
// type tables in TypeInfo.g.h than it should be, but we're making that
// simplification for now until we can decide our generalized text syntax
// strategy.
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_CreateFromValue(
    _In_ void* pServiceProviderContext,
    _In_ XamlTypeToken tokTextSyntaxType,
    _In_ ::XamlQualifiedObject* pqoValue,
    _In_ XamlPropertyToken tokProperty,
    _In_ ::XamlQualifiedObject* pqoRootInstance,
    _Out_ ::XamlQualifiedObject* pqoInstance)
{
    const CClassInfo* pType = nullptr;
    DirectUI::XamlQualifiedObject* pResult = nullptr;
    DirectUI::XamlQualifiedObject* pValue = nullptr;
    ctl::ComPtr<IInspectable> spInstance;

    IFCEXPECT_RETURN(tokTextSyntaxType.GetProviderKind() != tpkUnknown);
    IFCEXPECT_RETURN(tokTextSyntaxType.GetHandle() != KnownTypeIndex::UnknownType);

    // Get the type.
    pType = MetadataAPI::GetClassInfoByIndex(tokTextSyntaxType.GetHandle());

    pValue = (DirectUI::XamlQualifiedObject*)pqoValue;
    pResult = (DirectUI::XamlQualifiedObject*)pqoInstance;

    // Activate type from the string.
    IFC_RETURN(ActivationAPI::ActivateInstanceFromString(pType, pValue->GetValue().AsString(), &spInstance));

    IFC_RETURN(pResult->SetValue(spInstance.Get(), pType, /* fNewlyCreated */ TRUE));

    XamlTypeToken& tokResult = pResult->GetToken();
    tokResult = XamlTypeToken::FromType(pType);

    return S_OK;
}

// Get the value of a member on a particular instance.
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_GetValue(
    _In_ const ::XamlQualifiedObject* pqoInstance,
    _In_ XamlPropertyToken tokProperty,
    _Out_ ::XamlQualifiedObject* pqoValue)
{
    HRESULT hr = S_OK;

    const DirectUI::XamlQualifiedObject* pQualifiedInstance = nullptr;
    DirectUI::XamlQualifiedObject* pQualifiedValue = nullptr;
    const CDependencyProperty* pProperty = nullptr;
    ctl::ComPtr<IInspectable> spInstance;
    ctl::ComPtr<IInspectable> spValue;
    ctl::ComPtr<DependencyObject> spDO;

    IFCPTR(pqoInstance);
    IFCEXPECT(!tokProperty.IsUnknown());
    IFCPTR(pqoValue);

    // Get the instance.
    pQualifiedInstance = reinterpret_cast<const DirectUI::XamlQualifiedObject*>(pqoInstance);
    IFC(pQualifiedInstance->GetValue(&spInstance));

    // Get the value storage.
    pQualifiedValue = reinterpret_cast<DirectUI::XamlQualifiedObject*>(pqoValue);

    // Get the property.
    pProperty = MetadataAPI::GetPropertyByIndex(tokProperty.GetHandle());

    if (auto customProperty = pProperty->AsOrNull<CCustomProperty>())
    {
        IFC(customProperty->GetXamlPropertyNoRef()->GetValue(spInstance.Get(), &spValue));
    }
    else
    {
        IFC(spInstance.As(&spDO));
        IFC(spDO->GetValue(pProperty, &spValue));
    }

    IFC(pQualifiedValue->SetValue(spValue.Get(), pProperty->GetPropertyType(), /* fNewlyCreated */ true));

Cleanup:
    RRETURN(hr);
}

// Get the ambient value of a member on a particular instance.
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_GetAmbientValue(
    _In_ const ::XamlQualifiedObject* pqoInstance,
    _In_ XamlPropertyToken tokProperty,
    _Out_ ::XamlQualifiedObject* pqoValue)
{
    HRESULT hr = S_OK;

    const DirectUI::XamlQualifiedObject* pQualifiedInstance = nullptr;
    DirectUI::XamlQualifiedObject* pQualifiedValue = nullptr;
    ctl::ComPtr<IInspectable> spInstance;
    ctl::ComPtr<DependencyObject> spDO;
    const CDependencyProperty* pDP = nullptr;
    ctl::ComPtr<IInspectable> spValue;

    IFCEXPECT(!tokProperty.IsUnknown());

    // Get the instance.
    pQualifiedInstance = (DirectUI::XamlQualifiedObject*)pqoInstance;
    IFC(pQualifiedInstance->GetValue(&spInstance));

    // Get the value.
    pQualifiedValue = (DirectUI::XamlQualifiedObject*)pqoValue;

    // Get the member.
    pDP = MetadataAPI::GetPropertyByIndex(tokProperty.GetHandle());

    if (auto customProperty = pDP->AsOrNull<CCustomProperty>())
    {
        IFC(customProperty->GetXamlPropertyNoRef()->GetValue(spInstance.Get(), &spValue));
    }
    else
    {
        bool bAvailable = true;
        IFC(spInstance.As(&spDO));
        if (!pDP->IsSparse())
        {
            IFC(spDO->GetHandle()->IsAmbientPropertyValueAvailable(pDP, &bAvailable));
        }

        if (bAvailable)
        {
            IFC(spDO->GetValue(pDP, &spValue));
        }
    }

    if (spValue != nullptr)
    {
        IFC(pQualifiedValue->SetValue(spValue.Get(), pDP->GetPropertyType(), /* fNewlyCreated */ true));
    }

Cleanup:
    RRETURN(hr);
}

// Set the value of a member on a particular instance.  This currently fails if
// the member is an event.
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_SetValue(
    _In_ ::XamlQualifiedObject* pqoInstance,
    _In_ XamlPropertyToken tokProperty,
    _In_ ::XamlQualifiedObject* pqoValue)
{
    HRESULT hr = S_OK;

    DirectUI::XamlQualifiedObject* pQualifiedInstance = nullptr;
    DirectUI::XamlQualifiedObject* pQualifiedValue = nullptr;
    const CDependencyProperty* pProperty = NULL;
    const CClassInfo *pPropertyType = NULL;
    ctl::ComPtr<IInspectable> spInstance;
    ctl::ComPtr<IInspectable> spValue;
    ctl::ComPtr<wf::IUriRuntimeClass> spBaseUri;
    ctl::ComPtr<IFrameworkElement> spInstanceAsFE;
    ctl::ComPtr<DependencyObject> spInstanceAsDO;

    IFCPTR(pqoInstance);
    IFCEXPECT(!tokProperty.IsUnknown());
    IFCPTR(pqoValue);

    // Get the instance.
    pQualifiedInstance = (DirectUI::XamlQualifiedObject*)pqoInstance;
    IFC(pQualifiedInstance->GetValue(&spInstance));

    // Get the value.
    pQualifiedValue = (DirectUI::XamlQualifiedObject*)pqoValue;

    // Get the property.
    pProperty = MetadataAPI::GetPropertyByIndex(tokProperty.GetHandle());

    pPropertyType = pProperty->GetPropertyType();
    if (pPropertyType->GetIndex() == KnownTypeIndex::Uri)
    {
        spInstanceAsFE = spInstance.AsOrNull<IFrameworkElement>();
        if (spInstanceAsFE != nullptr)
        {
            IFC(spInstanceAsFE->get_BaseUri(&spBaseUri));
        }
    }
    IFC(CValueBoxer::UnboxObjectValue(&(pQualifiedValue->GetValue()), pPropertyType, &spValue));

    if (auto customProperty = pProperty->AsOrNull<CCustomProperty>())
    {
        IFC(customProperty->GetXamlPropertyNoRef()->SetValue(spInstance.Get(), spValue.Get()));
    }
    else
    {
        IFC(spInstance.As(&spInstanceAsDO));
        IFC(spInstanceAsDO->SetValue(pProperty, spValue.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Provide a value for a markup extension.
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_CallProvideValue(
    _In_ ::XamlQualifiedObject* pqoMarkupExtension,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _Out_ ::XamlQualifiedObject* pqoOutValue)
{
    DirectUI::XamlQualifiedObject* pQualifiedInstance = nullptr;
    DirectUI::XamlQualifiedObject* pQualifiedValue = nullptr;
    ctl::ComPtr<IInspectable> spInstance;
    ctl::ComPtr<IInspectable> spProvidedValue;
    const CClassInfo* typeInfo = nullptr;

    // Get the instance.
    pQualifiedInstance = (DirectUI::XamlQualifiedObject*)pqoMarkupExtension;
    IFC_RETURN(pQualifiedInstance->GetValue(&spInstance));

    // Get the value storage.
    pQualifiedValue = (DirectUI::XamlQualifiedObject*)pqoOutValue;

    // Create the IXamlServiceProvider to give to the markup extension
    ctl::ComPtr<ParserServiceProvider> xamlServiceProvider;
    IFC_RETURN(ctl::make(spServiceProviderContext, &xamlServiceProvider));

    // Provide the value.
    // First try as DirectUI::MarkupExtension. If that fails (i.e. not a custom
    // markup extension), then fall back to DirectUI::MarkupExtensionBase.
    ctl::ComPtr<MarkupExtension> spMarkupExtension = spInstance.AsOrNull<MarkupExtension>();
    if (spMarkupExtension)
    {
        IFC_RETURN(spMarkupExtension->ProvideValueWithIXamlServiceProviderProtected(xamlServiceProvider.Get(), &spProvidedValue));
        IFC_RETURN(MetadataAPI::GetClassInfoFromObject_ResolveWinRTPropertyOtherType(spProvidedValue.Get(), &typeInfo));
    }
    else
    {
        KnownTypeIndex providedValueTypeIndex = KnownTypeIndex::UnknownType;
        ctl::ComPtr<MarkupExtensionBase> spMarkupExtensionBase = spInstance.AsOrNull<MarkupExtensionBase>();
        IFC_RETURN(spInstance.As(&spMarkupExtensionBase));
        IFC_RETURN(spMarkupExtensionBase->ProvideValue(xamlServiceProvider.Get(), &spProvidedValue, &providedValueTypeIndex));
        typeInfo = MetadataAPI::GetClassInfoByIndex(providedValueTypeIndex);
    }

    // Return the value.
    IFC_RETURN(pQualifiedValue->SetValue(spProvidedValue.Get(), typeInfo, /* fNewlyCreated */ TRUE));

    return S_OK;
}

// Add a value to an instance of a collection.  This currently only works for
// types derived from PresentationFrameworkCollection.
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_Add(
    _In_ ::XamlQualifiedObject* pqoCollection,
    _In_ ::XamlQualifiedObject* pqoValue)
{
    HRESULT hr = S_OK;

    DirectUI::XamlQualifiedObject* pQualifiedInstance = nullptr;
    DirectUI::XamlQualifiedObject* pQualifiedValue = nullptr;
    ctl::ComPtr<IInspectable> spInstance;
    ctl::ComPtr<IInspectable> spValue;
    const CClassInfo* pCollectionType = nullptr;

    IFCPTR(pqoCollection);

    // Get the instance.
    pQualifiedInstance = (DirectUI::XamlQualifiedObject*)pqoCollection;
    IFC(pQualifiedInstance->GetValue(&spInstance));

    // Get the value.
    pQualifiedValue = (DirectUI::XamlQualifiedObject *)pqoValue;

    // Get the collection type.
    pCollectionType = MetadataAPI::GetClassInfoByIndex(pQualifiedInstance->GetToken().GetHandle());

    // Add the value.
    IFC(CValueBoxer::UnboxObjectValue(&(pQualifiedValue->GetValue()), /* pTargetType */  nullptr, &spValue));

    if (pCollectionType->IsBuiltinType())
    {
        ctl::ComPtr<IUntypedVector> spVector;
        IFC(spInstance.As(&spVector));
        IFC(spVector->UntypedAppend(spValue.Get()));
    }
    else
    {
        IFC(pCollectionType->AsCustomType()->GetXamlTypeNoRef()->AddToVector(spInstance.Get(), spValue.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Add a key/value pair to an instance of a dictionary.
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_AddToDictionary(
    _In_ ::XamlQualifiedObject* pqoDictionary,
    _In_ ::XamlQualifiedObject* pqoKey,
    _In_ ::XamlQualifiedObject* pqoValue)
{
    HRESULT hr = S_OK;

    DirectUI::XamlQualifiedObject* pQualifiedInstance = nullptr;
    DirectUI::XamlQualifiedObject* pQualifiedKey = nullptr;
    DirectUI::XamlQualifiedObject* pQualifiedValue = nullptr;
    const CClassInfo *pDictionaryType = nullptr;
    ctl::ComPtr<IInspectable> spInstance;
    ctl::ComPtr<IInspectable> spKey;
    ctl::ComPtr<IInspectable> spValue;

    // Get the instance.
    pQualifiedInstance = (DirectUI::XamlQualifiedObject*)pqoDictionary;
    IFC(pQualifiedInstance->GetValue(&spInstance));

    // Get the key.
    pQualifiedKey = (DirectUI::XamlQualifiedObject *)pqoKey;

    // Get the value.
    pQualifiedValue = (DirectUI::XamlQualifiedObject *)pqoValue;

    // Get the dictionary type.
    pDictionaryType = MetadataAPI::GetClassInfoByIndex(pQualifiedInstance->GetToken().GetHandle());

    // Get the key/value.
    IFC(CValueBoxer::UnboxObjectValue(&(pQualifiedKey->GetValue()), /* pTargetType */  NULL, &spKey));
    IFC(CValueBoxer::UnboxObjectValue(&(pQualifiedValue->GetValue()), /* pTargetType */  NULL, &spValue));

    // Add the key/value to the dictionary.
    if (pDictionaryType->IsBuiltinType())
    {
        // We don't currently expect to have any custom dictionaries.
        IFCEXPECT(FALSE);
    }
    else
    {
        IFC(pDictionaryType->AsCustomType()->GetXamlTypeNoRef()->AddToMap(spInstance.Get(), spKey.Get(), spValue.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// The parser has found an x:ConnectionId attribute and needs to wire up the event based on the connection Id.
_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_SetConnectionId(
    _In_ ::XamlQualifiedObject* qoComponentConnector,
    _In_ ::XamlQualifiedObject* qoConnectionId,
    _In_ ::XamlQualifiedObject* qoTarget)
{
    ctl::ComPtr<IInspectable> spComponentConnectorAsIInspectable;
    ctl::ComPtr<IWeakInspectable> spComponentConnectorAsValueWeakReference;
    XINT32 nConnectionId = 0;
    ctl::ComPtr<IInspectable> spTarget;

    static_assert(sizeof(::XamlQualifiedObject) == sizeof(DirectUI::XamlQualifiedObject), "Possibly incompatible types for cast");

    // Get the instance (we're not returning this to the caller, so we don't need it to have a PegNoRef).
    IFC_RETURN(reinterpret_cast<DirectUI::XamlQualifiedObject*>(qoComponentConnector)->GetValue(&spComponentConnectorAsIInspectable, FALSE /*fPegNoRef*/));

    spComponentConnectorAsValueWeakReference = spComponentConnectorAsIInspectable.AsOrNull<IWeakInspectable>();

    if (spComponentConnectorAsValueWeakReference)
    {
        IFC_RETURN(spComponentConnectorAsValueWeakReference->GetInspectable(&spComponentConnectorAsIInspectable));
    }

    // Get the id
    IFC_RETURN(reinterpret_cast<DirectUI::XamlQualifiedObject*>(qoConnectionId)->GetValue().GetSigned(nConnectionId));

    // Get the target (we're not returning this to the caller, so we don't need it to have a PegNoRef).
    IFC_RETURN(reinterpret_cast<DirectUI::XamlQualifiedObject*>(qoTarget)->GetValue(&spTarget, /* fPegNoRef */ FALSE));

    ctl::ComPtr<xaml_markup::IComponentConnector> spComponentConnector = spComponentConnectorAsIInspectable.AsOrNull<xaml_markup::IComponentConnector>();

    if (spComponentConnector)
    {
        IFC_RETURN(spComponentConnector->Connect(nConnectionId, spTarget.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlParserCallbacks::XamlManagedRuntimeRPInvokes_GetXBindConnector(
    _In_ ::XamlQualifiedObject* qoComponentConnector,
    _In_ ::XamlQualifiedObject* qoConnectionId,
    _In_ ::XamlQualifiedObject* qoTarget,
    _In_ ::XamlQualifiedObject* qoReturnConnector)
{
    ctl::ComPtr<IInspectable> spComponentConnectorAsIInspectable;
    XINT32 nConnectionId = 0;
    ctl::ComPtr<IInspectable> spTarget;

    static_assert(sizeof(::XamlQualifiedObject) == sizeof(DirectUI::XamlQualifiedObject), "Possibly incompatible types for cast");

    // Get the instance (we're not returning this to the caller, so we don't need it to have a PegNoRef).
    IFC_RETURN(reinterpret_cast<DirectUI::XamlQualifiedObject*>(qoComponentConnector)->GetValue(&spComponentConnectorAsIInspectable, FALSE /*fPegNoRef*/));

    // Get the id
    IFC_RETURN(reinterpret_cast<DirectUI::XamlQualifiedObject*>(qoConnectionId)->GetValue().GetSigned(nConnectionId));

    // Get the target (we're not returning this to the caller, so we don't need it to have a PegNoRef).
    IFC_RETURN(reinterpret_cast<DirectUI::XamlQualifiedObject*>(qoTarget)->GetValue(&spTarget, /* fPegNoRef */ FALSE));

    ctl::ComPtr<xaml_markup::IComponentConnector> spComponentConnector = spComponentConnectorAsIInspectable.AsOrNull<xaml_markup::IComponentConnector>();

    if (spComponentConnector)
    {
        ctl::ComPtr<xaml_markup::IComponentConnector> spReturnConnector;

        IFC_RETURN(spComponentConnector->GetBindingConnector(nConnectionId, spTarget.Get(), &spReturnConnector));

        if (spReturnConnector)
        {
            ctl::ComPtr<IInspectable> spReturnConnectorValueWeakReference;
            IFC_RETURN(ValueWeakReference::Create(ctl::iinspectable_cast(spReturnConnector.Get()), &spReturnConnectorValueWeakReference));
            reinterpret_cast<DirectUI::XamlQualifiedObject*>(qoReturnConnector)->GetValue().SetIInspectableAddRef(spReturnConnectorValueWeakReference.Get());
        }
    }

    return S_OK;
}

// Validate that the type loaded from XAML matches the object type.
_Check_return_ HRESULT XamlParserCallbacks::FrameworkCallbacks_CheckPeerType(
    _In_ CDependencyObject* nativeRoot,
    _In_ const xstring_ptr& strPeerType,
    _In_ XINT32 bCheckExact)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    if (nativeRoot && DependencyObjectTraits<CApplication>::Index == nativeRoot->GetTypeIndex())
    {
        // Special case: we allow any value for x:class when parsing app.xaml.
        goto Cleanup;
    }

    IFC(pCore->GetPeer(nativeRoot, &spPeer));
    if (spPeer == nullptr)
    {
        // No peer found.
        IFC(E_FAIL);
    }
    else
    {
        const CClassInfo *pCurrentTypeInfo = NULL;
        xstring_ptr strRuntimeClassName;

        // Get the class name of the instance.
        IFC(MetadataAPI::GetRuntimeClassName(ctl::iinspectable_cast(spPeer.Get()), &strRuntimeClassName));

        if (strRuntimeClassName.Equals(strPeerType))
        {
            goto Cleanup;
        }

        // If the class names don't match, fall back on getting the concrete type of the instance.
        // We'll do a name check based on the concrete type (and potentially its ancestors)'s name.
        IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(ctl::iinspectable_cast(spPeer.Get()), &pCurrentTypeInfo));
        bool hasNamespace = (strPeerType.FindChar(L'.') != xstring_ptr_view::npos);

        do
        {
            xstring_ptr strCurrentTypeInfoName(pCurrentTypeInfo->GetFullName());
            if (hasNamespace || bCheckExact)
            {
                if (strCurrentTypeInfoName.Equals(strPeerType))
                {
                    goto Cleanup;
                }
            }
            else
            {
                // TODO: Fix this. This was disabled during the type table merge.
                ASSERT(FALSE);
                IFC(E_NOTIMPL);
                /*bool endMatches = (strCurrentTypeInfoName.EndsWith(pCurrentTypeInfo->GetName(), xstrCompareCaseSensitive) > 0);

                if (endMatches)
                {
                    for (XUINT32 i = 0; i < ARRAY_SIZE(ConstNamespaces); i++)
                    {
                        if (strCurrentTypeInfoName.GetCount() == c_strNamespaceNameStorage[i].Count + 1 + strPeerType.GetCount())
                        {
                            if (   strCurrentTypeInfoName.StartsWith(xstring_ptr(c_strNamespaceNameStorage[i]))
                                && strCurrentTypeInfoName.GetBuffer()[c_strNamespaceNameStorage[i].Count] == L'.'
                                && strCurrentTypeInfoName.EndsWith(strPeerType))
                            {
                                goto Cleanup;
                            }
                        }
                    }
                }*/
            }

            if (bCheckExact)
            {
                // don't walk tree
                break;
            }
            else
            {
                pCurrentTypeInfo = pCurrentTypeInfo->GetBaseType();
            }
        } while (pCurrentTypeInfo->GetIndex() != KnownTypeIndex::UnknownType);
    }

    hr = E_FAIL;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlParserCallbacks::AddXmlnsDefinition(
    _In_ const xstring_ptr& strXmlNamespace,
    _In_ const xstring_ptr& strNamespace)
{
    HRESULT hr = S_OK;
    const CNamespaceInfo* pNamespace = nullptr;
    XamlTypeNamespaceToken tkNamespace;

    // Find the namespace.
    IFC(MetadataAPI::GetNamespaceByName(strNamespace, &pNamespace));

    // Register the mapping.
    tkNamespace.SetProviderKind(tpkManaged);
    tkNamespace.SetHandle(pNamespace->GetIndex());

    // According to the core this is hacky
    tkNamespace.AssemblyToken.SetProviderKind(tpkNative);
    tkNamespace.AssemblyToken.SetHandle(1);

    IFC(CoreImports::XamlSchemaContext_AddAssemblyXmlnsDefinition(
        static_cast<CCoreServices *>(DXamlCore::GetCurrent()->GetHandle()),
        tkNamespace.AssemblyToken,
        strXmlNamespace,
        tkNamespace,
        strNamespace));

Cleanup:
    RRETURN(hr);
}

