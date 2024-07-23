// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"
#include "ThemeResource.h"
#include "DeferredMapping.h"
//------------------------------------------------------------------------
//
//  Method:   CreateInstance
//
//  Synopsis:
//      Creates an instance of the type represented by the type token
//      returning it in the XamlQualifiedObject.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlManagedRuntime::CreateInstance(
            _In_ const XamlTypeToken& inXamlType,
            // XamlQualifiedObject args, // TODO: do we need args?
            _Out_ std::shared_ptr<XamlQualifiedObject>& returnQo)
{
    auto qo = std::make_shared<XamlQualifiedObject>();
    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_CreateInstance(inXamlType, qo.get()));
    returnQo = std::move(qo);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CreateFromValue
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
//CreateFromValue is expected to convert the provided value via any applicable converter (on property or type) or provide the original value if there is no converter
_Check_return_ HRESULT XamlManagedRuntime::CreateFromValue(
            _In_ std::shared_ptr<XamlServiceProviderContext> inServiceContext, // NOTE: This is deliberately no const-ref because is cast
            _In_ const XamlTypeToken& inTextSyntaxToken,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue,
            _In_ const XamlPropertyToken& inProperty,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoRootInstance,
            _In_ bool fIsPropertyAssignment,
            _Out_ std::shared_ptr<XamlQualifiedObject>& qo)
{
    qo = std::make_shared<XamlQualifiedObject>();
    XamlQualifiedObject qoStackValue;
    IFC_RETURN(qoValue->ConvertForManaged(qoStackValue));

    XamlQualifiedObject qoLocalRootInstance;
    if (qoRootInstance)
    {
        IFC_RETURN(qoRootInstance->ConvertForManaged(qoLocalRootInstance));
    }

    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_CreateFromValue(NULL, inTextSyntaxToken, &qoStackValue, inProperty, &qoLocalRootInstance, qo.get()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Xxxx
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlManagedRuntime::CreateFromXmlText(
            _In_ const XamlTypeToken& inXamlType,
            _In_ const xstring_ptr& inText,
            _Out_ std::shared_ptr<XamlQualifiedObject>& qo)
{
    IFC_RETURN(E_NOTIMPL);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetValue
//
//  Synopsis:
//      xxx
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlManagedRuntime::GetValue(
            _In_ const XamlQualifiedObject& qoObj,
            _In_ const XamlPropertyToken& inProperty,
            _Out_ XamlQualifiedObject& outValue)
{
    outValue.Clear();

    XamlQualifiedObject qo;
    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_GetValue(&qoObj, inProperty, &qo));

    outValue = std::move(qo);
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
_Check_return_ HRESULT XamlManagedRuntime::GetAmbientValue(
            _In_ const XamlQualifiedObject& qoObj,
            _In_ const XamlPropertyToken& inProperty,
            _Out_ XamlQualifiedObject& outValue)
{
    outValue.Clear();

    XamlQualifiedObject qo;
    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_GetAmbientValue(&qoObj, inProperty, &qo));

    outValue = std::move(qo);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Xxxx
//
//  Synopsis:
//      Sets a value on the provided object.
//
//      NOTE: The XamlQualifiedObject you pass as inValue will be cleared
//      if the function succeeds. This doesn't require anything from the
//      caller, because the XQO it passed in owned the contents and will
//      release it when it goes out of scope in the case that the call
//      fails. The only important thing is not to try to use the
//      object after this call because ownership has been given away.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlManagedRuntime::SetValue(
            _In_ const std::shared_ptr<XamlQualifiedObject>& inObj,
            _In_ const XamlPropertyToken& inProperty,
            _In_ const std::shared_ptr<XamlQualifiedObject>& inValue,
            _In_ bool bBindTemplates)
{
    HRESULT hr = S_OK;

    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::shared_ptr<XamlProperty> spProperty;

    // if the value is a CManagedObjectReference, there isn't anything
    // clever that we can do on the native side -- we should just fall
    // through to the managed implementation of SetValue.
    bool fIsPureSetValue = do_pointer_cast<CManagedObjectReference>(inValue->GetValue()) != nullptr;

    ASSERT(!!inObj);

    IFC(GetSchemaContext(spSchemaContext));
    IFC(spSchemaContext->GetXamlProperty(inProperty, spProperty));

    // TODO: this is a bit of a hack since we don't have a generic way
    // to handle expressions in native code. Better way to do this?
    // NOTE: When we do initial template validation we set BindTemplates to false
    // so that we don't try to bind where the parent may not be known.
    if (!fIsPureSetValue && (
        (inValue->GetTypeToken().GetHandle() == KnownTypeIndex::TemplateBinding) ||
        (inValue->GetTypeToken().GetHandle() == KnownTypeIndex::Binding) ||
        (inValue->GetTypeToken().GetHandle() == KnownTypeIndex::ThemeResource)))
    {
        switch (inValue->GetTypeToken().GetHandle())
        {
            case KnownTypeIndex::Binding:
                {
                    CBinding* pBinding = NULL;

                    CDependencyObject *pdo = inObj->GetDependencyObject();
                    IFC(DoPointerCast(pBinding, inValue->GetDependencyObject()));
                    IFC(pBinding->SetBinding(pdo, inProperty.GetHandle()));
                }
                break;
            case KnownTypeIndex::TemplateBinding:
                {
                    CDependencyObject *pdo = inObj->GetDependencyObject();
                    CTemplateBindingExtension *pTemplateBinding = NULL;
                    IFC(DoPointerCast(pTemplateBinding, inValue->GetDependencyObject()));

                    if (bBindTemplates)
                    {
                        IFC(pTemplateBinding->SetTemplateBinding(pdo, inProperty));
                        pTemplateBinding->SetTemplateBindingComplete();

                        // Returns S_FALSE if the SetTemplateBinding virtual method is not overridden
                        if (hr == S_FALSE)
                        {
                            hr = S_OK; // Do not propagate S_FALSE
                            goto Cleanup;
                        }
                    }
                    else
                    {
                        // if we only expanded the templates for validation but did not apply the template bindings
                        // we should still indicate that the template binding was completed and the extension can be reused
                        pTemplateBinding->SetTemplateBindingComplete();
                    }
                }
                break;
            case KnownTypeIndex::ThemeResource:
                {
                    CDependencyObject *pdo = inObj->GetDependencyObject();
                    CThemeResourceExtension *pThemeResourceExtension = nullptr;
                    CThemeResource* pThemeResource = nullptr;
                    const CDependencyProperty* pProperty = DirectUI::MetadataAPI::GetPropertyByIndex(inProperty.GetHandle());

                    IFC(DirectUI::MetadataAPI::GetUnderlyingDependencyProperty(pProperty, &pProperty));

                    if (inValue->GetValue().GetType() == valueObject)
                    {
                        IFC(DoPointerCast(pThemeResourceExtension, inValue->GetDependencyObject()));
                        IFC(pThemeResourceExtension->SetThemeResourceBinding(pdo, pProperty));
                    }
                    else
                    {
                        ASSERT(inValue->GetValue().GetType() == valueThemeResource);
                        pThemeResource = inValue->GetValue().AsThemeResource();
                        IFC(pThemeResource->SetThemeResourceBinding(pdo, pProperty));
                    }

                    break;
                }
        }
    }
    else
    {
        XamlQualifiedObject qoValue;
        IFC(inValue->ConvertForManaged(qoValue));
        IFC(FxCallbacks::XamlManagedRuntimeRPInvokes_SetValue(inObj.get(), inProperty, &qoValue));
    }

Cleanup:

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   Xxxx
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlManagedRuntime::Add(
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoCollection,
            _In_ const std::shared_ptr<XamlQualifiedObject>& inValue)
{
    XamlQualifiedObject qoLocalCollection;
    IFC_RETURN(qoCollection->ConvertForManaged(qoLocalCollection));

    XamlQualifiedObject qoValue;
    IFC_RETURN(inValue->ConvertForManaged(qoValue));
    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_Add(&qoLocalCollection, &qoValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   InitializationGuard
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlManagedRuntime::InitializationGuard(
            _In_ const std::shared_ptr<XamlType>& spXamlType,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance,
            _In_ bool fInitializing,
            _In_ const std::shared_ptr<XamlServiceProviderContext>& context)
{
    ASSERT(!!qoInstance);
    if (fInitializing)
    {
        auto pDependencyObject = qoInstance->GetDependencyObject();
        if (pDependencyObject)
        {
            pDependencyObject->SetIsParsing(TRUE);
            pDependencyObject->SetParserParentLock();
        }
    }
    else
    {
        auto pDependencyObject = qoInstance->GetDependencyObject();
        if (pDependencyObject)
        {
            bool bIsISupportInitialize = false;

            IFC_RETURN(pDependencyObject->CreationComplete());
            IFC_RETURN(spXamlType->IsISupportInitialize(bIsISupportInitialize));

            if (bIsISupportInitialize)
            {
                IFC_RETURN(FxCallbacks::FrameworkCallbacks_SupportInitializeEndInit(pDependencyObject, context));
            }

            pDependencyObject->ResetParserParentLock();
            pDependencyObject->SetIsParsing(FALSE);
        }

    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   AddToDictionary
//
//  Synopsis:
//      Adds items to a managed dictionary.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlManagedRuntime::AddToDictionary(
            _In_ const std::shared_ptr<XamlQualifiedObject>& dictionary,
            _In_ const std::shared_ptr<XamlQualifiedObject>& value,
            _In_ const std::shared_ptr<XamlQualifiedObject>& key)
{
    XamlQualifiedObject qoLocalDictionary;
    IFC_RETURN(dictionary->ConvertForManaged(qoLocalDictionary));

    XamlQualifiedObject qoValue;
    IFC_RETURN(value->ConvertForManaged(qoValue));

    XamlQualifiedObject qoKey;
    IFC_RETURN(key->ConvertForManaged(qoKey));
    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_AddToDictionary(&qoLocalDictionary, &qoKey, &qoValue));

    return S_OK;
}

_Check_return_ HRESULT XamlManagedRuntime::CallProvideValue(
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoMarkupExtension,
            _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _Out_ std::shared_ptr<XamlQualifiedObject>& outQo)
{
    XamlQualifiedObject qoLocalMarkupExtension;
    IFC_RETURN(qoMarkupExtension->ConvertForManaged(qoLocalMarkupExtension));

    auto qo = std::make_shared<XamlQualifiedObject>();
    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_CallProvideValue(
        &qoLocalMarkupExtension,
        spServiceProviderContext,
        qo.get()));
    outQo = std::move(qo);

    return S_OK;
}



_Check_return_ HRESULT XamlManagedRuntime::SetUriBase(
            _In_ const std::shared_ptr<XamlType>& spXamlType,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance,
            _In_ xref_ptr<IPALUri> spBaseUri)
{
    CDependencyObject* pDependencyObject = qoInstance->GetDependencyObject();

    IFCEXPECT_RETURN(pDependencyObject);

    if (pDependencyObject->GetCanParserOverwriteBaseUri())
    {
        pDependencyObject->SetBaseUri(spBaseUri.get());
    }
    return S_OK;
}

_Check_return_ HRESULT XamlManagedRuntime::SetConnectionId(
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoComponentConnector,
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoConnectionId,
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoTarget)
{
    XamlQualifiedObject qoManagedConnectionId;
    IFC_RETURN(qoConnectionId->ConvertForManaged(qoManagedConnectionId));

    XamlQualifiedObject qoManagedTarget;
    IFC_RETURN(qoTarget->ConvertForManaged(qoManagedTarget));

    // Setting the ParticipatesInManagedTreeDefault earlier than the set in AddEventListener
    // allows the first DXamlPeer created in the RPInvoke to be created with a PegNoRef.
    // Without this set, the DXamlPeer will be lost during parse at the end of this call.

    CDependencyObject* pTarget = qoTarget->GetDependencyObject();

    if (pTarget)
    {
        if (auto frameworkTemplate = do_pointer_cast<CFrameworkTemplate>(pTarget))
        {
            if (auto controlTemplate = do_pointer_cast<CControlTemplate>(pTarget))
            {
                int32_t connectionId = -1;
                IFC_RETURN(qoConnectionId->GetValue().GetSigned(connectionId));
                controlTemplate->SetConnectionId(connectionId);
            }
            frameworkTemplate->SetParentXBindConnector(qoComponentConnector);
        }
        IFC_RETURN(pTarget->SetParticipatesInManagedTreeDefault());
    }

    XamlQualifiedObject qoLocalComponentConnector;
    IFC_RETURN(qoComponentConnector->ConvertForManaged(qoLocalComponentConnector));

    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_SetConnectionId(&qoLocalComponentConnector, &qoManagedConnectionId, &qoManagedTarget));

    return S_OK;
}

_Check_return_ HRESULT XamlManagedRuntime::GetXBindConnector(
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoComponentConnector,
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoConnectionId,
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoTarget,
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoReturnConnector)
{
    XamlQualifiedObject qoManagedConnectionId;
    IFC_RETURN(qoConnectionId->ConvertForManaged(qoManagedConnectionId));

    XamlQualifiedObject qoManagedTarget;
    IFC_RETURN(qoTarget->ConvertForManaged(qoManagedTarget));

    // Setting the ParticipatesInManagedTreeDefault earlier than the set in AddEventListener
    // allows the first DXamlPeer created in the RPInvoke to be created with a PegNoRef.
    // Without this set, the DXamlPeer will be lost during parse at the end of this call.

    CDependencyObject* pTarget = qoTarget->GetDependencyObject();
    if (pTarget)
    {
        IFC_RETURN(pTarget->SetParticipatesInManagedTreeDefault());
    }

    XamlQualifiedObject qoLocalComponentConnector;
    IFC_RETURN(qoComponentConnector->ConvertForManaged(qoLocalComponentConnector));
    IFC_RETURN(FxCallbacks::XamlManagedRuntimeRPInvokes_GetXBindConnector(&qoLocalComponentConnector, &qoManagedConnectionId, &qoManagedTarget, qoReturnConnector.get()));

    return S_OK;
}

_Check_return_ HRESULT XamlManagedRuntime::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_UNEXPECTED);
    }
    return S_OK;
}

_Check_return_ HRESULT XamlManagedRuntime::AddDeferredProxy(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParent,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const XamlPropertyToken& inProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spProxy)
{
    KnownPropertyIndex inPropertyIndex = inProperty.GetHandle();

    if (inPropertyIndex == KnownPropertyIndex::SemanticZoom_ZoomedInView ||
        inPropertyIndex == KnownPropertyIndex::SemanticZoom_ZoomedOutView)
    {
        // These two properties are interfaces and go through Managed runtime.
        // For now they are the only two allowed - there is no custom property support.

        CDependencyObject* collection = nullptr;
        CDeferredElement* proxy = static_cast<CDeferredElement*>(spProxy->GetDependencyObject());
        CDependencyObject* scope = proxy->GetDeclaringOwner(); // get the scope from proxy, as in case of templates it will be template

        if (spParentCollection)
        {
            collection = spParentCollection->GetDependencyObject();
        }

        IFC_RETURN(scope->EnsureAndGetScopeDeferredStorage().AddProxy(
            scope,
            spParent->GetDependencyObject(),
            collection,
            inPropertyIndex,
            proxy));
    }
    else
    {
        return E_FAIL;
    }

    return S_OK;
}