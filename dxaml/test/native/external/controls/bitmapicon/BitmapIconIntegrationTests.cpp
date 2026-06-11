// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BitmapIconIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace BitmapIcon {

    bool BitmapIconIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool BitmapIconIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool BitmapIconIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void BitmapIconIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::BitmapIcon>::CanInstantiate();
    }

    void BitmapIconIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::BitmapIcon>::CanEnterAndLeaveLiveTree();
    }

    void BitmapIconIntegrationTests::CanSetAndGetProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto bitmapIcon = ref new xaml_controls::BitmapIcon();

            // Verify default values for BitmapIcon properties.
            VERIFY_IS_NULL(bitmapIcon->UriSource);

            // Verify that we can set and get BitmapIcon properties.
            auto uri = ref new wf::Uri("ms-appx:///foo");
            bitmapIcon->UriSource = uri;
            VERIFY_IS_TRUE(bitmapIcon->UriSource->Equals(uri));

            auto foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            bitmapIcon->Foreground = foreground;
            VERIFY_ARE_EQUAL(bitmapIcon->Foreground, foreground);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::BitmapIcon
