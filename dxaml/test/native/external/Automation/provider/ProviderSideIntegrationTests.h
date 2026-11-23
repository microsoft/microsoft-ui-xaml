// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Provider {

    class ProviderSideIntegrationTests : public WEX::TestClass<ProviderSideIntegrationTests>
    {

    public:
        BEGIN_TEST_CLASS(ProviderSideIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dd3493a5-54ef-4337-93bc-89b726406385")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyLocalizedControlType)
            TEST_METHOD_PROPERTY(L"Description", L"Validates LocalizedControl type returned by XAML for a particular ControlType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBaseAutomationPeerDefaultCoreValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the default values a peer deriving directly from AutomationPeer reports when it overrides no Core method.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDefaultNavigateCoreWithCustomChildren)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the default NavigateCore implementation for first/last child, siblings, parent and an unsupported direction.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLabeledByControlledPeersAndShowContextMenuThroughUia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that UIA clients see LabeledBy, ControllerFor and can call ShowContextMenu on a XAML element.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDockGridAndTableProvidersThroughUia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that app-implemented Dock, Grid, GridItem and Table providers are marshalled to UIA clients.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMultipleViewTransformAndInputProvidersThroughUia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that app-implemented MultipleView, Transform, Transform2, VirtualizedItem and SynchronizedInput providers are marshalled to UIA clients.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyWindowProviderThroughUia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an app-implemented Window provider is marshalled to UIA clients.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTextChildAnnotationAndTextProvidersThroughUia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that app-implemented Text, Text2, TextChild and Annotation providers are marshalled to UIA clients.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySpreadsheetStylesObjectModelAndDragProvidersThroughUia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that app-implemented Spreadsheet, SpreadsheetItem, Styles, ObjectModel and Drag providers are marshalled to UIA clients.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarDatePickerAutomationPeerPublicConstructor)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the public CalendarDatePickerAutomationPeer constructor.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLocalizedControlTypeForFlipViewAndInvalidType)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the localized control type for FlipView and that an unknown control type is rejected.")
        END_TEST_METHOD()
    };

} } } } } }
