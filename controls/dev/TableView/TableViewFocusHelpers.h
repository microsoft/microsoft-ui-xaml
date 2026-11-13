// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace TableViewDetails
{
    inline winrt::Thickness OffsetContentPadding(winrt::Thickness padding, double offset)
    {
        offset = std::clamp(offset, -padding.Top, padding.Bottom);
        padding.Top += offset;
        padding.Bottom -= offset;
        return padding;
    }

    inline void SetRowChromeSeparatorInsets(
        winrt::FrameworkElement const& chrome, winrt::Thickness const& separator)
    {
        if (chrome)
        {
            winrt::Thickness const margin{ 0.0, separator.Top, 0.0, separator.Bottom };
            if (chrome.Margin() != margin)
            {
                chrome.Margin(margin);
            }
        }
    }

    inline double RoundLayoutValue(winrt::FrameworkElement const& element, double value)
    {
        const auto layoutValue = static_cast<float>(value);
        if (element.UseLayoutRounding())
        {
            if (auto const root = element.XamlRoot())
            {
                const auto scale = root.RasterizationScale();
                return static_cast<float>(std::floor(layoutValue * scale + 0.5) / scale);
            }
        }
        return layoutValue;
    }

    inline winrt::Thickness GetRowChromeSeparatorInsets(winrt::FrameworkElement const& chrome)
    {
        if (!chrome)
        {
            return {};
        }
        auto const margin = chrome.Margin();
        const auto top = RoundLayoutValue(chrome, margin.Top);
        // FrameworkElement rounds the total margin and the near-edge offset separately.
        const auto height = RoundLayoutValue(chrome, static_cast<float>(margin.Top) + static_cast<float>(margin.Bottom));
        return { 0.0, top, 0.0, height - top };
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
