// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "XamlTailored.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class ApplicationThemeOverrider final
    {
        Microsoft::UI::Xaml::ApplicationTheme m_previousApplicationTheme;

    public:
        ApplicationThemeOverrider()
        {
            RunOnUIThread([&] {
                m_previousApplicationTheme = Application::Current->RequestedTheme;
                test_infra::TestServices::ThemingHelper->UnsetApplicationRequestedTheme();
            });
        }

        explicit ApplicationThemeOverrider(Microsoft::UI::Xaml::ApplicationTheme requestedTeme)
        {
            RunOnUIThread([&] {
                m_previousApplicationTheme = Application::Current->RequestedTheme;
                test_infra::TestServices::ThemingHelper->SetApplicationRequestedTheme(requestedTeme);
            });
        }

        ~ApplicationThemeOverrider()
        {
            RunOnUIThread([&] {
                test_infra::TestServices::ThemingHelper->SetApplicationRequestedTheme(m_previousApplicationTheme);
            });
        }

        // Disallow copying/moving
        ApplicationThemeOverrider(ApplicationThemeOverrider&& other) = delete;
        ApplicationThemeOverrider(const ApplicationThemeOverrider&) = delete;
        ApplicationThemeOverrider& operator=(const ApplicationThemeOverrider&) = delete;
        ApplicationThemeOverrider& operator=(ApplicationThemeOverrider&&) = delete;
    };

} } } } }
