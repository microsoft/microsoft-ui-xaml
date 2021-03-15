// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "ElementFactory.h"

class NavigationViewItemsFactory :
    public winrt::implements<NavigationViewItemsFactory, ElementFactory>
{
public:
    void UserElementFactory(winrt::IInspectable const& newValue);
    void SettingsItem(winrt::NavigationViewItemBase const& settingsItem);

    void IsFooterFactory(const bool value) { m_isFooterFactory = value; }

#pragma region IElementFactoryOverrides
    winrt::UIElement GetElementCore(winrt::ElementFactoryGetArgs const& args);
    void RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args);
#pragma endregion

private:
    winrt::IElementFactoryShim m_itemTemplateWrapper{ nullptr };
    winrt::NavigationViewItemBase m_settingsItem{ nullptr };
    std::vector<winrt::NavigationViewItem> navigationViewItemPool;

    bool m_isFooterFactory{ false };

    void UnlinkElementFromParent(winrt::ElementFactoryRecycleArgs const& args);
};
