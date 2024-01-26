// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "ItemContainer.h"
#include "SelectorBarItem.g.h"
#include "SelectorBarItem.properties.h"

class SelectorBarItem :
    public winrt::implementation::SelectorBarItemT<SelectorBarItem, ItemContainer>,
    public SelectorBarItemProperties
{
public:
    ForwardRefToBaseReferenceTracker(ItemContainer)

    SelectorBarItem();
    ~SelectorBarItem();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer() override;

    // IFrameworkElement
    void OnApplyTemplate() override;

    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // These functions are ambiguous with ItemContainer, disambiguate 
    using SelectorBarItemProperties::EnsureProperties;
    using SelectorBarItemProperties::ClearProperties;

private:
#ifdef DBG
    static winrt::hstring DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty);
#endif
    void UpdatePartsVisibility(bool isForIcon, bool isForText);

    tracker_ref<winrt::ContentPresenter> m_iconVisual{ this };
    tracker_ref<winrt::TextBlock> m_textVisual{ this };

    static constexpr std::wstring_view s_iconVisualPartName{ L"PART_IconVisual"sv };
    static constexpr std::wstring_view s_textVisualPartName{ L"PART_TextVisual"sv };
};
