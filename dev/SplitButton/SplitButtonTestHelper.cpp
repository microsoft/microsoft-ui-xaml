// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SharedHelpers.h"
#include "SplitButtonTestHelper.h"
#include "common.h"

thread_local com_ptr<SplitButtonTestHelper> SplitButtonTestHelper::s_instance;

com_ptr<SplitButtonTestHelper> SplitButtonTestHelper::EnsureInstance()
{
    if (!s_instance)
    {
        s_instance = winrt::make_self<SplitButtonTestHelper>();
    }

    return s_instance;
}

void SplitButtonTestHelper::SimulateTouch(bool value)
{
    auto instance = EnsureInstance();
    instance->m_simulateTouch = value;
}

bool SplitButtonTestHelper::SimulateTouch()
{
    auto instance = EnsureInstance();
    return instance->m_simulateTouch;
}
