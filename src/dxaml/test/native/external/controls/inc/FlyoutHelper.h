// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

#include <TreeHelper.h>
#include <PopupHelper.h>

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    public enum class FlyoutOpenMethod
    {
        Mouse,
        Touch,
        Pen,
        Keyboard,
        Gamepad,
        Programmatic_ShowAt,
        Programmatic_ShowAttachedFlyout
    };

    class FlyoutHelper
    {
    public:
        static xaml_controls::Flyout^ CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode placement)
        {
            xaml_controls::Flyout^ flyout = nullptr;

            RunOnUIThread([&]()
            {
                auto content = ref new xaml_controls::StackPanel();
                content->Width = 300;

                auto button = ref new xaml_controls::Button();
                button->Margin = xaml::ThicknessHelper::FromUniformLength(12);
                button->FontSize = 25;
                button->FontWeight = Microsoft::UI::Text::FontWeights::Thin;
                button->HorizontalAlignment = xaml::HorizontalAlignment::Stretch;
                button->Content = "button";

                auto textbox = ref new xaml_controls::TextBox();
                textbox->Margin = xaml::ThicknessHelper::FromUniformLength(12);
                textbox->Height = 225;
                textbox->HorizontalAlignment = xaml::HorizontalAlignment::Stretch;
                button->FontSize = 18;
                button->FontWeight = Microsoft::UI::Text::FontWeights::Bold;
                textbox->Text = "textbox";

                auto checkbox = ref new xaml_controls::CheckBox();
                checkbox->Margin = xaml::ThicknessHelper::FromUniformLength(12);
                checkbox->FontSize = 25;
                checkbox->FontWeight = Microsoft::UI::Text::FontWeights::Thin;
                checkbox->HorizontalAlignment = Microsoft::UI::Xaml::HorizontalAlignment::Stretch;
                checkbox->Content = "checkbox";

                content->Children->Append(button);
                content->Children->Append(textbox);
                content->Children->Append(checkbox);

                wxaml_interop::TypeName type = wxaml_interop::TypeName();
                type.Name = "Microsoft.UI.Xaml.Controls.FlyoutPresenter";
                type.Kind = wxaml_interop::TypeKind::Metadata;

                auto style = ref new xaml::Style(type);
                style->Setters->Append(ref  new xaml::Setter(xaml_controls::FlyoutPresenter::TagProperty, "presenter_style"));

                flyout = ref new xaml_controls::Flyout();
                flyout->Placement = placement;
                flyout->Content = content;
                flyout->FlyoutPresenterStyle = style;
            });
            return flyout;
        }

        static xaml_controls::FlyoutPresenter^ GetFlyoutPresenter(xaml_controls::Flyout^ flyout)
        {
            VERIFY_IS_NOT_NULL(flyout->Content);
            return TreeHelper::FindAncestor<xaml_controls::FlyoutPresenter^>(flyout->Content);
        }

        // Create target for Flyout or MenuFlyout.
        static xaml_controls::Border^ CreateTarget(
            double width,
            double height,
            Thickness margin,
            xaml::HorizontalAlignment halign,
            xaml::VerticalAlignment valign)
        {
            xaml_controls::Border^ target = nullptr;
            RunOnUIThread([&]()
            {
                target = ref new xaml_controls::Border();
                target->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::RoyalBlue);
                target->Height = height;
                target->Width = width;
                target->Margin = margin;
                target->HorizontalAlignment = halign;
                target->VerticalAlignment = valign;
            });
            return target;
        }

        template <class TFlyoutControl>
        static void OpenFlyout(
            TFlyoutControl^ flyoutControl,
            FrameworkElement^ target,
            FlyoutOpenMethod openMethod)
        {
            Event openingEvent;
            auto openingRegistration = CreateSafeEventRegistration(TFlyoutControl, Opening);
            openingRegistration.Attach(flyoutControl, [&]() { openingEvent.Set(); });

            Event openedEvent;
            auto openedRegistration = CreateSafeEventRegistration(TFlyoutControl, Opened);
            openedRegistration.Attach(flyoutControl, [&]() { openedEvent.Set(); });

            if (openMethod == FlyoutOpenMethod::Mouse)
            {
                TestServices::InputHelper->LeftMouseClick(target);
                TestServices::WindowHelper->WaitForIdle();
                // Wait for the sub menu to open. It opens after a delay - clicking and waiting for idle doesn't open it.
                TestServices::WindowHelper->SynchronouslyTickUIThread(60);
            }
            else if (openMethod == FlyoutOpenMethod::Touch)
            {
                TestServices::InputHelper->Tap(target);
                TestServices::WindowHelper->WaitForIdle();
            }
            else if (openMethod == FlyoutOpenMethod::Pen)
            {
                TestServices::InputHelper->PenHold(target);
                TestServices::WindowHelper->WaitForIdle();
                TestServices::WindowHelper->SynchronouslyTickUIThread(60);
            }
            else if (openMethod == FlyoutOpenMethod::Keyboard)
            {
                RunOnUIThread([&]()
                {
                    safe_cast<xaml_controls::Control^>(target)->Focus(xaml::FocusState::Keyboard);
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::KeyboardHelper->PressKeySequence(L" ");
                TestServices::WindowHelper->WaitForIdle();
            }
            else if (openMethod == FlyoutOpenMethod::Gamepad)
            {
                RunOnUIThread([&]()
                {
                    safe_cast<xaml_controls::Control^>(target)->Focus(xaml::FocusState::Keyboard);
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::KeyboardHelper->GamepadA();
                TestServices::WindowHelper->WaitForIdle();
            }
            else if (openMethod == FlyoutOpenMethod::Programmatic_ShowAt)
            {
                RunOnUIThread([&]()
                {
                    flyoutControl->ShowAt(target);
                });
                TestServices::WindowHelper->WaitForIdle();
            }
            else if (openMethod == FlyoutOpenMethod::Programmatic_ShowAttachedFlyout)
            {
                RunOnUIThread([&]()
                {
                    xaml_primitives::FlyoutBase::ShowAttachedFlyout(target);
                });
                TestServices::WindowHelper->WaitForIdle();
            }
            openingEvent.WaitForDefault();
            openedEvent.WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();
        }

        template <class TFlyoutControl>
        static void ShowFlyoutWithOptions(
            TFlyoutControl^ flyoutControl,
            xaml::DependencyObject^ targetElement,
            xaml_primitives::FlyoutShowOptions^ showOptions)
        {
            Event openingEvent;
            auto openingRegistration = CreateSafeEventRegistration(TFlyoutControl, Opening);
            openingRegistration.Attach(flyoutControl, [&]() { openingEvent.Set(); });

            Event openedEvent;
            auto openedRegistration = CreateSafeEventRegistration(TFlyoutControl, Opened);
            openedRegistration.Attach(flyoutControl, [&]() { openedEvent.Set(); });

            RunOnUIThread([&]()
            {
                flyoutControl->ShowAt(targetElement, showOptions);
            });

            openingEvent.WaitForDefault();
            openedEvent.WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();
        }

        template <class TFlyoutControl>
        static void HideFlyout(TFlyoutControl^ flyoutControl)
        {
            Event closedEvent;
            auto closedRegistration = CreateSafeEventRegistration(TFlyoutControl, Closed);
            closedRegistration.Attach(flyoutControl, [&]() { closedEvent.Set(); });

            RunOnUIThread([&]()
            {
                flyoutControl->Hide();
            });

            closedEvent.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        static xaml::FrameworkElement^ GetOpenFlyoutPresenter()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(test_infra::TestServices::WindowHelper->WindowContent->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1, E_FAIL, L"Expected exactly one open Popup.");

            auto flyoutPopup = popups->GetAt(0);
            WEX::Common::Throw::If(flyoutPopup->Child == nullptr, E_FAIL, L"Popup child should not be null.");

            return safe_cast<xaml::FrameworkElement^>(flyoutPopup->Child);
        }

        static xaml::FrameworkElement^ GetOpenFlyoutOverlayElement()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                test_infra::TestServices::WindowHelper->WindowContent->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1, E_FAIL, L"Expected exactly one open Popup.");

            auto flyoutPopup = popups->GetAt(0);
            return test_infra::TestServices::Utilities->GetPopupOverlayElement(flyoutPopup);
        }

        template <class TFlyoutControl>
        static void ValidateOverlayBrush(Platform::String^ expectedBrushKey)
        {
            TFlyoutControl^ flyout = nullptr;
            xaml_controls::Button^ target = nullptr;

            RunOnUIThread([&]()
            {
                flyout = ref new TFlyoutControl();
                flyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

                target = ref new xaml_controls::Button();

                TestServices::WindowHelper->WindowContent = target;
            });

            TestServices::WindowHelper->WaitForIdle();

            FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

            ValidateOpenFlyoutOverlayBrush(expectedBrushKey);

            FlyoutHelper::HideFlyout(flyout);

            RunOnUIThread([&]()
            {
                // If we don't release our refs on flyout and target here, we hit an assert due to calling  off-thread.
                // The destructor calls TrackerPtr release off the UI thread, which triggers an exception during cleanup.
                flyout = nullptr;
                target = nullptr;
            });
        }

        static void ValidateOpenFlyoutOverlayBrush(Platform::String^ expectedBrushKey)
        {
            RunOnUIThread([&]()
            {
                auto expectedBrush = safe_cast<xaml_media::SolidColorBrush^>(xaml::Application::Current->Resources->Lookup(expectedBrushKey));

                auto overlayElement = FlyoutHelper::GetOpenFlyoutOverlayElement();
                WEX::Common::Throw::IfNull(reinterpret_cast<void*>(overlayElement), L"An overlay element should exist for the flyout.");

                auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
                WEX::Common::Throw::IfNull(reinterpret_cast<void*>(overlayRect), L"The overlay element should be a rectangle.");

                auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
                VERIFY_IS_NOT_NULL(overlayBrush);
                VERIFY_IS_TRUE(overlayBrush->Equals(expectedBrush));
            });
        }

        static HWND GetUIAHandleToUseForWindowedFlyout(xaml_primitives::FlyoutBase^ flyout)
        {
            xaml_controls::MenuFlyout^ menuFlyout = dynamic_cast<xaml_controls::MenuFlyout^>(flyout);
            HWND result = nullptr;

            if (PopupHelper::AreWindowedPopupsEnabled() || menuFlyout != nullptr)
            {
                // On Desktop, UIA uses the HWND that contains the UI Element as a handle.
                uint64_t windowHandle = TestServices::WindowHelper->FlyoutBase_GetWindow(flyout);
                result = reinterpret_cast<HWND>(windowHandle);
            }

            return result;
        }
    };

} } } } }
