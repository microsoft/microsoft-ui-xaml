﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "BreadcrumbBar.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "BreadcrumbBarItem.h"
#include "BreadcrumbLayout.h"
#include "BreadcrumbBarItemClickedEventArgs.h"

BreadcrumbBar::BreadcrumbBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_BreadcrumbBar);

    SetDefaultStyleKey(this);
    m_itemsRepeaterElementFactory = winrt::make_self<BreadcrumbElementFactory>();
    m_itemsRepeaterLayout = winrt::make_self<BreadcrumbLayout>(*this);
    m_itemsIterable = winrt::make_self<BreadcrumbIterable>();
}

void BreadcrumbBar::RevokeListeners()
{
    m_itemsRepeaterLoadedRevoker.revoke();
    m_itemsRepeaterElementPreparedRevoker.revoke();
    m_itemsRepeaterElementIndexChangedRevoker.revoke();
    m_itemsRepeaterElementClearingRevoker.revoke();
    m_itemsSourceChanged.revoke();
    m_itemsSourceAsObservableVectorChanged.revoke();
}

void BreadcrumbBar::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    RevokeListeners();

    winrt::IControlProtected controlProtected{ *this };

    m_itemsRepeater.set(GetTemplateChildT<winrt::ItemsRepeater>(s_itemsRepeaterPartName, controlProtected));

    if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        thisAsIUIElement7.PreviewKeyDown({ this, &BreadcrumbBar::OnChildPreviewKeyDown });
    }
    else if (auto const& thisAsUIElement = this->try_as<winrt::UIElement>())
    {
        m_breadcrumbKeyDownHandlerRevoker = AddRoutedEventHandler<RoutedEventType::KeyDown>(thisAsUIElement,
            { this, &BreadcrumbBar::OnChildPreviewKeyDown },
            true /*handledEventsToo*/);
    }

    AccessKeyInvoked({ this, &BreadcrumbBar::OnAccessKeyInvoked });
    GettingFocus({ this, &BreadcrumbBar::OnGettingFocus });

    RegisterPropertyChangedCallback(winrt::FrameworkElement::FlowDirectionProperty(), { this, &BreadcrumbBar::OnFlowDirectionChanged });

    if (const auto& itemsRepeater = m_itemsRepeater.get())
    {
        itemsRepeater.Layout(*m_itemsRepeaterLayout);
        itemsRepeater.ItemsSource(winrt::make<Vector<IInspectable>>());
        itemsRepeater.ItemTemplate(*m_itemsRepeaterElementFactory);

        m_itemsRepeaterElementPreparedRevoker = itemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &BreadcrumbBar::OnElementPreparedEvent });
        m_itemsRepeaterElementIndexChangedRevoker = itemsRepeater.ElementIndexChanged(winrt::auto_revoke, { this, &BreadcrumbBar::OnElementIndexChangedEvent });
        m_itemsRepeaterElementClearingRevoker = itemsRepeater.ElementClearing(winrt::auto_revoke, { this, &BreadcrumbBar::OnElementClearingEvent });

        m_itemsRepeaterLoadedRevoker = itemsRepeater.Loaded(winrt::auto_revoke, { this, &BreadcrumbBar::OnBreadcrumbBarItemsRepeaterLoaded });
    }

    UpdateItemsRepeaterItemsSource();
}

void BreadcrumbBar::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const winrt::IDependencyProperty& property = args.Property();

    if (property == s_ItemsSourceProperty)
    {
        UpdateItemsRepeaterItemsSource();
    }
    else if (property == s_ItemTemplateProperty)
    {
        UpdateItemTemplate();
        UpdateEllipsisBreadcrumbBarItemDropDownItemTemplate();
    }
}

void BreadcrumbBar::OnFlowDirectionChanged(winrt::DependencyObject const& o, winrt::DependencyProperty const& p)
{
    UpdateBreadcrumbBarItemsFlowDirection();
}

void BreadcrumbBar::OnBreadcrumbBarItemsRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (const auto& breadcrumbItemsRepeater = m_itemsRepeater.get())
    {
        OnBreadcrumbBarItemsSourceCollectionChanged(nullptr, nullptr);
    }
}

void BreadcrumbBar::UpdateItemTemplate()
{
    const winrt::IInspectable& newItemTemplate = ItemTemplate();
    m_itemsRepeaterElementFactory->UserElementFactory(newItemTemplate);
}

void BreadcrumbBar::UpdateEllipsisBreadcrumbBarItemDropDownItemTemplate()
{
    const winrt::IInspectable& newItemTemplate = ItemTemplate();

    // Copy the item template to the ellipsis item too
    if (const auto& ellipsisBreadcrumbBarItem = m_ellipsisBreadcrumbBarItem.get())
    {
        if (const auto& itemImpl = winrt::get_self<BreadcrumbBarItem>(ellipsisBreadcrumbBarItem))
        {
            itemImpl->SetEllipsisDropDownItemDataTemplate(newItemTemplate);
        }
    }
}

void BreadcrumbBar::UpdateBreadcrumbBarItemsFlowDirection()
{
    // Only if some ItemsSource has been defined then we change the BreadcrumbBarItems flow direction
    if (ItemsSource())
    {
        if (const auto& itemsRepeater = m_itemsRepeater.get())
        {
            // Add 1 to account for the leading null
            const int32_t elementCount = m_breadcrumbItemsSourceView.Count() + 1;
            for (int32_t i{}; i < elementCount; ++i)
            {
                const auto& element = itemsRepeater.TryGetElement(i).try_as<winrt::BreadcrumbBarItem>();
                element.FlowDirection(FlowDirection());
            }
        }
    }
}

void BreadcrumbBar::UpdateItemsRepeaterItemsSource()
{
    m_itemsSourceChanged.revoke();
    m_itemsSourceAsObservableVectorChanged.revoke();

    m_breadcrumbItemsSourceView = nullptr;
    if (ItemsSource())
    {
        m_breadcrumbItemsSourceView = winrt::ItemsSourceView(ItemsSource());
        if (const auto& itemsRepeater = m_itemsRepeater.get())
        {
            m_itemsIterable = winrt::make_self<BreadcrumbIterable>(ItemsSource());
            itemsRepeater.ItemsSource(*m_itemsIterable);
        }
        if (m_breadcrumbItemsSourceView)
        {
            m_itemsSourceChanged = m_breadcrumbItemsSourceView.CollectionChanged(winrt::auto_revoke, { this, &BreadcrumbBar::OnBreadcrumbBarItemsSourceCollectionChanged });
        }
    }
}

void BreadcrumbBar::OnBreadcrumbBarItemsSourceCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable& args)
{
    if (const auto& itemsRepeater = m_itemsRepeater.get())
    {
        // A new BreadcrumbIterable must be created as ItemsRepeater compares if the previous
        // itemsSource is equals to the new one
        m_itemsIterable = winrt::make_self<BreadcrumbIterable>(ItemsSource());
        itemsRepeater.ItemsSource(*m_itemsIterable);

        // For some reason, when interacting with keyboard, the last element doesn't raise the OnPrepared event
        ForceUpdateLastElement();
    }
}

void BreadcrumbBar::ResetLastBreadcrumbBarItem()
{
    if (const auto& lastItem = m_lastBreadcrumbBarItem.get())
    {
        auto lastItemImpl = winrt::get_self<BreadcrumbBarItem>(lastItem);
        lastItemImpl->ResetVisualProperties();
    }
}

void BreadcrumbBar::ForceUpdateLastElement()
{
    if (m_breadcrumbItemsSourceView)
    {
        const uint32_t itemCount = m_breadcrumbItemsSourceView.Count();

        if (const auto& itemsRepeater = m_itemsRepeater.get())
        {
            const auto& newLastItem = itemsRepeater.TryGetElement(itemCount).try_as<winrt::BreadcrumbBarItem>();
            UpdateLastElement(newLastItem);
        }

        // If the given collection is empty, then reset the last element visual properties
        if (itemCount == 0)
        {
            ResetLastBreadcrumbBarItem();
        }
    }
    else
    {
        // Or if the ItemsSource was null, also reset the last breadcrumb Item
        ResetLastBreadcrumbBarItem();
    }
}

void BreadcrumbBar::UpdateLastElement(const winrt::BreadcrumbBarItem& newLastBreadcrumbBarItem)
{
    // If the element is the last element in the array,
    // then we reset the visual properties for the previous
    // last element
    ResetLastBreadcrumbBarItem();

    if (const auto& newLastItemImpl = winrt::get_self<BreadcrumbBarItem>(newLastBreadcrumbBarItem))
    {
        newLastItemImpl->SetPropertiesForLastItem();
        m_lastBreadcrumbBarItem.set(newLastBreadcrumbBarItem);
    }
}

void BreadcrumbBar::OnElementPreparedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs& args)
{
    if (const auto& item = args.Element().try_as<winrt::BreadcrumbBarItem>())
    {
        if (const auto& itemImpl = winrt::get_self<BreadcrumbBarItem>(item))
        {
            itemImpl->SetIsEllipsisDropDownItem(false /*isEllipsisDropDownItem*/);

            // Set the parent breadcrumb reference for raising click events
            itemImpl->SetParentBreadcrumb(*this);

            // Set the item index to fill the Index parameter in the ClickedEventArgs
            const uint32_t itemIndex = args.Index();
            itemImpl->SetIndex(itemIndex);

            // The first element is always the ellipsis item
            if (itemIndex == 0)
            {
                itemImpl->SetPropertiesForEllipsisItem();
                m_ellipsisBreadcrumbBarItem.set(item);
                UpdateEllipsisBreadcrumbBarItemDropDownItemTemplate();

                winrt::AutomationProperties::SetName(item, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameEllipsisBreadcrumbBarItem));
            }
            else
            {
                if (m_breadcrumbItemsSourceView)
                {
                    const uint32_t itemCount = m_breadcrumbItemsSourceView.Count();

                    if (itemIndex == itemCount)
                    {
                        UpdateLastElement(item);
                    }
                    else
                    {
                        // Any other element just resets the visual properties
                        itemImpl->ResetVisualProperties();
                    }
                }
            }
        }
    }
}

void BreadcrumbBar::OnElementIndexChangedEvent(const winrt::ItemsRepeater& sender, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    if (m_focusedIndex == args.OldIndex())
    {
        const uint32_t newIndex = args.NewIndex();

        if (const auto& item = args.Element().try_as<winrt::BreadcrumbBarItem>())
        {
            if (const auto& itemImpl = winrt::get_self<BreadcrumbBarItem>(item))
            {
                itemImpl->SetIndex(newIndex);
            }
        }

        FocusElementAt(newIndex);
    }
}

void BreadcrumbBar::OnElementClearingEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementClearingEventArgs& args)
{
    if (const auto& item = args.Element().try_as<winrt::BreadcrumbBarItem>())
    {
        const auto& itemImpl = winrt::get_self<BreadcrumbBarItem>(item);
        itemImpl->ResetVisualProperties();
    }
}

void BreadcrumbBar::RaiseItemClickedEvent(const winrt::IInspectable& content, const uint32_t index)
{
    const auto& eventArgs = winrt::make_self<BreadcrumbBarItemClickedEventArgs>();
    eventArgs->Item(content);
    eventArgs->Index(index);

    if (m_itemClickedEventSource)
    {
        m_itemClickedEventSource(*this, *eventArgs);
    }
}

winrt::IVector<winrt::IInspectable> BreadcrumbBar::GetHiddenElementsList(uint32_t firstShownElement) const
{
    auto hiddenElements = winrt::make<Vector<winrt::IInspectable>>();

    if (m_breadcrumbItemsSourceView)
    {
        for (uint32_t i = 0; i < firstShownElement - 1; ++i)
        {
            hiddenElements.Append(m_breadcrumbItemsSourceView.GetAt(i));
        }
    }

    return hiddenElements;
}

winrt::IVector<winrt::IInspectable> BreadcrumbBar::HiddenElements() const
{
    // The hidden element list is generated in the BreadcrumbLayout during
    // the arrange method, so we retrieve the list from it
    if (const auto& itemsRepeater = m_itemsRepeater.get())
    {
        if (m_itemsRepeaterLayout)
        {
            if (m_itemsRepeaterLayout->EllipsisIsRendered())
            {
                return GetHiddenElementsList(m_itemsRepeaterLayout->FirstRenderedItemIndexAfterEllipsis());
            }
        }
    }

    // By default just return an empty list
    return winrt::make<Vector<winrt::IInspectable>>();
}

void BreadcrumbBar::ReIndexVisibleElementsForAccessibility() const
{
    // Once the arrangement of BreadcrumbBar Items has happened then index all visible items
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        const uint32_t visibleItemsCount{ m_itemsRepeaterLayout->GetVisibleItemsCount() };
        const auto isEllipsisRendered = m_itemsRepeaterLayout->EllipsisIsRendered();
        uint32_t firstItemToIndex{ 1 };

        if (isEllipsisRendered)
        {
            firstItemToIndex = m_itemsRepeaterLayout->FirstRenderedItemIndexAfterEllipsis();
        }

        // In order to make the ellipsis inaccessible to accessbility tools when it's hidden,
        // we set the accessibilityView to raw and restore it to content when it becomes visible.
        if (const auto ellipsisItem = m_ellipsisBreadcrumbBarItem.get())
        {
            const auto accessibilityView = isEllipsisRendered ? winrt::AccessibilityView::Content : winrt::AccessibilityView::Raw;
            ellipsisItem.SetValue(winrt::AutomationProperties::AccessibilityViewProperty(), box_value(accessibilityView));
        }

        const auto& itemsSourceView = itemsRepeater.ItemsSourceView();

        // For every BreadcrumbBar item we set the index (starting from 1 for the root/highest-level item)
        // accessibilityIndex is the index to be assigned to each item
        // itemToIndex is the real index and it may differ from accessibilityIndex as we must only index the visible items
        for (uint32_t accessibilityIndex = 1, itemToIndex = firstItemToIndex; accessibilityIndex <= visibleItemsCount; ++accessibilityIndex, ++itemToIndex)
        {
            if (const auto& element = itemsRepeater.TryGetElement(itemToIndex))
            {
                element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(accessibilityIndex));
                element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(visibleItemsCount));
            }
        }
    }
}

// When focus comes from outside the BreadcrumbBar control we will put focus on the selected item.
void BreadcrumbBar::OnGettingFocus(const winrt::IInspectable&, const winrt::GettingFocusEventArgs& args)
{
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        auto const& inputDevice = args.InputDevice();
        if (inputDevice == winrt::FocusInputDeviceKind::Keyboard)
        {
            // If focus is coming from outside the repeater, put focus on the selected item.
            auto const& oldFocusedElement = args.OldFocusedElement();
            if (!oldFocusedElement || itemsRepeater != winrt::VisualTreeHelper::GetParent(oldFocusedElement))
            {
                // Reset the focused index
                if (m_itemsRepeaterLayout)
                {
                    if (m_itemsRepeaterLayout.get()->EllipsisIsRendered())
                    {
                        m_focusedIndex = 0;
                    }
                    else
                    {
                        m_focusedIndex = 1;
                    }

                    FocusElementAt(m_focusedIndex);
                }

                if (auto const& selectedItem = itemsRepeater.TryGetElement(m_focusedIndex))
                {
                    if (auto const& argsAsIGettingFocusEventArgs2 = args.try_as<winrt::IGettingFocusEventArgs2>())
                    {
                        if (args.TrySetNewFocusedElement(selectedItem))
                        {
                            args.Handled(true);
                        }
                    }
                }
            }

            // Focus was already in the repeater: in RS3+ Selection follows focus unless control is held down.
            else if (SharedHelpers::IsRS3OrHigher() &&
                (winrt::Window::Current().CoreWindow().GetKeyState(winrt::VirtualKey::Control) &
                    winrt::CoreVirtualKeyStates::Down) != winrt::CoreVirtualKeyStates::Down)
            {
                if (auto const& newFocusedElementAsUIE = args.NewFocusedElement().as<winrt::UIElement>())
                {
                    FocusElementAt(itemsRepeater.GetElementIndex(newFocusedElementAsUIE));
                    args.Handled(true);
                }
            }
        }
    }
}

void BreadcrumbBar::FocusElementAt(int index)
{
    if (index >= 0)
    {
        m_focusedIndex = index;
    }
}

bool BreadcrumbBar::MoveFocus(int indexIncrement)
{
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        const auto& focusedElem = winrt::FocusManager::GetFocusedElement();

        if (auto const& focusedElement = focusedElem.try_as<winrt::UIElement>())
        {
            auto focusedIndex = itemsRepeater.GetElementIndex(focusedElement);

            if (focusedIndex >= 0 && indexIncrement != 0)
            {
                focusedIndex += indexIncrement;
                auto const itemCount = itemsRepeater.ItemsSourceView().Count();
                while (focusedIndex >= 0 && focusedIndex < itemCount)
                {
                    if (auto const item = itemsRepeater.TryGetElement(focusedIndex))
                    {
                        if (auto const itemAsControl = item.try_as<winrt::IControl>())
                        {
                            if (itemAsControl.Focus(winrt::FocusState::Programmatic))
                            {
                                FocusElementAt(focusedIndex);
                                return true;
                            }
                        }
                    }
                    focusedIndex += indexIncrement;
                }
            }
        }
    }
    return false;
}

bool BreadcrumbBar::MoveFocusPrevious()
{
    int movementPrevious{ -1 };

    // If the focus is in the first visible item, then move to the ellipsis
    if (const auto& itemsRepeater = m_itemsRepeater.get())
    {
        const auto& repeaterLayout = itemsRepeater.Layout();
        if (m_itemsRepeaterLayout)
        {
            if (m_focusedIndex == 1)
            {
                movementPrevious = 0;
            }
            else if (m_itemsRepeaterLayout->EllipsisIsRendered() &&
                m_focusedIndex == static_cast<int>(m_itemsRepeaterLayout->FirstRenderedItemIndexAfterEllipsis()))
            {
                movementPrevious = -m_focusedIndex;
            }
        }
    }

    return MoveFocus(movementPrevious);
}

bool BreadcrumbBar::MoveFocusNext()
{
    int movementNext{ 1 };

    // If the focus is in the ellipsis, then move to the first visible item 
    if (m_focusedIndex == 0)
    {
        if (const auto& itemsRepeater = m_itemsRepeater.get())
        {
            const auto& repeaterLayout = itemsRepeater.Layout();
            if (m_itemsRepeaterLayout)
            {
                movementNext = m_itemsRepeaterLayout->FirstRenderedItemIndexAfterEllipsis();
            }
        }
    }

    return MoveFocus(movementNext);
}

winrt::FindNextElementOptions BreadcrumbBar::GetFindNextElementOptions()
{
    auto const& findNextElementOptions = winrt::FindNextElementOptions{};
    findNextElementOptions.SearchRoot(*this);
    return findNextElementOptions;
}

void BreadcrumbBar::OnChildPreviewKeyDown(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args)
{
    const bool flowDirectionIsLTR = (FlowDirection() == winrt::FlowDirection::LeftToRight);
    const bool keyIsLeft = (args.Key() == winrt::VirtualKey::Left);
    const bool keyIsRight = (args.Key() == winrt::VirtualKey::Right);

    // Moving to the next element
    if ((flowDirectionIsLTR && keyIsRight) || (!flowDirectionIsLTR && keyIsLeft))
    {
        if (MoveFocusNext())
        {
            args.Handled(true);
            return;
        }
        else if ((flowDirectionIsLTR && (args.OriginalKey() == winrt::VirtualKey::GamepadDPadRight)) ||
            (!flowDirectionIsLTR && (args.OriginalKey() == winrt::VirtualKey::GamepadDPadLeft)))
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Next))
            {
                args.Handled(true);
                return;
            }
        }
    }
    // Moving to previous element
    else if ((flowDirectionIsLTR && keyIsLeft) || (!flowDirectionIsLTR && keyIsRight))
    {
        if (MoveFocusPrevious())
        {
            args.Handled(true);
            return;
        }
        else if ((flowDirectionIsLTR && (args.OriginalKey() == winrt::VirtualKey::GamepadDPadLeft)) ||
            (!flowDirectionIsLTR && (args.OriginalKey() == winrt::VirtualKey::GamepadDPadRight)))
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Previous))
            {
                args.Handled(true);
                return;
            }
        }
    }
}

void BreadcrumbBar::OnAccessKeyInvoked(const winrt::UIElement&, const winrt::AccessKeyInvokedEventArgs& args)
{
    // If BreadcrumbBar is an AccessKeyScope then we do not want to handle the access
    // key invoked event because the user has (probably) set up access keys for the
    // BreadcrumbBarItem elements.
    if (!IsAccessKeyScope())
    {
        if (m_focusedIndex)
        {
            if (auto const itemsRepeater = m_itemsRepeater.get())
            {
                if (auto const selectedItem = itemsRepeater.TryGetElement(m_focusedIndex))
                {
                    if (auto const selectedItemAsControl = selectedItem.try_as<winrt::Control>())
                    {
                        args.Handled(selectedItemAsControl.Focus(winrt::FocusState::Programmatic));
                        return;
                    }
                }
            }
        }

        // If we don't have a selected index, focus the RadioButton's which under normal
        // circumstances will put focus on the first radio button.
        args.Handled(this->Focus(winrt::FocusState::Programmatic));
    }
}
