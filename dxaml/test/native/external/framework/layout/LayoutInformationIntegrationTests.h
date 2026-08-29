// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layout {

    class LayoutInformationIntegrationTests : public WEX::TestClass<LayoutInformationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(LayoutInformationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;27f34780-4ef6-4102-929f-29737dfda1b9")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(VerifyGetLayoutExceptionElement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that GetLayoutExceptionElement returns an element responsible for throwing an exception inside a layout cycle.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLayoutRoundingMargin)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the correct Arrange Size is calculated when using rounded margins.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDirtyPathWhenDirty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CUIElement::Measure and CUIElement::Arrange do not restore the "
                L"OnMeasureDirtyPath or OnArrangeDirtyPath flags after a UIElement that was originally MeasureDirty-and-OnMeasureDirtyPath "
                L"or ArrangeDirty-and-OnArrangeDirtyPath is marked as IsAncestorDirty after calling MeasureInternal or ArrangeInternal.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGetAvailableSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the correct previous available size is returned")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()
    };

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layout

