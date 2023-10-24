// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RepeaterTestHooksFactory.h"
#include "layout.h"

/* static */
int RepeaterTestHooks::s_elementFactoryElementIndex;

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
void RepeaterTestHooks::SetLayoutId(winrt::IInspectable const& layout, const hstring& id)
{
    if (auto instance = layout.as<Layout>())
    {
        instance->LayoutId(id);
    }
}


/* static */
void RepeaterTestHooks::SetElementFactoryElementIndex(int index)
{
    s_elementFactoryElementIndex = index;
}

/* static */
int RepeaterTestHooks::GetElementFactoryElementIndex()
{
    return s_elementFactoryElementIndex;
}
