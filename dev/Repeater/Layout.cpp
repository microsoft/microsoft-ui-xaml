// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "Layout.h"
#include "VirtualizingLayoutContext.h"
#include "LayoutContextAdapter.h"

#pragma region ILayout

winrt::hstring Layout::LayoutId()
{
    return m_layoutId;
}

void Layout::LayoutId(winrt::hstring const& value)
{
    m_layoutId = value;
}

namespace
{
    winrt::VirtualizingLayoutContext GetVirtualizingLayoutContext(winrt::LayoutContext const& context)
    {
        if (auto virtualizingContext = context.try_as<winrt::VirtualizingLayoutContext>())
        {
            return virtualizingContext;
        }
        else if (auto nonVirtualizingContext = context.try_as<winrt::NonVirtualizingLayoutContext>())
        {
            auto adapter = winrt::get_self<NonVirtualizingLayoutContext>(nonVirtualizingContext)->GetVirtualizingContextAdapter();
            return adapter;
        }
        else
        {
            throw winrt::hresult_not_implemented();
        }
    }

    winrt::NonVirtualizingLayoutContext GetNonVirtualizingLayoutContext(winrt::LayoutContext const& context)
    {
        if (auto nonVirtualizingContext = context.try_as<winrt::NonVirtualizingLayoutContext>())
        {
            return nonVirtualizingContext;
        }
        else if (auto virtualizingContext = context.try_as<winrt::VirtualizingLayoutContext>())
        {
            auto adapter = winrt::get_self<VirtualizingLayoutContext>(virtualizingContext)->GetNonVirtualizingContextAdapter();
            return adapter;
        }
        else
        {
            throw winrt::hresult_not_implemented();
        }
    }
}


void Layout::InitializeForContext(winrt::LayoutContext const& context)
{
    auto spThis = get_strong();
    if (auto virtualizingLayout = spThis.try_as<winrt::IVirtualizingLayoutOverrides>())
    {
        auto virtualizingContext = GetVirtualizingLayoutContext(context);
        return virtualizingLayout.InitializeForContextCore(virtualizingContext);
    }
    else if (auto nonVirtualizingLayout = get_strong().try_as<winrt::INonVirtualizingLayoutOverrides>())
    {
        auto nonVirtualizingContext = GetNonVirtualizingLayoutContext(context);
        return nonVirtualizingLayout.InitializeForContextCore(nonVirtualizingContext);
    }
    else
    {
        throw winrt::hresult_not_implemented();
    }
}

void Layout::UninitializeForContext(winrt::LayoutContext const& context)
{
    auto spThis = get_strong();
    if (auto virtualizingLayout = spThis.try_as<winrt::IVirtualizingLayoutOverrides>())
    {
        auto virtualizingContext = GetVirtualizingLayoutContext(context);
        return virtualizingLayout.UninitializeForContextCore(virtualizingContext);
    }
    else if (auto nonVirtualizingLayout = spThis.try_as<winrt::INonVirtualizingLayoutOverrides>())
    {
        auto nonVirtualizingContext = GetNonVirtualizingLayoutContext(context);
        return nonVirtualizingLayout.UninitializeForContextCore(nonVirtualizingContext);
    }
    else
    {
        throw winrt::hresult_not_implemented();
    }
}

winrt::Size Layout::Measure(
    winrt::LayoutContext const& context,
    winrt::Size const& availableSize
    )
{
    auto spThis = get_strong();
    if (auto virtualizingLayout = spThis.try_as<winrt::IVirtualizingLayoutOverrides>())
    {
        auto virtualizingContext = GetVirtualizingLayoutContext(context);
        return virtualizingLayout.MeasureOverride(virtualizingContext, availableSize);
    }
    else if (auto nonVirtualizingLayout = spThis.try_as<winrt::INonVirtualizingLayoutOverrides>())
    {
        auto nonVirtualizingContext = GetNonVirtualizingLayoutContext(context);
        return nonVirtualizingLayout.MeasureOverride(nonVirtualizingContext, availableSize);
    }
    else
    {
        throw winrt::hresult_not_implemented();
    }
}

winrt::Size Layout::Arrange(
    winrt::LayoutContext const& context,
    winrt::Size const& finalSize)
{
    auto spThis = get_strong();
    if (auto virtualizingLayout = spThis.try_as<winrt::IVirtualizingLayoutOverrides>())
    {
        auto virtualizingContext = GetVirtualizingLayoutContext(context);
        return virtualizingLayout.ArrangeOverride(virtualizingContext, finalSize);
    }
    else if (auto nonVirtualizingLayout = spThis.try_as<winrt::INonVirtualizingLayoutOverrides>())
    {
        auto nonVirtualizingContext = GetNonVirtualizingLayoutContext(context);
        return nonVirtualizingLayout.ArrangeOverride(nonVirtualizingContext, finalSize);
    }
    else
    {
        throw winrt::hresult_not_implemented();
    }
}

winrt::event_token Layout::MeasureInvalidated(winrt::TypedEventHandler<winrt::Layout, winrt::IInspectable> const& value)
{
    return m_measureInvalidatedEventSource.add(value);
}

void Layout::MeasureInvalidated(winrt::event_token const& token)
{
    m_measureInvalidatedEventSource.remove(token);
}

winrt::event_token Layout::ArrangeInvalidated(winrt::TypedEventHandler<winrt::Layout, winrt::IInspectable> const& value)
{
    return m_arrangeInvalidatedEventSource.add(value);
}

void Layout::ArrangeInvalidated(winrt::event_token const& token)
{
    m_arrangeInvalidatedEventSource.remove(token);
}


#pragma endregion

#pragma region ILayoutProtected

void Layout::InvalidateMeasure()
{
    m_measureInvalidatedEventSource(*this, nullptr);
}

void Layout::InvalidateArrange()
{
    m_arrangeInvalidatedEventSource(*this, nullptr);
}

#pragma endregion
