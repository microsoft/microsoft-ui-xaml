// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NavigationViewItemsFactory.h"
#include "NavigationViewItemBase.h"

NavigationViewItemsFactory::NavigationViewItemsFactory()
{
}

winrt::UIElement NavigationViewItemsFactory::GetElementCore(winrt::ElementFactoryGetArgs const& args)
{
    winrt::IInspectable newContent = nullptr;
    // Attempt to get NavigationViewItemBase from user defined IElementFactory
    if (m_userElementFactory)
    {
        // Convert to args argument in the Windows::UI::Xaml namespace
        m_getArgsWUX.Data(args.Data());
        m_getArgsWUX.Parent(args.Parent());
        auto element = m_userElementFactory.GetElement(m_getArgsWUX);

        if (element.try_as<winrt::NavigationViewItemBase>())
        {
            return element;
        }

        newContent = element;
    }

    //Check whether the data is its own container
    if (auto data = args.Data().try_as<winrt::NavigationViewItemBase>())
    {
        return data;
    }

    // Create a wrapping container for the data
    auto nvi = winrt::make_self<NavigationViewItemBase>();
    nvi->Content(newContent);
    return *nvi;
}

void NavigationViewItemsFactory::RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args)
{
    if (m_userElementFactory)
    {
        // Convert to args argument in the Windows::UI::Xaml namespace
        m_recycleArgsWUX.Element(args.Element());
        m_recycleArgsWUX.Parent(args.Parent());
        m_userElementFactory.RecycleElement(m_recycleArgsWUX);
    }
}
