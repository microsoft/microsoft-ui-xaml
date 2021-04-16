// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbBarItem.h"
#include "RuntimeProfiler.h"
#include "ItemTemplateWrapper.h"
#include "BreadcrumbBar.h"
#include "BreadcrumbBarItemAutomationPeer.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithBasicFactory(BreadcrumbBarItem)
}

#include "BreadcrumbBarItem.g.cpp"

BreadcrumbBarItem::BreadcrumbBarItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_BreadcrumbBarItem);

    SetDefaultStyleKey(this);
}

BreadcrumbBarItem::~BreadcrumbBarItem()
{
    RevokeListeners();
}

void BreadcrumbBarItem::HookListeners(bool forEllipsisDropDownItem)
{
    if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        if (!m_childPreviewKeyDownToken.value)
        {
            m_childPreviewKeyDownToken = thisAsIUIElement7.PreviewKeyDown({ this, &BreadcrumbBarItem::OnChildPreviewKeyDown });
        }
    }
    else if (auto const& thisAsUIElement = this->try_as<winrt::UIElement>())
    {
        if (!m_keyDownRevoker)
        {
            m_keyDownRevoker = AddRoutedEventHandler<RoutedEventType::KeyDown>(thisAsUIElement,
                { this, &BreadcrumbBarItem::OnChildPreviewKeyDown },
                true /*handledEventsToo*/);
        }
    }

    if (forEllipsisDropDownItem)
    {
        if (!m_isEnabledChangedRevoker)
        {
            m_isEnabledChangedRevoker = IsEnabledChanged(winrt::auto_revoke, { this,  &BreadcrumbBarItem::OnIsEnabledChanged });
        }
    }
    else if (!m_flowDirectionChangedToken.value)
    {
        m_flowDirectionChangedToken.value = RegisterPropertyChangedCallback(winrt::FrameworkElement::FlowDirectionProperty(), { this, &BreadcrumbBarItem::OnFlowDirectionChanged });
    }
}

void BreadcrumbBarItem::RevokeListeners()
{
    if (m_flowDirectionChangedToken.value)
    {
        UnregisterPropertyChangedCallback(winrt::FrameworkElement::FlowDirectionProperty(), m_flowDirectionChangedToken.value);
        m_flowDirectionChangedToken.value = 0;
    }

    if (m_childPreviewKeyDownToken.value)
    {
        if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
        {
            thisAsIUIElement7.PreviewKeyDown(m_childPreviewKeyDownToken);
            m_childPreviewKeyDownToken.value = 0;
        }
    }

    m_keyDownRevoker.revoke();
}

void BreadcrumbBarItem::RevokePartsListeners()
{
    m_buttonLoadedRevoker.revoke();
    m_buttonClickRevoker.revoke();
    m_ellipsisRepeaterElementPreparedRevoker.revoke();
    m_ellipsisRepeaterElementIndexChangedRevoker.revoke();
    m_isPressedButtonRevoker.revoke();
    m_isPointerOverButtonRevoker.revoke();
    m_isEnabledButtonRevoker.revoke();
}

void BreadcrumbBarItem::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    if (m_isEllipsisDropDownItem)
    {
        UpdateEllipsisDropDownItemCommonVisualState(false /*useTransitions*/);
    }
    else
    {
        RevokePartsListeners();
        winrt::IControlProtected controlProtected{ *this };

        if (m_isEllipsisItem)
        {
            m_ellipsisFlyout.set(GetTemplateChildT<winrt::Flyout>(s_itemEllipsisFlyoutPartName, controlProtected));
        }

        m_button.set(GetTemplateChildT<winrt::Button>(s_itemButtonPartName, controlProtected));

        if (const auto& button = m_button.get())
        {
            m_buttonLoadedRevoker = button.Loaded(winrt::auto_revoke, { this, &BreadcrumbBarItem::OnLoadedEvent });

            m_isPressedButtonRevoker = RegisterPropertyChanged(button, winrt::ButtonBase::IsPressedProperty(), { this, &BreadcrumbBarItem::OnVisualPropertyChanged });
            m_isPointerOverButtonRevoker = RegisterPropertyChanged(button, winrt::ButtonBase::IsPointerOverProperty(), { this, &BreadcrumbBarItem::OnVisualPropertyChanged });
            m_isEnabledButtonRevoker = RegisterPropertyChanged(button, winrt::Control::IsEnabledProperty(), { this, &BreadcrumbBarItem::OnVisualPropertyChanged });
        }

        UpdateButtonCommonVisualState(false /*useTransitions*/);
        UpdateInlineItemTypeVisualState(false /*useTransitions*/);
    }

    UpdateItemTypeVisualState();
}

void BreadcrumbBarItem::OnLoadedEvent(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    m_buttonLoadedRevoker.revoke();

    if (auto button = m_button.get())
    {
        m_buttonClickRevoker.revoke();
        if (m_isEllipsisItem)
        {
            m_buttonClickRevoker = button.Click(winrt::auto_revoke, { this, &BreadcrumbBarItem::OnEllipsisItemClick });
        }
        else
        {
            m_buttonClickRevoker = button.Click(winrt::auto_revoke, { this, &BreadcrumbBarItem::OnBreadcrumbBarItemClick });
        }
    }

    if (m_isEllipsisItem)
    {
        SetPropertiesForEllipsisItem();
    }
    else if (m_isLastItem)
    {
        SetPropertiesForLastItem();
    }
    else
    {
        ResetVisualProperties();
    }
}

void BreadcrumbBarItem::SetParentBreadcrumb(const winrt::BreadcrumbBar& parent)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    m_parentBreadcrumb.set(parent);
}

void BreadcrumbBarItem::SetEllipsisDropDownItemDataTemplate(const winrt::IInspectable& newDataTemplate)
{
    if (auto const& dataTemplate = newDataTemplate.try_as<winrt::DataTemplate>())
    {
        m_ellipsisDropDownItemDataTemplate.set(dataTemplate);
    }
    else if (!newDataTemplate)
    {
        m_ellipsisDropDownItemDataTemplate.set(nullptr);
    }
}

void BreadcrumbBarItem::SetIndex(const uint32_t index)
{
    m_index = index;
}

void BreadcrumbBarItem::SetIsEllipsisDropDownItem(bool isEllipsisDropDownItem)
{
    m_isEllipsisDropDownItem = isEllipsisDropDownItem;

    HookListeners(m_isEllipsisDropDownItem);

    UpdateItemTypeVisualState();
}

void BreadcrumbBarItem::RaiseItemClickedEvent(const winrt::IInspectable& content, const uint32_t index)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        auto breadcrumbImpl = winrt::get_self<BreadcrumbBar>(breadcrumb);
        breadcrumbImpl->RaiseItemClickedEvent(content, index);
    }
}

void BreadcrumbBarItem::OnBreadcrumbBarItemClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    RaiseItemClickedEvent(Content(), m_index - 1);
}

void BreadcrumbBarItem::OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (const auto& ellipsisDropDownItem = args.Element().try_as<winrt::BreadcrumbBarItem>())
    {
        if (const auto& ellipsisDropDownItemImpl = winrt::get_self<BreadcrumbBarItem>(ellipsisDropDownItem))
        {
            ellipsisDropDownItemImpl->SetIsEllipsisDropDownItem(true /*isEllipsisDropDownItem*/);
        }
    }

    UpdateFlyoutIndex(args.Element(), args.Index());
}

void BreadcrumbBarItem::OnFlyoutElementIndexChangedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    UpdateFlyoutIndex(args.Element(), args.NewIndex());
}

void BreadcrumbBarItem::OnFlowDirectionChanged(winrt::DependencyObject const&, winrt::DependencyProperty const&)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    UpdateInlineItemTypeVisualState(true /*useTransitions*/);
}

void BreadcrumbBarItem::OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (m_isEllipsisDropDownItem)
    {
        if (args.Key() == winrt::VirtualKey::Enter || args.Key() == winrt::VirtualKey::Space)
        {
            this->OnClickEvent(sender, nullptr);
            args.Handled(true);
        }
        else if (SharedHelpers::IsRS2OrHigher() && !SharedHelpers::IsRS3OrHigher())
        {
            if (args.Key() == winrt::VirtualKey::Down)
            {
                winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Next);
                args.Handled(true);
            }
            else if (args.Key() == winrt::VirtualKey::Up)
            {
                winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Previous);
                args.Handled(true);
            }
        }
    }
    else if (args.Key() == winrt::VirtualKey::Enter || args.Key() == winrt::VirtualKey::Space)
    {
        if (m_isEllipsisItem)
        {
            OnEllipsisItemClick(nullptr, nullptr);
        }
        else
        {
            OnBreadcrumbBarItemClick(nullptr, nullptr);
        }
        args.Handled(true);
    }
}

void BreadcrumbBarItem::OnIsEnabledChanged(
    const winrt::IInspectable&,
    const winrt::DependencyPropertyChangedEventArgs&)
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

    UpdateEllipsisDropDownItemCommonVisualState(true /*useTransitions*/);
}

void BreadcrumbBarItem::UpdateFlyoutIndex(const winrt::UIElement& element, const uint32_t index)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (auto const& ellipsisItemsRepeater = m_ellipsisItemsRepeater.get())
    {
        if (auto const& itemSourceView = ellipsisItemsRepeater.ItemsSourceView())
        {
            const uint32_t itemCount = itemSourceView.Count();

            if (const auto& ellipsisDropDownItemImpl = element.try_as<BreadcrumbBarItem>())
            {
                ellipsisDropDownItemImpl->SetEllipsisItem(*this);
                ellipsisDropDownItemImpl->SetIndex(itemCount - index);
            }

            element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(index + 1));
            element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(itemCount));
        }
    }
}

winrt::IInspectable BreadcrumbBarItem::CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    // A copy of the hidden elements array in BreadcrumbLayout is created
    // to avoid getting a Layout cycle exception
    auto newItemsSource = winrt::make<Vector<winrt::IInspectable>>();

    // The new list contains all the elements in reverse order
    const int itemsSourceSize = ellipsisItemsSource.Size();

    // The itemsSourceSize should always be at least 1 as it must always contain the ellipsis item
    assert(itemsSourceSize > 0);

    for (int i = itemsSourceSize - 1; i >= 0; --i)
    {
        const auto& item = ellipsisItemsSource.GetAt(i);
        newItemsSource.Append(item);
    }

    return newItemsSource;
}

void BreadcrumbBarItem::OpenFlyout()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (auto flyout = m_ellipsisFlyout.get())
    {
        if (SharedHelpers::IsFlyoutShowOptionsAvailable())
        {
            winrt::FlyoutShowOptions options{};
            flyout.ShowAt(*this, options);
        }
        else
        {
            flyout.ShowAt(*this);
        }
    }
}

void BreadcrumbBarItem::CloseFlyout()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (auto flyout = m_ellipsisFlyout.get())
    {
        flyout.Hide();
    }
}

void BreadcrumbBarItem::OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    UpdateButtonCommonVisualState(true /*useTransitions*/);
}

void BreadcrumbBarItem::UpdateItemTypeVisualState()
{
    winrt::VisualStateManager::GoToState(*this, m_isEllipsisDropDownItem ? s_ellipsisDropDownStateName : s_inlineStateName, false /*useTransitions*/);
}

void BreadcrumbBarItem::UpdateEllipsisDropDownItemCommonVisualState(bool useTransitions)
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

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

void BreadcrumbBarItem::UpdateInlineItemTypeVisualState(bool useTransitions)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    const bool isLeftToRight = (FlowDirection() == winrt::FlowDirection::LeftToRight);
    hstring visualStateName;

    if (m_isEllipsisItem)
    {
        if (isLeftToRight)
        {
            visualStateName = s_ellipsisStateName;
        }
        else
        {
            visualStateName = s_ellipsisRTLStateName;
        }
    }
    else if (m_isLastItem)
    {
        visualStateName = s_lastItemStateName;
    }
    else if (isLeftToRight)
    {
        visualStateName = s_defaultStateName;
    }
    else
    {
        visualStateName = s_defaultRTLStateName;
    }

    winrt::VisualStateManager::GoToState(*this, visualStateName, useTransitions);
}

void BreadcrumbBarItem::UpdateButtonCommonVisualState(bool useTransitions)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (const auto& button = m_button.get())
    {
        hstring commonVisualStateName = L"";

        // If is last item: place Current as prefix for visual state
        if (m_isLastItem)
        {
            commonVisualStateName = s_currentStateName;
        }

        if (!button.IsEnabled())
        {
            commonVisualStateName = commonVisualStateName + s_disabledStateName;
        }
        else if (button.IsPressed())
        {
            commonVisualStateName = commonVisualStateName + s_pressedStateName;
        }
        else if (button.IsPointerOver())
        {
            commonVisualStateName = commonVisualStateName + s_pointerOverStateName;
        }
        else
        {
            commonVisualStateName = commonVisualStateName + s_normalStateName;
        }

        winrt::VisualStateManager::GoToState(button, commonVisualStateName, useTransitions);
    }
}

void BreadcrumbBarItem::OnEllipsisItemClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        if (const auto& breadcrumbImpl = breadcrumb.try_as<BreadcrumbBar>())
        {
            const auto& hiddenElements = CloneEllipsisItemSource(breadcrumbImpl->HiddenElements());

            if (const auto& dataTemplate = m_ellipsisDropDownItemDataTemplate.get())
            {
                m_ellipsisElementFactory->UserElementFactory(dataTemplate);
            }

            if (const auto& flyoutRepeater = m_ellipsisItemsRepeater.get())
            {
                flyoutRepeater.ItemsSource(hiddenElements);
            }

            OpenFlyout();
        }
    }
}

void BreadcrumbBarItem::SetPropertiesForLastItem()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    m_isEllipsisItem = false;
    m_isLastItem = true;

    UpdateButtonCommonVisualState(false /*useTransitions*/);
    UpdateInlineItemTypeVisualState(false /*useTransitions*/);
}

void BreadcrumbBarItem::ResetVisualProperties()
{
    if (m_isEllipsisDropDownItem)
    {
        UpdateEllipsisDropDownItemCommonVisualState(false /*useTransitions*/);
    }
    else
    {
        m_isEllipsisItem = false;
        m_isLastItem = false;

        if (const auto& button = m_button.get())
        {
            button.Flyout(nullptr);
        }
        m_ellipsisFlyout.set(nullptr);
        m_ellipsisItemsRepeater.set(nullptr);
        m_ellipsisElementFactory = nullptr;

        UpdateButtonCommonVisualState(false /*useTransitions*/);
        UpdateInlineItemTypeVisualState(false /*useTransitions*/);
    }
}

void BreadcrumbBarItem::InstantiateFlyout()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    // Only if the element has been created visually, instantiate the flyout
    if (const auto& button = m_button.get())
    {
        if (const auto& ellipsisFlyout = m_ellipsisFlyout.get())
        {
            // Create ItemsRepeater and set the DataTemplate 
            const auto& ellipsisItemsRepeater = winrt::ItemsRepeater();
            ellipsisItemsRepeater.Name(s_ellipsisItemsRepeaterPartName);
            winrt::AutomationProperties::SetName(ellipsisItemsRepeater, s_ellipsisItemsRepeaterAutomationName);
            ellipsisItemsRepeater.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);

            m_ellipsisElementFactory = winrt::make_self<BreadcrumbElementFactory>();
            ellipsisItemsRepeater.ItemTemplate(*m_ellipsisElementFactory);

            if (const auto& dataTemplate = m_ellipsisDropDownItemDataTemplate.get())
            {
                m_ellipsisElementFactory->UserElementFactory(dataTemplate);
            }

            m_ellipsisRepeaterElementPreparedRevoker = ellipsisItemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &BreadcrumbBarItem::OnFlyoutElementPreparedEvent });
            m_ellipsisRepeaterElementIndexChangedRevoker = ellipsisItemsRepeater.ElementIndexChanged(winrt::auto_revoke, { this, &BreadcrumbBarItem::OnFlyoutElementIndexChangedEvent });

            m_ellipsisItemsRepeater.set(ellipsisItemsRepeater);

            // Set the repeater as the content.
            winrt::AutomationProperties::SetName(ellipsisFlyout, s_ellipsisFlyoutAutomationName);
            ellipsisFlyout.Content(ellipsisItemsRepeater);
            ellipsisFlyout.Placement(winrt::FlyoutPlacementMode::Bottom);

            m_ellipsisFlyout.set(ellipsisFlyout);
        }
    }
}

void BreadcrumbBarItem::SetPropertiesForEllipsisItem()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    m_isEllipsisItem = true;
    m_isLastItem = false;

    InstantiateFlyout();

    UpdateButtonCommonVisualState(false /*useTransitions*/);
    UpdateInlineItemTypeVisualState(false /*useTransitions*/);
}

void BreadcrumbBarItem::SetEllipsisItem(const winrt::BreadcrumbBarItem& ellipsisItem)
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

    m_ellipsisItem.set(ellipsisItem);
}

winrt::AutomationPeer BreadcrumbBarItem::OnCreateAutomationPeer()
{
    return winrt::make<BreadcrumbBarItemAutomationPeer>(*this);
}

void BreadcrumbBarItem::OnPointerEntered(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerEntered(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerOver(args);
    }
}

void BreadcrumbBarItem::OnPointerMoved(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerMoved(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerOver(args);
    }
}

void BreadcrumbBarItem::OnPointerExited(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerExited(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerCanceled(args);
    }
}

void BreadcrumbBarItem::OnPointerPressed(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerPressed(args);

    if (m_isEllipsisDropDownItem)
    {
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
            UpdateEllipsisDropDownItemCommonVisualState(true /*useTransitions*/);
        }
    }
}

void BreadcrumbBarItem::OnPointerReleased(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerReleased(args);

    if (m_isEllipsisDropDownItem)
    {
        if (IgnorePointerId(args))
        {
            return;
        }

        if (m_isPressed)
        {
            m_isPressed = false;
            UpdateEllipsisDropDownItemCommonVisualState(true /*useTransitions*/);
            OnClickEvent(nullptr, nullptr);
        }
    }
}

void BreadcrumbBarItem::OnPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerCanceled(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerCanceled(args);
    }
}

void BreadcrumbBarItem::OnPointerCaptureLost(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerCaptureLost(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerCanceled(args);
    }
}

void BreadcrumbBarItem::ProcessPointerOver(const winrt::PointerRoutedEventArgs& args)
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

    if (IgnorePointerId(args))
    {
        return;
    }

    if (!m_isPointerOver)
    {
        m_isPointerOver = true;
        UpdateEllipsisDropDownItemCommonVisualState(true /*useTransitions*/);
    }
}

void BreadcrumbBarItem::ProcessPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

    if (IgnorePointerId(args))
    {
        return;
    }

    m_isPressed = false;
    m_isPointerOver = false;
    ResetTrackedPointerId();
    UpdateEllipsisDropDownItemCommonVisualState(true /*useTransitions*/);
}

void BreadcrumbBarItem::ResetTrackedPointerId()
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

    m_trackedPointerId = 0;
}

// Returns False when the provided pointer Id matches the currently tracked Id.
// When there is no currently tracked Id, sets the tracked Id to the provided Id and returns False.
// Returns True when the provided pointer Id does not match the currently tracked Id.
bool BreadcrumbBarItem::IgnorePointerId(const winrt::PointerRoutedEventArgs& args)
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

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

void BreadcrumbBarItem::OnClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (m_isEllipsisDropDownItem)
    {
        if (const auto& ellipsisItem = m_ellipsisItem.get())
        {
            // Once an element has been clicked, close the flyout
            if (const auto& ellipsisItemImpl = winrt::get_self<BreadcrumbBarItem>(ellipsisItem))
            {
                ellipsisItemImpl->CloseFlyout();
                ellipsisItemImpl->RaiseItemClickedEvent(Content(), m_index - 1);
            }
        }
    }
    else if (m_isEllipsisItem)
    {
        OnEllipsisItemClick(nullptr, nullptr);
    }
    else
    {
        OnBreadcrumbBarItemClick(nullptr, nullptr);
    }
}
