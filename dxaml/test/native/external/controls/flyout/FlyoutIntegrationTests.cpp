// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FlyoutIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

#include <TreeHelper.h>
#include <FlyoutHelper.h>
#include <ControlHelper.h>
#include <ComboBoxHelper.h>
#include <CommonInputHelper.h>
#include <DateTimePickerHelper.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Flyout {

    bool FlyoutIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FlyoutIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool FlyoutIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void SetupBasicFlyoutTest(xaml_controls::Flyout^& flyout, xaml_controls::Button^& button);

    //
    // Test Cases
    //
    void FlyoutIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::Flyout>::CanInstantiate();
    }

    void FlyoutIntegrationTests::CanFlyoutOpenCloseProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanFlyoutOpenClose();
    }

    void FlyoutIntegrationTests::CanFlyoutOpenCloseDropShadow()
    {
        CanFlyoutOpenClose();
    }

    void FlyoutIntegrationTests::CanFlyoutOpenClose()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        const auto& u = TestServices::Utilities;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='button' Content='button.flyout' HorizontalAlignment='Center' VerticalAlignment='Center' FontSize='25' > "
                L"    <Button.Flyout> "
                L"      <Flyout Placement='TopEdgeAlignedLeft'> "
                L"        <TextBlock FontSize='30' Text='BUTTON.FLYOUT' Margin='30' Foreground='DarkRed' x:Name='button_flyout_content'/> "
                L"      </Flyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            flyout = dynamic_cast<xaml_controls::Flyout^>(button->Flyout);
            VERIFY_IS_NOT_NULL(flyout);

            openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"PopupOpenClose: Flyout Opened event is fired!");
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"PopupOpenClose: Flyout Closed event is fired!");
                flyoutClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Button Tap operation to show the Flyout.");
        TestServices::InputHelper->Tap(button);
        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"RootPanel Tap operation to close the Flyout.");
            flyout->Hide();
        });

        flyoutClosedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Button Tap operation to show the Flyout.");
        TestServices::InputHelper->Tap(button);
        flyoutOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Keyboard Escape to close the flyout.");
        TestServices::KeyboardHelper->Escape();

        flyoutClosedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Button Tap operation to show the Flyout.");
        TestServices::InputHelper->Tap(button);
        flyoutOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Gamepad B to close the flyout.");
        TestServices::KeyboardHelper->GamepadB();

        flyoutClosedEvent->WaitForDefault();
    }

    void FlyoutIntegrationTests::VerifyDependencyProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Border();

            auto textBlock = ref new xaml_controls::TextBlock();
            textBlock->Text = L"Flyout";

            auto flyout = ref new xaml_controls::Flyout();
            VERIFY_IS_NOT_NULL(flyout);
            flyout->Content = textBlock;

            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;

            rootPanel->SetValue(xaml_controls::Flyout::AttachedFlyoutProperty, flyout);
            auto attachedFlyout = rootPanel->GetValue(xaml_controls::Flyout::AttachedFlyoutProperty);
            VERIFY_ARE_EQUAL(flyout, attachedFlyout);

            flyout->SetValue(xaml_controls::Flyout::PlacementProperty, xaml_primitives::FlyoutPlacementMode::Bottom);
            auto placementMode = safe_cast<xaml_primitives::FlyoutPlacementMode>(flyout->GetValue(xaml_controls::Flyout::PlacementProperty));
            VERIFY_ARE_EQUAL(placementMode, xaml_primitives::FlyoutPlacementMode::Bottom);

            auto button = ref new xaml_controls::Button();
            button->Content = L"I'm a button on Flyout!";
            flyout->SetValue(xaml_controls::Flyout::ContentProperty, button);
            auto content = flyout->GetValue(xaml_controls::Flyout::ContentProperty);
            VERIFY_ARE_EQUAL(button, content);

            auto styleLocal = ref new xaml::Style();
            flyout->SetValue(xaml_controls::Flyout::FlyoutPresenterStyleProperty, styleLocal);
            auto style = flyout->GetValue(xaml_controls::Flyout::FlyoutPresenterStyleProperty);
            VERIFY_ARE_EQUAL(style, styleLocal);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FlyoutIntegrationTests::VerifyDependencyPropertyDefaultValues()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        SetupBasicFlyoutTest(flyout, button);
        FlyoutHelper::OpenFlyout(flyout, button, FlyoutOpenMethod::Programmatic_ShowAt);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            VERIFY_ARE_EQUAL(true, presenter->IsDefaultShadowEnabled);
        });
    }

    void FlyoutIntegrationTests::VerifyFlyoutPresenterStyle()
    {
        TestCleanupWrapper cleanup;

        auto flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);
        VERIFY_IS_NOT_NULL(flyout);

        auto target = FlyoutHelper::CreateTarget(
            300 /*width*/, 300 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Center);
        VERIFY_IS_NOT_NULL(target);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"VerifyFlyoutPresenterStyle: Execute the ShowAt.");
        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            VERIFY_IS_NOT_NULL(presenter);
            auto tag = presenter->GetValue(xaml_controls::FlyoutPresenter::TagProperty);
            VERIFY_IS_NOT_NULL(tag);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"presenter_style"), tag->ToString());
        });

        LOG_OUTPUT(L"VerifyFlyoutPresenterStyle: Execute the Hide.");
        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::CanAttachedFlyoutShowHide()
    {
        TestCleanupWrapper cleanup;

        auto flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Right);
        VERIFY_IS_NOT_NULL(flyout);

        auto target = FlyoutHelper::CreateTarget(
            300 /*width*/, 300 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Left,
            xaml::VerticalAlignment::Center);
        VERIFY_IS_NOT_NULL(target);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;

            auto attachedFlyout = xaml_controls::Flyout::GetAttachedFlyout(target);
            VERIFY_IS_NULL(attachedFlyout);

            xaml_controls::Flyout::SetAttachedFlyout(target, flyout);

            attachedFlyout = xaml_controls::Flyout::GetAttachedFlyout(target);
            VERIFY_IS_NOT_NULL(attachedFlyout);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"CanAttachedFlyoutShowHide: Execute ShowAttachedFlyout.");
        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAttachedFlyout);

        LOG_OUTPUT(L"CanAttachedFlyoutShowHide: Execute the Hide.");
        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateUIElementTree()
    {
        ValidateUIETreeWorker(Theme::Dark, "Dark");
        ValidateUIETreeWorker(Theme::Light, "Light");
        ValidateUIETreeWorker(Theme::HighContrast, "HC");
    }

    void FlyoutIntegrationTests::ValidateUIETreeWorker(Theme theme, Platform::String^ variation)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::TextBlock^ flyoutContent = nullptr;
        xaml_controls::Button^ flyoutTarget = nullptr;

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            variation += ref new Platform::String(L".Windowed");
        }
        else
        {
            variation += ref new Platform::String(L".Unwindowed");
        }

        auto validationRules = ref new Platform::String(DefaultUIElementTreeValidationRules);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::Grid();

            flyout = ref new xaml_controls::Flyout();
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Top;
            flyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;

            flyoutContent = ref new xaml_controls::TextBlock();
            flyoutContent->Text = "Flyout Content";
            flyout->Content = flyoutContent;

            flyoutTarget = ref new xaml_controls::Button();
            flyoutTarget->IsEnabled = false;    // Disabled so it is not Focusable and IsPressed is set to False to ensure same dumps for Desktop and Phone.
            flyoutTarget->Content = "Flyout Target";
            flyoutTarget->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            flyoutTarget->VerticalAlignment = xaml::VerticalAlignment::Center;
            rootPanel->Children->Append(flyoutTarget);

            openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"Flyout Opened event is fired!");
                flyoutOpenedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        std::function<void()> validator = [&]() { TestServices::Utilities->VerifyUIElementTreeWithRulesInline(variation, validationRules); };

        if (theme == Theme::Light)
        {
            RunOnUIThread([&]()
            {
                rootPanel->RequestedTheme = xaml::ElementTheme::Light;
                rootPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::White);
            });
        }
        else if (theme == Theme::HighContrast)
        {
            // Use a different validator for high-contrast, it'll put us in high-contrast mode as well
            // as do some extra validation.
            validator = [&]() { ControlHelper::ValidateUIElementTreeForHighContrast(variation, rootPanel, validationRules); };
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flyout->ShowAt(flyoutTarget);
        });
        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        validator();

        RunOnUIThread([&]()
        {
            flyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void FlyoutIntegrationTests::VerifyTargetPropertiesForwardedToPresenter()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        Platform::String^ dataContext = "DataContext";
        xaml::FlowDirection flowDirection = xaml::FlowDirection::RightToLeft;
        Platform::String^ language = "fr-FR";
        bool isTextScaleFactorEnabled = false;
        xaml::ElementTheme requestedTheme = xaml::ElementTheme::Light;

        SetupBasicFlyoutTest(flyout, button);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Panel^>(button->Parent);
            rootPanel->DataContext = dataContext;
            rootPanel->FlowDirection = flowDirection;
            rootPanel->Language = language;
            button->IsTextScaleFactorEnabled = isTextScaleFactorEnabled;
            rootPanel->RequestedTheme = requestedTheme;
            rootPanel->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::White);
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            VERIFY_ARE_EQUAL(dataContext, dynamic_cast<Platform::String^>(presenter->DataContext));
            VERIFY_ARE_EQUAL(flowDirection, presenter->FlowDirection);
            VERIFY_ARE_EQUAL(language, presenter->Language);
            VERIFY_ARE_EQUAL(isTextScaleFactorEnabled, presenter->IsTextScaleFactorEnabled);
            VERIFY_ARE_EQUAL(requestedTheme, presenter->RequestedTheme);
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::VerifyThemeAppliedToPresenterWhenTargetIsInPopup()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;
        xaml_primitives::Popup^ popup = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      Width='400' Height='200'> "
                L"  <StackPanel RequestedTheme='Light' Background='White' >"
                L"    <Popup x:Name='popup'>"
                L"      <Button x:Name='button' Content='button' > "
                L"        <Button.Flyout> "
                L"          <Flyout x:Name='flyout'> "
                L"            <TextBlock Text='flyout' /> "
                L"          </Flyout> "
                L"        </Button.Flyout> "
                L"      </Button> "
                L"    </Popup>"
                L"  </StackPanel>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup"));
            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            flyout = dynamic_cast<xaml_controls::Flyout^>(button->Flyout);

            // We need to open the popup after WindowContent is set.  If we set IsOpen="true" in the Popup's Xaml, the Popup is opened
            // during parse time, but the island doesn't exist yet so Xaml doesn't know what PopupRoot to use.
            popup->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            VERIFY_ARE_EQUAL(xaml::ElementTheme::Light, presenter->RequestedTheme);
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateIsLightDismissWhenFull()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        SetupBasicFlyoutTest(flyout, button);

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Full;

            openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutClosedEvent->Set();
            }));
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Show the flyout.");
            flyout->ShowAt(button);
        });

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Dismiss flyout by tapping outside of it.");
        wf::Point targetPoint = {};
        RunOnUIThread([&]()
        {
            // We tap to the left of the flyout, half-way down.
            auto presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            auto presenterBounds = ControlHelper::GetBounds(presenter);
            targetPoint = wf::Point(0, presenterBounds.Height / 2);
        });
        TestServices::InputHelper->Tap(targetPoint);

        LOG_OUTPUT(L"Waiting for flyout to close...");
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void SetupBasicFlyoutTest(xaml_controls::Flyout^& flyout, xaml_controls::Button^& button)
    {
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='button' Content='button.flyout' HorizontalAlignment='Left' FontSize='25' > "
                L"    <Button.Flyout> "
                L"      <Flyout Placement='Top'> "
                L"        <TextBlock FontSize='30' Text='BUTTON.FLYOUT' Margin='30' Foreground='DarkRed' x:Name='button_flyout_content'/> "
                L"      </Flyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = rootPanel;

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            flyout = dynamic_cast<xaml_controls::Flyout^>(button->Flyout);
            VERIFY_IS_NOT_NULL(flyout);
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    enum class UnloadBehavior : bool
    {
        remainOpen = false,
        shouldDismiss = true
    };

    void RunDismissOnUnloadTest(xaml_primitives::FlyoutPlacementMode mode, UnloadBehavior expected)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::Panel^ rootPanel = nullptr;
        xaml_controls::ContentPresenter^ flyoutTarget = nullptr;

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]
        {
            rootPanel = ref new xaml_controls::Grid;
            TestServices::WindowHelper->WindowContent = rootPanel;

            flyoutTarget = ref new xaml_controls::ContentPresenter;
            flyoutTarget->Content = ref new Platform::String(L"Flyout target");
            flyoutTarget->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            flyoutTarget->VerticalAlignment = xaml::VerticalAlignment::Center;
            rootPanel->Children->Append(flyoutTarget);

            auto flyoutContent = ref new xaml_controls::TextBlock;
            flyoutContent->Text = ref new Platform::String(L"I'm a flyout!");
            flyoutContent->FontSize = 20;

            flyout = ref new xaml_controls::Flyout;
            flyout->Placement = mode;
            flyout->Content = flyoutContent;
            xaml_controls::Flyout::SetAttachedFlyout(flyoutTarget, flyout);

            openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutClosedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Show the flyout.");
            flyout->ShowAt(flyoutTarget);
        });

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Dismiss flyout by unloading its placement target");
        RunOnUIThread([&]
        {
            rootPanel->Children->Clear();
        });

        if (expected == UnloadBehavior::shouldDismiss)
        {
            flyoutClosedEvent->WaitForDefault();
        }
        TestServices::WindowHelper->WaitForIdle();

        const auto flyoutClosedEventFired = flyoutClosedEvent->HasFired();
        if (expected == UnloadBehavior::shouldDismiss)
        {
            VERIFY_IS_TRUE(flyoutClosedEventFired);
        }
        else
        {
            VERIFY_IS_FALSE(flyoutClosedEventFired);
        }

        // This is cleaning up the remaining flyout after test.
        if (expected == UnloadBehavior::remainOpen)
        {
            RunOnUIThread([&]
            {
                flyout->Hide();
            });
        }
    }

    // Runs on all Win10 and later builds. The converged (UAP) behavior for
    // fullscreen flyouts yields an "overlay" flyout that isn't its own page
    // with its own AppBar.

    // This isn't a valid test for Phone 8.1 because fullscreen flyouts in
    // Phone 8.1 could have their own AppBar, which would cause the original
    // page's AppBar to unload, which could then result in immediately
    // dismissing the Flyout (because its placement target (the AppBar) unloaded).
    void FlyoutIntegrationTests::ValidateDismissOnPlacementTargetUnloaded()
    {
        RunDismissOnUnloadTest(xaml_primitives::FlyoutPlacementMode::Full, UnloadBehavior::shouldDismiss);
        RunDismissOnUnloadTest(xaml_primitives::FlyoutPlacementMode::Top, UnloadBehavior::shouldDismiss);
        RunDismissOnUnloadTest(xaml_primitives::FlyoutPlacementMode::Bottom, UnloadBehavior::shouldDismiss);
        RunDismissOnUnloadTest(xaml_primitives::FlyoutPlacementMode::Left, UnloadBehavior::shouldDismiss);
        RunDismissOnUnloadTest(xaml_primitives::FlyoutPlacementMode::Right, UnloadBehavior::shouldDismiss);
    }

    void FlyoutIntegrationTests::VerifyFlyoutPropertiesAtOpening()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::Button^ buttonFlyout = nullptr;
        xaml::Style^ styleLocal = nullptr;
        xaml_media::SolidColorBrush^ blue = nullptr;

        auto flyoutOpeningEvent = std::make_shared<Event>();
        auto openingRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opening);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='button' Content='button.flyout' HorizontalAlignment='Left' FontSize='25' > "
                L"    <Button.Flyout> "
                L"      <Flyout Placement='Bottom'> "
                L"        <TextBlock FontSize='30' Text='Before_ContentChangedAtOpening' /> "
                L"      </Flyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            flyout = dynamic_cast<xaml_controls::Flyout^>(button->Flyout);

            openingRegistration.Attach(
                flyout,
                ref new wf::EventHandler<Platform::Object^>(
                    [flyoutOpeningEvent, flyout, &buttonFlyout, &styleLocal, &blue](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"VerifyFlyoutContentChange: Flyout Opening event is fired!");

                buttonFlyout = ref new xaml_controls::Button();
                buttonFlyout->Content = L"Content_Changed!";

                // Replace the Content with new value.
                flyout->Content = buttonFlyout;

                // Replace the FlyoutPresenterStyle with new value.
                blue = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
                auto type = wxaml_interop::TypeName();
                type.Name = "Microsoft.UI.Xaml.Controls.FlyoutPresenter";
                type.Kind = wxaml_interop::TypeKind::Metadata;
                styleLocal = ref new xaml::Style(type);
                styleLocal->Setters->Append(ref  new xaml::Setter(xaml_controls::Control::BackgroundProperty, blue));
                flyout->FlyoutPresenterStyle = styleLocal;

                flyoutOpeningEvent->Set();
            }));

        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        flyoutOpeningEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flyout->Content, buttonFlyout);
            VERIFY_ARE_EQUAL(flyout->FlyoutPresenterStyle, styleLocal);
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateLoadedAndUnloadedRaisedAtCorrectTimes()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml::FrameworkElement^ flyoutContent = nullptr;

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        auto contentLoadedEventCount = std::make_shared<int>();
        auto contentUnloadedEventCount = std::make_shared<int>();
        auto contentLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        auto contentUnloadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Unloaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='button' Content='button.flyout' HorizontalAlignment='Left' FontSize='25' > "
                L"    <Button.Flyout> "
                L"      <Flyout Placement='Bottom'> "
                L"        <TextBlock FontSize='30' Text='Before_ContentChangedAtOpening' /> "
                L"      </Flyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            flyout = safe_cast<xaml_controls::Flyout^>(button->Flyout);
            flyoutContent = safe_cast<xaml::FrameworkElement^>(flyout->Content);

            openedRegistration.Attach(
                flyout,
                ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(
                flyout,
                ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutClosedEvent->Set();
            }));

            contentLoadedRegistration.Attach(
                flyoutContent,
                ref new xaml::RoutedEventHandler([contentLoadedEventCount](Platform::Object^, Platform::Object^)
            {
                (*contentLoadedEventCount)++;
            }));

            contentUnloadedRegistration.Attach(
                flyoutContent,
                ref new xaml::RoutedEventHandler([contentUnloadedEventCount](Platform::Object^, Platform::Object^)
            {
                (*contentUnloadedEventCount)++;
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, *contentLoadedEventCount);
            VERIFY_ARE_EQUAL(0, *contentUnloadedEventCount);
        });

        FlyoutHelper::HideFlyout(flyout);
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, *contentLoadedEventCount);
            VERIFY_ARE_EQUAL(1, *contentUnloadedEventCount);
        });
    }

    void FlyoutIntegrationTests::CanPopupNestedInFlyoutOpenClose()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::FlyoutPresenter^ flyoutPresenter = nullptr;
        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  x:Name='root' Background='SlateBlue' Width='400' Height='500' VerticalAlignment='Top' HorizontalAlignment='Left'>"
                L"  <Button x:Name='button' Content='button.flyout' VerticalAlignment='Top' HorizontalAlignment='Left' FontSize='25' Margin='50'>"
                L"    <Button.Flyout>"
                L"      <Flyout Placement='Top'>"
                L"          <ComboBox x:Name='comboBox' Margin='0,0,0,250'>"
                L"            <ComboBoxItem Content='TEST'/>"
                L"            <ComboBoxItem Content='TEST'/>"
                L"          </ComboBox>"
                L"      </Flyout>"
                L"    </Button.Flyout>"
                L"  </Button>"
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = rootPanel;
            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            flyout = safe_cast<xaml_controls::Flyout^>(button->Flyout);
            comboBox = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the Flyout hosting the ComboBox.");
        FlyoutHelper::OpenFlyout(flyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate that there is one popup open.");
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(button->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 1u);

            flyoutPresenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            VERIFY_IS_NOT_NULL(flyoutPresenter);
        });

        LOG_OUTPUT(L"Click the open flyout.");
        TestServices::InputHelper->LeftMouseClick(flyoutPresenter);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the ComboBox");
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate that there are two popups open.");
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(button->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 2u);
        });

        ComboBoxHelper::CloseComboBox(comboBox);
        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateRTLReversesHorizontalPlacementModes()
    {
        TestCleanupWrapper cleanup;

        auto flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);

        auto target = FlyoutHelper::CreateTarget(
            200 /*width*/, 200 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Center);

        wf::Point leftFlyoutPosition;
        wf::Point rightFlyoutPosition;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            leftFlyoutPosition = FlyoutHelper::GetFlyoutPresenter(flyout)->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point(0, 0));
        });

        FlyoutHelper::HideFlyout(flyout);

        RunOnUIThread([&]()
        {
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Right;
        });

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            rightFlyoutPosition = FlyoutHelper::GetFlyoutPresenter(flyout)->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point(0, 0));

            VERIFY_IS_LESS_THAN(leftFlyoutPosition.X, rightFlyoutPosition.X);
            VERIFY_ARE_EQUAL(leftFlyoutPosition.Y, rightFlyoutPosition.Y);
        });

        FlyoutHelper::HideFlyout(flyout);

        RunOnUIThread([&]()
        {
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Left;
            target->FlowDirection = xaml::FlowDirection::RightToLeft;
        });

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            auto flyoutPosition = flyoutPresenter->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point((float)flyoutPresenter->ActualWidth, 0));

            VERIFY_ARE_EQUAL(rightFlyoutPosition.X, flyoutPosition.X);
            VERIFY_ARE_EQUAL(rightFlyoutPosition.Y, flyoutPosition.Y);
        });

        FlyoutHelper::HideFlyout(flyout);

        RunOnUIThread([&]()
        {
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Right;
        });

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            auto flyoutPosition = flyoutPresenter->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point((float)flyoutPresenter->ActualWidth, 0));

            VERIFY_ARE_EQUAL(leftFlyoutPosition.X, flyoutPosition.X);
            VERIFY_ARE_EQUAL(leftFlyoutPosition.Y, flyoutPosition.Y);
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidatePlacementWithNoOpenPositions()
    {
        TestCleanupWrapper cleanup;

        auto leftFlyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);
        auto rightFlyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Right);
        auto topFlyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Top);
        auto bottomFlyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);
        wf::Rect windowBounds;

        RunOnUIThread([&]()
        {
            windowBounds = TestServices::WindowHelper->WindowBounds;
        });

        auto leftFlyoutTarget = FlyoutHelper::CreateTarget(
            windowBounds.Width - 10, windowBounds.Height - 10,
            ThicknessHelper::FromUniformLength(0),
            xaml::HorizontalAlignment::Right,
            xaml::VerticalAlignment::Center);
        auto rightFlyoutTarget = FlyoutHelper::CreateTarget(
            windowBounds.Width - 10, windowBounds.Height - 10,
            ThicknessHelper::FromUniformLength(0),
            xaml::HorizontalAlignment::Left,
            xaml::VerticalAlignment::Center);
        auto topFlyoutTarget = FlyoutHelper::CreateTarget(
            windowBounds.Width - 10, windowBounds.Height - 10,
            ThicknessHelper::FromUniformLength(0),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Bottom);
        auto bottomFlyoutTarget = FlyoutHelper::CreateTarget(
            windowBounds.Width - 10, windowBounds.Height - 10,
            ThicknessHelper::FromUniformLength(0),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Top);

        wf::Point leftFlyoutPosition;
        wf::Point topFlyoutPosition;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();

            rootPanel->Children->Append(leftFlyoutTarget);
            rootPanel->Children->Append(rightFlyoutTarget);
            rootPanel->Children->Append(topFlyoutTarget);
            rootPanel->Children->Append(bottomFlyoutTarget);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(leftFlyout, leftFlyoutTarget, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetFlyoutPresenter(leftFlyout);
            leftFlyoutPosition = flyoutPresenter->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point(0, 0));
        });

        FlyoutHelper::HideFlyout(leftFlyout);
        FlyoutHelper::OpenFlyout(rightFlyout, rightFlyoutTarget, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetFlyoutPresenter(rightFlyout);
            auto rightFlyoutPosition = flyoutPresenter->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point((float)flyoutPresenter->ActualWidth, 0));

            VERIFY_IS_LESS_THAN(leftFlyoutPosition.X, rightFlyoutPosition.X);
            VERIFY_ARE_EQUAL(leftFlyoutPosition.Y, rightFlyoutPosition.Y);
        });

        FlyoutHelper::HideFlyout(rightFlyout);
        FlyoutHelper::OpenFlyout(topFlyout, topFlyoutTarget, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetFlyoutPresenter(topFlyout);
            topFlyoutPosition = flyoutPresenter->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point(0, 0));
        });

        FlyoutHelper::HideFlyout(topFlyout);
        FlyoutHelper::OpenFlyout(bottomFlyout, bottomFlyoutTarget, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto flyoutPresenter = FlyoutHelper::GetFlyoutPresenter(bottomFlyout);
            auto bottomFlyoutPosition = flyoutPresenter->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point(0, (float)flyoutPresenter->ActualHeight));

            VERIFY_ARE_EQUAL(topFlyoutPosition.X, bottomFlyoutPosition.X);
            VERIFY_IS_LESS_THAN(topFlyoutPosition.Y, bottomFlyoutPosition.Y);
        });

        FlyoutHelper::HideFlyout(bottomFlyout);
    }

    void FlyoutIntegrationTests::CanOpenChildFlyout()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml_controls::Button^ childButton = nullptr;
        xaml_controls::Flyout^ childFlyout = nullptr;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            childFlyout = ref new xaml_controls::Flyout();
            childFlyout->Content = rect;

            childButton = ref new xaml_controls::Button();
            childButton->Content = "child button";

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = childButton;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // First open our parent.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Now try to open the child flyout.
        FlyoutHelper::OpenFlyout(childFlyout, childButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 2 flyout popups open.
        RunOnUIThread([&]()
        {
            auto openPopups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(openPopups->Size, 2u);
        });

        Event childFlyoutClosedEvent;
        auto childFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        childFlyoutClosedRegistration.Attach(childFlyout, [&]() { childFlyoutClosedEvent.Set(); });

        // Hiding the parent flyout should also close the child flyout.
        FlyoutHelper::HideFlyout(parentFlyout);
        childFlyoutClosedEvent.WaitForDefault();

        // There should be no open popup's now.
        RunOnUIThread([&]()
        {
            auto openPopups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(openPopups->Size, 0u);
        });
    }

    void FlyoutIntegrationTests::ValidatePressAndHoldOnlyOpensOnce()
    {
        TestCleanupWrapper cleanup;

        auto flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);
        auto target = FlyoutHelper::CreateTarget(
            200 /*width*/, 200 /*height*/,
            ThicknessHelper::FromUniformLength(10),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Center);

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutOpenedCount = std::make_shared<unsigned int>();
        auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutClosedCount = std::make_shared<unsigned int>();
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);
        auto holdingRegistration = CreateSafeEventRegistration(xaml::UIElement, Holding);

        *flyoutOpenedCount = 0;
        *flyoutClosedCount = 0;

        RunOnUIThread([&]()
        {
            flyoutOpenedRegistration.Attach(flyout, [flyoutOpenedEvent, flyoutOpenedCount]()
            {
                flyoutOpenedEvent->Set();
                (*flyoutOpenedCount)++;
            });

            flyoutClosedRegistration.Attach(flyout, [flyoutClosedEvent, flyoutClosedCount]()
            {
                flyoutClosedEvent->Set();
                (*flyoutClosedCount)++;
            });

            holdingRegistration.Attach(target, [flyout, target]()
            {
                auto showOptions = ref new xaml_primitives::FlyoutShowOptions();
                showOptions->Position = wf::Point(0, 0);
                flyout->ShowAt(target, showOptions);
            });

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Hold(target);

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flyout->Hide();
        });

        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(1u, *flyoutOpenedCount);
        VERIFY_ARE_EQUAL(1u, *flyoutClosedCount);
    }

    void FlyoutIntegrationTests::ValidateShowAtWindowEdge()
    {
        ValidateShowAtWindowEdge(1.0f  /*scaleFactor*/, wf::Point() /*offset*/);
        ValidateShowAtWindowEdge(1.25f /*scaleFactor*/, wf::Point() /*offset*/);
        ValidateShowAtWindowEdge(1.5f  /*scaleFactor*/, wf::Point() /*offset*/);
        ValidateShowAtWindowEdge(1.75f /*scaleFactor*/, wf::Point() /*offset*/);
        ValidateShowAtWindowEdge(2.25f /*scaleFactor*/, wf::Point() /*offset*/);
    }

    void FlyoutIntegrationTests::ValidateShowBeyondWindowEdge()
    {
        ValidateShowAtWindowEdge(1.0f  /*scaleFactor*/, wf::Point(-600, -600) /*offset*/);
        ValidateShowAtWindowEdge(1.5f  /*scaleFactor*/, wf::Point(100, 100) /*offset*/);
    }

    void FlyoutIntegrationTests::ValidateShowAtWindowEdge(float scaleFactor, wf::Point offset)
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(421, 421);
        TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, scaleFactor);

        wf::Rect windowBounds;

        RunOnUIThread([&]()
        {
            windowBounds = TestServices::WindowHelper->WindowBounds;
            LOG_OUTPUT(L"WindowBounds: %f,%f,%f,%f.", windowBounds.X, windowBounds.Y, windowBounds.Width, windowBounds.Height);
        });

        auto flyout = FlyoutHelper::CreateDefaultFlyout(
            xaml_primitives::FlyoutPlacementMode::Right);

        auto target = FlyoutHelper::CreateTarget(
            windowBounds.Width /*width*/, windowBounds.Height /*height*/,
            ThicknessHelper::FromUniformLength(0) /*margin*/,
            xaml::HorizontalAlignment::Right,
            xaml::VerticalAlignment::Bottom);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutOpenedCount = std::make_shared<unsigned int>();
        auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutClosedCount = std::make_shared<unsigned int>();
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        *flyoutOpenedCount = 0;
        *flyoutClosedCount = 0;

        RunOnUIThread([&]()
        {
            flyoutOpenedRegistration.Attach(flyout, [flyoutOpenedEvent, flyoutOpenedCount]()
            {
                LOG_OUTPUT(L"Flyout.Opened event raised.");
                flyoutOpenedEvent->Set();
                (*flyoutOpenedCount)++;
            });

            flyoutClosedRegistration.Attach(flyout, [flyoutClosedEvent, flyoutClosedCount]()
            {
                LOG_OUTPUT(L"Flyout.Closed event raised.");
                flyoutClosedEvent->Set();
                (*flyoutClosedCount)++;
            });

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);

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
            LOG_OUTPUT(L"Showing flyout at position %f,%f.", windowBounds.Width, windowBounds.Height);
            auto showOptions = ref new xaml_primitives::FlyoutShowOptions();
            showOptions->Position = wf::Point(windowBounds.Width + offset.X, windowBounds.Height + offset.Y);
            flyout->ShowAt(target, showOptions);
        });

        LOG_OUTPUT(L"Waiting for Opened event...");
        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Hiding flyout.");
            flyout->Hide();
        });

        LOG_OUTPUT(L"Waiting for Closed event...");
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying event counts.");
            VERIFY_ARE_EQUAL(1u, *flyoutOpenedCount);
            VERIFY_ARE_EQUAL(1u, *flyoutClosedCount);
        });
    }

    void FlyoutIntegrationTests::ValidateFlyoutCanOpenLeftAndUpOutsideWindowBoundaries()
    {
        TestCleanupWrapper cleanup([&]()
        {
            TestServices::WindowHelper->MaximizeDesktopWindow();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });
        
        // We'll set the window size to something small to allow the popup to open up.
        TestServices::WindowHelper->SetDesktopWindowSize(500, 400);
        TestServices::WindowHelper->WaitForIdle();
        
        HWND currentHwnd = reinterpret_cast<HWND>(TestServices::WindowHelper->GetUIAWindowHandle());
        
        RECT windowRect = {};
        VERIFY_IS_TRUE(::GetWindowRect(currentHwnd, &windowRect));
        
        // We'll also move the window to the bottom-right of the screen in order to ensure that the popup can open left and up.
        auto monitorBounds = TestServices::WindowHelper->MonitorBounds;
        int windowPositionX = static_cast<int>(monitorBounds.Width - (windowRect.right - windowRect.left));
        int windowPositionY = static_cast<int>(monitorBounds.Height - (windowRect.bottom - windowRect.top));
        TestServices::WindowHelper->MoveDesktopWindow(static_cast<int>(windowPositionX), static_cast<int>(windowPositionY));
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::Flyout^ flyout = nullptr;
            
        RunOnUIThread([&]()
        {
            flyout = ref new xaml_controls::Flyout();
            flyout->ShouldConstrainToRootBounds = false;
            
            auto rectangle = ref new xaml_shapes::Rectangle();
            rectangle->Width = 100;
            rectangle->Height = 100;
            rectangle->Fill = ref new xaml_media::SolidColorBrush(wu::Colors::Red);
            
            flyout->Content = rectangle;
        });

        xaml_controls::Button^ target = nullptr;
        
        RunOnUIThread([&]()
        {
            target = ref new xaml_controls::Button();
            target->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            target->VerticalAlignment = xaml::VerticalAlignment::Top;
        });

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        RunOnUIThread([&]()
        {
            flyoutOpenedRegistration.Attach(flyout, [flyoutOpenedEvent]()
            {
                LOG_OUTPUT(L"Flyout.Opened event raised.");
                flyoutOpenedEvent->Set();
            });

            flyoutClosedRegistration.Attach(flyout, [flyoutClosedEvent]()
            {
                LOG_OUTPUT(L"Flyout.Closed event raised.");
                flyoutClosedEvent->Set();
            });

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);

            loadedRegistration.Attach(rootPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"Grid.Loaded event raised.");
                loadedEvent->Set();
            });
            
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Left;
            flyout->ShowAt(target);
        });

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        
        RunOnUIThread([&]()
        {
            auto flyoutPopup = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(Private::Infrastructure::TestServices::WindowHelper->WindowContent->XamlRoot)->GetAt(0);
            HWND flyoutHwnd = reinterpret_cast<HWND>(Private::Infrastructure::TestServices::WindowHelper->Popup_GetWindow(flyoutPopup));
            
            RECT flyoutRect = {};
            VERIFY_IS_TRUE(::GetWindowRect(flyoutHwnd, &flyoutRect));
            
            LOG_OUTPUT(L"Opening left. Window x-position = %d; popup x-position = %d", windowPositionX, flyoutRect.left);
            VERIFY_IS_LESS_THAN(flyoutRect.left, windowPositionX);
                
            flyout->Hide();
        });

        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Top;
            flyout->ShowAt(target);
        });

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto flyoutPopup = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(Private::Infrastructure::TestServices::WindowHelper->WindowContent->XamlRoot)->GetAt(0);
            HWND flyoutHwnd = reinterpret_cast<HWND>(Private::Infrastructure::TestServices::WindowHelper->Popup_GetWindow(flyoutPopup));

            RECT flyoutRect = {};
            VERIFY_IS_TRUE(::GetWindowRect(flyoutHwnd, &flyoutRect));

            LOG_OUTPUT(L"Opening top. Window y-position = %d; popup y-position = %d", windowPositionY, flyoutRect.top);
            VERIFY_IS_LESS_THAN(flyoutRect.top, windowPositionY);
            
            flyout->Hide();
        });

        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void FlyoutIntegrationTests::CanOpenChildFlyoutWithOpenSibling()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml_controls::Button^ siblingButton = nullptr;
        xaml_controls::Flyout^ siblingFlyout = nullptr;

        xaml_controls::Button^ childButton = nullptr;
        xaml_controls::Flyout^ childFlyout = nullptr;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
            rect->Width = 100;
            rect->Height = 100;

            siblingFlyout = ref new xaml_controls::Flyout();
            siblingFlyout->Content = rect;

            siblingButton = ref new xaml_controls::Button();
            siblingButton->Content = "sibling button";

            rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            childFlyout = ref new xaml_controls::Flyout();
            childFlyout->Content = rect;

            childButton = ref new xaml_controls::Button();
            childButton->Content = "child button";

            auto flyoutRoot = ref new xaml_controls::StackPanel();
            flyoutRoot->Children->Append(siblingButton);
            flyoutRoot->Children->Append(childButton);

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = flyoutRoot;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open the parent and sibling flyout to set up the test.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::OpenFlyout(siblingFlyout, siblingButton, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 2u, E_FAIL, L"There should be 2 popups open, one for each flyout.");
        });

        Event siblingFlyoutClosedEvent;
        auto siblingFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        siblingFlyoutClosedRegistration.Attach(siblingFlyout, [&]() { siblingFlyoutClosedEvent.Set(); });

        // Try to open the child flyout.
        FlyoutHelper::OpenFlyout(childFlyout, childButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Opening our child flyout should have caused its sibling to close.
        siblingFlyoutClosedEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 2u);
        });

        // Hiding the parent flyout will hide our child flyout as well, so no need
        // to do it explicitly.
        FlyoutHelper::HideFlyout(parentFlyout);
    }

    void FlyoutIntegrationTests::CanReopenChildFlyoutWithNewPlacementTarget()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml_controls::Button^ placement1 = nullptr;
        xaml_controls::Button^ placement2 = nullptr;

        xaml_controls::Flyout^ childFlyout = nullptr;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            childFlyout = ref new xaml_controls::Flyout();
            childFlyout->Content = rect;

            placement1 = ref new xaml_controls::Button();
            placement1->Content = "placement 1";

            placement2 = ref new xaml_controls::Button();
            placement2->Content = "placement 2";

            auto flyoutRoot = ref new xaml_controls::StackPanel();
            flyoutRoot->Children->Append(placement1);
            flyoutRoot->Children->Append(placement2);

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = flyoutRoot;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open the parent and the child to set up the test.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::OpenFlyout(childFlyout, placement1, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 2u, E_FAIL, L"There should be 2 popups open, one for each flyout.");
        });

        Event childFlyoutClosedEvent;
        auto childFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        childFlyoutClosedRegistration.Attach(childFlyout, [&]() { childFlyoutClosedEvent.Set(); });

        Event childFlyoutOpeningEvent;
        auto childFlyoutOpeningRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opening);
        childFlyoutOpeningRegistration.Attach(childFlyout, [&]() { childFlyoutOpeningEvent.Set(); });

        Event childFlyoutOpenedEvent;
        auto childFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        childFlyoutOpenedRegistration.Attach(childFlyout, [&]() { childFlyoutOpenedEvent.Set(); });

        // Re-open the child with a different placement target.
        FlyoutHelper::OpenFlyout(childFlyout, placement2, FlyoutOpenMethod::Programmatic_ShowAt);

        // Opening our child flyout with a new placement target should have caused it
        // to close and then re-open.
        childFlyoutClosedEvent.WaitForDefault();
        childFlyoutOpeningEvent.WaitForDefault();
        childFlyoutOpenedEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 2u);
        });

        // Hiding the parent flyout will hide our child flyout as well, so no need
        // to do it explicitly.
        FlyoutHelper::HideFlyout(parentFlyout);
    }

    void FlyoutIntegrationTests::CanOpenChildFlyoutTargetingItemInListView()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml::FrameworkElement^ childTarget = nullptr;
        xaml_controls::Flyout^ childFlyout = nullptr;

        xaml_controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            childFlyout = ref new xaml_controls::Flyout();
            childFlyout->Content = rect;

            listView = ref new xaml_controls::ListView();
            listView->Items->Append(L"Item");

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = listView;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // First open our parent.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Do this after the hosting flyout has opened so that the containers will have been realized.
        RunOnUIThread([&]()
        {
            childTarget = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));
        });

        // Now try to open the child flyout.
        FlyoutHelper::OpenFlyout(childFlyout, childTarget, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 2 flyout popups open.
        RunOnUIThread([&]()
        {
            auto openPopups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(openPopups->Size, 2u);
        });

        Event childFlyoutClosedEvent;
        auto childFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        childFlyoutClosedRegistration.Attach(childFlyout, [&]() { childFlyoutClosedEvent.Set(); });

        // Hiding the parent flyout should also close the child flyout.
        FlyoutHelper::HideFlyout(parentFlyout);
        childFlyoutClosedEvent.WaitForDefault();

        // There should be no open popup's now.
        RunOnUIThread([&]()
        {
            auto openPopups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(openPopups->Size, 0u);
        });
    }

    void FlyoutIntegrationTests::DoesFlyoutChainCloseWhenOpeningNewRootFlyout()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml_controls::Button^ childButton = nullptr;
        xaml_controls::Flyout^ childFlyout = nullptr;

        xaml_controls::Flyout^ otherFlyout = nullptr;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
            rect->Width = 100;
            rect->Height = 100;

            childFlyout = ref new xaml_controls::Flyout();
            childFlyout->Content = rect;

            childButton = ref new xaml_controls::Button();
            childButton->Content = "child button";

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = childButton;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            otherFlyout = ref new xaml_controls::Flyout();
            otherFlyout->Content = rect;

            auto root = ref new xaml_controls::StackPanel();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open the parent and the child to set up the test.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::OpenFlyout(childFlyout, childButton, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 2u, E_FAIL, L"There should be 2 popups open, one for each flyout.");
        });

        Event childFlyoutClosedEvent;
        auto childFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        childFlyoutClosedRegistration.Attach(childFlyout, [&]() { childFlyoutClosedEvent.Set(); });

        Event parentFlyoutClosedEvent;
        auto parentFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        parentFlyoutClosedRegistration.Attach(parentFlyout, [&]() { parentFlyoutClosedEvent.Set(); });

        // Open a new root flyout.
        FlyoutHelper::OpenFlyout(otherFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Opening a new root flyout should close the previous chain.
        childFlyoutClosedEvent.WaitForDefault();
        parentFlyoutClosedEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 1u);
        });

        FlyoutHelper::HideFlyout(otherFlyout);
    }

    void FlyoutIntegrationTests::DoesChildFlyoutCloseWhenPlacementTargetUnloads()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;
        xaml_controls::Grid^ parentFlyoutRoot = nullptr;

        xaml_controls::Button^ childButton = nullptr;
        xaml_controls::Flyout^ childFlyout = nullptr;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            childFlyout = ref new xaml_controls::Flyout();
            childFlyout->Content = rect;

            childButton = ref new xaml_controls::Button();
            childButton->Content = "child button";

            parentFlyoutRoot = ref new xaml_controls::Grid();
            parentFlyoutRoot->Children->Append(childButton);

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = parentFlyoutRoot;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open the parent and child to setup the test.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::OpenFlyout(childFlyout, childButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 2 flyout popups open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 2u, E_FAIL, L"There should be 2 popups open, one for each flyout.");
        });

        Event childFlyoutClosedEvent;
        auto childFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        childFlyoutClosedRegistration.Attach(childFlyout, [&]() { childFlyoutClosedEvent.Set(); });

        // Remove the child's placement target from the tree, which should close the child flyout.
        RunOnUIThread([&]()
        {
            parentFlyoutRoot->Children->Clear();
        });
        childFlyoutClosedEvent.WaitForDefault();

        // There should be 1 open popup now, the parent's.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 1u);
        });

        FlyoutHelper::HideFlyout(parentFlyout);
    }

    void FlyoutIntegrationTests::CanOpenChildMenuFlyout()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml_controls::Button^ menuFlyoutButton = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;

        RunOnUIThread([&]()
        {
            menuFlyout = ref new xaml_controls::MenuFlyout();

            for (size_t i = 0; i < 5; ++i)
            {
                auto item = ref new xaml_controls::MenuFlyoutItem();
                item->Text = "menu item";
                menuFlyout->Items->Append(item);
            }

            menuFlyoutButton = ref new xaml_controls::Button();
            menuFlyoutButton->Content = "menu flyout button";

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = menuFlyoutButton;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open the flyout to setup the test.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 1 flyout popup open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1u, E_FAIL, L"There should be 1 popup open.");
        });

        // Try to open the menu flyout.
        FlyoutHelper::OpenFlyout(menuFlyout, menuFlyoutButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 2 flyout popups open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 2u);
        });

        // Hiding the parent flyout should close the child MenuFlyout as well.
        FlyoutHelper::HideFlyout(parentFlyout);
    }

    void FlyoutIntegrationTests::CanOpenChildDateTimePickerFlyout()
    {
        TestCleanupWrapper cleanup;

        CanOpenDateTimePickerWorker<xaml_controls::DatePicker>();
        CanOpenDateTimePickerWorker<xaml_controls::TimePicker>();
    }

    template <class DateTimePickerType>
    void FlyoutIntegrationTests::CanOpenDateTimePickerWorker()
    {
        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        DateTimePickerType^ dateTimePicker = nullptr;

        RunOnUIThread([&]()
        {
            dateTimePicker = ref new DateTimePickerType();

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = dateTimePicker;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open the flyout to setup the test.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 1 flyout popup open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1u, E_FAIL, L"There should be 1 popup open.");
        });

        // Try to open the DatePicker from within the flyout.
        DateTimePickerHelper::OpenDateTimePicker<DateTimePickerType>(dateTimePicker);

        // Validate that there are now have 2 flyout popups open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 2u);
        });

        // Hiding the parent flyout should close the child DatePickerFlyout as well.
        FlyoutHelper::HideFlyout(parentFlyout);
    }

    void FlyoutIntegrationTests::CanOpenChildCalenderViewFlyout()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml_controls::CalendarDatePicker^ calendarDatePicker = nullptr;

        RunOnUIThread([&]()
        {
            calendarDatePicker = ref new xaml_controls::CalendarDatePicker();

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = calendarDatePicker;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open the flyout to setup the test.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 1 flyout popup open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1u, E_FAIL, L"There should be 1 popup open.");

            // Try to open the DatePicker from within the flyout.
            calendarDatePicker->IsCalendarOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Validate that there are now have 2 flyout popups open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 2u);
        });

        // Hiding the parent flyout should close the child CalendarView flyout as well.
        FlyoutHelper::HideFlyout(parentFlyout);
    }

    void FlyoutIntegrationTests::DoesChainRightClickWithChildFlyouts()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ target = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml_controls::Button^ childButton1 = nullptr;
        xaml_controls::Flyout^ childFlyout1 = nullptr;

        xaml_controls::Button^ childButton2 = nullptr;
        xaml_controls::Flyout^ childFlyout2 = nullptr;

        auto targetRightTappedRegistration = CreateSafeEventRegistration(xaml_controls::Button, RightTapped);
        bool wasTargetRightTapped = false;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            childFlyout2 = ref new xaml_controls::Flyout();
            childFlyout2->Placement = xaml_primitives::FlyoutPlacementMode::Bottom;
            childFlyout2->Content = rect;

            childButton2 = ref new xaml_controls::Button();
            childButton2->Content = "child button 2";

            childFlyout1 = ref new xaml_controls::Flyout();
            childFlyout1->Placement = xaml_primitives::FlyoutPlacementMode::Bottom;
            childFlyout1->Content = childButton2;

            childButton1 = ref new xaml_controls::Button();
            childButton1->Content = "child button 1";

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Placement = xaml_primitives::FlyoutPlacementMode::Bottom;
            parentFlyout->Content = childButton1;

            target = ref new xaml_controls::Button();
            target->VerticalAlignment = xaml::VerticalAlignment::Top;
            target->Content = "button";
            target->Height = 100;

            targetRightTappedRegistration.Attach(target, [&]() { wasTargetRightTapped = true; });

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(target);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open our flyouts to setup the test.
        FlyoutHelper::OpenFlyout(parentFlyout, target, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::OpenFlyout(childFlyout1, childButton1, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::OpenFlyout(childFlyout2, childButton2, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 3 flyout popups open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(target->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 3u, E_FAIL, L"There should be 3 popups open.");
        });

        // Inject right-click.
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, target);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // There should be no popups open now becaue the flyouts should have closed.
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(target->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 0, E_FAIL, L"The flyouts should have closed.");

            // The right-click should have been chained, so the target's handler should have
            // been invoked.
            VERIFY_IS_TRUE(wasTargetRightTapped);
        });
    }

    void FlyoutIntegrationTests::CanCancelClosing()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        RunOnUIThread([&]()
        {
            // Don't put a focusable control in the popup because we want to
            // test dismissing the popup via its key handlers, so it needs to
            // get focus.
            auto textBlock = ref new xaml_controls::TextBlock();
            textBlock->Text = "You can cancel closing me!";

            flyout = ref new xaml_controls::Flyout();
            flyout->Content = textBlock;

            button = ref new xaml_controls::Button();
            button->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(button);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open the flyout to setup the test.
        FlyoutHelper::OpenFlyout(flyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there is 1 flyout popup open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(button->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1u, E_FAIL, L"There should be 1 popup open.");
        });

        Event closingEvent;
        auto closingRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closing);
        closingRegistration.Attach(
            flyout,
            ref new wf::TypedEventHandler<xaml_primitives::FlyoutBase^, xaml_primitives::FlyoutBaseClosingEventArgs^>([&](xaml_primitives::FlyoutBase^, xaml_primitives::FlyoutBaseClosingEventArgs^ args)
            {
                args->Cancel = true;
                closingEvent.Set();
            }));

        // A list of the various scenarios that could cause a flyout to close; we want
        // to make sure that each raises a Closing event which can be canceled.
        std::vector<std::pair<const wchar_t*, std::function<void()>>> closingScenarios;
        closingScenarios.push_back(std::make_pair(L"Programmatic", [&]() { RunOnUIThread([&]() { flyout->Hide(); }); }));
        closingScenarios.push_back(std::make_pair(L"Touch light-dismiss", [&]() { TestServices::InputHelper->Tap(button); }));
        closingScenarios.push_back(std::make_pair(L"Keyboard cancel", []() { CommonInputHelper::Cancel(InputDevice::Keyboard); }));
        closingScenarios.push_back(std::make_pair(L"Gamepad cancel", []() { CommonInputHelper::Cancel(InputDevice::Gamepad); }));

        for (auto scenario : closingScenarios)
        {
            LOG_OUTPUT(L"Try to dismiss the flyout: %s", scenario.first);
            scenario.second();
            TestServices::WindowHelper->WaitForIdle();
            closingEvent.WaitForDefault();

            // Validate that there is still 1 flyout popup open.
            RunOnUIThread([&]()
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(button->XamlRoot);
                VERIFY_ARE_EQUAL(popups->Size, 1u);
            });
        }

        // Detach the Closing event registration before trying to hide the flyout to
        // make sure we properly cleanup.
        closingRegistration.Detach();
        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::CanCancelChildClosing()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ parentButton = nullptr;
        xaml_controls::Flyout^ parentFlyout = nullptr;

        xaml_controls::Button^ childButton1 = nullptr;
        xaml_controls::Flyout^ childFlyout1 = nullptr;

        xaml_controls::Button^ childButton2 = nullptr;
        xaml_controls::Flyout^ childFlyout2 = nullptr;

        RunOnUIThread([&]()
        {
            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;

            childFlyout2 = ref new xaml_controls::Flyout();
            childFlyout2->Content = rect;

            childButton2 = ref new xaml_controls::Button();
            childButton2->Content = "child button 2";

            childFlyout1 = ref new xaml_controls::Flyout();
            childFlyout1->Content = childButton2;

            childButton1 = ref new xaml_controls::Button();
            childButton1->Content = "child button 1";

            parentFlyout = ref new xaml_controls::Flyout();
            parentFlyout->Content = childButton1;

            parentButton = ref new xaml_controls::Button();
            parentButton->Content = "button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(parentButton);

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Open our flyouts to setup the test.
        FlyoutHelper::OpenFlyout(parentFlyout, parentButton, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::OpenFlyout(childFlyout1, childButton1, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::OpenFlyout(childFlyout2, childButton2, FlyoutOpenMethod::Programmatic_ShowAt);

        // Validate that there are now have 3 flyout popups open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 3u, E_FAIL, L"There should be 3 popups open.");
        });

        Event closingEvent;
        auto closingRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closing);
        closingRegistration.Attach(
            childFlyout1,
            ref new wf::TypedEventHandler<xaml_primitives::FlyoutBase^, xaml_primitives::FlyoutBaseClosingEventArgs^>([&](xaml_primitives::FlyoutBase^, xaml_primitives::FlyoutBaseClosingEventArgs^ args)
        {
            args->Cancel = true;
            closingEvent.Set();
        }));

        // Hiding the parent flyout, but the child flyouts should remain
        // open because we cancel closing them.
        FlyoutHelper::HideFlyout(parentFlyout);

        // Validate that the child flyouts are still open.
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(parentButton->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 2u);
        });

        // Detach the Closing event registration before trying to hide the flyout to
        // make sure we properly cleanup.  This should also close any child flyouts
        // to this flyout.
        closingRegistration.Detach();
        FlyoutHelper::HideFlyout(childFlyout1);
    }

    void FlyoutIntegrationTests::ValidateLightDismissOverlayMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::Button^ target = nullptr;

        RunOnUIThread([&]()
        {
            flyout = ref new xaml_controls::Flyout();
            target = ref new xaml_controls::Button();

            TestServices::WindowHelper->WindowContent = target;
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        LOG_OUTPUT(L"Validate that the default is Auto and the flyout's popup is set to Off (or On if on Xbox)");
        {
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(flyout->LightDismissOverlayMode, xaml_controls::LightDismissOverlayMode::Auto);
            });

            ValidateFlyoutPopupLightDismissOverlayMode(
                TestServices::Utilities->IsXBox ? xaml_controls::LightDismissOverlayMode::On : xaml_controls::LightDismissOverlayMode::Off);
        }

        LOG_OUTPUT(L"Validate that when set to On the flyout's popup is also set to On.");
        {
            RunOnUIThread([&]()
            {
                flyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            });

            ValidateFlyoutPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode::On);
        }

        LOG_OUTPUT(L"Validate that when set to Off the flyout's popup is also set to Off.");
        {
            RunOnUIThread([&]()
            {
                flyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            });

            ValidateFlyoutPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode::Off);
        }

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::DoesAutoLightDismissOverlayModeCreateOverlayOnXbox()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::Button^ target = nullptr;

        RunOnUIThread([&]()
        {
            flyout = ref new xaml_controls::Flyout();
            flyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Auto;

            target = ref new xaml_controls::Button();
            TestServices::WindowHelper->WindowContent = target;
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        ValidateFlyoutPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode::On);

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateFlyoutPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode expectedMode)
    {
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(TestServices::WindowHelper->WindowContent->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1, E_FAIL, L"Expected exactly one open Popup.");

            auto flyoutPopup = popups->GetAt(0);
            VERIFY_ARE_EQUAL(flyoutPopup->LightDismissOverlayMode, expectedMode);

            if (expectedMode == xaml_controls::LightDismissOverlayMode::On)
            {
                auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(flyoutPopup);
                VERIFY_IS_NOT_NULL(overlayElement);
            }
        });
    };

    void FlyoutIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;
        FlyoutHelper::ValidateOverlayBrush<xaml_controls::Flyout>(L"FlyoutLightDismissOverlayBackground");
    }

    void FlyoutIntegrationTests::ValidatePointerEventsCanBeRoutedThroughLightDismissLayerSetInXaml()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ buttonPassThrough = nullptr;
        xaml_controls::Button^ buttonNonPassThrough = nullptr;
        xaml_controls::Button^ flyoutTarget = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto root = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Margin='100' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='buttonPassThrough'>Pass-through element</Button>"
                L"  <Button x:Name='buttonNonPassThrough'>Non-pass-through element</Button>"
                L"  <Button x:Name='flyoutTarget' Content='Click to open flyout'>"
                L"    <Button.Flyout>"
                L"      <Flyout Placement='Bottom' OverlayInputPassThroughElement='{Binding ElementName=buttonPassThrough}'/>"
                L"    </Button.Flyout>"
                L"  </Button>"
                L"</StackPanel>"));

            buttonPassThrough = safe_cast<xaml_controls::Button^>(root->FindName(L"buttonPassThrough"));
            buttonNonPassThrough = safe_cast<xaml_controls::Button^>(root->FindName(L"buttonNonPassThrough"));
            flyoutTarget = safe_cast<xaml_controls::Button^>(root->FindName(L"flyoutTarget"));

            loadedEventRegistration.Attach(root, [&]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        ValidatePointerEventsCanBeRoutedThroughLightDismissLayer(
            buttonPassThrough,
            buttonNonPassThrough,
            flyoutTarget);
    }

    void FlyoutIntegrationTests::ValidatePointerEventsCanBeRoutedThroughLightDismissLayerHiDpi()
    {
        TestServices::WindowHelper->ResetWindowContentAndScaleWaitForIdle(2.0f);
        ValidatePointerEventsCanBeRoutedThroughLightDismissLayerSetInXaml();
    }

    void FlyoutIntegrationTests::ValidatePointerEventsCanBeRoutedThroughLightDismissLayerManuallySet()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ buttonPassThrough = nullptr;
        xaml_controls::Button^ buttonNonPassThrough = nullptr;
        xaml_controls::Button^ flyoutTarget = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            buttonPassThrough = ref new xaml_controls::Button();
            buttonPassThrough->Content = "Pass-through element";

            buttonNonPassThrough = ref new xaml_controls::Button();
            buttonNonPassThrough->Content = "Non-pass-through element";

            flyoutTarget = ref new xaml_controls::Button();
            flyoutTarget->Content = "Click to open flyout";

            auto flyout = ref new xaml_controls::Flyout();
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::Bottom;
            flyout->OverlayInputPassThroughElement = buttonPassThrough;
            flyoutTarget->Flyout = flyout;

            auto root = ref new xaml_controls::StackPanel();
            root->Margin = xaml::Thickness({ 10, 10, 10, 10 });
            root->Children->Append(buttonPassThrough);
            root->Children->Append(buttonNonPassThrough);
            root->Children->Append(flyoutTarget);

            loadedEventRegistration.Attach(root, [&]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        ValidatePointerEventsCanBeRoutedThroughLightDismissLayer(
            buttonPassThrough,
            buttonNonPassThrough,
            flyoutTarget);
    }

    void FlyoutIntegrationTests::ValidatePointerEventsCanBeRoutedThroughLightDismissLayer(
        xaml_controls::Button^ buttonPassThrough,
        xaml_controls::Button^ buttonNonPassThrough,
        xaml_controls::Button^ flyoutTarget)
    {
        xaml_primitives::FlyoutBase^ flyout = nullptr;
        xaml::VisualStateGroup^ commonStates = ControlHelper::GetVisualStateGroup(buttonPassThrough, L"CommonStates");

        auto pointerOverStateEvent = std::make_shared<Event>();
        auto passThroughClickEvent = std::make_shared<Event>();
        auto nonPassThroughClickEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();
        auto currentStateChangedRegistration = CreateSafeEventRegistration(xaml::VisualStateGroup, CurrentStateChanged);
        auto passThroughClickEventRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto nonPassThroughClickEventRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        RunOnUIThread([&]()
        {
            flyout = flyoutTarget->Flyout;
        });

        currentStateChangedRegistration.Attach(
            commonStates,
            ref new xaml::VisualStateChangedEventHandler(
                [&](Platform::Object^ sender, xaml::VisualStateChangedEventArgs^ e)
                {
                    if (e->NewState->Name == L"PointerOver")
                    {
                        pointerOverStateEvent->Set();
                    }
                }));

        passThroughClickEventRegistration.Attach(buttonPassThrough, [&]() { passThroughClickEvent->Set(); });
        nonPassThroughClickEventRegistration.Attach(buttonNonPassThrough, [&]() { nonPassThroughClickEvent->Set(); });
        closedRegistration.Attach(flyout, [&]() { closedEvent->Set(); });

        LOG_OUTPUT(L"Open the flyout by clicking the button.");
        FlyoutHelper::OpenFlyout(flyout, flyoutTarget, FlyoutOpenMethod::Mouse);

        LOG_OUTPUT(L"We should be able to put the pass-through element into the pointer-over state by mousing over it, even with the flyout open.");
        TestServices::InputHelper->MoveMouse(buttonPassThrough);

        pointerOverStateEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"That shouldn't close the flyout, though.");
            VERIFY_ARE_EQUAL(1u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(flyoutTarget->XamlRoot)->Size);
        });

        LOG_OUTPUT(L"And clicking it should raise the click event while closing it.");
        TestServices::InputHelper->LeftMouseClick(buttonPassThrough);

        passThroughClickEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(flyoutTarget->XamlRoot)->Size);
        });

        LOG_OUTPUT(L"Now open the flyout again.");
        FlyoutHelper::OpenFlyout(flyout, flyoutTarget, FlyoutOpenMethod::Mouse);

        LOG_OUTPUT(L"Mousing over the non-pass-through element with the flyout open should not change its visual state.");
        TestServices::InputHelper->MoveMouse(buttonNonPassThrough);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(buttonNonPassThrough, L"CommonStates", L"Normal"));

        LOG_OUTPUT(L"And clicking it should not raise the click event.");
        TestServices::InputHelper->LeftMouseClick(buttonNonPassThrough);

        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(nonPassThroughClickEvent->HasFired());

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void FlyoutIntegrationTests::ValidatePresenterLoadedWithoutShowAtDoesNotCrash()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ flyoutTarget = nullptr;
        xaml_primitives::FlyoutBase^ flyout = nullptr;
        xaml_controls::FlyoutPresenter^ flyoutPresenter = nullptr;
        xaml_primitives::Popup^ flyoutPopup = nullptr;

        auto rootLoadedEvent = std::make_shared<Event>();
        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto presenterUnloadedEvent = std::make_shared<Event>();
        auto presenterLoadedEvent = std::make_shared<Event>();
        auto rootLoadedEventRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto flyoutOpenedEventRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto presenterUnloadedEventRegistration = CreateSafeEventRegistration(xaml_controls::FlyoutPresenter, Unloaded);
        auto presenterLoadedEventRegistration = CreateSafeEventRegistration(xaml_controls::FlyoutPresenter, Loaded);

        RunOnUIThread([&]()
        {
            auto root = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='FlyoutTarget' Content='Click to open flyout'>"
                L"    <Button.Flyout>"
                L"      <Flyout ShouldConstrainToRootBounds='True' />"
                L"    </Button.Flyout>"
                L"  </Button>"
                L"</StackPanel>"));

            flyoutTarget = safe_cast<xaml_controls::Button^>(root->FindName(L"FlyoutTarget"));
            flyout = flyoutTarget->Flyout;

            // In very specific timing circumstances, we can incur the occurrence where
            // FlyoutBase::OnPresenterLoaded() is called after FlyoutBase::OnPresenterUnloaded()
            // without ShowAt() having ever been called.  We want to ensure that we don't crash
            // with a null placement target in this circumstance.
            flyoutOpenedEventRegistration.Attach(flyout,
                [&]()
                {
                    flyoutPopup = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(flyoutTarget->XamlRoot)->GetAt(0);
                    flyoutPresenter = safe_cast<xaml_controls::FlyoutPresenter^>(flyoutPopup->Child);

                    presenterUnloadedEventRegistration.Attach(flyoutPresenter,
                        [&]()
                        {
                            flyoutPopup->Child = flyoutPresenter;
                            flyoutPopup->IsOpen = true;
                            presenterUnloadedEvent->Set();
                            presenterUnloadedEventRegistration.Detach();
                        });

                    presenterLoadedEventRegistration.Attach(flyoutPresenter,
                        [&]()
                        {
                            presenterLoadedEvent->Set();
                            presenterLoadedEventRegistration.Detach();
                        });

                    flyoutPopup->Child = nullptr;
                    flyoutOpenedEvent->Set();

                    flyoutOpenedEventRegistration.Detach();
                });

            rootLoadedEventRegistration.Attach(root, [&]() { rootLoadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = root;
        });

        rootLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            flyout->ShowAt(flyoutTarget);
        });

        flyoutOpenedEvent->WaitForDefault();
        presenterUnloadedEvent->WaitForDefault();
        presenterLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateFlyoutTakesThePositionItsSetToTheFirstTime()
    {
        TestCleanupWrapper cleanup;

        xaml_shapes::Rectangle^ rectangle = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::Grid^ flyoutGrid = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto openingRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opening);

        wf::Point rightFlyoutPosition;
        wf::Point rightRectanglePosition;

        RunOnUIThread([&]()
        {
            auto root = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Rectangle x:Name='Rectangle' Width = '100' Height = '40' Fill = 'Gray'> "
                L"      <Rectangle.ContextFlyout>"
                L"          <Flyout x:Name='Flyout' Opening = 'Flyout_Opening'>"
                L"              <Grid x:Name='FlyoutGrid' Width = '300' Height = '300' />"
                L"          </Flyout>"
                L"      </Rectangle.ContextFlyout>"
                L"  </Rectangle>"
                L"</Grid>"));

            rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"Rectangle"));
            flyout = safe_cast<xaml_controls::Flyout^>(root->FindName(L"Flyout"));
            flyoutGrid = safe_cast<xaml_controls::Grid^>(root->FindName(L"FlyoutGrid"));

            loadedEventRegistration.Attach(root, [&]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        openingRegistration.Attach(
            flyout,
            ref new wf::EventHandler<Platform::Object^>(
                [flyout](Platform::Object^, Platform::Object^)
        {
            LOG_OUTPUT(L"===========================================================================================");
            LOG_OUTPUT(L"VerifyFlyoutContentChange: Flyout Opening event is fired!");
            LOG_OUTPUT(L"===========================================================================================");
            flyout->SetValue(xaml_controls::Flyout::PlacementProperty, xaml_primitives::FlyoutPlacementMode::Right);
        }));

        // Inject right-click.
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, rectangle);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rightFlyoutPosition = FlyoutHelper::GetFlyoutPresenter(flyout)->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point(0, 0));
            rightRectanglePosition = rectangle->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point(0, 0));

            VERIFY_IS_LESS_THAN(rightRectanglePosition.X, rightFlyoutPosition.X);
        });

        // Not doing that causes a memory leak
        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateRightClickCanHideOneFlyoutWithoutHidingAllFlyouts()
    {
        TestCleanupWrapper cleanup;

        xaml_shapes::Rectangle^ rectangle = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;
        xaml_shapes::Rectangle^ innerRectangle1 = nullptr;
        xaml_shapes::Rectangle^ innerRectangle2 = nullptr;
        xaml_controls::Flyout^ innerFlyout1 = nullptr;
        xaml_controls::Flyout^ innerFlyout2 = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto mainFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto mainFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        auto innerFlyout1OpenedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto innerFlyout1ClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        auto innerFlyout2OpenedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto innerFlyout2ClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        bool mainFlyoutIsOpen = false;
        bool innerFlyout1IsOpen = false;
        bool innerFlyout2IsOpen = false;

        RunOnUIThread([&]()
        {
            auto root = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='Rectangle' Width='100' Height='40' Fill='Gray'>"
                L"        <Rectangle.ContextFlyout>"
                L"            <Flyout x:Name='Flyout'>"
                L"                <StackPanel>"
                L"                    <Rectangle x:Name='InnerRectangle1' Width='100' Height='40' Fill='Red'>"
                L"                        <Rectangle.ContextFlyout>"
                L"                            <Flyout x:Name='InnerFlyout1'>"
                L"                                <Rectangle Width='100' Height='40' Fill='Yellow' />"
                L"                            </Flyout>"
                L"                        </Rectangle.ContextFlyout>"
                L"                    </Rectangle>"
                L"                    <Rectangle x:Name='InnerRectangle2' Width='100' Height='40' Fill='Blue'>"
                L"                        <Rectangle.ContextFlyout>"
                L"                            <Flyout x:Name='InnerFlyout2'>"
                L"                                <Rectangle Width='100' Height='40' Fill='Yellow' />"
                L"                            </Flyout>"
                L"                        </Rectangle.ContextFlyout>"
                L"                    </Rectangle>"
                L"                </StackPanel>"
                L"            </Flyout>"
                L"        </Rectangle.ContextFlyout>"
                L"    </Rectangle>"
                L"</Grid>"));

            rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"Rectangle"));
            flyout = safe_cast<xaml_controls::Flyout^>(root->FindName(L"Flyout"));
            innerRectangle1 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"InnerRectangle1"));
            innerRectangle2 = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"InnerRectangle2"));
            innerFlyout1 = safe_cast<xaml_controls::Flyout^>(root->FindName(L"InnerFlyout1"));
            innerFlyout2 = safe_cast<xaml_controls::Flyout^>(root->FindName(L"InnerFlyout2"));

            mainFlyoutOpenedRegistration.Attach(flyout, [&]() { mainFlyoutIsOpen = true; });
            mainFlyoutClosedRegistration.Attach(flyout, [&]() { mainFlyoutIsOpen = false; });
            innerFlyout1OpenedRegistration.Attach(innerFlyout1, [&]() { innerFlyout1IsOpen = true; });
            innerFlyout1ClosedRegistration.Attach(innerFlyout1, [&]() { innerFlyout1IsOpen = false; });
            innerFlyout2OpenedRegistration.Attach(innerFlyout2, [&]() { innerFlyout2IsOpen = true; });
            innerFlyout2ClosedRegistration.Attach(innerFlyout2, [&]() { innerFlyout2IsOpen = false; });

            loadedEventRegistration.Attach(root, [&]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Inject a right-click on the main rectangle to open the main flyout.");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, rectangle);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Inject a right-click on the main rectangle to open the first flyout.");
            VERIFY_IS_TRUE(mainFlyoutIsOpen);
            VERIFY_IS_FALSE(innerFlyout1IsOpen);
            VERIFY_IS_FALSE(innerFlyout2IsOpen);
        });

        LOG_OUTPUT(L"Inject a right-click on the first inner rectangle to open the second inner flyout.");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, innerRectangle1);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The main and second inner flyouts should now be open.");
            VERIFY_IS_TRUE(mainFlyoutIsOpen);
            VERIFY_IS_TRUE(innerFlyout1IsOpen);
            VERIFY_IS_FALSE(innerFlyout2IsOpen);
        });

        LOG_OUTPUT(L"Inject a right-click on the second inner rectangle to open the first inner flyout.");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, innerRectangle2);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The main and first inner flyouts should now be open.");
            VERIFY_IS_TRUE(mainFlyoutIsOpen);
            VERIFY_IS_FALSE(innerFlyout1IsOpen);
            VERIFY_IS_TRUE(innerFlyout2IsOpen);
        });

        LOG_OUTPUT(L"Close the main flyout to ensure that everything is cleaned up properly.");
        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateLosingFocusClosesFlyout()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::Button^ flyoutButton = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            auto root = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <TextBox x:Name='TextBox' />"
                L"    <Button x:Name='FlyoutButton'>"
                L"        <Button.Flyout>"
                L"            <Flyout x:Name='Flyout' />"
                L"        </Button.Flyout>"
                L"    </Button>"
                L"</StackPanel>"));

            textBox = safe_cast<xaml_controls::TextBox^>(root->FindName(L"TextBox"));
            flyoutButton = safe_cast<xaml_controls::Button^>(root->FindName(L"FlyoutButton"));
            flyout = safe_cast<xaml_controls::Flyout^>(root->FindName(L"Flyout"));

            flyoutClosedRegistration.Attach(flyout, [&]() { flyoutClosedEvent->Set(); });

            loadedEventRegistration.Attach(root, [&]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Click on the textBox to set last input device type to pointer.");
        TestServices::InputHelper->LeftMouseClick(textBox);

        LOG_OUTPUT(L"First, open the flyout.");
        FlyoutHelper::OpenFlyout(flyout, flyoutButton, FlyoutOpenMethod::Programmatic_ShowAt);

        LOG_OUTPUT(L"Now give focus to the text box.");
        ControlHelper::EnsureFocused(textBox);

        LOG_OUTPUT(L"The flyout should have closed...");
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"...and the TextBox should still have the same focus as before.");
            VERIFY_IS_TRUE(textBox->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            VERIFY_ARE_EQUAL(xaml::FocusState::Pointer, textBox->FocusState);
        });
    }

    void FlyoutIntegrationTests::VerifyFlyoutTakesFocusWithAllowFocusOnInteractionFalseWithUIA()
    {
        TestCleanupWrapper cleanup;

        // We want to test the scenario where:
        // We have a button with AllowFocusOnInteraction=false (default for AppBarButton)
        // The Button has a flyout
        // The user uses UIA (e.g. Narrator) to move focus to the button and invoke it to open the flyout.
        // Focus should move into the Flyout.
        // See MSFT:[January PreProd Blocking] [RS4 Inbox/Store][Screen Reader - Photos - More->Set as] "Set as" Menu items are not accessible using Narrator scan mode.

        xaml_controls::Button^ dummyButton;
        xaml_controls::Button^ flyoutButton;
        xaml_controls::Button^ buttonInFlyout;
        xaml_controls::Flyout^ flyout;

        Event loadedEvent;
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        Event clickEvent;
        auto passThroughClickEventRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        Event openedEvent;
        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);

        Event buttonInFlyoutGotFocus;
        auto buttonInFlyoutGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        Event flyoutButtonGotFocus;
        auto flyoutButtonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <Button x:Name="dummyButton" Content="Dummy Button" />
                        <Button x:Name="flyoutButton" Content="Flyout Button" AllowFocusOnInteraction="False" >
                            <Button.Flyout>
                                <Flyout x:Name="flyout">
                                    <Button x:Name="buttonInFlyout" Content="Button in Flyout" />
                                </Flyout>
                            </Button.Flyout>
                        </Button>
                    </StackPanel>)"));

            flyoutButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName("flyoutButton"));
            dummyButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName("dummyButton"));
            buttonInFlyout = safe_cast<xaml_controls::Button^>(rootPanel->FindName("buttonInFlyout"));
            flyout = safe_cast<xaml_controls::Flyout^>(rootPanel->FindName("flyout"));

            loadedEventRegistration.Attach(rootPanel, [&loadedEvent]() { loadedEvent.Set(); });
            passThroughClickEventRegistration.Attach(dummyButton, [&clickEvent]() { clickEvent.Set(); });
            openedRegistration.Attach(flyout, [&openedEvent]() { openedEvent.Set(); });
            flyoutButtonGotFocusRegistration.Attach(flyoutButton, [&flyoutButtonGotFocus]() { flyoutButtonGotFocus.Set(); });
            buttonInFlyoutGotFocusRegistration.Attach(buttonInFlyout, [&buttonInFlyoutGotFocus]() { buttonInFlyoutGotFocus.Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for Loaded event");
        loadedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping dummy button");
        TestServices::InputHelper->Tap(dummyButton);
        LOG_OUTPUT(L"Waiting for Click event");
        clickEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to flyoutButton using UIA");
        xaml_automation_peers::ButtonAutomationPeer^ buttonAp;
        RunOnUIThread([&]()
        {
            buttonAp = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(flyoutButton));
            buttonAp->SetFocus();
        });
        LOG_OUTPUT(L"Waiting for Button to get focus");
        flyoutButtonGotFocus.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking flyoutButton using UIA");
        RunOnUIThread([&]()
        {
            buttonAp->Invoke();
        });

        LOG_OUTPUT(L"Waiting for Flyout to open");
        openedEvent.WaitForDefault();

        LOG_OUTPUT(L"Waiting for buttonInFlyout to get focus");
        buttonInFlyoutGotFocus.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
            LOG_OUTPUT(L"FocusedElement: %s", focusedElement->ToString()->Data());
            if(auto fe = dynamic_cast<xaml::FrameworkElement^>(focusedElement))
            {
                LOG_OUTPUT(L"FocusedElement Name: %s", fe->Name->ToString()->Data());
            }
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(buttonInFlyout));
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateFlyoutPlacementMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button;
        xaml_controls::Flyout^ flyout;

        SetupFlyoutPlacementModeTest(&button, &flyout);

        // Note:
        // The target is 100x100 and centered in a 300x300 panel.
        // The flyout is 50x50.
        // There is also a 4px padding between the target and the flyout

        // The PlacementMode and the expected bounds:
        std::vector<std::pair<xaml_primitives::FlyoutPlacementMode, wf::Rect>> placementsAndBounds =
        {
            { xaml_primitives::FlyoutPlacementMode::Top,                    wf::Rect(125, 46,50,50) },
            { xaml_primitives::FlyoutPlacementMode::TopEdgeAlignedLeft,     wf::Rect(100, 46,50,50) },
            { xaml_primitives::FlyoutPlacementMode::TopEdgeAlignedRight,    wf::Rect(150, 46,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Bottom,                 wf::Rect(125,204,50,50) },
            { xaml_primitives::FlyoutPlacementMode::BottomEdgeAlignedLeft,  wf::Rect(100,204,50,50) },
            { xaml_primitives::FlyoutPlacementMode::BottomEdgeAlignedRight, wf::Rect(150,204,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Left,                   wf::Rect( 46,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::LeftEdgeAlignedTop,     wf::Rect( 46,100,50,50) },
            { xaml_primitives::FlyoutPlacementMode::LeftEdgeAlignedBottom,  wf::Rect( 46,150,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Right,                  wf::Rect(204,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::RightEdgeAlignedTop,    wf::Rect(204,100,50,50) },
            { xaml_primitives::FlyoutPlacementMode::RightEdgeAlignedBottom, wf::Rect(204,150,50,50) }
        };

        for (auto placementAndBounds : placementsAndBounds)
        {
            auto placement = placementAndBounds.first;
            auto expectedBounds = placementAndBounds.second;

            LOG_OUTPUT(L"Testing with Placement=%s", placement.ToString()->Data());
            RunOnUIThread([&]()
            {
                flyout->Placement = placement;
            });

            FlyoutHelper::OpenFlyout(flyout, button, FlyoutOpenMethod::Programmatic_ShowAt);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
                auto bounds = ControlHelper::GetBounds(presenter);

                LOG_OUTPUT(L"Expected Bounds: %f,%f,%f,%f", expectedBounds.X, expectedBounds.Y, expectedBounds.Width, expectedBounds.Height);
                LOG_OUTPUT(L"Actual Bounds: %f,%f,%f,%f", bounds.X, bounds.Y, bounds.Width, bounds.Height);
                VERIFY_IS_TRUE(bounds.Equals(expectedBounds));
            });

            FlyoutHelper::HideFlyout(flyout);
        }
    }

    void FlyoutIntegrationTests::ValidateFlyoutPlacementModeAtPoint()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button;
        xaml_controls::Flyout^ flyout;

        SetupFlyoutPlacementModeTest(&button, &flyout);

        // Note:
        // The target is 100x100 and centered in a 300x300 panel.
        // The flyout is 50x50.
        // The flyout is shown at (50,25) within the target.

        // The PlacementMode and the expected bounds:
        std::vector<std::pair<xaml_primitives::FlyoutPlacementMode, wf::Rect>> placementsAndBounds =
        {
            { xaml_primitives::FlyoutPlacementMode::Top,                  wf::Rect(125, 75,50,50) },
            { xaml_primitives::FlyoutPlacementMode::TopEdgeAlignedLeft,     wf::Rect(150, 75,50,50) },
            { xaml_primitives::FlyoutPlacementMode::TopEdgeAlignedRight,    wf::Rect(100, 75,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Bottom,               wf::Rect(125,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::BottomEdgeAlignedLeft,  wf::Rect(150,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::BottomEdgeAlignedRight, wf::Rect(100,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Left,                 wf::Rect(100,100,50,50) },
            { xaml_primitives::FlyoutPlacementMode::LeftEdgeAlignedTop,     wf::Rect(100,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::LeftEdgeAlignedBottom,  wf::Rect(100, 75,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Right,                wf::Rect(150,100,50,50) },
            { xaml_primitives::FlyoutPlacementMode::RightEdgeAlignedTop,    wf::Rect(150,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::RightEdgeAlignedBottom, wf::Rect(150, 75,50,50) }
        };

        xaml_primitives::FlyoutShowOptions^ showOptions = nullptr;

        RunOnUIThread([&]()
        {
            showOptions = ref new xaml_primitives::FlyoutShowOptions();
            showOptions->Position = wf::Point(50, 25);
        });

        for (auto placementAndBounds : placementsAndBounds)
        {
            auto placement = placementAndBounds.first;
            auto expectedBounds = placementAndBounds.second;

            LOG_OUTPUT(L"Testing with Placement=%s", placement.ToString()->Data());
            RunOnUIThread([&]()
            {
                flyout->Placement = placement;
            });

            FlyoutHelper::ShowFlyoutWithOptions(flyout, button, showOptions);

            RunOnUIThread([&]()
            {
                auto presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
                auto bounds = ControlHelper::GetBounds(presenter);

                LOG_OUTPUT(L"Expected Bounds: %f,%f,%f,%f", expectedBounds.X, expectedBounds.Y, expectedBounds.Width, expectedBounds.Height);
                LOG_OUTPUT(L"Actual Bounds: %f,%f,%f,%f", bounds.X, bounds.Y, bounds.Width, bounds.Height);
                VERIFY_IS_TRUE(bounds.Equals(expectedBounds));
            });

            FlyoutHelper::HideFlyout(flyout);
        }
    }

    void FlyoutIntegrationTests::ValidateFlyoutPlacementModeAtPointWithExclusionRect()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button;
        xaml_controls::Flyout^ flyout;

        SetupFlyoutPlacementModeTest(&button, &flyout);

        // Note:
        // The target is 100x100 and centered in a 300x300 panel.
        // The panel has a 120x120 exclusion rect centered at (-10,-10)
        // The flyout is 50x50.
        // The flyout is shown at (50,25) within the target.

        // The PlacementMode and the expected bounds:
        std::vector<std::pair<xaml_primitives::FlyoutPlacementMode, wf::Rect>> placementsAndBounds =
        {
            { xaml_primitives::FlyoutPlacementMode::Top,                    wf::Rect(125, 40,50,50) },
            { xaml_primitives::FlyoutPlacementMode::TopEdgeAlignedLeft,     wf::Rect(150, 40,50,50) },
            { xaml_primitives::FlyoutPlacementMode::TopEdgeAlignedRight,    wf::Rect(100, 40,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Bottom,                 wf::Rect(125,210,50,50) },
            { xaml_primitives::FlyoutPlacementMode::BottomEdgeAlignedLeft,  wf::Rect(150,210,50,50) },
            { xaml_primitives::FlyoutPlacementMode::BottomEdgeAlignedRight, wf::Rect(100,210,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Left,                   wf::Rect( 40,100,50,50) },
            { xaml_primitives::FlyoutPlacementMode::LeftEdgeAlignedTop,     wf::Rect( 40,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::LeftEdgeAlignedBottom,  wf::Rect( 40, 75,50,50) },
            { xaml_primitives::FlyoutPlacementMode::Right,                  wf::Rect(210,100,50,50) },
            { xaml_primitives::FlyoutPlacementMode::RightEdgeAlignedTop,    wf::Rect(210,125,50,50) },
            { xaml_primitives::FlyoutPlacementMode::RightEdgeAlignedBottom, wf::Rect(210, 75,50,50) }
        };

        xaml_primitives::FlyoutShowOptions^ showOptions = nullptr;

        RunOnUIThread([&]()
        {
            showOptions = ref new xaml_primitives::FlyoutShowOptions();
            showOptions->Position = wf::Point(50, 25);
            showOptions->ExclusionRect = wf::Rect(-10, -10, 120, 120);
        });

        for (auto placementAndBounds : placementsAndBounds)
        {
            auto placement = placementAndBounds.first;
            auto expectedBounds = placementAndBounds.second;

            LOG_OUTPUT(L"Testing with Placement=%s", placement.ToString()->Data());
            RunOnUIThread([&]()
            {
                flyout->Placement = placement;
            });

            FlyoutHelper::ShowFlyoutWithOptions(flyout, button, showOptions);

            RunOnUIThread([&]()
            {
                auto presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
                auto bounds = ControlHelper::GetBounds(presenter);

                LOG_OUTPUT(L"Expected Bounds: %f,%f,%f,%f", expectedBounds.X, expectedBounds.Y, expectedBounds.Width, expectedBounds.Height);
                LOG_OUTPUT(L"Actual Bounds: %f,%f,%f,%f", bounds.X, bounds.Y, bounds.Width, bounds.Height);
                VERIFY_IS_TRUE(bounds.Equals(expectedBounds));
            });

            FlyoutHelper::HideFlyout(flyout);
        }
    }

    void FlyoutIntegrationTests::ValidateFlyoutShowMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout;
        xaml_controls::Button^ targetButton;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <StackPanel.Resources>
                            <Flyout x:Name="flyout" ShowMode="Transient">
                                <StackPanel>
                                    <TextBox />
                                </StackPanel>
                            </Flyout>
                        </StackPanel.Resources>
                        <Button Content="Flyout target" x:Name="targetButton" />
                    </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            flyout = dynamic_cast<xaml_controls::Flyout^>(rootPanel->FindName(L"flyout"));
            targetButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"targetButton"));
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, targetButton, FlyoutOpenMethod::Programmatic_ShowAt);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml_primitives::FlyoutShowMode::Transient, flyout->ShowMode);
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateTransientFlyoutDoesNotTakeFocus()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout;
        xaml_controls::Button^ focusButton;
        xaml_controls::Button^ targetButton;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <StackPanel.Resources>
                            <Flyout x:Name="flyout">
                                <StackPanel>
                                    <TextBox />
                                </StackPanel>
                            </Flyout>
                        </StackPanel.Resources>
                        <Button Content="Button for focus" x:Name="focusButton" />
                        <Button Content="Flyout target" x:Name="targetButton" />
                    </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            flyout = dynamic_cast<xaml_controls::Flyout^>(rootPanel->FindName(L"flyout"));
            focusButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"focusButton"));
            targetButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"targetButton"));
        });

        TestServices::WindowHelper->WaitForIdle();

        ControlHelper::EnsureFocused(focusButton);

        xaml_primitives::FlyoutShowOptions^ showOptions = nullptr;
        RunOnUIThread([&]()
        {
            showOptions = ref new xaml_primitives::FlyoutShowOptions();
            showOptions->ShowMode = xaml_primitives::FlyoutShowMode::Transient;
        });

        FlyoutHelper::ShowFlyoutWithOptions(flyout, targetButton, showOptions);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, focusButton->FocusState);
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutIntegrationTests::ValidateTransientFlyoutLightDismissLayerAllowsInputToPassThrough()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout;
        xaml_controls::Button^ clickHandlerButton;
        xaml_controls::Button^ targetButton;

        auto clickEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <StackPanel.Resources>
                            <Flyout x:Name="flyout" Placement="Bottom">
                                <StackPanel>
                                    <TextBox />
                                </StackPanel>
                            </Flyout>
                        </StackPanel.Resources>
                        <Button Content="Button with click handler" x:Name="clickHandlerButton" />
                        <Button Content="Flyout target" x:Name="targetButton" />
                    </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            flyout = dynamic_cast<xaml_controls::Flyout^>(rootPanel->FindName(L"flyout"));
            clickHandlerButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"clickHandlerButton"));
            targetButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"targetButton"));

            flyoutClosedRegistration.Attach(flyout, [&]() { flyoutClosedEvent->Set(); });
            clickRegistration.Attach(clickHandlerButton, [&]() { clickEvent->Set(); });
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_primitives::FlyoutShowOptions^ showOptions = nullptr;
        RunOnUIThread([&]()
        {
            showOptions = ref new xaml_primitives::FlyoutShowOptions();
            showOptions->ShowMode = xaml_primitives::FlyoutShowMode::Transient;
        });

        FlyoutHelper::ShowFlyoutWithOptions(flyout, targetButton, showOptions);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->LeftMouseClick(clickHandlerButton);

        clickEvent->WaitForDefault();
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void FlyoutIntegrationTests::ValidateFlyoutTransientWithDismissOnPointerMoveAway()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout;
        xaml_controls::Button^ targetButton;

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Background="Green">
                        <Button Content="Flyout target" x:Name="targetButton">
                            <Button.Flyout>
                                <Flyout x:Name="flyout" ShowMode="TransientWithDismissOnPointerMoveAway">
                                    <StackPanel>
                                        <Button Content="Button in Flyout" />
                                    </StackPanel>
                                </Flyout>
                            </Button.Flyout>
                        </Button>
                    </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            flyout = dynamic_cast<xaml_controls::Flyout^>(rootPanel->FindName(L"flyout"));
            targetButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"targetButton"));

            flyoutOpenedRegistration.Attach(flyout, [&flyoutOpenedEvent]()
            {
                LOG_OUTPUT(L"Opened raised.");
                flyoutOpenedEvent->Set();
            });

            flyoutClosedRegistration.Attach(flyout, [&flyoutClosedEvent]()
            {
                LOG_OUTPUT(L"Closed raised.");
                flyoutClosedEvent->Set();
            });
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Opening the flyout...");
        TestServices::InputHelper->LeftMouseClick(targetButton);
        TestServices::WindowHelper->WaitForIdle();
        flyoutOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml_primitives::FlyoutShowMode::TransientWithDismissOnPointerMoveAway, flyout->ShowMode);
            VERIFY_ARE_EQUAL(flyout->IsOpen, true);
        });

        LOG_OUTPUT(L"Moving the mouse away, which should close it.");
        TestServices::InputHelper->MoveMouse(wf::Point(300,500));
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for close...");
        flyoutClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flyout->IsOpen, false);
        });

        // Repeat!
        LOG_OUTPUT(L"Repeating : Opening the flyout again...");
        TestServices::InputHelper->LeftMouseClick(targetButton);
        TestServices::WindowHelper->WaitForIdle();
        flyoutOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml_primitives::FlyoutShowMode::TransientWithDismissOnPointerMoveAway, flyout->ShowMode);
            VERIFY_ARE_EQUAL(flyout->IsOpen, true);
        });

        LOG_OUTPUT(L"Moving the mouse away, which should close it.");
        TestServices::InputHelper->MoveMouse(wf::Point(300,500));
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for close...");
        flyoutClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(flyout->IsOpen, false);
        });
    }

    void FlyoutIntegrationTests::ValidateContextFlyoutPlacedAtMouse()
    {
        TestCleanupWrapper cleanup;

        xaml_shapes::Rectangle^ rectangle;
        xaml_controls::Flyout^ flyout;

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);
        auto pointerPressedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, PointerPressed);
        auto pointerReleasedRegistration = CreateSafeEventRegistration(xaml_shapes::Rectangle, PointerReleased);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Rectangle Width="100" Height="100" Fill="Red" x:Name="rectangle">
                            <Rectangle.ContextFlyout>
                                <Flyout x:Name="flyout" Placement="Bottom">
                                    <StackPanel>
                                        <TextBox />
                                    </StackPanel>
                                </Flyout>
                            </Rectangle.ContextFlyout>
                        </Rectangle>
                    </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            rectangle = dynamic_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectangle"));
            flyout = dynamic_cast<xaml_controls::Flyout^>(rootPanel->FindName(L"flyout"));

            flyoutOpenedRegistration.Attach(flyout, [&flyoutOpenedEvent]()
            {
                LOG_OUTPUT(L"Opened raised.");
                flyoutOpenedEvent->Set();
            });

            flyoutClosedRegistration.Attach(flyout, [&flyoutClosedEvent]()
            {
                LOG_OUTPUT(L"Closed raised.");
                flyoutClosedEvent->Set();
            });

            pointerPressedRegistration.Attach(rectangle, []()
            {
                LOG_OUTPUT(L"PointerPressed raised.");
            });

            pointerReleasedRegistration.Attach(rectangle, []()
            {
                LOG_OUTPUT(L"PointerReleased raised.");
            });
        });

        TestServices::WindowHelper->WaitForIdle();

        wf::Point point1 = {0, 0};
        wf::Point point2 = {25, 25};
  
        LOG_OUTPUT(L"Right click over Rectangle at position %f, %f from center.", point1.X, point1.Y);
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, rectangle, static_cast<int>(point1.X), static_cast<int>(point1.Y));
        LOG_OUTPUT(L"Waiting for Flyout.Opened event.");
        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::FlyoutPresenter^ presenter = nullptr;
        wf::Rect bounds1 = {};
        wf::Rect bounds2 = {};

        RunOnUIThread([&]()
        {
            presenter = FlyoutHelper::GetFlyoutPresenter(flyout);
            bounds1 = ControlHelper::GetBounds(presenter);
        });

        LOG_OUTPUT(L"Hiding flyout.");
        FlyoutHelper::HideFlyout(flyout);
        LOG_OUTPUT(L"Waiting for Flyout.Closed event.");
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Right click over Rectangle at position %f, %f from center.", point2.X, point2.Y);
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, rectangle, static_cast<int>(point2.X), static_cast<int>(point2.Y));

        LOG_OUTPUT(L"Waiting for Flyout.Opened event.");
        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            bounds2 = ControlHelper::GetBounds(presenter);
        });

        LOG_OUTPUT(L"Hiding flyout.");
        FlyoutHelper::HideFlyout(flyout);
        LOG_OUTPUT(L"Waiting for Flyout.Closed event.");
        flyoutClosedEvent->WaitForDefault();

        LOG_OUTPUT(L"Verifying relative flyout positions X: %f, %f", bounds1.X + point2.X, bounds2.X + point1.X);
        LOG_OUTPUT(L"Verifying relative flyout positions Y: %f, %f", bounds1.Y + point2.Y, bounds2.Y + point1.Y);
        VERIFY_IS_LESS_THAN_OR_EQUAL(bounds1.X + point2.X, bounds2.X + point1.X + 1);
        VERIFY_IS_LESS_THAN_OR_EQUAL(bounds1.Y + point2.Y, bounds2.Y + point1.Y + 1);
    }

    void FlyoutIntegrationTests::VerifyInputDevicePrefersPrimaryCommands()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Bottom);
        xaml_controls::Button^ target = nullptr;

        RunOnUIThread([&]()
        {
            target = ref new xaml_controls::Button();
            target->Content = ref new Platform::String(L"Click for flyout");
            target->Flyout = flyout;
            target->ContextFlyout = flyout;

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        auto verifyOpenInputDevice =
            [flyout, target](FlyoutOpenMethod openMethod, bool expectedInputDevicePrefersPrimaryCommands)
            {
                auto openingRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opening);

                RunOnUIThread([&]()
                {
                    openingRegistration.Attach(
                        flyout,
                        ref new wf::EventHandler<Platform::Object^>(
                            [flyout, expectedInputDevicePrefersPrimaryCommands](Platform::Object^, Platform::Object^)
                            {
                                LOG_OUTPUT(L"Inside FlyoutBase.Opening. Expecting OpenInputDevice to be %s.", expectedInputDevicePrefersPrimaryCommands ? L"true" : L"false");
                                VERIFY_ARE_EQUAL(expectedInputDevicePrefersPrimaryCommands, flyout->InputDevicePrefersPrimaryCommands);
                            }));
                });

                LOG_OUTPUT(L"Opening flyout using %s.", openMethod.ToString()->Data());
                FlyoutHelper::OpenFlyout(flyout, target, openMethod);

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Flyout opened. Expecting InputDevicePrefersPrimaryCommands to be %s.", expectedInputDevicePrefersPrimaryCommands ? L"true" : L"false");
                    VERIFY_ARE_EQUAL(expectedInputDevicePrefersPrimaryCommands, flyout->InputDevicePrefersPrimaryCommands);
                });

                LOG_OUTPUT(L"Closing flyout.");
                FlyoutHelper::HideFlyout(flyout);

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Flyout closed. Expecting InputDevicePrefersPrimaryCommands to be false.");
                    VERIFY_IS_FALSE(flyout->InputDevicePrefersPrimaryCommands);
                });
            };

        verifyOpenInputDevice(FlyoutOpenMethod::Mouse, false);
        verifyOpenInputDevice(FlyoutOpenMethod::Touch, true);
        verifyOpenInputDevice(FlyoutOpenMethod::Pen, true);
        verifyOpenInputDevice(FlyoutOpenMethod::Keyboard, false);
        verifyOpenInputDevice(FlyoutOpenMethod::Gamepad, false);
    }

    void FlyoutIntegrationTests::ValidateHyperlinkTakingFocusDoesNotCloseFlyout()
    {
        TestCleanupWrapper cleanup;

        auto target = FlyoutHelper::CreateTarget(
            300 /*width*/, 300 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Center);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::Flyout^ flyout = nullptr;
        xaml_docs::Hyperlink^ hyperlinkFocusTarget = nullptr;

        RunOnUIThread([&]()
        {
            auto richTextBlock = dynamic_cast<xaml_controls::RichTextBlock^> (xaml_markup::XamlReader::Load(
                LR"(<RichTextBlock xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Paragraph>
                            <Hyperlink x:Name="HyperlinkFocusTarget" NavigateUri="http://www.bing.com">Link text</Hyperlink>
                        </Paragraph>
                    </RichTextBlock>)"));

            hyperlinkFocusTarget = dynamic_cast<xaml_docs::Hyperlink^>(richTextBlock->FindName(L"HyperlinkFocusTarget"));

            flyout = ref new xaml_controls::Flyout();
            flyout->Content = richTextBlock;
        });

        LOG_OUTPUT(L"Open the flyout. The hyperlink should take focus and the flyout should stay open.");
        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Ensuring the flyout is open...");
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(target->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 1u);

            LOG_OUTPUT(L"Ensuring the hyperlink is the focused element...");
            VERIFY_IS_TRUE(hyperlinkFocusTarget->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
        });

        FlyoutHelper::HideFlyout(flyout);
        TestServices::WindowHelper->WaitForIdle();
    }
    void FlyoutIntegrationTests::ValidateThemeChangeWhileOpen()
    {
        // Verify that if a theme change comes in while the flyout is showing, it will be updated.

        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        const auto& u = TestServices::Utilities;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::Grid^ rootPanel = nullptr;
        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"      x:Name='root' Background='SlateBlue' Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                    L"  <Button x:Name='button' Content='button.flyout' HorizontalAlignment='Center' VerticalAlignment='Center' FontSize='25' > "
                    L"    <Button.Flyout> "
                    L"      <Flyout Placement='TopEdgeAlignedLeft'> "
                    L"        <TextBlock FontSize='30' Text='BUTTON.FLYOUT' Margin='30' Foreground='DarkRed' x:Name='button_flyout_content'/> "
                    L"      </Flyout> "
                    L"    </Button.Flyout> "
                    L"  </Button> "
                    L"</Grid>"));

                VERIFY_IS_NOT_NULL(rootPanel);
                TestServices::WindowHelper->WindowContent = rootPanel;

                button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);

                flyout = dynamic_cast<xaml_controls::Flyout^>(button->Flyout);
                VERIFY_IS_NOT_NULL(flyout);

                openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
                    {
                        LOG_OUTPUT(L"PopupOpenClose: Flyout Opened event is fired!");
                        flyoutOpenedEvent->Set();
                    }));

                closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
                    {
                        LOG_OUTPUT(L"PopupOpenClose: Flyout Closed event is fired!");
                        flyoutClosedEvent->Set();
                    }));

                // Ensure we start in the light theme.
                LOG_OUTPUT(L"Ensure Light Theme");
                rootPanel->RequestedTheme = xaml::ElementTheme::Light;

            });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Button Tap operation to show the Flyout.");
        TestServices::InputHelper->Tap(button);
        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Change the theme
        RunOnUIThread([&]()
           {
                LOG_OUTPUT(L"Change To Dark Theme");
                rootPanel->RequestedTheme = xaml::ElementTheme::Dark;
           });
        TestServices::WindowHelper->WaitForIdle();
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        // Close the flyout
        RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Close the Flyout");
                flyout->Hide();
            });
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

    }

    void FlyoutIntegrationTests::ValidateHideInOpeningCancelsOpening()
    {
        TestCleanupWrapper cleanup;

        auto target = FlyoutHelper::CreateTarget(
            300 /*width*/, 300 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Center);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::Flyout^ flyout = nullptr;
        auto flyoutOpeningEvent = std::make_shared<Event>();
        auto openingRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opening);

        RunOnUIThread([&]()
        {
            flyout = ref new xaml_controls::Flyout();

            openingRegistration.Attach(
                flyout,
                ref new wf::EventHandler<Platform::Object^>(
                    [flyout, flyoutOpeningEvent](Platform::Object^, Platform::Object^)
            {
                flyout->Hide();
                flyoutOpeningEvent->Set();
            }));

            LOG_OUTPUT(L"Open the flyout. It should not actually open because we call Hide() in the Opening event.");
            flyout->ShowAt(target);
        });

        flyoutOpeningEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Ensuring the flyout is not open...");
            VERIFY_ARE_EQUAL(0u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(target->XamlRoot)->Size);
        });
    }

    void FlyoutIntegrationTests::ValidateImmediateRetargeting()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Button^ button2 = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        auto flyoutOpenedEventCount = std::make_shared<int>();
        auto flyoutClosedEventCount = std::make_shared<int>();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Loading XAML.");
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='root' Background='SlateBlue' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left' Orientation='Horizontal'>"
                L"  <Button x:Name='button1' Content='button1' Margin='10' FontSize='25' VerticalAlignment='Bottom'/>"
                L"  <Button x:Name='button2' Content='button2' Margin='10' FontSize='25' VerticalAlignment='Bottom'/>"
                L"</StackPanel>"));

            VERIFY_IS_NOT_NULL(rootPanel);
            loadedRegistration.Attach(rootPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"StackPanel.Loaded event raised.");
                loadedEvent->Set();
            });

            LOG_OUTPUT(L"Setting WindowContent.");
            TestServices::WindowHelper->WindowContent = rootPanel;

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            button2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));

            VERIFY_IS_NOT_NULL(button1);
            VERIFY_IS_NOT_NULL(button2);

            LOG_OUTPUT(L"Setting up Flyout.Content.");
            auto flyoutContent = ref new xaml_controls::TextBlock;
            flyoutContent->Text = ref new Platform::String(L"FlyoutContent");
            flyoutContent->FontSize = 20;

            LOG_OUTPUT(L"Setting up Flyout.");
            flyout = ref new xaml_controls::Flyout;
            flyout->Content = flyoutContent;

            openedRegistration.Attach(
                flyout,
                ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent, flyoutOpenedEventCount](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"Flyout.Opened event raised.");
                (*flyoutOpenedEventCount)++;
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(
                flyout,
                ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent, flyoutClosedEventCount](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"Flyout.Closed event raised.");
                (*flyoutClosedEventCount)++;
                flyoutClosedEvent->Set();
            }));
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Opening flyout targeting button1.");
            flyout->ShowAt(button1);
            LOG_OUTPUT(L"Opening flyout targeting button2.");
            flyout->ShowAt(button2);
        });

        LOG_OUTPUT(L"Waiting for Opened event...");
        flyoutOpenedEvent->WaitForDefault();
        LOG_OUTPUT(L"Waiting for Closed event...");
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying event counts.");
            VERIFY_ARE_EQUAL(1, *flyoutOpenedEventCount);
            VERIFY_ARE_EQUAL(1, *flyoutClosedEventCount);
        });

        LOG_OUTPUT(L"Closing Flyout.");
        FlyoutHelper::HideFlyout(flyout);

        LOG_OUTPUT(L"Waiting for Closed event...");
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying event counts.");
            VERIFY_ARE_EQUAL(1, *flyoutOpenedEventCount);
            VERIFY_ARE_EQUAL(2, *flyoutClosedEventCount);
        });
    }

    void FlyoutIntegrationTests::VerifyContextFlyoutOnTextBlock()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBlock^ textBlockWithContextFlyout;
        xaml_controls::TextBlock^ textBlockWithoutContextFlyout;
        xaml_controls::TextBlock^ textBlockWithTextSelection;
        xaml_controls::Flyout^ flyout1;
        xaml_primitives::FlyoutBase^ defaultTextBlockContextFlyout;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        Event flyout1OpenedEvent;
        auto flyout1OpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);

        Event defaultFlyoutOpenedEvent;
        auto defaultFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);

        RunOnUIThread([&]()
        {
            auto root = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <TextBlock x:Name='textBlockWithContextFlyout' Text='TextBlock' >"
                L"        <TextBlock.ContextFlyout>"
                L"            <Flyout x:Name='flyout1' />"
                L"        </TextBlock.ContextFlyout>"
                L"    </TextBlock>"
                L"    <TextBlock x:Name='textBlockWithoutContextFlyout' Text='TextBlock' />"
                L"    <TextBlock x:Name='textBlockWithTextSelection' Text='TextBlock' IsTextSelectionEnabled='True' />"
                L"</StackPanel>"));

            textBlockWithContextFlyout = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"textBlockWithContextFlyout"));
            textBlockWithoutContextFlyout = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"textBlockWithoutContextFlyout"));
            textBlockWithTextSelection = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"textBlockWithTextSelection"));

            flyout1 = safe_cast<xaml_controls::Flyout^>(root->FindName(L"flyout1"));
            defaultTextBlockContextFlyout = textBlockWithoutContextFlyout->ContextFlyout;

            VERIFY_ARE_EQUAL(defaultTextBlockContextFlyout, textBlockWithTextSelection->ContextFlyout, L"TextBlocks should share same default ContextFlyout");

            flyout1OpenedRegistration.Attach(flyout1, [&flyout1OpenedEvent]() { flyout1OpenedEvent.Set(); });
            defaultFlyoutOpenedRegistration.Attach(defaultTextBlockContextFlyout, [&defaultFlyoutOpenedEvent]() { defaultFlyoutOpenedEvent.Set(); });

            loadedEventRegistration.Attach(root, [&]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();


        // Right clicking on a TextBlock with an explict ContextFlyout should show the flyout:
        LOG_OUTPUT(L"Right-click the TextBlock with a ContextFlyout");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, textBlockWithContextFlyout);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for Flyout to open");
        flyout1OpenedEvent.WaitForDefault();

        LOG_OUTPUT(L"Close the flyout.");
        FlyoutHelper::HideFlyout(flyout1);


        // Right-clicking on a TextBlock without an explict ContextFlyout should not show a flyout:
        LOG_OUTPUT(L"Right-click the TextBlock with no explict ContextFlyout");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, textBlockWithoutContextFlyout);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(defaultFlyoutOpenedEvent.HasFired(), L"Default flyout should not open");


        // A TextBlock with IsTextSelectionEnabled=True should show the default flyout:
        LOG_OUTPUT(L"Right-click the TextBlock with Text Selection enabled");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, textBlockWithTextSelection);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for Flyout to open");
        defaultFlyoutOpenedEvent.WaitForDefault();

        LOG_OUTPUT(L"Close the flyout.");
        FlyoutHelper::HideFlyout(defaultTextBlockContextFlyout);
    }

    void FlyoutIntegrationTests::VerifyFlyoutOpenedOffTheLeftSideOfTheScreenIsKeptInView()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Flyout^ flyout = nullptr;

        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            flyout = ref new xaml_controls::Flyout();
            flyout->Placement = xaml_primitives::FlyoutPlacementMode::LeftEdgeAlignedTop;

            auto rect = ref new xaml_shapes::Rectangle();
            rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            rect->Width = 100;
            rect->Height = 100;
            flyout->Content = rect;

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

            auto rootPanel = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootPanel;

            flyout->XamlRoot = rootPanel->XamlRoot;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Show the flyout.");

        RunOnUIThread([&]()
        {
            auto showOptions = ref new xaml_primitives::FlyoutShowOptions();
            showOptions->Position = wf::Point(20.0f, 20.0f);
            flyout->ShowAt(nullptr, showOptions);
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);

            LOG_OUTPUT(L"There should be one popup for the flyout.");
            VERIFY_ARE_EQUAL(1u, popups->Size);

            auto flyoutPopup = popups->GetAt(0);
            auto presenter = safe_cast<xaml::FrameworkElement^>(flyoutPopup->Child);
            auto flyoutBounds = ControlHelper::GetBounds(presenter);

            LOG_OUTPUT(L"Flyout bounds: left=%.0f top=%.0f width=%.0f height=%.0f", flyoutBounds.Left, flyoutBounds.Top, flyoutBounds.Width, flyoutBounds.Height);
            LOG_OUTPUT(L"The bounds' left position should be zero.");
            VERIFY_IS_TRUE(abs(flyoutBounds.Left) < 0.001f);
        });

        LOG_OUTPUT(L"Close the flyout.");
        FlyoutHelper::HideFlyout(flyout);

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void FlyoutIntegrationTests::SetupFlyoutPlacementModeTest(xaml_controls::Button^* button, xaml_controls::Flyout^* flyout)
    {
        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                            Background="SlateBlue" Width="300" Height="300" VerticalAlignment="Top" HorizontalAlignment="Left">
                        <Grid.Resources>
                            <Flyout x:Name="flyout">
                                <Flyout.FlyoutPresenterStyle>
                                    <Style TargetType="FlyoutPresenter">
                                        <Setter Property="BorderThickness" Value="0,0,0,0" />
                                        <Setter Property="Padding" Value="0,0,0,0" />
                                        <Setter Property="MinWidth" Value="0" />
                                        <Setter Property="MinHeight" Value="0" />
                                    </Style>
                                </Flyout.FlyoutPresenterStyle>
                                <Border Background="Green" Width="50" Height="50">
                                    <TextBlock Text="Flyout" />
                                </Border>
                            </Flyout>
                        </Grid.Resources>
                        <Rectangle Width="120" Height="120" Fill="Yellow" HorizontalAlignment="Center" VerticalAlignment="Center" />
                        <Button x:Name="button" Content="button" Width="100" Height="100" HorizontalAlignment="Center" VerticalAlignment="Center" />
                    </Grid>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            *button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            *flyout = dynamic_cast<xaml_controls::Flyout^>(rootPanel->FindName(L"flyout"));
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void FlyoutIntegrationTests::VerifyAppResourceOverrideContextFlyoutAcceleratorsPerfOptInOn()
    {
        // PerfOptIn=true is set via Data:PerfOptIn in the header.
        // WindowHelper::InitializeXamlCore reads it during TestSetup and sets the override.
        VerifyAppResourceOverrideContextFlyoutAcceleratorsHelper();
    }

    void FlyoutIntegrationTests::VerifyAppResourceOverrideContextFlyoutAcceleratorsPerfOptInOff()
    {
        // PerfOptIn=false is set via Data:PerfOptIn in the header.
        // WindowHelper::InitializeXamlCore reads it during TestSetup and sets the override.
        VerifyAppResourceOverrideContextFlyoutAcceleratorsHelper();
    }

    void FlyoutIntegrationTests::VerifyAppResourceOverrideContextFlyoutAcceleratorsHelper()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ root;
        xaml_controls::TextBlock^ textBlock;
        xaml_controls::Button^ focusButton;
        xaml_controls::MenuFlyout^ customFlyout;
        xaml_controls::MenuFlyoutItem^ flyoutItem;
        xaml_input::KeyboardAccelerator^ accelerator;

        Event clickEvent;
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

        RunOnUIThread([&]()
        {
            // Create a MenuFlyout with two items. The keyboard accelerator (Ctrl+2) is on the
            // SECOND item, not the first. This is important: when the flyout opens, the first
            // item gets focus and ProcessLocalAccelerators checks accelerators on the focused
            // element. By putting the accelerator on the second item, we ensure it can only be
            // found via the global live accelerator collection (AddToLiveKeyboardAccelerators),
            // which exposes the visualTree=nullptr / wrong-ContentRoot bug.
            customFlyout = ref new xaml_controls::MenuFlyout();

            auto dummyItem = ref new xaml_controls::MenuFlyoutItem();
            dummyItem->Text = L"Dummy First Item";
            customFlyout->Items->Append(dummyItem);

            flyoutItem = ref new xaml_controls::MenuFlyoutItem();
            flyoutItem->Text = L"Custom Action";

            accelerator = ref new xaml_input::KeyboardAccelerator();
            accelerator->Key = ::Windows::System::VirtualKey::Number2;
            accelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Control;
            flyoutItem->KeyboardAccelerators->Append(accelerator);

            customFlyout->Items->Append(flyoutItem);

            // Override the default TextControlCommandBarContextFlyout in Application.Resources.
            xaml::Application::Current->Resources->Insert(L"TextControlCommandBarContextFlyout", customFlyout);

            root = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <TextBlock x:Name='textBlock' Text='Hello World' IsTextSelectionEnabled='True' />"
                L"    <Button x:Name='focusButton' Content='FocusButton' />"
                L"</StackPanel>"));

            textBlock = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"textBlock"));
            focusButton = safe_cast<xaml_controls::Button^>(root->FindName(L"focusButton"));

            // Verify the TextBlock picks up our custom flyout as its default ContextFlyout.
            auto contextFlyout = textBlock->ContextFlyout;
            VERIFY_IS_NOT_NULL(contextFlyout, L"TextBlock should have a ContextFlyout");
            VERIFY_ARE_EQUAL(customFlyout, contextFlyout, L"TextBlock should use our custom flyout from Application.Resources");

            clickRegistration.Attach(flyoutItem, [&clickEvent]() {
                LOG_OUTPUT(L"MenuFlyoutItem Click event fired!");

                clickEvent.Set(); 
            });

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Focus the button so keyboard input is routed into the XAML tree.
        RunOnUIThread([&]()
        {
            focusButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Before opening the flyout, the accelerator does NOT fire. This is a known limitation
        // (https://github.com/microsoft/microsoft-ui-xaml/issues/11025):
        // CUIElement::EnterImpl passes visualTree=nullptr when entering the
        // ContextFlyout, so accelerators are registered on the vestigial CoreWindow ContentRoot
        // instead of the island's ContentRoot where keyboard input is actually processed.
        // See docs/design-notes/context-flyout.md ("The visualTree=nullptr Problem").
        LOG_OUTPUT(L"==> Press accelerator sequence before flyout has been opened: Ctrl + 2 (expect no invocation)");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_2#$u$_2#$u$_ctrlscan");
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(clickEvent.HasFired(), L"Accelerator should not fire when flyout has never been opened - accelerators are on the wrong ContentRoot (known limitation)");

        // Right click on textBlock to open the ContextFlyout, then try the accelerator while
        // the flyout is open. The accelerator is on the second MenuFlyoutItem, which does NOT
        // have focus (the first item gets focus). ProcessLocalAccelerators won't find it, and
        // the global collection has it on the wrong ContentRoot, so it still doesn't fire.
        LOG_OUTPUT(L"==> Right-click the TextBlock to open the ContextFlyout.");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, textBlock);
        TestServices::WindowHelper->WaitForIdle();

        clickEvent.Reset();
        LOG_OUTPUT(L"==> Press accelerator sequence while flyout is open: Ctrl + 2 (expect no invocation)");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_2#$u$_2#$u$_ctrlscan");
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(clickEvent.HasFired(), L"Accelerator should not fire even with flyout open - it's on the second item (not focused) and global registration is on wrong ContentRoot");

        // Clean up: remove the override from Application.Resources.
        RunOnUIThread([&]()
        {
            xaml::Application::Current->Resources->Remove(L"TextControlCommandBarContextFlyout");
        });
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Flyout

