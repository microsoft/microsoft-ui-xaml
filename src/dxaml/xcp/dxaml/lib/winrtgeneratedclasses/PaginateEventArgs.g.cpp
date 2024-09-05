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
#include "PaginateEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::PaginateEventArgsGenerated::PaginateEventArgsGenerated()
{
}

DirectUI::PaginateEventArgsGenerated::~PaginateEventArgsGenerated()
{
}

HRESULT DirectUI::PaginateEventArgsGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PaginateEventArgs)))
    {
        *ppObject = static_cast<DirectUI::PaginateEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Printing::IPaginateEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Printing::IPaginateEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

CEventArgs* DirectUI::PaginateEventArgsGenerated::CreateCorePeer()
{
    return new CPaginateEventArgs();
}

// Properties.
IFACEMETHODIMP DirectUI::PaginateEventArgsGenerated::get_PrintTaskOptions(_Outptr_result_maybenull_ ABI::Windows::Graphics::Printing::IPrintTaskOptionsCore** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue={};
    IFC(CheckThread());
    IFC(static_cast<PaginateEventArgs*>(this)->get_PrintTaskOptionsImpl(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PaginateEventArgsGenerated::put_PrintTaskOptions(_In_opt_ ABI::Windows::Graphics::Printing::IPrintTaskOptionsCore* pValue)
{
    HRESULT hr = S_OK;
    
    IFC(CheckThread());
    IFC(static_cast<PaginateEventArgs*>(this)->put_PrintTaskOptionsImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PaginateEventArgsGenerated::get_CurrentPreviewPageNumber(_Out_ INT* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<PaginateEventArgs*>(this)->get_CurrentPreviewPageNumberImpl(pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PaginateEventArgsGenerated::put_CurrentPreviewPageNumber(_In_ INT value)
{
    HRESULT hr = S_OK;
    
    IFC(CheckThread());
    IFC(static_cast<PaginateEventArgs*>(this)->put_CurrentPreviewPageNumberImpl(value));
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreatePaginateEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::PaginateEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_PaginateEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::ActivationFactory<DirectUI::PaginateEventArgs>>::CreateActivationFactory());
    }
}