// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerTestHooksExpressionAnimationStatusChangedEventArgs.g.h"

class ScrollerTestHooksExpressionAnimationStatusChangedEventArgs :
    public winrt::implementation::ScrollerTestHooksExpressionAnimationStatusChangedEventArgsT<ScrollerTestHooksExpressionAnimationStatusChangedEventArgs>
{
public:
    ScrollerTestHooksExpressionAnimationStatusChangedEventArgs(bool isExpressionAnimationStarted, wstring_view const& propertyName);

    // IScrollerTestHooksExpressionAnimationStatusChangedEventArgs overrides
    bool IsExpressionAnimationStarted();
    winrt::hstring PropertyName();

private:
    bool m_isExpressionAnimationStarted{ false };
    winrt::hstring m_propertyName{};
};
