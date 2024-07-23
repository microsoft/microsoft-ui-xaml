// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TypeTable.g.h"
#include "MetadataAPI.h"
#include "ThemeResource.h"
#include "DeferredMapping.h"
#include "XamlNativeRuntime_SimpleProperties.g.h"

using namespace DirectUI;

_Check_return_ HRESULT XamlNativeRuntime::CreateInstance(
            _In_ const XamlTypeToken& inXamlType,
            _Out_ std::shared_ptr<XamlQualifiedObject>& qo )
{
    HRESULT hr = S_OK;
    CCoreServices *pCore = nullptr;
    CDependencyObject *pdo = nullptr;
    XamlTypeToken sActualTypeToken;

    IFC(TryGetCachedInstance(inXamlType, qo));

    if (!qo)
    {
        const CREATEPFN pfnCreate = c_aTypeActivations[static_cast<XUINT32>(inXamlType.GetHandle())].m_pfnCreate;

        IFC(GetCore(&pCore));
        if (!pfnCreate)
        {
            std::shared_ptr<XamlSchemaContext> spSchemaContext;
            std::shared_ptr<XamlType> spType;
            xstring_ptr spTypeName;
            // Make sure we can create an instance of the type
            IFC(GetSchemaContext(spSchemaContext));
            IFC(spSchemaContext->GetXamlType(inXamlType, spType));

            IFCPTR(pCore);
            IFC(spType->get_FullName(&spTypeName));

            // Types without a create function can't be created, so we'll just bail
            // out and raise an error like "No matching constructor found on type
            // '%0'.".  If you come across this failure, check your metadata in
            // XcpTypes.h.
            IFC(CErrorService::OriginateInvalidOperationError(
                pCore,
                AG_E_PARSER2_OW_NO_CTOR,
                spTypeName));
        }

        {
            CREATEPARAMETERS cp(pCore);
            IFC(pfnCreate(&pdo, &cp));
        }

        IFC(GetTypeTokenForDO(pdo, sActualTypeToken));
        auto tempQo = std::make_shared<XamlQualifiedObject>(sActualTypeToken);
        IFC(tempQo->SetValue(inXamlType, pdo));
        qo = std::move(tempQo);
    }

Cleanup:
    ReleaseInterface(pdo);
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
//CreateFromValue is expected to convert the provided value via any applicable converter (on property or type) or provide the original value if there is no converter
_Check_return_ HRESULT XamlNativeRuntime::CreateFromValue(
            _In_ std::shared_ptr<XamlServiceProviderContext> inServiceContext,  // NOTE: This is deliberately no const-ref because is cast
            _In_ const XamlTypeToken& inTextSyntaxToken,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue,
            _In_ const XamlPropertyToken& inProperty,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoRootInstance,
            _In_ bool fIsPropertyAssignment,
            _Out_ std::shared_ptr<XamlQualifiedObject>& qo )
{
    qo.reset();

    CCoreServices* pCore = nullptr;
    IFC_RETURN(GetCore(&pCore));

    CREATEPARAMETERS cp(pCore, qoValue->GetValue());
    cp.m_spServiceProviderContext = inServiceContext;

    // Handle value types and enums.
    if (fIsPropertyAssignment)
    {
        auto valueType = qoValue->GetValue().GetType();

        if (valueType == valueString)
        {
            CValue value;

            switch (inTextSyntaxToken.GetHandle())
            {
                case KnownTypeIndex::Boolean:
                    IFC_RETURN(CBoolean::CreateCValue(&cp, value));
                    break;

                case KnownTypeIndex::Int32:
                    IFC_RETURN(CInt32::CreateCValue(&cp, value));
                    break;

                case KnownTypeIndex::Double:
                    IFC_RETURN(CDouble::CreateCValue(&cp, value));
                    break;
            }

            if (!value.IsUnset())
            {
                auto tempQo = std::make_shared<XamlQualifiedObject>();
                IFC_RETURN(tempQo->SetValue(value));
                qo = std::move(tempQo);
            }
        }
    }

    // If we haven't already handled the value, call the applicable
    // creation function to create a new DO converted from the value.
    if (qo == nullptr)
    {
        XamlTypeToken sActualTypeToken;
        xref_ptr<CDependencyObject> depObj;

        // Invoke the type converter
        const CREATEPFN pfnCreate = c_aTypeActivations[static_cast<XUINT32>(inTextSyntaxToken.GetHandle())].m_pfnCreate;
        ASSERT(pfnCreate != nullptr);
        IFC_RETURN(pfnCreate(depObj.ReleaseAndGetAddressOf(), &cp));

        IFC_RETURN(GetTypeTokenForDO(depObj.get(), sActualTypeToken));
        IFC_RETURN(XamlQualifiedObject::CreateNoAddRef(pCore, sActualTypeToken, depObj.detach(), qo));
    }

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
_Check_return_ HRESULT XamlNativeRuntime::CreateFromXmlText(
            _In_ const XamlTypeToken& inXamlType,
            _In_ const xstring_ptr& inText,
            _Out_ std::shared_ptr<XamlQualifiedObject>& qo )
{
    // You should be using CreateFromValue instead.
    IFC_RETURN(E_NOTIMPL);

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
_Check_return_ HRESULT XamlNativeRuntime::GetValue(
            _In_ const XamlQualifiedObject& qoObj,
            _In_ const XamlPropertyToken& inProperty,
            _Out_ XamlQualifiedObject& outValue )
{
    XamlTypeToken sActualTypeToken;

    outValue.Clear();
    auto pdp = MetadataAPI::GetDependencyPropertyByIndex(inProperty.GetHandle());
    auto pdo = qoObj.GetDependencyObject();

    // Only go through if we have a DO
    if (pdo)
    {
        CValue val;
        IFC_RETURN(pdo->GetValue(pdp, &val));

        if (!val.IsNull())
        {
            XamlQualifiedObject qo;
            IFC_RETURN(qo.AttachValue(XamlTypeToken(tpkNative, pdp->GetPropertyType()->m_nIndex), std::move(val)));

            if (qo.GetValue().GetType() == valueObject)
            {
                IFC_RETURN(GetTypeTokenForDO(qo.GetDependencyObject(), sActualTypeToken));
                qo.SetTypeToken(sActualTypeToken);
            }

            outValue = std::move(qo);
        }
    }

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
_Check_return_ HRESULT XamlNativeRuntime::GetAmbientValue(
            _In_ const XamlQualifiedObject& qoObj,
            _In_ const XamlPropertyToken& inProperty,
            _Out_ XamlQualifiedObject& outValue )
{
    outValue.Clear();

    const CDependencyProperty* pdp = MetadataAPI::GetDependencyPropertyByIndex(inProperty.GetHandle());
    CDependencyObject* pdo = qoObj.GetDependencyObject();

    // Only go through if we have a DO
    if (pdo != nullptr)
    {
        bool bAvailable = false;
        IFC_RETURN(pdo->IsAmbientPropertyValueAvailable(pdp, &bAvailable));
        if (bAvailable)
        {
            IFC_RETURN(GetValue(qoObj, inProperty, outValue));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Xxxx
//
//  Synopsis:
//      Xxx
//
//      NOTE: The XamlQualifiedObject you pass as inValue will be cleared
//      if the function succeeds. This doesn't require anything from the
//      caller, because the XQO it passed in owned the contents and will
//      release it when it goes out of scope in the case that the call
//      fails. The only important thing is not to try to use the
//      object after this call because ownership has been given away.
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlNativeRuntime::SetValue(
            _In_ const std::shared_ptr<XamlQualifiedObject>& inObj,
            _In_ const XamlPropertyToken& inProperty,
            _In_ const std::shared_ptr<XamlQualifiedObject>& inValue,
            _In_ bool bBindTemplates )
{
    HRESULT hr = S_OK;

    CDependencyObject *pdo = inObj->GetDependencyObject();
    KnownTypeIndex inValueTypeIndex = inValue->GetTypeToken().GetHandle();

    const auto property = MetadataAPI::GetPropertyBaseByIndex(inProperty.GetHandle());
    bool targetIsSimpleProperty = false;
    if (property->Is<CSimpleProperty>())
    {
        targetIsSimpleProperty = true;
    }

    // TODO: this is a bit of a hack since we don't have a generic way to handle expressions in native code. Better way to do this?
    switch (inValueTypeIndex)
    {
        case KnownTypeIndex::Binding:
            {
                if (targetIsSimpleProperty)
                {
                    std::shared_ptr<XamlProperty> spProperty;
                    std::shared_ptr<XamlSchemaContext> spSchemaContext;
                    IFC(GetSchemaContext(spSchemaContext));
                    IFC(spSchemaContext->GetXamlProperty(inProperty, spProperty));
                    xstring_ptr propertyName;
                    IFC(spProperty->get_FullName(&propertyName));

                    CCoreServices* pCore = nullptr;
                    IFC(GetCore(&pCore));

                    IFC(CErrorService::OriginateInvalidOperationError(
                        pCore,
                        AG_E_PARSER2_BINDING_NOT_ALLOWED_ON_PROPERTY,
                        propertyName));
                }
                else
                {
                    CBinding* pBinding = NULL;

                    IFC(DoPointerCast(pBinding, inValue->GetDependencyObject()));
                    IFC(pBinding->SetBinding(pdo, inProperty.GetHandle()));
                }
            }
            break;
        case KnownTypeIndex::TemplateBinding:
            {
                if (targetIsSimpleProperty)
                {
                    std::shared_ptr<XamlProperty> spProperty;
                    std::shared_ptr<XamlSchemaContext> spSchemaContext;
                    IFC(GetSchemaContext(spSchemaContext));
                    IFC(spSchemaContext->GetXamlProperty(inProperty, spProperty));
                    xstring_ptr propertyName;
                    IFC(spProperty->get_FullName(&propertyName));

                    CCoreServices* pCore = nullptr;
                    IFC(GetCore(&pCore));

                    IFC(CErrorService::OriginateInvalidOperationError(
                        pCore,
                        AG_E_PARSER2_TEMPLATEBINDING_NOT_ALLOWED_ON_PROPERTY,
                        propertyName));
                }
                else
                {
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
            }
            break;
        case KnownTypeIndex::ThemeResource:
            {
                const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(inProperty.GetHandle());
                CThemeResourceExtension *pThemeResourceExtension = nullptr;
                CThemeResource *pThemeResource = nullptr;
                if (inValue->GetValue().GetType() == valueObject)
                {
                    IFC(DoPointerCast(pThemeResourceExtension, inValue->GetDependencyObject()));
                    IFC(pThemeResourceExtension->SetThemeResourceBinding(pdo, pDP));
                }
                else
                {
                    ASSERT(inValue->GetValue().GetType() == valueThemeResource);
                    pThemeResource = inValue->GetValue().AsThemeResource();
                    IFC(pThemeResource->SetThemeResourceBinding(pdo, pDP));
                }
            }
            break;

        default:

            {
                ASSERT(inProperty.GetProviderKind() == tpkNative);

                CValue *pValue = nullptr;
                bool fShouldDelegateSetValue = false;

                IFC(GetAdjustedCValue(inValue, inProperty, &fShouldDelegateSetValue, &pValue));

                if (fShouldDelegateSetValue)
                {
                    IFC(DelegateSetValue(inObj, inProperty, inValue, bBindTemplates));
                }
                else
                {
                    if (targetIsSimpleProperty)
                    {
                        // Validate the strictness of this property - whether or not the API call is allowed based on strict mode.
                        // We do this validation here as this code path bypasses the public setter API which also validates.
                        // Note:  We do not need to perform strict validation for DP's here as this is done centrally in UpdateEffectiveValue.
                        IFC(pdo->ValidateStrictnessOnProperty(property));

                        IFC(Parser::XamlNativeRuntime_SetValueSimpleProperty(property, pdo, *pValue));
                    }
                    else
                    {
                        const auto dp = property->AsOrNull<CDependencyProperty>();
                        IFCEXPECT(dp);
                        CCoreServices *pCore = nullptr;

                        IFC(GetCore(&pCore));
                        IFC(CheckIsPropertyWriteable(pCore, dp));

                        HRESULT xr = S_OK;

                        {
                            // Normally would not check for IsAssignable before SetValue, but for native we
                            // do this for safety.

                            xr = CheckAssignableForSafety(dp, pValue);

                            if (SUCCEEDED(xr))
                            {
                                xr = pdo->SetValue(dp, *pValue);
                            }
                        }

                        // Try to wrap any failures with richer error information (note that
                        // we only do this after a failure to avoid the performance hit of
                        // looking up metadata on every set value).
                        if (FAILED(xr))
                        {
                            std::shared_ptr<XamlSchemaContext> spSchemaContext;
                            std::shared_ptr<XamlProperty> spProperty;

                            IFC(GetSchemaContext(spSchemaContext));
                            IFC(spSchemaContext->GetXamlProperty(XamlPropertyToken::FromProperty(dp), spProperty));

                            // Check to see whether the property was readonly
                            if (spProperty->IsReadOnly())
                            {
                                // Raise an error like "Cannot set read-only property '%0'."
                                xstring_ptr spPropertyName;
                                IFC(spProperty->get_FullName(&spPropertyName));
                                IFC(CErrorService::OriginateInvalidOperationError(
                                    pCore,
                                    AG_E_PARSER2_OW_READ_ONLY,
                                    spPropertyName));
                            }

                            // Check to see whether property was not assignable from the value.
                            {
                                std::shared_ptr<XamlType> spPropertyType;
                                XamlTypeToken tokValueType;
                                std::shared_ptr<XamlType> spValueType;

                                // Get the types of the property and value
                                IFC(spProperty->get_Type(spPropertyType));
                                tokValueType = inValue->GetTypeToken();
                                IFC(spSchemaContext->GetXamlType(tokValueType, spValueType));
                                if (spPropertyType && spValueType)
                                {
                                    bool bIsAssignable = false;
                                    IFC(spPropertyType->IsAssignableFrom(spValueType, bIsAssignable));
                                    if (!bIsAssignable)
                                    {
                                        // Report an error like "Failed to assign to
                                        // property '%0' because the type '%2' cannot be
                                        // assigned to the type '%1'."
                                        xstring_ptr spPropertyName;
                                        xstring_ptr spPropertyTypeName;
                                        xstring_ptr spValueTypeName;

                                        IFC(spProperty->get_FullName(&spPropertyName));
                                        IFC(spPropertyType->get_FullName(&spPropertyTypeName));
                                        IFC(spValueType->get_FullName(&spValueTypeName));

                                        IFC(CErrorService::OriginateInvalidOperationError(
                                            pCore,
                                            AG_E_PARSER2_OW_NOT_ASSIGNABLE_FROM,
                                            spPropertyName,
                                            spPropertyTypeName,
                                            spValueTypeName));
                                    }
                                }
                            }

                            // Otherwise just dump out the error code because it's
                            // indicating something else.
                            IFC(xr);
                        }
                    }
                }
            }
            break;
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
//      NOTE: The XamlQualifiedObject you pass as inValue will be cleared
//      if the function succeeds. This doesn't require anything from the
//      caller, because the XQO it passed in owned the contents and will
//      release it when it goes out of scope in the case that the call
//      fails. The only important thing is not to try to use the
//      object after this call because ownership has been given away.
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlNativeRuntime::Add(
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoCollection,
            _In_ const std::shared_ptr<XamlQualifiedObject>& inValue)
{
    CCollection *pCollection = NULL;
    XUINT32 nIndex;

    IFC_RETURN(DoPointerCast(pCollection, qoCollection->GetValue()));
    IFC_RETURN(pCollection->Append(inValue->GetValue(), &nIndex));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   XamlNativeRuntime::AddToDictionary
//
//  Synopsis:
//      Add a value to a ResourceDictionary.
//
//      NOTE: The XamlQualifiedObject you pass as inValue will be cleared
//      if the function succeeds. This doesn't require anything from the
//      caller, because the XQO it passed in owned the contents and will
//      release it when it goes out of scope in the case that the call
//      fails. The only important thing is not to try to use the
//      object after this call because ownership has been given away.
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlNativeRuntime::AddToDictionary(
            _In_ const std::shared_ptr<XamlQualifiedObject>& collection,
            _In_ const std::shared_ptr<XamlQualifiedObject>& value,
            _In_ const std::shared_ptr<XamlQualifiedObject>& key )
{
    CCoreServices *pCore = NULL;
    bool keyIsType = false;
    xstring_ptr ssKey;

    // Currently the only native dictionary we have is the ResourceDictionary.  If
    // we get a generic Dictionary that ResourceDictionary derives from, then we
    // would use that here.  Until then, CResourceDictionary it is.
    CResourceDictionary *pDictionary = NULL;

    IFC_RETURN(GetCore(&pCore));

    // Currently we only support strings as keys to native ResourceDictionary.
    IFC_RETURN(key->GetCopyAsString(&ssKey));

    if (ssKey.IsNull())
    {
        if (key->GetValue().GetType() == valueObject)
        {
            CType *pType = do_pointer_cast<CType>(key->GetDependencyObject());
            if (pType)
            {
                IFC_RETURN(pType->GetClassName(&ssKey));
                keyIsType = true;
            }
            else
            {
                std::shared_ptr<XamlSchemaContext> spSchemaContext;
                std::shared_ptr<XamlType> spKeyType;
                xstring_ptr spKeyTypeName;

                IFC_RETURN(GetSchemaContext(spSchemaContext));
                IFC_RETURN(spSchemaContext->GetXamlType(key->GetTypeToken(), spKeyType));
                IFC_RETURN(spKeyType->get_FullName(&spKeyTypeName));

                // Raise an error like "ResourceDictionary keys cannot be of
                // type '%0'. Only keys of type 'System.String' or of type
                // 'System.Type' are allowed."
                IFC_RETURN(CErrorService::OriginateInvalidOperationError(
                    pCore,
                    AG_E_PARSER2_OW_INVALID_DICTIONARY_KEY_TYPE,
                    spKeyTypeName));
            }
        }
        else if (key->GetValue().GetType() == valueTypeHandle)
        {
            ssKey = MetadataAPI::GetClassInfoByIndex(key->GetValue().AsTypeHandle())->GetFullName();
            keyIsType = true;
        }
        else
        {
            IFC_RETURN(CORE_E_INVALIDTYPE);
        }
    }

    IFC_RETURN(DoPointerCast(pDictionary, collection->GetValue()));
    IFC_RETURN(pDictionary->Add(ssKey, value->GetCValuePtr(), NULL, keyIsType));

    return S_OK;
}

_Check_return_ HRESULT XamlNativeRuntime::InitializationGuard(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance,
    _In_ bool fInitializing,
    _In_ const std::shared_ptr<XamlServiceProviderContext>&)
{
    CDependencyObject* pDependencyObject = qoInstance->GetDependencyObject();

    if (pDependencyObject)
    {
        if (fInitializing)
        {
            pDependencyObject->SetIsParsing(TRUE);
            pDependencyObject->SetParserParentLock();
        }
        else
        {
            IFC_RETURN(pDependencyObject->CreationComplete());
            pDependencyObject->ResetParserParentLock();
            pDependencyObject->SetIsParsing(FALSE);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlNativeRuntime::GetCore(_Outptr_ CCoreServices **ppCore)
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

_Check_return_ HRESULT XamlNativeRuntime::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_UNEXPECTED);
    }
    return S_OK;
}

_Check_return_ HRESULT XamlNativeRuntime::CallProvideValue(
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoMarkupExtension,
            _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _Out_ std::shared_ptr<XamlQualifiedObject>& qo )
{
    CMarkupExtensionBase *pMarkupExtension = NULL;

    IFC_RETURN(DoPointerCast(pMarkupExtension, qoMarkupExtension->GetValue()));

    IFC_RETURN(pMarkupExtension->ProvideValue(spServiceProviderContext, qo));

    // try to cache the markup extension so that we don't have to recreate it
    IFC_RETURN(CacheInstance(qoMarkupExtension));

    return S_OK;
}

_Check_return_ HRESULT XamlNativeRuntime::SetUriBase(
            _In_ const std::shared_ptr<XamlType>& spXamlType,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance,
            _In_ xref_ptr<IPALUri> spBaseUri )
{
    CDependencyObject* pDependencyObject = qoInstance->GetDependencyObject();

    IFCEXPECT_RETURN(pDependencyObject);

    if (pDependencyObject->GetCanParserOverwriteBaseUri())
    {
        pDependencyObject->SetBaseUri(static_cast<IPALUri*>(spBaseUri));
    }

    return S_OK;
}

// Since SetConnectionId can safely be an no-op
// it is ok to return S_OK and do nothing for Native runtime
_Check_return_ HRESULT XamlNativeRuntime::SetConnectionId(
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoComponentConnector,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoConnectionId,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoTarget)
{
    return S_OK;
}

_Check_return_ HRESULT XamlNativeRuntime::GetXBindConnector(
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoComponentConnector,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoConnectionId,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoTarget,
            _In_ const std::shared_ptr<XamlQualifiedObject>& qoReturnConnector)
{
    return S_OK;
}

// Get the XamlTypeToken for a dependency object.  Some entries in the type
// table use the same type for text syntax (specifically enumerations and things
// like booleans which are treated like enumerations on the native side), so
// this method it used to obtain the correct type token for a dependency object.
_Check_return_ HRESULT XamlNativeRuntime::GetTypeTokenForDO(
            _In_ CDependencyObject* pdo,
            _Out_ XamlTypeToken& ttTypeToken )
{
    KnownTypeIndex uiTypeIndex = KnownTypeIndex::UnknownType;

    // Bug 100475 : We don't have a repro for this crash case so IFCPTR should do for now...
    IFCPTR_RETURN(pdo);

    uiTypeIndex = pdo->GetTypeIndex();
    if (pdo->GetClassInformation()->IsEnum())
    {
        CEnumerated* pEnum = static_cast<CEnumerated*>(pdo);
        uiTypeIndex = pEnum->GetEnumTypeIndex();
    }

    ttTypeToken = XamlTypeToken(tpkNative, uiTypeIndex);

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CheckIsPropertyWriteable
//
//  Synopsis:
//
//  Make sure we're not setting a readonly property (unless the property
//  is DependencyProperty.Name - which we need to allow even though it's
//  technically readonly because x:Name directives won't be registered
//  unless they're in the visual tree).
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlNativeRuntime::CheckIsPropertyWriteable(
        _In_ CCoreServices *pCore,
        _In_ const CDependencyProperty *pdp)
{
    // **********
    // BUG 86071: We're special casing the properties on a number of
    // deployment types that are declared readonly, but the OOB folks need
    // them to be set by the parser.
    // **********
    if (pdp->IsReadOnly() &&
        pdp->GetIndex() != KnownPropertyIndex::DependencyObject_Name &&
        pdp->GetDeclaringType()->m_nIndex != KnownTypeIndex::Deployment)

    {
        // Report an error like "Cannot set read-only property '%0'."
        xstring_ptr spPropertyName;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlProperty> spProperty;

        IFC_RETURN(GetSchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetXamlProperty(XamlPropertyToken::FromProperty(pdp), spProperty));
        IFC_RETURN(spProperty->get_FullName(&spPropertyName));

        IFC_RETURN(CErrorService::OriginateInvalidOperationError(
            pCore,
            AG_E_PARSER2_OW_READ_ONLY,
            spPropertyName));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CheckAssignableForSafety
//
//  Synopsis:
//
//  Normally we would *not* do an IsAssignable check before doing a SetValue
//  (and indeed one is not needed for the XamlManagedRuntime), but the native
//  CDependencyObject::SetValue() doesn't have the same type-safety guarantees
//  as we would have with property setters.  This requires a bit of caution.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlNativeRuntime::CheckAssignableForSafety(
    _In_ const CDependencyProperty *pdp,
    _In_ CValue *pValue)
{

    if (!pdp->AllowsObjects())
    {
        CDependencyObject *pdo = pValue->AsObject();

        if (pdo)
        {
            KnownTypeIndex uiTypeIndex = pdo->GetTypeIndex();
            KnownTypeIndex uiPropertyTypeIndex = pdp->GetPropertyType()->m_nIndex;

            // special case for Enums which always return INDEX_ENUMERATED from GetTypeIndex()
            if (uiTypeIndex == KnownTypeIndex::Enumerated)
            {
                uiTypeIndex = static_cast<CEnumerated*>(pdo)->GetEnumTypeIndex();
            }

            if ( (uiTypeIndex != uiPropertyTypeIndex)
                && (uiTypeIndex != KnownTypeIndex::String)
                && !MetadataAPI::IsAssignableFrom(uiPropertyTypeIndex, uiTypeIndex))
            {
                IFC_RETURN(E_FAIL);
            }
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   GetAdjustedCValue
//
//  Synopsis:
//
//  If the value is a ManagedObjectReference (and we are clearly setting a native property),
//  then attempt to get the native value if there is one.  There are cases where calls to
//  managed code will try to wrap a non-do in a ManagedObjectReference regardless of whether
//  the value is a native value or not.
//
//  There are times when setting a native property on a native type may
//  require special handling from the value's type's XamlRuntime.
//  One case is BindingExpression (because native code has no concept of
//  Expression currently.  Another would be a managed delegate as an event-handler
//  for a natively defined event.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlNativeRuntime::GetAdjustedCValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& inValue,
    _In_ const XamlPropertyToken& inProperty,
    _Out_ bool *pfShouldDelegateCall,
    _Outptr_ CValue **ppValue)
{
    *ppValue = nullptr;
    *pfShouldDelegateCall = FALSE;

    CValue* pValue = inValue->GetCValuePtr();

    if (CManagedObjectReference* pMOR = do_pointer_cast<CManagedObjectReference>(*pValue))
    {
        // this ManagedObjectReference may have a native value
        // if so, get it.
        if (!pMOR->m_nativeValue.IsNull())
        {
            pValue = &pMOR->m_nativeValue;

            // if after unwrapping the ManagedObjectReference, we still don't have a native value,
            // then the native XamlRuntime may not be able to do anything with it.
            if (do_pointer_cast<CManagedObjectReference>(*pValue) != nullptr)
            {
                *pfShouldDelegateCall = TRUE;
            }
        }
        else
        {
            *pfShouldDelegateCall = TRUE;
        }
    }

    *ppValue = pValue;

    return S_OK; //RRETURN_REMOVAL
}



//------------------------------------------------------------------------
//
//  Method:   DelegateSetValue
//
//  Synopsis:
//
//  *   n.b.: if the call is delegated, and the other XamlRuntime returns a failing
//      HRESULT, the caller should return that HRESULT, but assume that the callee
//      has done whatever is necessary to set error information.
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlNativeRuntime::DelegateSetValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& inObj,
    _In_ const XamlPropertyToken& inProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& inValue,
    _In_ bool bBindTemplates
    )
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    // failing the IFC's for the following two lines should not be expected.
    IFC_RETURN(GetSchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetRuntime(tfkManaged, spXamlRuntime));


    IFC_RETURN(spXamlRuntime->SetValue(inObj, inProperty, inValue, bBindTemplates));

    return S_OK;
}


// Get a cached instance for shareable objects
_Check_return_ HRESULT XamlNativeRuntime::TryGetCachedInstance(
    _In_ const XamlTypeToken& inXamlTypeToken,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    switch(inXamlTypeToken.GetHandle())
    {
        case KnownTypeIndex::TemplateBinding:
            {
                if (m_spTemplateBindingExtensionXamlInstance)
                {
                    CMarkupExtensionBase *pMarkupExtension = NULL;

                    spInstance = m_spTemplateBindingExtensionXamlInstance;
                    IFC_RETURN(DoPointerCast(pMarkupExtension, spInstance->GetValue()));
                    pMarkupExtension->Reset();

                    // once the cached instance is returned to the caller, we don't want to
                    // reuse that instance if a different caller in a recursive call attempts
                    // to fetch it. If there is a recursive caller we will not return a cached
                    // extension object and allow it to recreate the extension object.
                    // The recursive call occurs when loading on demand keys or during templates
                    // expanding other templates.
                    m_spTemplateBindingExtensionXamlInstance.reset();
                }
            }
            break;

        case KnownTypeIndex::StaticResource:
            {
                if (m_spStaticResourceExtensionXamlInstance)
                {
                    CMarkupExtensionBase *pMarkupExtension = NULL;

                    spInstance = m_spStaticResourceExtensionXamlInstance;
                    IFC_RETURN(DoPointerCast(pMarkupExtension, spInstance->GetValue()));
                    pMarkupExtension->Reset();

                    m_spStaticResourceExtensionXamlInstance.reset();
                }
            }
            break;

        case KnownTypeIndex::ThemeResource:
            {
                if (m_spThemeResourceExtensionXamlInstance)
                {
                    CMarkupExtensionBase *pMarkupExtension = NULL;

                    spInstance = m_spThemeResourceExtensionXamlInstance;
                    IFC_RETURN(DoPointerCast(pMarkupExtension, spInstance->GetValue()));
                    pMarkupExtension->Reset();
                    // the parser always creates ThemeResource MarkupExtension objects with the Peg bit set.
                    // Since the object is recycled, the peg gets removed every time the extension is consumed.
                    spInstance->SetHasPeggedManagedPeer();
                    m_spThemeResourceExtensionXamlInstance.reset();
                }
            }
            break;

        default:
            break;
    }

    return S_OK;
}

// Get a cached instance for shareable objects
_Check_return_ HRESULT XamlNativeRuntime::CacheInstance(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    HRESULT hr = S_OK;
    XamlTypeToken xamlTypeToken;

    xamlTypeToken = spInstance->GetTypeToken();

    switch(xamlTypeToken.GetHandle())
    {
        case KnownTypeIndex::TemplateBinding:
            {
                m_spTemplateBindingExtensionXamlInstance = spInstance;
            }
            break;

        case KnownTypeIndex::StaticResource:
            {
                m_spStaticResourceExtensionXamlInstance = spInstance;
            }
            break;

        case KnownTypeIndex::ThemeResource:
            {
                m_spThemeResourceExtensionXamlInstance = spInstance;
            }
            break;
        default:
            break;
    }

    RRETURN(hr);
}

_Check_return_ HRESULT XamlNativeRuntime::AddDeferredProxy(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParent,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const XamlPropertyToken& inProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spProxy)
{
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
        inProperty.GetHandle(),
        proxy));

    return S_OK;
}
