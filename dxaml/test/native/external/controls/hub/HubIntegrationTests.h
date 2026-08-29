// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Controls { namespace Hub {

        class HubIntegrationTests : public WEX::TestClass<HubIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(HubIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"d96ffe50-c538-42ef-918b-517ad455f1e0;70a0f79e-de5f-46d3-bd45-a84dcb6e99df;b34da8d2-333d-40a9-a19c-94b1f9785580")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CanInstantiate)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Hub.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a Hub from the live tree.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCanGetSetProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can set and get properties defined on Hub/HubSection")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanAddRemoveSections)
                TEST_METHOD_PROPERTY(L"Description", L"Tests adding and removing HubSections from the Hub")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetHubSectionVisibility)
                TEST_METHOD_PROPERTY(L"Description", L"Tests setting Visibility on HubSections in the Hub")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HubZeroItems)
                TEST_METHOD_PROPERTY(L"Description", L"Loads xaml with a Hub without any HubSections.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HubOneSection)
                TEST_METHOD_PROPERTY(L"Description", L"Valdiates Hub with only one HubSection.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HubDefaultValues)
                TEST_METHOD_PROPERTY(L"Description", L"Valdiates the default property values.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HubDefaultLayout)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the layout of hub sections and heights of the header and title.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HubSectionsInViewChanged)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the HubSections changes.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HubSectionHeaderClick)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the Hub SectionHeaderClick event.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateSemanticZoomedOutView)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the Hub semantic zoomed out view.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateUIElementTree)
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
                TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of Hub which does not have a SemanticZoom as its parent")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateUIETInSemanticZoom)
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
                TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of Hub which has a SemanticZoom as its parent")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HubSectionsEnteredOnlyOnce)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateSectionHeadersBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the SectionHeaders binding with other ItemsControl")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateSeeMoreButton)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the SeeMore button with the header template")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateFootprint)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of Hub.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NavigateThroughHubSectionsUsingKeyboard)
                TEST_METHOD_PROPERTY(L"Description", L"In this test we set the hubsections of the hub to IsTabStop=true. When this is done, the left and right keys scroll in small steps, and the page up and page down button jumps between sections.")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // DCPP: RS4 Test failure: 4 tests disabled due to Scrollviewer issues
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NavigateThroughHubSectionsWithHeaderButtonsUsingKeyboard)
                TEST_METHOD_PROPERTY(L"Description", L"In this test we set the hubsections of the hub to IsTabStop=false. However we define the hubsection's headers as buttons. When this is done, the left and right keys jump between sections headers, and the page up and page down button also jumps between headers.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTextProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the text properties of Hub/HubSection header.")
            END_TEST_METHOD()

        private:
            xaml_controls::Hub^ SetupHubTest();
            xaml_controls::Hub^ SetupHubSeeMoreTest();
            xaml_controls::SemanticZoom^ SetupHubSemanticZoomedTest();
            xaml_controls::Hub^ SetupHubWithHeaderButtonsTest();

            void HubIntegrationTests::DoSectionChange(
                xaml_controls::Hub^ hubControl,
                xaml_controls::ScrollViewer^ scrollViewer,
                Platform::String^ sectionName,
                test_infra::FlickDirection direction);

            void ValidateHubOnFirstSection(xaml_controls::ScrollViewer^ scrollViewer);

            void ValidateHubOnLastSection(xaml_controls::ScrollViewer^ scrollViewer);

            xaml_controls::HubSection^ CreateHubSection(Platform::String^ name, Platform::Object^ content = nullptr, Platform::Object^ header = nullptr, double width = 100);

            static xaml_controls::Panel^ SetupValidateUIElementTreeTest(xaml_controls::Orientation orientation);
        };

    } } 
} } } }

