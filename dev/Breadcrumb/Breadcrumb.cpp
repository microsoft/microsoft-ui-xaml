// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "Breadcrumb.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "BreadcrumbItem.h"
#include "BreadcrumbLayout.h"
#include "BreadcrumbItemClickedEventArgs.h"

Breadcrumb::Breadcrumb()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Breadcrumb);

    SetDefaultStyleKey(this);
    m_breadcrumbElementFactory = winrt::make_self<BreadcrumbElementFactory>();
    m_itemsIterable = winrt::make_self<BreadcrumbIterable>();
}

void Breadcrumb::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
    m_itemsRepeater.set(GetTemplateChildT<winrt::ItemsRepeater>(L"PART_BreadcrumbItemsRepeater", controlProtected));

    if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        thisAsIUIElement7.PreviewKeyDown({ this, &Breadcrumb::OnChildPreviewKeyDown });
    }

    AccessKeyInvoked({ this, &Breadcrumb::OnAccessKeyInvoked });
    GettingFocus({ this, &Breadcrumb::OnGettingFocus });

    if (const auto& breadcrumbItemsRepeater = m_itemsRepeater.get())
    {
        breadcrumbItemsRepeater.ItemsSource(winrt::make<Vector<IInspectable>>());
        breadcrumbItemsRepeater.ItemTemplate(*m_breadcrumbElementFactory);
        
        m_itemRepeaterElementPreparedRevoker = breadcrumbItemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &Breadcrumb::OnElementPreparedEvent });
        m_itemRepeaterElementIndexChangedRevoker = breadcrumbItemsRepeater.ElementIndexChanged(winrt::auto_revoke, { this, &Breadcrumb::OnElementIndexChangedEvent });
        m_itemRepeaterElementClearingRevoker = breadcrumbItemsRepeater.ElementClearing(winrt::auto_revoke, { this, &Breadcrumb::OnElementClearingEvent });

        m_itemsRepeaterLoadedRevoker = breadcrumbItemsRepeater.Loaded(winrt::auto_revoke, { this, &Breadcrumb::OnBreadcrumbItemRepeaterLoaded });
    }

    UpdateItemsSource();
}

void Breadcrumb::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const winrt::IDependencyProperty& property = args.Property();

    // TODO: Implement
    if (property == s_ItemsSourceProperty)
    {
        UpdateItemsSource();
    }
    else if (property == s_ItemTemplateProperty)
    {
        UpdateItemTemplate();
    }
    else if (property == s_DropdownItemTemplateProperty)
    {
        UpdateDropdownItemTemplate();
    }
}

void Breadcrumb::OnBreadcrumbItemRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (const auto& breadcrumbItemRepeater = m_itemsRepeater.get())
    {
        OnRepeaterCollectionChanged(nullptr, nullptr);
    }
}

void Breadcrumb::UpdateItemTemplate()
{
    const winrt::IInspectable& newItemTemplate = ItemTemplate();
    m_breadcrumbElementFactory->UserElementFactory(newItemTemplate);
}

void Breadcrumb::UpdateDropdownItemTemplate()
{
    const winrt::IInspectable& newItemTemplate = DropdownItemTemplate();

    // Copy the item template to the ellipsis button too
    if (const auto& ellipsisBreadcrumbItem = m_ellipsisBreadcrumbItem.get())
    {
        if (const auto& itemImpl = winrt::get_self<BreadcrumbItem>(ellipsisBreadcrumbItem))
        {
            itemImpl->SetFlyoutDataTemplate(newItemTemplate);
        }
    }
}

void Breadcrumb::UpdateItemsSource()
{
    m_itemsSourceChanged.revoke();
    m_itemsSourceChanged2.revoke();

    m_itemsRepeaterItemsSource = winrt::ItemsSourceView(ItemsSource());
    m_itemsSourceChanged = m_itemsRepeaterItemsSource.CollectionChanged(winrt::auto_revoke, { this, &Breadcrumb::OnRepeaterCollectionChanged });

    if (const auto& breadcrumbItemRepeater = m_itemsRepeater.get())
    {
        if (const auto& itemsSource = this->ItemsSource())
        {
            const auto& incc = [this, itemsSource]() {
                return itemsSource.try_as<winrt::INotifyCollectionChanged>();
            }();

            if (incc)
            {
                m_collectionChanged = incc.CollectionChanged({ this, &Breadcrumb::OnRepeaterCollectionChanged });
                m_notifyCollectionChanged.set(incc);
            }

            if (const auto& collection = itemsSource.try_as<winrt::IObservableVector<IInspectable>>())
            {
                m_itemsSourceChanged2 = collection.VectorChanged(winrt::auto_revoke, { this, &Breadcrumb::OnRepeaterCollectionChanged });
            }
        }
    }
}

void Breadcrumb::OnRepeaterCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable& args)
{
    if (const auto& itemsRepeater = m_itemsRepeater.get())
    {
        // A new BreadcrumbIterable must be created as ItemsRepeater compares if the previous
        // itemsSource is equals to the new one
        m_itemsIterable = winrt::make_self<BreadcrumbIterable>(ItemsSource());
        itemsRepeater.ItemsSource(*m_itemsIterable);
        itemsRepeater.UpdateLayout();

        // For some reason, when interacting with keyboard, the last element doesn't raise the OnPrepared event
        ForceUpdateLastElement();
    }
}

void Breadcrumb::ForceUpdateLastElement()
{
    const uint32_t itemCount = m_itemsRepeaterItemsSource.Count();

    if (const auto& itemsRepeater = m_itemsRepeater.get())
    {
        const auto& newLastItem = itemsRepeater.TryGetElement(itemCount).try_as<winrt::BreadcrumbItem>();
        UpdateLastElement(newLastItem);
    }
}

void Breadcrumb::UpdateLastElement(const winrt::BreadcrumbItem& newLastBreadcrumbItem)
{
    // If the element is the last element in the array,
    // then we reset the visual properties for the previous
    // last element
    if (const auto& lastItem = m_lastBreadcrumbItem.get())
    {
        auto lastItemImpl = winrt::get_self<BreadcrumbItem>(lastItem);
        lastItemImpl->ResetVisualProperties();
    }

    if (const auto& newLastItemImpl = winrt::get_self<BreadcrumbItem>(newLastBreadcrumbItem))
    {
        newLastItemImpl->SetPropertiesForLastNode();
        m_lastBreadcrumbItem.set(newLastBreadcrumbItem);
    }
}

void Breadcrumb::OnElementPreparedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs& args)
{
    if (const auto& item = args.Element().try_as<winrt::BreadcrumbItem>())
    {
        if (const auto& itemImpl = winrt::get_self<BreadcrumbItem>(item))
        {
            // The first element is always the ellipsis item
            itemImpl->SetItemsRepeater(*this);

            const uint32_t itemIndex = args.Index();
            if (itemIndex == 0)
            {
                itemImpl->SetPropertiesForEllipsisNode();
                itemImpl->SetFlyoutDataTemplate(DropdownItemTemplate());

                m_ellipsisBreadcrumbItem.set(item);
            }
            else
            {
                const uint32_t itemCount = m_itemsRepeaterItemsSource.Count();

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

void Breadcrumb::OnElementIndexChangedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    if (auto const element = args.Element())
    {
        element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(args.NewIndex() + 1));

        // When the selected item's index changes, update selection to match
        if (auto const elementAsToggle = element.try_as<winrt::ToggleButton>())
        {
            if (SharedHelpers::IsTrue(elementAsToggle.IsChecked()))
            {
                FocusElement(args.NewIndex());
            }
        }
    }
}

void Breadcrumb::OnElementClearingEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementClearingEventArgs& args)
{
    if (const auto& item = args.Element().try_as<winrt::BreadcrumbItem>())
    {
        const auto& itemImpl = winrt::get_self<BreadcrumbItem>(item);
        itemImpl->ResetVisualProperties();
        itemImpl->RevokeListeners();
    }
}

void Breadcrumb::RaiseItemClickedEvent(const winrt::IInspectable& content)
{
    const auto& eventArgs = winrt::make_self<BreadcrumbItemClickedEventArgs>();
    eventArgs->Item(content);

    if (m_itemClickedEventSource)
    {
        m_itemClickedEventSource(*this, *eventArgs);
    }
}

winrt::IVector<winrt::IInspectable> Breadcrumb::GetHiddenElementsList(uint32_t firstShownElement) const
{
    auto hiddenElements = winrt::make<Vector<winrt::IInspectable>>();
    for (uint32_t i = 0; i < firstShownElement - 1; ++i)
    {
        hiddenElements.Append(m_itemsRepeaterItemsSource.GetAt(i));
    }

    return hiddenElements;
}

winrt::IVector<winrt::IInspectable> Breadcrumb::HiddenElements() const
{
    // The hidden element list is generated in the BreadcrumbLayout during
    // the arrange method, so we retrieve the list from it
    if (const auto& itemsRepeater = m_itemsRepeater.get())
    {
        if (const auto& breadcrumbLayout = itemsRepeater.Layout().try_as<BreadcrumbLayout>())
        {
            if (breadcrumbLayout->EllipsisIsRendered())
            {
                return GetHiddenElementsList(breadcrumbLayout->FirstRenderedItemIndexAfterEllipsis());
            }
        }
    }

    // By default just return an empty list
    return winrt::make<Vector<winrt::IInspectable>>();
}

// When focus comes from outside the Breadcrumb control we will put focus on the selected item.
void Breadcrumb::OnGettingFocus(const winrt::IInspectable&, const winrt::GettingFocusEventArgs& args)
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
                // If the last focused element is now hidden, then focus the ellipsis button
                if (auto const& repeaterLayout = itemsRepeater.Layout())
                {
                    auto const& breadcrumbLayout = repeaterLayout.try_as<BreadcrumbLayout>();

                    if (breadcrumbLayout->EllipsisIsRendered() &&
                        m_focusedIndex < (int)breadcrumbLayout->FirstRenderedItemIndexAfterEllipsis())
                    {
                        FocusElement(0);
                    }
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

            // Focus was already in the repeater: in On RS3+ Selection follows focus unless control is held down.
            else if (SharedHelpers::IsRS3OrHigher() &&
                (winrt::Window::Current().CoreWindow().GetKeyState(winrt::VirtualKey::Control) &
                    winrt::CoreVirtualKeyStates::Down) != winrt::CoreVirtualKeyStates::Down)
            {
                if (auto const& newFocusedElementAsUIE = args.NewFocusedElement().as<winrt::UIElement>())
                {
                    FocusElement(itemsRepeater.GetElementIndex(newFocusedElementAsUIE));
                    args.Handled(true);
                }
            }
        }
    }
}

void Breadcrumb::FocusElement(int index)
{
    if (index >= 0)
    {
        m_focusedIndex = index;
    }
}

bool Breadcrumb::MoveFocus(int indexIncrement)
{
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        const auto& focusedElem = winrt::FocusManager::GetFocusedElement();

        if (auto const& focusedElement = focusedElem.try_as<winrt::UIElement>())
        {
            auto focusedIndex = itemsRepeater.GetElementIndex(focusedElement);

            if (focusedIndex >= 0)
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
                                FocusElement(focusedIndex);
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

bool Breadcrumb::MoveFocusPrevious()
{
    int movementPrevious{ -1 };

    // If the focus is in the first visible item, then move to the ellipsis
    if (const auto& itemsRepeater = m_itemsRepeater.get())
    {
        const auto& repeaterLayout = itemsRepeater.Layout();
        if (const auto& breadcrumbLayout = repeaterLayout.try_as<BreadcrumbLayout>())
        {
            if (breadcrumbLayout->EllipsisIsRendered() &&
                m_focusedIndex == static_cast<int>(breadcrumbLayout->FirstRenderedItemIndexAfterEllipsis()))
            {
                movementPrevious = -m_focusedIndex;
            }
        }
    }

    return MoveFocus(movementPrevious);
}

bool Breadcrumb::MoveFocusNext()
{
    int movementNext{ 1 };

    // If the focus is in the ellipsis, then move to the first visible item 
    if (m_focusedIndex == 0)
    {
        if (const auto& itemsRepeater = m_itemsRepeater.get())
        {
            const auto& repeaterLayout = itemsRepeater.Layout();
            if (const auto& breadcrumbLayout = repeaterLayout.try_as<BreadcrumbLayout>())
            {
                movementNext = breadcrumbLayout->FirstRenderedItemIndexAfterEllipsis();
            }
        }
    }

    return MoveFocus(movementNext);
}

// If we haven't handled the key yet and the original source was the first(for up and left)
// or last(for down and right) element in the repeater we need to handle the key so
// BreadcrumbItem doesn't, which would result in the behavior.
bool Breadcrumb::HandleEdgeCaseFocus(bool first, const winrt::IInspectable& source)
{
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        if (auto const& sourceAsUIElement = source.try_as<winrt::UIElement>())
        {
            auto const index = [first, itemsRepeater]()
            {
                if (first)
                {
                    return 0;
                }
                if (auto const& itemsSourceView = itemsRepeater.ItemsSourceView())
                {
                    return itemsSourceView.Count() - 1;
                }
                return -1;
            }();

            if (itemsRepeater.GetElementIndex(sourceAsUIElement) == index)
            {
                return true;
            }
        }
    }
    return false;
}

winrt::FindNextElementOptions Breadcrumb::GetFindNextElementOptions()
{
    auto const& findNextElementOptions = winrt::FindNextElementOptions{};
    findNextElementOptions.SearchRoot(*this);
    return findNextElementOptions;
}

void Breadcrumb::OnChildPreviewKeyDown(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args)
{
    switch (args.Key())
    {
    case winrt::VirtualKey::Right:
        if (MoveFocusNext())
        {
            args.Handled(true);
            return;
        }
        else if (args.OriginalKey() == winrt::VirtualKey::GamepadDPadRight)
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Next))
            {
                args.Handled(true);
                return;
            }
        }
        args.Handled(HandleEdgeCaseFocus(false, args.OriginalSource()));
        break;
    case winrt::VirtualKey::Left:
        if (MoveFocusPrevious())
        {
            args.Handled(true);
            return;
        }
        else if (args.OriginalKey() == winrt::VirtualKey::GamepadDPadLeft)
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Previous))
            {
                args.Handled(true);
                return;
            }
        }
        args.Handled(HandleEdgeCaseFocus(true, args.OriginalSource()));
        break;
    case winrt::VirtualKey::Down:
        if (args.OriginalKey() != winrt::VirtualKey::GamepadDPadDown)
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Right, GetFindNextElementOptions()))
            {
                args.Handled(true);
                return;
            }
        }
        else
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Right))
            {
                args.Handled(true);
                return;
            }
        }
        args.Handled(HandleEdgeCaseFocus(false, args.OriginalSource()));
        break;

    case winrt::VirtualKey::Up:
        if (args.OriginalKey() != winrt::VirtualKey::GamepadDPadUp)
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Left, GetFindNextElementOptions()))
            {
                args.Handled(true);
                return;
            }
        }
        else
        {
            if (winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Left))
            {
                args.Handled(true);
                return;
            }
        }
        args.Handled(HandleEdgeCaseFocus(true, args.OriginalSource()));
        break;
    }
}

void Breadcrumb::OnAccessKeyInvoked(const winrt::UIElement&, const winrt::AccessKeyInvokedEventArgs& args)
{
    // If Breadcrumb is an AccessKeyScope then we do not want to handle the access
    // key invoked event because the user has (probably) set up access keys for the
    // BreadcrumbItem elements.
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
