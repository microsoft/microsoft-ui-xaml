// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs.g.h"

class ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs :
    public winrt::implementation::ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgsT<ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs>
{
public:
    ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs(bool isExpressionAnimationStarted, wstring_view const& propertyName);

    // IScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs overrides
    bool IsExpressionAnimationStarted();
    winrt::hstring PropertyName();

private:
    bool m_isExpressionAnimationStarted{ false };
    winrt::hstring m_propertyName{};
};
