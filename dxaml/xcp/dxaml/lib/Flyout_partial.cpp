// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Flyout_partial.h"
#include "FlyoutPresenter.g.h"
#include "PropertyChangedParamsHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using xaml_primitives::FlyoutPlacementMode;

Flyout::Flyout()
{
}

Flyout::~Flyout()
{
}

_Check_return_ HRESULT Flyout::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOldValue;
    ctl::ComPtr<IInspectable> spNewValue;

    IFC(FlyoutGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Flyout_Content:
            {
                ctl::ComPtr<IContentControl> spPresenter = ctl::query_interface_cast<IContentControl>(GetPresenter());
                if (spPresenter)
                {
                    IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));

                    // Apply the Content property change with new value if the content is really changed.
                    bool areEqual = false;
                    IFC(PropertyValue::AreEqual(spOldValue.Get(), spNewValue.Get(), &areEqual));
                    if (!areEqual)
                    {
                        IFC(spPresenter->put_Content(spNewValue.Get()));
                    }
                }
                break;
            }

        case KnownPropertyIndex::Flyout_FlyoutPresenterStyle:
            {
                if (GetPresenter())
                {
                    IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));

                    // Apply the PresenterStyle property change with new value if the PresenterStyle is really changed.
                    bool areEqual = false;
                    IFC(PropertyValue::AreEqual(spOldValue.Get(), spNewValue.Get(), &areEqual));
                    if (!areEqual)
                    {
                        ctl::ComPtr<IStyle> spNewStyle;

                        IFC(spNewValue.As(&spNewStyle));
                        IFC(SetPresenterStyle(GetPresenter(), spNewStyle.Get()));
                    }
                }
                break;
            }

        default:
            break;
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP Flyout::CreatePresenter(
    _Outptr_ IControl** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<FlyoutPresenter> spPresenter;
    ctl::ComPtr<IUIElement> spContent;
    ctl::ComPtr<IStyle> spStyle;
    ctl::ComPtr<IFlyoutBase> spFlyoutBase;

    *ppReturnValue = NULL;

    IFC(ctl::make(&spPresenter));

    IFC(get_Content(&spContent));
    IFC(spPresenter->put_Content(spContent.Get()));

    IFC(get_FlyoutPresenterStyle(&spStyle));
    IFC(SetPresenterStyle(spPresenter.Get(), spStyle.Get()));

    IFC(spPresenter.MoveTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Flyout::OnProcessKeyboardAcceleratorsImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs)
{
    ctl::ComPtr<IUIElement> spContentInterface;
    IFC_RETURN(get_Content(&spContentInterface));
    if (spContentInterface.Get())
    {
        IFC_RETURN(spContentInterface.Cast<UIElement>()->TryInvokeKeyboardAccelerator(pArgs));
    }

    return S_OK;
}
