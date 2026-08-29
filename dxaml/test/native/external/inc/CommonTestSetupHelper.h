// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class CommonTestSetupHelper
    {
    public:
        static void CommonTestClassSetup()
        {
            if(!_isInitialized)
            {
                if(IsInWPFHostingMode())
                {
                    ConfigureWin32Host();
                }
                _isInitialized = true;
            }
            test_infra::TestServices::EnsureInitialized();
        }
    private:
        inline static bool _isInitialized = false;

        static bool IsInWPFHostingMode()
        {
            WEX::Common::String hostingMode;
            WEX::TestExecution::RuntimeParameters::TryGetValue(L"HostingMode", hostingMode);
            return hostingMode.CompareNoCase(L"WPF") == 0;
        }

        static void ConfigureWin32Host()
        {
            auto hostingSetupHelper = ref new test_infra::Hosting::HostingHelpers::HostingSetupHelper();
            hostingSetupHelper->InitializeWPFHostFactory();
        }
    };


} } } } }