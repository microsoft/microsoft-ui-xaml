// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SetContact.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <DisableErrorReportingScopeGuard.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;
using namespace test_infra;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        bool SetContactTest::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            TestServices::WindowHelper->InjectMockDComp();
            return true;
        }

        bool SetContactTest::ClassCleanup()
        {
            TestServices::WindowHelper->DetachMockDComp();
            return true;
        }

        bool SetContactTest::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ SetContactTest::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dmanip\\";
        }

        FrameworkElement^ SetContactTest::SetupUI(
            _In_ Platform::String^ filename
            )
        {
            auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(filename));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            return root;
        }

        // Tests the most basic scenario of manually starting a pan manipulation
        void SetContactTest::Basics()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
            auto pointerMovedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerMoved);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();
            std::shared_ptr<Event> pointerMovedEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();

            Platform::String^ filename = GetResourcesPath() + L"SetContact.xaml";
            FrameworkElement^ root = SetupUI(filename);
            xaml_shapes::Rectangle^ rectangle = nullptr;
            ScrollViewer^ scrollViewer = nullptr;

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"myRectangle"));
                scrollViewer = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));
            });

            pointerPressedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
            {
                // Set ManipulationMode to None to prevent XAML from doing default manipulation.
                // This is when an app such as Office would typically turn off System ManipulationMode.
                rectangle->ManipulationMode = ManipulationModes::None;
                pointerPressedEvent->Set();
            }));

            Pointer^ cachedPointer;
            bool processedPointerMove = false;
            pointerMovedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
            {
                if (!processedPointerMove)
                {
                    // Now set ManipulationMode back to System and call TryStartDM to begin manipulation
                    processedPointerMove = true;
                    rectangle->ManipulationMode = ManipulationModes::System;
                    LOG_OUTPUT(L"Calling TryStartDirectManipulation");
                    VERIFY_IS_TRUE(UIElement::TryStartDirectManipulation(args->Pointer));
                    pointerMovedEvent->Set();
                    cachedPointer = args->Pointer;
                }
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged event fired.");
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            LOG_OUTPUT(L"Injecting input");
            for (int i = 0; i < 100; i++)
            {
                TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, -i, PointerFinger::Finger1);
            }

            LOG_OUTPUT(L"Waiting for pointerPressedEvent");
            pointerPressedEvent->WaitForDefault();
            LOG_OUTPUT(L"Waiting for pointerMovedEvent");
            pointerMovedEvent->WaitForDefault();
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
            LOG_OUTPUT(L"Waiting for viewChangedEvent");
            viewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(scrollViewer->VerticalOffset != 0.0);
            });

            RunOnUIThread([&]()
            {
                // Bonus test case that's convenient to do at this point:
                // After the manipulation is completed, verify that calling TryStartDM returns false.
                VERIFY_IS_FALSE(UIElement::TryStartDirectManipulation(cachedPointer));
            });
        }

        // Validates TryStartDM fails if used for mouse input
        void SetContactTest::Validation1()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
            std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

            Platform::String^ filename = GetResourcesPath() + L"SetContact.xaml";
            FrameworkElement^ root = SetupUI(filename);
            xaml_shapes::Rectangle^ rectangle = nullptr;
            ScrollViewer^ scrollViewer = nullptr;

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"myRectangle"));
                scrollViewer = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));
            });

            pointerPressedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"PointerPressed event fired.");
                DisableErrorReportingScopeGuard disableErrors;

                rectangle->ManipulationMode = ManipulationModes::None;

                // The expected behavior is XAML throws an InvalidArgumentException because Pointer has input type != Touch
                VERIFY_THROWS_WINRT(UIElement::TryStartDirectManipulation(args->Pointer), Platform::InvalidArgumentException^);
                pointerPressedEvent->Set();
            }));

            TestServices::InputHelper->ClickMouseButton(MouseButton::Left, scrollViewer);

            LOG_OUTPUT(L"Waiting for pointerPressedEvent.");
            pointerPressedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(scrollViewer->VerticalOffset == 0.0);
            });
        }

        // Validates that TryStartDM returns false if ManipulationMode doesn't contain System
        void SetContactTest::EdgeCase1()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
            std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

            Platform::String^ filename = GetResourcesPath() + L"SetContact.xaml";
            FrameworkElement^ root = SetupUI(filename);
            xaml_shapes::Rectangle^ rectangle = nullptr;
            ScrollViewer^ scrollViewer = nullptr;

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"myRectangle"));
                scrollViewer = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));
            });

            pointerPressedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"PointerPressed event fired.");

                // First turn off System ManipulationMode.
                rectangle->ManipulationMode = ManipulationModes::None;

                // Now when we try to call TryStartDM, XAML no-ops and returns false because System must be present.
                VERIFY_IS_FALSE(UIElement::TryStartDirectManipulation(args->Pointer));
                pointerPressedEvent->Set();
            }));

            for (int i = 0; i < 100; i++)
            {
                TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, -i, PointerFinger::Finger1);
            }
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);

            LOG_OUTPUT(L"Waiting for pointerPressed event.");
            pointerPressedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(scrollViewer->VerticalOffset == 0.0);
            });
        }

        // Validates that both the app and XAML can try to start a manipulation and the manipulation still happens without error
        void SetContactTest::EdgeCase2()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
            auto pointerMovedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerMoved);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();
            std::shared_ptr<Event> pointerMovedEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();

            Platform::String^ filename = GetResourcesPath() + L"SetContact.xaml";
            FrameworkElement^ root = SetupUI(filename);
            xaml_shapes::Rectangle^ rectangle = nullptr;
            ScrollViewer^ scrollViewer = nullptr;

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"myRectangle"));
                scrollViewer = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));
            });

            pointerPressedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"PointerPressed event fired.");

                // Since we're in the PointerPressed handler, XAML is about to take a crack at starting a
                // manipulation after this code returns.
                // But first we go ahead and call TryStartDM.  This will succeed.
                // Then after we return back to XAML it will try to SetContact and this will no-op gracefully.
                // This is similar to calling TryStartDM twice (as we do in EdgeCase3).
                VERIFY_IS_TRUE(UIElement::TryStartDirectManipulation(args->Pointer));
                pointerPressedEvent->Set();
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged event fired.");
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            for (int i = 0; i < 100; i++)
            {
                TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, -i, PointerFinger::Finger1);
            }

            LOG_OUTPUT(L"Waiting for pointerPressed event.");
            pointerPressedEvent->WaitForDefault();
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
            LOG_OUTPUT(L"Waiting for viewChanged event.");
            viewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(scrollViewer->VerticalOffset != 0.0);
            });
        }

        // Validates that calling TryStartDM multiple times manipulates correctly and gracefully no-ops
        void SetContactTest::EdgeCase3()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
            auto pointerMovedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerMoved);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();
            std::shared_ptr<Event> pointerMovedEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();

            Platform::String^ filename = GetResourcesPath() + L"SetContact.xaml";
            FrameworkElement^ root = SetupUI(filename);
            xaml_shapes::Rectangle^ rectangle = nullptr;
            ScrollViewer^ scrollViewer = nullptr;

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"myRectangle"));
                scrollViewer = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));
            });

            pointerPressedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"PointerPressed event fired.");
                rectangle->ManipulationMode = ManipulationModes::None;
                pointerPressedEvent->Set();
            }));

            bool processedPointerMove = false;

            pointerMovedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"PointerMoved event fired.");
                if (!processedPointerMove)
                {
                    processedPointerMove = true;

                    // Set ManipulationMode back to System and call TryStartDM two times in a row
                    rectangle->ManipulationMode = ManipulationModes::System;

                    // The first TryStartDM should succeed.
                    VERIFY_IS_TRUE(UIElement::TryStartDirectManipulation(args->Pointer));
                    {
                        // The second TryStartDM should no-op and return false.
                        // Note that the manipulation should still proceed just fine.
                        DisableErrorReportingScopeGuard disableErrors;
                        VERIFY_IS_FALSE(UIElement::TryStartDirectManipulation(args->Pointer));
                    }
                    pointerMovedEvent->Set();
                }
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged event fired.");
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            for (int i = 0; i < 100; i++)
            {
                TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, -i, PointerFinger::Finger1);
            }

            LOG_OUTPUT(L"Waiting for pointerPressed event.");
            pointerPressedEvent->WaitForDefault();
            LOG_OUTPUT(L"Waiting for pointerMoved event.");
            pointerMovedEvent->WaitForDefault();
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
            LOG_OUTPUT(L"Waiting for viewChanged event.");
            viewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(scrollViewer->VerticalOffset != 0.0);
            });
        }

        // Validates that removing a UIelement from the tree mid-manipulation does not crash
        void SetContactTest::EdgeCase4()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto pointerMovedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerMoved);
            std::shared_ptr<Event> pointerMovedEvent = std::make_shared<Event>();

            Platform::String^ filename = GetResourcesPath() + L"SetContact.xaml";
            FrameworkElement^ root = SetupUI(filename);
            xaml_shapes::Rectangle^ rectangle = nullptr;
            ScrollViewer^ scrollViewer = nullptr;

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"myRectangle"));
                rectangle->ManipulationMode = ManipulationModes::None;
                scrollViewer = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));
            });

            bool processedPointerMove = false;

            pointerMovedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"PointerMoved event fired.");
                if (!processedPointerMove)
                {
                    processedPointerMove = true;

                    // Remove the Rectangle from the live tree
                    scrollViewer->Content = nullptr;

                    pointerMovedEvent->Set();
                }
            }));

            for (int i = 0; i < 100; i++)
            {
                TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, -i, PointerFinger::Finger1);
            }

            LOG_OUTPUT(L"Waiting for pointerMoved event.");
            pointerMovedEvent->WaitForDefault();
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
            TestServices::WindowHelper->WaitForIdle();
        }

    } } }
} } } }
