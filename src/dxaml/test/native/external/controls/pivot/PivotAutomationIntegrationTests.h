// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Controls { namespace Pivot {

        class PivotAutomationIntegrationTests : public WEX::TestClass<PivotAutomationIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(PivotAutomationIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") // Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP") // Pivot dtor calling on wrong thread during GC causes crash on shutdown in WPF host mode
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(VerifyAutomationProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Verify AutomationProperties for Pivot.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyOverriddenValuesForPivotItems)
                TEST_METHOD_PROPERTY(L"Description", L"Verify the values for automation properties that have been overridden on PivotItemAutomationPeer.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFocusChangesOnSelectedItemChange)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that UI automation focus change events are raised when the selected item in Pivot changes.")
                // TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyHitTestOnPivotItems)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that UI automation hit-testing finds the expected pivot item beneath the point.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Re-enable
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyPivotItemsReportCorrectPositionAndItemCount)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that directly accessing a PivotItem still allows you to retrieve its position and item count information.")
                // TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBindingPivotSelectedItemToInterfacePreservesSelectionAutomationProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that binding a Pivot's SelectedItem to an interface type doesn't cause IsSelected to always return false.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyScanModeNavigation)
                TEST_METHOD_PROPERTY(L"Description", L"Verify narrator can navigate to previous pivotItem in scan mode. On RS4 and RS5, selected pivot doesn't change when navigating via narrator in scan mode. No repro on RS3.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

        private:
            xaml_controls::Pivot^ SetupPivotAutomationTest();
            xaml::FrameworkElement^ GetPivotHeader(xaml_controls::Pivot^ pivot, int index);

        };

    } }
} } } }
