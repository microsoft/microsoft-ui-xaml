// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class SyncTest : public WEX::TestClass<SyncTest>
        {
        public:
            BEGIN_TEST_CLASS(SyncTest)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(SyncRectangle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that calling ChangeView at the end of a manipulation properly synchronizes DManip transform")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;

            xaml_controls::ScrollViewer^ SetupUI(
                _In_ Platform::String^ filename,
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& viewChangedEvent,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanging)& viewChangingRegistration,
                _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanged)& viewChangedRegistration);
        };


    } } }
} } } }
