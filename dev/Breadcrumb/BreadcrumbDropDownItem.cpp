// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbDropDownItem.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "ItemTemplateWrapper.h"
#include "BreadcrumbItem.h"
#include "BreadcrumbDropDownItemAutomationPeer.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithBasicFactory(BreadcrumbDropDownItem)
}

#include "BreadcrumbDropDownItem.g.cpp"

BreadcrumbDropDownItem::BreadcrumbDropDownItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_BreadcrumbDropDownItem);

    SetDefaultStyleKey(this);
}

BreadcrumbDropDownItem::~BreadcrumbDropDownItem()
{
    RevokeListeners();
}

void BreadcrumbDropDownItem::RevokeListeners()
{
    m_dropDownItemContentPresenterLoadedRevoker.revoke();
}

void BreadcrumbDropDownItem::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    RevokeListeners();

    winrt::IControlProtected controlProtected{ *this };

    m_dropDownItemContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(L"PART_ContentPresenter", controlProtected));

    if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        thisAsIUIElement7.PreviewKeyDown({ this, &BreadcrumbDropDownItem::OnChildPreviewKeyDown });
    }
    else if (auto const& thisAsUIElement = this->try_as<winrt::UIElement>())
    {
        m_dropDownItemKeyDownRevoker = AddRoutedEventHandler<RoutedEventType::KeyDown>(thisAsUIElement,
            { this, &BreadcrumbDropDownItem::OnChildPreviewKeyDown },
            true /*handledEventsToo*/);
    }

    if (const auto& contentPresenter = m_dropDownItemContentPresenter.get())
    {
        m_dropDownItemContentPresenterLoadedRevoker = contentPresenter.Loaded(winrt::auto_revoke, { this, &BreadcrumbDropDownItem::OnLoadedEvent });

        // Register for pointer events so we can keep track of the last used pointer type
        m_breadcrumbItemPointerEnteredRevoker = PointerEntered(winrt::auto_revoke, { this, &BreadcrumbDropDownItem::OnPointerEnteredEvent });
        m_breadcrumbItemPointerExitedRevoker = PointerExited(winrt::auto_revoke, { this, &BreadcrumbDropDownItem::OnPointerExitedEvent });
        m_breadcrumbItemPointerPressedRevoker = PointerPressed(winrt::auto_revoke, { this, &BreadcrumbDropDownItem::OnPointerPressedEvent });
        m_breadcrumbItemPointerReleasedRevoker = PointerReleased(winrt::auto_revoke, { this, &BreadcrumbDropDownItem::OnPointerReleasedEvent });
        m_breadcrumbItemPointerCanceledRevoker = PointerCanceled(winrt::auto_revoke, { this, &BreadcrumbDropDownItem::OnPointerReleasedEvent });
        m_breadcrumbItemPointerCaptureLostRevoker = PointerCaptureLost(winrt::auto_revoke, { this, &BreadcrumbDropDownItem::OnPointerReleasedEvent });
    }
       
    AddHandler(winrt::UIElement::PointerPressedEvent(),
        winrt::box_value<winrt::PointerEventHandler>({ this, &BreadcrumbDropDownItem::OnClickEvent }),
        true);
}

void BreadcrumbDropDownItem::OnLoadedEvent(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
}

void BreadcrumbDropDownItem::OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Enter)
    {
        this->OnClickEvent(sender, nullptr);
        args.Handled(true);
    }
    else
    {
        args.Handled(false);
    }
}

void BreadcrumbDropDownItem::OnClickEvent(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (const auto& breadcrumbItem = m_ellipsisBreadcrumbItem.get())
    {
        // Once an element has been clicked, close the flyout
        if (const auto& breadcrumbItemImpl = winrt::get_self<BreadcrumbItem>(breadcrumbItem))
        {
            breadcrumbItemImpl->CloseFlyout();
            breadcrumbItemImpl->RaiseItemClickedEvent(Content());
        }   
    }
}

void BreadcrumbDropDownItem::SetEllipsisBreadcrumbItem(const winrt::BreadcrumbItem& ellipsisBreadcrumbItem)
{
    m_ellipsisBreadcrumbItem.set(ellipsisBreadcrumbItem);
}

void BreadcrumbDropDownItem::OnPointerEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    UpdateCommonVisualState();
}

void BreadcrumbDropDownItem::OnPointerEnteredEvent(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs&)
{
    m_isPointerOver = true;
    UpdateCommonVisualState();
}

void BreadcrumbDropDownItem::OnPointerPressedEvent(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs&)
{
    m_isPressed = true;
    UpdateCommonVisualState();
}

void BreadcrumbDropDownItem::OnPointerReleasedEvent(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
{
    OnClickEvent(nullptr, nullptr);
    m_isPressed = false;
    UpdateCommonVisualState();
}

void BreadcrumbDropDownItem::OnPointerExitedEvent(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs&)
{
    m_isPointerOver = false;
    UpdateCommonVisualState();
}

void BreadcrumbDropDownItem::OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    UpdateCommonVisualState();
}

void BreadcrumbDropDownItem::UpdateCommonVisualState()
{
    if (const auto& contentPresenter = m_dropDownItemContentPresenter.get())
    {
        hstring commonVisualStateName = L"";

        if (!IsEnabled())
        {
            commonVisualStateName = commonVisualStateName + L"Disabled";
        }
        else if (m_isPressed)
        {
            commonVisualStateName = commonVisualStateName + L"Pressed";
        }
        else if (m_isPointerOver)
        {
            commonVisualStateName = commonVisualStateName + L"PointerOver";
        }
        else
        {
            commonVisualStateName = commonVisualStateName + L"Normal";
        }

        winrt::VisualStateManager::GoToState(*this, commonVisualStateName, false);
    }
}

winrt::AutomationPeer BreadcrumbDropDownItem::OnCreateAutomationPeer()
{
    return winrt::make<BreadcrumbDropDownItemAutomationPeer>(*this);
}
