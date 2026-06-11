// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MenuFlyoutIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <StoryboardMonitorWrapper.h>
#include <TestCleanupWrapper.h>

#include <TreeHelper.h>
#include <FlyoutHelper.h>
#include <CommandHelper.h>
#include <ControlHelper.h>

#include <RuntimeEnabledFeaturesEnum.h>
#include <WUCRenderingScopeGuard.h>

#include "KeyboardInjectionOverride.h"
#include <HolographicOverride.h>

#include <Utils.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MenuFlyout {

    bool MenuFlyoutIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        return true;
    }

    bool MenuFlyoutIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool MenuFlyoutIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void MenuFlyoutIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::MenuFlyout>::CanInstantiate();
    }

    void MenuFlyoutIntegrationTests::CanMenuFlyoutOpenCloseProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanMenuFlyoutOpenClose();
    }

    void MenuFlyoutIntegrationTests::CanMenuFlyoutOpenCloseDropShadow()
    {
        CanMenuFlyoutOpenClose();
    }

    void MenuFlyoutIntegrationTests::CanMenuFlyoutOpenClose()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='button' Content='button.flyout' VerticalAlignment='Center' HorizontalAlignment='Left' FontSize='25' Padding='25,10' Margin='50'> "
                L"    <Button.Flyout> "
                L"      <MenuFlyout> "
                L"        <MenuFlyoutItem FontSize='30' Text='SUPERMAN' Foreground='RoyalBlue' Width='300' /> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <ToggleMenuFlyoutItem FontSize='30' Text='THE FLASH' Foreground='RoyalBlue' Width='300' IsChecked='False' /> "
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            VERIFY_IS_NOT_NULL(menuFlyout);

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"CanMenuFlyoutOpenClose: MenuFlyout Opened event is fired!");
                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"CanMenuFlyoutOpenClose: MenuFlyout Closed event is fired!");
                menuFlyoutClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Button Tap operation to show the MenuFlyout.");
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"RootPanel Tap operation to close the MenuFlyout.");
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
    }

    void MenuFlyoutIntegrationTests::VerifyMenuFlyoutPresenterStyle()
    {
        TestCleanupWrapper cleanup;

        auto menuFlyout = CreateMenuFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);
        VERIFY_IS_NOT_NULL(menuFlyout);

        auto target = FlyoutHelper::CreateTarget(
            100 /*width*/, 100 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Top);
        VERIFY_IS_NOT_NULL(target);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"VerifyMenuFlyoutPresenterStyle: Execute the ShowAt.");
        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto presenter = GetMenuFlyoutPresenter(menuFlyout);
            VERIFY_IS_NOT_NULL(presenter);
            auto tag = presenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"presenter_style"), tag->ToString());
        });

        LOG_OUTPUT(L"VerifyMenuFlyoutPresenterStyle: Execute the Hide.");
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::CanChangeMenuFlyoutPresenterStyleAtRuntime()
    {
        TestCleanupWrapper cleanup;

        auto menuFlyout = CreateMenuFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);

        auto target = FlyoutHelper::CreateTarget(
            100 /*width*/, 100 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Top);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutPresenter";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto style = ref new xaml::Style(type);
            style->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutPresenter::TagProperty, "presenter_style_2"));

            menuFlyout->MenuFlyoutPresenterStyle = style;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto presenter = GetMenuFlyoutPresenter(menuFlyout);
            auto tag = presenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"presenter_style_2"), tag->ToString());
        });

        FlyoutHelper::HideFlyout(menuFlyout);

        RunOnUIThread([&]()
        {
            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutPresenter";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto style = ref new xaml::Style(type);
            style->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutPresenter::TagProperty, "presenter_style_3"));

            menuFlyout->MenuFlyoutPresenterStyle = style;
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto presenter = GetMenuFlyoutPresenter(menuFlyout);
            auto tag = presenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"presenter_style_3"), tag->ToString());
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::CanAttachedMenuFlyoutShowHide()
    {
        TestCleanupWrapper cleanup;

        auto menuFlyout = CreateMenuFlyout(xaml_primitives::FlyoutPlacementMode::Left);
        VERIFY_IS_NOT_NULL(menuFlyout);

        auto target = FlyoutHelper::CreateTarget(
            200 /*width*/, 200 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Center);
        VERIFY_IS_NOT_NULL(target);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;

            auto attachedMenuFlyout = xaml_controls::MenuFlyout::GetAttachedFlyout(target);
            VERIFY_IS_NULL(attachedMenuFlyout);

            xaml_controls::MenuFlyout::SetAttachedFlyout(target, menuFlyout);

            attachedMenuFlyout = xaml_controls::MenuFlyout::GetAttachedFlyout(target);
            VERIFY_IS_NOT_NULL(attachedMenuFlyout);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"CanAttachedMenuFlyoutShowHide: Execute ShowAttachedMenuFlyout.");
        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAttachedFlyout);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"CanAttachedMenuFlyoutShowHide: Execute the Hide.");
        FlyoutHelper::HideFlyout(menuFlyout);

        LOG_OUTPUT(L"CanAttachedMenuFlyoutShowHide: Execute ShowAt.");
        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"CanAttachedMenuFlyoutShowHide: Execute the Hide.");
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::CanClickMenuFlyoutItem()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::MenuFlyoutItem^ menuItem = nullptr;

        auto menuItemClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

        auto menuFlyout = CreateMenuFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);
        VERIFY_IS_NOT_NULL(menuFlyout);

        auto target = FlyoutHelper::CreateTarget(
            100 /*width*/, 100 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Left,
            xaml::VerticalAlignment::Top);
        VERIFY_IS_NOT_NULL(target);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ExecuteDelegate^ commandExecute = ref new ExecuteDelegate([&](Platform::Object^ param)
            {
                LOG_OUTPUT(L"VerifyMenuFlyoutPresenterStyle: Process the delegate!");
            });

            menuItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyout->Items->GetAt(1));
            VERIFY_IS_NOT_NULL(menuItem);

            auto menuItemCommand = ref new MenuCommand(commandExecute, true /*canExecute*/, m_menuCommandParam1);
            menuItem->Command = menuItemCommand;
            menuItem->CommandParameter = m_menuCommandParam1;

            clickRegistration.Attach(menuItem, ref new xaml::RoutedEventHandler([menuItemClickEvent](Platform::Object^ s, xaml::RoutedEventArgs^ e)
            {
                menuItemClickEvent->Set();
            }));
        });

        LOG_OUTPUT(L"VerifyMenuFlyoutPresenterStyle: Execute the ShowAt.");
        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"MenuItem Tap operation to execute menu item command.");
        TestServices::InputHelper->Tap(menuItem);
        menuItemClickEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            menuItem->Command = nullptr;
            menuItem->CommandParameter = nullptr;
        });
    }

    void MenuFlyoutIntegrationTests::CanClickToggleMenuFlyoutItem()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ToggleMenuFlyoutItem^ toggleMenuItem = nullptr;

        auto toggleMenuItemClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

        auto menuFlyout = CreateMenuFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);
        VERIFY_IS_NOT_NULL(menuFlyout);

        auto target = FlyoutHelper::CreateTarget(
            50 /*width*/, 50 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Right,
            xaml::VerticalAlignment::Top);
        VERIFY_IS_NOT_NULL(target);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ExecuteDelegate^ commandExecute = ref new ExecuteDelegate([&](Platform::Object^ param)
            {
                LOG_OUTPUT(L"VerifyMenuFlyoutPresenterStyle: Process the delegate!");
            });

            toggleMenuItem = safe_cast<xaml_controls::ToggleMenuFlyoutItem^>(menuFlyout->Items->GetAt(4));
            VERIFY_IS_NOT_NULL(toggleMenuItem);

            auto toggleMenuItemCommand = ref new MenuCommand(commandExecute, true /*canExecute*/, m_menuCommandParam2);
            toggleMenuItem->Command = toggleMenuItemCommand;
            toggleMenuItem->CommandParameter = m_menuCommandParam2;

            clickRegistration.Attach(toggleMenuItem, ref new xaml::RoutedEventHandler([toggleMenuItemClickEvent](Platform::Object^ s, xaml::RoutedEventArgs^ e)
            {
                toggleMenuItemClickEvent->Set();
            }));
        });

        LOG_OUTPUT(L"VerifyMenuFlyoutPresenterStyle: Execute the ShowAt.");
        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"ToggleMenuItem Tap operation to execute menu item command.");
        TestServices::InputHelper->Tap(toggleMenuItem);
        toggleMenuItemClickEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            toggleMenuItem->Command = nullptr;
            toggleMenuItem->CommandParameter = nullptr;
        });
    }

    void MenuFlyoutIntegrationTests::CanClickSplitMenuFlyoutItem()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::SplitMenuFlyoutItem^ splitMenuItem = nullptr;

        auto splitMenuItemClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::SplitMenuFlyoutItem, Click);

        auto menuFlyout = CreateMenuFlyoutWithSplitItems();
        VERIFY_IS_NOT_NULL(menuFlyout);

        auto target = FlyoutHelper::CreateTarget(
            50 /*width*/, 50 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Right,
            xaml::VerticalAlignment::Top);
        VERIFY_IS_NOT_NULL(target);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            splitMenuItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(menuFlyout->Items->GetAt(3));
            VERIFY_IS_NOT_NULL(splitMenuItem);

            ExecuteDelegate^ commandExecute = ref new ExecuteDelegate([&](Platform::Object^ param)
            {
                LOG_OUTPUT(L"CanClickSplitMenuFlyoutItem: Process the delegate!");
            });

            auto splitMenuItemCommand = ref new MenuCommand(commandExecute, true /*canExecute*/, m_menuCommandParam3);
            splitMenuItem->Command = splitMenuItemCommand;
            splitMenuItem->CommandParameter = m_menuCommandParam3;

            clickRegistration.Attach(splitMenuItem, ref new xaml::RoutedEventHandler([splitMenuItemClickEvent](Platform::Object^ s, xaml::RoutedEventArgs^ e)
            {
                splitMenuItemClickEvent->Set();
            }));
        });

        LOG_OUTPUT(L"CanClickSplitMenuFlyoutItem: Execute the ShowAt.");
        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"SplitMenuItem Tap operation to execute menu item command.");
        TestServices::InputHelper->Tap(splitMenuItem);
        splitMenuItemClickEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            splitMenuItem->Command = nullptr;
            splitMenuItem->CommandParameter = nullptr;
        });
    }

    xaml_controls::MenuFlyout^ MenuFlyoutIntegrationTests::CreateMenuFlyout(
        xaml_primitives::FlyoutPlacementMode placement)
    {
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            auto item1 = ref new xaml_controls::MenuFlyoutItem();
            item1->Text = "COPY";
            item1->Width = 250;
            item1->FontSize = 20;

            auto item2 = ref new xaml_controls::MenuFlyoutItem();
            item2->Text = "SELECT";
            item2->Width = 250;
            item2->FontSize = 20;

            auto item3 = ref new xaml_controls::MenuFlyoutItem();
            item3->Text = "PASTE";
            item3->Width = 250;
            item3->FontSize = 20;
            item3->IsEnabled = false;

            auto item4 = ref new xaml_controls::MenuFlyoutSeparator();
            item4->Width = 250;

            auto item5 = ref new xaml_controls::ToggleMenuFlyoutItem();
            item5->Text = "SMALL";
            item5->Width = 250;
            item5->FontSize = 20;

            auto item6 = ref new xaml_controls::ToggleMenuFlyoutItem();
            item6->Text = "MEDIUM";
            item6->Width = 250;
            item6->FontSize = 20;
            item6->IsChecked = true;
            item6->IsEnabled = false;

            auto item7 = ref new xaml_controls::ToggleMenuFlyoutItem();
            item7->Text = "LARGE";
            item7->Width = 250;
            item7->FontSize = 20;

            auto item8 = ref new xaml_controls::MenuFlyoutSeparator();
            item8->Width = 250;

            auto item9 = ref new xaml_controls::MenuFlyoutItem();
            item9->Text = "EXIT";
            item9->Width = 250;
            item9->FontSize = 20;

            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutPresenter";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto style = ref new xaml::Style(type);
            style->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutPresenter::TagProperty, "presenter_style"));

            menuFlyout = ref new xaml_controls::MenuFlyout();
            menuFlyout->Placement = placement;
            menuFlyout->MenuFlyoutPresenterStyle = style;

            menuFlyout->Items->Append(item1);
            menuFlyout->Items->Append(item2);
            menuFlyout->Items->Append(item3);
            menuFlyout->Items->Append(item4);
            menuFlyout->Items->Append(item5);
            menuFlyout->Items->Append(item6);
            menuFlyout->Items->Append(item7);
            menuFlyout->Items->Append(item8);
            menuFlyout->Items->Append(item9);
        });
        return menuFlyout;
    }

    xaml_controls::MenuFlyoutPresenter^ MenuFlyoutIntegrationTests::GetMenuFlyoutPresenter(
        xaml_controls::MenuFlyout^ menuFlyout)
    {
        VERIFY_IS_TRUE(menuFlyout->Items->Size > 0);
        auto item = dynamic_cast<xaml::DependencyObject^>(menuFlyout->Items->GetAt(0));
        VERIFY_IS_NOT_NULL(item);

        return TreeHelper::FindAncestor<xaml_controls::MenuFlyoutPresenter^>(item);
    }

    void MenuFlyoutIntegrationTests::ValidateShowAtTargetPosition()
    {
        TestCleanupWrapper cleanup;

        // The rules for positioning the menuflyout when calling ShowAt with Point(X,Y) are as follows:
        //   - The menu should be positioned so that (X,Y) is the Top Left corner of the menu.
        //   - For touch input (X,Y) should be the Bottom Left corner instead.
        //   - If this would place the menu so that it is vertically clipped off-screen we do the opposite of above (i.e. Bottom Left for mouse, Top Left for touch).
        //   - If the menuflyout is too tall for either of these to fit, we align it to the top of the screen.
        // We also adjust in the horizontal direction:
        //   - We try to position the menuflyout so that X is at the Left edge of the menu.
        //   - If this would result in the menu being clipped horizontally, we try position it so that X is at the Right edge.
        //   - If the menuflyout is too wide to position either its Right or Left edge at X, we align to the Left of the screen.
        //   - When in Right-To-Left mode, the logic is the same as the above, with Right and Left swapped.
        //   - When using Pen, if  SPI_GETHANDEDNESS returns "Right", then the menuflyout should show with the right edge at the X location.

        // We test the above logic by choosing values for Point(X,Y), the input mode and the FlowDirection.
        //  - Point(X,Y):
        //      This is the point passed to MenuFlyout ShowAt.
        //      Values for X are chosen to be either near the left edge of the screen, the right edge of the screen, or the center.
        //      Values for Y are chosen to be either near the top of the screen, the bottom of the screen or the center.
        //  - Input Mode:
        //      Either Mouse, Keyboard or Touch
        //      (note, we mostly test with Keyboard and Touch because Mouse input helper doesn't work on phone).
        //  - FlowDirection:
        //      Either LeftToRight or RightToLeft
        //
        // Points that are "near" a particular edge of the screen are such that the point is too close to that edge for the menuflyout to open in that direction
        //
        // For each set of values that we test we specify the expected position of the open flyout:
        //   HorizontalOpenDirection:
        //     OpenRight: The menu should open to the Right, i.e. Point(X,Y) is on the Left edge of the menu.
        //     OpenLeft: The menu should open to the Left, i.e. Point(X,Y) is on the Right edge of the menu.
        //   VerticalOpenDirection:
        //     OpenUp: the menu should open Up, i.e. Point(X,Y) is on the Bottom edge of the menu.
        //     OpenDown: the menu should open Down, i.e. Point(X,Y) is on the Top edge of the menu.

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Background = ref new SolidColorBrush(Microsoft::UI::Colors::White);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect windowBounds = {};
        RunOnUIThread([&]()
        {
            windowBounds = TestServices::WindowHelper->WindowBounds;
        });
        LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);

        float nearTop = 50;
        float nearBottom = windowBounds.Bottom - 50;
        float verticalCenter = std::floor(windowBounds.Y + (windowBounds.Height / 2));
        float nearLeft = 50;
        float nearRight = windowBounds.Right - 50;
        float horizontalCenter = std::floor(windowBounds.X + (windowBounds.Width / 2));

        // Simple Mouse case:
        if (!TestServices::Utilities->IsOneCore)
        {
            // Mouse input helper doesn't work on phone or onecore.
            LOG_OUTPUT(L"-------------");
            LOG_OUTPUT(L"TEST: Simple Mouse case");
                DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Mouse, xaml::FlowDirection::LeftToRight,
                HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);
        }

        // Simple Keyboard case:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Simple Keyboard case");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Keyboard, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Simple Touch case:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Simple Touch case");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Touch, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenUp, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near bottom, non-touch:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near bottom, non-touch");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearBottom), InputMethod::Keyboard, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenUp, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near bottom, touch:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near bottom, touch");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearBottom), InputMethod::Touch, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenUp, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near top, non-touch:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near top, non-touch");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearTop), InputMethod::Keyboard, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near top, touch:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near top, touch");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearTop), InputMethod::Touch, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near right:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near right");
        DoValidateShowAtTargetPosition(wf::Point(nearRight, verticalCenter), InputMethod::Keyboard, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near left:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near left");
        DoValidateShowAtTargetPosition(wf::Point(nearLeft, verticalCenter), InputMethod::Keyboard, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Simple RTL case:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Simple RTL case");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Keyboard, xaml::FlowDirection::RightToLeft,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near right, RTL
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near right, RTL");
        DoValidateShowAtTargetPosition(wf::Point(nearRight, verticalCenter), InputMethod::Keyboard, xaml::FlowDirection::RightToLeft,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near left, RTL
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near left, RTL");
        DoValidateShowAtTargetPosition(wf::Point(nearLeft, verticalCenter), InputMethod::Keyboard, xaml::FlowDirection::RightToLeft,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Right handed touch case");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Touch, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenUp, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // RTL touch case:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: RTL touch case");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Touch, xaml::FlowDirection::RightToLeft,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenUp, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Left handed touch case");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Touch, xaml::FlowDirection::LeftToRight,
        //    HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenUp, true /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);
    }

    void MenuFlyoutIntegrationTests::ValidateShowAtTargetPositionForPen()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect windowBounds = {};
        RunOnUIThread([&]()
        {
            windowBounds = TestServices::WindowHelper->WindowBounds;
        });
        LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);

        float nearTop = 50;
        float nearBottom = windowBounds.Bottom - 50;
        float verticalCenter = std::floor(windowBounds.Y + (windowBounds.Height / 2));
        float nearLeft = 50;
        //float nearRight = windowBounds.Right - 50; // MOCK10_REMOVAL avoid local variable is initialized but not referenced error
        float horizontalCenter = std::floor(windowBounds.X + (windowBounds.Width / 2));

        // Right handed (default) Pen cases:

        // Simple Pen case:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Simple Pen case");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near top, pen:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near top, pen");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearTop), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near bottom, pen:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near bottom, pen");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearBottom), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenUp, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // RTL cases, Pen should still open to the left

        // Simple Pen case:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Simple Pen case (RTL)");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near top, pen:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near top, pen (RTL)");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearTop), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        //// MOCK10_REMOVAL : Reenable left handed cases, now menus should open to the right.

        //// Simple Pen case:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Simple Pen case (left handed)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
        //    HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, true /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        //// Simple Pen case:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Simple Pen case (RTL, left-handed)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
        //    HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, true /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        //// Near top, pen:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Near top, pen (left-handed)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearTop), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
        //    HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, true /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near bottom, pen:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Near bottom, pen (left-handed)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearBottom), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
        //    HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenUp, true /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // RTL right-handed cases, Pen should still open to the left

        // Simple Pen case:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Simple Pen case (RTL, right-handed)");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near top, pen:
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near top, pen (RTL, right-handed)");
        DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearTop), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
            HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // Near left, pen, right-handed (should flip to right)
        LOG_OUTPUT(L"-------------");
        LOG_OUTPUT(L"TEST: Near left, pen, right-handed -- should flip to right");
        DoValidateShowAtTargetPosition(wf::Point(nearLeft, verticalCenter), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
            HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        // MOCK10_REMOVAL : Reenable near left, pen, left-handed (should flip to left)
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Near right, pen, left-handed -- should flip to left");
        //DoValidateShowAtTargetPosition(wf::Point(nearRight, verticalCenter), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
        //    HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, true /*mockLeftHandedness*/, false /*disableFullHwndSupport*/);

        //// Explicitly force popups to be not windowed even on desktop using FlyoutBase::ShouldConstrainToRootBounds and run the Pen scenarios
        //// again to exercise the non-windowed codepath.

        //// Simple Pen case:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Simple Pen case (MOCK: non-HWND path)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
        //    HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, true /*disableFullHwndSupport*/);

        //// RTL cases, Pen should still open to the left

        //// Simple Pen case:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Simple Pen case (RTL) (MOCK: non-HWND path)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
        //    HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, true /*disableFullHwndSupport*/);

        //// Near top, pen:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Near top, pen (RTL) (MOCK: non-HWND path)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, nearTop), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
        //    HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, true /*disableFullHwndSupport*/);

        //// Change handedness to left, now menus should open to the right.

        //// Simple Pen case:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Simple Pen case (left handed) (MOCK: non-HWND path)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
        //    HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, true /*mockLeftHandedness*/, true /*disableFullHwndSupport*/);

        //// Simple Pen case:
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Simple Pen case (RTL, left-handed) (MOCK: non-HWND path)");
        //DoValidateShowAtTargetPosition(wf::Point(horizontalCenter, verticalCenter), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
        //    HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, true /*mockLeftHandedness*/, true /*disableFullHwndSupport*/);

        //// Near left, pen, right-handed (should flip to right)
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Near left, pen, right-handed -- should flip to right (MOCK: non-HWND path)");
        //DoValidateShowAtTargetPosition(wf::Point(nearLeft, verticalCenter), InputMethod::Pen, xaml::FlowDirection::LeftToRight,
        //    HorizontalOpenDirection::OpenRight, VerticalOpenDirection::OpenDown, false /*mockLeftHandedness*/, true /*disableFullHwndSupport*/);

        //// Near left, pen, left-handed (should flip to left)
        //LOG_OUTPUT(L"-------------");
        //LOG_OUTPUT(L"TEST: Near right, pen, left-handed -- should flip to left (MOCK: non-HWND path)");
        //DoValidateShowAtTargetPosition(wf::Point(nearRight, verticalCenter), InputMethod::Pen, xaml::FlowDirection::RightToLeft,
        //    HorizontalOpenDirection::OpenLeft, VerticalOpenDirection::OpenDown, true /*mockLeftHandedness*/, true /*disableFullHwndSupport*/);
    }

    void MenuFlyoutIntegrationTests::InjectInput(InputMethod inputMethod) {
        xaml::FrameworkElement^ rootPanel;
        RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml::FrameworkElement^>(TestServices::WindowHelper->WindowContent);
            });

        // Send some input of the appropriate type to the app.
        // The placement of the menuflyout is affected by the most recently used input device type.
        // Note: it is NOT sufficient at this time to simply set the last input type.  This is becuase the flyout present will explicitly query
        //       the popup window to determine whether the keyboard focus or accelerators are being shown.  If they are, last input type is forced
        //       to keyboard and if they aren't and last input type is keyboard, it will be changed to none.  So, make sure we do real input.
        if (inputMethod == InputMethod::Mouse)
        {
            LOG_OUTPUT(L"Calling InputHelper->LeftMouseClick");
            TestServices::InputHelper->LeftMouseClick(rootPanel);
        }
        else if (inputMethod == InputMethod::Touch)
        {
            LOG_OUTPUT(L"Calling InputHelper->Tap");
            TestServices::InputHelper->Tap(rootPanel);
        }
        else if (inputMethod == InputMethod::Keyboard)
        {
            LOG_OUTPUT(L"Calling KeyboardHelper->PressKeySequence");
            TestServices::KeyboardHelper->PressKeySequence(" ");
        }
        else if (inputMethod == InputMethod::Pen)
        {
            LOG_OUTPUT(L"Calling InputHelper->PenTap");
            TestServices::InputHelper->PenTap(rootPanel);
        }
        else
        {
            WEX::Common::Throw::Exception(E_NOTIMPL);
        }
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::DoValidateShowAtTargetPosition(
        wf::Point showAtPosition,
        InputMethod inputMethod,
        xaml::FlowDirection flowDirection,
        HorizontalOpenDirection expectedHorizontalOpenDirection,
        VerticalOpenDirection expectedVerticalOpenDirection,
        bool /*mockLeftHandedness*/,
        bool /*disableFullHwndSupport*/)
    {
        /* MOCK10_REMOVAL : Reenable when mock support is available
        typedef Mock10::MockFunction<BOOL NTAPI(UINT, UINT, PVOID, UINT)>::Prototype SystemParametersInfoWPrototype;

        DWORD dwMockHandedness = mockLeftHandedness ? HANDEDNESS_LEFT : HANDEDNESS_RIGHT;

        // Within this scope, SystemParametersInfo is mocked
        Mock10::MockFunction<SystemParametersInfoWPrototype> functionSystemParametersInfo(Mock10::Mock::Function<SystemParametersInfoWPrototype>(SystemParametersInfoW));
        functionSystemParametersInfo.Set(
            [&](UINT uiAction,
                UINT uiParam,
                PVOID pvParam,
                UINT fWinIni) -> BOOL volatile
        {
            if (uiAction == SPI_GETHANDEDNESS)
            {
                *static_cast<LPDWORD>(pvParam) = dwMockHandedness;
                return TRUE;
            }

            return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
        });
        */

        auto menuFlyout = CreateMenuFlyout();

        xaml::FrameworkElement^ rootPanel;
        wf::Rect windowBounds;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml::FrameworkElement^>(TestServices::WindowHelper->WindowContent);
            rootPanel->FlowDirection = flowDirection;
            menuFlyout->XamlRoot = rootPanel->XamlRoot;
            windowBounds = TestServices::WindowHelper->WindowBounds;

            // TODO: Set menuFlyout->ShouldConstrainToRootBounds based on disableFullHwndSupport
            // to handle windowed popups.
        });
        TestServices::WindowHelper->WaitForIdle();

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        openedRegistration.Attach(menuFlyout, [&](){ openedEvent->Set(); });

        // Make sure our last input is of the correct type.
        InjectInput(inputMethod);

        // Show the MenuFlyout:
        RunOnUIThread([&]()
        {
            menuFlyout->ShowAt(nullptr, showAtPosition);
        });
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetOpenFlyoutPresenter();
            auto menuFlyoutBounds = ControlHelper::GetBounds(flyoutPresenter);

            LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);
            LOG_OUTPUT(L"showAtPosition (%f,%f)", showAtPosition.X, showAtPosition.Y);

            VERIFY_IS_TRUE(ControlHelper::IsContainedIn(menuFlyoutBounds, windowBounds));

            // Validate Horizontal position:
            if (expectedHorizontalOpenDirection == HorizontalOpenDirection::OpenRight)
            {
                VERIFY_IS_LESS_THAN(menuFlyoutBounds.Left, showAtPosition.X + 1);
                VERIFY_IS_GREATER_THAN(menuFlyoutBounds.Left, showAtPosition.X - 1);
            }
            else if (expectedHorizontalOpenDirection == HorizontalOpenDirection::OpenLeft)
            {
                VERIFY_IS_LESS_THAN(menuFlyoutBounds.Right, showAtPosition.X + 1);
                VERIFY_IS_GREATER_THAN(menuFlyoutBounds.Right, showAtPosition.X - 1);
            }

            // Validate Vertical position:
            if (expectedVerticalOpenDirection == VerticalOpenDirection::OpenDown)
            {
                VERIFY_IS_LESS_THAN(menuFlyoutBounds.Top, showAtPosition.Y + 1);
                VERIFY_IS_GREATER_THAN(menuFlyoutBounds.Top, showAtPosition.Y - 1);
            }
            else if (expectedVerticalOpenDirection == VerticalOpenDirection::OpenUp)
            {
                VERIFY_IS_LESS_THAN(menuFlyoutBounds.Bottom, showAtPosition.Y + 1);
                VERIFY_IS_GREATER_THAN(menuFlyoutBounds.Bottom, showAtPosition.Y - 1);
            }
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateShowAtTargetPositionRelativeToElement()
    {
        // When a UIElement is passed to MenuFlyout->ShowAt(UIElement, Point), we verify that the Point is transformed from the UIElement's space to
        // the global space for the positioning of the MenuFlyout.
        TestCleanupWrapper cleanup;

        auto menuFlyout = CreateMenuFlyout();

        xaml_controls::Button^ button1;
        wf::Point showAtPositionRelative = wf::Point(50, 50); // This is the position we pass to ShowAt. It is relative to button1.
        wf::Point showAtPositionAbsolute; // The showAtPositionRelative point in absolute space.
        // We pass showAtPositionRelative to ShowAt and expect the menuflyout to be positioned at showAtPositionAbsolute.

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid Background="Orange"
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                      <Button x:Name="button1" Content="Button" Width="100" Height="50" HorizontalAlignment="Left" VerticalAlignment="Top" Margin="50, 200, 0, 0" />
                    </Grid>)"));

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, showAtPositionRelative.X, showAtPositionRelative.Y, true);

        RunOnUIThread([&]()
        {
            showAtPositionAbsolute = button1->TransformToVisual(nullptr)->TransformPoint(showAtPositionRelative);

            auto flyoutPresenter = FlyoutHelper::GetOpenFlyoutPresenter();
            auto menuFlyoutBounds = ControlHelper::GetBounds(flyoutPresenter);

            VERIFY_ARE_EQUAL(menuFlyoutBounds.Left, showAtPositionAbsolute.X);
            VERIFY_ARE_EQUAL(menuFlyoutBounds.Bottom, showAtPositionAbsolute.Y);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::TallMenuFlyoutShouldAlignToTopOfScreen()
    {
        TestCleanupWrapper cleanup;

        if (!TestServices::Utilities->IsDesktop)
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
        }

        // MenuFlyout will normally align either it's top or bottom edge to the point passed to ShowAt.
        // But if the MenuFlyout is too tall to fit in either of these positions, it aligns to the top of the screen.
        //
        // To test this:
        // We set the height of a menuflyoutitem so that the menuflyout size is greater than half the height of the screen.
        // We call ShowAt with a point in the middle of the screen.
        // This ensures that it can not align either it's top or bottom edge to the given point without getting clipped.
        // We validate that the menuflyout opens aligned to the top of the screen.

        auto menuFlyout = CreateMenuFlyout();

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
        });
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect windowBounds = {};
        RunOnUIThread([&]()
        {
            windowBounds = TestServices::WindowHelper->WindowBounds;
            menuFlyout->Items->GetAt(0)->Height = windowBounds.Height * 0.75;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, nullptr, windowBounds.Width / 2, windowBounds.Height / 2);

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetOpenFlyoutPresenter();
            auto menuFlyoutBounds = ControlHelper::GetBounds(flyoutPresenter);

            // Verify that the top of the menuflyout is at the top of the screen.
            VERIFY_ARE_EQUAL(menuFlyoutBounds.Top, -windowBounds.Y);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::WideMenuFlyoutShouldAlignToLeftOfScreen()
    {
        TestCleanupWrapper cleanup;

        // Similar to TallMenuFlyoutShouldAlignToTopOfScreen, if a MenuFlyout is too wide to open at the point passed to ShowAt, it will
        // open aligned to the left instead.

        xaml_controls::MenuFlyout^ menuFlyout;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem;

        RunOnUIThread([&]()
        {
            // We need to set MenuFlyoutPresenter.MaxWidth to infinity, otherwise its default value may prevent us from creating
            // a menuflyout wide enough to hit the scenario we are trying to test.
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <Grid.Resources>
                            <MenuFlyout x:Name="menuFlyout">
                                <MenuFlyout.MenuFlyoutPresenterStyle>
                                    <Style TargetType="MenuFlyoutPresenter">
                                        <Setter Property="MaxWidth" Value="Infinity" />
                                    </Style>
                                </MenuFlyout.MenuFlyoutPresenterStyle>
                                <MenuFlyoutItem x:Name="menuFlyoutItem" Text="MenuFlyoutItem" />
                            </MenuFlyout>
                        </Grid.Resources>
                    </Grid>)"));

            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(rootPanel->FindName(L"menuFlyout"));
            menuFlyoutItem = safe_cast<xaml_controls::MenuFlyoutItem^>(rootPanel->FindName(L"menuFlyoutItem"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect windowBounds = {};
        RunOnUIThread([&]()
        {
            windowBounds = TestServices::WindowHelper->WindowBounds;
            menuFlyoutItem->Width = windowBounds.Width * 0.75;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, nullptr, windowBounds.Width / 2, windowBounds.Height / 2);

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetOpenFlyoutPresenter();
            auto menuFlyoutBounds = ControlHelper::GetBounds(flyoutPresenter);

            // Verify that the left of the menuflyout is at the left of the screen.
            VERIFY_ARE_EQUAL(menuFlyoutBounds.Left, -windowBounds.X);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    xaml_controls::MenuFlyout^ MenuFlyoutIntegrationTests::CreateMenuFlyout()
    {
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            auto item1 = ref new xaml_controls::MenuFlyoutItem();
            item1->Text = "Menu Item1";
            item1->Tag = "MFI1";

            auto item2 = ref new xaml_controls::MenuFlyoutItem();
            item2->Text = "Menu Item2";
            item2->Tag = "MFI2";
            item2->IsEnabled = false;

            auto item3 = ref new xaml_controls::MenuFlyoutSeparator();
            item3->Tag = "MFS1";

            auto subItem = ref new xaml_controls::MenuFlyoutSubItem();
            subItem->Text = "Menu SubItem";
            subItem->Tag = "MFSI1";

            auto subItemItem1 = ref new xaml_controls::MenuFlyoutItem();
            subItemItem1->Text = "Menu SubItem Item1";
            subItemItem1->Tag = "MFSII1";

            auto subItemItem2 = ref new xaml_controls::MenuFlyoutItem();
            subItemItem2->Text = "Menu SubItem Item2";
            subItemItem2->Tag = "MFSII2";

            subItem->Items->Append(subItemItem1);
            subItem->Items->Append(subItemItem2);

            menuFlyout = ref new xaml_controls::MenuFlyout();
            menuFlyout->Items->Append(item1);
            menuFlyout->Items->Append(item2);
            menuFlyout->Items->Append(item3);
            menuFlyout->Items->Append(subItem);
        });

        return menuFlyout;
    }

    xaml_controls::MenuFlyout^ MenuFlyoutIntegrationTests::CreateMenuFlyoutWithSubItem()
    {
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            auto item1 = ref new xaml_controls::MenuFlyoutItem();
            item1->Text = "Menu Item1";

            auto item2 = ref new xaml_controls::MenuFlyoutSubItem();
            item2->Text = "Menu Item2";

            auto item3 = ref new xaml_controls::MenuFlyoutItem();
            item3->Text = "Menu Item3";

            item2->Items->Append(item3);

            menuFlyout = ref new xaml_controls::MenuFlyout();
            menuFlyout->Items->Append(item1);
            menuFlyout->Items->Append(item2);
        });

        return menuFlyout;
    }

    xaml_controls::MenuFlyout^ MenuFlyoutIntegrationTests::CreateMenuFlyoutWithSplitItems()
    {
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = ref new xaml_controls::MenuFlyout();

            auto item1 = ref new xaml_controls::MenuFlyoutItem();
            item1->Text = "Menu Item 1";
            item1->Tag = "MFI1";
            menuFlyout->Items->Append(item1);

            auto item2 = ref new xaml_controls::MenuFlyoutItem();
            item2->Text = "Menu Item 2";
            item2->Tag = "MFI2";
            item2->IsEnabled = false;
            menuFlyout->Items->Append(item2);

            auto item3 = ref new xaml_controls::MenuFlyoutSeparator();
            item3->Tag = "MFS1";
            menuFlyout->Items->Append(item3);

            auto splitItem = ref new xaml_controls::SplitMenuFlyoutItem();
            splitItem->Text = "Split Menu Item 1";
            splitItem->Tag = "SMFI1";
            splitItem->Width = 250;

            auto splitItemSubItem1 = ref new xaml_controls::MenuFlyoutItem();
            splitItemSubItem1->Text = "SubItem 1.1";
            splitItemSubItem1->Tag = "SMFISI1";

            auto splitItemSubItem2 = ref new xaml_controls::MenuFlyoutItem();
            splitItemSubItem2->Text = "SubItem 1.2";
            splitItemSubItem2->Tag = "SMFISI2";

            splitItem->Items->Append(splitItemSubItem1);
            splitItem->Items->Append(splitItemSubItem2);

            menuFlyout->Items->Append(splitItem);
        });

        return menuFlyout;
    }

    void MenuFlyoutIntegrationTests::ValidateOnlyOneSubMenuItemIsOpenAtATimeByTouch()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        auto subItem1LostFocusEvent = std::make_shared<Event>();
        auto subItem1LostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutSubItem, LostFocus);

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutSubItem x:Name='subItem1' Text='Menu sub item 1'>"
                L"        <MenuFlyoutItem>Menu item 1.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Menu item 1.2</MenuFlyoutItem>"
                L"    </MenuFlyoutSubItem>"
                L"    <MenuFlyoutSubItem x:Name='subItem2' Text='Menu sub item 2'>"
                L"        <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"    </MenuFlyoutSubItem>"
                L"</MenuFlyout>"));

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        ShowMenuFlyout(menuFlyout, button1, -50, 50);

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        // Get sub menu items 1, set up event registration, and tap on it
        subItem = GetSubItem(items, 0);
        RunOnUIThread([&]()
        {
            subItem1LostFocusRegistration.Attach(subItem,[subItem1LostFocusEvent]()
            {
                subItem1LostFocusEvent->Set();
            });
        });
        TapSubMenuItem(subItem);

        // Tap sub menu item 2 and verify that subItem1 is closed
        subItem = GetSubItem(items, 1);
        TapSubMenuItem(subItem);
        subItem1LostFocusEvent->WaitForDefault();

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateOnlyOneSplitMenuItemIsOpenAtATimeByTouch()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        auto splitItem1LostFocusEvent = std::make_shared<Event>();
        auto splitItem1LostFocusRegistration = CreateSafeEventRegistration(xaml_controls::SplitMenuFlyoutItem, LostFocus);

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split menu item 1'>"
                L"        <MenuFlyoutItem>Menu item 1.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Menu item 1.2</MenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"    <SplitMenuFlyoutItem x:Name='splitItem2' Text='Split menu item 2'>"
                L"        <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"</MenuFlyout>"));

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        ShowMenuFlyout(menuFlyout, button1, -50, 50);

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        // Get split menu item 1, set up event registration, and tap on its secondary part
        RunOnUIThread([&]()
        {
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(0));
            splitItem1LostFocusRegistration.Attach(splitItem,[splitItem1LostFocusEvent]()
            {
                splitItem1LostFocusEvent->Set();
            });
        });
        TapSplitMenuItemSecondary(splitItem);

        // Tap split menu item 2's secondary part and verify that splitItem1 is closed
        RunOnUIThread([&]()
        {
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(1));
        });
        TapSplitMenuItemSecondary(splitItem);
        splitItem1LostFocusEvent->WaitForDefault();

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemByGamepad()
    {
        PerformValidateSubMenuItem(InputMethod::Gamepad);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemByKeyboard()
    {
        PerformValidateSubMenuItem(InputMethod::Keyboard);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemByMouse()
    {
        PerformValidateSubMenuItem(InputMethod::Mouse);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemByRemote()
    {
        PerformValidateSubMenuItem(InputMethod::Remote);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemByTouch()
    {
        PerformValidateSubMenuItem(InputMethod::Touch);
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemByGamepad()
    {
        PerformValidateSplitMenuItem(InputMethod::Gamepad);
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemByKeyboard()
    {
        PerformValidateSplitMenuItem(InputMethod::Keyboard);
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemByMouse()
    {
        PerformValidateSplitMenuItem(InputMethod::Mouse);
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemByTouch()
    {
        PerformValidateSplitMenuItem(InputMethod::Touch);
    }

    void MenuFlyoutIntegrationTests::ValidateTraverseMenuFlyoutItemsByGamepad()
    {
        PerformValidateTraverseMenuFlyoutItems(InputMethod::Gamepad, true);
        PerformValidateTraverseMenuFlyoutItems(InputMethod::Gamepad, false);
    }

    void MenuFlyoutIntegrationTests::ValidateTraverseMenuFlyoutItemsByKeyboard()
    {
        PerformValidateTraverseMenuFlyoutItems(InputMethod::Keyboard, true);
        PerformValidateTraverseMenuFlyoutItems(InputMethod::Keyboard, false);
    }

    void MenuFlyoutIntegrationTests::ValidateTraverseSplitMenuFlyoutItemsByGamepad()
    {
        PerformValidateTraverseSplitMenuFlyoutItems(InputMethod::Gamepad, true);
        PerformValidateTraverseSplitMenuFlyoutItems(InputMethod::Gamepad, false);
    }

    void MenuFlyoutIntegrationTests::ValidateTraverseSplitMenuFlyoutItemsByKeyboard()
    {
        PerformValidateTraverseSplitMenuFlyoutItems(InputMethod::Keyboard, true);
        PerformValidateTraverseSplitMenuFlyoutItems(InputMethod::Keyboard, false);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemInPage()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;

        xaml_controls::Page^ page = nullptr;
        xaml_controls::Canvas^ rootPanel = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutSubItemsFromXaml();

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);

        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            rootPanel = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                L"<Canvas Background='RoyalBlue' "
                L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <Button x:Name='button1' Content='Button' Width='100' Height='50' Canvas.Left='50' Canvas.Top='50' />"
                L"</Canvas>"));
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));

            page->Content = rootPanel;
            loadedRegistration.Attach(page, [loadedEvent]() { loadedEvent->Set(); });
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, -50, 50);
        NavigateSubMenu(menuFlyout, button1, InputDevice::Keyboard);
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemInPage()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;

        xaml_controls::Page^ page = nullptr;
        xaml_controls::Canvas^ rootPanel = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = CreateSplitMenuFlyoutItemsFromXaml();

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);

        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            rootPanel = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                L"<Canvas Background='RoyalBlue' "
                L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <Button x:Name='button1' Content='Button' Width='100' Height='50' Canvas.Left='50' Canvas.Top='50' />"
                L"</Canvas>"));
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));

            page->Content = rootPanel;
            loadedRegistration.Attach(page, [loadedEvent]() { loadedEvent->Set(); });
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, -50, 50);
        NavigateSplitMenu(menuFlyout, button1, InputDevice::Keyboard);
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateCollapsingResetsVisualState()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::MenuFlyout^ menuFlyout;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem;
        xaml_controls::Grid^ menuFlyoutItemLayoutRoot;
        xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem;
        xaml_controls::Grid^ menuFlyoutSubItemLayoutRoot;
        xaml_controls::SplitMenuFlyoutItem^ splitMenuFlyoutItem;

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Button^ rootButton = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            menuFlyout = ref new xaml_controls::MenuFlyout();

            menuFlyoutItem = ref new xaml_controls::MenuFlyoutItem();
            menuFlyoutItem->Text = "Item 1";
            menuFlyout->Items->Append(menuFlyoutItem);

            menuFlyoutSubItem = ref new xaml_controls::MenuFlyoutSubItem();
            menuFlyoutSubItem->Text = "Item 2";
            menuFlyout->Items->Append(menuFlyoutSubItem);

            splitMenuFlyoutItem = ref new xaml_controls::SplitMenuFlyoutItem();
            splitMenuFlyoutItem->Text = "Item 3";
            menuFlyout->Items->Append(splitMenuFlyoutItem);

            rootButton = ref new xaml_controls::Button();
            rootButton->Flyout = menuFlyout;
            rootPanel->Children->Append(rootButton);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, rootButton, 0, 0, false /* forceTapAsPreviousInputMessage */);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(menuFlyoutItem, L"CommonStates", L"Normal"));

            VisualStateManager::GoToState(menuFlyoutItem, "Pressed", false);
            VisualStateManager::GoToState(menuFlyoutSubItem, "Pressed", false);
            VisualStateManager::GoToState(splitMenuFlyoutItem, "Pressed", false);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(menuFlyoutItem, L"CommonStates", L"Pressed"));

            menuFlyoutItem->Visibility = xaml::Visibility::Collapsed;
            menuFlyoutSubItem->Visibility = xaml::Visibility::Collapsed;
            splitMenuFlyoutItem->Visibility = xaml::Visibility::Collapsed;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(menuFlyoutItem, L"CommonStates", L"Normal"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(menuFlyoutSubItem, L"CommonStates", L"Normal"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(splitMenuFlyoutItem, L"CommonStates", L"Normal"));
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    xaml_controls::MenuFlyoutSubItem^ MenuFlyoutIntegrationTests::CreateMenuFlyoutSubItem()
    {
        auto subItem = ref new xaml_controls::MenuFlyoutSubItem();
        subItem->Text = "Sub Item";

        auto item1 = ref new xaml_controls::MenuFlyoutItem();
        item1->Text = "Item1";

        auto item2 = ref new xaml_controls::MenuFlyoutItem();
        item2->Text = "Item2";

        auto item3 = ref new xaml_controls::MenuFlyoutSeparator();

        auto item4 = ref new xaml_controls::ToggleMenuFlyoutItem();
        item4->Text = "Item4";

        subItem->Items->Append(item1);
        subItem->Items->Append(item2);
        subItem->Items->Append(item3);
        subItem->Items->Append(item4);

        return subItem;
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemPosition()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1024, 768));

        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ subItems = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem = nullptr;
        wf::Rect windowBounds = {};
        wf::Rect menuFlyoutBounds = {};
        wf::Rect subMenu1tBounds = {};
        wf::Rect subMenu2tBounds = {};

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <MenuFlyoutSubItem x:Name='subItem1' Text='sub item4'>"
                L"        <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <MenuFlyoutSubItem  x:Name='subItem2' Text='subItem2'>"
                L"            <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"            <MenuFlyoutSubItem x:Name='subItem1' Text='subitem3'/>"
                L"        </MenuFlyoutSubItem>"
                L"    </MenuFlyoutSubItem>"
                L"</MenuFlyout>"));

            windowBounds = TestServices::WindowHelper->WindowBounds;

            LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);
        });

        ShowMenuFlyout(menuFlyout, nullptr, windowBounds.Width / 4, 50);

        auto presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);
        });

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        subItem = GetSubItem(items);
        TapSubMenuItem(subItem);

        // Verify the sub menu1 position that it must be positioned to the left side of the MenuFlyout.
        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenu1tBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SubMenuFlyout bounds left=%f top=%f width=%f height=%f", subMenu1tBounds.Left, subMenu1tBounds.Top, subMenu1tBounds.Width, subMenu1tBounds.Height);
        });
        VERIFY_IS_TRUE(menuFlyoutBounds.Left < subMenu1tBounds.Left);

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            subItems = subItem->Items;
        });

        TapSubMenuItem(GetSubItem(subItems));

        // Verify the sub menu2 position that it must be positioned to the right side of the sub menu2 flyout.
        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenu2tBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SubMenuFlyout bounds left=%f top=%f width=%f height=%f", subMenu2tBounds.Left, subMenu2tBounds.Top, subMenu2tBounds.Width, subMenu2tBounds.Height);
        });
        VERIFY_IS_TRUE(subMenu1tBounds.Left <= subMenu2tBounds.Left);

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemPosition()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1024, 768));

        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ subItems = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;
        wf::Rect windowBounds = {};
        wf::Rect menuFlyoutBounds = {};
        wf::Rect subMenu1tBounds = {};
        wf::Rect subMenu2tBounds = {};

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <SplitMenuFlyoutItem x:Name='splitItem1' Text='split item4'>"
                L"        <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <SplitMenuFlyoutItem  x:Name='splitItem2' Text='splitItem2'>"
                L"            <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"            <SplitMenuFlyoutItem x:Name='splitItem3' Text='splititem3'/>"
                L"        </SplitMenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"</MenuFlyout>"));

            windowBounds = TestServices::WindowHelper->WindowBounds;

            LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);
        });

        ShowMenuFlyout(menuFlyout, nullptr, windowBounds.Width / 4, 50);

        auto presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);
        });

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
            // SplitMenuFlyoutItem is at index 2 (after item1 and separator)
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(2));
        });

        VERIFY_IS_NOT_NULL(splitItem);
        TapSplitMenuItemSecondary(splitItem);

        // Verify the split menu1 position that it must be positioned to the right side of the MenuFlyout.
        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenu1tBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SplitMenuFlyout bounds left=%f top=%f width=%f height=%f", subMenu1tBounds.Left, subMenu1tBounds.Top, subMenu1tBounds.Width, subMenu1tBounds.Height);
        });
        VERIFY_IS_TRUE(menuFlyoutBounds.Left < subMenu1tBounds.Left);

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            subItems = splitItem->Items;
            // Nested SplitMenuFlyoutItem is at index 2 (after item1 and separator)
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(subItems->GetAt(2));
        });

        VERIFY_IS_NOT_NULL(splitItem);
        TapSplitMenuItemSecondary(splitItem);

        // Verify the split menu2 position that it must be positioned to the right side of the split menu1 flyout.
        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenu2tBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SplitMenuFlyout2 bounds left=%f top=%f width=%f height=%f", subMenu2tBounds.Left, subMenu2tBounds.Top, subMenu2tBounds.Width, subMenu2tBounds.Height);
        });
        VERIFY_IS_TRUE(subMenu1tBounds.Left <= subMenu2tBounds.Left);

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateNestedSubMenuItemPosition()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1024, 768));

        wf::Rect windowBounds = {};
        wf::Rect menuFlyoutBounds = {};
        wf::Rect subMenuBounds = {};

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ subItems = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutSubItem x:Name='item1' Text='item1'>"
                L"        <MenuFlyoutSubItem x:Name='item11' Text='item11'>"
                L"            <MenuFlyoutItem x:Name='item111'>item111</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item112'>item112</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item113'>item113</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item114'>item114</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item115'>item115</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item116'>item116</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item117'>item117</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item118'>item118</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item119'>item119</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11A'>item11A</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11B'>item11B</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11C'>item11C</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11D'>item11D</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11E'>item11E</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11F'>item11F</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11G'>item11G</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11H'>item11H</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11I'>item11I</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11H'>item11J</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11I'>item11K</MenuFlyoutItem>"
                L"        </MenuFlyoutSubItem>"
                L"    </MenuFlyoutSubItem>"
                L"    <MenuFlyoutItem x:Name='item2'>item2</MenuFlyoutItem>"
                L"    <MenuFlyoutItem x:Name='item3'>item3</MenuFlyoutItem>"
                L"</MenuFlyout>"));

            windowBounds = TestServices::WindowHelper->WindowBounds;

            LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);
            VERIFY_IS_TRUE(windowBounds.Width == 1024);
            VERIFY_IS_TRUE(windowBounds.Height == 768);
        });

        ShowMenuFlyout(menuFlyout, nullptr, windowBounds.Width / 4, windowBounds.Height);

        auto presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);
        });

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        subItem = GetSubItem(items, 0);
        TapSubMenuItem(subItem);

        presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            subMenuBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SubMenuFlyout bounds left=%f top=%f width=%f height=%f", subMenuBounds.Left, subMenuBounds.Top, subMenuBounds.Width, subMenuBounds.Height);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            subItems = subItem->Items;
        });

        TapSubMenuItem(GetSubItem(subItems, 0));

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenuBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SubMenuFlyout bounds left=%f top=%f width=%f height=%f", subMenuBounds.Left, subMenuBounds.Top, subMenuBounds.Width, subMenuBounds.Height);
        });
        // Verify the sub menu is positioned towards the top of the window and not the bottom.
        VERIFY_IS_TRUE(subMenuBounds.Top <= 0);

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateNestedSplitMenuItemPosition()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1024, 768));

        wf::Rect windowBounds = {};
        wf::Rect menuFlyoutBounds = {};
        wf::Rect splitMenuBounds = {};

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ subItems = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <SplitMenuFlyoutItem x:Name='item1' Text='item1'>"
                L"        <SplitMenuFlyoutItem x:Name='item11' Text='item11'>"
                L"            <MenuFlyoutItem x:Name='item111'>item111</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item112'>item112</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item113'>item113</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item114'>item114</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item115'>item115</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item116'>item116</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item117'>item117</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item118'>item118</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item119'>item119</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11A'>item11A</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11B'>item11B</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11C'>item11C</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11D'>item11D</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11E'>item11E</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11F'>item11F</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11G'>item11G</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11H'>item11H</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11I'>item11I</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11J'>item11J</MenuFlyoutItem>"
                L"            <MenuFlyoutItem x:Name='item11K'>item11K</MenuFlyoutItem>"
                L"        </SplitMenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"    <MenuFlyoutItem x:Name='item2'>item2</MenuFlyoutItem>"
                L"    <MenuFlyoutItem x:Name='item3'>item3</MenuFlyoutItem>"
                L"</MenuFlyout>"));

            windowBounds = TestServices::WindowHelper->WindowBounds;

            LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);
            VERIFY_IS_TRUE(windowBounds.Width == 1024);
            VERIFY_IS_TRUE(windowBounds.Height == 768);
        });

        ShowMenuFlyout(menuFlyout, nullptr, windowBounds.Width / 4, windowBounds.Height);

        auto presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);
        });

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(0));
        });

        VERIFY_IS_NOT_NULL(splitItem);
        TapSplitMenuItemSecondary(splitItem);

        presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            splitMenuBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SplitMenuFlyout bounds left=%f top=%f width=%f height=%f", splitMenuBounds.Left, splitMenuBounds.Top, splitMenuBounds.Width, splitMenuBounds.Height);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            subItems = splitItem->Items;
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(subItems->GetAt(0));
        });

        VERIFY_IS_NOT_NULL(splitItem);
        TapSplitMenuItemSecondary(splitItem);

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            splitMenuBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SplitMenuFlyout bounds left=%f top=%f width=%f height=%f", splitMenuBounds.Left, splitMenuBounds.Top, splitMenuBounds.Width, splitMenuBounds.Height);
        });
        // Verify the split menu is positioned towards the top of the window and not the bottom.
        VERIFY_IS_TRUE(splitMenuBounds.Top <= 0);

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemUIElementTree()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::Button^ button1 = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutSubItemsFromXaml();

        RunOnUIThread([&]()
        {
            menuFlyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        // Tap on the button to ensure the button's initial focus state
        TestServices::InputHelper->Tap(button1);
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button1, -50, 50);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        auto subItem = GetSubItem(items);
        TapSubMenuItem(subItem);
        TestServices::WindowHelper->WaitForIdle();

        if (TestServices::Utilities->IsOneCore)
        {
            TestServices::Utilities->VerifyUIElementTree("WindowlessPopup");
        }
        else
        {
            TestServices::Utilities->VerifyUIElementTree("WindowedPopup");
        }

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidatePopupWindowedPosition()
    {
        ValidatePopupWindowedPosition(true /* expectWindowedPopup */);
    }

    void MenuFlyoutIntegrationTests::ValidatePopupWindowedPositionInSimulatedHolographic()
    {
        HolographicOverride holographicOverride;
        ValidatePopupWindowedPosition(false /* expectWindowedPopup */);

    }

    void MenuFlyoutIntegrationTests::ValidateWindowedPositionNearMonitorEdge()
    {
        TestCleanupWrapper cleanup([&]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        wf::Rect menuFlyoutBounds = {};
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();

        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutSubItemsFromXaml();

        wf::Rect monitor = TestServices::WindowHelper->MonitorBounds;
        LOG_OUTPUT(L"Monitor size: %f, %f", monitor.Width, monitor.Height);

        LOG_OUTPUT(L"Calling ShowAt %f, %f", monitor.Width - 10, monitor.Height - 50);
        ShowMenuFlyout(menuFlyout, nullptr, monitor.Width - 10, monitor.Height - 50);

        auto presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            menuFlyoutBounds = ControlHelper::GetBounds(presenter);
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);
            VERIFY_IS_LESS_THAN(monitor.Width - 260, menuFlyoutBounds.Left);
            VERIFY_IS_GREATER_THAN(monitor.Width - 240, menuFlyoutBounds.Left);
        });

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        auto subItem = GetSubItem(items);
        TapSubMenuItem(subItem);

        // Validate the position
        auto subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            wf::Rect subMenu = ControlHelper::GetBounds(subPresenter);
            LOG_OUTPUT(L"Sub Menu bounds left=%f top=%f width=%f height=%f", subMenu.Left, subMenu.Top, subMenu.Width, subMenu.Height);
            VERIFY_IS_LESS_THAN(monitor.Width - 540, subMenu.Left);
            VERIFY_IS_GREATER_THAN(monitor.Width - 440, subMenu.Left);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }


    void MenuFlyoutIntegrationTests::ValidatePopupWindowedPosition(bool expectWindowedPopup)
    {
        TestCleanupWrapper cleanup([&]()
        {
            if (expectWindowedPopup)
            {
                TestServices::WindowHelper->MaximizeDesktopWindow();
                TestServices::WindowHelper->WaitForIdle();
            }

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        // If we're expecting a windowed popup, we'll set the desktop window size
        // in order to test the popup breaking out of the bounds of the desktop window.
        // Otherwise, we'll just set the window size in XAML.
        if (!expectWindowedPopup)
        {
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));
        }

        wf::Rect menuFlyoutBounds = {};
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();

        if (expectWindowedPopup)
        {
            TestServices::WindowHelper->SetDesktopWindowSize(400, 600);
            TestServices::WindowHelper->WaitForIdle();
        }

        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutSubItemsFromXaml();

        ShowMenuFlyout(menuFlyout, nullptr, 400, 280);

        auto presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            menuFlyoutBounds = ControlHelper::GetBounds(presenter);
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);

            if(expectWindowedPopup)
            {
                VERIFY_IS_TRUE(menuFlyoutBounds.Left == 400);
            }
            else
            {
                // It is not at 400 because the menu needs to shift left to fit inside the app window.
                // This is similar for the checks below that do not expectWindowedPopup, the popup needs
                // to be adjusted to fit in the windowed dimensions.
                VERIFY_IS_FALSE(menuFlyoutBounds.Left == 400);
            }
        });

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        auto subItem = GetSubItem(items);
        TapSubMenuItem(subItem);

        // Validate the position
        auto subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            wf::Rect subMenu = ControlHelper::GetBounds(subPresenter);
            LOG_OUTPUT(L"Sub Menu bounds left=%f top=%f width=%f height=%f", subMenu.Left, subMenu.Top, subMenu.Width, subMenu.Height);

            if(expectWindowedPopup)
            {
                VERIFY_IS_TRUE(subMenu.Left > 550);
                VERIFY_IS_TRUE(subMenu.Top + subMenu.Bottom > menuFlyoutBounds.Top + subMenu.Bottom);
            }
            else
            {
                VERIFY_IS_FALSE(subMenu.Left > 550);
            }
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuPositionWithinWindow()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' > "
                L"  <Button x:Name='button' Content='button.flyout' VerticalAlignment='Center' HorizontalAlignment='Center' FontSize='20'> "
                L"    <Button.Flyout> "
                L"      <MenuFlyout Placement='Bottom'> "
                L"        <MenuFlyoutItem Text='Copy' /> "
                L"        <MenuFlyoutItem Text='Paste' /> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <ToggleMenuFlyoutItem FontSize='30' Text='Cut' IsChecked='True' /> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <MenuFlyoutSubItem x:Name='subItem1' Text='Menu sub item 4'>"
                L"          <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"          <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"          <MenuFlyoutSeparator/>"
                L"          <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 2.3</ToggleMenuFlyoutItem>"
                L"          <MenuFlyoutSeparator/>"
                L"          <MenuFlyoutSubItem  x:Name='subItem2' Text='Menu sub item 2.4'>"
                L"            <MenuFlyoutItem>Menu item 3.1</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.2</MenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"            <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 3.3</ToggleMenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"          </MenuFlyoutSubItem>"
                L"         </MenuFlyoutSubItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        auto subItem = GetSubItem(items);
        TapSubMenuItem(subItem);

        auto subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            wf::Rect subMenu = ControlHelper::GetBounds(subPresenter);
            LOG_OUTPUT(L"Sub Menu bounds left=%f top=%f width=%f height=%f", subMenu.Left, subMenu.Top, subMenu.Width, subMenu.Height);

            VERIFY_IS_TRUE(subMenu.Left + subMenu.Width <= 400);
            VERIFY_IS_TRUE(subMenu.Top + subMenu.Height <= 600);
        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
    }

    void MenuFlyoutIntegrationTests::PerformValidateSubMenuItem(InputMethod inputMethod)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutSubItemsFromXaml();

        RunOnUIThread([&]()
        {
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        ShowMenuFlyout(menuFlyout, button1, -50, 50);

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        subItem = GetSubItem(items);

        switch (inputMethod)
        {
            case InputMethod::Touch:
                TapSubMenuItem(subItem);
                break;
            case InputMethod::Mouse:
                MoveToSubMenuItem(subItem);
                break;
            case InputMethod::Keyboard:
                NavigateSubMenu(menuFlyout, button1, InputDevice::Keyboard);
                break;
            case InputMethod::Gamepad:
                NavigateSubMenu(menuFlyout, button1, InputDevice::Gamepad);
                break;
        }

        if (inputMethod == InputMethod::Mouse)
        {
            auto presenter = GetCurrentPresenter();

            // Mouse over on the sub menu presenter and out of sub menu
            test_infra::TestServices::InputHelper->MoveMouse(presenter);
            test_infra::TestServices::InputHelper->MoveMouse(wf::Point(0, 400));

            // Mouse over on the main menu presenter and out of main menu flyout to close it
            test_infra::TestServices::InputHelper->MoveMouse(subItem);
            test_infra::TestServices::InputHelper->MoveMouse(wf::Point(0, 400));
        }

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::PerformValidateSplitMenuItem(InputMethod inputMethod)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateSplitMenuFlyoutItemsFromXaml();

        RunOnUIThread([&]()
        {
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        ShowMenuFlyout(menuFlyout, button1, -50, 50);

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
            // Get the second SplitMenuFlyoutItem (index 6) which has a submenu
            // Structure: MenuFlyoutItem(0), MenuFlyoutItem(1), Separator(2), ToggleMenuFlyoutItem(3), 
            // Separator(4), SplitMenuFlyoutItem(5), SplitMenuFlyoutItem with submenu(6)
            splitItem = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(6));
        });

        switch (inputMethod)
        {
            case InputMethod::Touch:
                // Tap the secondary button to open submenu
                TapSplitMenuItemSecondary(splitItem);
                break;
            case InputMethod::Mouse:
                // Move to and click the secondary button to open submenu
                MoveToSplitMenuItemSecondary(splitItem);
                break;
            case InputMethod::Keyboard:
                NavigateSplitMenu(menuFlyout, button1, InputDevice::Keyboard);
                break;
            case InputMethod::Gamepad:
                NavigateSplitMenu(menuFlyout, button1, InputDevice::Gamepad);
                break;
        }

        if (inputMethod == InputMethod::Mouse)
        {
            auto presenter = GetCurrentPresenter();

            // Mouse over on the split menu presenter and out of sub menu
            test_infra::TestServices::InputHelper->MoveMouse(presenter);
            test_infra::TestServices::InputHelper->MoveMouse(wf::Point(0, 400));

            // Mouse over on the main menu presenter and out of main menu flyout to close it
            test_infra::TestServices::InputHelper->MoveMouse(splitItem);
            test_infra::TestServices::InputHelper->MoveMouse(wf::Point(0, 400));
        }

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::PerformValidateTraverseMenuFlyoutItems(InputMethod inputMethod, boolean goDownFirst)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyout();
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        unsigned int itemsSize = 0;

        auto loadedEvent = std::make_shared<Event>();
        auto gotFocusEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        xaml::RoutedEventHandler^ gotFocusHandler = nullptr;
        std::vector<SafeEventRegistrationType(xaml_controls::Control, GotFocus)> gotFocusRegistrations;

        // When a MenuFlyout is opened, the first focusable element will receive focus. So regardless of which direction we attempt to traverse the list
        // the fist focused element will be MFI1.
        // For list of size n, we keep going in one direction for n+1 times and then we go in the opposite direction for n+1 times.
        // If we're using the keyboard, which allows wrapping, then we'll see 2*(n+1) focus changes, since we'll be wrapping along the way.
        // On the other hand, if we're using gamepad or remote, wrapping is disabled.  In this circumstance,
        // if we start by traversing down this will mean we transition focus twice, and so the expectedFocusSequence with have three tags,
        // whereas if we start by trying to navigate up, we will see no change in focus because the first element will already have focus
        // and menu flyout does not allow focus looping with those input methods. For this reason, you will see only two tags in that expected focus sequence.
        // This is because the number of focusable items is 2 (MFI1 - MenuFlyoutItem1, MFSI1 - MenuFlyoutSubItem1) whereas the
        // other 2 items in this MenuFlyout are, a disabled MenuFlyoutItem and a MenuFlyoutSeparator which do not get focus.
        Platform::String^ expectedFocusSequence =
            inputMethod == InputMethod::Keyboard ?
                L"[MFI1][MFSI1][MFI1][MFSI1][MFI1][MFSI1][MFI1][MFSI1][MFI1][MFSI1][MFI1]" :
                (goDownFirst ? L"[MFI1][MFSI1][MFI1]" : L"[MFI1][MFSI1]");
        Platform::String^ focusSequence = "";

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Button Content="Initial focus button" />
                        <Button x:Name="button" Content="Button" Width="100" Height="50"/>
                    </Grid>)"));
            TestServices::WindowHelper->WindowContent = rootPanel;

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            button->Flyout = menuFlyout;

            items = menuFlyout->Items;
            itemsSize = items->Size;

            gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(sender)->Tag + "]";
            });

            for (unsigned int i = 0; i < itemsSize; i++)
            {
                auto item = safe_cast<xaml_controls::Control^>(items->GetAt(i));
                auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Control, GotFocus);
                gotFocusRegistration.Attach(item, gotFocusHandler);
                gotFocusRegistrations.push_back(std::move(gotFocusRegistration));
            }

            loadedRegistration.Attach(rootPanel, [&]() { loadedEvent->Set(); });
            buttonGotFocusRegistration.Attach(button, [&]() { gotFocusEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button->Focus(xaml::FocusState::Keyboard);
        });

        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        InputDevice inputDevice = InputDevice::Gamepad;
        switch (inputMethod)
        {
            case InputMethod::Gamepad:
                inputDevice = InputDevice::Gamepad;
                break;
            case InputMethod::Keyboard:
                inputDevice = InputDevice::Keyboard;
                break;
        }

        CommonInputHelper::Accept(inputDevice);

        // Go in one direction, for the length of the list + 1.
        for (unsigned int i = 0; i < itemsSize + 1; i++)
        {
            if (goDownFirst)
            {
                CommonInputHelper::Down(inputDevice);
            }
            else
            {
                CommonInputHelper::Up(inputDevice);
            }
            TestServices::WindowHelper->WaitForIdle();
        }

        // Go in the opposite direction, for the length of the list + 1.
        for (unsigned int i = 0; i < itemsSize + 1; i++)
        {
            if (goDownFirst)
            {
                CommonInputHelper::Up(inputDevice);
            }
            else
            {
                CommonInputHelper::Down(inputDevice);
            }
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);

        CommonInputHelper::Cancel(inputDevice);
    }

    void MenuFlyoutIntegrationTests::PerformValidateTraverseSplitMenuFlyoutItems(InputMethod inputMethod, boolean goDownFirst)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutWithSplitItems();
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        unsigned int itemsSize = 0;

        auto loadedEvent = std::make_shared<Event>();
        auto gotFocusEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        xaml::RoutedEventHandler^ gotFocusHandler = nullptr;
        std::vector<SafeEventRegistrationType(xaml_controls::Control, GotFocus)> gotFocusRegistrations;

        // When a MenuFlyout with SplitMenuFlyoutItems is opened, the first focusable element will receive focus.
        // SplitMenuFlyoutItem has two focusable parts: primary button and secondary button.
        // When navigating down focus moves to a SplitMenuFlyoutItem's Primary Button first, then to its Secondary Button.
        // When navigating up, focus moves to a SplitMenuFlyoutItem's Secondary Button first, then to its Primary Button. 
        // And then on the next navigation it moves to the next or previous menu items respectively.
        // Primary button is tagged as SMFI1-P, and secondary button is tagged as SMFI1-S.
        //
        // For a list with items: MFI1 (focusable), MFI2 (disabled), MFS1 (separator), SMFI1 (split item with 2 parts)
        // The focusable items are: MFI1, SMFI1-P, SMFI1-S (3 focusable elements)
        //
        // For keyboard (which allows wrapping):
        // - Going down from MFI1 -> SMFI1-P -> SMFI1-S -> (wrap) MFI1 -> ...
        // - Going up from MFI1 -> SMFI1-S -> SMFI1-P -> (wrap) MFI1 -> ...
        //
        // For gamepad/remote (no wrapping):
        // - Starting at MFI1, going down: MFI1 -> SMFI1-P -> SMFI1-S (stops)
        // - Starting at MFI1, going up: MFI1 (stops, no wrap)
        Platform::String^ expectedFocusSequence =
            inputMethod == InputMethod::Keyboard ?
                (goDownFirst ? L"[MFI1][SMFI1-P][SMFI1-S][MFI1][SMFI1-P][SMFI1-S][SMFI1-P][MFI1][SMFI1-S][SMFI1-P][MFI1]" :
                               L"[MFI1][SMFI1-S][SMFI1-P][MFI1][SMFI1-S][SMFI1-P][SMFI1-S][MFI1][SMFI1-P][SMFI1-S][MFI1]") :
                (goDownFirst ? L"[MFI1][SMFI1-P][SMFI1-S][SMFI1-P][MFI1]" : L"[MFI1][SMFI1-P][SMFI1-S]");
        Platform::String^ focusSequence = "";

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Button Content="Initial focus button" />
                        <Button x:Name="button" Content="Button" Width="100" Height="50"/>
                    </Grid>)"));
            TestServices::WindowHelper->WindowContent = rootPanel;

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            button->Flyout = menuFlyout;

            items = menuFlyout->Items;
            itemsSize = items->Size;

            gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                auto tag = safe_cast<xaml::FrameworkElement^>(sender)->Tag;
                if (tag != nullptr)
                {
                    focusSequence += "[" + tag->ToString() + "]";
                }
            });

            // Using itemsSize-1 to skip the last SplitMenuFlyoutItem which has sub parts.
            // And we want to assign focus handlers to those parts and not the complete control.
            for (unsigned int i = 0; i < itemsSize-1; i++)
            {
                auto item = safe_cast<xaml_controls::Control^>(items->GetAt(i));
                auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Control, GotFocus);
                gotFocusRegistration.Attach(item, gotFocusHandler);
                gotFocusRegistrations.push_back(std::move(gotFocusRegistration));
            }

            loadedRegistration.Attach(rootPanel, [&]() { loadedEvent->Set(); });
            buttonGotFocusRegistration.Attach(button, [&]() { gotFocusEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button->Focus(xaml::FocusState::Keyboard);
        });

        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        InputDevice inputDevice = InputDevice::Gamepad;
        switch (inputMethod)
        {
            case InputMethod::Gamepad:
                inputDevice = InputDevice::Gamepad;
                break;
            case InputMethod::Keyboard:
                inputDevice = InputDevice::Keyboard;
                break;
        }

        CommonInputHelper::Accept(inputDevice);
        TestServices::WindowHelper->WaitForIdle();

        // After the menu is opened, find and register focus handlers for split item buttons
        RunOnUIThread([&]()
        {
            auto item = safe_cast<xaml_controls::Control^>(items->GetAt(itemsSize-1));
            auto splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(item);
            if (splitItem)
            {
                // Now that the menu is shown, the visual tree should be loaded
                // splitItem->ApplyTemplate();
                // splitItem->UpdateLayout();
                
                // Find the primary button (usually named "PrimaryButton" or similar)
                auto primaryButton = dynamic_cast<xaml_controls::Button^>(
                    TreeHelper::GetVisualChildByName(splitItem, L"PrimaryButton"));
                if (primaryButton)
                {
                    primaryButton->Tag = splitItem->Tag + "-P";
                    auto primaryGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Control, GotFocus);
                    primaryGotFocusRegistration.Attach(primaryButton, gotFocusHandler);
                    gotFocusRegistrations.push_back(std::move(primaryGotFocusRegistration));
                }

                // Find the secondary button (usually named "SecondaryButton" or similar)
                auto secondaryButton = dynamic_cast<xaml_controls::Button^>(
                    TreeHelper::GetVisualChildByName(splitItem, L"SecondaryButton"));
                if (secondaryButton)
                {
                    secondaryButton->Tag = splitItem->Tag + "-S";
                    auto secondaryGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Control, GotFocus);
                    secondaryGotFocusRegistration.Attach(secondaryButton, gotFocusHandler);
                    gotFocusRegistrations.push_back(std::move(secondaryGotFocusRegistration));
                }
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        // Go in one direction, for the length of itemsSize + 1.
        for (unsigned int i = 0; i < itemsSize + 1; i++)
        {
            if (goDownFirst)
            {
                CommonInputHelper::Down(inputDevice);
            }
            else
            {
                CommonInputHelper::Up(inputDevice);
            }
            TestServices::WindowHelper->WaitForIdle();
        }

        // Go in the opposite direction, for the length of itemsSize + 1.
        for (unsigned int i = 0; i < itemsSize + 1; i++)
        {
            if (goDownFirst)
            {
                CommonInputHelper::Up(inputDevice);
            }
            else
            {
                CommonInputHelper::Down(inputDevice);
            }
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);

        CommonInputHelper::Cancel(inputDevice);
    }

    xaml_controls::Canvas^ MenuFlyoutIntegrationTests::SetupRootPanelForSubMenuTest()
    {
        xaml_controls::Canvas^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                L"<Canvas Background='RoyalBlue' "
                L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <Button x:Name='button1' Content='Button' Width='100' Height='50' Canvas.Left='50' Canvas.Top='50' />"
                L"</Canvas>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return rootPanel;
    }

    void MenuFlyoutIntegrationTests::TapSubMenuItem(xaml_controls::MenuFlyoutSubItem^ subItem)
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

        RunOnUIThread([&]()
        {
            menuFlyoutPresenter = GetCurrentPresenter();

            lostFocusRegistration.Attach(
                menuFlyoutPresenter,
                ref new xaml::RoutedEventHandler(
                [lostFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                lostFocusEvent->Set();
            }));
        });

        TestServices::InputHelper->Tap(subItem);

        lostFocusEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    // Navigate in and out of submenu using both directional and Space/Escape keys.
    // Also test Gamepad functionality (using the internal Gamepad -> kb mapping).
    void MenuFlyoutIntegrationTests::NavigateSubMenu(
        xaml_controls::MenuFlyout^ menuFlyout,
        xaml_controls::Button^ button,
        InputDevice device
        )
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            menuFlyoutPresenter = GetCurrentPresenter();

            lostFocusRegistration.Attach(
                menuFlyoutPresenter,
                ref new xaml::RoutedEventHandler(
                [lostFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                lostFocusEvent->Set();
            }));

            closedRegistration.Attach(
                menuFlyout,
                ref new wf::EventHandler<Platform::Object^>(
                [menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutClosedEvent->Set();
            }));

        });

        // Go down four times to give a focus to the submenu item
        // from the first menu item.
        for (int i = 0; i < 4; i++)
        {
            CommonInputHelper::Down(device);
        }

        // Expand/collapse the submenu using right and left.
        CommonInputHelper::Right(device);
        CommonInputHelper::Down(device);
        CommonInputHelper::Left(device);

        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Expand/collapse the submenu using accept and cancel.
        CommonInputHelper::Accept(device);
        CommonInputHelper::Down(device);
        CommonInputHelper::Cancel(device);

        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Close the top-level menu (which will close the MenuFlyout itself)
        CommonInputHelper::Cancel(device);

        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Opened flyout expected by caller
        ShowMenuFlyout(menuFlyout, button, -50, 50);
    }

    void MenuFlyoutIntegrationTests::MoveToSubMenuItem(xaml_controls::MenuFlyoutSubItem^ subItem)
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

        RunOnUIThread([&]()
        {
            menuFlyoutPresenter = GetCurrentPresenter();

            lostFocusRegistration.Attach(
                menuFlyoutPresenter,
                ref new xaml::RoutedEventHandler(
                [lostFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                lostFocusEvent->Set();
            }));
        });

        test_infra::TestServices::InputHelper->MoveMouse(subItem);
        test_infra::TestServices::InputHelper->LeftMouseClick(subItem);

        TestServices::WindowHelper->WaitForIdle();

        lostFocusEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::TapSplitMenuItemPrimary(xaml_controls::SplitMenuFlyoutItem^ splitItem)
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

        wf::Rect splitItemBounds = {};

        RunOnUIThread([&]()
        {
            menuFlyoutPresenter = GetCurrentPresenter();

            lostFocusRegistration.Attach(
                menuFlyoutPresenter,
                ref new xaml::RoutedEventHandler(
                [lostFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                lostFocusEvent->Set();
            }));

            splitItemBounds = ControlHelper::GetBounds(splitItem);
        });

        // Tap on the left side (primary button area) - approximately 35% from the left, 50% from the top
        double offsetX = splitItemBounds.Width * 0.35;
        double offsetY = splitItemBounds.Height * 0.5;
        TestServices::InputHelper->Tap(splitItem, offsetX, offsetY);

        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::TapSplitMenuItemSecondary(xaml_controls::SplitMenuFlyoutItem^ splitItem)
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;
        xaml_controls::Button^ secondaryButton = nullptr;

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

        RunOnUIThread([&]()
        {
            menuFlyoutPresenter = GetCurrentPresenter();

            lostFocusRegistration.Attach(
                menuFlyoutPresenter,
                ref new xaml::RoutedEventHandler(
                [lostFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                lostFocusEvent->Set();
            }));

            // Find the secondary button in the visual tree
            secondaryButton = dynamic_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(splitItem, L"SecondaryButton"));
            VERIFY_IS_NOT_NULL(secondaryButton);
        });

        // Tap on the secondary button
        TestServices::InputHelper->Tap(secondaryButton);

        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::MoveToSplitMenuItemPrimary(xaml_controls::SplitMenuFlyoutItem^ splitItem)
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

        wf::Rect splitItemBounds = {};

        RunOnUIThread([&]()
        {
            menuFlyoutPresenter = GetCurrentPresenter();

            lostFocusRegistration.Attach(
                menuFlyoutPresenter,
                ref new xaml::RoutedEventHandler(
                [lostFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                lostFocusEvent->Set();
            }));

            splitItemBounds = ControlHelper::GetBounds(splitItem);
        });

        // Move to and click on the left side (primary button area) - approximately 35% from the left
        int offsetX = static_cast<int>(splitItemBounds.Width * 0.35f);
        int offsetY = static_cast<int>(splitItemBounds.Height * 0.5f);

        test_infra::TestServices::InputHelper->MoveMouse(splitItem, offsetX, offsetY);
        test_infra::TestServices::InputHelper->LeftMouseClick(splitItem);

        TestServices::WindowHelper->WaitForIdle();

        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::MoveToSplitMenuItemSecondary(xaml_controls::SplitMenuFlyoutItem^ splitItem)
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;
        xaml_controls::Button^ secondaryButton = nullptr;

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

        RunOnUIThread([&]()
        {
            menuFlyoutPresenter = GetCurrentPresenter();

            lostFocusRegistration.Attach(
                menuFlyoutPresenter,
                ref new xaml::RoutedEventHandler(
                [lostFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                lostFocusEvent->Set();
            }));

            // Find the secondary button in the visual tree
            secondaryButton = dynamic_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(splitItem, L"SecondaryButton"));
            VERIFY_IS_NOT_NULL(secondaryButton);
        });

        // Move to and click on the secondary button
        test_infra::TestServices::InputHelper->MoveMouse(secondaryButton);
        test_infra::TestServices::InputHelper->LeftMouseClick(secondaryButton);

        TestServices::WindowHelper->WaitForIdle();

        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    // Navigate in and out of split menu flyout item's submenu using both directional and Space/Escape keys.
    // Also test Gamepad functionality (using the internal Gamepad -> kb mapping).
    // This method assumes the MenuFlyout is created with CreateSplitMenuFlyoutItemsFromXaml which contains
    // SplitMenuFlyoutItem controls with submenus. It navigates to a SplitMenuFlyoutItem and tests
    // expanding/collapsing its submenu using keyboard/gamepad inputs.
    void MenuFlyoutIntegrationTests::NavigateSplitMenu(
        xaml_controls::MenuFlyout^ menuFlyout,
        xaml_controls::Button^ button,
        InputDevice device
        )
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            menuFlyoutPresenter = GetCurrentPresenter();

            lostFocusRegistration.Attach(
                menuFlyoutPresenter,
                ref new xaml::RoutedEventHandler(
                [lostFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                lostFocusEvent->Set();
            }));

            closedRegistration.Attach(
                menuFlyout,
                ref new wf::EventHandler<Platform::Object^>(
                [menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutClosedEvent->Set();
            }));

        });

        // Navigate down to a SplitMenuFlyoutItem that has a submenu
        // The exact number of Down presses will depend on the menu structure from CreateSplitMenuFlyoutItemsFromXaml
        // Typical structure: MenuFlyoutItem, MenuFlyoutItem, Separator, ToggleMenuFlyoutItem, Separator, 
        // SplitMenuFlyoutItem (no submenu), SplitMenuFlyoutItem (with submenu)
        // Navigate to the SplitMenuFlyoutItem with submenu (around 6-7 down presses)
        for (int i = 0; i < 6; i++)
        {
            CommonInputHelper::Down(device);
        }

        // Expand/collapse the submenu using right and left.
        CommonInputHelper::Right(device);
        CommonInputHelper::Down(device);
        CommonInputHelper::Left(device);

        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Expand/collapse the submenu using accept and cancel.
        CommonInputHelper::Accept(device);
        CommonInputHelper::Down(device);
        CommonInputHelper::Cancel(device);

        lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Close the top-level menu (which will close the MenuFlyout itself)
        CommonInputHelper::Cancel(device);

        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Opened flyout expected by caller
        ShowMenuFlyout(menuFlyout, button, -50, 50);
    }

    xaml_controls::MenuFlyoutSubItem^ MenuFlyoutIntegrationTests::GetSubItem(wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items)
    {
        xaml_controls::MenuFlyoutSubItem^  subItem = nullptr;

        RunOnUIThread([&]()
        {
            subItem = GetSubItem(items, items->Size - 1);
        });

        return subItem;
    }

    xaml_controls::MenuFlyoutSubItem^ MenuFlyoutIntegrationTests::GetSubItem(wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items, int index)
    {
        xaml_controls::MenuFlyoutSubItem^  subItem = nullptr;

        RunOnUIThread([&]()
        {
            subItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(items->GetAt(index));
        });

        return subItem;
    }

    xaml_controls::MenuFlyoutPresenter^ MenuFlyoutIntegrationTests::GetCurrentPresenter()
    {
        xaml_controls::MenuFlyoutPresenter^ menuFlyoutPresenter = nullptr;

        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);

            auto popup = popups->GetAt(0);
            menuFlyoutPresenter = dynamic_cast<xaml_controls::MenuFlyoutPresenter^>(popup->Child);
        });

        return menuFlyoutPresenter;
    }

    void MenuFlyoutIntegrationTests::ShowMenuFlyout(xaml_controls::MenuFlyout^ menuFlyout, xaml::UIElement^ relativeTo, float horizontalOffset, float verticalOffset, bool forceTapAsPreviousInputMessage)
    {
        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        if (forceTapAsPreviousInputMessage)
        {
            XamlRoot^ xamlRoot = nullptr;
            RunOnUIThread([&]()
            {
                xamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
            });

            // Inject a tap. MenuFlyout looks different depending on how it was opened (mouse gives narrower padding than touch). We're
            // opening a flyout with ShowAt, which just grabs the last input device type and uses that. Set it explicitly to tap so that
            // the previous test doesn't mess up the state for this test. Use a test hook for this - tapping at arbitrary places can mess
            // up focus and flyout state.
            InjectInput(InputMethod::Touch);
        }

        RunOnUIThread([&]()
        {
            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([openedEvent](Platform::Object^, Platform::Object^)
            {
                openedEvent->Set();
            }));

            menuFlyout->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
            menuFlyout->ShowAt(relativeTo, wf::Point(horizontalOffset, verticalOffset));
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::ValidateUIElementTree()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 730));

        auto validationRules = ref new Platform::String(
            LR"(<?xml version='1.0' encoding='UTF-8'?>
                <Rules>
                    <Rule Applicability='//Element' Inclusion='Blacklist'>
                        <Property Name='FocusState'/>
                        <Property Name='IsPressed'/>
                    </Rule>
                </Rules>)");

        xaml_controls::MenuFlyout^ menuFlyout;
        xaml_controls::MenuFlyoutItem^ restMenuFlyoutItem;
        xaml_controls::MenuFlyoutItem^ restMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::MenuFlyoutItem^ pointerOverMenuFlyoutItem;
        xaml_controls::MenuFlyoutItem^ pointerOverMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::MenuFlyoutItem^ pressedMenuFlyoutItem;
        xaml_controls::MenuFlyoutItem^ pressedMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::MenuFlyoutItem^ disabledMenuFlyoutItem;
        xaml_controls::MenuFlyoutItem^ disabledMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::MenuFlyoutItem^ focusedMenuFlyoutItem;

        xaml_controls::MenuFlyoutSeparator^ firstMenuFlyoutSeparator;

        xaml_controls::ToggleMenuFlyoutItem^ restUncheckedToggleMenuFlyoutItem;
        xaml_controls::ToggleMenuFlyoutItem^ restUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::ToggleMenuFlyoutItem^ pointerOverUncheckedToggleMenuFlyoutItem;
        xaml_controls::ToggleMenuFlyoutItem^ pointerOverUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::ToggleMenuFlyoutItem^ pressedUncheckedToggleMenuFlyoutItem;
        xaml_controls::ToggleMenuFlyoutItem^ pressedUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::ToggleMenuFlyoutItem^ disabledUncheckedToggleMenuFlyoutItem;
        xaml_controls::ToggleMenuFlyoutItem^ disabledUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::ToggleMenuFlyoutItem^ focusedUncheckedToggleMenuFlyoutItem;

        xaml_controls::MenuFlyoutSeparator^ secondMenuFlyoutSeparator;

        xaml_controls::MenuFlyoutSubItem^ restMenuFlyoutSubItem;
        xaml_controls::MenuFlyoutSubItem^ pointerOverMenuFlyoutSubItem;
        xaml_controls::MenuFlyoutSubItem^ pressedMenuFlyoutSubItem;
        xaml_controls::MenuFlyoutSubItem^ disabledMenuFlyoutSubItem;
        xaml_controls::MenuFlyoutSubItem^ focusedMenuFlyoutSubItem;

        xaml_controls::MenuFlyoutPresenter^ thirdMenuFlyoutPresenter;

        xaml_controls::ToggleMenuFlyoutItem^ restCheckedToggleMenuFlyoutItem;
        xaml_controls::ToggleMenuFlyoutItem^ restCheckedToggleMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::ToggleMenuFlyoutItem^ pointerOverCheckedToggleMenuFlyoutItem;
        xaml_controls::ToggleMenuFlyoutItem^ pointerOverCheckedToggleMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::ToggleMenuFlyoutItem^ pressedCheckedToggleMenuFlyoutItem;
        xaml_controls::ToggleMenuFlyoutItem^ pressedCheckedToggleMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::ToggleMenuFlyoutItem^ disabledCheckedToggleMenuFlyoutItem;
        xaml_controls::ToggleMenuFlyoutItem^ disabledCheckedToggleMenuFlyoutItemWithKeyboardAccelerator;
        xaml_controls::ToggleMenuFlyoutItem^ focusedCheckedToggleMenuFlyoutItem;

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Button^ rootButton = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            menuFlyout = ref new xaml_controls::MenuFlyout();
            menuFlyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;

            restMenuFlyoutItem = ref new xaml_controls::MenuFlyoutItem();
            restMenuFlyoutItem->Text = "Rest MenuFlyoutItem";
            menuFlyout->Items->Append(restMenuFlyoutItem);

            restMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::MenuFlyoutItem();
            restMenuFlyoutItemWithKeyboardAccelerator->Text = "Rest MenuFlyoutItem with keyboard accelerator";
            restMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
            menuFlyout->Items->Append(restMenuFlyoutItemWithKeyboardAccelerator);

            pointerOverMenuFlyoutItem = ref new xaml_controls::MenuFlyoutItem();
            pointerOverMenuFlyoutItem->Text = "Pointer Over MenuFlyoutItem";
            menuFlyout->Items->Append(pointerOverMenuFlyoutItem);

            pointerOverMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::MenuFlyoutItem();
            pointerOverMenuFlyoutItemWithKeyboardAccelerator->Text = "Pointer Over MenuFlyoutItem with keyboard accelerator";
            pointerOverMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
            menuFlyout->Items->Append(pointerOverMenuFlyoutItemWithKeyboardAccelerator);

            pressedMenuFlyoutItem = ref new xaml_controls::MenuFlyoutItem();
            pressedMenuFlyoutItem->Text = "Pressed MenuFlyoutItem";
            menuFlyout->Items->Append(pressedMenuFlyoutItem);

            pressedMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::MenuFlyoutItem();
            pressedMenuFlyoutItemWithKeyboardAccelerator->Text = "Pressed MenuFlyoutItem with keyboard accelerator";
            pressedMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
            menuFlyout->Items->Append(pressedMenuFlyoutItemWithKeyboardAccelerator);

            disabledMenuFlyoutItem = ref new xaml_controls::MenuFlyoutItem();
            disabledMenuFlyoutItem->Text = "Disabled MenuFlyoutItem";
            disabledMenuFlyoutItem->IsEnabled = false;
            menuFlyout->Items->Append(disabledMenuFlyoutItem);

            disabledMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::MenuFlyoutItem();
            disabledMenuFlyoutItemWithKeyboardAccelerator->Text = "Disabled MenuFlyoutItem with keyboard accelerator";
            disabledMenuFlyoutItemWithKeyboardAccelerator->IsEnabled = false;
            disabledMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
            menuFlyout->Items->Append(disabledMenuFlyoutItemWithKeyboardAccelerator);

            focusedMenuFlyoutItem = ref new xaml_controls::MenuFlyoutItem();
            focusedMenuFlyoutItem->Text = "Focused MenuFlyoutItem";
            menuFlyout->Items->Append(focusedMenuFlyoutItem);

            firstMenuFlyoutSeparator = ref new xaml_controls::MenuFlyoutSeparator();
            menuFlyout->Items->Append(firstMenuFlyoutSeparator);

            restMenuFlyoutSubItem = ref new xaml_controls::MenuFlyoutSubItem();
            restMenuFlyoutSubItem->Text = "Rest MenuFlyoutSubItem";
            menuFlyout->Items->Append(restMenuFlyoutSubItem);

            pointerOverMenuFlyoutSubItem = ref new xaml_controls::MenuFlyoutSubItem();
            pointerOverMenuFlyoutSubItem->Text = "Pointer Over MenuFlyoutSubItem";
            menuFlyout->Items->Append(pointerOverMenuFlyoutSubItem);

            pressedMenuFlyoutSubItem = ref new xaml_controls::MenuFlyoutSubItem();
            pressedMenuFlyoutSubItem->Text = "Pressed MenuFlyoutSubItem";
            menuFlyout->Items->Append(pressedMenuFlyoutSubItem);

            disabledMenuFlyoutSubItem = ref new xaml_controls::MenuFlyoutSubItem();
            disabledMenuFlyoutSubItem->Text = "Disabled MenuFlyoutSubItem";
            disabledMenuFlyoutSubItem->IsEnabled = false;
            menuFlyout->Items->Append(disabledMenuFlyoutSubItem);

            focusedMenuFlyoutSubItem = ref new xaml_controls::MenuFlyoutSubItem();
            focusedMenuFlyoutSubItem->Text = "Focused MenuFlyoutSubItem";
            menuFlyout->Items->Append(focusedMenuFlyoutSubItem);

            secondMenuFlyoutSeparator = ref new xaml_controls::MenuFlyoutSeparator();
            menuFlyout->Items->Append(secondMenuFlyoutSeparator);

            restUncheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            restUncheckedToggleMenuFlyoutItem->Text = "Rest Unchecked ToggleMenuFlyoutItem";
            menuFlyout->Items->Append(restUncheckedToggleMenuFlyoutItem);

            restUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::ToggleMenuFlyoutItem();
            restUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->Text = "Rest Unchecked ToggleMenuFlyoutItem with keyboard accelerator";
            restUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
            menuFlyout->Items->Append(restUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator);

            pointerOverUncheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            pointerOverUncheckedToggleMenuFlyoutItem->Text = "Pointer Over Unchecked ToggleMenuFlyoutItem";
            menuFlyout->Items->Append(pointerOverUncheckedToggleMenuFlyoutItem);

            pointerOverUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::ToggleMenuFlyoutItem();
            pointerOverUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->Text = "Pointer Over Unchecked ToggleMenuFlyoutItem with keyboard accelerator";
            pointerOverUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
            menuFlyout->Items->Append(pointerOverUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator);

            pressedUncheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            pressedUncheckedToggleMenuFlyoutItem->Text = "Pressed Unchecked ToggleMenuFlyoutItem";
            menuFlyout->Items->Append(pressedUncheckedToggleMenuFlyoutItem);

            pressedUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::ToggleMenuFlyoutItem();
            pressedUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->Text = "Pressed Unchecked ToggleMenuFlyoutItem with keyboard accelerator";
            pressedUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
            menuFlyout->Items->Append(pressedUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator);

            disabledUncheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            disabledUncheckedToggleMenuFlyoutItem->Text = "Disabled Unchecked ToggleMenuFlyoutItem";
            disabledUncheckedToggleMenuFlyoutItem->IsEnabled = false;
            menuFlyout->Items->Append(disabledUncheckedToggleMenuFlyoutItem);

            disabledUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::ToggleMenuFlyoutItem();
            disabledUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->Text = "Disabled Unchecked ToggleMenuFlyoutItem with keyboard accelerator";
            disabledUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->IsEnabled = false;
            disabledUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
            menuFlyout->Items->Append(disabledUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator);

            focusedUncheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            focusedUncheckedToggleMenuFlyoutItem->Text = "Focused Unchecked ToggleMenuFlyoutItem";
            menuFlyout->Items->Append(focusedUncheckedToggleMenuFlyoutItem);

            restCheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            restCheckedToggleMenuFlyoutItem->Text = "Rest Checked ToggleMenuFlyoutItem";
            restCheckedToggleMenuFlyoutItem->IsChecked = true;
            restMenuFlyoutSubItem->Items->Append(restCheckedToggleMenuFlyoutItem);

            restCheckedToggleMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::ToggleMenuFlyoutItem();
            restCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->Text = "Rest Checked ToggleMenuFlyoutItem with keyboard accelerator";
            restCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->IsChecked = true;
            restCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::A, ::Windows::System::VirtualKeyModifiers::None));
            menuFlyout->Items->Append(restCheckedToggleMenuFlyoutItemWithKeyboardAccelerator);

            pointerOverCheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            pointerOverCheckedToggleMenuFlyoutItem->Text = "Pointer Over Checked ToggleMenuFlyoutItem";
            pointerOverCheckedToggleMenuFlyoutItem->IsChecked = true;
            restMenuFlyoutSubItem->Items->Append(pointerOverCheckedToggleMenuFlyoutItem);

            pointerOverCheckedToggleMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::ToggleMenuFlyoutItem();
            pointerOverCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->Text = "Pointer Over Checked ToggleMenuFlyoutItem with keyboard accelerator";
            pointerOverCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->IsChecked = true;
            pointerOverCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::S, ::Windows::System::VirtualKeyModifiers::Control));
            menuFlyout->Items->Append(pointerOverCheckedToggleMenuFlyoutItemWithKeyboardAccelerator);

            pressedCheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            pressedCheckedToggleMenuFlyoutItem->Text = "Pressed Checked ToggleMenuFlyoutItem";
            pressedCheckedToggleMenuFlyoutItem->IsChecked = true;
            restMenuFlyoutSubItem->Items->Append(pressedCheckedToggleMenuFlyoutItem);

            pressedCheckedToggleMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::ToggleMenuFlyoutItem();
            pressedCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->Text = "Pressed Checked ToggleMenuFlyoutItem with keyboard accelerator";
            pressedCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->IsChecked = true;
            pressedCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::D, ::Windows::System::VirtualKeyModifiers::Shift));
            menuFlyout->Items->Append(pressedCheckedToggleMenuFlyoutItemWithKeyboardAccelerator);

            disabledCheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            disabledCheckedToggleMenuFlyoutItem->Text = "Disabled Checked ToggleMenuFlyoutItem";
            disabledCheckedToggleMenuFlyoutItem->IsChecked = true;
            disabledCheckedToggleMenuFlyoutItem->IsEnabled = false;
            restMenuFlyoutSubItem->Items->Append(disabledCheckedToggleMenuFlyoutItem);

            disabledCheckedToggleMenuFlyoutItemWithKeyboardAccelerator = ref new xaml_controls::ToggleMenuFlyoutItem();
            disabledCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->Text = "Disabled Checked ToggleMenuFlyoutItem with keyboard accelerator";
            disabledCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->IsChecked = true;
            disabledCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->IsEnabled = false;
            disabledCheckedToggleMenuFlyoutItemWithKeyboardAccelerator->KeyboardAccelerators->Append(CreateKeyboardAccelerator(::Windows::System::VirtualKey::F, ::Windows::System::VirtualKeyModifiers::Menu | ::Windows::System::VirtualKeyModifiers::Windows));
            menuFlyout->Items->Append(disabledCheckedToggleMenuFlyoutItemWithKeyboardAccelerator);

            focusedCheckedToggleMenuFlyoutItem = ref new xaml_controls::ToggleMenuFlyoutItem();
            focusedCheckedToggleMenuFlyoutItem->Text = "Focused Checked ToggleMenuFlyoutItem";
            focusedCheckedToggleMenuFlyoutItem->IsChecked = true;
            restMenuFlyoutSubItem->Items->Append(focusedCheckedToggleMenuFlyoutItem);

            rootButton = ref new xaml_controls::Button();
            rootButton->Flyout = menuFlyout;
            rootPanel->Children->Append(rootButton);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto setupValidation = [&]()
            {
                ShowMenuFlyout(menuFlyout, rootButton, 0, 0, true /* forceTapAsPreviousInputMessage */);
                TapSubMenuItem(restMenuFlyoutSubItem);

                RunOnUIThread([&]()
                {
                    // MenuFlyoutItems
                    VisualStateManager::GoToState(pointerOverMenuFlyoutItem, "PointerOver", false);
                    VisualStateManager::GoToState(pointerOverMenuFlyoutItemWithKeyboardAccelerator, "PointerOver", false);
                    VisualStateManager::GoToState(pressedMenuFlyoutItem, "Pressed", false);
                    VisualStateManager::GoToState(pressedMenuFlyoutItemWithKeyboardAccelerator, "Pressed", false);
                    VisualStateManager::GoToState(focusedMenuFlyoutItem, "Focused", false);

                    // Unchecked ToggleMenuFlyoutItems
                    VisualStateManager::GoToState(pointerOverUncheckedToggleMenuFlyoutItem, "PointerOver", false);
                    VisualStateManager::GoToState(pointerOverUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator, "PointerOver", false);
                    VisualStateManager::GoToState(pressedUncheckedToggleMenuFlyoutItem, "Pressed", false);
                    VisualStateManager::GoToState(pressedUncheckedToggleMenuFlyoutItemWithKeyboardAccelerator, "Pressed", false);
                    VisualStateManager::GoToState(focusedUncheckedToggleMenuFlyoutItem, "Focused", false);

                    // MenuFlyoutSubItems
                    VisualStateManager::GoToState(pointerOverMenuFlyoutSubItem, "PointerOver", false);
                    VisualStateManager::GoToState(pressedMenuFlyoutSubItem, "Pressed", false);
                    VisualStateManager::GoToState(focusedMenuFlyoutSubItem, "Focused", false);

                    // Checked ToggleMenuFlyoutItems
                    VisualStateManager::GoToState(pointerOverCheckedToggleMenuFlyoutItem, "PointerOver", false);
                    VisualStateManager::GoToState(pointerOverCheckedToggleMenuFlyoutItemWithKeyboardAccelerator, "PointerOver", false);
                    VisualStateManager::GoToState(pressedCheckedToggleMenuFlyoutItem, "Pressed", false);
                    VisualStateManager::GoToState(pressedCheckedToggleMenuFlyoutItemWithKeyboardAccelerator, "Pressed", false);
                    VisualStateManager::GoToState(focusedCheckedToggleMenuFlyoutItem, "Focused", false);
                });
                TestServices::WindowHelper->WaitForIdle();
            };

        // Validate the Dark theme of controls.
        {
            setupValidation();
            if (TestServices::Utilities->IsOneCore)
            {
                TestServices::Utilities->VerifyUIElementTreeWithRulesInline("WindowlessPopup_Dark", validationRules);
            }
            else
            {
                TestServices::Utilities->VerifyUIElementTreeWithRulesInline("WindowedPopup_Dark", validationRules);
            }

            RunOnUIThread([&]()
            {
                menuFlyout->Hide();
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // Validate the light theme of controls.
        {
            RunOnUIThread([&]()
            {
                rootPanel->RequestedTheme = xaml::ElementTheme::Light;
                rootPanel->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::White);
            });
            TestServices::WindowHelper->WaitForIdle();

            setupValidation();
            if (TestServices::Utilities->IsOneCore)
            {
                TestServices::Utilities->VerifyUIElementTreeWithRulesInline("WindowlessPopup_Light", validationRules);
            }
            else
            {
                TestServices::Utilities->VerifyUIElementTreeWithRulesInline("WindowedPopup_Light", validationRules);
            }

            RunOnUIThread([&]()
            {
                menuFlyout->Hide();
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // Validate the high-contrast theme of controls.
        {
            setupValidation();

            // This method will turn on high-contrast mode before it does the validation.
            if (TestServices::Utilities->IsOneCore)
            {
                ControlHelper::ValidateUIElementTreeForHighContrast("WindowlessPopup_HC", rootPanel, validationRules);
            }
            else
            {
                ControlHelper::ValidateUIElementTreeForHighContrast("WindowedPopup_HC", rootPanel, validationRules);
            }

            RunOnUIThread([&]()
            {
                menuFlyout->Hide();
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    xaml_controls::MenuFlyout^ MenuFlyoutIntegrationTests::CreateSplitMenuFlyoutItemsFromXaml()
    {
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>Menu item 1</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 2</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 3</ToggleMenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split item 4' />"
                L"    <SplitMenuFlyoutItem x:Name='splitItem2' Text='Split item 5'>"
                L"        <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 2.3</ToggleMenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <MenuFlyoutSubItem x:Name='subItem1' Text='Menu sub item 2.4'>"
                L"            <MenuFlyoutItem>Menu item 3.1</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.2</MenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"            <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 3.3</ToggleMenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"        </MenuFlyoutSubItem>"
                L"    </SplitMenuFlyoutItem>"
                L"</MenuFlyout>"));
        });

        return menuFlyout;
    }

    xaml_controls::MenuFlyout^ MenuFlyoutIntegrationTests::CreateMenuFlyoutSubItemsFromXaml()
    {
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>Menu item 1</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 2</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 3</ToggleMenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <MenuFlyoutSubItem x:Name='subItem1' Text='Menu sub item 4' />"
                L"    <MenuFlyoutSubItem x:Name='subItem2' Text='Menu sub item 5'>"
                L"        <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 2.3</ToggleMenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <MenuFlyoutSubItem  x:Name='subItem3' Text='Menu sub item 2.4'>"
                L"            <MenuFlyoutItem>Menu item 3.1</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.2</MenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"            <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 3.3</ToggleMenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"        </MenuFlyoutSubItem>"
                L"    </MenuFlyoutSubItem>"
                L"</MenuFlyout>"));
        });

        return menuFlyout;
    }

    void MenuFlyoutIntegrationTests::GetMenuFlyoutItemsHorizontalPadding(wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items, double &leftPadding, double &rightPadding)
    {
        for (unsigned int i = 0; i < items->Size; i++)
        {
            xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem = dynamic_cast<xaml_controls::MenuFlyoutSubItem^>(items->GetAt(i));
            xaml_controls::ToggleMenuFlyoutItem^ toggleMenuFlyoutItem = dynamic_cast<xaml_controls::ToggleMenuFlyoutItem^>(items->GetAt(i));
            xaml_controls::MenuFlyoutItem^ menuFlyoutItem = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(i));

            if (menuFlyoutSubItem != nullptr)
            {
                xaml_controls::Grid^ layoutRoot = safe_cast<xaml_controls::Grid^>(TreeHelper::GetVisualChildByName(menuFlyoutSubItem, L"LayoutRoot"));
                leftPadding = layoutRoot->Padding.Left;
                rightPadding = layoutRoot->Padding.Right;
            }
            else if (toggleMenuFlyoutItem != nullptr)
            {
                xaml_controls::Grid^ layoutRoot = safe_cast<xaml_controls::Grid^>(TreeHelper::GetVisualChildByName(toggleMenuFlyoutItem, L"LayoutRoot"));
                leftPadding = layoutRoot->Padding.Left;
                rightPadding = layoutRoot->Padding.Right;
            }
            else if (menuFlyoutItem != nullptr)
            {
                xaml_controls::Grid^ layoutRoot = safe_cast<xaml_controls::Grid^>(TreeHelper::GetVisualChildByName(menuFlyoutItem, L"LayoutRoot"));
                leftPadding = layoutRoot->Padding.Left;
                rightPadding = layoutRoot->Padding.Right;
            }
        }
    }

    void MenuFlyoutIntegrationTests::VerifyMenuFlyoutItemsPadding(wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items, xaml::Thickness expectedPadding)
    {
        for (unsigned int i = 0; i < items->Size; i++)
        {
            xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem = dynamic_cast<xaml_controls::MenuFlyoutSubItem^>(items->GetAt(i));
            xaml_controls::ToggleMenuFlyoutItem^ toggleMenuFlyoutItem = dynamic_cast<xaml_controls::ToggleMenuFlyoutItem^>(items->GetAt(i));
            xaml_controls::MenuFlyoutItem^ menuFlyoutItem = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(i));

            if (menuFlyoutSubItem != nullptr)
            {
                xaml_controls::Grid^ layoutRoot = safe_cast<xaml_controls::Grid^>(TreeHelper::GetVisualChildByName(menuFlyoutSubItem, L"LayoutRoot"));

                LOG_OUTPUT(L"SubItem InnerBorder top=%f bottom=%f", layoutRoot->Padding.Top, layoutRoot->Padding.Bottom);

                VERIFY_ARE_EQUAL(expectedPadding, layoutRoot->Padding);

                VerifyMenuFlyoutItemsPadding(menuFlyoutSubItem->Items, expectedPadding);
            }
            else if (toggleMenuFlyoutItem != nullptr)
            {
                xaml_controls::Grid^ layoutRoot = safe_cast<xaml_controls::Grid^>(TreeHelper::GetVisualChildByName(toggleMenuFlyoutItem, L"LayoutRoot"));

                LOG_OUTPUT(L"ToggleMenuItem InnerBorder top=%f bottom=%f", layoutRoot->Padding.Top, layoutRoot->Padding.Bottom);

                VERIFY_ARE_EQUAL(expectedPadding, layoutRoot->Padding);
            }
            else if (menuFlyoutItem != nullptr)
            {
                xaml_controls::Grid^ layoutRoot = safe_cast<xaml_controls::Grid^>(TreeHelper::GetVisualChildByName(menuFlyoutItem, L"LayoutRoot"));

                LOG_OUTPUT(L"MenuItem InnerBorder top=%f bottom=%f", layoutRoot->Padding.Top, layoutRoot->Padding.Bottom);

                VERIFY_ARE_EQUAL(expectedPadding, layoutRoot->Padding);
            }
        }
    }

    void MenuFlyoutIntegrationTests::ValidateRTLSubMenuItemPosition()
    {
        TestCleanupWrapper cleanup;

        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ subItems = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem = nullptr;
        wf::Rect windowBounds = {};
        wf::Rect menuFlyoutBounds = {};
        wf::Rect subMenu1Bounds = {};
        wf::Rect subMenu2Bounds = {};

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <MenuFlyoutSubItem x:Name='subItem1' Text='subitem1'>"
                L"        <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <MenuFlyoutSubItem  x:Name='subItem2' Text='subItem2' FlowDirection='RightToLeft'>"
                L"            <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"            <MenuFlyoutSubItem x:Name='subItem3' Text='subitem3' FlowDirection='RightToLeft'/>"
                L"        </MenuFlyoutSubItem>"
                L"    </MenuFlyoutSubItem>"
                L"</MenuFlyout>"));

            windowBounds = TestServices::WindowHelper->WindowBounds;
            LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);
        });

        ShowMenuFlyout(menuFlyout, nullptr, 50, 100);

        auto presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);
        });

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        subItem = GetSubItem(items);
        TapSubMenuItem(subItem);

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenu1Bounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SubMenuFlyout bounds left=%f top=%f width=%f height=%f", subMenu1Bounds.Left, subMenu1Bounds.Top, subMenu1Bounds.Width, subMenu1Bounds.Height);
        });

        VERIFY_IS_TRUE(menuFlyoutBounds.Left < subMenu1Bounds.Left);

        RunOnUIThread([&]()
        {
            subItems = subItem->Items;
        });

        auto subItem2 = GetSubItem(subItems);
        TapSubMenuItem(subItem2);

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenu2Bounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SubMenuFlyout RTL bounds left=%f top=%f width=%f height=%f", subMenu2Bounds.Left, subMenu2Bounds.Top, subMenu2Bounds.Width, subMenu2Bounds.Height);
        });

        VERIFY_IS_TRUE(subMenu2Bounds.Left < subMenu1Bounds.Left);
        VERIFY_IS_TRUE(subMenu1Bounds.Left < subMenu2Bounds.Left + subMenu2Bounds.Width);

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateRTLSplitMenuItemPosition()
    {
        TestCleanupWrapper cleanup;

        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ subItems = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;
        wf::Rect windowBounds = {};
        wf::Rect menuFlyoutBounds = {};
        wf::Rect subMenu1Bounds = {};
        wf::Rect subMenu2Bounds = {};

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <SplitMenuFlyoutItem x:Name='splitItem1' Text='splititem1'>"
                L"        <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <SplitMenuFlyoutItem x:Name='splitItem2' Text='splitItem2' FlowDirection='RightToLeft'>"
                L"            <MenuFlyoutItem>item1</MenuFlyoutItem>"
                L"            <MenuFlyoutSeparator/>"
                L"            <SplitMenuFlyoutItem x:Name='splitItem3' Text='splititem3' FlowDirection='RightToLeft'/>"
                L"        </SplitMenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"</MenuFlyout>"));

            windowBounds = TestServices::WindowHelper->WindowBounds;
            LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);
        });

        ShowMenuFlyout(menuFlyout, nullptr, 50, 100);

        auto presenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);
        });

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        // Find the SplitMenuFlyoutItem (last item)
        RunOnUIThread([&]()
        {
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(items->Size - 1));
        });
        TapSplitMenuItemSecondary(splitItem);

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenu1Bounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SplitMenuFlyout bounds left=%f top=%f width=%f height=%f", subMenu1Bounds.Left, subMenu1Bounds.Top, subMenu1Bounds.Width, subMenu1Bounds.Height);
        });

        VERIFY_IS_TRUE(menuFlyoutBounds.Left < subMenu1Bounds.Left);

        RunOnUIThread([&]()
        {
            subItems = splitItem->Items;
        });

        // Find the nested SplitMenuFlyoutItem (last item in subItems)
        xaml_controls::SplitMenuFlyoutItem^ splitItem2 = nullptr;
        RunOnUIThread([&]()
        {
            splitItem2 = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(subItems->GetAt(subItems->Size - 1));
        });
        TapSplitMenuItemSecondary(splitItem2);

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            subMenu2Bounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"SplitMenuFlyout RTL bounds left=%f top=%f width=%f height=%f", subMenu2Bounds.Left, subMenu2Bounds.Top, subMenu2Bounds.Width, subMenu2Bounds.Height);
        });

        VERIFY_IS_TRUE(subMenu2Bounds.Left < subMenu1Bounds.Left);
        VERIFY_IS_TRUE(subMenu1Bounds.Left < subMenu2Bounds.Left + subMenu2Bounds.Width);

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemProperties()
    {
        TestCleanupWrapper cleanup;

        // Known leak in MenuFlyoutIntegrationTests::ValidateSubMenuItemProperties due to DataContext (796 bytes)
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' >"
                L"  <Button x:Name='button' Content='button.flyout' VerticalAlignment='Center' HorizontalAlignment='Center' "
                L"          RequestedTheme='light' FlowDirection='LeftToRight' Language='En-UK' IsTextScaleFactorEnabled='true' >"
                L"    <Button.Flyout> "
                L"      <MenuFlyout Placement='Bottom'> "
                L"        <MenuFlyoutItem Text='Item 1' /> "
                L"        <MenuFlyoutItem Text='Item 2' /> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <ToggleMenuFlyoutItem Text='Toggle Item3' IsChecked='True' /> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <MenuFlyoutSubItem x:Name='subItem1' Text='Sub item 4'>"
                L"          <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"          <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"          <MenuFlyoutSeparator/>"
                L"          <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 2.3</ToggleMenuFlyoutItem>"
                L"         </MenuFlyoutSubItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);

            // Set the DataContext to button from the root panel
            button->DataContext = rootPanel;

            // Set the MenuFlyout presenter style
            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutPresenter";
            type.Kind = wxaml_interop::TypeKind::Metadata;
            auto style = ref new xaml::Style(type);
            style->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutPresenter::TagProperty, "presenter_style"));
            menuFlyout->MenuFlyoutPresenterStyle = style;

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        auto subItem = GetSubItem(items);
        TapSubMenuItem(subItem);

        auto subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(subPresenter->RequestedTheme == xaml::ElementTheme::Light);
            VERIFY_IS_TRUE(subPresenter->FlowDirection == xaml::FlowDirection::LeftToRight);
            VERIFY_IS_TRUE(subPresenter->Language == L"En-UK");
            VERIFY_IS_TRUE(subPresenter->IsTextScaleFactorEnabled);

            VERIFY_IS_NOT_NULL(subPresenter->DataContext);

            auto tag = subPresenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"presenter_style"), tag->ToString());

        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemProperties()
    {
        TestCleanupWrapper cleanup;

        // Keeping it similar to ValidateSuMenuItemProperties. 
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' >"
                L"  <Button x:Name='button' Content='button.flyout' VerticalAlignment='Center' HorizontalAlignment='Center' "
                L"          RequestedTheme='light' FlowDirection='LeftToRight' Language='En-UK' IsTextScaleFactorEnabled='true' >"
                L"    <Button.Flyout> "
                L"      <MenuFlyout Placement='Bottom'> "
                L"        <MenuFlyoutItem Text='Item 1' /> "
                L"        <MenuFlyoutItem Text='Item 2' /> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <ToggleMenuFlyoutItem Text='Toggle Item3' IsChecked='True' /> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split item 4'>"
                L"          <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"          <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"          <MenuFlyoutSeparator/>"
                L"          <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 2.3</ToggleMenuFlyoutItem>"
                L"         </SplitMenuFlyoutItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);

            // Set the DataContext to button from the root panel
            button->DataContext = rootPanel;

            // Set the MenuFlyout presenter style
            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutPresenter";
            type.Kind = wxaml_interop::TypeKind::Metadata;
            auto style = ref new xaml::Style(type);
            style->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutPresenter::TagProperty, "presenter_style"));
            menuFlyout->MenuFlyoutPresenterStyle = style;

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
        });

        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;
        RunOnUIThread([&]()
        {
            // SplitMenuFlyoutItem is at index 5 (after 2 items, separator, toggle item, separator)
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(5));
        });
        VERIFY_IS_NOT_NULL(splitItem);
        TapSplitMenuItemSecondary(splitItem);

        auto subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(subPresenter->RequestedTheme == xaml::ElementTheme::Light);
            VERIFY_IS_TRUE(subPresenter->FlowDirection == xaml::FlowDirection::LeftToRight);
            VERIFY_IS_TRUE(subPresenter->Language == L"En-UK");
            VERIFY_IS_TRUE(subPresenter->IsTextScaleFactorEnabled);

            VERIFY_IS_NOT_NULL(subPresenter->DataContext);

            auto tag = subPresenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"presenter_style"), tag->ToString());

        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
    }

    void MenuFlyoutIntegrationTests::ValidateThatLayoutTransitionsDoRun()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items;
        auto storyboardMonitor = ref new StoryboardMonitorWrapper();
        int startedStoryboardCount = 0;

        // Prepare the menu flyout
        RunOnUIThread([&]()
        {
            auto grid = ref new xaml_controls::Grid();
            button = ref new xaml_controls::Button();
            button->Content = L"ValidateThatLayoutTransitionsDoRun";
            button->Flyout = CreateMenuFlyoutSubItemsFromXaml();
            items = safe_cast<xaml_controls::MenuFlyout^>(button->Flyout)->Items;
            grid->Children->Append(button);
            TestServices::WindowHelper->WindowContent = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Validate storboard count
        storyboardMonitor->AttachStartedHandler(
            [&](xaml_animation::Storyboard^, xaml::UIElement^ target)
        {
            if (dynamic_cast<xaml_controls::MenuFlyoutPresenter^>(target))
            {
                ++startedStoryboardCount;
            }
        });

        LOG_OUTPUT(L"Open the menu flyout");
        RunOnUIThread([&]()
        {
            button->Flyout->ShowAt(button);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Windowed menus, which we have on desktop, don't have layout transitions.
        VERIFY_ARE_EQUAL(TestServices::Utilities->IsDesktop ? 0 : 1, startedStoryboardCount);

        LOG_OUTPUT(L"Open the sub menu");
        TapSubMenuItem(GetSubItem(items));

        VERIFY_ARE_EQUAL(TestServices::Utilities->IsDesktop ? 0 : 2, startedStoryboardCount);

        LOG_OUTPUT(L"Close menu and sub menu");
        bool backButtonPressHandled = false;
        TestServices::Utilities->InjectBackButtonPress(&backButtonPressHandled);
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_ARE_EQUAL(TestServices::Utilities->IsDesktop ? 0 : 4, startedStoryboardCount);
    }

    void MenuFlyoutIntegrationTests::ValidateRightClickChaining()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Button^ button2 = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto rightTappedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);
        auto rightTappedRegistration = CreateSafeEventRegistration(xaml_controls::Button, RightTapped);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='button1' Content='button.righttapped' VerticalAlignment='Center' HorizontalAlignment='Left' FontSize='25' Padding='25,10' Margin='50'> "
                L"  </Button> "
                L"  <Button x:Name='button2' Content='button.flyout' VerticalAlignment='Center' HorizontalAlignment='Left' FontSize='25' Padding='25,10' Margin='50'> "
                L"    <Button.Flyout> "
                L"      <MenuFlyout Placement='Bottom'> "
                L"        <MenuFlyoutItem FontSize='30' Text='SUPERMAN' Foreground='RoyalBlue' Width='300' /> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <ToggleMenuFlyoutItem FontSize='30' Text='THE FLASH' Foreground='RoyalBlue' Width='300' IsChecked='False' /> "
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;

            button1 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            VERIFY_IS_NOT_NULL(button1);
            button2 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
            VERIFY_IS_NOT_NULL(button2);

            rightTappedRegistration.Attach(button1, ref new xaml::Input::RightTappedEventHandler([rightTappedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                rightTappedEvent->Set();
            }));

            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button2->Flyout);
            VERIFY_IS_NOT_NULL(menuFlyout);

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"CanMenuFlyoutOpenClose: MenuFlyout Opened event is fired!");
                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"CanMenuFlyoutOpenClose: MenuFlyout Closed event is fired!");
                menuFlyoutClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Button Tap operation to show the MenuFlyout.");
        TestServices::InputHelper->Tap(button2);
        menuFlyoutOpenedEvent->WaitForDefault();

        // Inject right-click.
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, button1);
        TestServices::WindowHelper->WaitForIdle();

        // Make sure that the right tap tiggered the flyout's light dismiss
        menuFlyoutClosedEvent->WaitForDefault();

        // Make sure that the right tap gesture was chained through the MenuFlyout's light dismiss layer
        // and received by the next hit target - button1
        rightTappedEvent->WaitForDefault();
    }


    xaml_controls::MenuFlyout^ MenuFlyoutIntegrationTests::CreateMenuFlyoutLongItemsFromXaml()
    {
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>Menu item 0</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <MenuFlyoutSubItem x:Name='subItem1' Text='Menu sub item 1'>"
                L"        <MenuFlyoutItem>Menu item 2.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Menu item 2.2</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 2.3</ToggleMenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <MenuFlyoutSubItem  x:Name='subItem2' Text='Menu sub item 2.4'>"
                L"            <MenuFlyoutItem>Menu item 3.1</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.2</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.3</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.4</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.5</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.6</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.7</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.8</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.9</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.10</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.11</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.12</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.13</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.14</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.15</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.16</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.17</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.18</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.19</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.20</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.21</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.22</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.23</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.24</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.25</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.26</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.27</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.28</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.29</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Menu item 3.30</MenuFlyoutItem>"
                L"        </MenuFlyoutSubItem>"
                L"    </MenuFlyoutSubItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split menu item 2'>"
                L"        <MenuFlyoutItem>Split item 4.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem>Split item 4.2</MenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <ToggleMenuFlyoutItem IsChecked='True'>Toggle split item 4.3</ToggleMenuFlyoutItem>"
                L"        <MenuFlyoutSeparator/>"
                L"        <SplitMenuFlyoutItem x:Name='splitItem2' Text='Split sub item 4.4'>"
                L"            <MenuFlyoutItem>Split item 5.1</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.2</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.3</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.4</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.5</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.6</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.7</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.8</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.9</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.10</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.11</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.12</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.13</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.14</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.15</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.16</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.17</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.18</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.19</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.20</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.21</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.22</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.23</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.24</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.25</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.26</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.27</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.28</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.29</MenuFlyoutItem>"
                L"            <MenuFlyoutItem>Split item 5.30</MenuFlyoutItem>"
                L"        </SplitMenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <MenuFlyoutItem>Menu item 2</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 3</MenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <ToggleMenuFlyoutItem IsChecked='True'>Toggle item 3</ToggleMenuFlyoutItem>"
                L"    <MenuFlyoutSeparator/>"
                L"    <MenuFlyoutItem>Menu item 4</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 5</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 6</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 7</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 8</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 9</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 10</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 11</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 12</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 13</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 14</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 15</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 16</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 17</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 18</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 19</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 20</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 21</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 22</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 23</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 24</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 25</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 26</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 27</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 28</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 29</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 30</MenuFlyoutItem>"
                L"</MenuFlyout>"));
        });

        return menuFlyout;
    }

    void MenuFlyoutIntegrationTests::ValidateSubMenuItemWithLongItems()
    {
        TestCleanupWrapper cleanup;

        wf::Rect subItemBounds = {};
        wf::Rect subPresenterBounds = {};
        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ subItems = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutLongItemsFromXaml();

        RunOnUIThread([&]()
        {
            rootPanel->RequestedTheme = xaml::ElementTheme::Light;
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        // Set the mouse operation for showing the ScrollBar on the long menu items
        TestServices::InputHelper->LeftMouseClick(button1);

        // Show the long main MenuFlyout
        ShowMenuFlyout(menuFlyout, button1, 200, 50);

        auto subPresenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(subPresenter, L"MenuFlyoutPresenterScrollViewer"));
            LOG_OUTPUT(L"Scrollable Height=%f", scrollViewer->ScrollableHeight);
        });
        VERIFY_IS_TRUE(scrollViewer->ScrollableHeight > 0);

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
            subItem = dynamic_cast<xaml_controls::MenuFlyoutSubItem^>(items->GetAt(2));
        });

        MoveToSubMenuItem(subItem);

        subPresenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            subItemBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(subItem));
            LOG_OUTPUT(L"MenuFlyoutSubItem bounds left=%f top=%f width=%f height=%f", subItemBounds.Left, subItemBounds.Top, subItemBounds.Width, subItemBounds.Height);
        });

        // Move the mouse to the boundary of sub menu item then move to the sub presenter
        test_infra::TestServices::InputHelper->MoveMouse(wf::Point(subItemBounds.Left + subItemBounds.Width, subItemBounds.Top + subItemBounds.Height / 2));
        test_infra::TestServices::InputHelper->MoveMouse(wf::Point(subItemBounds.Left + subItemBounds.Width + 100, subItemBounds.Top + subItemBounds.Height / 2));
        TestServices::WindowHelper->WaitForIdle();

        // Verify the sub presenter
        VERIFY_IS_TRUE(subPresenter == GetCurrentPresenter());

        // Get the sub items to invoke the second menu sub presenter
        RunOnUIThread([&]()
        {
            subItems = subItem->Items;
        });

        auto subItem2 = GetSubItem(subItems);

        // Show the second menu sub presenter
        TapSubMenuItem(subItem2);

        auto subPresenter2 = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            subPresenterBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(subPresenter2));
            LOG_OUTPUT(L"Second SubPresenter bounds left=%f top=%f width=%f height=%f", subPresenterBounds.Left, subPresenterBounds.Top, subPresenterBounds.Width, subPresenterBounds.Height);
        });

        test_infra::TestServices::InputHelper->MoveMouse(wf::Point(subPresenterBounds.Left + subPresenterBounds.Width / 2, subPresenterBounds.Top + subPresenterBounds.Height / 2));
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(subPresenter2, L"MenuFlyoutPresenterScrollViewer"));
            LOG_OUTPUT(L"Scrollable Height=%f", scrollViewer->ScrollableHeight);
        });
        VERIFY_IS_TRUE(scrollViewer->ScrollableHeight > 0);

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemWithLongItems()
    {
        TestCleanupWrapper cleanup;

        wf::Rect splitItemBounds = {};
        wf::Rect subPresenterBounds = {};
        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ subItems = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        xaml_controls::MenuFlyout^ menuFlyout = CreateMenuFlyoutLongItemsFromXaml();

        RunOnUIThread([&]()
        {
            rootPanel->RequestedTheme = xaml::ElementTheme::Light;
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        // Set the mouse operation for showing the ScrollBar on the long menu items
        TestServices::InputHelper->LeftMouseClick(button1);

        // Show the long main MenuFlyout
        ShowMenuFlyout(menuFlyout, button1, 200, 50);

        auto subPresenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(subPresenter, L"MenuFlyoutPresenterScrollViewer"));
            LOG_OUTPUT(L"Scrollable Height=%f", scrollViewer->ScrollableHeight);
        });
        VERIFY_IS_TRUE(scrollViewer->ScrollableHeight > 0);

        RunOnUIThread([&]()
        {
            items = menuFlyout->Items;
            // The SplitMenuFlyoutItem with submenu is at index 4 (after Menu item 0, separator, MenuFlyoutSubItem, separator)
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(4));
        });

        VERIFY_IS_NOT_NULL(splitItem);
        MoveToSplitMenuItemSecondary(splitItem);

        subPresenter = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            splitItemBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(splitItem));
            LOG_OUTPUT(L"SplitMenuFlyoutItem bounds left=%f top=%f width=%f height=%f", splitItemBounds.Left, splitItemBounds.Top, splitItemBounds.Width, splitItemBounds.Height);
        });

        // Move the mouse to the boundary of split menu item then move to the sub presenter
        test_infra::TestServices::InputHelper->MoveMouse(wf::Point(splitItemBounds.Left + splitItemBounds.Width, splitItemBounds.Top + splitItemBounds.Height / 2));
        test_infra::TestServices::InputHelper->MoveMouse(wf::Point(splitItemBounds.Left + splitItemBounds.Width + 100, splitItemBounds.Top + splitItemBounds.Height / 2));
        TestServices::WindowHelper->WaitForIdle();

        // Verify the sub presenter
        VERIFY_IS_TRUE(subPresenter == GetCurrentPresenter());

        // Get the sub items to invoke the nested split menu sub presenter
        xaml_controls::SplitMenuFlyoutItem^ nestedSplitItem = nullptr;
        RunOnUIThread([&]()
        {
            subItems = splitItem->Items;
            // The nested SplitMenuFlyoutItem with submenu is the last item in the collection
            nestedSplitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(subItems->GetAt(subItems->Size - 1));
        });

        VERIFY_IS_NOT_NULL(nestedSplitItem);

        // Show the nested split menu sub presenter
        TapSplitMenuItemSecondary(nestedSplitItem);

        auto subPresenter2 = GetCurrentPresenter();

        RunOnUIThread([&]()
        {
            subPresenterBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(subPresenter2));
            LOG_OUTPUT(L"Nested SplitMenuFlyoutItem SubPresenter bounds left=%f top=%f width=%f height=%f", subPresenterBounds.Left, subPresenterBounds.Top, subPresenterBounds.Width, subPresenterBounds.Height);
        });

        test_infra::TestServices::InputHelper->MoveMouse(wf::Point(subPresenterBounds.Left + subPresenterBounds.Width / 2, subPresenterBounds.Top + subPresenterBounds.Height / 2));
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(subPresenter2, L"MenuFlyoutPresenterScrollViewer"));
            LOG_OUTPUT(L"Nested Scrollable Height=%f", scrollViewer->ScrollableHeight);
        });
        VERIFY_IS_TRUE(scrollViewer->ScrollableHeight > 0);

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::OpenSubItemWithMouse(xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem)
    {
        // Open the MenuFlyoutSubItem by moving the mouse on the first sub item
        TestServices::InputHelper->MoveMouse(menuFlyoutSubItem);
        // Wait for the sub menu to open. It opens after a delay - clicking and waiting for idle doesn't open it.
        // MenuFlyout sub items don't expand on mouse click - they need to wait for the timeout.
        TestServices::WindowHelper->SynchronouslyTickUIThread(60);
    }

    void MenuFlyoutIntegrationTests::ValidateOpenMultiSubMenuItemByMouse()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem = nullptr;
        wf::Rect menuFlyoutSubItem1Bounds = {};
        wf::Rect menuFlyoutSubItem2Bounds = {};

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        auto menuFlyoutSubItemClosedEvent = std::make_shared<Event>();
        auto subItemClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='button' Content='button.flyout' VerticalAlignment='Center' HorizontalAlignment='Left' FontSize='25' Padding='25,10' Margin='50'> "
                L"    <Button.Flyout> "
                L"      <MenuFlyout Placement='Bottom'> "
                L"        <MenuFlyoutItem Text='Item 1' /> "
                L"        <MenuFlyoutSubItem Text='Sub Item 1'> "
                L"            <MenuFlyoutItem>Sub item 1.1</MenuFlyoutItem> "
                L"            <MenuFlyoutItem>Sub item 1.2</MenuFlyoutItem> "
                L"        </MenuFlyoutSubItem> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <MenuFlyoutItem Text='Item 2' /> "
                L"        <MenuFlyoutSubItem Text='Sub Item 2'> "
                L"            <MenuFlyoutItem>Sub item 2.1</MenuFlyoutItem> "
                L"            <MenuFlyoutItem>Sub item 2.2</MenuFlyoutItem> "
                L"        </MenuFlyoutSubItem> "
                L"        <MenuFlyoutSeparator Width='300' /> "
                L"        <MenuFlyoutItem Text='Item 3' /> "
                L"        <MenuFlyoutSubItem Text='Sub Item 3'> "
                L"            <MenuFlyoutItem>Sub item 3.1</MenuFlyoutItem> "
                L"            <MenuFlyoutItem>Sub item 3.2</MenuFlyoutItem> "
                L"        </MenuFlyoutSubItem> "
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            menuFlyoutSubItem = dynamic_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyout->Items->GetAt(1));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutClosedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open the MenuFlyout by tapping the button
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Open the MenuFlyoutSubItem by moving the mouse on the first sub item
        OpenSubItemWithMouse(menuFlyoutSubItem);

        auto presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            menuFlyoutSubItem1Bounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyoutSubItem1 bounds left=%f top=%f width=%f height=%f", menuFlyoutSubItem1Bounds.Left, menuFlyoutSubItem1Bounds.Top, menuFlyoutSubItem1Bounds.Width, menuFlyoutSubItem1Bounds.Height);

            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                button->XamlRoot);

            auto popup = popups->GetAt(0);
            subItemClosedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([menuFlyoutSubItemClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutSubItemClosedEvent->Set();
            }));
        });

        // Move the mouse to the out of the MenuFlyout bounds
        test_infra::TestServices::InputHelper->MoveMouse(wf::Point(menuFlyoutSubItem1Bounds.Left + menuFlyoutSubItem1Bounds.Width / 2, 0));
        test_infra::TestServices::InputHelper->MoveMouse(wf::Point(menuFlyoutSubItem1Bounds.Left + menuFlyoutSubItem1Bounds.Width / 2, 1));
        menuFlyoutSubItemClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Get the second sub item
            menuFlyoutSubItem = dynamic_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyout->Items->GetAt(4));
        });

        // Move the mouse to the second sub menu item to close the previous sub item and open the second sub item
        OpenSubItemWithMouse(menuFlyoutSubItem);

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            menuFlyoutSubItem2Bounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyoutSubItem2 bounds left=%f top=%f width=%f height=%f", menuFlyoutSubItem2Bounds.Left, menuFlyoutSubItem2Bounds.Top, menuFlyoutSubItem2Bounds.Width, menuFlyoutSubItem2Bounds.Height);

            VERIFY_IS_TRUE(menuFlyoutSubItem1Bounds.Top < menuFlyoutSubItem2Bounds.Top);

            // Close the MenuFlyout
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::ValidateRequestedThemeOnPresenterTakesEffect()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Grid.Resources> "
                L"      <ResourceDictionary> "
                L"          <ResourceDictionary.ThemeDictionaries> "
                L"              <ResourceDictionary x:Key='Default'> "
                L"                  <SolidColorBrush x:Key='CustomPresenterBackground' Color='Green' /> "
                L"              </ResourceDictionary> "
                L"              <ResourceDictionary x:Key='Light'> "
                L"                  <SolidColorBrush x:Key='CustomPresenterBackground' Color='Blue' /> "
                L"              </ResourceDictionary> "
                L"          </ResourceDictionary.ThemeDictionaries> "
                L"      </ResourceDictionary> "
                L"  </Grid.Resources> "
                L"  <Button x:Name='button' Content='button.flyout' VerticalAlignment='Center' HorizontalAlignment='Left' FontSize='25' Padding='25,10' Margin='50' RequestedTheme='Dark'> "
                L"    <Button.Flyout> "
                L"      <MenuFlyout Placement='Bottom'> "
                L"        <MenuFlyout.MenuFlyoutPresenterStyle> "
                L"          <Style TargetType='MenuFlyoutPresenter'> "
                L"            <Setter Property='Background' Value='{ThemeResource CustomPresenterBackground}' /> "
                L"            <Setter Property='RequestedTheme' Value='Light' /> "
                L"          </Style> "
                L"        </MenuFlyout.MenuFlyoutPresenterStyle> "
                L"        <MenuFlyoutItem Text='Item 1' /> "
                L"        <MenuFlyoutItem Text='Item 2' /> "
                L"        <MenuFlyoutItem Text='Item 3' /> "
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutClosedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open the MenuFlyout by tapping the button
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            auto backgroundColor = safe_cast<xaml_media::SolidColorBrush^>(presenter->Background)->Color;

            VERIFY_ARE_EQUAL(0xFF, backgroundColor.A);
            VERIFY_ARE_EQUAL(0x00, backgroundColor.R);
            VERIFY_ARE_EQUAL(0x00, backgroundColor.G);
            VERIFY_ARE_EQUAL(0xFF, backgroundColor.B);
        });

        RunOnUIThread([&]()
        {
            // Close the MenuFlyout
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::ValidateCanChangeRequestedThemeOnPresenterOwner()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Grid.Resources> "
                L"      <ResourceDictionary> "
                L"          <ResourceDictionary.ThemeDictionaries> "
                L"              <ResourceDictionary x:Key='Default'> "
                L"                  <SolidColorBrush x:Key='CustomPresenterBackground' Color='Green' /> "
                L"              </ResourceDictionary> "
                L"              <ResourceDictionary x:Key='Light'> "
                L"                  <SolidColorBrush x:Key='CustomPresenterBackground' Color='Blue' /> "
                L"              </ResourceDictionary> "
                L"          </ResourceDictionary.ThemeDictionaries> "
                L"      </ResourceDictionary> "
                L"  </Grid.Resources> "
                L"  <Button x:Name='button' Content='button.flyout' VerticalAlignment='Center' HorizontalAlignment='Left' FontSize='25' Padding='25,10' Margin='50' RequestedTheme='Dark'> "
                L"    <Button.Flyout> "
                L"      <MenuFlyout Placement='Bottom'> "
                L"        <MenuFlyout.MenuFlyoutPresenterStyle> "
                L"          <Style TargetType='MenuFlyoutPresenter'> "
                L"            <Setter Property='Background' Value='{ThemeResource CustomPresenterBackground}' /> "
                L"          </Style> "
                L"        </MenuFlyout.MenuFlyoutPresenterStyle> "
                L"        <MenuFlyoutItem Text='Item 1' /> "
                L"        <MenuFlyoutItem Text='Item 2' /> "
                L"        <MenuFlyoutItem Text='Item 3' /> "
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(button->Flyout);

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutClosedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open the MenuFlyout by tapping the button
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            auto backgroundColor = safe_cast<xaml_media::SolidColorBrush^>(presenter->Background)->Color;

            VERIFY_ARE_EQUAL(0xFF, backgroundColor.A);
            VERIFY_ARE_EQUAL(0x00, backgroundColor.R);
            VERIFY_ARE_EQUAL(0x80, backgroundColor.G);
            VERIFY_ARE_EQUAL(0x00, backgroundColor.B);
        });

        RunOnUIThread([&]()
        {
            // Close the MenuFlyout
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button->RequestedTheme = xaml::ElementTheme::Light;
        });

        // Open the MenuFlyout by tapping the button
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            auto backgroundColor = safe_cast<xaml_media::SolidColorBrush^>(presenter->Background)->Color;

            VERIFY_ARE_EQUAL(0xFF, backgroundColor.A);
            VERIFY_ARE_EQUAL(0x00, backgroundColor.R);
            VERIFY_ARE_EQUAL(0x00, backgroundColor.G);
            VERIFY_ARE_EQUAL(0xFF, backgroundColor.B);
        });

        RunOnUIThread([&]()
        {
            // Close the MenuFlyout
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::PerformFlowDirectionTest(bool hasElement, bool hasPoint, bool isRTL, bool expectSuccess)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ button1 = nullptr;
        wf::Rect button1Bounds = {};
        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        auto menuFlyout = CreateMenuFlyoutWithSubItem();

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid Background='Orange' "
                L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <Button x:Name='button1' Content='Button' Width='100' Height='50' />"
                L"</Grid>"));

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));

            if (isRTL)
            {
                rootPanel->FlowDirection = xaml::FlowDirection::RightToLeft;
            }

            openedRegistration.Attach(
                menuFlyout,
                ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
            {
                openedEvent->Set();
            }));

            closedRegistration.Attach(
                menuFlyout,
                ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
            {
                closedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Is Relative=%d Has Offset=%d Is RTL=%d", hasElement, hasPoint, isRTL);

            Platform::Exception^ caughtException;
            try
            {
                if (hasPoint)
                {
                    if (hasElement)
                    {
                        menuFlyout->ShowAt(button1, wf::Point(50, 50));
                    }
                    else
                    {
                        menuFlyout->ShowAt(nullptr, wf::Point(50, 50));
                    }
                }
                else
                {
                    menuFlyout->ShowAt(button1);
                }
            }
            catch (Platform::Exception^ ex)
            {
                caughtException = ex;
                if (expectSuccess)
                {
                    throw ex;
                }
                else
                {
                    openedEvent->Set();
                }
            }

            if (!expectSuccess)
            {
                VERIFY_IS_NOT_NULL(caughtException);
            }
        });
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                button1->XamlRoot);

            if (expectSuccess)
            {
                auto popup = popups->GetAt(popups->Size - 1);
                auto child = popup->Child;

                VERIFY_IS_TRUE(popup->IsOpen);

                if (isRTL)
                {
                    VERIFY_ARE_EQUAL(xaml::FlowDirection::RightToLeft, (safe_cast<xaml::FrameworkElement^>(child))->FlowDirection);
                }
                else
                {
                    VERIFY_ARE_EQUAL(xaml::FlowDirection::LeftToRight, (safe_cast<xaml::FrameworkElement^>(child))->FlowDirection);
                }
            }
            else
            {
                VERIFY_ARE_EQUAL(0, popups->Size);
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        if (expectSuccess)
        {
            FlyoutHelper::HideFlyout(menuFlyout);
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void MenuFlyoutIntegrationTests::ValidateShowAtFlowDirection()
    {
        TestCleanupWrapper cleanup;
        //Test Case:ShowAt(element)
        PerformFlowDirectionTest(true, false, false, true);
        //Test Case:ShowAt(element,point)
        PerformFlowDirectionTest(true, true, false, true);
        //Test Case:ShowAt(null,point)
        PerformFlowDirectionTest(false, true, false, false);
        //RTL
        //Test Case:ShowAt(element)
        PerformFlowDirectionTest(true, false, true, true);
        //Test Case:ShowAt(element,point)
        PerformFlowDirectionTest(true, true, true, true);
        //Test Case:ShowAt(null,point)
        PerformFlowDirectionTest(false, true, true, false);
    }

    void MenuFlyoutIntegrationTests::ValidateFlyoutSizingForDifferentInputModes()
    {
        TestCleanupWrapper cleanup;

        // There is a reliability issue for keyboard injecting that waits for the event from the InputManager
        // after sending the input injection. This is the work around to disable WaitForEvent.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        const double expectedFlyoutWidth_Touch = 240;
        const double expectedFlyoutWidth_NonTouch = 96;
        const xaml::Thickness expectedFlyoutContentMargin = xaml::Thickness({ 0, 4, 0, 4 });

        // Narrow should occur when interacting using a mouse, pen, or keyboard.
        // Wide should occur when interacting using touch, a gamepad, or a remote.
        const xaml::Thickness expectedMenuFlyoutItemPadding_Narrow = xaml::Thickness({ 12, 5, 12, 7 });
        const xaml::Thickness expectedMenuFlyoutItemPadding_Wide = xaml::Thickness({ 12, 11, 12, 13 });

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::Button^ button;
        xaml_controls::MenuFlyout^ menuFlyout;
        xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem;

        xaml::FrameworkElement^ menuFlyoutRoot;
        xaml_controls::ItemsPresenter^ menuFlyoutItemsPresenter;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                          x:Name='root'>
                        <Button x:Name='button' Content='button.flyout'>
                            <Button.Flyout>
                                <MenuFlyout>
                                    <MenuFlyoutItem Text='Item 1' />
                                    <MenuFlyoutSeparator />
                                    <ToggleMenuFlyoutItem Text='Item 2' />
                                    <MenuFlyoutSubItem Text='Sub Item 1'>
                                        <MenuFlyoutItem>Sub item 1</MenuFlyoutItem>
                                        <MenuFlyoutItem>Sub item 2</MenuFlyoutItem>
                                    </MenuFlyoutSubItem>
                                </MenuFlyout>
                            </Button.Flyout>
                        </Button>
                    </Grid>)"));

            TestServices::WindowHelper->WindowContent = root;

            button = safe_cast<xaml_controls::Button^>(root->FindName(L"button"));
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            menuFlyoutSubItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyout->Items->GetAt(3));

            button->ContextFlyout = menuFlyout;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open MenuFlyout via touch.  The items should have wide padding.");
        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Touch);

        TestServices::InputHelper->Tap(menuFlyoutSubItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            menuFlyoutRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"MenuFlyoutPresenterScrollViewer", button));
            menuFlyoutItemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(menuFlyoutRoot);
            VERIFY_IS_NOT_NULL(menuFlyoutRoot);
            VERIFY_IS_NOT_NULL(menuFlyoutItemsPresenter);

            VERIFY_ARE_EQUAL(expectedFlyoutWidth_Touch, menuFlyoutRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedFlyoutContentMargin, menuFlyoutItemsPresenter->Margin);
            VerifyMenuFlyoutItemsPadding(menuFlyout->Items, expectedMenuFlyoutItemPadding_Wide);
        });
        FlyoutHelper::HideFlyout(menuFlyout);

        LOG_OUTPUT(L"Open MenuFlyout via gamepad.  The items should have wide padding.");
        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Gamepad);

        RunOnUIThread([&]()
        {
            menuFlyoutSubItem->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Accept(InputDevice::Gamepad);

        RunOnUIThread([&]()
        {
            menuFlyoutRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"MenuFlyoutPresenterScrollViewer", button));
            menuFlyoutItemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(menuFlyoutRoot);

            VERIFY_ARE_EQUAL(expectedFlyoutWidth_Touch, menuFlyoutRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedFlyoutContentMargin, menuFlyoutItemsPresenter->Margin);
            VerifyMenuFlyoutItemsPadding(menuFlyout->Items, expectedMenuFlyoutItemPadding_Wide);
        });
        FlyoutHelper::HideFlyout(menuFlyout);

        LOG_OUTPUT(L"Open MenuFlyout via pen.  The items should have narrow padding.");
        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Pen);

        TestServices::WindowHelper->WaitForIdle();
        //Sub menu opens with pen hover and not with a tap, but InputHelper doesn't currently have a way to input pen hovers.
        //Luckily PenHold does the trick.
        TestServices::InputHelper->PenHold(menuFlyoutSubItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            menuFlyoutRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"MenuFlyoutPresenterScrollViewer", button));
            menuFlyoutItemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(menuFlyoutRoot);

            VERIFY_ARE_EQUAL(expectedFlyoutWidth_NonTouch, menuFlyoutRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedFlyoutContentMargin, menuFlyoutItemsPresenter->Margin);
            VerifyMenuFlyoutItemsPadding(menuFlyout->Items, expectedMenuFlyoutItemPadding_Narrow);
        });
        FlyoutHelper::HideFlyout(menuFlyout);

        if (!TestServices::Utilities->IsOneCore)
        {
            LOG_OUTPUT(L"Open MenuFlyout via mouse.  The items should have narrow padding.");
            FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Mouse);

            OpenSubItemWithMouse(menuFlyoutSubItem);

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(expectedFlyoutWidth_NonTouch, menuFlyoutRoot->MinWidth);
                VERIFY_ARE_EQUAL(expectedFlyoutContentMargin, menuFlyoutItemsPresenter->Margin);
                VerifyMenuFlyoutItemsPadding(menuFlyout->Items, expectedMenuFlyoutItemPadding_Narrow);
            });
            FlyoutHelper::HideFlyout(menuFlyout);
        }

        LOG_OUTPUT(L"Open MenuFlyout via keyboard.  The items should have narrow padding.");
        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Keyboard);

        // Move to the sub menu item from the first item that requires two down
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        // Open the sub menu item by sending the right keyboard
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedFlyoutWidth_NonTouch, menuFlyoutRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedFlyoutContentMargin, menuFlyoutItemsPresenter->Margin);
            VerifyMenuFlyoutItemsPadding(menuFlyout->Items, expectedMenuFlyoutItemPadding_Narrow);
        });
        FlyoutHelper::HideFlyout(menuFlyout);

        LOG_OUTPUT(L"Open MenuFlyout programmatically.  The items should have the same padding as before.");
        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedFlyoutWidth_NonTouch, menuFlyoutRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedFlyoutContentMargin, menuFlyoutItemsPresenter->Margin);
            VerifyMenuFlyoutItemsPadding(menuFlyout->Items, expectedMenuFlyoutItemPadding_Narrow);
        });
        FlyoutHelper::HideFlyout(menuFlyout);

        LOG_OUTPUT(L"Open MenuFlyout via gamepad.  The items should have wide padding.");
        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Gamepad);

        // Move to the sub menu item from the first item that requires two Gamepad Dpad down
        TestServices::KeyboardHelper->GamepadDpadDown();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->GamepadDpadDown();
        TestServices::WindowHelper->WaitForIdle();

        // Open the sub menu item by using Dpad right
        TestServices::KeyboardHelper->GamepadDpadRight();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedFlyoutWidth_Touch, menuFlyoutRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedFlyoutContentMargin, menuFlyoutItemsPresenter->Margin);
            VerifyMenuFlyoutItemsPadding(menuFlyout->Items, expectedMenuFlyoutItemPadding_Wide);
        });
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateMenuFlyoutSizeByTouch()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        double const expectedMenuFlyoutWidth_Touch = 242;           // 240 (touch min width with touch) + 2 (margin)
        double const expectedMenuFlyoutHeight_Touch = 169;          // Updated to include SplitMenuFlyoutItem
        double const expectedMenuFlyoutSubWidth_Touch = 242;        // 240 (touch min width with touch) + 2 (margin)
        double const expectedMenuFlyoutSubHeight_Touch = 106;
        double const expectedMenuFlyoutItemWidth_Touch = 240;       // 240 (touch min width with touch)
        double const expectedMenuFlyoutItemHeight_Touch = 40;
        double const expectedMenuFlyoutSeparatorWidth_Touch = 240;  // 240 (touch min width with touch)
        double const expectedMenuFlyoutSeparatorHeight_Touch = 3;
        double const expectedMenuFlyoutMinWidth_Touch = 242;        // 240 (touch min width with touch) + 2 (margin)
        double const expectedMenuFlyoutMinHeight_Touch = 32;

        xaml_controls::Grid^ root = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitMenuFlyoutItem = nullptr;
        xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem = nullptr;

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                          x:Name='root'>
                        <Button x:Name='button' Content='button.flyout'>
                            <Button.Flyout>
                                <MenuFlyout>
                                    <MenuFlyoutItem x:Name='firstMenuFlyoutItem' Text='Item 1' />
                                    <MenuFlyoutSeparator x:Name='menuFlyoutSeparator' />
                                    <ToggleMenuFlyoutItem x:Name='toggleMenuFlyoutItem' Text='Item 2' />
                                    <SplitMenuFlyoutItem x:Name='splitMenuFlyoutItem' Text='Split Item 1'>
                                        <MenuFlyoutItem>Split sub item 1</MenuFlyoutItem>
                                        <MenuFlyoutItem>Split sub item 2</MenuFlyoutItem>
                                    </SplitMenuFlyoutItem>
                                    <MenuFlyoutSubItem x:Name='menuFlyoutSubItem' Text='Sub Item 1'>
                                        <MenuFlyoutItem>Sub item 1</MenuFlyoutItem>
                                        <MenuFlyoutItem>Sub item 2</MenuFlyoutItem>
                                    </MenuFlyoutSubItem>
                                </MenuFlyout>
                            </Button.Flyout>
                        </Button>
                    </Grid>)"));

            TestServices::WindowHelper->WindowContent = root;

            button = safe_cast<xaml_controls::Button^>(root->FindName(L"button"));
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitMenuFlyoutItem = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(menuFlyout->Items->GetAt(3));
            menuFlyoutSubItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyout->Items->GetAt(4));
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Touch);

        RunOnUIThread([&]()
        {
            // Verify MenuFlyout actual width/height.
            auto menuFlyoutPresenter = GetCurrentPresenter();
            VERIFY_ARE_EQUAL(expectedMenuFlyoutWidth_Touch, menuFlyoutPresenter->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutHeight_Touch, menuFlyoutPresenter->ActualHeight);

            // Verify MenuFlyoutItem actual width/height.
            auto firstMenuFlyoutItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"firstMenuFlyoutItem"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemWidth_Touch, firstMenuFlyoutItem->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemHeight_Touch, firstMenuFlyoutItem->ActualHeight);

            // Verify MenuFlyoutSeparator actual width/height.
            auto menuFlyoutSeparator = safe_cast<xaml_controls::MenuFlyoutSeparator^>(menuFlyoutPresenter->FindName(L"menuFlyoutSeparator"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutSeparatorWidth_Touch, menuFlyoutSeparator->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutSeparatorHeight_Touch, menuFlyoutSeparator->ActualHeight);

            // Verify ToggleMenuFlyout actual width/height.
            auto toggleMenuFlyoutItem = safe_cast<xaml_controls::ToggleMenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"toggleMenuFlyoutItem"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemWidth_Touch, toggleMenuFlyoutItem->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemHeight_Touch, toggleMenuFlyoutItem->ActualHeight);

            // Verify SplitMenuFlyoutItem actual width/height.
            auto splitMenuFlyoutItem = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"splitMenuFlyoutItem"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemWidth_Touch, splitMenuFlyoutItem->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemHeight_Touch, splitMenuFlyoutItem->ActualHeight);

            // Verify MenuFlyoutSubItem actual width/height.
            auto menuFlyoutSubItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyoutPresenter->FindName(L"menuFlyoutSubItem"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemWidth_Touch, menuFlyoutSubItem->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemHeight_Touch, menuFlyoutSubItem->ActualHeight);
        });

        // Verify the empty MenuFlyout width/height with touch.
        TestServices::InputHelper->Tap(root);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            menuFlyout = ref new xaml_controls::MenuFlyout();
        });
        ShowMenuFlyout(menuFlyout, button, 0, 50);
        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            VERIFY_ARE_EQUAL(expectedMenuFlyoutMinWidth_Touch, menuFlyoutPresenter->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutMinHeight_Touch, menuFlyoutPresenter->ActualHeight);
        });
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateMenuFlyoutSizeByMouse()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        double const expectedMenuFlyoutWidth_Mouse = 163;
        double const expectedMenuFlyoutHeight_Mouse = 137;
        double const expectedMenuFlyoutSubWidth_Mouse = 99;
        double const expectedMenuFlyoutSubHeight_Mouse = 70;
        double const expectedMenuFlyoutItemWidth_Mouse = 161;
        double const expectedMenuFlyoutItemHeight_Mouse = 32;
        double const expectedMenuFlyoutSeparatorWidth_Mouse = 161;
        double const expectedMenuFlyoutSeparatorHeight_Mouse = 3;
        double const expectedMenuFlyoutMinWidth_Mouse = 98;
        double const expectedMenuFlyoutMinHeight_Mouse = 32;

        xaml_controls::Grid^ root = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitMenuFlyoutItem = nullptr;
        xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem = nullptr;

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                          x:Name='root'>
                        <Button x:Name='button' Content='button.flyout'>
                            <Button.Flyout>
                                <MenuFlyout>
                                    <MenuFlyoutItem x:Name='firstMenuFlyoutItem' Text='Item 1' />
                                    <MenuFlyoutSeparator x:Name='menuFlyoutSeparator' />
                                    <ToggleMenuFlyoutItem x:Name='toggleMenuFlyoutItem' Text='Item 2' />
                                    <SplitMenuFlyoutItem x:Name='splitMenuFlyoutItem' Text='Split 1'>
                                        <MenuFlyoutItem>Sub item 1</MenuFlyoutItem>
                                        <MenuFlyoutItem>Sub item 2</MenuFlyoutItem>
                                    </SplitMenuFlyoutItem>
                                    <MenuFlyoutSubItem x:Name='menuFlyoutSubItem' Text='Sub Item 1'>
                                        <MenuFlyoutItem>Sub item 1</MenuFlyoutItem>
                                        <MenuFlyoutItem>Sub item 2</MenuFlyoutItem>
                                    </MenuFlyoutSubItem>
                                </MenuFlyout>
                            </Button.Flyout>
                        </Button>
                    </Grid>)"));

            TestServices::WindowHelper->WindowContent = root;

            button = safe_cast<xaml_controls::Button^>(root->FindName(L"button"));
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitMenuFlyoutItem = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(menuFlyout->Items->GetAt(3));
            menuFlyoutSubItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyout->Items->GetAt(4));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verif the MenuFlyout presenter's actual width/height with mouse.
        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Mouse);
        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            VERIFY_ARE_EQUAL(expectedMenuFlyoutWidth_Mouse, menuFlyoutPresenter->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutHeight_Mouse, menuFlyoutPresenter->ActualHeight);

            auto firstMenuFlyoutItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"firstMenuFlyoutItem"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemWidth_Mouse, firstMenuFlyoutItem->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemHeight_Mouse, firstMenuFlyoutItem->ActualHeight);

            auto menuFlyoutSeparator = safe_cast<xaml_controls::MenuFlyoutSeparator^>(menuFlyoutPresenter->FindName(L"menuFlyoutSeparator"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutSeparatorWidth_Mouse, menuFlyoutSeparator->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutSeparatorHeight_Mouse, menuFlyoutSeparator->ActualHeight);

            auto toggleMenuFlyoutItem = safe_cast<xaml_controls::ToggleMenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"toggleMenuFlyoutItem"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemWidth_Mouse, toggleMenuFlyoutItem->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemHeight_Mouse, toggleMenuFlyoutItem->ActualHeight);

            auto splitMenuFlyoutItem = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"splitMenuFlyoutItem"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemWidth_Mouse, splitMenuFlyoutItem->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemHeight_Mouse, splitMenuFlyoutItem->ActualHeight);

            auto menuFlyoutSubItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyoutPresenter->FindName(L"menuFlyoutSubItem"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemWidth_Mouse, menuFlyoutSubItem->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutItemHeight_Mouse, menuFlyoutSubItem->ActualHeight);
        });

        // Verif the MenuFlyoutSubItem presenter's actual width/height with mouse.
        OpenSubItemWithMouse(menuFlyoutSubItem);
        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            VERIFY_ARE_EQUAL(expectedMenuFlyoutSubWidth_Mouse, menuFlyoutPresenter->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutSubHeight_Mouse, menuFlyoutPresenter->ActualHeight);
        });
        FlyoutHelper::HideFlyout(menuFlyout);

        // Verify the SplitMenuFlyoutItem submenu presenter's actual width/height with mouse.
        FlyoutHelper::OpenFlyout(menuFlyout, button, FlyoutOpenMethod::Mouse);
        MoveToSplitMenuItemSecondary(splitMenuFlyoutItem);
        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            VERIFY_ARE_EQUAL(expectedMenuFlyoutSubWidth_Mouse, menuFlyoutPresenter->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutSubHeight_Mouse, menuFlyoutPresenter->ActualHeight);
        });
        FlyoutHelper::HideFlyout(menuFlyout);

        // Verify the empty MenuFlyout width/height with mouse.
        test_infra::TestServices::InputHelper->LeftMouseClick(root);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            menuFlyout = ref new xaml_controls::MenuFlyout();
        });
        ShowMenuFlyout(menuFlyout, button, 0, 50, false /* forceTapAsPreviousInputMessage */);
        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            VERIFY_ARE_EQUAL(expectedMenuFlyoutMinWidth_Mouse, menuFlyoutPresenter->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuFlyoutMinHeight_Mouse, menuFlyoutPresenter->ActualHeight);
        });
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateScrollableItems()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        double const expectedMenuFlyoutHeight = 300;
        double const expectedMenuFlyoutScrollableHeight = 1041;

        xaml_controls::Grid^ root = nullptr;

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto menuFlyout = CreateMenuFlyoutLongItemsFromXaml();

        // Specify the MenuFlyout height with 300
        RunOnUIThread([&]()
        {
            auto type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutPresenter";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto style = ref new xaml::Style(type);
            style->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutPresenter::MaxHeightProperty, "300"));

            menuFlyout->MenuFlyoutPresenterStyle = style;
        });

        ShowMenuFlyout(menuFlyout, nullptr, 100, 200);
        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            VERIFY_ARE_EQUAL(expectedMenuFlyoutHeight, menuFlyoutPresenter->ActualHeight);

            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(menuFlyoutPresenter, L"MenuFlyoutPresenterScrollViewer"));
            VERIFY_ARE_EQUAL(expectedMenuFlyoutScrollableHeight, scrollViewer->ScrollableHeight);
        });
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateMenuFlyoutInVisibleBounds()
    {
        // Flyout uses the visible bounds since TH2 platform.
        ValidateMenuFlyoutPosition(::Windows::UI::ViewManagement::ApplicationViewBoundsMode::UseVisible);
    }

    void MenuFlyoutIntegrationTests::ValidateMenuFlyoutPosition(::Windows::UI::ViewManagement::ApplicationViewBoundsMode expectedBoundsMode)
    {
        TestCleanupWrapper cleanup;

        // Set the specified visible bounds to verify the MenuFlyout position on the visible bounds.
        TestServices::WindowHelper->SetVisibleBounds(wf::Rect(0, 50, 480, 700));

        auto menuFlyout = CreateMenuFlyoutWithSubItem();

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Orange' >
                        <TextBlock Text='Test MenuFlyout on the visible bounds.' FontSize='20' />
                    </Grid>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Show the MenuFlout on the out of the visible bounds.
        ShowMenuFlyout(menuFlyout, nullptr, 100, 0, true /* forceTapAsPreviousInputMessage */);
        TestServices::WindowHelper->WaitForIdle();

        auto presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            auto menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);

            if (expectedBoundsMode == ::Windows::UI::ViewManagement::ApplicationViewBoundsMode::UseVisible)
            {
                VERIFY_ARE_EQUAL(menuFlyoutBounds.Top, 50);
            }
            else
            {
                VERIFY_IS_LESS_THAN(menuFlyoutBounds.Top, 50);
            }
        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });

        // Set the window with the landscape mode with specifying the visible bounds.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 480));
        TestServices::WindowHelper->SetVisibleBounds(wf::Rect(50, 0, 700, 480));
        TestServices::WindowHelper->WaitForIdle();

        // Show the MenuFlout on the out of the visible bounds.
        ShowMenuFlyout(menuFlyout, nullptr, 550, 100, true /* forceTapAsPreviousInputMessage */);
        TestServices::WindowHelper->WaitForIdle();

        presenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            auto menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(presenter));
            LOG_OUTPUT(L"MenuFlyout bounds left=%f top=%f width=%f height=%f", menuFlyoutBounds.Left, menuFlyoutBounds.Top, menuFlyoutBounds.Width, menuFlyoutBounds.Height);

            if (expectedBoundsMode == ::Windows::UI::ViewManagement::ApplicationViewBoundsMode::UseVisible)
            {
                VERIFY_IS_LESS_THAN(menuFlyoutBounds.Left + menuFlyoutBounds.Width, 750);
            }
            else
            {
                VERIFY_IS_GREATER_THAN(menuFlyoutBounds.Left + menuFlyoutBounds.Width, 750);
            }
        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
    }

    void MenuFlyoutIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;
        FlyoutHelper::ValidateOverlayBrush<xaml_controls::MenuFlyout>(L"MenuFlyoutLightDismissOverlayBackground");
    }

    void MenuFlyoutIntegrationTests::ValidateCanUseBothShowAtMethods()
    {
        TestCleanupWrapper cleanup;

        auto menuFlyout = CreateMenuFlyout();
        xaml_controls::Button^ button = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Loaded);

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button();
            button->Content = "Button for flyout";

            loadedRegistration.Attach(button, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = button;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        openedRegistration.Attach(menuFlyout, [&](){ openedEvent->Set(); });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Show the MenuFlyout first using ShowAt() with one parameter.");
            menuFlyout->ShowAt(button);
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Now show the MenuFlyout using ShowAt() with two parameters.");
            menuFlyout->ShowAt(button, {0, 0});
        });
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);

        openedRegistration.Detach();

        LOG_OUTPUT(L"Now create a new MenuFlyout to test the other calling order.");
        menuFlyout = CreateMenuFlyout();
        openedRegistration.Attach(menuFlyout, [&](){ openedEvent->Set(); });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Show the MenuFlyout first using ShowAt() with two parameters.");
            menuFlyout->ShowAt(button, {0, 0});
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Now show the MenuFlyout using ShowAt() with one parameter.");
            menuFlyout->ShowAt(button);
        });
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidatePopupWindowedScrollingWithMouse()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));
        xaml_controls::Grid^ root = nullptr;
        xaml_primitives::Thumb^ thumb = nullptr;
        wf::Rect thumbBoundsBeforeDrag = {};
        wf::Rect thumbBoundsAfterDrag = {};

        RunOnUIThread([&]()
        {
            root = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto menuFlyout = CreateMenuFlyoutLongItemsFromXaml();

        ShowMenuFlyout(menuFlyout, nullptr, 100, 200);
        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(menuFlyoutPresenter, L"MenuFlyoutPresenterScrollViewer"));
            VERIFY_IS_NOT_NULL(scrollViewer);

            thumb = safe_cast<xaml_primitives::Thumb^>(TreeHelper::GetVisualChildByName(scrollViewer, L"VerticalThumb"));
            VERIFY_IS_NOT_NULL(thumb);

            thumbBoundsBeforeDrag = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(thumb));
            LOG_OUTPUT(L"ValidatePopupWindowedScrollingWithMouse: Thumb bounds left=%f top=%f width=%f height=%f", thumbBoundsBeforeDrag.Left, thumbBoundsBeforeDrag.Top, thumbBoundsBeforeDrag.Width, thumbBoundsBeforeDrag.Height);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Dragging the thumb to scroll ListViewItem content
        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        LOG_OUTPUT(L"ValidatePopupWindowedScrollingWithMouse: Dragging the thumb to scroll the content.");
        TestServices::InputHelper->MouseDrag(
            wf::Point(thumbBoundsBeforeDrag.Left + (thumbBoundsBeforeDrag.Width / 2), thumbBoundsBeforeDrag.Top + (thumbBoundsBeforeDrag.Height / 2)),
            wf::Point(thumbBoundsBeforeDrag.Left + (thumbBoundsBeforeDrag.Width / 2), thumbBoundsBeforeDrag.Top + (thumbBoundsBeforeDrag.Height / 2) + 200),
            MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            thumbBoundsAfterDrag = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(thumb));
            LOG_OUTPUT(L"ValidatePopupWindowedScrollingWithMouse: Thumb bounds after dragging left=%f top=%f width=%f height=%f", thumbBoundsAfterDrag.Left, thumbBoundsAfterDrag.Top, thumbBoundsAfterDrag.Width, thumbBoundsAfterDrag.Height);
            // Conscious scrollbars feature results in the thumb expanding to 16 wide when the pointer is over the scrollbar, which it will be after dragging here
            // So checking the left and width of the thumb is not desirable
            VERIFY_IS_GREATER_THAN(thumbBoundsAfterDrag.Top, thumbBoundsBeforeDrag.Top);
            VERIFY_ARE_EQUAL(thumbBoundsBeforeDrag.Height, thumbBoundsAfterDrag.Height);
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateKeyboardNavigationAfterClosingSubMenu()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);

        auto closedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootGrid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      Width='400' Height='400'>"
                L"  <Button x:Name='button' Content='Button with Flyout' HorizontalAlignment='Center' VerticalAlignment='Center'>"
                L"    <Button.Flyout>"
                L"      <Flyout x:Name='flyout'>"
                L"        <StackPanel>"
                L"          <MenuFlyoutItem>MenuFlyoutItem</MenuFlyoutItem>"
                L"          <MenuFlyoutSubItem Text='MenuFlyoutSubItem'>"
                L"            <MenuFlyoutItem>MenuFlyoutItem</MenuFlyoutItem>"
                L"          </MenuFlyoutSubItem>"
                L"        </StackPanel>"
                L"      </Flyout>"
                L"    </Button.Flyout>"
                L"  </Button>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootGrid);

            loadedRegistration.Attach(rootGrid, [loadedEvent]()
            {
                LOG_OUTPUT(L"Grid loaded.");
                loadedEvent->Set();
            });

            auto button = safe_cast<xaml_controls::Button^>(rootGrid->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            auto flyout = safe_cast<xaml_controls::Flyout^>(button->Flyout);
            VERIFY_IS_NOT_NULL(flyout);

            openedRegistration.Attach(flyout, [openedEvent]()
            {
                LOG_OUTPUT(L"Flyout opened.");
                openedEvent->Set();
            });

            closedRegistration.Attach(flyout, [closedEvent]()
            {
                LOG_OUTPUT(L"Flyout closed.");
                closedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tab into the Button.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open menu.");
        TestServices::KeyboardHelper->Enter();
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Move down to the sub menu.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the sub menu.");
        CommonInputHelper::Right(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Close the sub menu.");
        CommonInputHelper::Left(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Hit up arrow key.");
        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Close menu.");
        TestServices::KeyboardHelper->Escape();
        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::ValidateOpenedSubMenuFocusItemByKeyboard()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem1 = nullptr;
        xaml_controls::MenuFlyoutItem^ subItem11 = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem1 = nullptr;
        xaml_controls::MenuFlyoutItem^ splitItem11 = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutSubItem x:Name='subItem1' Text='Menu sub item 1'>"
                L"        <MenuFlyoutItem x:Name='subItem1.1' >Menu item 1.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem x:Name='subItem1.2' >Menu item 1.2</MenuFlyoutItem>"
                L"        <MenuFlyoutItem x:Name='subItem1.3' >Menu item 1.3</MenuFlyoutItem>"
                L"    </MenuFlyoutSubItem>"
                L"    <MenuFlyoutItem>item2</MenuFlyoutItem>"
                L"    <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split menu item 1'>"
                L"        <MenuFlyoutItem x:Name='splitItem1.1' >Split item 1.1</MenuFlyoutItem>"
                L"        <MenuFlyoutItem x:Name='splitItem1.2' >Split item 1.2</MenuFlyoutItem>"
                L"        <MenuFlyoutItem x:Name='splitItem1.3' >Split item 1.3</MenuFlyoutItem>"
                L"    </SplitMenuFlyoutItem>"
                L"    <MenuFlyoutItem>item3</MenuFlyoutItem>"
                L"</MenuFlyout>"));

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
        });

        ShowMenuFlyout(menuFlyout, button1, -50, 50);

        // Open the sub menu by using the Keyboard Right key
        CommonInputHelper::Right(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            subItem11 = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"subItem1.1"));
        });

        auto subItem11GotFocusEvent = std::make_shared<Event>();
        auto subItem11gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);
        subItem11gotFocusRegistration.Attach(subItem11, [&](){ subItem11GotFocusEvent->Set(); });

        // Close the sub menu by using the Keyboard Left key
        CommonInputHelper::Left(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        // Re-open the sub menu to verify the focused first item
        CommonInputHelper::Right(InputDevice::Keyboard);
        subItem11GotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Close the sub menu
        CommonInputHelper::Left(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            subItem1 = safe_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyoutPresenter->FindName(L"subItem1"));
        });

        // Re-open the sub menu to verify the focused first item
        CommonInputHelper::Right(InputDevice::Keyboard);
        subItem11GotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto subItem1GotFocusEvent = std::make_shared<Event>();
        auto subItem1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutSubItem, GotFocus);
        subItem1GotFocusRegistration.Attach(subItem1, [&]() { subItem1GotFocusEvent->Set(); });

        // Close the sub menu
        TestServices::KeyboardHelper->Escape();
        TestServices::WindowHelper->WaitForIdle();
        subItem1GotFocusEvent->WaitForDefault();

        // Now test SplitMenuFlyoutItem focus behavior
        LOG_OUTPUT(L"Testing SplitMenuFlyoutItem submenu focus behavior.");
        
        // Navigate down to the split menu item
        CommonInputHelper::Down(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Down(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Down(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        // Open the split menu item's sub menu by using the Keyboard Right key
        CommonInputHelper::Right(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            splitItem11 = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"splitItem1.1"));
        });

        auto splitItem11GotFocusEvent = std::make_shared<Event>();
        auto splitItem11gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);
        splitItem11gotFocusRegistration.Attach(splitItem11, [&](){ splitItem11GotFocusEvent->Set(); });

        // Close the split menu's sub menu by using the Keyboard Left key
        CommonInputHelper::Left(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        // Re-open the split menu's sub menu to verify the focused first item
        CommonInputHelper::Right(InputDevice::Keyboard);
        splitItem11GotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Close the split menu's sub menu
        CommonInputHelper::Left(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto menuFlyoutPresenter = GetCurrentPresenter();
            splitItem1 = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(menuFlyoutPresenter->FindName(L"splitItem1"));
        });

        // Re-open the split menu's sub menu to verify the focused first item
        CommonInputHelper::Right(InputDevice::Keyboard);
        splitItem11GotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto splitItem1GotFocusEvent = std::make_shared<Event>();
        auto splitItem1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::SplitMenuFlyoutItem, GotFocus);
        splitItem1GotFocusRegistration.Attach(splitItem1, [&]() { splitItem1GotFocusEvent->Set(); });

        // Close the split menu's sub menu
        TestServices::KeyboardHelper->Escape();
        TestServices::WindowHelper->WaitForIdle();
        splitItem1GotFocusEvent->WaitForDefault();

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemInternalKeyboardTraversal()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;
        xaml_controls::Button^ primaryButton = nullptr;
        xaml_controls::Button^ secondaryButton = nullptr;
        wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items = nullptr;

        xaml_controls::Canvas^ rootPanel = SetupRootPanelForSubMenuTest();
        menuFlyout = CreateSplitMenuFlyoutItemsFromXaml();
        VERIFY_IS_NOT_NULL(menuFlyout);

        RunOnUIThread([&]()
        {
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            items = menuFlyout->Items;
            splitItem = safe_cast<xaml_controls::SplitMenuFlyoutItem^>(items->GetAt(6));
            VERIFY_IS_NOT_NULL(splitItem);
        });

        ShowMenuFlyout(menuFlyout, button1, -50, 50);

        // Navigate down to the split item with submenu (index 6)
        // Need to navigate through: Item(0), Item(1), Toggle(3), Split(5)-Primary, Split(5)-Secondary, Split(6)-Primary
        LOG_OUTPUT(L"Navigate to split menu item with submenu.");
        for (int i = 0; i < 5; i++)
        {
            CommonInputHelper::Down(InputDevice::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            primaryButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(splitItem, L"PrimaryButton"));
            secondaryButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(splitItem, L"SecondaryButton"));
            VERIFY_IS_NOT_NULL(primaryButton);
            VERIFY_IS_NOT_NULL(secondaryButton);
        });

        // Test 1: Verify focus is on primary button
        LOG_OUTPUT(L"Test 1: Verify focus is on primary button.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(primaryButton->FocusState != xaml::FocusState::Unfocused);
        });

        // Test 2: Press Right to move focus from primary to secondary
        LOG_OUTPUT(L"Test 2: Press Right to move focus to secondary button.");
        auto secondaryGotFocusEvent = std::make_shared<Event>();
        auto secondaryGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        secondaryGotFocusRegistration.Attach(secondaryButton, [secondaryGotFocusEvent]() { secondaryGotFocusEvent->Set(); });

        CommonInputHelper::Right(InputDevice::Keyboard);
        secondaryGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(secondaryButton->FocusState != xaml::FocusState::Unfocused);
            VERIFY_IS_TRUE(primaryButton->FocusState == xaml::FocusState::Unfocused);
        });

        // Test 3: Press Left to move focus from secondary back to primary
        LOG_OUTPUT(L"Test 3: Press Left to move focus back to primary button.");
        auto primaryGotFocusEvent = std::make_shared<Event>();
        auto primaryGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        primaryGotFocusRegistration.Attach(primaryButton, [primaryGotFocusEvent]() { primaryGotFocusEvent->Set(); });

        CommonInputHelper::Left(InputDevice::Keyboard);
        primaryGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(primaryButton->FocusState != xaml::FocusState::Unfocused);
            VERIFY_IS_TRUE(secondaryButton->FocusState == xaml::FocusState::Unfocused);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateFocusedItemDownAndUpAfterOverrideFocusItem()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::MenuFlyoutItem^ secondMenuItem = nullptr;
        xaml_controls::MenuFlyoutItem^ thirdMenuItem = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootPanel;

            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                LR"(<MenuFlyout x:Name="menuFlyout" xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <MenuFlyoutItem>MenuItem1</MenuFlyoutItem>
                        <MenuFlyoutItem>MenuItem2</MenuFlyoutItem>
                        <MenuFlyoutItem>MenuItem3</MenuFlyoutItem>
                    </MenuFlyout>)"));
            menuFlyout->XamlRoot = rootPanel->XamlRoot;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        openedRegistration.Attach(menuFlyout, [&]()
        {
            auto menuItems = menuFlyout->Items;

            auto firstMenuItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuItems->GetAt(0));
            secondMenuItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuItems->GetAt(1));
            thirdMenuItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuItems->GetAt(2));

            // Override the focus on the first menu item
            firstMenuItem->Focus(FocusState::Keyboard);

            openedEvent->Set();
        });

        RunOnUIThread([&]()
        {
            menuFlyout->ShowAt(nullptr, wf::Point(50, 50));
        });
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);
        gotFocusRegistration.Attach(secondMenuItem, [&]()
        {
            VERIFY_IS_TRUE(secondMenuItem->Text == L"MenuItem2");
            gotFocusEvent->Set();
        });

        CommonInputHelper::Down(InputDevice::Keyboard);
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);

        // Navigate to the last menu item through Keyboard Up event
        openedEvent->Reset();
        RunOnUIThread([&]()
        {
            menuFlyout->ShowAt(nullptr, wf::Point(50, 50));
        });
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto gotFocusEvent2 = std::make_shared<Event>();
        auto gotFocusRegistration2 = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);
        gotFocusRegistration2.Attach(thirdMenuItem, [&]()
        {
            VERIFY_IS_TRUE(thirdMenuItem->Text == L"MenuItem3");
            gotFocusEvent2->Set();
        });

        CommonInputHelper::Up(InputDevice::Keyboard);
        gotFocusEvent2->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateSingleItemGetsInitialKeyboardFocus()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Loaded);

        RunOnUIThread([&]()
        {
            button = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
                LR"(<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    Click for flyout
                    <Button.Flyout>
                        <MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <MenuFlyoutItem>The menu item</MenuFlyoutItem>
                        </MenuFlyout>
                    </Button.Flyout>
                </Button>)"));

            loadedRegistration.Attach(button, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = button;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto openedEvent = std::make_shared<Event>();
        auto gotFocusEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);

        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            menuFlyoutItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyout->Items->GetAt(0));

            openedRegistration.Attach(menuFlyout, [&]() { openedEvent->Set(); });
            gotFocusRegistration.Attach(menuFlyoutItem, [&]() { gotFocusEvent->Set(); });
        });

        ControlHelper::EnsureFocused(button);
        CommonInputHelper::Accept(InputDevice::Keyboard);

        openedEvent->WaitForDefault();
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateMenuItemsShowIcons()
    {
        TestCleanupWrapper cleanup;

        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            [&]()
            {
                xaml_controls::Grid^ rootPanel = nullptr;
                xaml_controls::Button^ button = nullptr;
                xaml_controls::MenuFlyout^ menuFlyout = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                        LR"(<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button x:Name="button1" Content="Button 1"/>
                            </Grid>)"));
                    TestServices::WindowHelper->WindowContent = rootPanel;

                    menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                        LR"(<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <MenuFlyoutItem Icon="Accept" Text="menu item 1"/>
                                <MenuFlyoutItem Text="menu item 2"/>
                                <MenuFlyoutSeparator/>
                                <ToggleMenuFlyoutItem Icon="Play" Text="toggle menu item 1"/>
                                <ToggleMenuFlyoutItem IsChecked="True" Text="toggle menu item 2"/>
                                <MenuFlyoutSeparator/>
                                <SplitMenuFlyoutItem Icon="Home" Text="split menu item 1">
                                    <MenuFlyoutItem Icon="Calendar" Text="split item 1.1"/>
                                    <MenuFlyoutItem Text="split item 1.2"/>
                                </SplitMenuFlyoutItem>
                                <MenuFlyoutSeparator/>
                                <MenuFlyoutSubItem Icon="Like" Text="sub menu item 1">
                                    <MenuFlyoutItem Icon="Setting" Text="menu item 1.1"/>
                                    <MenuFlyoutItem Text="menu item 2.2"/>
                                </MenuFlyoutSubItem>
                            </MenuFlyout>)"));

                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                });
                TestServices::WindowHelper->WaitForIdle();

                ShowMenuFlyout(menuFlyout, button, 50, 50);

                TestServices::WindowHelper->WaitForIdle();

                FlyoutHelper::HideFlyout(menuFlyout);

                return rootPanel;
            });
    }

    void MenuFlyoutIntegrationTests::FocusDoesNotJumpWhenUsingGamepadTriggersFollowedByDPad()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        xaml::DependencyObject^ beforeTriggerFocusedObject = nullptr;
        xaml::DependencyObject^ afterTriggerFocusedObject = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = ref new xaml_controls::MenuFlyout();
            for (size_t i = 0; i < 50; ++i)
            {
                auto item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = L"Item";

                menuFlyout->Items->Append(item);
            }

            button = ref new xaml_controls::Button();
            button->Content = "Button";

            TestServices::WindowHelper->WindowContent = button;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button, 0, 0, true /* forceTapAsPreviousInputMessage */);

        RunOnUIThread([&]()
        {
            beforeTriggerFocusedObject = safe_cast<xaml::DependencyObject^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            WEX::Common::Throw::IfNull(reinterpret_cast<void*>(beforeTriggerFocusedObject), L"An item should get focus when the MenuFlyout opens.");
        });

        TestServices::KeyboardHelper->GamepadRightTrigger();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            afterTriggerFocusedObject = safe_cast<xaml::DependencyObject^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            WEX::Common::Throw::If(beforeTriggerFocusedObject->Equals(afterTriggerFocusedObject), E_FAIL, L"The RightTrigger should have moved focus.");
        });

        TestServices::KeyboardHelper->GamepadDpadDown();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml::DependencyObject^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            VERIFY_IS_FALSE(afterTriggerFocusedObject->Equals(focusedElement));

            auto presenter = GetMenuFlyoutPresenter(menuFlyout);

            auto expectedFocusItemIndex = presenter->IndexFromContainer(afterTriggerFocusedObject) + 1;
            WEX::Common::Throw::If(expectedFocusItemIndex == -1, E_FAIL, L"The expected focused element should have an item index.");

            auto focusedItemIndex = presenter->IndexFromContainer(focusedElement);
            WEX::Common::Throw::If(focusedItemIndex == -1, E_FAIL, L"The new focused element should have an item index.");

            VERIFY_ARE_EQUAL(expectedFocusItemIndex, focusedItemIndex);

            beforeTriggerFocusedObject = focusedElement;
        });

        TestServices::KeyboardHelper->GamepadLeftTrigger();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            afterTriggerFocusedObject = safe_cast<xaml::DependencyObject^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            WEX::Common::Throw::If(beforeTriggerFocusedObject->Equals(afterTriggerFocusedObject), E_FAIL, L"The LeftTrigger should have moved focus.");
        });

        TestServices::KeyboardHelper->GamepadDpadDown();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml::DependencyObject^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            VERIFY_IS_FALSE(afterTriggerFocusedObject->Equals(focusedElement));

            auto presenter = GetMenuFlyoutPresenter(menuFlyout);

            auto expectedFocusItemIndex = presenter->IndexFromContainer(afterTriggerFocusedObject) + 1;
            WEX::Common::Throw::If(expectedFocusItemIndex == -1, E_FAIL, L"The expected focused element should have an item index.");

            auto focusedItemIndex = presenter->IndexFromContainer(focusedElement);
            WEX::Common::Throw::If(focusedItemIndex == -1, E_FAIL, L"The new focused element should have an item index.");

            VERIFY_ARE_EQUAL(expectedFocusItemIndex, focusedItemIndex);

            beforeTriggerFocusedObject = focusedElement;
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::CanDetectChangesToCanExecuteWithoutBeingInVisualTree()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem = nullptr;
        xaml_controls::Button^ button = nullptr;

        MenuCommand^ command = ref new MenuCommand(ref new ExecuteDelegate([](Platform::Object^) {}), false /*canExecute*/, nullptr);

        RunOnUIThread([&]()
        {
            menuFlyoutItem = ref new xaml_controls::MenuFlyoutItem();
            menuFlyoutItem->Text = L"Item";
            menuFlyoutItem->Command = command;

            menuFlyout = ref new xaml_controls::MenuFlyout();
            menuFlyout->Items->Append(menuFlyoutItem);

            button = ref new xaml_controls::Button();
            button->Flyout = menuFlyout;
            button->Content = "Button";

            TestServices::WindowHelper->WindowContent = button;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, button, 0, 0);

        RunOnUIThread([&]()
        {
            WEX::Common::Throw::IfFalse(ControlHelper::IsInVisualState(menuFlyoutItem, L"CommonStates", L"Disabled"), E_FAIL, L"Expected that the menu flyout item starts out as disabled.");
        });

        FlyoutHelper::HideFlyout(menuFlyout);

        LOG_OUTPUT(L"Change the MenuFlyoutItem's attached command's CanExecute property to true -> item should now show as enabled.");
        command->CanExecuteFlag = true;

        ShowMenuFlyout(menuFlyout, button, 0, 0);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(menuFlyoutItem, L"CommonStates", L"Normal"));
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::VerifyIsEnabledPropagatesTreeFromCommand()
    {
        TestCleanupWrapper cleanup;

        // Regression coverage for:
        // High Contrast: Selected item in aspect ratio doesn't follow High contrast standards after modifying the value.
        // The issue is that MenuFlyoutItem.Command.CanExecute returns false, but this was not propagating down the visual tree to the TextBlock template part.
        // Note: IsEnabled is only publically visible on Control via the public api even though internally it is on all UIElements.
        // So, to test this scenario, we insert a dummy ContentControl into the MenuFlyoutItem's template so we have a way of reading the IsEnabled property
        // on the descendant elements of the MenuFlyoutItem.

        xaml_controls::MenuFlyout^ menuFlyout;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem;
        xaml_controls::SplitMenuFlyoutItem^ splitMenuFlyoutItem;

        MenuCommand^ command = ref new MenuCommand(ref new ExecuteDelegate([](Platform::Object^) {}), false /*canExecute*/, nullptr);
        MenuCommand^ splitCommand = ref new MenuCommand(ref new ExecuteDelegate([](Platform::Object^) {}), false /*canExecute*/, nullptr);

        RunOnUIThread([&]()
        {
            auto rootGrid = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                          Background="SlateBlue">
                        <Grid.Resources>
                            <Style x:Key="MenuFlyoutItemStyle" TargetType="MenuFlyoutItem">
                                <Setter Property="Template">
                                    <Setter.Value>
                                        <ControlTemplate TargetType="MenuFlyoutItem">
                                            <Grid x:Name="LayoutRoot">
                                                <ContentControl x:Name="testContentControl">
                                                    <TextBlock x:Name="TextBlock" />
                                                </ContentControl>
                                            </Grid>
                                        </ControlTemplate>
                                    </Setter.Value>
                                </Setter>
                            </Style>
                            <Style x:Key="SplitMenuFlyoutItemStyle" TargetType="SplitMenuFlyoutItem">
                                <Setter Property="Template">
                                    <Setter.Value>
                                        <ControlTemplate TargetType="SplitMenuFlyoutItem">
                                            <Grid x:Name="LayoutRoot">
                                                <ContentControl x:Name="testSplitContentControl">
                                                    <TextBlock x:Name="TextBlock" />
                                                </ContentControl>
                                            </Grid>
                                        </ControlTemplate>
                                    </Setter.Value>
                                </Setter>
                            </Style>
                        </Grid.Resources>
                        <FlyoutBase.AttachedFlyout>
                            <MenuFlyout x:Name="menuFlyout">
                                <MenuFlyoutItem x:Name="menuFlyoutItem" Style="{StaticResource MenuFlyoutItemStyle}">Some Text</MenuFlyoutItem>
                                <SplitMenuFlyoutItem x:Name="splitMenuFlyoutItem" Style="{StaticResource SplitMenuFlyoutItemStyle}" Text="Split Item" />
                            </MenuFlyout>
                        </FlyoutBase.AttachedFlyout>
                    </Grid>)"));

            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(rootGrid->FindName(L"menuFlyout"));
            menuFlyoutItem = dynamic_cast<xaml_controls::MenuFlyoutItem^>(rootGrid->FindName(L"menuFlyoutItem"));
            splitMenuFlyoutItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootGrid->FindName(L"splitMenuFlyoutItem"));

            menuFlyoutItem->Command = command;
            splitMenuFlyoutItem->Command = splitCommand;

            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        ShowMenuFlyout(menuFlyout, nullptr, 0, 0);

        RunOnUIThread([&]()
        {
            auto testContentControl = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(menuFlyoutItem, L"testContentControl"));
            VERIFY_IS_FALSE(testContentControl->IsEnabled);

            auto testSplitContentControl = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(splitMenuFlyoutItem, L"testSplitContentControl"));
            VERIFY_IS_FALSE(testSplitContentControl->IsEnabled);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    // Shows the same MenuFlyout twice in a row, without closing it, attempting to position it beyond the screen's boundaries.
    // Ensures it is moved within the screen's boundaries.
    void MenuFlyoutIntegrationTests::MenuFlyoutRemainsInBoundsWhenShownTwice()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootGrid = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        wf::Point initialMenuItemPosition{};
        wf::Point showAtPosition{};

        auto loadedEvent = std::make_shared<Event>();
        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            rootGrid = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='root' Background='SlateBlue'> "
                L"  <FlyoutBase.AttachedFlyout> "
                L"    <MenuFlyout> "
                L"      <MenuFlyoutItem>Some Text</MenuFlyoutItem> "
                L"    </MenuFlyout> "
                L"  </FlyoutBase.AttachedFlyout> "
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootGrid);

            loadedRegistration.Attach(rootGrid, [&]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootGrid;

            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_controls::MenuFlyout::GetAttachedFlyout(rootGrid));
            VERIFY_IS_NOT_NULL(menuFlyout);

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([rootGrid, menuFlyout, menuFlyoutOpenedEvent, &initialMenuItemPosition](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"MenuFlyout.Opened event raised");

                xaml_controls::MenuFlyoutItem^ menuItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyout->Items->GetAt(0));
                VERIFY_IS_NOT_NULL(menuItem);

                initialMenuItemPosition = menuItem->TransformToVisual(rootGrid)->TransformPoint(wf::Point(0, 0));
                LOG_OUTPUT(L"Initial MenuFlyoutItem position in MenuFlyout.Opened event handler: %f, %f.", initialMenuItemPosition.X, initialMenuItemPosition.Y);

                menuFlyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"MenuFlyout.Closed event raised");
                menuFlyoutClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for root Grid to load.");
        loadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            showAtPosition.X = static_cast<float>(rootGrid->ActualWidth) - 25.0f;
            showAtPosition.Y = static_cast<float>(rootGrid->ActualHeight) - 25.0f;

            LOG_OUTPUT(L"Showing MenuFlyout at position %f, %f.", showAtPosition.X, showAtPosition.Y);
            menuFlyout->ShowAt(rootGrid, showAtPosition);
        });

        LOG_OUTPUT(L"Waiting for MenuFlyout.Opened event.");
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Showing MenuFlyout at position %f, %f again.", showAtPosition.X, showAtPosition.Y);
            menuFlyout->ShowAt(rootGrid, showAtPosition);
        });

        LOG_OUTPUT(L"Waiting for a few ticks to make sure the MenuFlyout has a chance to reposition itself.");
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_controls::MenuFlyoutItem^ menuItem = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyout->Items->GetAt(0));
            VERIFY_IS_NOT_NULL(menuItem);

            wf::Point finalMenuItemPosition = menuItem->TransformToVisual(rootGrid)->TransformPoint(wf::Point(0, 0));
            LOG_OUTPUT(L"Final MenuFlyoutItem position: %f, %f.", finalMenuItemPosition.X, finalMenuItemPosition.Y);

            LOG_OUTPUT(L"Verifying initial and final MenuFlyoutItem positions are identical.");
            VERIFY_ARE_EQUAL(finalMenuItemPosition.X, initialMenuItemPosition.X);
            VERIFY_ARE_EQUAL(finalMenuItemPosition.Y, initialMenuItemPosition.Y);

            LOG_OUTPUT(L"Verifying MenuFlyoutItem position was shifted to be completely visible.");
            VERIFY_IS_LESS_THAN(finalMenuItemPosition.X, static_cast<float>(rootGrid->ActualWidth - menuItem->ActualWidth));
            VERIFY_IS_LESS_THAN(finalMenuItemPosition.Y, static_cast<float>(rootGrid->ActualHeight - menuItem->ActualHeight));

            LOG_OUTPUT(L"Closing the MenuFlyout.");
            menuFlyout->Hide();
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for MenuFlyout.Closed event.");
        menuFlyoutClosedEvent->WaitForDefault();
    }

    void MenuFlyoutIntegrationTests::ValidateSettingKeyboardAcceleratorCreatesDefaultItemKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto item = ref new xaml_controls::MenuFlyoutItem();

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            item->KeyboardAccelerators->Append(keyboardAccelerator);

            Platform::String^ expectedKeyboardAcceleratorText = ref new Platform::String(L"Ctrl+A");

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", expectedKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text:   \"%s\"", item->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedKeyboardAcceleratorText, item->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSettingKeyboardAcceleratorDoesNotOverrideItemCustomKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto item = ref new xaml_controls::MenuFlyoutItem();

            Platform::String^ customKeyboardAcceleratorText = ref new Platform::String(L"Custom keyboard accelerator text");
            item->KeyboardAcceleratorTextOverride = customKeyboardAcceleratorText;

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            item->KeyboardAccelerators->Append(keyboardAccelerator);

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", customKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text:   \"%s\"", item->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(customKeyboardAcceleratorText, item->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSettingKeyboardAcceleratorCreatesDefaultToggleItemKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto item = ref new xaml_controls::ToggleMenuFlyoutItem();

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            item->KeyboardAccelerators->Append(keyboardAccelerator);

            Platform::String^ expectedKeyboardAcceleratorText = ref new Platform::String(L"Ctrl+A");

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", expectedKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text:   \"%s\"", item->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedKeyboardAcceleratorText, item->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSettingKeyboardAcceleratorDoesNotOverrideToggleItemCustomKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto item = ref new xaml_controls::ToggleMenuFlyoutItem();

            Platform::String^ customKeyboardAcceleratorText = ref new Platform::String(L"Custom keyboard accelerator text");
            item->KeyboardAcceleratorTextOverride = customKeyboardAcceleratorText;

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            item->KeyboardAccelerators->Append(keyboardAccelerator);

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", customKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text:   \"%s\"", item->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(customKeyboardAcceleratorText, item->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSettingKeyboardAcceleratorCreatesDefaultSplitItemKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto item = ref new xaml_controls::SplitMenuFlyoutItem();

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            item->KeyboardAccelerators->Append(keyboardAccelerator);

            Platform::String^ expectedKeyboardAcceleratorText = ref new Platform::String(L"Ctrl+A");

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", expectedKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text:   \"%s\"", item->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedKeyboardAcceleratorText, item->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSettingKeyboardAcceleratorDoesNotOverrideSplitItemCustomKeyboardAcceleratorText()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto item = ref new xaml_controls::SplitMenuFlyoutItem();

            Platform::String^ customKeyboardAcceleratorText = ref new Platform::String(L"Custom keyboard accelerator text");
            item->KeyboardAcceleratorTextOverride = customKeyboardAcceleratorText;

            auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = ::Windows::System::VirtualKey::A;
            keyboardAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            item->KeyboardAccelerators->Append(keyboardAccelerator);

            LOG_OUTPUT(L"Expected keyboard accelerator text: \"%s\"", customKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Actual keyboard accelerator text:   \"%s\"", item->KeyboardAcceleratorTextOverride->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(customKeyboardAcceleratorText, item->KeyboardAcceleratorTextOverride) == 0);
        });
    }

    void MenuFlyoutIntegrationTests::ValidateKeyboardAcceleratorTextHiddenInSplitMenuItem()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootGrid = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]
        {
            rootGrid = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='root' Background='SlateBlue'> "
                L"  <FlyoutBase.AttachedFlyout> "
                L"    <MenuFlyout> "
                L"      <SplitMenuFlyoutItem Icon='AddFriend' Text='Split Item 1'> "
                L"          <SplitMenuFlyoutItem.KeyboardAccelerators> "
                L"              <KeyboardAccelerator Key='F10' Modifiers='Control' /> "
                L"          </SplitMenuFlyoutItem.KeyboardAccelerators> "
                L"          <MenuFlyoutItem Text='Sub Item 1.1'/> "
                L"      </SplitMenuFlyoutItem> "
                L"    </MenuFlyout> "
                L"  </FlyoutBase.AttachedFlyout> "
                L"</Grid>"));

            loadedRegistration.Attach(rootGrid, [&]() { loadedEvent->Set(); });

            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_controls::MenuFlyout::GetAttachedFlyout(rootGrid));
            VERIFY_IS_NOT_NULL(menuFlyout);

            openedRegistration.Attach(menuFlyout, [menuFlyoutOpenedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout.Opened event raised");
                menuFlyoutOpenedEvent->Set();
            });

            closedRegistration.Attach(menuFlyout, [menuFlyoutClosedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout.Closed event raised");
                menuFlyoutClosedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        TestServices::WindowHelper->WaitForIdle();
        loadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            menuFlyout->ShowAt(rootGrid);
        });

        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto keyboardAcceleratorTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(menuFlyout->Items->GetAt(0), L"KeyboardAcceleratorTextBlock"));
            VERIFY_IS_NULL(keyboardAcceleratorTextBlock);

            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }


    void MenuFlyoutIntegrationTests::ValidateDefaultKeyboardAcceleratorTextPopulatesFully()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootGrid = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            rootGrid = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='root' Background='SlateBlue'> "
                L"  <FlyoutBase.AttachedFlyout> "
                L"    <MenuFlyout> "
                L"      <MenuFlyoutItem Icon='AddFriend' Text='MenuFlyoutItemWithIcon'> "
                L"          <MenuFlyoutItem.KeyboardAccelerators> "
                L"              <KeyboardAccelerator Key='F10' Modifiers='Control' /> "
                L"          </MenuFlyoutItem.KeyboardAccelerators> "
                L"      </MenuFlyoutItem> "
                L"      <MenuFlyoutItem Text='MenuFlyoutItem1'> "
                L"          <MenuFlyoutItem.KeyboardAccelerators> "
                L"              <KeyboardAccelerator Key='F1' Modifiers='Control' /> "
                L"          </MenuFlyoutItem.KeyboardAccelerators> "
                L"      </MenuFlyoutItem> "
                L"      <ToggleMenuFlyoutItem Text='ToggleMenuFlyoutItem'> "
                L"          <MenuFlyoutItem.KeyboardAccelerators> "
                L"              <KeyboardAccelerator Key='F11' Modifiers='Control' /> "
                L"          </MenuFlyoutItem.KeyboardAccelerators> "
                L"      </ToggleMenuFlyoutItem> "
                L"      <MenuFlyoutItem Text='MenuFlyoutItem2'> "
                L"          <MenuFlyoutItem.KeyboardAccelerators> "
                L"              <KeyboardAccelerator Key='F2' Modifiers='Control' /> "
                L"          </MenuFlyoutItem.KeyboardAccelerators> "
                L"      </MenuFlyoutItem> "
                L"    </MenuFlyout> "
                L"  </FlyoutBase.AttachedFlyout> "
                L"</Grid>"));

            loadedRegistration.Attach(rootGrid, [&]() { loadedEvent->Set(); });

            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_controls::MenuFlyout::GetAttachedFlyout(rootGrid));
            VERIFY_IS_NOT_NULL(menuFlyout);

            openedRegistration.Attach(menuFlyout, [menuFlyoutOpenedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout.Opened event raised");
                menuFlyoutOpenedEvent->Set();
            });

            closedRegistration.Attach(menuFlyout, [menuFlyoutClosedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout.Closed event raised");
                menuFlyoutClosedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        TestServices::WindowHelper->WaitForIdle();
        loadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            menuFlyout->ShowAt(rootGrid);
        });

        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto fourthKeyboardAcceleratorTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(menuFlyout->Items->GetAt(3), L"KeyboardAcceleratorTextBlock"));
            auto fourthKeyboardAcceleratorText = fourthKeyboardAcceleratorTextBlock->Text;
            LOG_OUTPUT(L"Fourth keyboard accelerator text: %s", fourthKeyboardAcceleratorText->Data());
            LOG_OUTPUT(L"Expected: Ctrl+F2");
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(L"Ctrl+F2", fourthKeyboardAcceleratorText) == 0);
            
            menuFlyout->Hide();
        });

        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::ValidateSettingUICommandSetsProperties()
    {
        CommandHelper::ValidateSettingUICommandSetsProperties<xaml_controls::MenuFlyoutItem>(
            xaml_controls::MenuFlyoutItem::CommandProperty,
            xaml_controls::MenuFlyoutItem::TextProperty,
            xaml_controls::MenuFlyoutItem::IconProperty);
    }

    void MenuFlyoutIntegrationTests::ValidateSettingUICommandDoesNotOverwriteProperties()
    {
        CommandHelper::ValidateSettingUICommandDoesNotOverwriteProperties<xaml_controls::MenuFlyoutItem>(
            xaml_controls::MenuFlyoutItem::CommandProperty,
            xaml_controls::MenuFlyoutItem::TextProperty,
            xaml_controls::MenuFlyoutItem::IconProperty);
    }

    void MenuFlyoutIntegrationTests::ValidateSettingUICommandSetsToggleProperties()
    {
        CommandHelper::ValidateSettingUICommandSetsProperties<xaml_controls::ToggleMenuFlyoutItem>(
            xaml_controls::ToggleMenuFlyoutItem::CommandProperty,
            xaml_controls::ToggleMenuFlyoutItem::TextProperty,
            xaml_controls::ToggleMenuFlyoutItem::IconProperty);
    }

    void MenuFlyoutIntegrationTests::ValidateSettingUICommandDoesNotOverwriteToggleProperties()
    {
        CommandHelper::ValidateSettingUICommandDoesNotOverwriteProperties<xaml_controls::ToggleMenuFlyoutItem>(
            xaml_controls::ToggleMenuFlyoutItem::CommandProperty,
            xaml_controls::ToggleMenuFlyoutItem::TextProperty,
            xaml_controls::ToggleMenuFlyoutItem::IconProperty);
    }

    void MenuFlyoutIntegrationTests::ValidateSettingUICommandSetsSplitProperties()
    {
        CommandHelper::ValidateSettingUICommandSetsProperties<xaml_controls::SplitMenuFlyoutItem>(
            xaml_controls::SplitMenuFlyoutItem::CommandProperty,
            xaml_controls::SplitMenuFlyoutItem::TextProperty,
            xaml_controls::SplitMenuFlyoutItem::IconProperty);
    }

    void MenuFlyoutIntegrationTests::ValidateSettingUICommandDoesNotOverwriteSplitProperties()
    {
        CommandHelper::ValidateSettingUICommandDoesNotOverwriteProperties<xaml_controls::SplitMenuFlyoutItem>(
            xaml_controls::SplitMenuFlyoutItem::CommandProperty,
            xaml_controls::SplitMenuFlyoutItem::TextProperty,
            xaml_controls::SplitMenuFlyoutItem::IconProperty);
    }

    void MenuFlyoutIntegrationTests::ValidateMenuFlyoutTouchPositionForExclusionRect()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::Grid^ root = nullptr;
        xaml_controls::Button^ buttonTop = nullptr;
        xaml_controls::Button^ buttonBottom = nullptr;

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                          x:Name='root'>
                        <Button x:Name='buttonTop' Content='button top' VerticalAlignment='Top'/>
                        <Button x:Name='buttonBottom' Content='button bottom'  VerticalAlignment='Bottom'/>
                    </Grid>)"));

            TestServices::WindowHelper->WindowContent = root;

            buttonTop = safe_cast<xaml_controls::Button^>(root->FindName(L"buttonTop"));
            buttonBottom = safe_cast<xaml_controls::Button^>(root->FindName(L"buttonBottom"));
        });
        TestServices::WindowHelper->WaitForIdle();

        xaml::XamlRoot^ xamlRoot = nullptr;
        RunOnUIThread([&]()
        {
            xamlRoot = root->XamlRoot;
        });

        LOG_OUTPUT(L"Setting last input method to Touch");
        InjectInput(InputMethod::Touch);

        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        RunOnUIThread([&]()
        {
            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
                L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <MenuFlyoutItem>Menu item 1</MenuFlyoutItem>"
                L"    <MenuFlyoutItem>Menu item 2</MenuFlyoutItem>"
                L"</MenuFlyout>"));
        });

        xaml_primitives::FlyoutShowOptions^ showOptions = nullptr;

        RunOnUIThread([&]()
        {
            showOptions = ref new xaml_primitives::FlyoutShowOptions();
            showOptions->Position = wf::Point(0, (float)buttonBottom->ActualHeight);
            showOptions->ExclusionRect = wf::Rect(0, 0, (float)buttonBottom->ActualWidth, (float)buttonBottom->ActualHeight);

            LOG_OUTPUT(L"Showing flyout on bottom button");
            menuFlyout->ShowAt(buttonBottom, showOptions);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify MenuFlyout is above the bottom button.
            auto menuFlyoutPresenter = GetCurrentPresenter();
            auto menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(menuFlyoutPresenter));
            auto buttonBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(buttonBottom));

            VERIFY_ARE_EQUAL(menuFlyoutBounds.Bottom, buttonBounds.Top);
        });

        FlyoutHelper::HideFlyout(menuFlyout);

        RunOnUIThread([&]()
        {
            showOptions = ref new xaml_primitives::FlyoutShowOptions();
            showOptions->Position = wf::Point(0, (float)buttonTop->ActualHeight);
            showOptions->ExclusionRect = wf::Rect(0, 0, (float)buttonTop->ActualWidth, (float)buttonTop->ActualHeight);

            LOG_OUTPUT(L"Showing flyout on top button");
            menuFlyout->ShowAt(buttonTop, showOptions);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify MenuFlyout is below the top button.
            auto menuFlyoutPresenter = GetCurrentPresenter();
            auto menuFlyoutBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(menuFlyoutPresenter));
            auto buttonBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(buttonTop));

            VERIFY_ARE_EQUAL(menuFlyoutBounds.Top, buttonBounds.Bottom);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::ValidateShowAtMonitorEdge()
    {
        ValidateShowAtMonitorEdge(1.0f  /*scaleFactor*/);
        ValidateShowAtMonitorEdge(1.25f /*scaleFactor*/);
        ValidateShowAtMonitorEdge(1.5f  /*scaleFactor*/);
        ValidateShowAtMonitorEdge(1.75f /*scaleFactor*/);
        ValidateShowAtMonitorEdge(2.25f /*scaleFactor*/);
    }

    void MenuFlyoutIntegrationTests::ValidateShowAtMonitorEdge(float scaleFactor)
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Setting Window Size to 421x421 and global Scale to %f.", scaleFactor);
        ::Windows::Foundation::Size size(421, 421);
        TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, scaleFactor);

        wf::Rect windowBounds;
        wf::Rect monitorBounds;

        RunOnUIThread([&]()
        {
            windowBounds = TestServices::WindowHelper->WindowBounds;
            LOG_OUTPUT(L"WindowBounds: %f,%f,%f,%f.", windowBounds.X, windowBounds.Y, windowBounds.Width, windowBounds.Height);

            monitorBounds = TestServices::WindowHelper->MonitorBounds;
            LOG_OUTPUT(L"MonitorBounds: %f,%f,%f,%f.", monitorBounds.X, monitorBounds.Y, monitorBounds.Width, monitorBounds.Height);
        });

        auto menuFlyout = CreateMenuFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);
        VERIFY_IS_NOT_NULL(menuFlyout);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutOpenedCount = std::make_shared<unsigned int>();
        auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutClosedCount = std::make_shared<unsigned int>();
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        *flyoutOpenedCount = 0;
        *flyoutClosedCount = 0;

        RunOnUIThread([&]()
        {
            flyoutOpenedRegistration.Attach(menuFlyout, [flyoutOpenedEvent, flyoutOpenedCount]()
            {
                LOG_OUTPUT(L"MenuFlyout.Opened event raised.");
                flyoutOpenedEvent->Set();
                (*flyoutOpenedCount)++;
            });

            flyoutClosedRegistration.Attach(menuFlyout, [flyoutClosedEvent, flyoutClosedCount]()
            {
                LOG_OUTPUT(L"MenuFlyout.Closed event raised.");
                flyoutClosedEvent->Set();
                (*flyoutClosedCount)++;
            });

            auto rootPanel = ref new xaml_controls::Grid();

            loadedRegistration.Attach(rootPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"Grid.Loaded event raised.");
                loadedEvent->Set();
            });

            LOG_OUTPUT(L"Setting WindowContent.");
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Waiting for Loaded event...");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            wf::Point targetPointClientLogical = { 1, 1 };
            wf::Rect availableMonitorBounds = {};
            wf::Point screenOffset = {};
            wf::Point targetPointScreenPhysical = {};
            wf::Rect inputPaneOccludeRectScreenLogical = {};

            TestServices::WindowHelper->GetAvailableMonitorBounds(
                TestServices::WindowHelper->WindowContent,
                targetPointClientLogical,
                &availableMonitorBounds,
                &screenOffset,
                &targetPointScreenPhysical,
                &inputPaneOccludeRectScreenLogical);

            LOG_OUTPUT(L"availableMonitorBounds: %f,%f,%f,%f.", availableMonitorBounds.X, availableMonitorBounds.Y, availableMonitorBounds.Width, availableMonitorBounds.Height);
            LOG_OUTPUT(L"screenOffset: %f,%f.", screenOffset.X, screenOffset.Y);
            LOG_OUTPUT(L"targetPointScreenPhysical: %f,%f.", targetPointScreenPhysical.X, targetPointScreenPhysical.Y);
            LOG_OUTPUT(L"inputPaneOccludeRectScreenLogical: %f,%f,%f,%f.", inputPaneOccludeRectScreenLogical.X, inputPaneOccludeRectScreenLogical.Y, inputPaneOccludeRectScreenLogical.Width, inputPaneOccludeRectScreenLogical.Height);

            LOG_OUTPUT(L"Showing MenuFlyout at position %f,%f as a windowed popup.", monitorBounds.Width / scaleFactor, (monitorBounds.Height - 32.0f) / scaleFactor);
            wf::Point showAtPosition = { monitorBounds.Width / scaleFactor, (monitorBounds.Height - 32.0f) / scaleFactor };
            menuFlyout->ShowAt(nullptr, showAtPosition);
        });

        LOG_OUTPUT(L"Waiting for Opened event...");
        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Hiding MenuFlyout.");
        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::VerifyDependencyPropertyDefaultValues()
    {
        TestCleanupWrapper cleanup;

        auto menuFlyout = CreateMenuFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);
        VERIFY_IS_NOT_NULL(menuFlyout);

        auto target = FlyoutHelper::CreateTarget(
            100 /*width*/, 100 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Top);
        VERIFY_IS_NOT_NULL(target);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(menuFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto presenter = GetMenuFlyoutPresenter(menuFlyout);
            VERIFY_IS_NOT_NULL(presenter);
            VERIFY_ARE_EQUAL(true, presenter->IsDefaultShadowEnabled);
        });

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void MenuFlyoutIntegrationTests::VerifyLargeNonWindowedMenuIsPositionedCorrectly()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::MenuFlyout^ flyout = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;
        xaml_controls::Button^ flyoutTarget = nullptr;

        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            flyout = ref new xaml_controls::MenuFlyout();
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Right;
            flyout->ShouldConstrainToRootBounds = true;

            for (int i = 0; i < 10; i++)
            {
                auto item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = "Item";
                flyout->Items->Append(item);
            }

            subItem = ref new xaml_controls::MenuFlyoutSubItem();
            subItem->Text = "Sub item";
            flyout->Items->Append(subItem);

            splitItem = ref new xaml_controls::SplitMenuFlyoutItem();
            splitItem->Text = "Split item";
            flyout->Items->Append(splitItem);

            for (int i = 0; i < 20; i++)
            {
                auto item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = "Item";
                flyout->Items->Append(item);
            }

            for (int i = 0; i < 30; i++)
            {
                auto item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = "Item";
                subItem->Items->Append(item);
            }

            for (int i = 0; i < 30; i++)
            {
                auto item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = "Item";
                splitItem->Items->Append(item);
            }

            openedRegistration.Attach(flyout, [openedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout opened.");
                openedEvent->Set();
            });

            closedRegistration.Attach(flyout, [closedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout closed.");
                closedEvent->Set();
            });

            flyoutTarget = ref new xaml_controls::Button();
            flyoutTarget->Content = ref new Platform::String(L"Click for flyout");
            flyoutTarget->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            flyoutTarget->VerticalAlignment = xaml::VerticalAlignment::Center;
            flyoutTarget->Flyout = flyout;

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(flyoutTarget);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping on the button to show the MenuFlyout.");
        TestServices::InputHelper->Tap(flyoutTarget);

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping on the sub item to show the sub-menu.");
        TapSubMenuItem(subItem);

        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);

            LOG_OUTPUT(L"There should be two popups: one for the MenuFlyout, and one for its sub-menu.");
            VERIFY_ARE_EQUAL(2u, popups->Size);

            auto flyoutPopup = popups->GetAt(1);
            auto flyoutPresenter = safe_cast<xaml::FrameworkElement^>(flyoutPopup->Child);
            auto flyoutBounds = ControlHelper::GetBounds(flyoutPresenter);

            auto flyoutSubMenuPopup = popups->GetAt(0);
            auto flyoutSubMenuPresenter = safe_cast<xaml::FrameworkElement^>(flyoutSubMenuPopup->Child);
            auto flyoutSubMenuBounds = ControlHelper::GetBounds(flyoutSubMenuPresenter);

            auto xamlRootSize = TestServices::WindowHelper->WindowContent->XamlRoot->Size;
            LOG_OUTPUT(L"XAML root size:             width=%.0f height=%.0f", xamlRootSize.Width, xamlRootSize.Height);
            LOG_OUTPUT(L"MenuFlyout bounds:          left=%.0f top=%.0f width=%.0f height=%.0f", flyoutBounds.Left, flyoutBounds.Top, flyoutBounds.Width, flyoutBounds.Height);
            LOG_OUTPUT(L"MenuFlyout sub-menu bounds: left=%.0f top=%.0f width=%.0f height=%.0f", flyoutSubMenuBounds.Left, flyoutSubMenuBounds.Top, flyoutSubMenuBounds.Width, flyoutSubMenuBounds.Height);

            VERIFY_ARE_EQUAL(0.0, flyoutBounds.Top);
            VERIFY_ARE_EQUAL(xamlRootSize.Height, flyoutBounds.Height);
            VERIFY_ARE_EQUAL(0.0, flyoutSubMenuBounds.Top);
            VERIFY_ARE_EQUAL(xamlRootSize.Height, flyoutSubMenuBounds.Height);
        });

        LOG_OUTPUT(L"Tapping on the split item secondary part to show the split item sub-menu.");
        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);

            LOG_OUTPUT(L"There should be two popups: one for the MenuFlyout, and one for the split item's sub-menu.");
            VERIFY_ARE_EQUAL(2u, popups->Size);

            auto flyoutPopup = popups->GetAt(1);
            auto flyoutPresenter = safe_cast<xaml::FrameworkElement^>(flyoutPopup->Child);
            auto flyoutBounds = ControlHelper::GetBounds(flyoutPresenter);

            auto splitItemSubMenuPopup = popups->GetAt(0);
            auto splitItemSubMenuPresenter = safe_cast<xaml::FrameworkElement^>(splitItemSubMenuPopup->Child);
            auto splitItemSubMenuBounds = ControlHelper::GetBounds(splitItemSubMenuPresenter);

            auto xamlRootSize = TestServices::WindowHelper->WindowContent->XamlRoot->Size;
            LOG_OUTPUT(L"XAML root size:                    width=%.0f height=%.0f", xamlRootSize.Width, xamlRootSize.Height);
            LOG_OUTPUT(L"MenuFlyout bounds:                 left=%.0f top=%.0f width=%.0f height=%.0f", flyoutBounds.Left, flyoutBounds.Top, flyoutBounds.Width, flyoutBounds.Height);
            LOG_OUTPUT(L"Split item sub-menu bounds:        left=%.0f top=%.0f width=%.0f height=%.0f", splitItemSubMenuBounds.Left, splitItemSubMenuBounds.Top, splitItemSubMenuBounds.Width, splitItemSubMenuBounds.Height);

            VERIFY_ARE_EQUAL(0.0, flyoutBounds.Top);
            VERIFY_ARE_EQUAL(xamlRootSize.Height, flyoutBounds.Height);
            VERIFY_ARE_EQUAL(0.0, splitItemSubMenuBounds.Top);
            VERIFY_ARE_EQUAL(xamlRootSize.Height, splitItemSubMenuBounds.Height);
        });

        LOG_OUTPUT(L"Tapping on the button again to hide the MenuFlyout.");
        TestServices::InputHelper->Tap(flyoutTarget);

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void MenuFlyoutIntegrationTests::VerifyMenuFlyoutResizesWhenItemsChangeWhileOpen()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::MenuFlyout^ flyout = nullptr;
        xaml_controls::MenuFlyoutSubItem^ subItem = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;
        xaml_controls::Button^ flyoutTarget = nullptr;

        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
            {
                flyout = ref new xaml_controls::MenuFlyout();
                flyout->Placement = xaml_primitives::FlyoutPlacementMode::Right;

                auto item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = "Item 1";
                flyout->Items->Append(item);

                subItem = ref new xaml_controls::MenuFlyoutSubItem();
                subItem->Text = "Sub item 2";
                flyout->Items->Append(subItem);

                item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = "Item 2.1";
                subItem->Items->Append(item);

                splitItem = ref new xaml_controls::SplitMenuFlyoutItem();
                splitItem->Text = "Split item 3";
                flyout->Items->Append(splitItem);

                item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = "Item 3.1";
                splitItem->Items->Append(item);

                openedRegistration.Attach(flyout, [openedEvent]()
                {
                    LOG_OUTPUT(L"MenuFlyout opened.");
                    openedEvent->Set();
                });

                closedRegistration.Attach(flyout, [closedEvent]()
                {
                    LOG_OUTPUT(L"MenuFlyout closed.");
                    closedEvent->Set();
                });

                flyoutTarget = ref new xaml_controls::Button();
                flyoutTarget->Content = ref new Platform::String(L"Click for flyout");
                flyoutTarget->HorizontalAlignment = xaml::HorizontalAlignment::Left;
                flyoutTarget->VerticalAlignment = xaml::VerticalAlignment::Center;
                flyoutTarget->Flyout = flyout;

                auto rootPanel = ref new xaml_controls::Grid();
                rootPanel->Children->Append(flyoutTarget);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Click on the button to show the MenuFlyout.");
        TestServices::InputHelper->LeftMouseClick(flyoutTarget);

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml::FrameworkElement^ presenter = nullptr;
        wf::Rect originalBounds{};

        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);

            LOG_OUTPUT(L"There should be one popup for the MenuFlyout.");
            VERIFY_ARE_EQUAL(1u, popups->Size);

            auto flyoutPopup = popups->GetAt(0);
            presenter = safe_cast<xaml::FrameworkElement^>(flyoutPopup->Child);
            originalBounds = ControlHelper::GetBounds(presenter);

            LOG_OUTPUT(L"Adding another MenuFlyoutItem to the MenuFlyout.");

            auto item = ref new xaml_controls::MenuFlyoutItem();
            item->Text = "Item 3";
            flyout->Items->Append(item);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto newBounds = ControlHelper::GetBounds(presenter);

            VERIFY_IS_GREATER_THAN(newBounds.Height, originalBounds.Height);
        });

        LOG_OUTPUT(L"Move to the sub item to show the sub-menu.");
        OpenSubItemWithMouse(subItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);

            LOG_OUTPUT(L"There should be two popups: one for the MenuFlyout, and one for its sub-menu.");
            VERIFY_ARE_EQUAL(2u, popups->Size);

            auto flyoutSubMenuPopup = popups->GetAt(0);
            presenter = safe_cast<xaml::FrameworkElement^>(flyoutSubMenuPopup->Child);
            originalBounds = ControlHelper::GetBounds(presenter);

            LOG_OUTPUT(L"Adding another MenuFlyoutItem to the MenuFlyoutSubItem.");

            auto item = ref new xaml_controls::MenuFlyoutItem();
            item->Text = "Item 2.2";
            subItem->Items->Append(item);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto newBounds = ControlHelper::GetBounds(presenter);

            VERIFY_IS_GREATER_THAN(newBounds.Height, originalBounds.Height);
        });

        LOG_OUTPUT(L"Close the flyout after testing sub menu item.");
        FlyoutHelper::HideFlyout(flyout);
        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Reopen the flyout to test split menu item.");
        closedEvent->Reset();
        openedEvent->Reset();
        TestServices::InputHelper->LeftMouseClick(flyoutTarget);
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tap the split item secondary part to show the split item sub-menu.");
        TapSplitMenuItemSecondary(splitItem);

        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);

            LOG_OUTPUT(L"There should be two popups: one for the MenuFlyout, and one for the split item's sub-menu.");
            VERIFY_ARE_EQUAL(2u, popups->Size);

            auto flyoutSplitItemSubMenuPopup = popups->GetAt(0);
            presenter = safe_cast<xaml::FrameworkElement^>(flyoutSplitItemSubMenuPopup->Child);
            originalBounds = ControlHelper::GetBounds(presenter);

            LOG_OUTPUT(L"Adding another MenuFlyoutItem to the SplitMenuFlyoutItem.");

            auto item = ref new xaml_controls::MenuFlyoutItem();
            item->Text = "Item 3.2";
            splitItem->Items->Append(item);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto newBounds = ControlHelper::GetBounds(presenter);

            VERIFY_IS_GREATER_THAN(newBounds.Height, originalBounds.Height);
        });

        LOG_OUTPUT(L"Click on the button again to hide the MenuFlyout.");
        TestServices::InputHelper->LeftMouseClick(flyoutTarget);

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    xaml_input::KeyboardAccelerator^ MenuFlyoutIntegrationTests::CreateKeyboardAccelerator(::Windows::System::VirtualKey key, ::Windows::System::VirtualKeyModifiers modifiers)
    {
        auto keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
        keyboardAccelerator->Key = key;
        keyboardAccelerator->Modifiers = modifiers;
        return keyboardAccelerator;
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemSubMenuPresenterStyleFromXaml()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' >"
                L"  <Button x:Name='button' Content='Test Button' VerticalAlignment='Center' HorizontalAlignment='Center' >"
                L"    <Button.Flyout> "
                L"      <MenuFlyout> "
                L"        <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split Item with Custom Style'>"
                L"          <SplitMenuFlyoutItem.SubMenuPresenterStyle>"
                L"            <Style TargetType='MenuFlyoutPresenter'>"
                L"              <Setter Property='Tag' Value='CustomSubMenuStyle' />"
                L"              <Setter Property='Background' Value='LightBlue' />"
                L"            </Style>"
                L"          </SplitMenuFlyoutItem.SubMenuPresenterStyle>"
                L"          <MenuFlyoutItem Text='Sub Item 1' />"
                L"          <MenuFlyoutItem Text='Sub Item 2' />"
                L"          <MenuFlyoutSeparator />"
                L"          <ToggleMenuFlyoutItem Text='Sub Toggle Item' IsChecked='True' />"
                L"        </SplitMenuFlyoutItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootPanel->FindName(L"splitItem1"));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the main MenuFlyout");
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tap the secondary button of SplitMenuFlyoutItem to open submenu");
        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify that the custom SubMenuPresenterStyle is applied");
        auto subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(subPresenter);

            // Verify the custom Tag value from the style
            auto tag = subPresenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"CustomSubMenuStyle"), tag->ToString());

            // Verify the custom Background color
            auto background = subPresenter->Background;
            VERIFY_IS_NOT_NULL(background);
            auto solidColorBrush = dynamic_cast<xaml_media::SolidColorBrush^>(background);
            VERIFY_IS_NOT_NULL(solidColorBrush);
            
            auto lightBlue = Microsoft::UI::Colors::LightBlue;
            VERIFY_ARE_EQUAL(lightBlue.A, solidColorBrush->Color.A);
            VERIFY_ARE_EQUAL(lightBlue.R, solidColorBrush->Color.R);
            VERIFY_ARE_EQUAL(lightBlue.G, solidColorBrush->Color.G);
            VERIFY_ARE_EQUAL(lightBlue.B, solidColorBrush->Color.B);
        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
    }

    void MenuFlyoutIntegrationTests::ValidateCanChangeSplitMenuItemSubMenuPresenterStyleAtRuntime()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' >"
                L"  <Button x:Name='button' Content='Test Button' VerticalAlignment='Center' HorizontalAlignment='Center' >"
                L"    <Button.Flyout> "
                L"      <MenuFlyout> "
                L"        <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split Item Runtime Style'>"
                L"          <SplitMenuFlyoutItem.SubMenuPresenterStyle>"
                L"            <Style TargetType='MenuFlyoutPresenter'>"
                L"              <Setter Property='Tag' Value='InitialSubMenuStyle' />"
                L"              <Setter Property='Background' Value='LightGreen' />"
                L"            </Style>"
                L"          </SplitMenuFlyoutItem.SubMenuPresenterStyle>"
                L"          <MenuFlyoutItem Text='Sub Item 1' />"
                L"          <MenuFlyoutItem Text='Sub Item 2' />"
                L"          <MenuFlyoutSeparator />"
                L"          <ToggleMenuFlyoutItem Text='Sub Toggle Item' IsChecked='True' />"
                L"        </SplitMenuFlyoutItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootPanel->FindName(L"splitItem1"));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 1: Test initial style from XAML");
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        auto subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(subPresenter);

            // Verify initial style
            auto tag = subPresenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"InitialSubMenuStyle"), tag->ToString());
        });

        // Close the menu
        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 2: Change SubMenuPresenterStyle at runtime");
        RunOnUIThread([&]()
        {
            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutPresenter";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto newStyle = ref new xaml::Style(type);
            newStyle->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutPresenter::TagProperty, "RuntimeModifiedStyle"));
            newStyle->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutPresenter::BackgroundProperty, ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::LightCoral)));

            splitItem->SubMenuPresenterStyle = newStyle;
        });

        // Reopen and test the modified style
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(subPresenter);

            // Verify the modified style
            auto tag = subPresenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"RuntimeModifiedStyle"), tag->ToString());

            auto background = subPresenter->Background;
            VERIFY_IS_NOT_NULL(background);
            auto solidColorBrush = dynamic_cast<xaml_media::SolidColorBrush^>(background);
            VERIFY_IS_NOT_NULL(solidColorBrush);
            
            auto lightCoral = Microsoft::UI::Colors::LightCoral;
            VERIFY_ARE_EQUAL(lightCoral.A, solidColorBrush->Color.A);
            VERIFY_ARE_EQUAL(lightCoral.R, solidColorBrush->Color.R);
            VERIFY_ARE_EQUAL(lightCoral.G, solidColorBrush->Color.G);
            VERIFY_ARE_EQUAL(lightCoral.B, solidColorBrush->Color.B);
        });

        // Close the menu
        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 3: Clear SubMenuPresenterStyle (set to null)");
        RunOnUIThread([&]()
        {
            splitItem->SubMenuPresenterStyle = nullptr;
        });

        // Reopen and test that default style is used
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(subPresenter);

            // Verify that the custom Tag is no longer set (should be null for default style)
            auto tag = subPresenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            // For default/cleared style, the Tag should be either null or not set to our custom values
            if (tag != nullptr)
            {
                Platform::String^ tagString = tag->ToString();
                VERIFY_ARE_NOT_EQUAL(ref new Platform::String(L"InitialSubMenuStyle"), tagString);
                VERIFY_ARE_NOT_EQUAL(ref new Platform::String(L"RuntimeModifiedStyle"), tagString);
            }

            // The background should revert to default theme background (not our custom colors)
            auto background = subPresenter->Background;
            if (background != nullptr)
            {
                auto solidColorBrush = dynamic_cast<xaml_media::SolidColorBrush^>(background);
                if (solidColorBrush != nullptr)
                {
                    // Should not be our custom colors anymore
                    auto lightGreen = Microsoft::UI::Colors::LightGreen;
                    auto lightCoral = Microsoft::UI::Colors::LightCoral;
                    VERIFY_IS_FALSE(
                        (lightGreen.A == solidColorBrush->Color.A && lightGreen.R == solidColorBrush->Color.R && 
                         lightGreen.G == solidColorBrush->Color.G && lightGreen.B == solidColorBrush->Color.B) ||
                        (lightCoral.A == solidColorBrush->Color.A && lightCoral.R == solidColorBrush->Color.R && 
                         lightCoral.G == solidColorBrush->Color.G && lightCoral.B == solidColorBrush->Color.B)
                    );
                }
            }
        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemSubMenuPresenterStyleWithItemsWrapGrid()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' >"
                L"  <Button x:Name='button' Content='Test Button' VerticalAlignment='Center' HorizontalAlignment='Center' >"
                L"    <Button.Flyout> "
                L"      <MenuFlyout> "
                L"        <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split Item with WrapGrid Panel'>"
                L"          <SplitMenuFlyoutItem.SubMenuPresenterStyle>"
                L"            <Style TargetType='MenuFlyoutPresenter'>"
                L"              <Setter Property='Tag' Value='WrapGridPanelStyle' />"
                L"              <Setter Property='ItemsPanel'>"
                L"                <Setter.Value>"
                L"                  <ItemsPanelTemplate>"
                L"                    <ItemsWrapGrid Orientation='Horizontal' MaximumRowsOrColumns='2' />"
                L"                  </ItemsPanelTemplate>"
                L"                </Setter.Value>"
                L"              </Setter>"
                L"            </Style>"
                L"          </SplitMenuFlyoutItem.SubMenuPresenterStyle>"
                L"          <MenuFlyoutItem Text='Item 1' />"
                L"          <MenuFlyoutItem Text='Item 2' />"
                L"          <MenuFlyoutItem Text='Item 3' />"
                L"          <MenuFlyoutItem Text='Item 4' />"
                L"          <ToggleMenuFlyoutItem Text='Toggle Item' IsChecked='True' />"
                L"        </SplitMenuFlyoutItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootPanel->FindName(L"splitItem1"));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the main MenuFlyout");
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tap the secondary button of SplitMenuFlyoutItem to open submenu with ItemsWrapGrid");
        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify that the submenu with ItemsWrapGrid panel is displayed correctly");
        auto subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(subPresenter);

            // Verify the custom Tag value from the style
            auto tag = subPresenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"WrapGridPanelStyle"), tag->ToString());

            // Find the actual panel in the visual tree (the ItemsWrapGrid should be a child of ItemsPresenter)
            auto itemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(subPresenter);
            VERIFY_IS_NOT_NULL(itemsPresenter);

            // The actual panel is a child of the ItemsPresenter
            auto panel = TreeHelper::GetVisualChildByType<xaml_controls::ItemsWrapGrid>(itemsPresenter);
            VERIFY_IS_NOT_NULL(panel);

            // Verify the WrapGrid properties are set correctly
            VERIFY_ARE_EQUAL(xaml_controls::Orientation::Horizontal, panel->Orientation);
            VERIFY_ARE_EQUAL(2, panel->MaximumRowsOrColumns);

            // Verify that menu items are actually displayed (should have child elements)
            auto children = panel->Children;
            VERIFY_IS_NOT_NULL(children);
            VERIFY_IS_GREATER_THAN(children->Size, 0u);
            
            LOG_OUTPUT(L"Successfully verified ItemsWrapGrid panel with %d child items", children->Size);
        });

        // Close the menu
        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 2: Clear SubMenuPresenterStyle and verify default ItemsPanel");
        RunOnUIThread([&]()
        {
            splitItem->SubMenuPresenterStyle = nullptr;
        });

        // Reopen and test that default ItemsPanel (StackPanel) is used
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify that the default ItemsPanel (StackPanel) is restored");
        subPresenter = GetCurrentPresenter();
        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(subPresenter);

            // Verify that the custom Tag is no longer set
            auto tag = subPresenter->GetValue(xaml_controls::MenuFlyoutPresenter::TagProperty);
            if (tag != nullptr)
            {
                Platform::String^ tagString = tag->ToString();
                VERIFY_ARE_NOT_EQUAL(ref new Platform::String(L"WrapGridPanelStyle"), tagString);
            }

            // Find the actual panel in the visual tree - should now be the default StackPanel
            auto itemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(subPresenter);
            VERIFY_IS_NOT_NULL(itemsPresenter);

            // The default panel should be a StackPanel, not an ItemsWrapGrid
            auto wrapGridPanel = TreeHelper::GetVisualChildByType<xaml_controls::ItemsWrapGrid>(itemsPresenter);
            VERIFY_IS_NULL(wrapGridPanel); // Should not find ItemsWrapGrid anymore

            auto stackPanel = TreeHelper::GetVisualChildByType<xaml_controls::StackPanel>(itemsPresenter);
            VERIFY_IS_NOT_NULL(stackPanel); // Should find the default StackPanel

            // Verify the StackPanel has the default Vertical orientation
            VERIFY_ARE_EQUAL(xaml_controls::Orientation::Vertical, stackPanel->Orientation);

            // Verify that menu items are still displayed (should have child elements)
            auto children = stackPanel->Children;
            VERIFY_IS_NOT_NULL(children);
            VERIFY_IS_GREATER_THAN(children->Size, 0u);
            
            LOG_OUTPUT(L"Successfully verified default StackPanel with %d child items", children->Size);
        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
    }

    xaml_controls::Grid^ MenuFlyoutIntegrationTests::SetupRootPanelForSubMenuItemStyleTests()
    {
        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' >"
                L"  <Grid.Resources>"
                L"    <Style x:Key='TestMenuItemStyle' TargetType='MenuFlyoutItem'>"
                L"      <Setter Property='Tag' Value='TestStyleTag' />"
                L"      <Setter Property='Foreground' Value='LightBlue' />"
                L"    </Style>"
                L"  </Grid.Resources>"
                L"  <Button x:Name='button' Content='Test Button' VerticalAlignment='Center' HorizontalAlignment='Center' >"
                L"    <Button.Flyout> "
                L"      <MenuFlyout> "
                L"        <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split Item with SubMenuItemStyle'>"
                L"          <SplitMenuFlyoutItem.SubMenuItemStyle>"
                L"            <Style TargetType='MenuFlyoutItem'>"
                L"              <Setter Property='Tag' Value='SubMenuItemStyleTag' />"
                L"              <Setter Property='Foreground' Value='LightGreen' />"
                L"            </Style>"
                L"          </SplitMenuFlyoutItem.SubMenuItemStyle>"
                L"          <MenuFlyoutItem Text='Sub Item 1' />"
                L"          <MenuFlyoutItem Text='Sub Item 2' />"
                L"          <MenuFlyoutItem Text='Sub Item 3' />"
                L"        </SplitMenuFlyoutItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return rootPanel;
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemSubMenuItemStyleFromXaml()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        xaml_controls::Grid^ rootPanel = SetupRootPanelForSubMenuItemStyleTests();

        RunOnUIThread([&]()
        {
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootPanel->FindName(L"splitItem1"));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([=](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the main MenuFlyout");
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tap the secondary button of SplitMenuFlyoutItem to open submenu");
        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify that SubMenuItemStyle is applied to all sub menu items");
        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            VERIFY_ARE_EQUAL(3u, items->Size);

            for (unsigned int i = 0; i < items->Size; ++i)
            {
                auto item = items->GetAt(i);
                auto menuItem = dynamic_cast<xaml_controls::MenuFlyoutItem^>(item);
                VERIFY_IS_NOT_NULL(menuItem);

                // Verify the Tag is set from the SubMenuItemStyle
                auto tag = menuItem->GetValue(xaml_controls::MenuFlyoutItem::TagProperty);
                VERIFY_IS_NOT_NULL(tag);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"SubMenuItemStyleTag"), tag->ToString());

                // Verify the Foreground color is set from the SubMenuItemStyle  
                auto foreground = menuItem->Foreground;
                VERIFY_IS_NOT_NULL(foreground);
                auto solidColorBrush = dynamic_cast<xaml_media::SolidColorBrush^>(foreground);
                VERIFY_IS_NOT_NULL(solidColorBrush);
                
                auto lightGreen = Microsoft::UI::Colors::LightGreen;
                VERIFY_ARE_EQUAL(lightGreen.A, solidColorBrush->Color.A);
                VERIFY_ARE_EQUAL(lightGreen.R, solidColorBrush->Color.R);
                VERIFY_ARE_EQUAL(lightGreen.G, solidColorBrush->Color.G);
                VERIFY_ARE_EQUAL(lightGreen.B, solidColorBrush->Color.B);
            }
        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemSubMenuItemStyleWithLocalStyles()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' >"
                L"  <Button x:Name='button' Content='Test Button' VerticalAlignment='Center' HorizontalAlignment='Center' >"
                L"    <Button.Flyout> "
                L"      <MenuFlyout> "
                L"        <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split Item with Mixed Styles'>"
                L"          <SplitMenuFlyoutItem.SubMenuItemStyle>"
                L"            <Style TargetType='MenuFlyoutItem'>"
                L"              <Setter Property='Tag' Value='SubMenuItemStyleTag' />"
                L"              <Setter Property='Foreground' Value='LightGreen' />"
                L"            </Style>"
                L"          </SplitMenuFlyoutItem.SubMenuItemStyle>"
                L"          <MenuFlyoutItem Text='Sub Item 1' />"
                L"          <MenuFlyoutItem Text='Sub Item 2' />"
                L"          <MenuFlyoutItem Text='Sub Item 3' >"
                L"            <MenuFlyoutItem.Style>"
                L"              <Style TargetType='MenuFlyoutItem'>"
                L"                <Setter Property='Tag' Value='LocalStyleTag' />"
                L"                <Setter Property='Foreground' Value='LightCoral' />"
                L"              </Style>"
                L"            </MenuFlyoutItem.Style>"
                L"          </MenuFlyoutItem>"
                L"        </SplitMenuFlyoutItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootPanel->FindName(L"splitItem1"));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the main MenuFlyout");
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tap the secondary button of SplitMenuFlyoutItem to open submenu");
        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify mixed styles: SubMenuItemStyle applied where no local style, local style preserved where set");
        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            VERIFY_ARE_EQUAL(3u, items->Size);

            for (unsigned int i = 0; i < items->Size; ++i)
            {
                auto menuItem = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(i));
                VERIFY_IS_NOT_NULL(menuItem);  
                auto tagString = menuItem->GetValue(xaml_controls::MenuFlyoutItem::TagProperty);
                VERIFY_IS_NOT_NULL(tagString);
                VERIFY_ARE_EQUAL(ref new Platform::String(i<2 ? L"SubMenuItemStyleTag" : L"LocalStyleTag"), tagString->ToString());
            }
        });

        RunOnUIThread([&]()
        {
            menuFlyout->Hide();
        });
    }

    void MenuFlyoutIntegrationTests::ValidateCanChangeSplitMenuItemSubMenuItemStyle()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' >"
                L"  <Button x:Name='button' Content='Test Button' VerticalAlignment='Center' HorizontalAlignment='Center' >"
                L"    <Button.Flyout> "
                L"      <MenuFlyout> "
                L"        <SplitMenuFlyoutItem x:Name='splitItem1' Text='Split Item Runtime Style Changes'>"
                L"          <MenuFlyoutItem Text='Sub Item 1' />"
                L"          <MenuFlyoutItem Text='Sub Item 2' />"
                L"        </SplitMenuFlyoutItem>"
                L"      </MenuFlyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootPanel->FindName(L"splitItem1"));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 1: Apply SubMenuItemStyle at runtime");
        RunOnUIThread([&]()
        {
            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutItem";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto initialStyle = ref new xaml::Style(type);
            initialStyle->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutItem::TagProperty, "InitialStyleTag"));
            initialStyle->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutItem::ForegroundProperty, ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::LightCoral)));

            splitItem->SubMenuItemStyle = initialStyle;
        });

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            for (unsigned int i = 0; i < items->Size; ++i)
            {
                auto item = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(i));
                auto tag = item->GetValue(xaml_controls::MenuFlyoutItem::TagProperty);
                VERIFY_IS_NOT_NULL(tag);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"InitialStyleTag"), tag->ToString());
            }
            menuFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 2: Change SubMenuItemStyle to different style");
        RunOnUIThread([&]()
        {
            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutItem";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto modifiedStyle = ref new xaml::Style(type);
            modifiedStyle->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutItem::TagProperty, "ModifiedStyleTag"));
            modifiedStyle->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutItem::ForegroundProperty, ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::LightBlue)));

            splitItem->SubMenuItemStyle = modifiedStyle;
        });

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            for (unsigned int i = 0; i < items->Size; ++i)
            {
                auto item = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(i));
                auto tag = item->GetValue(xaml_controls::MenuFlyoutItem::TagProperty);
                VERIFY_IS_NOT_NULL(tag);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"ModifiedStyleTag"), tag->ToString());
            }
            menuFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 3: Clear SubMenuItemStyle");
        RunOnUIThread([&]()
        {
            splitItem->SubMenuItemStyle = nullptr;
        });

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            for (unsigned int i = 0; i < items->Size; ++i)
            {
                auto item = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(i));
                auto tag = item->GetValue(xaml_controls::MenuFlyoutItem::TagProperty);
                // Tag should be null or not our custom values after clearing style
                if (tag != nullptr)
                {
                    Platform::String^ tagString = tag->ToString();
                    VERIFY_ARE_NOT_EQUAL(ref new Platform::String(L"InitialStyleTag"), tagString);
                    VERIFY_ARE_NOT_EQUAL(ref new Platform::String(L"ModifiedStyleTag"), tagString);
                }
            }
            menuFlyout->Hide();
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemSubMenuItemStyleWithDynamicItems()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        xaml_controls::Grid^ rootPanel = SetupRootPanelForSubMenuItemStyleTests();

        RunOnUIThread([&]()
        {            
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootPanel->FindName(L"splitItem1"));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([=](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 1: Add new item without local style - should get SubMenuItemStyle");
        RunOnUIThread([&]()
        {
            auto newItem = ref new xaml_controls::MenuFlyoutItem();
            newItem->Text = "Added Item Without Style";
            splitItem->Items->Append(newItem);
        });

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            VERIFY_ARE_EQUAL(4u, items->Size);
            
            // All items should have the SubMenuItemStyle
            for (unsigned int i = 0; i < items->Size; ++i)
            {
                auto item = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(i));
                auto tag = item->GetValue(xaml_controls::MenuFlyoutItem::TagProperty);
                VERIFY_IS_NOT_NULL(tag);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"SubMenuItemStyleTag"), tag->ToString());
            }
            menuFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 2: Add new item with local style - should preserve local style");
        RunOnUIThread([&]()
        {
            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.MenuFlyoutItem";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto localStyle = ref new xaml::Style(type);
            localStyle->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutItem::TagProperty, "LocalStyleTag"));
            localStyle->Setters->Append(ref new xaml::Setter(xaml_controls::MenuFlyoutItem::ForegroundProperty, ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red)));

            auto newItemWithStyle = ref new xaml_controls::MenuFlyoutItem();
            newItemWithStyle->Text = "Added Item With Local Style";
            newItemWithStyle->Style = localStyle;
            splitItem->Items->Append(newItemWithStyle);
        });

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            VERIFY_ARE_EQUAL(5u, items->Size);
            
            // First four items should have SubMenuItemStyle
            for (unsigned int i = 0; i < 4; ++i)
            {
                auto item = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(i));
                auto tag = item->GetValue(xaml_controls::MenuFlyoutItem::TagProperty);
                VERIFY_IS_NOT_NULL(tag);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"SubMenuItemStyleTag"), tag->ToString());
            }
            
            // Third item should preserve its local style
            auto item5 = dynamic_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(4));
            auto tag5 = item5->GetValue(xaml_controls::MenuFlyoutItem::TagProperty);
            VERIFY_IS_NOT_NULL(tag5);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"LocalStyleTag"), tag5->ToString());
            
            menuFlyout->Hide();
        });
    }

    void MenuFlyoutIntegrationTests::ValidateSplitMenuItemSubMenuItemStyleTypeCompatibility()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::Button^ button = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::SplitMenuFlyoutItem^ splitItem = nullptr;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

        xaml_controls::Grid^ rootPanel = SetupRootPanelForSubMenuItemStyleTests();

        RunOnUIThread([&]()
        {
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(button->Flyout);
            splitItem = dynamic_cast<xaml_controls::SplitMenuFlyoutItem^>(rootPanel->FindName(L"splitItem1"));

            openedRegistration.Attach(menuFlyout, ref new wf::EventHandler<Platform::Object^>([menuFlyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                menuFlyoutOpenedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 1: Verify initial setup - SubMenuItemStyle applies to normal MenuFlyoutItem");
        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            VERIFY_ARE_EQUAL(3u, items->Size);
            
            menuFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Step 2: Add ToggleMenuFlyoutItem with local style - should work correctly");
        RunOnUIThread([&]()
        {
            wxaml_interop::TypeName toggleType = wxaml_interop::TypeName();
            toggleType.Name = "Microsoft.UI.Xaml.Controls.ToggleMenuFlyoutItem";
            toggleType.Kind = wxaml_interop::TypeKind::Metadata;

            auto toggleLocalStyle = ref new xaml::Style(toggleType);
            toggleLocalStyle->Setters->Append(ref new xaml::Setter(xaml_controls::ToggleMenuFlyoutItem::TagProperty, "ToggleLocalStyleTag"));
            
            auto toggleItemWithStyle = ref new xaml_controls::ToggleMenuFlyoutItem();
            toggleItemWithStyle->Text = "Toggle Item With Local Style";
            toggleItemWithStyle->Style = toggleLocalStyle;
            splitItem->Items->Append(toggleItemWithStyle);
        });

        TestServices::InputHelper->Tap(button);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TapSplitMenuItemSecondary(splitItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto items = splitItem->Items;
            VERIFY_ARE_EQUAL(4u, items->Size);
            
            // Second item should have its local style preserved
            auto toggleItem = dynamic_cast<xaml_controls::ToggleMenuFlyoutItem^>(items->GetAt(3));
            VERIFY_IS_NOT_NULL(toggleItem);
            auto tag = toggleItem->GetValue(xaml_controls::ToggleMenuFlyoutItem::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"ToggleLocalStyleTag"), tag->ToString());
            
            menuFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::MenuFlyout
