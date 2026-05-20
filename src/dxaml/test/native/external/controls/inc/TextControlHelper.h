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

    class TextControlHelper
    {
    public:
        template<typename TTextControl>
        static void VerifySelectingTextWithTouchShowsSelectionFlyout(
            std::function<void(TTextControl^)> const& initializeControlFunc)
        {
            TestCleanupWrapper cleanup;

            TTextControl^ textControl = nullptr;

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(TTextControl, Loaded);

            RunOnUIThread([&]()
            {
                auto rootStackPanel = ref new xaml_controls::StackPanel();

                textControl = ref new TTextControl();
                initializeControlFunc(textControl);
                textControl->Width = 200;

                // We'll give the control a big margin to ensure that there's space for the SelectionFlyout to be shown.
                textControl->Margin = xaml::Thickness({ 100, 100, 100, 100 });

                loadedRegistration.Attach(textControl, [loadedEvent]() { loadedEvent->Set(); });

                rootStackPanel->Children->Append(textControl);
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });

            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            auto flyoutOpenedEvent = std::make_shared<Event>();
            auto flyoutClosedEvent = std::make_shared<Event>();
            auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
            auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

            RunOnUIThread([&]()
            {
                flyoutOpenedRegistration.Attach(textControl->SelectionFlyout, [flyoutOpenedEvent]() { flyoutOpenedEvent->Set(); });
                flyoutClosedRegistration.Attach(textControl->SelectionFlyout, [flyoutClosedEvent]() { flyoutClosedEvent->Set(); });

                // We'll set the clipboard to always contain text,
                // in order to ensure that the paste button always shows.
                // This makes a difference because it's the only button
                // to be shown in the case of PasswordBox,
                // and the SelectionFlyout won't show without it.
                auto dataPackage = ref new ::Windows::ApplicationModel::DataTransfer::DataPackage();

                dataPackage->RequestedOperation = ::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy;
                dataPackage->SetText("aaaaaaaaaa");

                ::Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(dataPackage);
            });

            constexpr bool isTextEditControl =
                std::is_same<xaml_controls::TextBox, TTextControl>::value ||
                std::is_same<xaml_controls::RichEditBox, TTextControl>::value ||
                std::is_same<xaml_controls::PasswordBox, TTextControl>::value;

            // The first tap on a text edit control places the caret,
            // which should not cause the SelectionFlyout to open.
            // On the other hand, a text display control only has selection,
            // so the first tap will select the word and should cause
            // the SelectionFlyout to open.
            if (isTextEditControl)
            {
                LOG_OUTPUT(L"Tap once to place the caret.  The SelectionFlyout should not open.");
                TestServices::InputHelper->Tap(textControl, 0.25f, 0.5f);

                TestServices::WindowHelper->WaitForIdle();
                VERIFY_IS_FALSE(flyoutOpenedEvent->HasFired());

                LOG_OUTPUT(L"Tap again to select the word.  The SelectionFlyout should now open.");
                TestServices::InputHelper->Tap(textControl, 0.25f, 0.5f);
            }
            else
            {
                LOG_OUTPUT(L"Tap to select the word.  The SelectionFlyout should open.");
                TestServices::InputHelper->Tap(textControl, 0.25f, 0.5f);
            }

            flyoutOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            xaml_shapes::Rectangle^ gripperRect = nullptr;

            RunOnUIThread([&]()
            {
                for (auto popup : xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                    TestServices::WindowHelper->WindowContent->XamlRoot))
                {
                    auto gripperCanvas = dynamic_cast<xaml_controls::Canvas^>(popup->Child);

                    if (gripperCanvas != nullptr)
                    {
                        gripperRect = TreeHelper::GetVisualChildByType<xaml_shapes::Rectangle>(gripperCanvas);
                        if (gripperRect != nullptr)
                        {
                            break;
                        }
                    }
                }
            });

            VERIFY_IS_NOT_NULL(gripperRect);

            LOG_OUTPUT(L"Pan the left gripper to the right. This should hide, and then re-show, the SelectionFlyout.");
            TestServices::InputHelper->PanFromCenter(gripperRect, 100 /*relX*/, 0 /*relY*/, 0.1 /*velocityFactor*/);

            flyoutClosedEvent->WaitForDefault();
            flyoutOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Tap well below the control to close the SelectionFlyout.");
            TestServices::InputHelper->Tap(textControl, 0.5f, 3.0f);

            flyoutClosedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }
    };

} } } } }
