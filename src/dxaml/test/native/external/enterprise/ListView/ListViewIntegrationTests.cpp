// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListViewIntegrationTests.h"
#include "StickyHeadersHelper.h"
#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>
#include <FocusTestHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ListView {

    //
    // Class & Test Setup
    //
    bool ListViewIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ListViewIntegrationTests::ClassCleanup()
    {
        return true;
    }

    bool ListViewIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ListViewIntegrationTests::AnimateItemIntoViewWithGamepad()
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPage;
            LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Root FE Loaded handler.");
                rootLoadedEvent->Set();
            }));

            listView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listView, 0));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                    args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                    args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                    args->IsInertial);
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
            }));

            listView->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving 10 items down, to the 11th item, with the gamepad.");
        for (int i = 0; i < 10; i++)
        {
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 124.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 10);
        });

        LOG_OUTPUT(L"Moving 10 times up, to the 1st item, with the gamepad.");
        for (int i = 0; i < 10; i++)
        {
            CommonInputHelper::Up(device);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
        });
    }

    void ListViewIntegrationTests::JumpItemIntoViewWithKeyboard()
    {
        JumpItemIntoViewWithKeyboard(false);
    }

    void ListViewIntegrationTests::JumpItemIntoViewWithKeyboardWithRootCanvas()
    {
        JumpItemIntoViewWithKeyboard(true);
    }

    void ListViewIntegrationTests::JumpItemIntoViewWithKeyboard(bool useRootCanvas) const
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;
        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        RunOnUIThread([&]()
        {
            if (useRootCanvas)
            {
                LOG_OUTPUT(L"Inserting a Canvas as the root element.");
                rootPage->Width = 400.0;
                rootPage->Height = 400.0;

                auto rootCanvas = ref new xaml_controls::Canvas();
                rootCanvas->Width = 400.0;
                rootCanvas->Height = 400.0;
                rootCanvas->Children->Append(rootPage);

                TestServices::WindowHelper->WindowContent = rootCanvas;

            }
            else
            {
                TestServices::WindowHelper->WindowContent = rootPage;
            }

            LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Root FE Loaded handler.");
                rootLoadedEvent->Set();
            }));

            listView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listView, 0));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                    args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                    args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                    args->IsInertial);
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
            }));

            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to first item.");
            listViewItem->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving 3 pages down.");
        for (int i = 0; i < 3; i++)
        {
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 784);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 25);
        });

        LOG_OUTPUT(L"Moving 3 pages up.");
        for (int i = 0; i < 3; i++)
        {
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
        });

        LOG_OUTPUT(L"Moving to the end.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 1952.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 52);
        });

        LOG_OUTPUT(L"Moving to the top.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_home#$u$_home");
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
        });
    }

    void ListViewIntegrationTests::AnimateZoomableItemIntoView()
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;
        xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ZoomableListView.xaml"));
        VERIFY_IS_NOT_NULL(rootGrid);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootGrid;
            LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
            rootLoadedRegistration.Attach(
                rootGrid,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Root FE Loaded handler.");
                rootLoadedEvent->Set();
            }));

            listView = safe_cast<xaml_controls::ListView^>(rootGrid->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listView, 0));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                    args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                    args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                    args->IsInertial);
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer, viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        RunOnUIThread([&]()
        {
            scrollViewer->ChangeView(nullptr, nullptr, 1.2f, true /*disableAnimation*/);
            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
        });

        LOG_OUTPUT(L"Waiting for ScrollViewer ViewChanged event.");
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to first item.");
            listViewItem->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving 8 items down, to the 9th item, with the gamepad.");
        for (int i = 0; i < 8; i++)
        {
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_IS_TRUE(scrollViewer->VerticalOffset >= 175.0);
            VERIFY_IS_TRUE(scrollViewer->VerticalOffset <= 176.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.2f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 8);
        });

        LOG_OUTPUT(L"Moving 8 times up, to the 1st item, with the gamepad.");
        for (int i = 0; i < 8; i++)
        {
            CommonInputHelper::Up(device);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Scrolling within the top margin, all the way to the top, with the gamepad.");
        for (int i = 0; i < 4; i++)
        {
            CommonInputHelper::Up(device);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.2f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
        });


        LOG_OUTPUT(L"Moving to the end.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving to bottom of margin.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_IS_TRUE(scrollViewer->VerticalOffset >= 2538.0);
            VERIFY_IS_TRUE(scrollViewer->VerticalOffset <= 2539.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.2f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 52);
        });

        LOG_OUTPUT(L"Moving to the top.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_home#$u$_home");
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 40.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.2f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
        });
    }

    void ListViewIntegrationTests::AnimateItemIntoViewProgrammatically()
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_data::CollectionViewSource^ cvs = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::IListViewBasePrivate^ listViewPrivate = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;
        xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GroupedListView.xaml"));
        VERIFY_IS_NOT_NULL(rootGrid);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootGrid;
            LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
            rootLoadedRegistration.Attach(
                rootGrid,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Root FE Loaded handler.");
                rootLoadedEvent->Set();
            }));

            listView = safe_cast<xaml_controls::ListView^>(rootGrid->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            listViewPrivate = dynamic_cast<xaml_controls::IListViewBasePrivate^>(listView);
            VERIFY_IS_NOT_NULL(listViewPrivate);

            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootGrid->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);

            itemsSource = GetGroupedData();
            VERIFY_IS_NOT_NULL(itemsSource);

            cvs->Source = itemsSource;
            cvs->IsSourceGrouped = true;
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listView, 0));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                    args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                    args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                    args->IsInertial);
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
            }));

            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to first item.");
            listViewItem->Focus(FocusState::Keyboard);

        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving to 2nd item.");
        CommonInputHelper::Down(device);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 1);

            LOG_OUTPUT(L"Animating to 10th item.");
            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(9));
            Platform::Object^ dataItem = listView->ItemFromContainer(listViewItem);
            listViewPrivate->ScrollIntoViewWithOptionalAnimation(dataItem, xaml_controls::ScrollIntoViewAlignment::Leading, false /*disableAnimation*/);
        });

        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 536.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 1);

            LOG_OUTPUT(L"Animating to footer.");
            listViewPrivate->ScrollIntoViewWithOptionalAnimation(listView->Footer, xaml_controls::ScrollIntoViewAlignment::Default, false /*disableAnimation*/);
        });

        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 1052.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 1);

            LOG_OUTPUT(L"Animating to header.");
            listViewPrivate->ScrollIntoViewWithOptionalAnimation(listView->Header, xaml_controls::ScrollIntoViewAlignment::Default, false /*disableAnimation*/);
        });

        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 1);

            LOG_OUTPUT(L"Animating to 4th group header.");
            auto group = safe_cast<xaml_data::ICollectionViewGroup^>(cvs->View->CollectionGroups->GetAt(3));
            VERIFY_IS_NOT_NULL(group);
            VERIFY_IS_NOT_NULL(group->Group);
            listViewPrivate->ScrollIntoViewWithOptionalAnimation(group->Group, xaml_controls::ScrollIntoViewAlignment::Leading, false /*disableAnimation*/);
        });

        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 1052.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 1);
        });
    }

    void ListViewIntegrationTests::BringItemIntoShrunkViewWithGamepad()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            TestServices::Utilities->ShrinkApplicationViewVisibleBounds(false);
        });

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        TestServices::Utilities->ShrinkApplicationViewVisibleBounds(true);

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPage;
            LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Root FE Loaded handler.");
                rootLoadedEvent->Set();
            }));

            listView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listView, 0));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                    args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                    args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                    args->IsInertial);
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
            }));

            listView->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving 11 items down, to the 12th item, with the gamepad.");
        for (int i = 0; i < 11; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 168.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 11);
        });

        LOG_OUTPUT(L"Moving 8 times up, to the 4th item, with the gamepad.");
        for (int i = 0; i < 8; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadUp();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_IS_TRUE(scrollViewer->VerticalOffset <= 82.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 3);
        });

        LOG_OUTPUT(L"Moving 3 times up, to the 1st item, with the gamepad.");
        for (int i = 0; i < 3; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadUp();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
        });
    }

    // Uses the gamepad up button to interrupt and reverse ScrollViewer's inertia.
    void ListViewIntegrationTests::StopInertialManipulationWithGamepad()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> inertiaEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListView.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Root FE Loaded handler.");
                rootLoadedEvent->Set();
            }));

            listView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            TestServices::WindowHelper->WindowContent = rootPage;
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listView, 0));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [inertiaEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                    args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                    args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                    args->IsInertial);
                if (args->IsInertial)
                {
                    inertiaEvent->Set();
                }
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer, viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            listView->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving 2 pages down.");
        for (int i = 0; i < 2; i++)
        {
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        LOG_OUTPUT(L"Waiting for ScrollViewer ViewChanged event.");
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 16);

            LOG_OUTPUT(L"Invoking ChangeView on ListView's ScrollViewer.");
            inertiaEvent->Reset();
            scrollViewer->ChangeView(nullptr, 800.0, nullptr);
        });

        // Let the ChangeView operation start in inertia mode.
        inertiaEvent->WaitForDefault();

        LOG_OUTPUT(L"Moving up once with the gamepad.");
        CommonInputHelper::Up(InputDevice::Gamepad);
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"ListView SelectedIndex is %d.", listView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 15);
        });
    }

    // Holds an item to bring up an empty context menu and then tries to pan the list with DManip.
    void ListViewIntegrationTests::CanOpenEmptyMenuFlyoutWhilePanningListView()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> holdingStartedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> flyoutOpenedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> flyoutClosedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto lviHoldingRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, Holding);
        auto directManipulationStartedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, DirectManipulationStarted);
        auto directManipulationCompletedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, DirectManipulationCompleted);
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        int directManipulationStartedCount = 0;
        int directManipulationCompletedCount = 0;

        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListViewWithDraggableItems.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPage;

            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Loaded raised.");
                rootLoadedEvent->Set();
            }));

            listView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            auto data = ref new Platform::Collections::Vector<Platform::String^>();
            data->Append("ListViewItem");

            listView->ItemsSource = data;
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(listViewItem);

            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listView, 0));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));

            directManipulationStartedRegistration.Attach(scrollViewer, ref new wf::EventHandler<Object^>(
                [&directManipulationStartedCount](Platform::Object^, Object^)
            {
                LOG_OUTPUT(L"DirectManipulationStarted raised.");
                directManipulationStartedCount++;
            }));

            directManipulationCompletedRegistration.Attach(scrollViewer, ref new wf::EventHandler<Object^>(
                [&directManipulationCompletedCount](Platform::Object^, Object^)
            {
                LOG_OUTPUT(L"DirectManipulationCompleted raised.");
                directManipulationCompletedCount++;
            }));

            lviHoldingRegistration.Attach(listViewItem, ref new xaml_input::HoldingEventHandler(
                [holdingStartedEvent](Platform::Object^, xaml_input::HoldingRoutedEventArgs^ args)
            {
                switch (args->HoldingState)
                {
                case Microsoft::UI::Input::HoldingState::Started:
                    LOG_OUTPUT(L"Holding HoldingState==Started raised.");
                    holdingStartedEvent->Set();
                    break;
                case Microsoft::UI::Input::HoldingState::Completed:
                    LOG_OUTPUT(L"Holding HoldingState==Completed raised.");
                    break;
                case Microsoft::UI::Input::HoldingState::Canceled:
                    LOG_OUTPUT(L"Holding HoldingState==Canceled raised.");
                    break;
                default:
                    VERIFY_FAIL(L"Unexpected value for HoldingState");
                    break;
                }
            }));
        });

        LOG_OUTPUT(L"Finger down on ListViewItem.");
        TestServices::InputHelper->DynamicPressCenter(listViewItem, 0, 0, PointerFinger::Finger1);
        LOG_OUTPUT(L"Waiting for ListViewItem Holding event with HoldingState==Started.");
        TestServices::WindowHelper->WaitForIdle();
        holdingStartedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Showing empty MenuFlyout.");
            menuFlyout = ref new xaml_controls::MenuFlyout();
            menuFlyout->Placement = xaml_primitives::FlyoutPlacementMode::Bottom;

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>(
                [flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"MenuFlyout Opened raised.");
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>(
                [flyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"MenuFlyout Closed raised.");
                flyoutClosedEvent->Set();
            }));

            menuFlyout->ShowAt(listViewItem);
        });

        LOG_OUTPUT(L"Waiting for MenuFlyout Opened event.");
        TestServices::WindowHelper->WaitForIdle();
        flyoutOpenedEvent->WaitForDefault();

        LOG_OUTPUT(L"Finger move on ListViewItem.");
        for (int i = 0; i < 10; i++)
        {
            TestServices::InputHelper->DynamicPressCenter(listViewItem, 0, 10 * i, PointerFinger::Finger1);
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Finger up from ListViewItem.");
        TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping ListViewItem to close MenuFlyout.");
        TestServices::InputHelper->Tap(listViewItem);
        LOG_OUTPUT(L"Waiting for MenuFlyout Closed event.");
        TestServices::WindowHelper->WaitForIdle();
        flyoutClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            // No DManip manipulation is expected to start
            VERIFY_ARE_EQUAL(directManipulationStartedCount, 0);

            // Expecting one DirectManipulationCompleted event when moving Finger1
            VERIFY_ARE_EQUAL(directManipulationCompletedCount, 1);
        });
    }

    void ListViewIntegrationTests::VerifyContainerFromItemAfterScrollIntoView()
    {
        TestCleanupWrapper cleanup;

        // Regression coverage for:
        // [Calculator] [XAML]: Narrator focus is easily decoupled from Keyboard focus when moving into and out of flyouts
        //
        // The root cause of this bug was an issue with container virtualization in ModernCollectionBasePanel.
        // ListView->GetContainerFromIndex and ListView->GetContainerFromItem were giving inconsistent results when
        // called immediately after ListView->ScrollIntoView

        xaml_controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            listView = ref new xaml_controls::ListView();
            listView->Height = 200;

            for (int i = 0; i < 50; i++)
            {
                listView->Items->Append(L"Item " + i);
            }

            TestServices::WindowHelper->WindowContent = listView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            unsigned index = 40;
            auto item = listView->Items->GetAt(index);

            // Calling ScrollIntoView should cause the container for the item to get realized:
            listView->ScrollIntoView(item);

            // We should be able to get the container using either the ContainerFromIndex or ContainerFromItem APIs.
            auto containerFromIndex = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(index));
            VERIFY_IS_NOT_NULL(containerFromIndex);

            auto containerFromItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromItem(item));
            VERIFY_IS_NOT_NULL(containerFromItem);

            VERIFY_IS_TRUE(containerFromIndex->Equals(containerFromItem));
        });
    }

    void ListViewIntegrationTests::VerifyTabOrderWithLocalTabNavigation()
    {
        Platform::String^ expectedFocusSequence = "[B][1][.1][2][.1][3][.1][A][.1][3][.1][2][.1][1][B]";

        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::Default, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::UseStackPanelAsItemsPanel, expectedFocusSequence);
    }

    void ListViewIntegrationTests::VerifyTabOrderWithCycleTabNavigation()
    {
        Platform::String^ expectedFocusSequence = "[B][1][.1][2][.1][3][.1][1][.1][3][.1][2][.1][1][.1]";

        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::Default, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::UseStackPanelAsItemsPanel, expectedFocusSequence);
    }

    void ListViewIntegrationTests::VerifyTabOrderWithOnceTabNavigation()
    {
        Platform::String^ expectedFocusSequence = "[B][1][.1][A][.1][1][B]";

        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::Default, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseStackPanelAsItemsPanel, expectedFocusSequence);
    }

    void ListViewIntegrationTests::VerifyTabOrderWithNestedButtons()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::UseNestedButtons, "[B][SI][one][SI][one][SI][one][A][one][SI][one][SI][one][SI][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::UseNestedButtons, "[B][SI][one][SI][one][SI][one][SI][one][SI][one][SI][one][SI][one]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseNestedButtons, "[B][SI][one][A][one][SI][B]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithLocalTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::UseHeaderAndFooter, "[B][H][1][.1][2][.1][3][.1][F][A][F][.1][3][.1][2][.1][1][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][1][.1][2][.1][3][.1][A][.1][3][.1][2][.1][1][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseStackPanelAsItemsPanel), "[B][H][1][.1][2][.1][3][.1][F][A][F][.1][3][.1][2][.1][1][H][B]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithCycleTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::UseHeaderAndFooter, "[B][H][1][.1][2][.1][3][.1][F][H][F][.1][3][.1][2][.1][1][H][F]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][1][.1][2][.1][3][.1][H][.1][3][.1][2][.1][1][H][.1]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseStackPanelAsItemsPanel), "[B][H][1][.1][2][.1][3][.1][F][H][F][.1][3][.1][2][.1][1][H][F]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithOnceTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseHeaderAndFooter, "[B][H][1][.1][F][A][F][.1][1][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][1][.1][A][.1][1][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseStackPanelAsItemsPanel), "[B][H][1][.1][F][A][F][.1][1][H][B]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithGroupedItemsAndLocalTabNavigation()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::UseGroupedItems, "[B][1][11][12][2][21][22][A][22][21][2][12][11][1][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][11][12][21][22][A][B][11][B][A][22][21][12][11][B]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithGroupedItemsAndCycleTabNavigation()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::UseGroupedItems, "[B][1][11][12][2][21][22][1][22][21][2][12][11][1][22]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][11][12][21][22][11][12][21][12][11][22][21][12][11][22]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithGroupedItemsAndOnceTabNavigation()
    {
        Platform::String^ expectedFocusSequence = "[B][11][A][B][A][11][B]";

        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseGroupedItems, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), expectedFocusSequence);
    }

    void ListViewIntegrationTests::VerifyTabOrderWithGroupedItemsLocalTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems), "[B][H][1][11][12][2][21][22][F][A][F][22][21][2][12][11][1][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][11][12][21][22][A][B][H][B][A][22][21][12][11][H][B]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithGroupedItemsCycleTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems), "[B][H][1][11][12][2][21][22][F][H][F][22][21][2][12][11][1][H][F]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][11][12][21][22][H][11][12][11][H][22][21][12][11][H][22]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithGroupedItemsOnceTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems), "[B][H][11][F][A][B][A][F][11][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][11][A][B][A][11][H][B]");
    }

    void ListViewIntegrationTests::VerifyTabOrderWithVirtualizedItemsAndLocalTabNavigation()
    {
        TestCleanupWrapper cleanup;

        Platform::String^ focusSequence = "";

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto rootGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);

        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListViewForTabNavigation.xaml"));
        xaml_controls::Button^ beforeButton = nullptr;
        xaml_controls::Button^ afterButton = nullptr;

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPage;

            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler([rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Loaded raised.");
                    rootLoadedEvent->Set();
                }));

            rootGotFocusRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler([&focusSequence](Platform::Object^, xaml::RoutedEventArgs^ args)
                {
                    xaml::FrameworkElement^ originalSourceFE = safe_cast<xaml::FrameworkElement^>(args->OriginalSource);
                    VERIFY_IS_NOT_NULL(originalSourceFE);

                    Platform::Object^ tag = originalSourceFE->Tag;

                    if (tag != nullptr)
                    {
                        focusSequence += "[" + tag + "]";
                    }
                }));

            beforeButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"BeforeButton"));
            VERIFY_IS_NOT_NULL(beforeButton);

            afterButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"AfterButton"));
            VERIFY_IS_NOT_NULL(afterButton);

            xaml_controls::ListView^ mainVirtualizedListView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"MainVirtualizedListView"));
            VERIFY_IS_NOT_NULL(mainVirtualizedListView);

            mainVirtualizedListView->Visibility = xaml::Visibility::Visible;
        });

        LOG_OUTPUT(L"Waiting for content to load.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Giving focus to AfterButton.");
        FocusTestHelper::EnsureFocus(afterButton, xaml::FocusState::Keyboard);

        LOG_OUTPUT(L"Pressing shift-tab twice.");
        TestServices::KeyboardHelper->ShiftTab();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->ShiftTab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing tab twice.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Giving focus to the BeforeButton.");
        FocusTestHelper::EnsureFocus(beforeButton, xaml::FocusState::Keyboard);

        LOG_OUTPUT(L"Pressing tab twice.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing shift-tab 5 times.");
        for (int tabOccurrence = 0; tabOccurrence < 5; tabOccurrence++)
        {
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();
        }

        Platform::String^ expectedFocusSequence = "[B][A][.1][15][.1][A][B][1][.1][1][B][A][.1][15]";

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence:   %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);
    }

    void ListViewIntegrationTests::VerifyTabOrderWithVirtualizedItemsAndCycleTabNavigation()
    {
        TestCleanupWrapper cleanup;

        Platform::String^ focusSequence = "";

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto rootGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);

        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ListViewForTabNavigation.xaml"));
        xaml_controls::Button^ beforeButton = nullptr;
        xaml_controls::Button^ afterButton = nullptr;

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPage;

            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler([rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Loaded raised.");
                    rootLoadedEvent->Set();
                }));

            rootGotFocusRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler([&focusSequence](Platform::Object^, xaml::RoutedEventArgs^ args)
                {
                    xaml::FrameworkElement^ originalSourceFE = safe_cast<xaml::FrameworkElement^>(args->OriginalSource);
                    VERIFY_IS_NOT_NULL(originalSourceFE);

                    Platform::Object^ tag = originalSourceFE->Tag;

                    if (tag != nullptr)
                    {
                        focusSequence += "[" + tag + "]";
                    }
                }));

            beforeButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"BeforeButton"));
            VERIFY_IS_NOT_NULL(beforeButton);

            afterButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"AfterButton"));
            VERIFY_IS_NOT_NULL(afterButton);

            xaml_controls::ListView^ mainVirtualizedListView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"MainVirtualizedListView"));
            VERIFY_IS_NOT_NULL(mainVirtualizedListView);

            mainVirtualizedListView->Visibility = xaml::Visibility::Visible;
            mainVirtualizedListView->TabNavigation = xaml_input::KeyboardNavigationMode::Cycle;
        });

        LOG_OUTPUT(L"Waiting for content to load.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Giving focus to AfterButton.");
        FocusTestHelper::EnsureFocus(afterButton, xaml::FocusState::Keyboard);

        LOG_OUTPUT(L"Pressing shift-tab twice.");
        TestServices::KeyboardHelper->ShiftTab();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->ShiftTab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing tab 5 times.");
        for (int tabOccurrence = 0; tabOccurrence < 5; tabOccurrence++)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Pressing shift-tab 5 times.");
        for (int tabOccurrence = 0; tabOccurrence < 5; tabOccurrence++)
        {
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();
        }

        Platform::String^ expectedFocusSequence = "[B][A][.1][15][.1][1][.1][2][.1][2][.1][1][.1][15]";

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence:   %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);
    }

    void ListViewIntegrationTests::VerifyTabOrder(
        xaml_input::KeyboardNavigationMode tabNavigation,
        VerifyTabOrderFlags flags,
        Platform::String^ expectedFocusSequence)
    {
        TestCleanupWrapper cleanup;

        Platform::String^ focusSequence = "";

        xaml_data::CollectionViewSource^ cvs = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto rootGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);

        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + ((flags & VerifyTabOrderFlags::UseGroupedItems) ? L"GroupedListViewForTabNavigation.xaml" : L"ListViewForTabNavigation.xaml")));
        xaml_controls::Button^ beforeButton = nullptr;
        xaml_controls::ListView^ mainListView = nullptr;

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPage;

            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler([rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Loaded raised.");
                    rootLoadedEvent->Set();
                }));

            rootGotFocusRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler([&focusSequence](Platform::Object^, xaml::RoutedEventArgs^ args)
                {
                    xaml::FrameworkElement^ originalSourceFE = safe_cast<xaml::FrameworkElement^>(args->OriginalSource);
                    VERIFY_IS_NOT_NULL(originalSourceFE);

                    Platform::Object^ tag = originalSourceFE->Tag;

                    if (tag == nullptr)
                    {
                        xaml_primitives::SelectorItem^ selectorItem = dynamic_cast<xaml_primitives::SelectorItem^>(originalSourceFE);

                        if (selectorItem == nullptr)
                        {
                            xaml_controls::ListViewHeaderItem^ listViewHeaderItem = safe_cast<xaml_controls::ListViewHeaderItem^>(originalSourceFE);
                            VERIFY_IS_NOT_NULL(listViewHeaderItem);
                            VERIFY_IS_NOT_NULL(listViewHeaderItem->Content);

                            Microsoft::UI::Xaml::Tests::Common::GroupedHeader^ groupHeader = safe_cast<Microsoft::UI::Xaml::Tests::Common::GroupedHeader^>(listViewHeaderItem->Content);
                            VERIFY_IS_NOT_NULL(groupHeader);

                            focusSequence += "[" + groupHeader->Header + "]";
                        }
                        else
                        {
                            focusSequence += "[SI]";
                        }
                    }
                    else
                    {
                        focusSequence += "[" + tag + "]";
                    }
                }));

            beforeButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"BeforeButton"));
            VERIFY_IS_NOT_NULL(beforeButton);

            if (flags & VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel)
            {
                mainListView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"MainListViewWithVirtualizingStackPanel"));
            }
            else if (flags & VerifyTabOrderFlags::UseStackPanelAsItemsPanel)
            {
                mainListView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"MainListViewWithStackPanel"));
            }
            else if (flags & VerifyTabOrderFlags::UseNestedButtons)
            {
                mainListView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"MainListViewWithNestedButtons"));
            }
            else
            {
                mainListView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"MainListView"));
            }
            VERIFY_IS_NOT_NULL(mainListView);

            mainListView->Visibility = xaml::Visibility::Visible;
            mainListView->TabNavigation = tabNavigation;

            if (flags & VerifyTabOrderFlags::UseHeaderAndFooter)
            {
                safe_cast<xaml_controls::Button^>(mainListView->Header)->Visibility = xaml::Visibility::Visible;

                // VirtualizingStackPanel does not support footer.
                if (!(flags & VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel))
                {
                    safe_cast<xaml_controls::Button^>(mainListView->Footer)->Visibility = xaml::Visibility::Visible;
                }
            }

            if (flags & VerifyTabOrderFlags::UseGroupedItems)
            {
                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPage->FindName(L"cvs"));
                VERIFY_IS_NOT_NULL(cvs);

                itemsSource = GetGroupedData(false /*getLargeDataSet*/);
                VERIFY_IS_NOT_NULL(itemsSource);

                cvs->Source = itemsSource;
                cvs->IsSourceGrouped = true;
            }
        });

        LOG_OUTPUT(L"Waiting for content to load.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Giving focus to the initial button.");
        FocusTestHelper::EnsureFocus(beforeButton, xaml::FocusState::Keyboard);

        int tabCount = xaml_input::KeyboardNavigationMode::Once == tabNavigation ? 3 : 7;

        if (flags & VerifyTabOrderFlags::UseHeaderAndFooter)
        {
            tabCount++;

            if (!(flags & VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel))
            {
                tabCount++;
            }
        }

        LOG_OUTPUT(L"Pressing tab %d times.", tabCount);
        for (int tabOccurrence = 0; tabOccurrence < tabCount; tabOccurrence++)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Pressing shift-tab %d times.", tabCount);
        for (int tabOccurrence = 0; tabOccurrence < tabCount; tabOccurrence++)
        {
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();
        }

        Platform::String^ logMessage = "Scenario: ItemsPanel=";

        if (flags & VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel)
        {
            logMessage += "VirtualizingStackPanel";
        }
        else if (flags & VerifyTabOrderFlags::UseStackPanelAsItemsPanel)
        {
            logMessage += "StackPanel";
        }
        else 
        {
            logMessage += "ItemsWrapPanel";
        }

        if (flags & VerifyTabOrderFlags::UseHeaderAndFooter)
        {
            if (flags & VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel)
            {
                logMessage += ", with Header";
            }
            else 
            {
                logMessage += ", with Header+Footer";
            }
        }

        if (flags & VerifyTabOrderFlags::UseGroupedItems)
        {
            logMessage += ", with groups";
        }

        LOG_OUTPUT(logMessage->Data());
        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence:   %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);
    }

    void ListViewIntegrationTests::VerifyFocusChangeAfterItemsReset()
    {
        VerifyFocusChangeAfterListViewReset(false /*useItemsSource*/);
    }

    void ListViewIntegrationTests::VerifyFocusChangeAfterItemsSourceReset()
    {
        VerifyFocusChangeAfterListViewReset(true /*useItemsSource*/);
    }

    void ListViewIntegrationTests::BringItemIntoViewWithLargeGlobalScale()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(400, 400), 1.5f);

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::ListView^ outerListView = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;
        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"NestedListViewBase.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPage;
            LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"Root FE Loaded handler.");
                rootLoadedEvent->Set();
            }));

            outerListView = safe_cast<xaml_controls::ListView^>(rootPage->FindName(L"outerListView"));
            VERIFY_IS_NOT_NULL(outerListView);
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(outerListView, 0));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                    args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                    args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                    args->IsInertial);
            }));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
            }));

            outerListView->Focus(FocusState::Keyboard);

            listViewItem = safe_cast<xaml_controls::ListViewItem^>(outerListView->ContainerFromIndex(0));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to first item.");
            listViewItem->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tabbing 4 times to the 5th outer ListView item.");
        for (int i = 0; i < 4; i++)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            });
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 426.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(outerListView->SelectedIndex, -1);
        });
    }

    //
    // Private Methods
    //
    Platform::String^ ListViewIntegrationTests::GetResourcesPath() const
    {
        return GetPackageFolder() + L"resources\\native\\enterprise\\listviewbase\\";
    }

    Platform::Collections::Vector<Platform::Object^>^ ListViewIntegrationTests::GetGroupedData(bool getLargeDataSet) const
    {
        auto groupedData = ref new Platform::Collections::Vector<Platform::Object^>();

        if (getLargeDataSet)
        {
            for (unsigned int i = 0; i < 4; ++i)
            {
                Microsoft::UI::Xaml::Tests::Common::GroupedHeader^ group = nullptr;

                switch (i)
                {
                case 0:
                    group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Tennis");
                    VERIFY_IS_NOT_NULL(group);

                    group->Append(L"Roger Federer");
                    group->Append(L"Rafael Nadal");
                    group->Append(L"Novak Djokovic");
                    group->Append(L"Andy Murray");
                    group->Append(L"Grigor Dimitrov");
                    break;

                case 1:
                    group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Soccer");
                    VERIFY_IS_NOT_NULL(group);

                    group->Append(L"Cristiano Ronaldo");
                    group->Append(L"Lionel Messi");
                    group->Append(L"Neymar");
                    group->Append(L"Andres Iniesta");
                    group->Append(L"Gareth Bale");
                    group->Append(L"Xavi");
                    group->Append(L"James Rodriguez");
                    group->Append(L"Ronaldinho");
                    group->Append(L"Arjen Robben");
                    break;

                case 2:
                    group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Basketball");
                    VERIFY_IS_NOT_NULL(group);

                    group->Append(L"Allen Iverson");
                    group->Append(L"Dwayne Wade");
                    group->Append(L"LeBron James");
                    group->Append(L"Kevin Durant");
                    group->Append(L"Kobe Bryant");
                    break;

                case 3:
                    group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Formula 1");
                    VERIFY_IS_NOT_NULL(group);

                    group->Append(L"Alain Prost");
                    group->Append(L"Ayrton Senna");
                    group->Append(L"Michael Schumacher");
                    group->Append(L"Niki Lauda");
                    group->Append(L"Sebastian Vettel");
                    break;
                }

                groupedData->Append(group);
            }

            return groupedData;
        }

        Microsoft::UI::Xaml::Tests::Common::GroupedHeader^ group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"1");
        group->Append(L"11");
        group->Append(L"12");
        groupedData->Append(group);

        group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"2");
        group->Append(L"21");
        group->Append(L"22");
        groupedData->Append(group);

        return groupedData;
    }

    void ListViewIntegrationTests::VerifyFocusChangeAfterListViewReset(bool useItemsSource)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ stackPanel = nullptr;
        xaml_controls::Button^ beforeButton = nullptr;
        xaml_controls::Button^ afterButton = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        wfc::IObservableVector<Platform::String^>^ _observableVectorOfStrings = nullptr;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Building UI with ListView and Before/After buttons.");

            beforeButton = ref new xaml_controls::Button();
            beforeButton->Content = "Before";

            afterButton = ref new xaml_controls::Button();
            afterButton->Content = "After";

            listView = ref new xaml_controls::ListView();

            if (useItemsSource)
            {
                _observableVectorOfStrings = ref new Platform::Collections::Vector<Platform::String^>();
                _observableVectorOfStrings->Append(ref new Platform::String(L"One"));
                _observableVectorOfStrings->Append(ref new Platform::String(L"Two"));
                _observableVectorOfStrings->Append(ref new Platform::String(L"Three"));
                listView->ItemsSource = _observableVectorOfStrings;
            }
            else
            {
                for (int i = 0; i < 5; i++)
                {
                    listView->Items->Append(L"Item " + i);
                }
            }

            stackPanel = ref new xaml_controls::StackPanel;
            stackPanel->Children->Append(beforeButton);
            stackPanel->Children->Append(listView);
            stackPanel->Children->Append(afterButton);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to Before button.");
            beforeButton->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (useItemsSource)
            {
                LOG_OUTPUT(L"Clearing ListView.ItemsSource collection.");
                _observableVectorOfStrings->Clear();
            }
            else
            {
                LOG_OUTPUT(L"Clearing ListView.Items collection.");
                listView->Items->Clear();
            }

            LOG_OUTPUT(L"Programmatically attempt to focus the next focusable element.");
            xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions;
            options->SearchRoot = TestServices::WindowHelper->WindowContent;
            bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(xaml_input::FocusNavigationDirection::Next, options);
            VERIFY_IS_TRUE(resultTryMoveFocus);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying focus landed on After button.");
            auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_IS_NOT_NULL(focusedElement);
            VERIFY_IS_TRUE(focusedElement->Equals(afterButton));
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::ListView