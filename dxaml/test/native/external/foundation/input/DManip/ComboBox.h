// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class ComboBoxTest : public WEX::TestClass<ComboBoxTest>
        {
        public:
            BEGIN_TEST_CLASS(ComboBoxTest)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_CLASS_PROPERTY(L"Ignore", L"True") // Disabled due to test instability.
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(Basics)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a ComboBox, pops it open, and validates the DComp tree with DManip-on-DComp enabled")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PanWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a ComboBox, pops it open, pans it, and validates the DComp tree with DManip-on-DComp enabled")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;

            void PanInternal();

            xaml_controls::ComboBox^ SetupUI(_In_ Platform::String^ filename);

            xaml_controls::ScrollViewer^ SetupScrollViewer(
                _In_ xaml_controls::ComboBox^ comboBox,
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& viewChangedEvent,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanging)& viewChangingRegistration,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanged)& viewChangedRegistration);

        };


    } } }
} } } }
