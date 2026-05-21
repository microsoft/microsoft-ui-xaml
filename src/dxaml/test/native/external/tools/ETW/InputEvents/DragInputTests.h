// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace InputEvents {

class DragInputTests : public WEX::TestClass<DragInputTests>
{
public:
    BEGIN_TEST_CLASS(DragInputTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(DragEvents)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"Description", L"Validates dragging a Rectangle in a Canvas with the left mouse button and raw pointer events.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()
};
} } } } } } }
