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
#include "WindowCreatedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::WindowCreatedEventArgs::WindowCreatedEventArgs()
{
}

DirectUI::WindowCreatedEventArgs::~WindowCreatedEventArgs()
{
}

HRESULT DirectUI::WindowCreatedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::WindowCreatedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::WindowCreatedEventArgs*>(this);
    }
#if WI_IS_FEATURE_PRESENT(Feature_UwpSupportApi)
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::IWindowCreatedEventArgs)) && Feature_UwpSupportApi::IsEnabled())
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::IWindowCreatedEventArgs*>(this);
    }
#endif
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::WindowCreatedEventArgs::get_Window(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IWindow** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_pWindow.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::WindowCreatedEventArgs::put_Window(_In_opt_ ABI::Microsoft::UI::Xaml::IWindow* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_pWindow, pValue);
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateWindowCreatedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::WindowCreatedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_WindowCreatedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
