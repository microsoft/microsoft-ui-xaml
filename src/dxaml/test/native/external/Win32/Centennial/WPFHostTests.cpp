// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include "WPFHostTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>

#include <ppltasks.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <vector>
#include <AsyncOperationWaiter.h>
#include <wil/result_macros.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

using namespace test_infra;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace WEX::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Centennial {

        bool WPFHostTests::ClassSetup()
        {
            return true;
        }

        bool WPFHostTests::TestSetup()
        {
            return true;
        }

        bool WPFHostTests::TestCleanup()
        {
            return true;
        }

        void WPFHostTests::Canary()
        {
            auto hostingSetupHelper = ref new test_infra::Hosting::HostingHelpers::HostingSetupHelper();
            hostingSetupHelper->InitializeWPFHostFactory();

            auto factory = test_infra::TestHostSettings::Win32HostFactory;
            VERIFY_IS_NOT_NULL(factory);
        }
    }
} } } }
