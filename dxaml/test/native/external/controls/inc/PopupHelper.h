// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class PopupHelper
    {
    public:
        static void OpenPopup(xaml_primitives::Popup^ popup)
        {
            std::shared_ptr<Event> openedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);

            RunOnUIThread([&]()
            {
                openedRegistration.Attach(popup, [&](){ openedEvent->Set(); });

                popup->IsOpen = true;
            });

            openedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        static bool AreWindowedPopupsEnabled()
        {
            return TestServices::Utilities->IsDesktop;
        }

        static void WaitForOpenPopup(UIElement ^element)
        {
            const int retries = 20;

            for (int i = 0; i < retries; i++)
            {
                bool popupWasFound = false;
                auto checkedForPopupEvent = std::make_shared<Event>();

                RunOnUIThread([&]()
                {
                    auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(element->XamlRoot);
                    if (popups->Size > 0)
                    {
                        popupWasFound = true;
                    }

                    checkedForPopupEvent->Set();
                });

                checkedForPopupEvent->WaitForDefault();

                if (popupWasFound)
                {
                    break;
                }
                else
                {
                    if (i < retries - 1)
                    {
                        LOG_OUTPUT(L"TimePicker popup not yet found. Retrying...");
                    }
                    else
                    {
                        VERIFY_FAIL(L"TimePicker popup never opened.");
                    }
                }
            }
        }
    };

} } } } }

