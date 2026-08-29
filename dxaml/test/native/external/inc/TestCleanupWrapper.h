// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

class TestCleanupWrapper
{
public:
    TestCleanupWrapper()
    {
        m_cleanupFunc = []()
        {
            //
            // Tests that use TestCleanupWrapper and complete immediately can fail in WPF mode during test shutdown.
            // The TestCleanupWrapper will dispose of the content bridge, which closes the underlying hwnd in IXP.
            // Xaml then gets around to processing its WM_INTERNAL_TICK, which tries to reinitialize InputServices
            // and the DirectManipulation cross slide service, and hits E_INVALIDARG for using a closed hwnd.
            //
            // To work around this, make sure Xaml goes idle first, which finishes its initialization. We're then
            // free to tear down the tree.
            //
            // Note that this causes a problem with tests that want to end with Xaml in a perpetually non-idle
            // state, like playing an animation that loops forever. Those tests will have to explicitly put Xaml
            // in an idle state first if they want to use TestCleanupWrapper.
            //
            test_infra::TestServices::WindowHelper->WaitForIdle();

            test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        };
    }

    explicit TestCleanupWrapper(std::function<void(void)>&& cleanupFunc)
        : m_cleanupFunc(std::move(cleanupFunc))
    {
    }

    ~TestCleanupWrapper()
    {
        if (m_cleanupFunc != nullptr)
        {
            m_cleanupFunc();
        }
    }

    // Allow this. Used to create & return a TestCleanupWrapper from a helper method.
    TestCleanupWrapper(TestCleanupWrapper&& other) = default;

    // Disallow other copying/moving
    TestCleanupWrapper(const TestCleanupWrapper&) = delete;
    TestCleanupWrapper& operator=(const TestCleanupWrapper&) = delete;
    TestCleanupWrapper& operator=(TestCleanupWrapper&&) = delete;

private:
    std::function<void(void)> m_cleanupFunc;
};

} } } } }
