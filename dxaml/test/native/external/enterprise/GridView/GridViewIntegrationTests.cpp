// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "GridViewIntegrationTests.h"
#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>
#include "KeyboardInjectionOverride.h"
#include <collection.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace GridView {

    //
    // Class & Test Setup
    //
    bool GridViewIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool GridViewIntegrationTests::ClassCleanup()
    {
        return true;
    }

    bool GridViewIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void GridViewIntegrationTests::AnimateItemIntoViewWithGamepad()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        InputDevice device = InputDevice::Gamepad;

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::GridView^ gridView = nullptr;
        xaml_controls::GridViewItem^ gridViewItem = nullptr;
        xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"HeaderedGridView.xaml"));
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

            gridView = safe_cast<xaml_controls::GridView^>(rootGrid->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);
        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(gridView, 0));
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

            gridView->Focus(FocusState::Keyboard);

            gridViewItem = safe_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(0));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to first item.");
            gridViewItem->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving 3 items right, to the 7th item, with the gamepad.");
        for (int i = 0; i < 3; i++)
        {
            CommonInputHelper::Right(device);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"GridView SelectedIndex is %d.", gridView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 268.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, 6);
        });

        LOG_OUTPUT(L"Moving 3 times left, to the 1st item, with the gamepad.");
        for (int i = 0; i < 3; i++)
        {
            CommonInputHelper::Left(device);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"GridView SelectedIndex is %d.", gridView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 116.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
        });
    }

    // Navigates through items, header and footer, nested in ScrollViewer, with the gamepad left and right buttons.
    void GridViewIntegrationTests::AnimateGridViewInScrollViewerWithGamepad()
    {
        TestCleanupWrapper cleanup;
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::WaitForIdleBeforeAndAfter);

        InputDevice device = InputDevice::Gamepad;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::GridView^ gridView = nullptr;
        xaml_controls::GridViewItem^ gridViewItem = nullptr;
        xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridViewInScrollViewer.xaml"));
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

            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootGrid->FindName(L"scrollViewer"));
            VERIFY_IS_NOT_NULL(scrollViewer);

            gridView = safe_cast<xaml_controls::GridView^>(rootGrid->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                [](Platform::Object^, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
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

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            gridView->Focus(FocusState::Keyboard);
            gridViewItem = safe_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(0));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to first item.");
            gridViewItem->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving to last column fast and into the right footer.");
        for (int i = 0; i < 8; i++)
        {
            viewChangedEvent->Reset();
            CommonInputHelper::Right(device);
        }

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"GridView SelectedIndex is %d.", gridView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 2252.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, 14);
        });

        LOG_OUTPUT(L"Moving to first column fast and into left header.");
        for (int i = 0; i < 8; i++)
        {
            viewChangedEvent->Reset();
            CommonInputHelper::Left(device);
        }

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"GridView SelectedIndex is %d.", gridView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
        });
    }

    void GridViewIntegrationTests::ValidateNavigationDoesNotHorizontallyWrapWithGamepad()
    {
        ValidateNavigationDoesNotWrapWithGamepad(true /* testHorizontalWrapping */);
    }

    void GridViewIntegrationTests::ValidateNavigationDoesNotVerticallyWrapWithGamepad()
    {
        ValidateNavigationDoesNotWrapWithGamepad(false /* testHorizontalWrapping */);
    }

    void GridViewIntegrationTests::ValidateNavigationDoesNotWrapWithGamepad(bool testHorizontalWrapping)
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

        xaml_controls::GridView^ gridView = nullptr;
        xaml_controls::GridViewItem^ gridViewItem = nullptr;
        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + (testHorizontalWrapping ? L"VerticalGridView.xaml" : L"GridView.xaml")));
        VERIFY_IS_NOT_NULL(rootPage);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::GridView, SelectionChanged);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPage;

            rootLoadedRegistration.Attach(
                rootPage,
                ref new xaml::RoutedEventHandler(
                [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                rootLoadedEvent->Set();
            }));

            gridView = safe_cast<xaml_controls::GridView^>(rootPage->FindName(L"gridView"));

            selectionChangedRegistration.Attach(gridView, [gridView, selectionChangedEvent]()
            {
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"SelectedIndex changed to %d.", gridView->SelectedIndex);
                });

                selectionChangedEvent->Set();
            });
        });

        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            gridViewItem = safe_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(0));
            gridViewItem->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        if (testHorizontalWrapping)
        {
            LOG_OUTPUT(L"Move right three times. We should end up on the third item, not the fourth, as we shouldn't be wrapping.");
            CommonInputHelper::Right(device);
            selectionChangedEvent->WaitForDefault();
            CommonInputHelper::Right(device);
            selectionChangedEvent->WaitForDefault();
            selectionChangedEvent->Reset();
            CommonInputHelper::Right(device);

        }
        else
        {
            LOG_OUTPUT(L"Move down twice. We should end up on the second item, not the third, as we shouldn't be wrapping.");
            CommonInputHelper::Down(device);
            selectionChangedEvent->WaitForDefault();
            selectionChangedEvent->Reset();
            CommonInputHelper::Down(device);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, testHorizontalWrapping ? 2 : 1);
            VERIFY_IS_FALSE(selectionChangedEvent->HasFired());
        });

        if (testHorizontalWrapping)
        {
            LOG_OUTPUT(L"Move down once and then left three times. We should end up on the fourth item, not the third, as we shouldn't be wrapping.");
            CommonInputHelper::Down(device);
            selectionChangedEvent->WaitForDefault();
            CommonInputHelper::Left(device);
            selectionChangedEvent->WaitForDefault();
            CommonInputHelper::Left(device);
            selectionChangedEvent->WaitForDefault();
            selectionChangedEvent->Reset();
            CommonInputHelper::Left(device);
        }
        else
        {
            LOG_OUTPUT(L"Move right once and then up twice. We should end up on the third item, not the second, as we shouldn't be wrapping.");
            CommonInputHelper::Right(device);
            selectionChangedEvent->WaitForDefault();
            CommonInputHelper::Up(device);
            selectionChangedEvent->WaitForDefault();
            selectionChangedEvent->Reset();
            CommonInputHelper::Up(device);
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, testHorizontalWrapping ? 3 : 2);
            VERIFY_IS_FALSE(selectionChangedEvent->HasFired());
        });
    }

    void GridViewIntegrationTests::BringItemIntoShrunkViewWithGamepad()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            TestServices::Utilities->ShrinkApplicationViewVisibleBounds(false);
        });

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(350, 400));

        TestServices::Utilities->ShrinkApplicationViewVisibleBounds(true);

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();

        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::GridView^ gridView = nullptr;
        xaml_controls::GridViewItem^ gridViewItem = nullptr;
        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridView.xaml"));
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

            gridView = safe_cast<xaml_controls::GridView^>(rootPage->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

        });

        LOG_OUTPUT(L"Waiting for root FE Loaded event.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto controlTemplateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(gridView, 0));
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

            gridView->Focus(FocusState::Keyboard);

            gridViewItem = safe_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(0));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving focus to first item.");
            gridViewItem->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving 4 items right, to the 9th item, with the gamepad.");
        for (int i = 0; i < 4; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadRight();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"GridView SelectedIndex is %d.", gridView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 296.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, 8);
        });

        LOG_OUTPUT(L"Moving 3 times left, to the 3rd item, with the gamepad.");
        for (int i = 0; i < 3; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadLeft();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"GridView SelectedIndex is %d.", gridView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 14.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, 2);
        });

        LOG_OUTPUT(L"Moving once to the left, to the 1st item, with the gamepad.");
        TestServices::KeyboardHelper->GamepadDpadLeft();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ScrollViewer view is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            LOG_OUTPUT(L"GridView SelectedIndex is %d.", gridView->SelectedIndex);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
        });
    }

    //
    // Code-authored DataTemplate (new DataTemplate(callback)) core-path coverage
    //

    // Orders map keys by object identity (hash) so realized elements can be used as keys.
    struct ObjectComparator
    {
        bool operator()(Platform::Object^ const& left, Platform::Object^ const& right) const
        {
            return left->GetHashCode() < right->GetHashCode();
        }
    };

    void GridViewIntegrationTests::CodeAuthoredItemTemplateReceivesDataContext()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::GridView^ gridView = nullptr;
        auto items = ref new Platform::Collections::Vector<Platform::Object^>();
        for (int i = 0; i < 5; i++)
        {
            items->Append(ref new Platform::String((L"Item " + std::to_wstring(i)).c_str()));
        }

        auto callbackCount = std::make_shared<int>(0);
        // Every element the callback produces, so realized (and later recycled) roots can be confirmed to be reused.
        auto created = ref new Platform::Collections::Vector<xaml::UIElement^>();
        // The DataContext each element last received via DataContextChanged, keyed by the element so it can
        // be looked up directly. Cleared before recycling to prove the event fires again on reused elements.
        auto changedContexts = ref new Platform::Collections::Map<Platform::Object^, Platform::String^, ObjectComparator>();

        RunOnUIThread([&]()
        {
            // A code-authored DataTemplate builds each container's tree from an app callback.
            auto factory = ref new xaml::DataTemplateElementFactory([callbackCount, created, changedContexts]() -> xaml::UIElement^
            {
                (*callbackCount)++;
                auto textBlock = ref new xaml_controls::TextBlock();
                created->Append(textBlock);
                // As the root enters the live tree (or is recycled to a new item) it inherits that item as
                // its DataContext and raises DataContextChanged. App code can hook this event to push data into
                // the subtree it created.
                textBlock->DataContextChanged += ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::DataContextChangedEventArgs^>(
                    [changedContexts](xaml::FrameworkElement^ sender, xaml::DataContextChangedEventArgs^ e)
                {
                    changedContexts->Insert(sender, dynamic_cast<Platform::String^>(e->NewValue));
                });
                textBlock->SetBinding(xaml_controls::TextBlock::TextProperty, ref new Microsoft::UI::Xaml::Data::Binding());
                return textBlock;
            });

            gridView = ref new xaml_controls::GridView();
            gridView->Height = 500;
            gridView->Width = 500;
            gridView->ItemsSource = items;
            gridView->ItemTemplate = ref new xaml::DataTemplate(factory);

            TestServices::WindowHelper->WindowContent = gridView;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verifies each realized container's root reflects its item via DataContext, the {Binding} on Text,
        // and DataContextChanged, and that every root is a distinct element the callback produced.
        auto verifyRealizedRoots = [&]()
        {
            auto seen = ref new Platform::Collections::Map<Platform::Object^, bool, ObjectComparator>();
            for (unsigned int i = 0; i < items->Size; i++)
            {
                auto container = dynamic_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(i));
                VERIFY_IS_NOT_NULL(container);

                auto root = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(container);
                VERIFY_IS_NOT_NULL(root);

                // The root is one the callback produced, not a fresh core-fabricated peer/tree.
                unsigned int createdIndex = 0;
                VERIFY_IS_TRUE(created->IndexOf(root, &createdIndex));

                // Each item maps to a distinct realized element.
                VERIFY_IS_FALSE(seen->HasKey(root));
                seen->Insert(root, true);

                // The root receives its item as DataContext, which drives classic {Binding} and x:Bind.
                VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(items->GetAt(i)), safe_cast<Platform::String^>(root->DataContext));

                // The {Binding} on Text resolved against that DataContext - proof the app's peer, with its
                // bindings, is the one in the live tree.
                VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(items->GetAt(i)), root->Text);

                // DataContextChanged delivered the item - the event app code hooks to update the template.
                VERIFY_IS_TRUE(changedContexts->HasKey(root));
                VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(items->GetAt(i)), changedContexts->Lookup(root));
            }
        };

        int callbackCountAfterFirstRealization = 0;
        RunOnUIThread([&]()
        {
            // The virtualizing panel may create extra prototype/measure elements, so assert the intent
            // (a container tree per item came from the callback) rather than an exact callback count.
            VERIFY_IS_TRUE(*callbackCount >= static_cast<int>(items->Size));
            verifyRealizedRoots();
            callbackCountAfterFirstRealization = *callbackCount;
        });

        // Recycle: clearing the source pushes the realized containers into the recycle pool.
        RunOnUIThread([&]()
        {
            items->Clear();
            changedContexts->Clear();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Re-populate with the same number of new items; the pooled containers should be reused for them.
        RunOnUIThread([&]()
        {
            for (int i = 0; i < 5; i++)
            {
                items->Append(ref new Platform::String((L"Recycled " + std::to_wstring(i)).c_str()));
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Recycled containers reuse pooled elements, so the callback is not invoked again.
            VERIFY_ARE_EQUAL(callbackCountAfterFirstRealization, *callbackCount);

            // Each reused root still receives its new item as DataContext, re-raises DataContextChanged, and
            // its {Binding} updates to the new value.
            verifyRealizedRoots();
        });
    }

    void GridViewIntegrationTests::CodeAuthoredItemTemplateWithNullSubtreeDoesNotCrash()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::GridView^ gridView = nullptr;
        auto items = ref new Platform::Collections::Vector<Platform::Object^>();
        for (int i = 0; i < 5; i++)
        {
            items->Append(ref new Platform::String((L"Item " + std::to_wstring(i)).c_str()));
        }

        auto callbackCount = std::make_shared<int>(0);

        RunOnUIThread([&]()
        {
            // A callback that produces no subtree (returns null) must be tolerated exactly like an
            // empty markup <DataTemplate/>: the core path returns no root and nothing crashes.
            auto factory = ref new xaml::DataTemplateElementFactory([callbackCount]() -> xaml::UIElement^
            {
                (*callbackCount)++;
                return nullptr;
            });

            gridView = ref new xaml_controls::GridView();
            gridView->Height = 500;
            gridView->Width = 500;
            gridView->ItemsSource = items;
            gridView->ItemTemplate = ref new xaml::DataTemplate(factory);

            TestServices::WindowHelper->WindowContent = gridView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Reaching here after idle proves the null element factory result was tolerated end-to-end
            // (realization, measure/arrange, and phasing) without crashing.
            VERIFY_IS_TRUE(*callbackCount >= 1);

            for (unsigned int i = 0; i < items->Size; i++)
            {
                auto container = dynamic_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(i));
                // The container itself still realizes; the null template simply contributes no factory root.
                VERIFY_IS_NOT_NULL(container);
            }
        });
    }

    //
    // Private Methods
    //
    Platform::String^ GridViewIntegrationTests::GetResourcesPath() const
    {
        return GetPackageFolder() + L"resources\\native\\enterprise\\listviewbase\\";
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::GridView
