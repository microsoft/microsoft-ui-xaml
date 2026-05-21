// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PasswordBoxIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TextControlHelper.h>

using namespace Platform;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace PasswordBox {

    bool PasswordBoxIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PasswordBoxIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PasswordBoxIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void PasswordBoxIntegrationTests::VerifySelectingTextWithTouchShowsSelectionFlyout()
    {
        TextControlHelper::VerifySelectingTextWithTouchShowsSelectionFlyout<xaml_controls::PasswordBox>(
            [](xaml_controls::PasswordBox^ passwordBox) { passwordBox->Password = "aaaaaaaaaa"; });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::PasswordBox
