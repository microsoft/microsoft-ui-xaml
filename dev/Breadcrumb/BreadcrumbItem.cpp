// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbItem.h"
#include "RuntimeProfiler.h"
#include "ItemTemplateWrapper.h"
#include "Breadcrumb.h"
#include "BreadcrumbItemAutomationPeer.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithBasicFactory(BreadcrumbItem)
}

#include "BreadcrumbItem.g.cpp"

BreadcrumbItem::BreadcrumbItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_BreadcrumbItem);

    SetDefaultStyleKey(this);
}

BreadcrumbItem::~BreadcrumbItem()
{
    RevokeListeners();
}

void BreadcrumbItem::HookListeners(bool forEllipsisDropDownItem)
{
    if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        if (!m_childPreviewKeyDownToken.value)
        {
            m_childPreviewKeyDownToken = thisAsIUIElement7.PreviewKeyDown({ this, &BreadcrumbItem::OnChildPreviewKeyDown });
        }
    }
    else if (auto const& thisAsUIElement = this->try_as<winrt::UIElement>())
    {
        if (!m_keyDownRevoker)
        {
            m_keyDownRevoker = AddRoutedEventHandler<RoutedEventType::KeyDown>(thisAsUIElement,
                { this, &BreadcrumbItem::OnChildPreviewKeyDown },
                true /*handledEventsToo*/);
        }
    }

    if (forEllipsisDropDownItem)
    {
        if (!m_isEnabledChangedRevoker)
        {
            m_isEnabledChangedRevoker = IsEnabledChanged(winrt::auto_revoke, { this,  &BreadcrumbItem::OnIsEnabledChanged });
        }
    }
    else if (!m_flowDirectionChangedToken.value)
    {
        m_flowDirectionChangedToken.value = RegisterPropertyChangedCallback(winrt::FrameworkElement::FlowDirectionProperty(), { this, &BreadcrumbItem::OnFlowDirectionChanged });
    }
}

void BreadcrumbItem::RevokeListeners()
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

void BreadcrumbItem::RevokePartsListeners()
{
    m_buttonLoadedRevoker.revoke();
    m_buttonClickRevoker.revoke();
    m_ellipsisRepeaterElementPreparedRevoker.revoke();
    m_ellipsisRepeaterElementIndexChangedRevoker.revoke();
    m_isPressedButtonRevoker.revoke();
    m_isPointerOverButtonRevoker.revoke();
    m_isEnabledButtonRevoker.revoke();
}

void BreadcrumbItem::OnApplyTemplate()
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

        m_button.set(GetTemplateChildT<winrt::Button>(s_itemButtonPartName, controlProtected));
        m_grid.set(GetTemplateChildT<winrt::Grid>(s_itemGridPartName, controlProtected));

        if (const auto& button = m_button.get())
        {
            m_buttonLoadedRevoker = button.Loaded(winrt::auto_revoke, { this, &BreadcrumbItem::OnLoadedEvent });

            m_isPressedButtonRevoker = RegisterPropertyChanged(button, winrt::ButtonBase::IsPressedProperty(), { this, &BreadcrumbItem::OnVisualPropertyChanged });
            m_isPointerOverButtonRevoker = RegisterPropertyChanged(button, winrt::ButtonBase::IsPointerOverProperty(), { this, &BreadcrumbItem::OnVisualPropertyChanged });
            m_isEnabledButtonRevoker = RegisterPropertyChanged(button, winrt::Control::IsEnabledProperty(), { this, &BreadcrumbItem::OnVisualPropertyChanged });
        }

        UpdateButtonCommonVisualState(false /*useTransitions*/);
        UpdateInlineItemTypeVisualState(false /*useTransitions*/);
    }

    UpdateItemTypeVisualState();
}

void BreadcrumbItem::OnLoadedEvent(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    m_buttonLoadedRevoker.revoke();

    if (auto button = m_button.get())
    {
        m_buttonClickRevoker.revoke();
        if (m_isEllipsisItem)
        {
            m_buttonClickRevoker = button.Click(winrt::auto_revoke, { this, &BreadcrumbItem::OnEllipsisItemClick });
        }
        else
        {
            m_buttonClickRevoker = button.Click(winrt::auto_revoke, { this, &BreadcrumbItem::OnBreadcrumbItemClick });
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

void BreadcrumbItem::SetParentBreadcrumb(const winrt::Breadcrumb& parent)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    m_parentBreadcrumb.set(parent);
}

void BreadcrumbItem::SetEllipsisDropDownItemDataTemplate(const winrt::IInspectable& newDataTemplate)
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

void BreadcrumbItem::SetIndex(const uint32_t index)
{
    m_index = index;
}

void BreadcrumbItem::SetIsEllipsisDropDownItem(bool isEllipsisDropDownItem)
{
    m_isEllipsisDropDownItem = isEllipsisDropDownItem;

    HookListeners(m_isEllipsisDropDownItem);

    UpdateItemTypeVisualState();
}

void BreadcrumbItem::RaiseItemClickedEvent(const winrt::IInspectable& content, const uint32_t index)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        auto breadcrumbImpl = winrt::get_self<Breadcrumb>(breadcrumb);
        breadcrumbImpl->RaiseItemClickedEvent(content, index);
    }
}

void BreadcrumbItem::OnBreadcrumbItemClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    RaiseItemClickedEvent(Content(), m_index - 1);
}

void BreadcrumbItem::OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (const auto& ellipsisDropDownItem = args.Element().try_as<winrt::BreadcrumbItem>())
    {
        if (const auto& ellipsisDropDownItemImpl = winrt::get_self<BreadcrumbItem>(ellipsisDropDownItem))
        {
            ellipsisDropDownItemImpl->SetIsEllipsisDropDownItem(true /*isEllipsisDropDownItem*/);
        }
    }

    UpdateFlyoutIndex(args.Element(), args.Index());
}

void BreadcrumbItem::OnFlyoutElementIndexChangedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    UpdateFlyoutIndex(args.Element(), args.NewIndex());
}

void BreadcrumbItem::OnFlowDirectionChanged(winrt::DependencyObject const&, winrt::DependencyProperty const&)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    UpdateInlineItemTypeVisualState(true /*useTransitions*/);
}

void BreadcrumbItem::OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
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
            OnBreadcrumbItemClick(nullptr, nullptr);
        }
        args.Handled(true);
    }
}

void BreadcrumbItem::OnIsEnabledChanged(
    const winrt::IInspectable&,
    const winrt::DependencyPropertyChangedEventArgs&)
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

    UpdateEllipsisDropDownItemCommonVisualState(true /*useTransitions*/);
}

void BreadcrumbItem::UpdateFlyoutIndex(const winrt::UIElement& element, const uint32_t index)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (auto const& ellipsisItemsRepeater = m_ellipsisItemsRepeater.get())
    {
        if (auto const& itemSourceView = ellipsisItemsRepeater.ItemsSourceView())
        {
            const uint32_t itemCount = itemSourceView.Count();

            if (const auto& ellipsisDropDownItemImpl = element.try_as<BreadcrumbItem>())
            {
                ellipsisDropDownItemImpl->SetEllipsisItem(*this);
                ellipsisDropDownItemImpl->SetIndex(itemCount - index);
            }

            hstring name = s_ellipsisItemAutomationName + winrt::to_hstring(index + 1);
            winrt::AutomationProperties::SetName(element, name);

            element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(index + 1));
            element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(itemCount));
        }
    }
}

winrt::IInspectable BreadcrumbItem::CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource)
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

winrt::Style BreadcrumbItem::RetrieveFlyoutStyle()
{
    if (const auto& grid = m_grid.get())
    {
        const auto& resources = grid.Resources();
        for (const auto& themeDictionaryEntry : resources)
        {
            winrt::hstring entryKey = winrt::unbox_value_or<winrt::hstring>(themeDictionaryEntry.Key(), L"");
            if (const auto& entryValue = themeDictionaryEntry.Value().try_as<winrt::Style>())
            {
                const auto& targetType = entryValue.TargetType();
                const auto& name = targetType.Name;
                return entryValue;
            }
        }
    }

    return winrt::Style();
}

void BreadcrumbItem::OpenFlyout()
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

void BreadcrumbItem::CloseFlyout()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (auto flyout = m_ellipsisFlyout.get())
    {
        flyout.Hide();
    }
}

void BreadcrumbItem::OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    UpdateButtonCommonVisualState(true /*useTransitions*/);
}

void BreadcrumbItem::UpdateItemTypeVisualState()
{
    winrt::VisualStateManager::GoToState(*this, m_isEllipsisDropDownItem ? s_ellipsisDropDownStateName : s_inlineStateName, false /*useTransitions*/);
}

void BreadcrumbItem::UpdateEllipsisDropDownItemCommonVisualState(bool useTransitions)
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

void BreadcrumbItem::UpdateInlineItemTypeVisualState(bool useTransitions)
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

void BreadcrumbItem::UpdateButtonCommonVisualState(bool useTransitions)
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

void BreadcrumbItem::OnEllipsisItemClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        if (const auto& breadcrumbImpl = breadcrumb.try_as<Breadcrumb>())
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

void BreadcrumbItem::SetPropertiesForLastItem()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    m_isEllipsisItem = false;
    m_isLastItem = true;

    UpdateButtonCommonVisualState(false /*useTransitions*/);
    UpdateInlineItemTypeVisualState(false /*useTransitions*/);
}

void BreadcrumbItem::ResetVisualProperties()
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

void BreadcrumbItem::InstantiateFlyout()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    // Only if the element has been created visually, instantiate the flyout
    if (const auto& button = m_button.get())
    {
        // Create ItemsRepeater and set the DataTemplate 
        const auto& ellipsisItemsRepeater = winrt::ItemsRepeater();
        ellipsisItemsRepeater.Name(s_ellipsisItemsRepeaterPartName);
        winrt::AutomationProperties::SetName(ellipsisItemsRepeater, s_ellipsisItemsRepeaterAutomationName);
        ellipsisItemsRepeater.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);

        m_ellipsisElementFactory = winrt::make_self<BreadcrumbElementFactory>();
        ellipsisItemsRepeater.ItemTemplate(*m_ellipsisElementFactory);

        const auto& stackLayout = winrt::StackLayout();
        stackLayout.Orientation(winrt::Controls::Orientation::Vertical);
        ellipsisItemsRepeater.Layout(stackLayout);

        if (const auto& dataTemplate = m_ellipsisDropDownItemDataTemplate.get())
        {
            m_ellipsisElementFactory->UserElementFactory(dataTemplate);
        }

        m_ellipsisRepeaterElementPreparedRevoker = ellipsisItemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &BreadcrumbItem::OnFlyoutElementPreparedEvent });
        m_ellipsisRepeaterElementIndexChangedRevoker = ellipsisItemsRepeater.ElementIndexChanged(winrt::auto_revoke, { this, &BreadcrumbItem::OnFlyoutElementIndexChangedEvent });
        
        m_ellipsisItemsRepeater.set(ellipsisItemsRepeater);

        // Create the Flyout and add the ItemsRepeater as content
        const auto& ellipsisFlyout = winrt::Flyout();
        winrt::AutomationProperties::SetName(ellipsisFlyout, s_ellipsisFlyoutAutomationName);     
        ellipsisFlyout.Content(ellipsisItemsRepeater);
        ellipsisFlyout.Placement(winrt::FlyoutPlacementMode::Bottom);
        ellipsisFlyout.FlyoutPresenterStyle(RetrieveFlyoutStyle());

        m_ellipsisFlyout.set(ellipsisFlyout);

        // Set the Flyout to the ellipsis button
        button.Flyout(ellipsisFlyout);
    }
}

void BreadcrumbItem::SetPropertiesForEllipsisItem()
{
    MUX_ASSERT(!m_isEllipsisDropDownItem);

    m_isEllipsisItem = true;
    m_isLastItem = false;

    InstantiateFlyout();

    UpdateButtonCommonVisualState(false /*useTransitions*/);
    UpdateInlineItemTypeVisualState(false /*useTransitions*/);
}

void BreadcrumbItem::SetEllipsisItem(const winrt::BreadcrumbItem& ellipsisItem)
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

    m_ellipsisItem.set(ellipsisItem);
}

winrt::AutomationPeer BreadcrumbItem::OnCreateAutomationPeer()
{
    return winrt::make<BreadcrumbItemAutomationPeer>(*this);
}

void BreadcrumbItem::OnPointerEntered(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerEntered(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerOver(args);
    }
}

void BreadcrumbItem::OnPointerMoved(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerMoved(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerOver(args);
    }
}

void BreadcrumbItem::OnPointerExited(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerExited(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerCanceled(args);
    }
}

void BreadcrumbItem::OnPointerPressed(const winrt::PointerRoutedEventArgs& args)
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

void BreadcrumbItem::OnPointerReleased(const winrt::PointerRoutedEventArgs& args)
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

void BreadcrumbItem::OnPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerCanceled(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerCanceled(args);
    }
}

void BreadcrumbItem::OnPointerCaptureLost(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerCaptureLost(args);

    if (m_isEllipsisDropDownItem)
    {
        ProcessPointerCanceled(args);
    }
}

void BreadcrumbItem::ProcessPointerOver(const winrt::PointerRoutedEventArgs& args)
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

void BreadcrumbItem::ProcessPointerCanceled(const winrt::PointerRoutedEventArgs& args)
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

void BreadcrumbItem::ResetTrackedPointerId()
{
    MUX_ASSERT(m_isEllipsisDropDownItem);

    m_trackedPointerId = 0;
}

// Returns False when the provided pointer Id matches the currently tracked Id.
// When there is no currently tracked Id, sets the tracked Id to the provided Id and returns False.
// Returns True when the provided pointer Id does not match the currently tracked Id.
bool BreadcrumbItem::IgnorePointerId(const winrt::PointerRoutedEventArgs& args)
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

void BreadcrumbItem::OnClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (m_isEllipsisDropDownItem)
    {
        if (const auto& ellipsisItem = m_ellipsisItem.get())
        {
            // Once an element has been clicked, close the flyout
            if (const auto& ellipsisItemImpl = winrt::get_self<BreadcrumbItem>(ellipsisItem))
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
        OnBreadcrumbItemClick(nullptr, nullptr);
    }
}
