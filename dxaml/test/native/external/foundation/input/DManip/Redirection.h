// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class Redirection : public WEX::TestClass<Redirection>
        {
        public:
            BEGIN_TEST_CLASS(Redirection)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(Popup)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a ScrollViewer with a Popup child and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PopupPan)
                TEST_METHOD_PROPERTY(L"Description", L"Pans a ScrollViewer with a Popup child and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP Test: Some tests show input differences in RS4 and 19H1
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PopupZoom)
                TEST_METHOD_PROPERTY(L"Description", L"Zooms a ScrollViewer with a Popup child and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP Test: Some tests show input differences in RS4 and 19H1
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NestedPopup)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a ScrollViewer with two nested Popups and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Test crash - Failed to assign Popup.Child
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NestedPopupPan)
                TEST_METHOD_PROPERTY(L"Description", L"Pans a ScrollViewer with two nested Popups and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Test crash - Failed to assign Popup.Child
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP Test: Some tests show input differences in RS4 and 19H1
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RegressionTest_3271313)
                TEST_METHOD_PROPERTY(L"Description", L"Validates pixel snapping behavior within scrolled Popup at high DPI")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;

            xaml_controls::ScrollViewer^ SetupUI(
                _In_ Platform::String^ filename,
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& viewChangedEvent,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanging)& viewChangingRegistration,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanged)& viewChangedRegistration);

            void RemovePopup();

        };


    } } }
} } } }

