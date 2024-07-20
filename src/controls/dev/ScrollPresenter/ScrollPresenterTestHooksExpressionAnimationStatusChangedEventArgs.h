// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs.g.h"

class ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs :
    public winrt::implementation::ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgsT<ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs>
{
public:
    ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs(bool isExpressionAnimationStarted, wstring_view const& propertyName);

    // IScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs overrides
    bool IsExpressionAnimationStarted();
    winrt::hstring PropertyName();

private:
    bool m_isExpressionAnimationStarted{ false };
    winrt::hstring m_propertyName{};
};
