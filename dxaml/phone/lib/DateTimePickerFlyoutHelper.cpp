// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

_Check_return_ HRESULT
DateTimePickerFlyoutHelper::CalculatePlacementPosition(
    _In_ IFrameworkElement* pTargetElement,
    _In_ IControl* pFlyoutPresenter,
    _Out_ wf::Point* pPoint)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IFrameworkElement> spTargetAsFE(pTargetElement);
    wrl::ComPtr<xaml_controls::IControl> spFlyoutPresenterAsControl(pFlyoutPresenter);
    wrl::ComPtr<xaml::IUIElement> spTargetAsUIE;
    wrl::ComPtr<xaml::IUIElement> spFlyoutPresenterAsUIE;
    wrl::ComPtr<xaml_controls::IControlProtected> spFlyoutPresenterAsControlProtected;
    wrl::ComPtr<xaml::IDependencyObject> spHighlightRectangleAsDo;
    wrl::ComPtr<xaml::IUIElement> spHighlightRectangleAsUIE;
    wrl::ComPtr<xaml::IFrameworkElement> spHighlightRectangleAsFE;
    wrl::ComPtr<xaml::Media::IGeneralTransform> spTransformFromPresenterToHighlight;
    wrl::ComPtr<xaml::Media::IGeneralTransform> spTransformFromTargetToWindow;
    DOUBLE width = 0.0;
    DOUBLE height = 0.0;
    wf::Point targetPoint = { 0, 0 };
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;

    IFCPTR(pTargetElement);
    IFCPTR(pFlyoutPresenter);
    IFCPTR(pPoint);

    IFC(spTargetAsFE.As(&spTargetAsUIE));
    IFC(spFlyoutPresenterAsControl.As(&spFlyoutPresenterAsUIE));
    IFC(spFlyoutPresenterAsControl.As(&spFlyoutPresenterAsControlProtected));

    //Calculate the correct targetPoint to show the DatePickerFlyout.
    //We want to choose a point such that the HighlightRect template part is centered over the target element.

    IFC(spFlyoutPresenterAsControlProtected->GetTemplateChild(
        wrl_wrappers::HStringReference(L"HighlightRect").Get(),
        &spHighlightRectangleAsDo));
    if (spHighlightRectangleAsDo)
    {
        IGNOREHR(spHighlightRectangleAsDo.As(&spHighlightRectangleAsUIE));
        IGNOREHR(spHighlightRectangleAsDo.As(&spHighlightRectangleAsFE));
    }

    if (spHighlightRectangleAsUIE)
    {
        IFC(spFlyoutPresenterAsUIE->TransformToVisual(spHighlightRectangleAsUIE.Get(), &spTransformFromPresenterToHighlight));
        IFC(spTransformFromPresenterToHighlight->TransformPoint(targetPoint, &targetPoint));
        IFC(spHighlightRectangleAsFE->get_ActualWidth(&width));
        IFC(spHighlightRectangleAsFE->get_ActualHeight(&height));
        targetPoint.X -= static_cast<float>(width / 2);
        targetPoint.Y -= static_cast<float>(height / 2);
    }

    IFC(spTargetAsUIE->TransformToVisual(nullptr, &spTransformFromTargetToWindow));
    IFC(spTransformFromTargetToWindow->TransformPoint(targetPoint, &targetPoint));

    IFC(spTargetAsFE->get_ActualWidth(&width));
    IFC(spTargetAsFE->get_ActualHeight(&height));

    IFC(spTargetAsFE.Get()->get_FlowDirection(&flowDirection));
    targetPoint.X = (flowDirection == FlowDirection_LeftToRight) ?
        targetPoint.X + static_cast<float>(width / 2) :
        targetPoint.X - static_cast<float>(width / 2);
    targetPoint.Y += static_cast<float>(height / 2);

    *pPoint = targetPoint;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DateTimePickerFlyoutHelper::ShouldInvertKeyDirection(_In_ IFrameworkElement* contentPanel, _Out_ BOOLEAN* invert)
{
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    if (contentPanel)
    {
        IFC_RETURN(contentPanel->get_FlowDirection(&flowDirection));
    }
    *invert = flowDirection == FlowDirection_RightToLeft;
    return S_OK;
}

bool DateTimePickerFlyoutHelper::ShouldFirstToThirdDirection(wsy::VirtualKey key, BOOLEAN invert)
{
    return (key == wsy::VirtualKey_Left && !invert) || (key == wsy::VirtualKey_Right && invert);
}

bool DateTimePickerFlyoutHelper::ShouldThirdToFirstDirection(wsy::VirtualKey key, BOOLEAN invert)
{
    return (key == wsy::VirtualKey_Left && invert) || (key == wsy::VirtualKey_Right && !invert);
}

_Check_return_ HRESULT DateTimePickerFlyoutHelper::OnKeyDownImpl(
    _In_ xaml_input::IKeyRoutedEventArgs* pEventArgs,
    _In_ xaml_controls::IControl* tpFirstPickerAsControl,
    _In_ xaml_controls::IControl* tpSecondPickerAsControl,
    _In_ xaml_controls::IControl* tpThirdPickerAsControl,
    _In_ IFrameworkElement* tpContentPanel)
{
    BOOLEAN handled = FALSE;
    wsy::VirtualKey key = wsy::VirtualKey_None;

    IFC_RETURN(pEventArgs->get_Handled(&handled));
    if (handled)
    {
        return S_OK;
    }

    IFC_RETURN(pEventArgs->get_Key(&key));

    if (key == wsy::VirtualKey_Left || key == wsy::VirtualKey_Right)
    {
        xaml::FocusState firstPickerFocusState = xaml::FocusState_Unfocused;
        xaml::FocusState secondPickerFocusState = xaml::FocusState_Unfocused;
        xaml::FocusState thirdPickerFocusState = xaml::FocusState_Unfocused;
        wrl::ComPtr<xaml::IUIElement> tpFirstPickerAsElement;
        wrl::ComPtr<xaml::IUIElement> tpSecondPickerAsElement;
        wrl::ComPtr<xaml::IUIElement> tpThirdPickerAsElement;
        BOOLEAN focusChanged = false;

        if (tpFirstPickerAsControl)
        {
            IFC_RETURN(tpFirstPickerAsControl->QueryInterface(IID_PPV_ARGS(&tpFirstPickerAsElement)));
            IFC_RETURN(tpFirstPickerAsElement->get_FocusState(&firstPickerFocusState));
        }

        if (tpSecondPickerAsControl)
        {
            IFC_RETURN(tpSecondPickerAsControl->QueryInterface(IID_PPV_ARGS(&tpSecondPickerAsElement)));
            IFC_RETURN(tpSecondPickerAsElement->get_FocusState(&secondPickerFocusState));
        }

        if (tpThirdPickerAsControl)
        {
            IFC_RETURN(tpThirdPickerAsControl->QueryInterface(IID_PPV_ARGS(&tpThirdPickerAsElement)));
            IFC_RETURN(tpThirdPickerAsElement->get_FocusState(&thirdPickerFocusState));
        }

        // In RTL, Grid 0 is on the right side. so visual effect from left to right is thirdPicker-secondPicker-firstPicker.
        // So we need to invert the key direction before it's handled: if left key is pressed, it's FirstToThirdDirection;
        // if right key is pressed, it's ThirdToFirstDirection

        BOOLEAN invert = false;
        IFC_RETURN(DateTimePickerFlyoutHelper::ShouldInvertKeyDirection(tpContentPanel, &invert));

        bool shouldFirstToThirdDirection = DateTimePickerFlyoutHelper::ShouldFirstToThirdDirection(key, invert);
        bool shouldThirdToFirstDirection = DateTimePickerFlyoutHelper::ShouldThirdToFirstDirection(key, invert);

        if (shouldFirstToThirdDirection)
        {
            if (secondPickerFocusState != xaml::FocusState_Unfocused)
            {
                if (tpFirstPickerAsElement)
                {
                    IFC_RETURN(tpFirstPickerAsElement->Focus(xaml::FocusState_Keyboard, &focusChanged));
                }
            }
            else if (thirdPickerFocusState != xaml::FocusState_Unfocused)
            {
                if (tpSecondPickerAsElement)
                {
                    IFC_RETURN(tpSecondPickerAsElement->Focus(xaml::FocusState_Keyboard, &focusChanged));
                }
                else if (tpFirstPickerAsElement)
                {
                    IFC_RETURN(tpFirstPickerAsElement->Focus(xaml::FocusState_Keyboard, &focusChanged));
                }
            }
        }
        else if (shouldThirdToFirstDirection)
        {
            if (firstPickerFocusState != xaml::FocusState_Unfocused)
            {
                if (tpSecondPickerAsElement)
                {
                    IFC_RETURN(tpSecondPickerAsElement->Focus(xaml::FocusState_Keyboard, &focusChanged));
                }
                else if (tpThirdPickerAsElement)
                {
                    IFC_RETURN(tpThirdPickerAsElement->Focus(xaml::FocusState_Keyboard, &focusChanged));
                }
            }
            else if (secondPickerFocusState != xaml::FocusState_Unfocused)
            {
                if (tpThirdPickerAsElement)
                {
                    IFC_RETURN(tpThirdPickerAsElement->Focus(xaml::FocusState_Keyboard, &focusChanged));
                }
            }
        }

        IFC_RETURN(pEventArgs->put_Handled(!!focusChanged));
    }

    return S_OK;
}

} } } } XAML_ABI_NAMESPACE_END
