// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs.h"

ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs::ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs(
    bool isExpressionAnimationStarted, wstring_view const& propertyName)
{
    m_isExpressionAnimationStarted = isExpressionAnimationStarted;
    m_propertyName = propertyName.data();
}

#pragma region IScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs

bool ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs::IsExpressionAnimationStarted()
{
    return m_isExpressionAnimationStarted;
}

winrt::hstring ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs::PropertyName()
{
    return m_propertyName;
}

#pragma endregion
