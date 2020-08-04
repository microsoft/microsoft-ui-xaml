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

#pragma region IElementFactoryOverrides
    winrt::UIElement GetElementCore(winrt::ElementFactoryGetArgs const& args);
    void RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args);
#pragma endregion

private:
    winrt::IElementFactoryShim m_itemTemplateWrapper{ nullptr };
    std::vector<winrt::NavigationViewItem> navigationViewItemPool;

    void UnlinkElementFromParent(winrt::ElementFactoryRecycleArgs const& args);
};
