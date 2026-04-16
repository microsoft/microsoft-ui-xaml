// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <FocusTestHelper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Text {

    template<typename TClassUnderTest>
    class TextBoxGenericTests
    {
    public:
        static void CanFireManipulationEvents()
        {
            TClassUnderTest^ boxUnderTest = nullptr;

            auto manipulationStartEvent = std::make_shared<Event>();
            auto manipulationStartEventRegistration = CreateSafeEventRegistration(TClassUnderTest, ManipulationStarted);

            auto manipulationCompletedEvent = std::make_shared<Event>();
            auto manipulationCompletedEventRegistration = CreateSafeEventRegistration(TClassUnderTest, ManipulationCompleted);

            RunOnUIThread([&]()
            {
                wf::Rect visibleBounds = TestServices::WindowHelper->VisibleBounds;

                boxUnderTest = ref new TClassUnderTest();
                boxUnderTest->ManipulationMode = xaml_input::ManipulationModes::All;
                boxUnderTest->Margin = Thickness({visibleBounds.X, visibleBounds.Y, 0, 0});

                manipulationStartEventRegistration.Attach(boxUnderTest, [&]() { manipulationStartEvent->Set(); });
                manipulationCompletedEventRegistration.Attach(boxUnderTest, [&]() { manipulationCompletedEvent->Set(); });

                TestServices::WindowHelper->WindowContent = boxUnderTest;
            });

            FocusTestHelper::EnsureFocus(boxUnderTest, FocusState::Pointer);
            TestServices::WindowHelper->WaitForIdle();

            // A drag should throw both events.
            LOG_OUTPUT(L"> Mouse down.");
            TestServices::InputHelper->MouseButtonDown(boxUnderTest, 50, 0, MouseButton::Left);
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            wf::Point point = {50, 50};
            TestServices::InputHelper->MoveMouse(point);
            manipulationStartEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            LOG_OUTPUT(L"> Mouse up.");
            TestServices::InputHelper->MouseButtonUp(boxUnderTest, 0, 0, MouseButton::Left);
            manipulationCompletedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }
    };
}}}}}}
