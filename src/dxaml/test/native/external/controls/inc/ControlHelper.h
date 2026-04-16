// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

#include <TreeHelper.h>
#include <ValidateTreeParams.h>

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class ControlHelper
    {
    public:
        template<typename TTargetControl>
        static void DoClickUsingTap(TTargetControl^ button)
        {
            std::shared_ptr<Event> clickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(TTargetControl, Click);

            RunOnUIThread([&]()
            {
                clickRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler([clickEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    clickEvent->Set();
                }));
            });

            test_infra::TestServices::InputHelper->Tap(button);
            clickEvent->WaitForDefault();
        }

        template<typename TTargetControl>
        static void DoClickUsingAP(TTargetControl^ button)
        {
            std::shared_ptr<Event> clickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(TTargetControl, Click);

            RunOnUIThread([&]()
            {
                clickRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler([clickEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    clickEvent->Set();
                }));

                auto buttonAP = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(
                    xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button)
                    );

                buttonAP->Invoke();
            });

            clickEvent->WaitForDefault();
        }

        static void ClickFlyoutCloseButton(xaml::DependencyObject^ element, bool isAccept)
        {
            // The Flyout close button could either be a part of the Flyout itself or it could be in the AppBar
            // We look in both places.
            xaml_primitives::ButtonBase^ button = nullptr;

            RunOnUIThread([&]()
            {
                Platform::String^ buttonName = isAccept ? L"AcceptButton" : L"DismissButton";

                button = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByNameFromOpenPopups(buttonName, element));

                if (!button)
                {
                    auto xamlRoot = TreeHelper::GetXamlRoot(element);
                    auto cmdBar = test_infra::TestServices::Utilities->RetrieveOpenBottomCommandBar(xamlRoot);
                    if (cmdBar != nullptr)
                    {
                        button = safe_cast<xaml_primitives::ButtonBase^>(cmdBar->PrimaryCommands->GetAt(isAccept ? 0 : 1));
                    }
                }
            });

            VERIFY_IS_NOT_NULL(button);

            DoClickUsingAP(button);
        }

        static wf::Rect GetBounds(FrameworkElement^ element)
        {
            wf::Rect rect;
            RunOnUIThread([&]()
            {
                auto point1 = element->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
                auto point2 = element->TransformToVisual(nullptr)->TransformPoint(wf::Point((float)element->ActualWidth, (float)element->ActualHeight));

                rect.X = min(point1.X, point2.X);
                rect.Y = min(point1.Y, point2.Y);
                rect.Width = abs(point1.X - point2.X);
                rect.Height = abs(point2.Y - point1.Y);
            });

            return rect;
        }

        static bool IsContainedIn(wf::Rect inner, wf::Rect outer)
        {
            auto outerRight = outer.X + outer.Width;
            auto outerBottom = outer.Y + outer.Height;
            auto innerRight = inner.X + inner.Width;
            auto innerBottom = inner.Y + inner.Height;

            return outer.X <= inner.X
                && outer.Y <= inner.Y
                && outerRight >= innerRight
                && outerBottom >= innerBottom;
        }

        static bool AreClose(wf::Point point1, wf::Point point2, float tolerance)
        {
            return std::abs(point1.X - point2.X) <= tolerance
                && std::abs(point1.Y - point2.Y) <= tolerance;
        }

        static bool AreClose(float x, float y, float tolerance)
        {
            return std::abs(x - y) <= tolerance;
        };

        template<typename T1, typename T2>
        static void RemoveItem(wfc::IVector<T1>^ items, T2 item)
        {
            unsigned int index;
            WEX::Common::Throw::IfFalse(items->IndexOf(item, &index), E_INVALIDARG, L"The item was not in the collection.");
            items->RemoveAt(index);
        }

        static bool IsInVisualState(xaml_controls::Control^ control, Platform::String^ visualStateGroupName, Platform::String^ visualStateName)
        {
            bool result = false;
            RunOnUIThread([&]()
            {
                auto rootVisual = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(control, 0));
                auto visualStateGroup = safe_cast<xaml::VisualStateGroup^>(rootVisual->FindName(visualStateGroupName));
                result = visualStateName == visualStateGroup->CurrentState->Name;
            });
            return result;
        }

        static xaml::VisualStateGroup^ GetVisualStateGroup(xaml_controls::Control^ control, Platform::String^ visualStateGroupName)
        {
            xaml::VisualStateGroup^ result = nullptr;
            RunOnUIThread([&]()
            {
                auto rootVisual = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(control, 0));
                result = safe_cast<xaml::VisualStateGroup^>(rootVisual->FindName(visualStateGroupName));
            });
            return result;
        }

        static void ValidateUIElementTree(
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc,
            std::function<void()> cleanupFunc = nullptr,
            bool disableHittestingOnRoot = true,
            bool ignorePopups = false
            )
        {
            ValidateUIElementTree(
                ValidateTreeParams(windowSizeOverride, scaleFactorOverride, setupFunc, cleanupFunc, disableHittestingOnRoot, ignorePopups)
                );
        }

        static void ValidateUIElementTree(
            Platform::String^ subTestId,
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc,
            std::function<void()> cleanupFunc = nullptr,
            bool disableHittestingOnRoot = true,
            bool ignorePopups = false
            )
        {
            ValidateUIElementTree(
                ValidateTreeParams(subTestId, windowSizeOverride, scaleFactorOverride, setupFunc, cleanupFunc, disableHittestingOnRoot, ignorePopups)
                );
        }

        static void ValidateUIElementTree(const ValidateTreeParams& params)
        {
            TestCleanupWrapper cleanup(
                [&params]()
                {
                    if (params.CleanupFunc != nullptr)
                    {
                        params.CleanupFunc();
                    }
                    test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            auto darkTest = (params.SubTestId != nullptr && !params.SubTestId->IsEmpty() ? params.SubTestId + L"." : L"") + L"Dark";
            auto lightTest = (params.SubTestId != nullptr && !params.SubTestId->IsEmpty() ? params.SubTestId + L"." : L"") + L"Light";
            auto hcTest = (params.SubTestId != nullptr && !params.SubTestId->IsEmpty() ? params.SubTestId + L"." : L"") + L"HC";

            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(params.WindowSizeOverride, params.ScaleFactorOverride);

            // Move the mouse to the top-left corner so it does not interfere with the Visual States.
            // Note that we are using ClickMouseButton() instead of just MoveMouse() because the latter was resulting in a pointer artifact for OneCore instances.
            TestServices::InputHelper->ClickMouseButton(MouseButton::Middle, wf::Point(0, 0));

            auto root = params.SetupFunc();

            if (params.DisableHittestingOnRoot)
            {
                // To prevent any possiblity of mouse movement interfering with the visual tree, we set IsHitTestVisible to false on the root.
                // This helps in case the mouse moves during the test but you may still need to set IsHitTestVisible on the root in the SetupFunc
                // to prevent issues due to pointer replay.
                RunOnUIThread([&]()
                {
                    root->IsHitTestVisible = false;
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            LOG_OUTPUT(L"Validate the dark theme of controls.");
            TestServices::Utilities->VerifyUIElementTreeWithRulesInline(darkTest, params.ValidationRules, params.IgnorePopups);

            LOG_OUTPUT(L"Validate the light theme of controls.");
            RunOnUIThread([&]()
            {
                root->RequestedTheme = xaml::ElementTheme::Light;
                root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::White);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTreeWithRulesInline(lightTest, params.ValidationRules, params.IgnorePopups);

            LOG_OUTPUT(L"Validate the high-contrast theme of controls.");
            ValidateUIElementTreeForHighContrast(hcTest, root, params.ValidationRules, params.SkipHCColorValidationOnMasterMismatch, params.IgnorePopups);
        }

        // Some controls have limitations that prevent them from using the common ControlsHelper::ValidateUIElementTree()
        // method so this method has been factored out to at least allow them to use common high-contrast validation.
        // New tests should try to use the common method rather than calling this one directly.
        static void ValidateUIElementTreeForHighContrast(
            Platform::String^ subTestId,
            xaml_controls::Panel^ root,
            Platform::String^ validationRules,
            bool skipHCColorValidationOnMasterMismatch = false,
            bool ignorePopups = false
            )
        {
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
                root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Black);
            });
            TestServices::WindowHelper->WaitForIdle();
            bool result = TestServices::Utilities->VerifyUIElementTreeWithRulesInline(subTestId, validationRules, ignorePopups);

            // Make sure all the colors in the output file are high-contrast colors.
            if (!skipHCColorValidationOnMasterMismatch)
            {
                LOG_OUTPUT(L"Validate High-Contrast Colors.");
                TestServices::Utilities->VerifyOutputFileHighContrastColors(subTestId, HighContrastTheme::Test);
            }

            if (!result)
            {
                WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogFailuresAsWarnings);

                // If high-contrast validation failed, then kick off placeholder validation
                // for our other high-contrast modes so that they can be visually
                // verified.
                LOG_OUTPUT(L"High-Contrast baseline validation failed. Performing dummy visual validation to \r\n verify that control updates look good in a few of the default high-contrast themes.");

                // Visual verification for HighContrast #1 palette.
                RunOnUIThread([&]() { TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Default; });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyUIElementTreeWithRulesInline(L"HC_DEFAULT.DO_NOT_SUBMIT", validationRules, ignorePopups);

                // Visual verification for HighContrast Black palette.
                RunOnUIThread([&]() { TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black; });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyUIElementTreeWithRulesInline(L"HC_BLACK.DO_NOT_SUBMIT", validationRules, ignorePopups);

                // Visual verification for HighContrast White palette.
                RunOnUIThread([&]()
                {
                    TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::White;
                    root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::White);
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyUIElementTreeWithRulesInline(L"HC_WHITE.DO_NOT_SUBMIT", validationRules, ignorePopups);
            }
        }

        static wf::Point GetCenterOfElement(xaml::FrameworkElement^ element)
        {
            wf::Point offsetFromCenter = {};

            return GetOffsetCenterOfElement(element, offsetFromCenter);
        }

        static wf::Point GetOffsetCenterOfElement(xaml::FrameworkElement^ element, wf::Point offsetFromCenter)
        {
            wf::Point result;
            RunOnUIThread([&]()
            {
                auto offsetCenterLocal = wf::Point(static_cast<float>(element->ActualWidth / 2) + offsetFromCenter.X, static_cast<float>(element->ActualHeight / 2) + offsetFromCenter.Y);
                result = element->TransformToVisual(nullptr)->TransformPoint(offsetCenterLocal);
            });
            return result;
        }

        static void EnsureFocused(xaml_controls::Control^ control)
        {
            Event hasFocus;
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Control, GotFocus);
            gotFocusRegistration.Attach(control, [&](){ hasFocus.Set(); });

            RunOnUIThread([&]()
            {
                if (control->FocusState == xaml::FocusState::Unfocused)
                {
                    control->Focus(xaml::FocusState::Programmatic);
                }
                else
                {
                    hasFocus.Set();
                }
            });

            hasFocus.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }
    };

} } } } }
