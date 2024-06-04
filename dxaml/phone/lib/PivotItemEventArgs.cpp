// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotItemEventArgs.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    //---------------------------------------------------------------------
    // PivotItemEventArgs
    //---------------------------------------------------------------------

    PivotItemEventArgs::PivotItemEventArgs()
    {
    }

    PivotItemEventArgs::~PivotItemEventArgs()
    {
    }

    HRESULT
    PivotItemEventArgs::RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IDependencyObjectFactory> spInnerFactory;
        wrl::ComPtr<xaml::IDependencyObject> spInnerInstance;
        wrl::ComPtr<IInspectable> spInnerInspectable;

        IFC(wf::GetActivationFactory(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DependencyObject).Get(),
              &spInnerFactory));

        IFC(spInnerFactory->CreateInstance(
               static_cast<IPivotItemEventArgs*>(this),
               &spInnerInspectable,
               &spInnerInstance));

        IFC(SetComposableBasePointers(
               spInnerInspectable.Get(),
               spInnerFactory.Get()));

    Cleanup:
        return hr;
    }

    IFACEMETHODIMP
    PivotItemEventArgs::get_Item(
        _Out_ xaml_controls::IPivotItem **value)
    {
        return m_wpPivotItem.CopyTo(value);
    }

    IFACEMETHODIMP
    PivotItemEventArgs::put_Item(
        _In_ xaml_controls::IPivotItem *value)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml_controls::IPivotItem> spItem(value);

        m_wpPivotItem = nullptr;
        if (nullptr != spItem)
        {
            IFC(spItem.AsWeak(&m_wpPivotItem));
        }

    Cleanup:
        return hr;
    }

} } } } XAML_ABI_NAMESPACE_END

