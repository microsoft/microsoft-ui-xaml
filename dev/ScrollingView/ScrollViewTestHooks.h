// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollView.h"
#include "ScrollViewTestHooks.g.h"

class ScrollViewTestHooks :
    public winrt::implementation::ScrollViewTestHooksT<ScrollViewTestHooks>
{
public:
    static com_ptr<ScrollViewTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<ScrollViewTestHooks> EnsureGlobalTestHooks();

    static winrt::IReference<bool> GetAutoHideScrollControllers(const winrt::ScrollView& scrollView);
    static void SetAutoHideScrollControllers(const winrt::ScrollView& scrollView, winrt::IReference<bool> value);

    static winrt::ScrollPresenter GetScrollPresenterPart(const winrt::ScrollView& scrollView);

private:
    static com_ptr<ScrollViewTestHooks> s_testHooks;

    std::map<winrt::IScrollView, winrt::IReference<bool>> m_autoHideScrollControllersMap;
};
