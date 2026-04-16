// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "Hub.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <TreeHelper.h>
#include <TraceConsumerSession.h>
#include <MUX-ETWEvents.h>

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace MockDComp;
using namespace test_infra;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        bool HubTest::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            TestServices::WindowHelper->InjectMockDComp();
            return true;
        }

        bool HubTest::ClassCleanup()
        {
            TestServices::WindowHelper->DetachMockDComp();
            return true;
        }

        bool HubTest::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        xaml_controls::Hub^ HubTest::SetupUI()
        {
            Microsoft::UI::Xaml::Controls::Hub^ hubControl = nullptr;
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SetupHubTest()");
                auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                    L"<Grid "
                    L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"  x:Name='LayoutRoot' Background='#000000' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' > "
                    L"      <Hub x:Name='hubContainer' Header='HubTest' ManipulationMode='All'> "
                    L"          <HubSection x:Name='item1' Header='Section 1' > "
                    L"              <DataTemplate> "
                    L"                  <Grid> "
                    L"                      <Rectangle Height='50' Fill='Red' VerticalAlignment='Top' /> "
                    L"                  </Grid> "
                    L"              </DataTemplate> "
                    L"          </HubSection> "
                    L"          <HubSection x:Name='item2' Header='Section 2'> "
                    L"              <DataTemplate> "
                    L"                  <Grid> "
                    L"                      <Rectangle Height='50' Fill='Yellow' VerticalAlignment='Top' /> "
                    L"                  </Grid> "
                    L"              </DataTemplate> "
                    L"          </HubSection> "
                    L"      </Hub> "
                    L"</Grid>"));

                hubControl = safe_cast<Microsoft::UI::Xaml::Controls::Hub^>(rootPanel->FindName(L"hubContainer"));
                VERIFY_IS_NOT_NULL(hubControl);

                loadedRegistration.Attach(
                    rootPanel,
                    ref new RoutedEventHandler([loadedEvent](Platform::Object^ sender, RoutedEventArgs^)
                {
                    loadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            loadedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            return hubControl;
        }

        void HubTest::Basics()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            // This test is basically just a regression test from Threshold:
            // When creating a Hub, a secondary content relationship curve was accidentally pushing a transform
            // into a Dependency Property because the DManip-on-DComp flag is initially set to FALSE.
            // This caused the transform to be "double-counted" making the hub section move to the wrong location.
            // All this test does is verify that we never accidentally push any values into such DPs when
            // DManip-on-DComp is turned on.
            TraceConsumerSession session(WINDOWS_UI_XAML_ETW_PROVIDER);
            TraceConsumer::EnableTracingByEventId(UpdateDependencyPropertiesForSCRInfo_value);

            auto hubControl = SetupUI();

            session.Stop();
            VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateDependencyPropertiesForSCRInfo_value, 0));
        }

    } } }
} } } }
