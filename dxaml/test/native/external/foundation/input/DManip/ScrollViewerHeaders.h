// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class ScrollViewerHeaders : public WEX::TestClass<ScrollViewerHeaders>
        {
        public:
            BEGIN_TEST_CLASS(ScrollViewerHeaders)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(HeadersPan)
                TEST_METHOD_PROPERTY(L"Description", L"Pans a ScrollViewer with Headers and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Ignore", L"True") //DCPP Test: InputManagerXaml.dll InjectPressAndDrag does not work correctly on 64 bit OS
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HeadersOverpanWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Overpans a ScrollViewer with Headers and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HeadersZoom)
                TEST_METHOD_PROPERTY(L"Description", L"Zooms a ScrollViewer with Headers and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"True") // DCPP Test: InputManagerXaml.dll InjectPressAndDrag does not work correctly on 64 bit OS
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeViewWithAnimation)
                TEST_METHOD_PROPERTY(L"Description", L"Does a ChangeView on a ScrollViewer with Headers and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            // -------Begin WUC versions---------

            BEGIN_TEST_METHOD(HeadersWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a ScrollViewer with Headers and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HeadersFractionalZoomWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Zooms a ScrollViewer with Headers using a fractional zoom factor and validates the DComp tree")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeViewWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Does a ChangeView on a ScrollViewer with Headers and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            void HeadersInternal();
            void HeadersPanInternal();
            void HeadersOverpanInternal();
            void HeadersZoomInternal();
            void HeadersFractionalZoomInternal();
            void ChangeViewInternal();
            void ChangeViewWithAnimationInternal();

            xaml_controls::ScrollViewer^ SetupUI(
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& viewChangingEvent,
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& viewChangedEvent,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanging)& viewChangingRegistration,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanged)& viewChangedRegistration);

        };


    } } }
} } } }

