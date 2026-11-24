// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Hub {

    // Validates HubSection focus transfer (Hub.DefaultSectionIndex and SemanticZoom zoom-in) and the
    // HubSectionAutomationPeer ScrollItem pattern.
    class HubSectionFocusAndAutomationTests : public WEX::TestClass<HubSectionFocusAndAutomationTests>
    {
    public:
        BEGIN_TEST_CLASS(HubSectionFocusAndAutomationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(DefaultSectionIndexFocusesSectionHeader)
            TEST_METHOD_PROPERTY(L"Description", L"Hub.DefaultSectionIndex scrolls to the section and moves focus into its header area once loaded (outside a SemanticZoom the header button is not a tab stop, so the See more button is focused).")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DefaultSectionWithoutHeaderFocusesSection)
            TEST_METHOD_PROPERTY(L"Description", L"When the default section has no header, the focusable HubSection itself receives focus.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SemanticZoomInFocusesDestinationSection)
            TEST_METHOD_PROPERTY(L"Description", L"Zooming into a Hub from its SemanticZoom zoomed-out view (programmatic view toggle) moves focus into the destination section's header.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HubSectionAutomationPeerScrollItemPattern)
            TEST_METHOD_PROPERTY(L"Description", L"HubSectionAutomationPeer reports its class name, exposes only the ScrollItem pattern, and ScrollIntoView brings the section into view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HubSectionDependencyPropertyIdentifiersMatchProperties)
            TEST_METHOD_PROPERTY(L"Description", L"The static HubSection dependency property identifiers read and write the corresponding instance properties.")
        END_TEST_METHOD()

    private:
        static xaml_controls::Hub^ LoadHub(Platform::String^ xamlString, _Out_ xaml::FrameworkElement^* root);
        static bool IsSectionInView(xaml_controls::Hub^ hub, xaml_controls::HubSection^ section);
    };

} } } } } }
