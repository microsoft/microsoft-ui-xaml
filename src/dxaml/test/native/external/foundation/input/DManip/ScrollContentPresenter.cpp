// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollContentPresenter.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;
using namespace test_infra;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        static const wchar_t* s_richTextBlockWithSmallText =
            L"<RichTextBlock xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' FontSize='9'>"
            L"  <Paragraph>Text in RichTextBlock</Paragraph>"
            L"</RichTextBlock>";

        static const wchar_t* s_richTextBlockWithLargeText =
            L"<RichTextBlock xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' FontSize='9'>"
            L"  <Paragraph>"
            L"  Text in RichTextBlock<LineBreak/>"
            L"  <Hyperlink NavigateUri='http://www.bing.com'>Bing hyperlink</Hyperlink><LineBreak/>"
            L"  Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse feugiat tristique erat, commodo fermentum tellus faucibus nec.<LineBreak/>"
            L"  <Hyperlink NavigateUri='http://www.microsoft.com'>Microsoft link</Hyperlink><LineBreak/>"
            L"  Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse feugiat tristique erat, commodo fermentum tellus faucibus nec.<LineBreak/>"
            L"  </Paragraph>"
            L"</RichTextBlock>";

        bool ScrollContentPresenter::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ScrollContentPresenter::ClassCleanup()
        {
            return true;
        }

        bool ScrollContentPresenter::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ScrollContentPresenter::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ ScrollContentPresenter::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\external\\foundation\\input\\dmanip\\");
        }

        void ScrollContentPresenter::AddScrollViewerAndRichTextBlock(
            xaml_controls::StackPanel^ stackPanel,
            bool smallText,
            bool wrap,
            bool canScrollHorizontally,
            bool canScrollVertically,
            int margin,
            double width,
            double height) const
        {
            xaml_controls::ScrollViewer^ scrollViewer = ref new xaml_controls::ScrollViewer();
            xaml_controls::RichTextBlock^ richTextBlock = nullptr;

            if (smallText)
            {
                richTextBlock = safe_cast<xaml_controls::RichTextBlock^>(Markup::XamlReader::Load(Platform::StringReference(s_richTextBlockWithSmallText)));
            }
            else
            {
                richTextBlock = safe_cast<xaml_controls::RichTextBlock^>(Markup::XamlReader::Load(Platform::StringReference(s_richTextBlockWithLargeText)));
            }

            richTextBlock->TextWrapping = wrap ? Microsoft::UI::Xaml::TextWrapping::Wrap : Microsoft::UI::Xaml::TextWrapping::NoWrap;

            if (margin > 0)
            {
                richTextBlock->Margin = xaml::ThicknessHelper::FromUniformLength(margin);
            }

            if (width > 0)
            {
                richTextBlock->Width = width;
            }

            if (height > 0)
            {
                richTextBlock->Height = height;
            }

            scrollViewer->Width = 300;
            scrollViewer->Height = 75;
            scrollViewer->HorizontalScrollBarVisibility = canScrollHorizontally ? xaml_controls::ScrollBarVisibility::Auto : xaml_controls::ScrollBarVisibility::Disabled;
            scrollViewer->VerticalScrollBarVisibility = canScrollVertically ? xaml_controls::ScrollBarVisibility::Auto : xaml_controls::ScrollBarVisibility::Disabled;

            scrollViewer->Content = richTextBlock;

            stackPanel->Children->Append(scrollViewer);
        }

        //------------------------------------------------------------------------
        // Test case: Tests alignment of ScrollContentPresenter content with a
        // Stretch alignment switched off and on.
        //------------------------------------------------------------------------
        void ScrollContentPresenter::StretchAlignmentSwitchedOffAndOn()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            xaml_controls::StackPanel^ stackPanel = nullptr;
            xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"ScrollContentPresenterWithStretchAlignment.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
                rootLoadedRegistration.Attach(
                    rootGrid,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootGrid;
            });

            LOG_OUTPUT(L"Waiting for root FE Loaded event.");
            rootLoadedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            RunOnUIThread([&]()
            {
                stackPanel = safe_cast<xaml_controls::StackPanel^>(rootGrid->FindName(L"stackPanel"));
                VERIFY_IS_NOT_NULL(stackPanel);
                LOG_OUTPUT(L"Changing StackPanel.VerticalAlignment from Stretch to Top.");
                stackPanel->VerticalAlignment = xaml::VerticalAlignment::Top;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing StackPanel.VerticalAlignment from Top to Stretch.");
                stackPanel->VerticalAlignment = xaml::VerticalAlignment::Stretch;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");
        }

        //------------------------------------------------------------------------
        // Test case: Tests alignment of ScrollContentPresenter content with a
        // Stretch alignment switched off and on within a ListView.
        //------------------------------------------------------------------------
        void ScrollContentPresenter::StretchAlignmentInListView()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            xaml_controls::ItemsPresenter^ itemsPresenter = nullptr;
            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            xaml_controls::ListView^ listView = nullptr;
            xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"ScrollContentPresenterInListView.xaml"));
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
                LOG_OUTPUT(L"ListView.UpdateLayout called.");
                listView->UpdateLayout();
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            RunOnUIThread([&]()
            {
                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(listView, L"ScrollViewer"));
                VERIFY_IS_NOT_NULL(scrollViewer);
                itemsPresenter = safe_cast<xaml_controls::ItemsPresenter^>(scrollViewer->Content);
                VERIFY_IS_NOT_NULL(itemsPresenter);
                VERIFY_ARE_EQUAL(itemsPresenter->HorizontalAlignment, xaml::HorizontalAlignment::Stretch);
                LOG_OUTPUT(L"Changing ItemsPresenter alignments from Stretch/Stretch to Left/Top.");
                itemsPresenter->HorizontalAlignment = xaml::HorizontalAlignment::Left;
                itemsPresenter->VerticalAlignment = xaml::VerticalAlignment::Top;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"ListView.UpdateLayout called.");
                listView->UpdateLayout();
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing ItemsPresenter.HorizontalAlignment from Left to Stretch.");
                itemsPresenter->HorizontalAlignment = xaml::HorizontalAlignment::Stretch;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"ListView.UpdateLayout called.");
                listView->UpdateLayout();
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");
        }

        //------------------------------------------------------------------------
        // Test case: Tests alignment of ScrollContentPresenter content inside an
        // unconstrained Flyout.
        //------------------------------------------------------------------------
        void ScrollContentPresenter::StretchAlignmentInFlyout()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(300, 400), 1.4f);

            xaml::FrameworkElement^ commandA = nullptr;
            xaml::FrameworkElement^ commandB = nullptr;
            xaml_controls::Primitives::FlyoutBase^ flyout = nullptr;
            xaml_controls::Page^ page = nullptr;

            auto loadedEvent = std::make_shared<Event>();
            auto appBarOpenedEvent = std::make_shared<Event>();
            auto appBarClosedEvent = std::make_shared<Event>();
            auto flyoutOpenedEvent = std::make_shared<Event>();
            auto flyoutClosedEvent = std::make_shared<Event>();

            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
            auto appBarOpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            auto appBarClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
            auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::Primitives::FlyoutBase, Opened);
            auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Primitives::FlyoutBase, Closed);

            xaml_controls::CommandBar^ topCommandBar = safe_cast<xaml_controls::CommandBar^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"ScrollContentPresenterInFlyout.xaml"));

            RunOnUIThread([&]()
            {
                page = TestServices::WindowHelper->SetupSimulatedAppPage();
                page->TopAppBar = topCommandBar;
                commandA = safe_cast<xaml::FrameworkElement^>(topCommandBar->PrimaryCommands->GetAt(0));
                commandB = safe_cast<xaml::FrameworkElement^>(topCommandBar->PrimaryCommands->GetAt(1));
                flyout = safe_cast<xaml_controls::Button^>(topCommandBar->PrimaryCommands->GetAt(0))->Flyout;
                VERIFY_IS_NOT_NULL(flyout);

                loadedRegistration.Attach(page, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                    loadedEvent->Set();
                }));

                appBarOpenedRegistration.Attach(topCommandBar, ref new wf::EventHandler<Platform::Object^>([appBarOpenedEvent](Platform::Object^ sender, Platform::Object^ e) {
                    appBarOpenedEvent->Set();
                }));

                appBarClosedRegistration.Attach(topCommandBar, ref new wf::EventHandler<Platform::Object^>([appBarClosedEvent](Platform::Object^ sender, Platform::Object^ e) {
                    appBarClosedEvent->Set();
                }));

                flyoutOpenedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^) {
                    flyoutOpenedEvent->Set();
                }));

                flyoutClosedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^) {
                    flyoutClosedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            loadedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Opening the top CommandBar.");
                topCommandBar->IsOpen = true;
            });

            LOG_OUTPUT(L"Waiting for CommandBar opening...");
            TestServices::WindowHelper->WaitForIdle();
            appBarOpenedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Showing Flyout.");
                flyout->ShowAt(commandA);
            });

            LOG_OUTPUT(L"Waiting for Flyout opening...");
            TestServices::WindowHelper->WaitForIdle();
            flyoutOpenedEvent->WaitForDefault();

            LOG_OUTPUT(L"Tapping the AppBarButton B.");
            TestServices::InputHelper->Tap(commandB);

            LOG_OUTPUT(L"Waiting for Flyout closing...");
            TestServices::WindowHelper->WaitForIdle();
            flyoutClosedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Closing the top CommandBar.");
                topCommandBar->IsOpen = false;
            });

            LOG_OUTPUT(L"Waiting for CommandBar closing...");
            TestServices::WindowHelper->WaitForIdle();
            appBarClosedEvent->WaitForDefault();
        }

        //------------------------------------------------------------------------
        // Test case: Tests alignment of ScrollContentPresenter content with a
        // Stretch alignment for text controls.
        //------------------------------------------------------------------------
        void ScrollContentPresenter::StretchAlignmentWithTextControl()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 1200));

            bool smallText = false;
            bool wrap = false;
            bool canScrollHorizontally = false;
            bool canScrollVertically = false;
            int margin = 0;
            double width = 0;
            double height = 0;

            xaml_controls::StackPanel^ stackPanel = nullptr;
            xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"ScrollContentPresenterWithTextControl.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;
                stackPanel = safe_cast<xaml_controls::StackPanel^>(rootGrid->FindName(L"stackPanel"));
                VERIFY_IS_NOT_NULL(stackPanel);
            });

            TestServices::WindowHelper->WaitForIdle();

            for (UINT16 hsbv = 0; hsbv < 2; hsbv++)
            {
                canScrollHorizontally = hsbv == 0;
                for (int vsbv = 0; vsbv < 2; vsbv++)
                {
                    canScrollVertically = vsbv == 0;
                    for (int tw = 0; tw < 2; tw++)
                    {
                        wrap = tw == 0;
                        for (double w = 0; w < 2; w++)
                        {
                            width = w * 200;
                            for (double h = 0; h < 2; h++)
                            {
                                height = h * 300;
                                for (int m = 0; m < 2; m++)
                                {
                                    margin = m * 20;
                                    for (int t = 0; t < 2; t++)
                                    {
                                        smallText = t == 0;
                                        RunOnUIThread([&]()
                                        {
                                            AddScrollViewerAndRichTextBlock(
                                            stackPanel,
                                            smallText,
                                            wrap,
                                            canScrollHorizontally,
                                            canScrollVertically,
                                            margin,
                                            width,
                                            height);
                                        });

                                        TestServices::WindowHelper->WaitForIdle();
                                    }
                                }
                            }
                        }
                    }

                    TestServices::WindowHelper->WaitForIdle();
                    TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison,
                        canScrollHorizontally ? (canScrollVertically ? L"1" : L"2") : (canScrollVertically ? L"3" : L"4"));

                    RunOnUIThread([&]()
                    {
                        stackPanel->Children->Clear();
                    });

                    TestServices::WindowHelper->WaitForIdle();
                }
            }
        }

        //------------------------------------------------------------------------
        // Test case: Tests the assignment of a viewport interaction.  We can't
        // really test if the interaction is in place, but we can test that the
        // process doesn't fail as we add/remove/swap viewer content.
        //------------------------------------------------------------------------
        void ScrollContentPresenter::ViewportInteraction()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 1200));


            xaml_controls::ScrollViewer^ scrollviewer1;
            xaml_controls::ScrollViewer^ scrollviewer2;

            xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"ViewportInteraction.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            RunOnUIThread([&]()
            {
                // Initial grid has two viewers and content
                TestServices::WindowHelper->WindowContent = rootGrid;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Swap the content in the two viewers
                xaml_controls::ScrollViewer^ scrollviewer1 = safe_cast<xaml_controls::ScrollViewer ^>(rootGrid->FindName(L"scrollviewer1"));
                xaml_controls::ScrollViewer^ scrollviewer2 = safe_cast<xaml_controls::ScrollViewer ^>(rootGrid->FindName(L"scrollviewer2"));
                VERIFY_IS_NOT_NULL(scrollviewer1);
                VERIFY_IS_NOT_NULL(scrollviewer2);

                Platform::Object^ content1 = scrollviewer1->Content;
                Platform::Object^ content2 = scrollviewer2->Content;
                VERIFY_IS_NOT_NULL(content1);
                VERIFY_IS_NOT_NULL(content2);

                scrollviewer1->Content = nullptr;
                scrollviewer2->Content = content1;
                scrollviewer1->Content = content2;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Remove the content from the two viewers
                xaml_controls::ScrollViewer^ scrollviewer1 = safe_cast<xaml_controls::ScrollViewer ^>(rootGrid->FindName(L"scrollviewer1"));
                xaml_controls::ScrollViewer^ scrollviewer2 = safe_cast<xaml_controls::ScrollViewer ^>(rootGrid->FindName(L"scrollviewer2"));
                VERIFY_IS_NOT_NULL(scrollviewer1);
                VERIFY_IS_NOT_NULL(scrollviewer2);

                scrollviewer1->Content = nullptr;
                scrollviewer2->Content = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();
        }

    } } }
} } } }