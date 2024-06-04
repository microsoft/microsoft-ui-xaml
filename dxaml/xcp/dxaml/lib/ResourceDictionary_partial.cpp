// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ResourceDictionary_partial.h"
#include "CValueUtil.h"
#include "NullKeyedResource.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Constructors/destructors.
ResourceDictionary::ResourceDictionary()
{
}

IFACEMETHODIMP ResourceDictionary::Lookup(_In_opt_ IInspectable* key, _Outptr_ IInspectable **ppValue)
{
    RRETURN(LookupCore(key, true /*originateErrorOnLookupFailure */, ppValue));
}

_Check_return_  HRESULT
ResourceDictionary::LookupCore(
    _In_opt_ IInspectable* key,
    _In_ bool originateErrorOnLookupFailure,
    _Outptr_ IInspectable **ppValue)
{
    IFC_RETURN(CheckThread());

    IFCPTR_RETURN(ppValue);
    *ppValue = nullptr;

    if (key)
    {
        xstring_ptr strKey;
        BOOLEAN bIsImplicitStyle;
        BOOLEAN bIsImplicitDataTemplate;
        IFC_RETURN(TryGetKeyAsString(key, &bIsImplicitStyle, &bIsImplicitDataTemplate, &strKey));
        const bool bKeyIsAType = (bIsImplicitStyle || bIsImplicitDataTemplate);
        HRESULT hr = TryGetItemCore(strKey, bKeyIsAType, ppValue );

        if (FAILED(hr) && originateErrorOnLookupFailure)
        {
            IFC_RETURN(ErrorHelper::OriginateErrorUsingFormattedResourceID(hr, ERROR_RESOURCEDICTIONARY_RESOURCE_LOOKUP_FAILED, strKey));
        }
        IFC_NOTRACE_RETURN(hr);
    }

    return S_OK;
}

_Check_return_ HRESULT
ResourceDictionary::GetItem(
    _In_ const xstring_ptr_view& strKey,
    _In_ bool checkLocalOnly,
    _In_ bool isImplicitStyle,
    _Out_ CValue* pValue)
{
    CDependencyObject* pResolvedObject;
    auto resourceDictionary = static_cast<CResourceDictionary*>(GetHandle());

    const auto scope = checkLocalOnly ? Resources::LookupScope::LocalOnly : Resources::LookupScope::All;
    if (isImplicitStyle)
    {
        IFC_RETURN(resourceDictionary->GetImplicitStyleKeyNoRef(strKey, scope, &pResolvedObject));
    }
    else
    {
        IFC_RETURN(resourceDictionary->GetKeyWithOverrideNoRef(strKey, scope, &pResolvedObject));
    }


    // Use intermediate CValue to force implicit conversion from CInt32/etc. to raw values.
    CValue intermediateValue;
    if (pResolvedObject)
    {
        intermediateValue.SetAddRef<valueObject>(pResolvedObject);
    }
    else
    {
        intermediateValue.SetNull();
    }

    IFC_RETURN(pValue->CopyConverted(intermediateValue));

    return S_OK;
}

_Check_return_ HRESULT
ResourceDictionary::TryGetItemCore(
    const xstring_ptr_view& key,
    bool isImplicitStyle,
    _COM_Outptr_ IInspectable **item)
{
    *item = nullptr;
    const bool checkOnlyLocalDictionaries = false;
    CValue valueFromCore;

    IFC_RETURN(GetItem(key, checkOnlyLocalDictionaries, isImplicitStyle, &valueFromCore));
        // If we got the type of the value in the core, resolve the corresponding TypeInfo.
    KnownTypeIndex valueTypeIndex = CValueUtil::GetTypeIndex(valueFromCore);

    const CClassInfo* type = nullptr;

    if (valueTypeIndex != KnownTypeIndex::UnknownType)
    {
        type = MetadataAPI::GetClassInfoByIndex(valueTypeIndex);
    }

    if (valueFromCore.IsNull())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    // NullKeyedResource is a proxy type used to hold null-value
    if (valueTypeIndex == KnownTypeIndex::NullKeyedResource)
    {
        valueFromCore.SetNull();
    }

    IFC_RETURN(CValueBoxer::UnboxObjectValue(&valueFromCore, type, item));
    return S_OK;
}

IFACEMETHODIMP ResourceDictionary::HasKey(_In_opt_ IInspectable* key, _Out_ BOOLEAN *found)
{
    HRESULT hr = S_OK;
    IInspectable* pValue = NULL;

    IFCPTR(found);
    *found = FALSE;
    if (SUCCEEDED(LookupCore(key, false /* originateErrorOnLookupFailure */, &pValue)))
    {
        *found = TRUE;
    }

Cleanup:
    ReleaseInterface(pValue);
    RRETURN(hr);
}


IFACEMETHODIMP ResourceDictionary::Insert(_In_opt_ IInspectable* key, _In_opt_ IInspectable* value, _Out_ BOOLEAN *replaced)
{
    xstring_ptr strKey;
    BOOLEAN bIsImplicitStyle;
    BOOLEAN bIsImplicitDataTemplate;
    ctl::ComPtr<DependencyObject> valueAsDO;

    if (key)
    {
        IFC_RETURN(TryGetKeyAsString(key, &bIsImplicitStyle, &bIsImplicitDataTemplate, &strKey));
        bool bKeyIsAType = (bIsImplicitStyle || bIsImplicitDataTemplate);

        // TODO: Implement TryGetValueNoMergedDictionaries.
        if (value)
        {
            // Three possible cases for resource value -
            // 1. value is a DO, add to native resource dictionary as-is.
            // 2. value is a custom object, say Customer. Wrap up in a ExternalObjectReference.
            // 3. value is a a valuetype, in which case it is already inside IPropertyValue. Wrap IPropertyValue in ExternalObjectReference.
            BOOLEAN wasWrapped;
            IFC_RETURN(ExternalObjectReference::ConditionalWrap(value, valueAsDO.ReleaseAndGetAddressOf(), &wasWrapped));

            if (wasWrapped)
            {
                // Propagate this value to the core since it might be needed there (e.g. for resource lookup of a non-DO like Color).
                IFC_RETURN(static_cast<ExternalObjectReference*>(valueAsDO.Get())->put_NativeValue(value));
            }
        }
        else
        {
            valueAsDO = DXamlCore::GetCurrent()->GetCachedNullKeyedResource();
        }

        // Removed the existing value for the key, if present.
        auto xr = static_cast<CResourceDictionary*>(GetHandle())->Remove(strKey, bKeyIsAType);
        IFC_RETURN(xr);

        BOOLEAN valueRemoved = (xr == S_FALSE) ? FALSE : TRUE;

        CValue valueAsCValue;
        valueAsCValue.Wrap<valueObject>(valueAsDO->GetHandle());

        IFC_RETURN(static_cast<CResourceDictionary*>(GetHandle())->Add(strKey, &valueAsCValue, nullptr, bKeyIsAType));

        *replaced = valueRemoved;
    }

    return S_OK;
}


IFACEMETHODIMP ResourceDictionary::Remove(_In_opt_ IInspectable* key)
{
    xstring_ptr strKey;
    BOOLEAN bIsImplicitStyle;
    BOOLEAN bIsImplicitDataTemplate;
    bool bKeyIsAType;

    if (key)
    {
        IFC_RETURN(TryGetKeyAsString(key, &bIsImplicitStyle, &bIsImplicitDataTemplate, &strKey));
        bKeyIsAType = (bIsImplicitStyle || bIsImplicitDataTemplate);

        IFC_RETURN(static_cast<CResourceDictionary*>(GetHandle())->Remove(strKey, bKeyIsAType));
    }

    return S_OK;
}

IFACEMETHODIMP ResourceDictionary::GetView(
    _Outptr_result_maybenull_ wfc::IMapView<IInspectable*, IInspectable*> **pView)
{
    HRESULT hr = S_OK;
    IFCPTR(pView);
    *pView = NULL;

    IFC((ctl::do_query_interface<wfc::IMapView<IInspectable*, IInspectable*>>(*pView, this)));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ResourceDictionary::First(
    _Outptr_result_maybenull_ wfc::IIterator<wfc::IKeyValuePair<IInspectable*, IInspectable*>*> **pFirst)
{
    HRESULT hr = S_OK;
    Iterator<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>* pIterator = NULL;
    wfc::IVectorView<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>* pView = NULL;
    IFCPTR(pFirst);
    *pFirst = NULL;

    IFC(GetView(&pView));
    IFC((ctl::ComObject<Iterator<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>>::CreateInstance(&pIterator)));
    if (pIterator && pView)
    {
        IFC(pIterator->SetView(pView));
        *pFirst = pIterator;
        pIterator = NULL;
    }

Cleanup:
    ReleaseInterface(pView);
    ctl::release_interface(pIterator);
    RRETURN(hr);
}

IFACEMETHODIMP ResourceDictionary::GetAt(_In_opt_ UINT index, _Out_ wfc::IKeyValuePair<IInspectable*, IInspectable*>** pItem)
{
    HRESULT hr = S_OK;
    ResourceKeyValuePair* pKeyValuePair = NULL;
    IInspectable* pKey = NULL;
    IInspectable* pValue = NULL;
    IFCPTR(pItem);
    *pItem = NULL;

    IFC(GetKeyAt(index, &pKey));
    IFC(Lookup(pKey, &pValue));
    IFC(ctl::ComObject<ResourceKeyValuePair>::CreateInstance(&pKeyValuePair));
    pKeyValuePair->put_KeyValuePair(pKey, pValue);

    *pItem = pKeyValuePair;
    pKeyValuePair = NULL;

Cleanup:
    ReleaseInterface(pKey);
    ReleaseInterface(pValue);
    ctl::release_interface(pKeyValuePair);
    RRETURN(hr);
}

// Code coverage note: this is purely for supporting an interface that is exposed via
// QueryInterface (IVector), but is not officially part of the public API. While this makes it
// effectively dead code, we can't actually remove it because it's still reachable via
// QueryInterface (and a bit of elbow grease)
IFACEMETHODIMP ResourceDictionary::GetView(
    _Outptr_result_maybenull_ wfc::IVectorView<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>** pView)
{
    HRESULT hr = S_OK;
    IFCPTR(pView);
    *pView = NULL;

    IFC((ctl::do_query_interface<wfc::IVectorView<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>>(*pView, this)));

Cleanup:
    RRETURN(hr);
}

// Code coverage note: this is purely for supporting an interface that is exposed via
// QueryInterface (IVector), but is not officially part of the public API. While this makes it
// effectively dead code, we can't actually remove it because it's still reachable via
// QueryInterface (and a bit of elbow grease)
IFACEMETHODIMP ResourceDictionary::IndexOf(
    _In_opt_ wfc::IKeyValuePair<IInspectable*, IInspectable*>* pKeyValuePair,
    _Out_ UINT *pIndex,
    _Out_ BOOLEAN *pbFound)
{
    HRESULT hr = S_OK;
    IInspectable* pKey = NULL;
    IInspectable* pCurrentKey = NULL;
    wfc::IKeyValuePair<IInspectable*, IInspectable*>* pCurrentItem = NULL;
    wfc::IIterator<wfc::IKeyValuePair<IInspectable*, IInspectable*>*> *pIterator = NULL;
    BOOLEAN bHasCurrent = FALSE;
    bool areEqual = false;
    UINT iCurrent = 0;

    IFCPTR(pKeyValuePair);
    IFCPTR(pIndex);
    IFCPTR(pbFound);
    *pbFound = FALSE;

    IFC(pKeyValuePair->get_Key(&pKey));
    IFC(First(&pIterator));

    if (!pIterator)
    {
        goto Cleanup;
    }

    IFC(pIterator->get_Current(&pCurrentItem));
    IFCPTR(pCurrentItem);
    IFC(pCurrentItem->get_Key(&pCurrentKey));
    IFC(pIterator->get_HasCurrent(&bHasCurrent));

    while(bHasCurrent && pCurrentKey)
    {
        IFC(PropertyValue::AreEqual(pKey, pCurrentKey, &areEqual));
        if (areEqual)
        {
            *pIndex = iCurrent;
            goto Cleanup;
        }

        IFC(pIterator->MoveNext(&bHasCurrent));
        IFC(pIterator->get_Current(&pCurrentItem));
        IFCPTR(pCurrentItem);
        IFC(pCurrentItem->get_Key(&pCurrentKey));
        iCurrent++;
    }

Cleanup:
    ReleaseInterface(pIterator);
    ReleaseInterface(pCurrentKey);
    ReleaseInterface(pCurrentItem);
    ReleaseInterface(pKey);
    RRETURN(hr);
}

// Code coverage note: this is purely for supporting an interface that is exposed via
// QueryInterface (IVector), but is not officially part of the public API. While this makes it
// effectively dead code, we can't actually remove it because it's still reachable via
// QueryInterface (and a bit of elbow grease)
IFACEMETHODIMP ResourceDictionary::RemoveAt(
    _In_ UINT index)
{
    HRESULT hr = S_OK;
    IInspectable* pKey = NULL;
    wfc::IKeyValuePair<IInspectable*, IInspectable*>* pItem = NULL;

    UINT nCount = 0;
    IFC(get_Size(&nCount));
    IFCEXPECTRC(index < nCount, E_BOUNDS);

    IFC(GetAt(index, &pItem));
    if (pItem)
    {
        IFC(pItem->get_Key(&pKey));
        IFC(Remove(pKey));
    }

Cleanup:
    ReleaseInterface(pKey);
    ReleaseInterface(pItem);
    RRETURN(hr);
}

// Code coverage note: this is purely for supporting an interface that is exposed via
// QueryInterface (IVector), but is not officially part of the public API. While this makes it
// effectively dead code, we can't actually remove it because it's still reachable via
// QueryInterface (and a bit of elbow grease)
IFACEMETHODIMP ResourceDictionary::Append(
    _In_opt_ wfc::IKeyValuePair<IInspectable*, IInspectable*>* pItem)
{
    HRESULT hr = S_OK;
    IInspectable* pKey = NULL;
    IInspectable* pValue = NULL;
    BOOLEAN bReplaced = FALSE;

    if (pItem)
    {
        IFC(pItem->get_Key(&pKey));
        IFC(pItem->get_Value(&pValue));
        IFC(Insert(pKey, pValue, &bReplaced));
    }

Cleanup:
    ReleaseInterface(pKey);
    ReleaseInterface(pValue);
    RRETURN(hr);
}

// Code coverage note: this is purely for supporting an interface that is exposed via
// QueryInterface (IVector), but is not officially part of the public API. While this makes it
// effectively dead code, we can't actually remove it because it's still reachable via
// QueryInterface (and a bit of elbow grease)
IFACEMETHODIMP ResourceDictionary::RemoveAtEnd()
{
    HRESULT hr = S_OK;
    UINT nCount = 0;

    IFC(get_Size(&nCount));
    if (nCount > 0)
    {
        IFC(RemoveAt(nCount-1));
    }

Cleanup:
    RRETURN(hr);
}

#pragma warning (push)
#pragma warning (disable : 28301)
// difference between STDMETHODIMP and IFACEMETHODIMP comes from windows SDK
// somehow how of the thousands of use of STDMETHODIMP only this one complain

IFACEMETHODIMP ResourceDictionary::Clear()
{
    IFC_RETURN(static_cast<CResourceDictionary*>(GetHandle())->Clear());

    return S_OK;
}

#pragma warning(pop)

IFACEMETHODIMP ResourceDictionary::get_Size(_Out_ UINT *size)
{
    auto resourceDictionary = static_cast<CResourceDictionary*>(GetHandle());
    IFC_RETURN(resourceDictionary->EnsureAll());
    *size = resourceDictionary->GetCount();

    return S_OK;
}


_Check_return_ HRESULT
ResourceDictionary::TryGetKeyAsString(
    _In_ IInspectable* pKey,
    _Out_ BOOLEAN* pbIsImplicitStyle,
    _Out_ BOOLEAN* pbIsImplicitDataTemplate,
    _Out_ xstring_ptr* pstrKey)
{
    HRESULT hr = S_OK;
    HSTRING keyAsHString = nullptr;
    wf::IPropertyValue* pPropertyValue = NULL;
    wf::IReference<wxaml_interop::TypeName>* pTypeNameRef = NULL;
    IDataTemplateKey* pDataTemplateKey = NULL;
    IInspectable* pDataTemplateType = NULL;
    wxaml_interop::TypeName typeName = {};
    const CClassInfo* pType = nullptr;

    IFCPTR(pKey);
    IFCPTR(pbIsImplicitStyle);
    IFCPTR(pbIsImplicitDataTemplate);

    *pbIsImplicitStyle = FALSE;
    *pbIsImplicitDataTemplate = FALSE;

    pTypeNameRef = ctl::query_interface<wf::IReference<wxaml_interop::TypeName>>(pKey);
    if (pTypeNameRef != NULL)
    {
        IFC(pTypeNameRef->get_Value(&typeName));

        IFC(MetadataAPI::GetClassInfoByTypeName(typeName, &pType));

        *pstrKey = pType->GetFullName();
        *pbIsImplicitStyle = TRUE;
    }
    else
    {
        pDataTemplateKey = ctl::query_interface<IDataTemplateKey>(pKey);
        if (pDataTemplateKey != NULL)
        {
            IFC(pDataTemplateKey->get_DataType(&pDataTemplateType));
            IFCPTR(pDataTemplateType);

            IFC(ctl::do_query_interface(pTypeNameRef, pDataTemplateType));
            IFC(pTypeNameRef->get_Value(&typeName));

            IFC(xstring_ptr::CloneRuntimeStringHandle(typeName.Name, pstrKey));
            *pbIsImplicitDataTemplate = TRUE;
        }
        else
        {
            IFC(ctl::do_get_value(keyAsHString, pKey));
            IFC(xstring_ptr::CloneRuntimeStringHandle(keyAsHString, pstrKey));
        }
    }

    if (pstrKey->IsNullOrEmpty())
    {
        IFC(ErrorHelper::OriginateError(AgError(AG_E_PARSER_RESOURCEDICTIONARY_KEY_INVALIDARG)));
        IFC(E_INVALIDARG);
    }

Cleanup:
    DELETE_STRING(typeName.Name);
    DELETE_STRING(keyAsHString);
    ReleaseInterface(pPropertyValue);
    ReleaseInterface(pTypeNameRef);
    ReleaseInterface(pDataTemplateKey);
    ReleaseInterface(pDataTemplateType);
    RRETURN(hr);
}

_Check_return_ HRESULT ResourceDictionary::GetKeyAt(
    _In_ UINT index,
    _Out_ IInspectable** pKey)
{
    CValue keyFromCore;
    bool keyIsImplicitStyle = false;
    bool keyIsType = false;

    *pKey = nullptr;

    IFC_RETURN(static_cast<CResourceDictionary*>(GetHandle())->GetKeyAtIndex(
        index,
        &keyFromCore,
        &keyIsImplicitStyle,
        &keyIsType));

    IFC_RETURN(GetStrKeyAsInspectable(keyFromCore.AsString(), keyIsType, pKey));

    return S_OK;
}

_Check_return_ HRESULT
ResourceDictionary::OnParentUpdated(
    _In_opt_ CDependencyObject* pOldParentCore,
    _In_opt_ CDependencyObject* pNewParentCore,
    _In_ bool isNewParentAlive)
{
    HRESULT hr = S_OK;

    if (pNewParentCore != NULL)
    {
       // Peg the ResourceDictionary as a Root
       // so that we can keep dependent CCWs protected.
       // NOTE: This is a special case because the App Dxaml DO is not in the
       // peer table map and hence doesn't make the associated RD reachable.
       if (pNewParentCore == DXamlCore::GetCurrent()->GetCoreAppHandle())
       {
           UpdatePeg(true);
       }
    }

    IFC(ResourceDictionaryGenerated::OnParentUpdated(pOldParentCore, pNewParentCore, isNewParentAlive));

    if (pOldParentCore != NULL)
    {
       // Unpeg the ResourceDictionary as a Root
       if (pOldParentCore == DXamlCore::GetCurrent()->GetCoreAppHandle())
       {
           UpdatePeg(false);
       }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ResourceDictionary::GetStrKeyAsInspectable(
    _In_ const xstring_ptr_view& strKey,
    _In_ bool keyIsType,
    _Outptr_ IInspectable** pKey)
{
    if (keyIsType)
    {
        wxaml_interop::TypeName typeName = {};
        auto nameCleanup = wil::scope_exit([&typeName] { ::WindowsDeleteString(typeName.Name); });

        IFC_RETURN(MetadataAPI::GetTypeNameByFullName(strKey, &typeName));
        IFC_RETURN(PropertyValue::CreateReference<wxaml_interop::TypeName>(typeName, pKey));
    }
    else
    {
        xruntime_string_ptr strRuntimeKey;

        IFC_RETURN(strKey.Promote(&strRuntimeKey));
        IFC_RETURN(DirectUI::PropertyValue::CreateFromString(strRuntimeKey.GetHSTRING(), pKey));
    }

    return S_OK;
}
