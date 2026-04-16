// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Generic {

    template<typename TClassUnderTest>
    class FrameworkElementTests
    {
    public:
        static void CanEnterAndLeaveLiveTree()
        {
            TestCleanupWrapper cleanup;
            TClassUnderTest^ element = nullptr;

            RunOnUIThread([&]()
            {
                element = ref new TClassUnderTest();
            });

            CanEnterAndLeaveLiveTree(element);
        }

        static void CanEnterAndLeaveLiveTree(TClassUnderTest^ existingInstance)
        {
            // We don't use TestCleanupWrapper here because the caller is holding a reference
            // on existingInstance which is going to be destroyed after this call is done.
            // The element will end up in the release queue. We want that queue to be empty by
            // the time a test finishes so it's the responsibility of the caller to place
            // a TestCleanupWrapper instance in the right scope.

            auto spHasLoadedEvent = std::make_shared<Event>();
            auto spHasUnloadedEvent = std::make_shared<Event>();

            auto loadedRegistration = CreateSafeEventRegistration(TClassUnderTest, Loaded);
            auto unloadedRegistration = CreateSafeEventRegistration(TClassUnderTest, Unloaded);

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(existingInstance, ref new xaml::RoutedEventHandler([spHasLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                    spHasLoadedEvent->Set();
                }));

                unloadedRegistration.Attach(existingInstance, ref new xaml::RoutedEventHandler([spHasUnloadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                    spHasUnloadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = existingInstance;
            });
            spHasLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"CanEnterAndLeaveLiveTree: Loaded.");

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            spHasUnloadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"CanEnterAndLeaveLiveTree: Unloaded.");
        }

    }; // class FrameworkElementTests

} } } } } // namespace Microsoft::UI::Xaml::Tests::Generic
