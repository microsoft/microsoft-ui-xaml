// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace TableViewDetails
{
    inline bool IsKeyDown(winrt::Windows::System::VirtualKey key)
    {
        return (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(key) &
            winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
    }

    // One step up that crosses popup boundaries. A popup's content (a ComboBox dropdown, a flyout)
    // is not a visual descendant of the element that owns it, so a plain VisualTreeHelper walk from
    // a dropdown item never reaches the editor that opened it.
    inline winrt::DependencyObject GetPopupAwareParent(
        winrt::DependencyObject const& element,
        winrt::IVectorView<winrt::Popup> const& openPopups)
    {
        if (auto const item = element.try_as<winrt::SelectorItem>())
        {
            if (auto const itemsControl = winrt::ItemsControl::ItemsControlFromItemContainer(item))
            {
                return itemsControl;
            }
        }

        if (auto const frameworkElement = element.try_as<winrt::FrameworkElement>())
        {
            if (auto const popup = frameworkElement.Parent().try_as<winrt::Popup>())
            {
                return popup;
            }
        }

        if (openPopups)
        {
            for (auto const& popup : openPopups)
            {
                if (popup && popup.Child() == element)
                {
                    return popup;
                }
            }
        }

        auto parent = winrt::VisualTreeHelper::GetParent(element);
        if (!parent)
        {
            if (auto const popup = element.try_as<winrt::Popup>())
            {
                parent = popup.PlacementTarget();
            }
        }
        return parent;
    }

    // True when `candidate` is `ancestor` or sits underneath it, including inside popups the
    // ancestor's content owns. The plain visual walk runs first so the common case stays cheap.
    inline bool IsWithinElementPopupAware(
        winrt::IInspectable const& candidate,
        winrt::DependencyObject const& ancestor,
        winrt::XamlRoot const& xamlRoot)
    {
        if (!ancestor)
        {
            return false;
        }

        auto const start = candidate.try_as<winrt::DependencyObject>();
        for (auto current = start; current; current = winrt::VisualTreeHelper::GetParent(current))
        {
            if (current == ancestor)
            {
                return true;
            }
        }

        winrt::IVectorView<winrt::Popup> openPopups{ nullptr };
        if (xamlRoot)
        {
            try
            {
                openPopups = winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(xamlRoot);
            }
            catch (...)
            {
                // Best-effort: item containers and popup parents are still crossed without it.
            }
        }

        // Bounded: a popup's PlacementTarget could in principle lead back into its own content.
        auto current = start;
        for (int depth = 0; current && depth < 1024; ++depth)
        {
            if (current == ancestor)
            {
                return true;
            }
            current = GetPopupAwareParent(current, openPopups);
        }
        return false;
    }

    // Closes any dropdown open inside `root`. An editor removed with its dropdown open (Enter both
    // opens a closed ComboBox and commits the edit) leaves the dropdown to restore focus to an
    // element that is no longer in the tree, and focus falls back to an arbitrary element.
    inline void CloseOpenDropDowns(winrt::DependencyObject const& root, int depth = 0)
    {
        if (!root || depth > 32)
        {
            return;
        }

        if (auto const comboBox = root.try_as<winrt::ComboBox>())
        {
            if (comboBox.IsDropDownOpen())
            {
                comboBox.IsDropDownOpen(false);
            }
            return;
        }

        const int32_t count = winrt::VisualTreeHelper::GetChildrenCount(root);
        for (int32_t i = 0; i < count; ++i)
        {
            CloseOpenDropDowns(winrt::VisualTreeHelper::GetChild(root, i), depth + 1);
        }
    }
}
