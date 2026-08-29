// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SampleCustomTypeTests.h"
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"

#include <CustomTypeMetadataProvider.h>
#include <Sample.CustomButton.h>
#include <Sample.CustomPage.xaml.h>
#include <CustomMetadataRegistrar.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Markup;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Sample {

        bool SampleCustomTypeTests::ClassSetup()
        {
            // It's very important to call EnsureInitialized on TestServices
            // from ClassSetup. This method will wait for the window to be
            // activated on launch, which avoids a race condition that will block
            // input from being routed to the app. It will also wait for the
            // debugger to attach when the waitForDebugger runtime parameter is
            // specified.
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool SampleCustomTypeTests::TestSetup()
        {
            // Initialize the framework before anything else. We need to make sure the test
            // is in a good state in case the framework was shut down after the previous test.
            // We pass in our registar to inform the WindowHelper that there is custom metadata
            // that needs to be initialized and subsequently cleared when we call ShutdownXaml.
            // Without the custom metadata provider types would not be activatable from XAML and certain
            // parts of Jupiter would fail to function (Jupiter will sometimes look up
            // properties by name, without this metadata that lookup will fail and create
            // lots of HRESULT spew).
            test_infra::TestServices::WindowHelper->InitializeXaml(
                ref new MetadataProvider(),
                // Note here that this is a templated ref class. Your custom types need to have static methods
                // for registering and clearing dependency properties. These must be named RegisterDependencyProperties
                // and ClearDependencyProperties.
                ref new CustomMetadataRegistrar<Private::Tests::Sample::CustomButton>());

            return true;
        }

        bool SampleCustomTypeTests::TestCleanup()
        {
            // Shutdown the framework. The purpose of this is to deallocate everything that
            // was allocated during the test and get to a "idle" state. We can then verify test
            // cleanup and check for leaks.
            test_infra::TestServices::WindowHelper->ShutdownXaml();

            // It's very important to have your test clean up the window contents
            // when it completes. When creating new tests be sure to copy this
            // method over or implement it in a similar way. By cleaning
            // up the window content and waiting for the page to go idle you ensure
            // that if your test fails while the UI element tree is being torn down
            // that the failure is associated with your test and doesn't occur
            // nondeterministically in the future. By waiting for the page to go
            // idle you ensure that all transitions have completed and that jupiter
            // is in a 'tabula rasa' state for the next test.
            //
            // Use the TestCleanupWrapper in each test method to handle cleanup, even
            // in cases of failure or repeated runs. Use VerifyTestCleanup here to
            // ensure that the test was cleaned up correctly.
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void SampleCustomTypeTests::CreateCustomButtonFromCodeBehind()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]() {
                auto customButton = ref new Private::Tests::Sample::CustomButton();
            });
        }

        void SampleCustomTypeTests::CreateCustomButtonFromXaml()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]() {
                auto button = static_cast<Private::Tests::Sample::CustomButton^>(XamlReader::Load(
                    L"<local:CustomButton"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Private.Tests.Sample' />"));
                VERIFY_IS_NOT_NULL(button);
            });
        }

        void SampleCustomTypeTests::CreateCustomPage()
        {
            RunOnUIThread([]() {
                auto customPage = ref new Private::Tests::Sample::CustomPage();
            });
        }

    }
} } } }
