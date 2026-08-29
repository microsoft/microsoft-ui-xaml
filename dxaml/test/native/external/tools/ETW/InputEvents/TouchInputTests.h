// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace InputEvents {
class TouchInputTests : public WEX::TestClass<TouchInputTests>
{
public:
    BEGIN_TEST_CLASS(TouchInputTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(TappedEvent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates dragging a Rectangle in a Canvas with the left mouse button and raw pointer events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoubleTappedEvent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates dragging a Rectangle in a Canvas with the left mouse button and raw pointer events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RightTappedEvent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates dragging a Rectangle in a Canvas with the left mouse button and raw pointer events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HoldingEvent)
        TEST_METHOD_PROPERTY(L"Description", L"Validates dragging a Rectangle in a Canvas with the left mouse button and raw pointer events.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()
private:
    void SetupElements(_Out_ Microsoft::UI::Xaml::Shapes::Rectangle^* pRect, _Out_ Microsoft::UI::Xaml::Controls::Grid^* pGrid);
};
} } } } } } }
