// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "precomp.h"
#include "LoadedImageSourceLoadCompletedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::LoadedImageSourceLoadCompletedEventArgs::LoadedImageSourceLoadCompletedEventArgs()
{
}

DirectUI::LoadedImageSourceLoadCompletedEventArgs::~LoadedImageSourceLoadCompletedEventArgs()
{
}

HRESULT DirectUI::LoadedImageSourceLoadCompletedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::LoadedImageSourceLoadCompletedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::LoadedImageSourceLoadCompletedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::ILoadedImageSourceLoadCompletedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::ILoadedImageSourceLoadCompletedEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::LoadedImageSourceLoadCompletedEventArgs::get_Status(_Out_ ABI::Microsoft::UI::Xaml::Media::LoadedImageSourceLoadStatus* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DirectUI::LoadedImageSourceLoadStatus valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CLoadedImageSourceLoadCompletedEventArgs*>(pCoreEventArgsNoRef)->get_Status(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::LoadedImageSourceLoadCompletedEventArgs::put_Status(ABI::Microsoft::UI::Xaml::Media::LoadedImageSourceLoadStatus value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DirectUI::LoadedImageSourceLoadStatus valueCore = static_cast<DirectUI::LoadedImageSourceLoadStatus>(value);

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CLoadedImageSourceLoadCompletedEventArgs*>(pCoreEventArgsNoRef)->put_Status(valueCore));


Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateLoadedImageSourceLoadCompletedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::LoadedImageSourceLoadCompletedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_LoadedImageSourceLoadCompletedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
