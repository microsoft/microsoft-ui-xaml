// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Window {

    class WindowIntegrationTests : public WEX::TestClass<WindowIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(WindowIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanHideAndShowWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can hide and show the window.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMoveWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can move the window.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore, WindowsCore") // This scenario is not supported on OneCore. Excluded on WindowsCore due to Bug TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetSetTitle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get and set the window title.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        BEGIN_TEST_METHOD(SetTitleInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that window title set in markup is correctly handled.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetSystemBackdropInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that window SystemBackdrop set in markup is correctly handled.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

    };

} } } } } }
