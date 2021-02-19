// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbDropDownItem.h"
#include "RuntimeProfiler.h"
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
    HookListeners();
}

BreadcrumbDropDownItem::~BreadcrumbDropDownItem()
{
    RevokeListeners();
}

void BreadcrumbDropDownItem::HookListeners()
{
    if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        thisAsIUIElement7.PreviewKeyDown({ this, &BreadcrumbDropDownItem::OnChildPreviewKeyDown });
    }
    else if (auto const& thisAsUIElement = this->try_as<winrt::UIElement>())
    {
        m_keyDownRevoker = AddRoutedEventHandler<RoutedEventType::KeyDown>(thisAsUIElement,
            { this, &BreadcrumbDropDownItem::OnChildPreviewKeyDown },
            true /*handledEventsToo*/);
    }

    m_isEnabledChangedRevoker = IsEnabledChanged(winrt::auto_revoke, { this,  &BreadcrumbDropDownItem::OnIsEnabledChanged });
}

void BreadcrumbDropDownItem::RevokeListeners()
{
    m_keyDownRevoker.revoke();
}

void BreadcrumbDropDownItem::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    UpdateCommonVisualState(false /*useTransitions*/);
}

void BreadcrumbDropDownItem::OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Enter || args.Key() == winrt::VirtualKey::Space)
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
            breadcrumbItemImpl->RaiseItemClickedEvent(Content(), m_index - 1);
        }   
    }
}

void BreadcrumbDropDownItem::SetEllipsisBreadcrumbItem(const winrt::BreadcrumbItem& ellipsisBreadcrumbItem)
{
    m_ellipsisBreadcrumbItem.set(ellipsisBreadcrumbItem);
}

void BreadcrumbDropDownItem::SetIndex(const uint32_t index)
{
    m_index = index;
}

void BreadcrumbDropDownItem::OnIsEnabledChanged(
    const winrt::IInspectable&,
    const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateCommonVisualState(true /*useTransitions*/);
}

void BreadcrumbDropDownItem::OnPointerEntered(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerEntered(args);

    ProcessPointerOver(args);
}

void BreadcrumbDropDownItem::OnPointerMoved(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerMoved(args);

    ProcessPointerOver(args);
}

void BreadcrumbDropDownItem::OnPointerExited(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerExited(args);

    ProcessPointerCanceled(args);
}

void BreadcrumbDropDownItem::OnPointerPressed(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerPressed(args);

    if (IgnorePointerId(args))
    {
        return;
    }

    MUX_ASSERT(!m_isPressed);

    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse)
    {
        auto pointerProperties = args.GetCurrentPoint(*this).Properties();
        m_isPressed = pointerProperties.IsLeftButtonPressed();
    }
    else
    {
        m_isPressed = true;
    }

    if (m_isPressed)
    {
        UpdateCommonVisualState(true /*useTransitions*/);
    }
}

void BreadcrumbDropDownItem::OnPointerReleased(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerReleased(args);

    if (IgnorePointerId(args))
    {
        return;
    }

    if (m_isPressed)
    {
        m_isPressed = false;
        UpdateCommonVisualState(true /*useTransitions*/);
        OnClickEvent(nullptr, nullptr);
    }
}

void BreadcrumbDropDownItem::OnPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerCanceled(args);

    ProcessPointerCanceled(args);
}

void BreadcrumbDropDownItem::OnPointerCaptureLost(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerCaptureLost(args);

    ProcessPointerCanceled(args);
}

void BreadcrumbDropDownItem::ProcessPointerOver(const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    if (!m_isPointerOver)
    {
        m_isPointerOver = true;
        UpdateCommonVisualState(true /*useTransitions*/);
    }
}

void BreadcrumbDropDownItem::ProcessPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isPressed = false;
    m_isPointerOver = false;
    ResetTrackedPointerId();
    UpdateCommonVisualState(true /*useTransitions*/);
}

void BreadcrumbDropDownItem::ResetTrackedPointerId()
{
    m_trackedPointerId = 0;
}

// Returns False when the provided pointer Id matches the currently tracked Id.
// When there is no currently tracked Id, sets the tracked Id to the provided Id and returns False.
// Returns True when the provided pointer Id does not match the currently tracked Id.
bool BreadcrumbDropDownItem::IgnorePointerId(const winrt::PointerRoutedEventArgs& args)
{
    uint32_t pointerId = args.Pointer().PointerId();

    if (m_trackedPointerId == 0)
    {
        m_trackedPointerId = pointerId;
    }
    else if (m_trackedPointerId != pointerId)
    {
        return true;
    }
    return false;
}

void BreadcrumbDropDownItem::UpdateCommonVisualState(bool useTransitions)
{
    hstring commonVisualStateName;

    if (!IsEnabled())
    {
        commonVisualStateName = s_disabledStateName;
    }
    else if (m_isPressed)
    {
        commonVisualStateName = s_pressedStateName;
    }
    else if (m_isPointerOver)
    {
        commonVisualStateName = s_pointerOverStateName;
    }
    else
    {
        commonVisualStateName = s_normalStateName;
    }

    winrt::VisualStateManager::GoToState(*this, commonVisualStateName, useTransitions);
}

winrt::AutomationPeer BreadcrumbDropDownItem::OnCreateAutomationPeer()
{
    return winrt::make<BreadcrumbDropDownItemAutomationPeer>(*this);
}
