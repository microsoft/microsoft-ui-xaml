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
#include "DropCompletedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::DropCompletedEventArgs::DropCompletedEventArgs(): m_dropResult()
{
}

DirectUI::DropCompletedEventArgs::~DropCompletedEventArgs()
{
}

HRESULT DirectUI::DropCompletedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::DropCompletedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::DropCompletedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::IDropCompletedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::IDropCompletedEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::RoutedEventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::DropCompletedEventArgs::get_DropResult(_Out_ ABI::Windows::ApplicationModel::DataTransfer::DataPackageOperation* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_dropResult, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::DropCompletedEventArgs::put_DropResult(ABI::Windows::ApplicationModel::DataTransfer::DataPackageOperation value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_dropResult));
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateDropCompletedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::DropCompletedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_DropCompletedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
