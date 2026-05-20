// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Platform;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace GridView {

    class GridViewIntegrationTests : public WEX::TestClass<GridViewIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(GridViewIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"309fb554-f012-4ac9-bcf1-833e4a372493;375cd7bd-e448-4315-b2a1-bc02d75b0c4f")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(AnimateItemIntoViewWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through items with the gamepad left and right buttons.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AnimateGridViewInScrollViewerWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through items, header and footer, nested in ScrollViewer, with the gamepad left and right buttons.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNavigationDoesNotHorizontallyWrapWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through a GridView with the gamepad buttons and validates that we do not wrap around when we reach the horizontal edge of the GridView.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF: Multiple XY focus failures in WPF mode
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNavigationDoesNotVerticallyWrapWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through a GridView with the gamepad buttons and validates that we do not wrap around when we reach the vertical edge of the GridView.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF: Multiple XY focus failures in WPF mode
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BringItemIntoShrunkViewWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through items with the gamepad left and right buttons using a shrunk ApplicationView.VisibleBounds rectangle.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        void ValidateNavigationDoesNotWrapWithGamepad(bool testHorizontalWrapping);

        String^ GetResourcesPath() const;
    };

} } } } } }
