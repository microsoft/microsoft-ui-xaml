// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class SplitButtonTestHelper :
    public winrt::implements<SplitButtonTestHelper, winrt::IInspectable>
{
public:
    SplitButtonTestHelper() = default;
    ~SplitButtonTestHelper() = default;

    static void SimulateTouch(bool value);
    static bool SimulateTouch();

private:
    static com_ptr<SplitButtonTestHelper> EnsureInstance();

    bool m_simulateTouch{ false };

    static thread_local com_ptr<SplitButtonTestHelper> s_instance;
};
