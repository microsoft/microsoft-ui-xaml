// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "ElementFactory.h"

class NavigationViewItemsFactory :
    public winrt::implements<NavigationViewItemsFactory, ElementFactory>
{
public:
    NavigationViewItemsFactory();

    void UserElementFactory(winrt::Windows::UI::Xaml::IElementFactory elementFactory) { m_userElementFactory = elementFactory; };

#pragma region IElementFactoryOverrides
    winrt::UIElement GetElementCore(winrt::ElementFactoryGetArgs const& args);
    void RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args);
#pragma endregion

private:
    winrt::Windows::UI::Xaml::IElementFactory m_userElementFactory{ nullptr };
    winrt::Windows::UI::Xaml::ElementFactoryGetArgs m_getArgsWUX{};
    winrt::Windows::UI::Xaml::ElementFactoryRecycleArgs m_recycleArgsWUX{};

};
