// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Hosting {

//
// This test class is meant to test scenarios where a XamlIsland is hosted without a DesktopSiteBridge,
// so there is no backing HWND.  It's how React Native Windows hosts Xaml content, for example.
//
// Similar to XamlIslandTests, each test sets up its own window and scene graph, it does not use the
// common infrastructure provided by TestHelper, etc.
//
class WindowlessXamlIslandTests : public WEX::TestClass<WindowlessXamlIslandTests>
{
public:
    BEGIN_TEST_CLASS(WindowlessXamlIslandTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        // Even though we're not a UAP, this is needed to load an AppxManifest and Xaml types. Without it, DWXS gives
        // "class not registered" errors.
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"Description", L"Various Xaml island configurations.")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
        // When running this test, make sure to run with -hostingMode set to "Win32Explicit"
        TEST_CLASS_PROPERTY(L"Hosting:Mode", L"Win32Explicit")
    END_TEST_CLASS()

    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(ValidateUiaTree);

    TEST_METHOD(ValidateDatePickerTimePickerDontCrash);

    TEST_METHOD(ValidateWebView2DoesntCrash);

    BEGIN_TEST_METHOD(ValidateTabNavigation)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")  // Not working yet
    END_TEST_METHOD()
};

} } } } } }
