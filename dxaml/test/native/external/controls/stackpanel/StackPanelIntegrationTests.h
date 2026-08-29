// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace StackPanel {

    class StackPanelIntegrationTests : public WEX::TestClass<StackPanelIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(StackPanelIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")


            TEST_CLASS_PROPERTY(L"Classification", L"Integration")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanStackItemsHorizontally)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StackPanels can stack items in a horizontal orientation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStackItemsVertically)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StackPanels can stack items in a vertical orientation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStackVariableSizedItemsHorizontally)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StackPanels can stack variable sized items in a horizontal orientation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStackVariableSizedItemsVertically)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StackPanels can stack variable sized items in a vertical orientation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeOrientation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StackPanels can change orientation. Includes visual validation.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDesiredSize_AutoLayout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StackPanels report the correct desired size when using auto layout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDesiredSize_MinWidthHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StackPanels report the correct desired size when using MinWidth/MinHeight.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDesiredSize_MaxWidthHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that StackPanels report the correct desired size when using MaxWidth/MaxHeight.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSpacing)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Spacing with different orientations.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBorderChrome)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that border properties draw correctly for StackPanel")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySnapPoints)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the calculation and event triggering of Snap Points in an Horizontal and Vertical StackPanel.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyContentClipping)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a clipped item that has it's position shifted by the insert of another still clips correctly.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateBoundingRectangle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the bounds of a StackPanel")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //
        BEGIN_TEST_METHOD(ValidateReorderListViewWithStackPanel)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ListView with a StackPanel supports reodering")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()


    private:
        static const int s_itemCount = 3;

        xaml_shapes::Rectangle^ CreateRectangle(int width, int height, bool visible = true);
        xaml_controls::StackPanel^ VerifyDesiredSize_Setup();
        void VerifySnapPointsOrientation(xaml_controls::Orientation orientation, int elementWidth, int elementHeight);

    };

} } } } } }

