// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RepeaterTestHooksFactory.h"
#include "layout.h"
#ifdef BUILD_WINDOWS
#include "ElementFactoryGetArgsDownlevel.h"
#include "ElementFactoryRecycleArgsDownlevel.h"
#else
#include "ElementFactoryGetArgs.h"
#include "ElementFactoryRecycleArgs.h"
#endif


winrt::event_token RepeaterTestHooks::BuildTreeCompletedImpl(
    winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value)
{
    return m_buildTreeCompleted.add(value);
}

void RepeaterTestHooks::BuildTreeCompletedImpl(winrt::event_token const& token)
{
    m_buildTreeCompleted.remove(token);
}

void RepeaterTestHooks::NotifyBuildTreeCompletedImpl()
{
    m_buildTreeCompleted(nullptr, nullptr);
}

// We removed index parameter from the GetElement call, which we used extensively for 
// validation in tests. In order to avoid rewriting the tests, we keep the index internally and have 
// a test hook to get it for validation in tests.
/* static */
int RepeaterTestHooks::GetElementFactoryElementIndex(winrt::IInspectable const& getArgs)
{
#ifdef BUILD_WINDOWS
    auto args = getArgs.as<ElementFactoryGetArgsDownlevel>();
#else
    auto args = getArgs.as<ElementFactoryGetArgs>();
#endif
    return args->Index();
}

/* static */
winrt::IInspectable RepeaterTestHooks::CreateRepeaterElementFactoryGetArgs()
{
#ifdef BUILD_WINDOWS
    auto instance = winrt::make_self<ElementFactoryGetArgsDownlevel>();
#else
    auto instance = winrt::make_self<ElementFactoryGetArgs>();
#endif
    return *instance;
}

/* static */
winrt::IInspectable RepeaterTestHooks::CreateRepeaterElementFactoryRecycleArgs()
{
#ifdef BUILD_WINDOWS
    auto instance = winrt::make_self<ElementFactoryRecycleArgsDownlevel>();
#else
    auto instance = winrt::make_self<ElementFactoryRecycleArgs>();
#endif
    return *instance;
}

/* static */
hstring RepeaterTestHooks::GetLayoutId(winrt::IInspectable const& layout)
{
    if (auto instance = layout.as<Layout>())
    {
        return instance->LayoutId();
    }

    return L"";
}

/* static */
void RepeaterTestHooks::SetLayoutId(winrt::IInspectable const& layout, hstring id)
{
    if (auto instance = layout.as<Layout>())
    {
        instance->LayoutId(id);
    }
}