// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class BasicScrollViewerManipulationTests : public WEX::TestClass<BasicScrollViewerManipulationTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicScrollViewerManipulationTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Ignore", L"TRUE") // [DCPP] Multitouch gestures are broken

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(BasicsExWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a ScrollViewer and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BasicsEx2WUC)
                TEST_METHOD_PROPERTY(L"Description", L"Slight variation of BasicsEx, adds a RenderTransform to the primary content")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PanNoInertiaExWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ScrollViewer panning without inertia via DirectManipulation with DManip-on-DComp enabled.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PanNoInertiaOptWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Validates DManip expression optimization while panning")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PanInertiaExWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ScrollViewer panning with inertia via DirectManipulation with DManip-on-DComp enabled.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ZoomInNoInertiaExWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ScrollViewer zooming without inertia via DirectManipulation with DManip-on-DComp enabled.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ZoomOutInertiaExWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ScrollViewer zooming with inertia via DirectManipulation with DManip-on-DComp enabled.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AlignContentWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Validates layout of small ScrollViewer content with various alignments, with DManip-on-DComp enabled.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NoHitTestableContentInRSV)
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NoHitTestableContentInSV)
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            xaml_controls::ScrollViewer^ SetupUI(
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& viewChangedEvent,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanging)& viewChangingRegistration,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanged)& viewChangedRegistration,
                bool withContentSmallerThanViewport = false, bool withInertia = false);


            void BasicsExInternal(bool addRenderTransform = false, bool changeView = false);
            void PanNoInertiaExInternal();
            void PanInertiaExInternal();
            void ZoomInNoInertiaExInternal();
            void ZoomOutInertiaExInternal();
            void AlignContentInternal();
        };

    } } }
} } } }

