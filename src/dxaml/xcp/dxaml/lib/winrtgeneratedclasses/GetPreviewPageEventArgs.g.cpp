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
#include "GetPreviewPageEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::GetPreviewPageEventArgs::GetPreviewPageEventArgs(): m_pageNumber()
{
}

DirectUI::GetPreviewPageEventArgs::~GetPreviewPageEventArgs()
{
}

HRESULT DirectUI::GetPreviewPageEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::GetPreviewPageEventArgs)))
    {
        *ppObject = static_cast<DirectUI::GetPreviewPageEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Printing::IGetPreviewPageEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Printing::IGetPreviewPageEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

CEventArgs* DirectUI::GetPreviewPageEventArgs::CreateCorePeer()
{
    return new CGetPreviewPageEventArgs();
}

// Properties.
IFACEMETHODIMP DirectUI::GetPreviewPageEventArgs::get_PageNumber(_Out_ INT* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_pageNumber, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::GetPreviewPageEventArgs::put_PageNumber(INT value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_pageNumber));
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateGetPreviewPageEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::GetPreviewPageEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_GetPreviewPageEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::ActivationFactory<DirectUI::GetPreviewPageEventArgs>>::CreateActivationFactory());
    }
}
