// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Type Activation API. Used to dynamically activate types at runtime.

#include "precomp.h"
#include "MetadataAPI.h"
#include "CustomClassInfo.h"
#include "CustomDependencyProperty.h"
#include "DependencyObject.h"
#include "UriXStringGetters.h"
#include "MediaPlayerElement_Partial.h"
#include <wininet.h>

using namespace DirectUI;

// Activates a type.
_Check_return_ HRESULT ActivationAPI::ActivateInstance(_In_ const CClassInfo* pType, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance)
{
    HRESULT hr = S_OK;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    CREATEPARAMETERS cp(pCore->GetHandle());
    CDependencyObject* pCoreObject = nullptr;
    ctl::ComPtr<DependencyObject> spDO;

    if (pType->IsBuiltinType())
    {
        // Create the core object first.
        const CREATEPFN pfnCreate = pType->GetCoreConstructor();
        IFC(pfnCreate(&pCoreObject, &cp));

        // And then create the framework object.
        IFC(pCore->GetPeer(pOuter, pCoreObject, &spDO));
        *ppInstance = ctl::iinspectable_cast(spDO.Detach());
    }
    else
    {
        // Delegate activation to the IXamlType.
        IFC(pType->AsCustomType()->GetXamlTypeNoRef()->ActivateInstance(ppInstance));
    }

Cleanup:
    ReleaseInterface(pCoreObject);
    RRETURN(hr);
}

_Check_return_ HRESULT ActivationAPI::ActivateInstance2(_In_ const CClassInfo* type, _Outptr_ IInspectable** instance)
{
    DXamlCore* pCore = DXamlCore::GetCurrent();
    CREATEPARAMETERS cp(pCore->GetHandle());

    if (type->IsBuiltinType())
    {
        // Create the core object first.
        xref_ptr<CDependencyObject> coreObj;
        const CREATEPFN pfnCreate = type->GetCoreConstructor();
        IFCPTR_RETURN(pfnCreate);

        IFC_RETURN(pfnCreate(coreObj.ReleaseAndGetAddressOf(), &cp));

        // And then create the framework object. Use UnboxObjectValue here because some types don't
        // have peers (like double/int/etc)
        CValue value;
        value.WrapObjectNoRef(coreObj.get());
        IFC_RETURN(CValueBoxer::UnboxObjectValue(&value, type, instance));
    }
    else
    {
        // Delegate activation to the IXamlType.
        IFC_RETURN(type->AsCustomType()->GetXamlTypeNoRef()->ActivateInstance(instance));
    }

    return S_OK;
}

_Check_return_ HRESULT ActivationAPI::ActivateCoreInstance(_In_ const CClassInfo* pType, _Outptr_ CDependencyObject** ppInstance)
{
    DXamlCore* pCore = DXamlCore::GetCurrent();
    CREATEPARAMETERS cp(pCore->GetHandle());

    *ppInstance = nullptr;

    if (pType->IsBuiltinType())
    {
        // Create the core object.
        const CREATEPFN pfnCreate = pType->GetCoreConstructor();
        if (pfnCreate)
        {
            CDependencyObject* pCDO = nullptr;
            IFC_RETURN(pfnCreate(&pCDO, &cp));

            *ppInstance = pCDO;
        }
        else
        {
            IFCFAILFAST(E_INVALIDARG);
        }
    }
    else
    {
        IFCFAILFAST(E_INVALIDARG);
    }

    return S_OK;
}

// Activates a type from a string.
_Check_return_ HRESULT ActivationAPI::ActivateInstanceFromString(_In_ const CClassInfo* pType, _In_ const xstring_ptr_view& strValue, _Outptr_ IInspectable** ppInstance)
{
    if (pType->IsBuiltinType())
    {
        // Speical case to support activate an instance of MediaPlayBackItem from Uri string set in the markup of MediaPlayerElement.Source property
        if (pType->GetIndex() == KnownTypeIndex::MediaPlaybackItemConverter)
        {
            IFC_RETURN(ActivateMediaPlaybackItemInstanceFromUri(strValue, ppInstance))
        }
        else
        {
            ASSERT(FALSE);
            IFC_RETURN(E_NOTIMPL);
        }
    }
    else
    {
        // We want to activate a custom type.  First, check whether
        // the type represents a boxed type and whether we should be
        // using the string conversion for the boxed type.  E.g.
        // If the type we were given is a Windows.Foundation.IReference`1<Boolean>,
        // we want to do our activation logic for Boolean, then wrap it.
        // However, for boxed custom types, like an enum, we'll trust
        // the original's type's CreateFromString method to do the conversion and boxing.
        if (pType->AsCustomType()->RepresentsBoxedType() && pType->AsCustomType()->GetBoxedType()->IsBuiltinType())
        {
            const CClassInfo* pBoxedType = pType->AsCustomType()->GetBoxedType();

            // For built-in types, get the core value, then box it
            DXamlCore* pCore = DXamlCore::GetCurrent();
            xstring_ptr xstrPromoted;

            IFC_RETURN(strValue.Promote(&xstrPromoted));
            CValue valParam;
            valParam.SetString(xstrPromoted);
            CREATEPARAMETERS cp(pCore->GetHandle(), valParam);

            // If we couldn't get the peer, just try unboxing the boxed type, which should
            // pack the value into an IReference.
            IFC_RETURN(CValueBoxer::UnboxObjectValue(&valParam, pBoxedType, ppInstance));
        }
        else
        {
            // If the type doesn't represent a boxed type, or represents a boxed type where the boxed type
            // is a custom user type, just call out to the XamlType's CreateFromString
            xruntime_string_ptr strValuePromoted;
            IFC_RETURN(strValue.Promote(&strValuePromoted));
            IFC_RETURN(pType->AsCustomType()->GetXamlTypeNoRef()->CreateFromString(strValuePromoted.GetHSTRING(), ppInstance));
        }
    }

    return S_OK;
}

// Activates a type for automation.
_Check_return_ HRESULT ActivationAPI::ActivateAutomationInstance(_In_ const CClassInfo* pType, _In_ CDependencyObject* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance)
{
    HRESULT hr = S_OK;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    CREATEPARAMETERS cp(pCore->GetHandle());
    CDependencyObject* pCoreObject = nullptr;
    ctl::ComPtr<DependencyObject> spDO;

    if (pOwner)
    {
        cp.m_value.WrapObjectNoRef(pOwner);
    }

    // Create the core object first.
    const CREATEPFN pfnCreate = pType->GetCoreConstructor();
    IFC(pfnCreate(&pCoreObject, &cp));

    // And then create the framework object.
    IFC(pCore->GetPeer(pOuter, pCoreObject, &spDO));
    *ppInstance = ctl::iinspectable_cast(spDO.Detach());

Cleanup:
    ReleaseInterface(pCoreObject);
    RRETURN(hr);
}

_Check_return_ HRESULT ActivationAPI::ActivateAutomationInstance(_In_ KnownTypeIndex eTypeIndex, _In_ CDependencyObject* pOwner, _In_ REFIID iid, _Outptr_ void** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInstance;
    IFC(ActivateAutomationInstance(MetadataAPI::GetClassInfoByIndex(eTypeIndex), pOwner, &spInstance));
    IFC(spInstance.Get()->QueryInterface(iid, ppInstance));
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ActivationAPI::ActivateMediaPlaybackItemInstanceFromUri(_In_ const xstring_ptr_view& uriString, _Outptr_ IInspectable** ppInstance)
{
    ctl::ComPtr<wmc::IMediaSourceStatics> spMediaSourceStatics;
    ctl::ComPtr<wf::IUriRuntimeClassFactory> spUriFactory;
    ctl::ComPtr<wf::IUriRuntimeClass> spUri;
    ctl::ComPtr<wf::IUriRuntimeClass> spProvidedUri;
    ctl::ComPtr<wmc::IMediaSource2> spMediaSource;
    ctl::ComPtr<wmp::IMediaPlaybackItemFactory> spItemFactory;

    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_Core_MediaSource).Get(),
        &spMediaSourceStatics));
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
        &spUriFactory));


    xruntime_string_ptr uriStrValuePromoted;
    IFC_RETURN(uriString.Promote(&uriStrValuePromoted));    
    IFC_RETURN(spUriFactory->CreateUri(uriStrValuePromoted.GetHSTRING(), &spProvidedUri));
    
    IFC_RETURN(MediaPlayerElement::ResolveLocalSourceUri(spProvidedUri.Get(), &spUri));
    if (!spUri)
    {
        spUri = spProvidedUri;
    }
    IFC_RETURN(spMediaSourceStatics->CreateFromUri(spUri.Get(), &spMediaSource));    

    IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_Playback_MediaPlaybackItem).Get(), &spItemFactory));
    IFC_RETURN(spItemFactory->Create(spMediaSource.Get(), reinterpret_cast<wmp::IMediaPlaybackItem **>(ppInstance)));
    
    return S_OK;
}