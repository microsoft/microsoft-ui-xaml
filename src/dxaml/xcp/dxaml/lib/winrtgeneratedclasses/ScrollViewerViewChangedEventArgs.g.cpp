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
#include "ScrollViewerViewChangedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::ScrollViewerViewChangedEventArgs::ScrollViewerViewChangedEventArgs(): m_isIntermediate()
{
}

DirectUI::ScrollViewerViewChangedEventArgs::~ScrollViewerViewChangedEventArgs()
{
}

HRESULT DirectUI::ScrollViewerViewChangedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::ScrollViewerViewChangedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::ScrollViewerViewChangedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IScrollViewerViewChangedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IScrollViewerViewChangedEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::ScrollViewerViewChangedEventArgs::get_IsIntermediate(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_isIntermediate, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::ScrollViewerViewChangedEventArgs::put_IsIntermediate(BOOLEAN value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_isIntermediate));
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateScrollViewerViewChangedEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::ScrollViewerViewChangedEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_ScrollViewerViewChangedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::ActivationFactory<DirectUI::ScrollViewerViewChangedEventArgs>>::CreateActivationFactory());
    }
}
