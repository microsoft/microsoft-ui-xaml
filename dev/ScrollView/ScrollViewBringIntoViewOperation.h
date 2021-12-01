// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

class ScrollViewBringIntoViewOperation
{
public:
    ScrollViewBringIntoViewOperation(winrt::UIElement const& targetElement);
    ~ScrollViewBringIntoViewOperation();

    bool HasMaxTicksCount() const;
    winrt::UIElement TargetElement() const;
    int8_t TicksCount() const;
    int8_t TickOperation();

private:
    // Number of UI thread ticks allowed before this expected bring-into-view operation is no
    // longer expected and removed from the ScrollView's m_bringIntoViewOperations list.
    static constexpr int8_t s_maxTicksCount{ 3 };

    int8_t m_ticksCount{ 0 };
    weak_ref<winrt::UIElement> m_targetElement;
};

