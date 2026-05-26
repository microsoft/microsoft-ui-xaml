// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ItemsControlIntegrationTests.h"
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

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ItemsControl {

    //
    // Class & Test Setup
    //
    bool ItemsControlIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ItemsControlIntegrationTests::ClassCleanup()
    {
        return true;
    }

    bool ItemsControlIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithLocalTabNavigation()
    {
        Platform::String^ expectedFocusSequence = "[B][1][.1][2][.1][3][.1][A][.1][3][.1][2][.1][1][B]";

        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::Default, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::UseStackPanelAsItemsPanel, expectedFocusSequence);
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithCycleTabNavigation()
    {
        Platform::String^ expectedFocusSequence = "[B][1][.1][2][.1][3][.1][1][.1][3][.1][2][.1][1][.1]";

        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::Default, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::UseStackPanelAsItemsPanel, expectedFocusSequence);
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithOnceTabNavigation()
    {
        Platform::String^ expectedFocusSequence = "[B][1][A][B][A][.1][1]";

        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::Default, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseStackPanelAsItemsPanel, expectedFocusSequence);
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithLocalTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, VerifyTabOrderFlags::UseHeaderAndFooter, "[B][H][1][.1][2][.1][3][.1][F][A][F][.1][3][.1][2][.1][1][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][1][.1][2][.1][3][.1][A][.1][3][.1][2][.1][1][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseStackPanelAsItemsPanel), "[B][H][1][.1][2][.1][3][.1][F][A][F][.1][3][.1][2][.1][1][H][B]");
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithCycleTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, VerifyTabOrderFlags::UseHeaderAndFooter, "[B][H][1][.1][2][.1][3][.1][F][H][F][.1][3][.1][2][.1][1][H][F]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][1][.1][2][.1][3][.1][H][.1][3][.1][2][.1][1][H][.1]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseStackPanelAsItemsPanel), "[B][H][1][.1][2][.1][3][.1][F][H][F][.1][3][.1][2][.1][1][H][F]");
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithOnceTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseHeaderAndFooter, "[B][H][1][F][A][B][A][F][.1][1][H]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][1][A][B][A][.1][1][H]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseStackPanelAsItemsPanel), "[B][H][1][F][A][B][A][F][.1][1][H]");
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithGroupedItemsAndLocalTabNavigation()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][11][21][A][B][11][21][A][21][11][B][A][21][11][B]");
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithGroupedItemsAndCycleTabNavigation()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][11][21][11][21][11][21][11][21][11][21][11][21][11][21]");
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithGroupedItemsAndOnceTabNavigation()
    {
        Platform::String^ expectedFocusSequence = "[B][11][A][B][A][11][B]";

        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, VerifyTabOrderFlags::UseGroupedItems, expectedFocusSequence);
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), expectedFocusSequence);
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithGroupedItemsLocalTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Local, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][11][21][A][B][H][11][21][11][H][B][A][21][11][H][B]");
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithGroupedItemsCycleTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Cycle, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][11][21][H][11][21][H][11][H][21][11][H][21][11][H][21]");
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithGroupedItemsOnceTabNavigationHeaderAndFooter()
    {
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems), "[B][H][11][F][A][B][A][F][11][H][B]");
        VerifyTabOrder(xaml_input::KeyboardNavigationMode::Once, static_cast<VerifyTabOrderFlags>(VerifyTabOrderFlags::UseHeaderAndFooter + VerifyTabOrderFlags::UseGroupedItems + VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel), "[B][H][11][A][B][A][11][H][B]");
    }

    void ItemsControlIntegrationTests::VerifyTabOrderWithVirtualizedItemsAndLocalTabNavigation()
    {
        TestCleanupWrapper cleanup;

        Platform::String^ focusSequence = "";

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto rootGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);

        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ItemsControlForTabNavigation.xaml"));
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

                    LOG_OUTPUT(L"GotFocus raised. OriginalSource Tag: %s, OriginalSource Type: %s", tag ? tag->ToString()->Data() : L"null", originalSourceFE->GetType()->ToString()->Data());

                    if (tag == nullptr)
                    {
                        focusSequence += "[]";
                    }
                    else
                    {
                        focusSequence += "[" + tag + "]";
                    }
                }));

            beforeButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"BeforeButton"));
            VERIFY_IS_NOT_NULL(beforeButton);

            afterButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"AfterButton"));
            VERIFY_IS_NOT_NULL(afterButton);

            xaml_controls::ItemsControl^ mainVirtualizedItemsControl = safe_cast<xaml_controls::ItemsControl^>(rootPage->FindName(L"MainVirtualizedItemsControl"));
            VERIFY_IS_NOT_NULL(mainVirtualizedItemsControl);

            mainVirtualizedItemsControl->Visibility = xaml::Visibility::Visible;
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

    void ItemsControlIntegrationTests::VerifyTabOrderWithVirtualizedItemsAndCycleTabNavigation()
    {
        TestCleanupWrapper cleanup;

        Platform::String^ focusSequence = "";

        std::shared_ptr<Event> rootLoadedEvent = std::make_shared<Event>();
        auto rootLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto rootGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);

        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ItemsControlForTabNavigation.xaml"));
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

                    LOG_OUTPUT(L"GotFocus raised. OriginalSource Tag: %s, OriginalSource Type: %s", tag ? tag->ToString()->Data() : L"null", originalSourceFE->GetType()->ToString()->Data());

                    if (tag == nullptr)
                    {
                        focusSequence += "[]";
                    }
                    else
                    {
                        focusSequence += "[" + tag + "]";
                    }
                }));

            beforeButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"BeforeButton"));
            VERIFY_IS_NOT_NULL(beforeButton);

            afterButton = safe_cast<xaml_controls::Button^>(rootPage->FindName(L"AfterButton"));
            VERIFY_IS_NOT_NULL(afterButton);

            xaml_controls::ItemsControl^ mainVirtualizedItemsControl = safe_cast<xaml_controls::ItemsControl^>(rootPage->FindName(L"MainVirtualizedItemsControl"));
            VERIFY_IS_NOT_NULL(mainVirtualizedItemsControl);

            mainVirtualizedItemsControl->Visibility = xaml::Visibility::Visible;
            mainVirtualizedItemsControl->TabNavigation = xaml_input::KeyboardNavigationMode::Cycle;
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

    void ItemsControlIntegrationTests::VerifyTabOrder(
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

        xaml_controls::Page^ rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetResourcesPath() + ((flags & VerifyTabOrderFlags::UseGroupedItems) ? L"GroupedItemsControlForTabNavigation.xaml" : L"ItemsControlForTabNavigation.xaml")));
        xaml_controls::Button^ beforeButton = nullptr;
        xaml_controls::ItemsControl^ mainItemsControl = nullptr;

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

                    LOG_OUTPUT(L"GotFocus raised. OriginalSource Tag: %s, OriginalSource Type: %s", tag ? tag->ToString()->Data() : L"null", originalSourceFE->GetType()->ToString()->Data());

                    if (tag == nullptr)
                    {
                        focusSequence += "[]";
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
                mainItemsControl = safe_cast<xaml_controls::ItemsControl^>(rootPage->FindName(L"MainItemsControlWithVirtualizingStackPanel"));
            }
            else if (flags & VerifyTabOrderFlags::UseStackPanelAsItemsPanel)
            {
                mainItemsControl = safe_cast<xaml_controls::ItemsControl^>(rootPage->FindName(L"MainItemsControlWithStackPanel"));
            }
            else
            {
                mainItemsControl = safe_cast<xaml_controls::ItemsControl^>(rootPage->FindName(L"MainItemsControl"));
            }
            VERIFY_IS_NOT_NULL(mainItemsControl);

            mainItemsControl->Visibility = xaml::Visibility::Visible;
            mainItemsControl->TabNavigation = tabNavigation;

            if (flags & VerifyTabOrderFlags::UseGroupedItems)
            {
                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPage->FindName(L"cvs"));
                VERIFY_IS_NOT_NULL(cvs);

                itemsSource = GetGroupedData();
                VERIFY_IS_NOT_NULL(itemsSource);

                cvs->Source = itemsSource;
                cvs->IsSourceGrouped = true;
            }
        });

        LOG_OUTPUT(L"Waiting for content to load.");
        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        if (flags & VerifyTabOrderFlags::UseHeaderAndFooter)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Accessing ItemsPresenter.");
                xaml_controls::ItemsPresenter^ itemsPresenter = safe_cast<xaml_controls::ItemsPresenter^>(xaml_media::VisualTreeHelper::GetChild(mainItemsControl, 0));
                VERIFY_IS_NOT_NULL(itemsPresenter);

                LOG_OUTPUT(L"Setting header.");
                xaml_controls::Button^ headerButton = ref new xaml_controls::Button();
                headerButton->Content = L"ItemsPresenter Header";
                headerButton->Tag = L"H";
                itemsPresenter->Header = headerButton;

                // VirtualizingStackPanel does not support footer.
                if (!(flags & VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel))
                {
                    LOG_OUTPUT(L"Setting footer.");
                    xaml_controls::Button^ footerButton = ref new xaml_controls::Button();
                    footerButton->Content = L"ItemsPresenter Footer";
                    footerButton->Tag = L"F";
                    itemsPresenter->Footer = footerButton;
                }
            });

            LOG_OUTPUT(L"Waiting for header/footer updates.");
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Giving focus to the initial button.");
        FocusTestHelper::EnsureFocus(beforeButton, xaml::FocusState::Keyboard);

        // When ItemsControl.TabNavigation is Once, the Tab key is hit at least enough times to enter and exit the control - exactly one item should be visited.
        // When ItemsControl.TabNavigation is Local or Cycle, it is hit at least enough times to go through all three items and either exit or cycle.
        int tabCount = xaml_input::KeyboardNavigationMode::Once == tabNavigation ? 3 : 7;

        if (flags & VerifyTabOrderFlags::UseHeaderAndFooter)
        {
            // All panels use a focusable header, so add 1 to the Tab count.
            tabCount++;

            if (!(flags & VerifyTabOrderFlags::UseVirtualizingStackPanelAsItemsPanel))
            {
                // All panels except the VirtualizingStackPanel use a footer, add 1 to the Tab count for those.
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
            logMessage += "Modern Panel";
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

    //
    // Private Methods
    //
    Platform::String^ ItemsControlIntegrationTests::GetResourcesPath() const
    {
        return GetPackageFolder() + L"resources\\native\\enterprise\\ItemsControl\\";
    }

    Platform::Collections::Vector<Platform::Object^>^ ItemsControlIntegrationTests::GetGroupedData() const
    {
        auto groupedData = ref new Platform::Collections::Vector<Platform::Object^>();

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

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::ItemsControl