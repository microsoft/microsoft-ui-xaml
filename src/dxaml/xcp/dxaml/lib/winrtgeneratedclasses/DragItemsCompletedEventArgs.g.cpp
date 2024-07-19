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
#include "DragItemsCompletedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::DragItemsCompletedEventArgs::DragItemsCompletedEventArgs(): m_dropResult()
{
}

DirectUI::DragItemsCompletedEventArgs::~DragItemsCompletedEventArgs()
{
}

HRESULT DirectUI::DragItemsCompletedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::DragItemsCompletedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::DragItemsCompletedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IDragItemsCompletedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IDragItemsCompletedEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::DragItemsCompletedEventArgs::get_Items(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVectorView<IInspectable*>** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_pItems.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::DragItemsCompletedEventArgs::put_Items(_In_opt_ ABI::Windows::Foundation::Collections::IVectorView<IInspectable*>* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_pItems, pValue);
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::DragItemsCompletedEventArgs::get_DropResult(_Out_ ABI::Windows::ApplicationModel::DataTransfer::DataPackageOperation* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_dropResult, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::DragItemsCompletedEventArgs::put_DropResult(_In_ ABI::Windows::ApplicationModel::DataTransfer::DataPackageOperation value)
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
    _Check_return_ HRESULT OnFrameworkCreateDragItemsCompletedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::DragItemsCompletedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_DragItemsCompletedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
