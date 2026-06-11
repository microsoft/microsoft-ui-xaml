// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollViewerInFlipViewTests.h"
#include <XamlTailored.h>
#include <TreeHelper.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ScrollViewer {

    bool ScrollViewerInFlipViewTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ScrollViewerInFlipViewTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ScrollViewerInFlipViewTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    // Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change.
    // FlipView's next button is tapped and FlipView's selection is changed when it re-enters the tree.
    void ScrollViewerInFlipViewTests::UnparentFlipViewDuringTapSelectionChange1()
    {
        TemporarilyUnparentFlipViewDuringSelectionChange(false /*flick*/, true /*tapNextButton*/, true /*changeSelectionOnReentry*/);
    }

    // Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change.
    // FlipView's next button is tapped.
    void ScrollViewerInFlipViewTests::UnparentFlipViewDuringTapSelectionChange2()
    {
        TemporarilyUnparentFlipViewDuringSelectionChange(false /*flick*/, true /*tapNextButton*/, false /*changeSelectionOnReentry*/);
    }

    // Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change.
    // FlipView's next button is clicked and FlipView's selection is changed when it re-enters the tree.
    void ScrollViewerInFlipViewTests::UnparentFlipViewDuringClickSelectionChange1()
    {
        TemporarilyUnparentFlipViewDuringSelectionChange(false /*flick*/, false /*tapNextButton*/, true /*changeSelectionOnReentry*/);
    }

    // Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change.
    // FlipView's next button is clicked.
    void ScrollViewerInFlipViewTests::UnparentFlipViewDuringClickSelectionChange2()
    {
        TemporarilyUnparentFlipViewDuringSelectionChange(false /*flick*/, false /*tapNextButton*/, false /*changeSelectionOnReentry*/);
    }

    // Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change.
    // FlipView is flicked and its selection is changed when it re-enters the tree.
    void ScrollViewerInFlipViewTests::UnparentFlipViewDuringFlickSelectionChange1()
    {
        TemporarilyUnparentFlipViewDuringSelectionChange(true /*flick*/, false /*tapNextButton*/, true /*changeSelectionOnReentry*/);
    }

    // Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change.
    // FlipView is flicked.
    void ScrollViewerInFlipViewTests::UnparentFlipViewDuringFlickSelectionChange2()
    {
        TemporarilyUnparentFlipViewDuringSelectionChange(true /*flick*/, false /*tapNextButton*/, false /*changeSelectionOnReentry*/);
    }

    //
    // Private methods
    //
    // First navigates from item 0 to item 1 within a FlipView using a touch flick, a touch tap on the next button or a click on the next button.
    // The FlipView is taken out of the tree during that navigation, then re-enters the tree. A second navigation is then performed using the keyboard.
    // flick==true && tapNextButton==false: first navigation attempt from item 0 to item 1 is through a touch flick. The selected item remains at 0.
    // flick==false && tapNextButton==true: first navigation attempt from item 0 to item 1 is through a next button tap. The selected item changes to 1.
    // flick==false && tapNextButton==false: first navigation attempt from item 0 to item 1 is through a next button click. The selected item changes to 1.
    // changeSelectionOnReentry==true: after re-entering the tree, the FlipView.SelectedIndex is changed to 2.
    void ScrollViewerInFlipViewTests::TemporarilyUnparentFlipViewDuringSelectionChange(bool flick, bool tapNextButton, bool changeSelectionOnReentry)
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->SetPostTickCallback(nullptr);
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        bool flipViewRemoved = false;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::FlipView, Loaded);

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\ScrollViewer\\ScrollViewerInFlipView.xaml";

        xaml_controls::Button^ nextButtonInFlipView = nullptr;
        xaml_controls::ScrollViewer^ scrollViewerInFlipView = nullptr;
        xaml_controls::FlipView^ flipView = nullptr;
        xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(xamlFile));
        VERIFY_IS_NOT_NULL(rootGrid);

        RunOnUIThread([&]()
        {
            flipView = safe_cast<xaml_controls::FlipView^>(rootGrid->FindName(L"flipView"));
            VERIFY_IS_NOT_NULL(flipView);

            loadedRegistration.Attach(
                flipView,
                ref new RoutedEventHandler([loadedEvent](Platform::Object^, RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));
            
            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewerInFlipView = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(flipView, L"ScrollingHost"));
            VERIFY_IS_NOT_NULL(scrollViewerInFlipView);

            nextButtonInFlipView = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(flipView, L"NextButtonHorizontal"));
            VERIFY_IS_NOT_NULL(nextButtonInFlipView);

            nextButtonInFlipView->Visibility = xaml::Visibility::Visible;

            viewChangedRegistration.Attach(scrollViewerInFlipView, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [rootGrid, flipView, scrollViewerInFlipView, viewChangedEvent, &flipViewRemoved](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewerInFlipView->HorizontalOffset, scrollViewerInFlipView->VerticalOffset, scrollViewerInFlipView->ZoomFactor, args->IsIntermediate);

                if (args->IsIntermediate)
                {
                    if (!flipViewRemoved)
                    {
                        LOG_OUTPUT(L"Setting PostTickCallback.");
                        // Could not get this to properly work on phone.
                        TestServices::WindowHelper->SetPostTickCallback(ref new PostTickCallback([rootGrid, &flipViewRemoved]()
                        {
                            LOG_OUTPUT(L"Running PostTickCallback.");
                            if (!flipViewRemoved)
                            {
                                flipViewRemoved = true;
                                LOG_OUTPUT(L"Removing FlipView.");
                                rootGrid->Children->RemoveAt(0);
                                TestServices::WindowHelper->SetPostTickCallback(nullptr);
                            }
                        }));
                    }
                }
                else
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        if (flick)
        {
            TestServices::InputHelper->Flick(flipView, FlickDirection::West);
        }
        else
        {
            TestServices::InputHelper->MoveMouse(scrollViewerInFlipView);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->MoveMouse(nextButtonInFlipView);
            TestServices::WindowHelper->WaitForIdle();

            if (tapNextButton)
            {
                LOG_OUTPUT(L"Tapping FlipView's next button.");
                TestServices::InputHelper->Tap(nextButtonInFlipView);
            }
            else
            {
                LOG_OUTPUT(L"Clicking FlipView's next button.");
                TestServices::InputHelper->LeftMouseClick(nextButtonInFlipView);
            }
        }

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (flipViewRemoved)
            {
                LOG_OUTPUT(L"Re-adding FlipView.");
                rootGrid->Children->Append(flipView);

                if (changeSelectionOnReentry)
                {
                    LOG_OUTPUT(L"Changing FlipView selection.");
                    flipView->SelectedIndex = 2;
                }
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flipView->SelectedIndex, changeSelectionOnReentry ? 2 : (flick ? 0 : 1));

            // Focus the FlipView to ensure it gets the key press.
            flipView->Focus(xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();
        viewChangedEvent->Reset();

        LOG_OUTPUT(L"Hitting right arrow key.");
        TestServices::KeyboardHelper->Right();

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flipView->SelectedIndex, changeSelectionOnReentry ? 3 : (flick ? 1 : 2));
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ScrollViewer
