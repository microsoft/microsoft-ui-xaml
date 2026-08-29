// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CustomMetadataRegistrar.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Grid {

    class GridIntegrationTests : public WEX::TestClass<GridIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(GridIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanPerformLayout)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the panel can perform layout correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMinWidthAndMinHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully prioritize auto sized cells over star sized cells.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMaxWidthAndMaxHeight)
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully prioritize auto sized cells over star sized cells.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGridLengthForRowsAndColumns)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that cells based on all the different possible combinations of GridLengths for rows and columns are sized correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPixelTakesPriorityOverStar)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully prioritize pixel sized cells over star sized cells.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutoTakesPriorityOverStar)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully prioritize auto sized cells over star sized cells.")
        END_TEST_METHOD()

        // Pixel Sized Cell Tests
        BEGIN_TEST_METHOD(ThrowsInvalidArgumentForNegativePixelSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that negative pixel sizing throws and invalid argument exception.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetCellDimensionsToZero)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can set the pixel size of a row or column to zero.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetCellPixelDimensions)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set a non-zero pixel size for a row or column.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLayoutRoundingForPixelDimensions)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that layout rounding gets applied to pixel dimensions in rows or columns.")
        END_TEST_METHOD()

        // Star Sized Cell Tests
        BEGIN_TEST_METHOD(CanStarSizedCellsEquallyDivideAllocatedSpace)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that star sizing equally divides allocated space.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNegativelyWeightedCellsThrowInvalidArgument)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the star weighting of a row or column throws an invalid argument exception.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanZeroWeightedCellsShrinkToZeroSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully assign a star sized cell with a zero weighting.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDivideAllocatedSpacingCorrectlyAccordingToStarWeightings)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully assign star sized cells their comparative weights.")
        END_TEST_METHOD()

        // Auto Sized Cell Tests
        BEGIN_TEST_METHOD(CanAutoSizeCellDefaultToZero)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that auto sized cells with auto sizing content default to size zero.")
        END_TEST_METHOD()

        // Row and Column Span Tests
        BEGIN_TEST_METHOD(CanLayoutWithColumnSpan)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully layout grids with column spans.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThrowsInvalidArgumentForNegativeRowAndColumnSpan)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we throw invalid argument for negative row and column spans.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThrowsInvalidArgumentForZeroRowAndColumnSpan)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we throw invalid argument for zero row and column spans.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSpanRowsAndColumns)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that rows and columns can successfully span more than one column and row.")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(CanSpanAllRowsAndColumns)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set a column or row to span the entire height or width of the grid.")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(CanSetSpanToMoreThanTotalRowsAndColumns)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that when column or row span is set to more than the total number of columns or rows, that the element will span all available rows or columns.")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(ValidateEnsureMinSizeInDefinitionRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that sizes are distributed appropriately when using spans.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD();

        // Misc. Tests
        BEGIN_TEST_METHOD(BorderChromeForSimpleGrid)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that border chrome can be rendered correctly for grid without row/column definitions.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(BorderChromeForComplexGrid)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that border chrome can be rendered correctly for grid with row/column definitions.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(ValidateSpacing)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Spacing with different GridLengths.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(ValidateSpacingWithSpans)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Spacing with Spans.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(ArrangeOverrideBeforeMeasure)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an app call to ArrangeOverride before Grid has been measured doesn't crash.")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(VerifyRowColumnDefinitionTypeConverter)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a RowDefinition/ColumnDefinition can be constructed from a string denoting Height/Width.")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(ThrowsInvalidArgumentForInvalidRowColumnDefinition)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we throw invalid argument for invalid input to Row/ColumnDefinition type converter.")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(VerifyRowColumnDefinitionContentPropertySyntax)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a RowDefinition/ColumnDefinition can be constructed by assigning Height/Width using content property syntax.")
        END_TEST_METHOD();

    private:
        static const double s_rectSize;
        static const double s_gridSize;
        static const double s_errorMargin; // On phone sometimes the pixel values returned are marginally off. This constant is used as an acceptable margin of error.
    };

} } } } } }

