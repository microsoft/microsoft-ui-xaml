// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class RenderNothingTests : public WEX::TestClass<RenderNothingTests>
{
public:
    BEGIN_TEST_CLASS(RenderNothingTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"Description", L"Renders nothing.")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(RenderNoContent)
        // This test sets WindowHelper->WindowContent to null. With the WPFTestWindow, setting the content to null will
        // call Close on the underlying DesktopWindowXamlSource, which will call Close on the DesktopWindowBridge,
        // which destroys its internal hwnd. All this happens before Xaml renders its first frame, so when Xaml ticks
        // again and initializes DManip, it will try to reuse the destroyed hwnd and hit an error. Limit this test to
        // UWP only.
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderNoVisualTree)
        // We can't recover from resetting the visual tree, so run this test in its own process.
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
    END_TEST_METHOD()
};

} } } } } }
