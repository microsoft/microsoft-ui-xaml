// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PeerIntegrationTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <Utils.h>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Peers {

bool PeerIntegrationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

        bool PeerIntegrationTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

bool PeerIntegrationTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

// Validates that parent peer is not unnecessarily created.
void PeerIntegrationTests::ParentPeerNotCreated()
{
    TestCleanupWrapper cleanup;
    Grid^ rootGrid = nullptr;
    Border^ border = nullptr;
    Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Load Xaml and check that button's parent border's peer has not been created");
        rootGrid = safe_cast<Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <Border x:Name='border'>"
            L"        <Button x:Name='button'/>"
            L"    </Border>"
            L"</Grid>"));

        TestServices::WindowHelper->WindowContent = rootGrid;
        VERIFY_IS_FALSE(TestServices::WindowHelper->FrameworkElement_HasPeer(rootGrid, L"border"));
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Get button peer and check that the parent border's peer has not been created");
        button = safe_cast<Button^>(rootGrid->FindName(L"button"));
        VERIFY_IS_NOT_NULL(button);
        VERIFY_IS_FALSE(TestServices::WindowHelper->FrameworkElement_HasPeer(rootGrid, L"border"));

        LOG_OUTPUT(L"Check that parent border's peer has been created on demand");
        border = safe_cast<Border^>(rootGrid->FindName(L"border"));
        VERIFY_IS_NOT_NULL(border);
        VERIFY_IS_TRUE(TestServices::WindowHelper->FrameworkElement_HasPeer(rootGrid, L"border"));
    });

    TestServices::WindowHelper->WaitForIdle();
}

} } } } } }
