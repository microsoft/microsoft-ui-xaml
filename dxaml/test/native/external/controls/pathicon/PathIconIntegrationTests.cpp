// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PathIconIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace PathIcon {

    bool PathIconIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PathIconIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PathIconIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void PathIconIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::PathIcon>::CanInstantiate();
    }

    void PathIconIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::PathIcon>::CanEnterAndLeaveLiveTree();
    }

    void PathIconIntegrationTests::CanSetAndGetProperties()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]
        {
            auto pathIcon = ref new xaml_controls::PathIcon();

            // Verify default values for PathIcon properties.
            VERIFY_IS_NULL(pathIcon->Data);

            // Verify default values for PathIcon properties.
            auto data = ref new xaml_media::PathGeometry();
            pathIcon->Data = data;
            VERIFY_IS_TRUE(pathIcon->Data->Equals(data));

            auto foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            pathIcon->Foreground = foreground;
            VERIFY_ARE_EQUAL(pathIcon->Foreground, foreground);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::PathIcon
