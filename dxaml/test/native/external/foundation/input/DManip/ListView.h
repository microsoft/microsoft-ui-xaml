// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <collection.h>
#include <RuntimeEnabledFeatureOverride.h>

using namespace Platform;
using namespace Platform::Collections;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class ListViewTest : public WEX::TestClass<ListViewTest>
        {
        public:
            BEGIN_TEST_CLASS(ListViewTest)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(Basics)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a ListView and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Pan)
                TEST_METHOD_PROPERTY(L"Description", L"Pans a ListView and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Poisons next test with crash in MockDComp
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Zoom)
                TEST_METHOD_PROPERTY(L"Description", L"Zooms a ListView and validates the DComp tree with DManip-on-DComp enabled")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // DManip transform off by 1 pixel
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP Test: Some tests show input differences in RS4 and 19H1
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ZoomOutWithMouseWheel)
                TEST_METHOD_PROPERTY(L"Description", L"Zooms out a ListView with an inner VirtualizingStackPanel using the ctrl key and mouse wheel")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Intermittent MockDComp crash
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ZoomInWithMouseWheel)
                TEST_METHOD_PROPERTY(L"Description", L"Zooms in a ListView with an inner VirtualizingStackPanel using the ctrl key and mouse wheel")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Intermittent MockDComp crash
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateEndOfInertiaView)
                TEST_METHOD_PROPERTY(L"Description", L"Zooms and pans a ListView of variable-sized items and validates the ViewChanging's FinalView property after inertia")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Intermittent MockDComp crash
            END_TEST_METHOD()

        private:
            String^ GetResourcesPath() const;

            xaml_controls::ScrollViewer^ SetupUI(
                _In_ Platform::String^ filename,
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& viewChangedEvent,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanging)& viewChangingRegistration,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanged)& viewChangedRegistration);

            void ZoomWithMouseWheel(bool zoomIn);

        };

    } } }
} } } }

